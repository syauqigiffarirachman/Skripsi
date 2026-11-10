/* Tanda tangan digital dengan DS28E38 lewat USB (DS9481P-300) - versi C.
 * Setara dengan sign.py (format berkas JSON sama, bisa saling diverifikasi).
 *
 *   sign info                                  ID chip + kunci publik
 *   sign sign "pesan"                          tandatangani teks   -> tanda_tangan.json
 *   sign sign --file dokumen.pdf               tandatangani file
 *   sign verify tanda_tangan.json ["pesan"]    periksa tanda tangan (tanpa chip)
 *
 *   Opsi: --port COM13  --debug  --page 0..5  --anon  --out nama.json
 *         --file berkas  --pub X_hex,Y_hex (verify)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <shellapi.h>

#include "ds28e38.h"
#include "p256.h"
#include "sha256.h"

typedef struct {
    uint8_t msg_hash[32], challenge[32], page_data[32];
    uint8_t rom[8], manid[2];
    uint8_t pub_x[32], pub_y[32], r[32], s[32];
    int page, anonymous;
} record_t;

typedef struct {
    const char *port, *out, *file, *pub, *positional[2];
    int npos, debug, page, anon;
} args_t;

/* ------------------------------------------------------------------ helpers */
static void print_hex(const uint8_t *b, size_t n, int upper)
{
    size_t i;
    for (i = 0; i < n; i++)
        printf(upper ? "%02X" : "%02x", b[i]);
}

static void hex_str(char *out, const uint8_t *b, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
        sprintf(out + 2 * i, "%02x", b[i]);
}

/* decode exactly `want` bytes from hex (optional 0x prefix, stops at ',' or end); 0 = ok */
static int hex_decode(const char *s, uint8_t *out, size_t want)
{
    size_t i;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    for (i = 0; i < want; i++) {
        int hi, lo;
        char a = s[2 * i], b = s[2 * i + 1];
        if (!a || !b)
            return -1;
        hi = (a >= '0' && a <= '9') ? a - '0' : ((a | 0x20) >= 'a' && (a | 0x20) <= 'f') ? (a | 0x20) - 'a' + 10 : -1;
        lo = (b >= '0' && b <= '9') ? b - '0' : ((b | 0x20) >= 'a' && (b | 0x20) <= 'f') ? (b | 0x20) - 'a' + 10 : -1;
        if (hi < 0 || lo < 0)
            return -1;
        out[i] = (uint8_t)(hi * 16 + lo);
    }
    return 0;
}

static uint8_t *read_file(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    uint8_t *buf;
    long sz;
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (uint8_t *)malloc((size_t)sz + 1);
    if (!buf || fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fclose(f);
        free(buf);
        return NULL;
    }
    fclose(f);
    buf[sz] = 0;
    *n = (size_t)sz;
    return buf;
}

/* --- tiny JSON reader for flat {"key": value} objects (same file as sign.py) --- */
static const char *json_find(const char *buf, const char *key)
{
    char pat[48];
    const char *p;
    snprintf(pat, sizeof pat, "\"%s\"", key);
    p = strstr(buf, pat);
    if (!p)
        return NULL;
    p += strlen(pat);
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;
    if (*p != ':')
        return NULL;
    p++;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;
    return p;
}

static int json_hex(const char *buf, const char *key, uint8_t *out, size_t nbytes)
{
    const char *p = json_find(buf, key);
    if (!p || *p != '"')
        return -1;
    return hex_decode(p + 1, out, nbytes);
}

static int json_int(const char *buf, const char *key, int *out)
{
    const char *p = json_find(buf, key);
    if (!p)
        return -1;
    *out = atoi(p);
    return 0;
}

static int json_bool(const char *buf, const char *key, int *out)
{
    const char *p = json_find(buf, key);
    if (!p)
        return -1;
    *out = strncmp(p, "true", 4) == 0;
    return 0;
}

/* ------------------------------------------------------------------ record */
static int record_check(const record_t *rec)
{
    uint8_t msg[8 + 32 + 32 + 1 + 2], digest[32];
    size_t n = 0;

    /* chip signs SHA-256( ROM | page data | challenge | page# | MANID ) */
    if (rec->anonymous)
        memset(msg, 0xFF, 8);
    else
        memcpy(msg, rec->rom, 8);
    n += 8;
    memcpy(msg + n, rec->page_data, 32); n += 32;
    memcpy(msg + n, rec->challenge, 32); n += 32;
    msg[n++] = (uint8_t)rec->page;
    memcpy(msg + n, rec->manid, 2); n += 2;
    sha256(msg, n, digest);
    return p256_ecdsa_verify(rec->pub_x, rec->pub_y, digest, rec->r, rec->s);
}

static int record_save(const char *path, const record_t *rec)
{
    FILE *f = fopen(path, "wb");
    char h[65];
    if (!f)
        return -1;
    fprintf(f, "{\n  \"format\": \"ds28e38-ecdsa-p256-v1\",\n");
    hex_str(h, rec->msg_hash, 32);  fprintf(f, "  \"message_sha256\": \"%s\",\n", h);
    hex_str(h, rec->challenge, 32); fprintf(f, "  \"challenge\": \"%s\",\n", h);
    fprintf(f, "  \"page\": %d,\n", rec->page);
    hex_str(h, rec->page_data, 32); fprintf(f, "  \"page_data\": \"%s\",\n", h);
    fprintf(f, "  \"anonymous\": %s,\n", rec->anonymous ? "true" : "false");
    hex_str(h, rec->rom, 8);        fprintf(f, "  \"rom\": \"%s\",\n", h);
    hex_str(h, rec->manid, 2);      fprintf(f, "  \"manid\": \"%s\",\n", h);
    hex_str(h, rec->pub_x, 32);     fprintf(f, "  \"pub_x\": \"%s\",\n", h);
    hex_str(h, rec->pub_y, 32);     fprintf(f, "  \"pub_y\": \"%s\",\n", h);
    hex_str(h, rec->r, 32);         fprintf(f, "  \"r\": \"%s\",\n", h);
    hex_str(h, rec->s, 32);         fprintf(f, "  \"s\": \"%s\",\n", h);
    {
        char rs[129];
        hex_str(rs, rec->r, 32);
        hex_str(rs + 64, rec->s, 32);
        fprintf(f, "  \"signature_rs\": \"%s\"\n}\n", rs);
    }
    fclose(f);
    return 0;
}

static int record_load(const char *path, record_t *rec)
{
    size_t n;
    char *buf = (char *)read_file(path, &n);
    int ok;
    if (!buf)
        return -1;
    memset(rec, 0, sizeof *rec);
    ok = json_hex(buf, "message_sha256", rec->msg_hash, 32) == 0 &&
         json_hex(buf, "challenge", rec->challenge, 32) == 0 &&
         json_int(buf, "page", &rec->page) == 0 &&
         json_hex(buf, "page_data", rec->page_data, 32) == 0 &&
         json_bool(buf, "anonymous", &rec->anonymous) == 0 &&
         json_hex(buf, "rom", rec->rom, 8) == 0 &&
         json_hex(buf, "manid", rec->manid, 2) == 0 &&
         json_hex(buf, "pub_x", rec->pub_x, 32) == 0 &&
         json_hex(buf, "pub_y", rec->pub_y, 32) == 0 &&
         json_hex(buf, "r", rec->r, 32) == 0 &&
         json_hex(buf, "s", rec->s, 32) == 0;
    free(buf);
    return ok ? 0 : -2;
}

/* ------------------------------------------------------------------ chip access */
typedef struct {
    ow_t ow;
    ds28e38_t chip;
    char port[16];
} session_t;

static int open_session(session_t *ss, const args_t *a)
{
    ds28e38_status_t st;
    int rc;

    printf("Menghubungi chip lewat USB ...\n");
    if (ow_open(&ss->ow, a->port, a->debug, ss->port, sizeof ss->port) != 0) {
        fprintf(stderr, "\nGAGAL: adaptor DS9481 tidak ditemukan%s%s\n",
                a->port ? " di " : " (coba --port COMx)", a->port ? a->port : "");
        return -1;
    }
    ds28e38_init(&ss->chip, &ss->ow);
    rc = ds28e38_detect(&ss->chip, &st);
    if (rc != DS_OK) {
        fprintf(stderr, "\nGAGAL: %s\n", ds_strerror(rc));
        ow_close(&ss->ow);
        return -1;
    }
    printf("  adaptor : %s (firmware %d.%d)\n", ss->port, ss->ow.ver.major, ss->ow.ver.minor);
    printf("  ROM ID  : ");
    if (ss->chip.have_rom)
        print_hex(ss->chip.rom, 8, 1);
    else
        printf("(READ ROM gagal)");
    printf("\n  MANID   : ");
    print_hex(ss->chip.manid, 2, 1);
    printf("\n");
    return 0;
}

static int read_public_key(session_t *ss, uint8_t x[32], uint8_t y[32])
{
    int rc = ds28e38_read_memory(&ss->chip, DS_PG_PUB_KEY_X, x);   /* halaman 4 = X */
    if (rc == DS_OK)
        rc = ds28e38_read_memory(&ss->chip, DS_PG_PUB_KEY_Y, y);   /* halaman 5 = Y */
    if (rc != DS_OK)
        fprintf(stderr, "\nGAGAL membaca kunci publik: %s\n", ds_strerror(rc));
    return rc;
}

static int fail_chip(const char *what, int rc, const ds28e38_t *chip)
{
    fprintf(stderr, "\nGAGAL %s: %s", what, ds_strerror(rc));
    if (rc == DS_E_DEVICE)
        fprintf(stderr, " (0x%02X)", chip->last_result);
    fprintf(stderr, "\n");
    return 1;
}

/* ------------------------------------------------------------------ commands */
static uint8_t *get_message(const args_t *a, size_t *n, char *desc, size_t dsize)
{
    if (a->file) {
        uint8_t *b = read_file(a->file, n);
        if (!b) {
            fprintf(stderr, "Tidak bisa membaca file %s\n", a->file);
            return NULL;
        }
        snprintf(desc, dsize, "file %s", a->file);
        return b;
    }
    if (a->npos > 0) {
        const char *m = a->positional[0];
        uint8_t *b = (uint8_t *)malloc(strlen(m) + 1);
        if (!b)
            return NULL;
        memcpy(b, m, strlen(m) + 1);
        *n = strlen(m);
        snprintf(desc, dsize, "teks '%s'", m);
        return b;
    }
    fprintf(stderr, "Tidak ada pesan. Tulis pesannya, mis.:  sign sign \"halo\"   atau  --file namafile\n");
    return NULL;
}

static int cmd_info(const args_t *a)
{
    session_t ss;
    uint8_t x[32], y[32];

    if (open_session(&ss, a) != 0)
        return 1;
    if (read_public_key(&ss, x, y) != DS_OK) {
        ow_close(&ss.ow);
        return 1;
    }
    printf("  Kunci publik X : "); print_hex(x, 32, 1); printf("\n");
    printf("  Kunci publik Y : "); print_hex(y, 32, 1); printf("\n");
    ow_close(&ss.ow);
    return 0;
}

static int cmd_sign(const args_t *a)
{
    session_t ss;
    record_t rec;
    uint8_t *msg;
    size_t mlen = 0;
    char desc[300];
    int rc, ok;
    DWORD t0;

    if (a->page < 0 || a->page > 5) {
        fprintf(stderr, "--page harus 0..5 (halaman 6 menyimpan kunci privat)\n");
        return 1;
    }
    msg = get_message(a, &mlen, desc, sizeof desc);
    if (!msg)
        return 1;
    memset(&rec, 0, sizeof rec);
    sha256(msg, mlen, rec.msg_hash);
    free(msg);
    memcpy(rec.challenge, rec.msg_hash, 32);
    printf("Pesan   : %s (%u byte)\n", desc, (unsigned)mlen);
    printf("SHA-256 : "); print_hex(rec.msg_hash, 32, 0); printf("\n");

    if (open_session(&ss, a) != 0)
        return 1;
    rec.page = a->page;
    rec.anonymous = a->anon;
    if (ss.chip.have_rom)
        memcpy(rec.rom, ss.chip.rom, 8);
    memcpy(rec.manid, ss.chip.manid, 2);

    rc = ds28e38_read_memory(&ss.chip, rec.page, rec.page_data);
    if (rc != DS_OK) {
        ow_close(&ss.ow);
        return fail_chip("membaca halaman", rc, &ss.chip);
    }
    if (read_public_key(&ss, rec.pub_x, rec.pub_y) != DS_OK) {
        ow_close(&ss.ow);
        return 1;
    }
    printf("Meminta chip menandatangani (halaman %d%s) ...\n", rec.page,
           rec.anonymous ? ", mode anonim" : "");
    t0 = GetTickCount();
    rc = ds28e38_compute_read_page_auth(&ss.chip, rec.page, rec.challenge, rec.anonymous, rec.r, rec.s);
    ow_close(&ss.ow);
    if (rc != DS_OK)
        return fail_chip("menandatangani", rc, &ss.chip);
    printf("  selesai dalam %.2f detik\n", (GetTickCount() - t0) / 1000.0);

    printf("\nTanda tangan (r || s, 64 byte):\n  r = "); print_hex(rec.r, 32, 0);
    printf("\n  s = "); print_hex(rec.s, 32, 0); printf("\n");

    ok = record_check(&rec);
    printf("\nVerifikasi di PC dengan kunci publik chip : %s\n", ok ? "VALID" : "TIDAK VALID");
    if (!ok)
        printf("  Tanda tangan diterima dari chip tetapi tidak cocok dengan kunci publik di "
               "halaman 4/5.\n  Kemungkinan: halaman 4/5 bukan pasangan kunci privat chip "
               "(belum di-generate), atau urutan byte berbeda.\n");
    if (record_save(a->out, &rec) != 0) {
        fprintf(stderr, "Tidak bisa menulis %s\n", a->out);
        return 1;
    }
    printf("Disimpan ke %s\n", a->out);
    return ok ? 0 : 3;
}

static int cmd_verify(const args_t *a)
{
    record_t rec;
    int rc, ok;

    if (a->npos < 1) {
        fprintf(stderr, "Pemakaian: sign verify tanda_tangan.json [\"pesan\"] [--file f] [--pub X,Y]\n");
        return 1;
    }
    rc = record_load(a->positional[0], &rec);
    if (rc != 0) {
        fprintf(stderr, "Tidak bisa membaca/menguraikan %s\n", a->positional[0]);
        return 1;
    }
    if (a->npos > 1 || a->file) {
        args_t m = *a;
        uint8_t *msg, h[32];
        size_t n = 0;
        char desc[300];
        m.positional[0] = a->positional[1];
        m.npos = a->npos > 1 ? 1 : 0;
        msg = get_message(&m, &n, desc, sizeof desc);
        if (!msg)
            return 1;
        sha256(msg, n, h);
        free(msg);
        if (memcmp(h, rec.msg_hash, 32) != 0) {
            printf("PESAN TIDAK SAMA dengan yang ditandatangani (%s).\n", desc);
            return 4;
        }
        printf("Pesan cocok dengan yang ditandatangani (%s).\n", desc);
    } else {
        printf("(pesan tidak diberikan: hanya memeriksa tanda tangan atas hash ");
        print_hex(rec.msg_hash, 8, 0);
        printf("...)\n");
    }
    if (a->pub) {
        const char *comma = strchr(a->pub, ',');
        if (!comma || hex_decode(a->pub, rec.pub_x, 32) != 0 || hex_decode(comma + 1, rec.pub_y, 32) != 0) {
            fprintf(stderr, "--pub harus berbentuk X_hex,Y_hex (masing-masing 64 digit hex)\n");
            return 1;
        }
        printf("Memakai kunci publik dari --pub (bukan dari berkas).\n");
    } else {
        printf("PERINGATAN: kunci publik diambil dari berkas itu sendiri. Untuk keamanan nyata,\n"
               "  bandingkan dengan kunci publik tepercaya (--pub X,Y).\n");
    }
    ok = record_check(&rec);
    printf("Tanda tangan : %s\n", ok ? "VALID" : "TIDAK VALID");
    return ok ? 0 : 3;
}

/* ------------------------------------------------------------------ main */
static void usage(void)
{
    printf("Tanda tangan digital dengan DS28E38 lewat USB (DS9481P-300)\n\n"
           "  sign info                                  ID chip + kunci publik\n"
           "  sign sign \"pesan\"                          tandatangani teks (hasil: tanda_tangan.json)\n"
           "  sign sign --file dokumen.pdf               tandatangani file\n"
           "  sign verify tanda_tangan.json [\"pesan\"]    periksa tanda tangan (tanpa chip)\n\n"
           "Opsi: --port COM13  --debug  --page 0..5  --anon  --out nama.json\n"
           "      --file berkas  --pub X_hex,Y_hex  (untuk verify)\n");
}

/* argv in UTF-8 (the C runtime would give the ANSI code page) */
static char **utf8_args(int *argc)
{
    int wc, i;
    wchar_t **w = CommandLineToArgvW(GetCommandLineW(), &wc);
    char **v;
    if (!w)
        return NULL;
    v = (char **)calloc((size_t)wc + 1, sizeof(char *));
    for (i = 0; i < wc; i++) {
        int n = WideCharToMultiByte(CP_UTF8, 0, w[i], -1, NULL, 0, NULL, NULL);
        v[i] = (char *)malloc((size_t)n);
        WideCharToMultiByte(CP_UTF8, 0, w[i], -1, v[i], n, NULL, NULL);
    }
    LocalFree(w);
    *argc = wc;
    return v;
}

int main(void)
{
    int argc, i;
    char **argv = utf8_args(&argc);
    args_t a;
    const char *cmd;

    if (!argv || argc < 2) {
        usage();
        return 1;
    }
    memset(&a, 0, sizeof a);
    a.out = "tanda_tangan.json";
    cmd = argv[1];
    for (i = 2; i < argc; i++) {
        const char *s = argv[i];
        int more = i + 1 < argc;
        if (!strcmp(s, "--port") && more)       a.port = argv[++i];
        else if (!strcmp(s, "--out") && more)   a.out = argv[++i];
        else if (!strcmp(s, "--file") && more)  a.file = argv[++i];
        else if (!strcmp(s, "--pub") && more)   a.pub = argv[++i];
        else if (!strcmp(s, "--page") && more)  a.page = atoi(argv[++i]);
        else if (!strcmp(s, "--anon"))          a.anon = 1;
        else if (!strcmp(s, "--debug"))         a.debug = 1;
        else if (s[0] == '-' && s[1] == '-') {
            fprintf(stderr, "Opsi tidak dikenal: %s\n", s);
            return 1;
        } else if (a.npos < 2)
            a.positional[a.npos++] = s;
    }
    if (!strcmp(cmd, "info"))   return cmd_info(&a);
    if (!strcmp(cmd, "sign"))   return cmd_sign(&a);
    if (!strcmp(cmd, "verify")) return cmd_verify(&a);
    usage();
    return 1;
}
