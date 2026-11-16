// Firmware board 1 (STM32F103C8 + STM32duino bootloader): jembatan USB CDC <-> chip OneWire.
#include <OneWire.h>

// Serial lewat USB CDC (USB-C: PA11/PA12), muncul sebagai port COM di laptop
#define ser SerialUSB

#define PIN_ONEWIRE PA15
#define PIN_RED     PB6
#define PIN_GREEN   PB5
#define PIN_BUSY    PC13      // LED onboard (aktif LOW) menyala selama transaksi OneWire

#define RED_TIME   5000
#define GREEN_TIME 500

#define LINE_MAX    255       // panjang maksimum satu baris perintah
#define PARA_MAX    50        // jumlah maksimum parameter perintah
#define DS_DATA_MAX 256       // ukuran buffer data balasan chip

OneWire ds(PIN_ONEWIRE);
bool auto_mode = false;
unsigned long red_timeout = 0;
unsigned long green_timeout = 0;

// ============================================================
// RAM & LATENCY MONITORING  (set 0 untuk mematikan seluruh pengukuran)
// ============================================================
#define ENABLE_MEASURE 1
// Alat ukur bersama dengan softwaretes: DWT cycle counter, stack paint, SHA-256 host
#include "bench.h"

typedef struct {
  uint32_t total;        // total RAM
  uint32_t statik;       // .data + .bss
  uint32_t heap;         // heap terpakai
  uint32_t stack_now;    // stack saat ini
  uint32_t stack_peak;   // stack puncak (high-water mark)
  uint32_t used;         // statik + heap + stack_now
  uint32_t free_now;     // sisa RAM saat ini
  uint32_t free_min;     // sisa RAM minimum (pada puncak stack)
} RamStats;

// Semua latensi diukur dengan DWT CYCCNT lalu dikonversi ke mikrodetik
typedef struct {
  uint32_t t_parse_us;   // waktu parsing / pembentukan parameter
  uint32_t t_hash_us;    // SHA-256 pesan di host (perintah AM), 0 untuk perintah lain
  uint32_t t_send_us;    // ds_send (termasuk retry)
  uint32_t t_wait_us;    // delay tunggu chip
  uint32_t t_read_us;    // ds_read (termasuk retry)
  uint32_t t_total_us;   // end-to-end (perintah diterima -> hasil siap)
} LatencyStats;

// Cari alamat terendah yang sudah tertimpa stack (hanya dipanggil saat laporan)
static uint32_t stack_peak_bytes() {
  uint32_t top = (uint32_t)&_estack;
  if (!g_paint_lo) return top - get_sp();
  uint32_t *p = g_paint_lo;
  uint32_t *sp = (uint32_t *)get_sp();
  while (p < sp && *p == STACK_PAINT) p++;
  return top - (uint32_t)p;
}

static RamStats measure_ram() {
  RamStats s;
  uint32_t heap_end = (uint32_t)sbrk(0);
  uint32_t sp       = get_sp();
  s.total      = (uint32_t)&_estack - (uint32_t)&_sdata;
  s.statik     = (uint32_t)&_end - (uint32_t)&_sdata;
  s.heap       = heap_end - (uint32_t)&_end;
  s.stack_now  = (uint32_t)&_estack - sp;
  s.stack_peak = stack_peak_bytes();
  if (s.stack_peak < s.stack_now) s.stack_peak = s.stack_now;
  s.used       = s.statik + s.heap + s.stack_now;
  s.free_now   = sp - heap_end;
  s.free_min   = s.total - s.statik - s.heap - s.stack_peak;
  return s;
}

static void print_ram_stats(const char *label, const RamStats &s) {
  ser.printf("[RAM %s] total=%lu static=%lu heap=%lu stack=%lu (peak %lu) "
             "used=%lu (%lu%%) free=%lu (min %lu) bytes\n",
             label, s.total, s.statik, s.heap, s.stack_now, s.stack_peak,
             s.used, (s.used * 100UL) / s.total, s.free_now, s.free_min);
}

static struct { uint32_t count, min_us, max_us, avg_us; } g_agg = {0, 0xFFFFFFFFUL, 0, 0};

static void report_latency(const LatencyStats &l) {
  g_agg.count++;
  if (l.t_total_us < g_agg.min_us) g_agg.min_us = l.t_total_us;
  if (l.t_total_us > g_agg.max_us) g_agg.max_us = l.t_total_us;
  g_agg.avg_us += ((int32_t)(l.t_total_us - g_agg.avg_us)) / (int32_t)g_agg.count;
  ser.printf("[LAT #%lu] parse=%lu hash=%lu send=%lu wait=%lu read=%lu total=%lu us | "
             "min=%lu max=%lu avg=%lu us\n",
             g_agg.count, l.t_parse_us, l.t_hash_us, l.t_send_us, l.t_wait_us, l.t_read_us,
             l.t_total_us, g_agg.min_us, g_agg.max_us, g_agg.avg_us);
}

// ============================================================
// UTILITAS
// ============================================================

// Kirim byte sebagai hex huruf besar tanpa pemisah, dalam satu/dua paket USB
static void print_hex(const uint8_t *p, size_t n) {
  static const char H[] = "0123456789ABCDEF";
  char out[64];
  while (n) {
    size_t k = n > sizeof(out) / 2 ? sizeof(out) / 2 : n;
    for (size_t i = 0; i < k; i++) { out[2*i] = H[p[i] >> 4]; out[2*i+1] = H[p[i] & 0xF]; }
    ser.write((const uint8_t *)out, 2 * k);
    p += k; n -= k;
  }
}

static int hex_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// Ubah 2*n karakter hex menjadi n byte; false jika teks terlalu pendek / bukan hex
static bool parse_hex(const char *s, uint8_t *out, int n) {
  for (int i = 0; i < n; i++) {
    int hi = hex_nibble(s[2*i]);
    if (hi < 0) return false;
    int lo = hex_nibble(s[2*i+1]);
    if (lo < 0) return false;
    out[i] = (hi << 4) | lo;
  }
  return true;
}

// CRC16 Maxim/Dallas (terinvers) atas header lalu data, memakai OneWire::crc16
static uint16_t crc16_maxim(const uint8_t *hdr, int hdr_len, const uint8_t *dat, int dat_len) {
  uint16_t crc = OneWire::crc16(hdr, hdr_len, 0);
  crc = OneWire::crc16(dat, dat_len, crc);
  return ~crc;
}

// Tunggu satu baris dari laptop sambil menjalankan timer LED. Mengembalikan panjang baris.
static int read_line(char *buf, int size) {
  int n = 0;
  for (;;) {
    unsigned long now = millis();
    digitalWrite(PIN_RED,   (now - red_timeout)   < RED_TIME);
    digitalWrite(PIN_GREEN, (now - green_timeout) < GREEN_TIME);

    if (n >= size - 1) break;              // buffer penuh: proses apa adanya
    if (ser.available() <= 0) continue;
    char c = ser.read();
    if (c == '\r') continue;               // abaikan CR (terminal CRLF)
    if (c == '\n') break;
    buf[n++] = c;
  }
  buf[n] = '\0';
  return n;
}

// ============================================================
// PROTOKOL OneWire
// ============================================================

static bool ds_send(uint8_t command, const uint8_t *param, int param_len) {
  const uint8_t hdr[3] = {0x66, (uint8_t)(param_len + 1), command};

  // power=1: jalur tetap ditahan HIGH di antara byte (write_bytes() justru melepasnya)
  for (int i = 0; i < 3; i++) ds.write(hdr[i], 1);
  if (!auto_mode) ser.printf("\nData send:\n %02X %02X %02X\n", hdr[0], hdr[1], hdr[2]);

  for (int i = 0; i < param_len; i++) ds.write(param[i], 1);
  if (!auto_mode) for (int i = 0; i < param_len; i++) ser.printf(" %02X", param[i]);

  uint8_t crc[2];
  ds.read_bytes(crc, 2);
  ds.write(0xAA, 1);

  uint16_t comb_crc = (crc[1] << 8) | crc[0];
  uint16_t calc_crc = crc16_maxim(hdr, 3, param, param_len);
  bool ok = (calc_crc == comb_crc);
  if (!auto_mode) ser.printf("\nCRC %s:\n %04X [%04X]", ok ? "SUCCESS" : "FAILED", comb_crc, calc_crc);
  return ok;
}

// Baca balasan chip ke dat[]; *n_data = jumlah byte data (tanpa header & CRC)
static bool ds_read(uint8_t *dat, int *n_data) {
  uint8_t dummy  = ds.read();
  int     len    = ds.read() + 2;
  uint8_t result = ds.read();
  if (len < 3 || len >= 0xFE) {
    if (!auto_mode) ser.println("LENGTH ERROR");
    return false;
  }
  int n = len - 3;

  if (!auto_mode) ser.printf("\n\nData recieve:\n %02X %02X %02X\n ", dummy, len - 2, result);

  ds.read_bytes(dat, n);
  if (!auto_mode) for (int i = 0; i < n; i++) ser.printf("%02X ", dat[i]);

  uint8_t crc[3];                  // 2 byte CRC + 1 byte penutup (dibuang)
  ds.read_bytes(crc, 3);

  const uint8_t hdr[2] = {(uint8_t)(len - 2), result};
  uint16_t comb_crc = (crc[1] << 8) | crc[0];
  uint16_t calc_crc = crc16_maxim(hdr, 2, dat, n);
  bool ok = (calc_crc == comb_crc);
  if (!auto_mode) ser.printf("\nCRC %s:\n %04X [%04X]\n", ok ? "SUCCES" : "FAILED", comb_crc, calc_crc);
  *n_data = n;
  return ok;
}

// ============================================================

void setup(void) {
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_BUSY, OUTPUT);
  digitalWrite(PIN_BUSY, HIGH);
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, LOW);

  // Aktifkan USB CDC sedini mungkin: setelah bootloader, host langsung mulai enumerasi
  // (terlambat -> "Device Descriptor Request Failed").
  ser.begin(115200);
  delay(3000);
  // Tunggu laptop membuka port COM (maks 10 detik) agar "START!!" tidak hilang
  for (uint32_t t = millis(); !ser && (millis() - t) < 10000; ) { }
  ser.println("START!!");
#if ENABLE_MEASURE
  cyccnt_init();
  randomSeed(analogRead(PA0) ^ micros());
  stack_paint();
  print_ram_stats("BASELINE", measure_ram());
#endif
}

static void fail_reply() {
  ser.println("-1");
  digitalWrite(PIN_BUSY, HIGH);
}

void loop(void) {
  static bool first_run = true;   // perintah pertama setelah boot: identifikasi otomatis
  char buf[LINE_MAX];
  uint8_t para[PARA_MAX] = {0};
  uint8_t command = 0xAA;
  int param_len = 1;
  int dly = 30;

  LatencyStats lat = {0, 0, 0, 0, 0, 0};
  RamStats ram_before = {0};
  bool do_measure = false;
  uint32_t t_total_start = 0;   // siklus DWT
  uint32_t sp_ref = 0;          // SP saat operasi dimulai (acuan stack per operasi)

  // Mulai pengukuran: RAM awal, cat stack, lalu jalankan pewaktu DWT
  auto start_measure = [&]() {
    if (!ENABLE_MEASURE) return;
    do_measure = true;
    ram_before = measure_ram();
    stack_paint();
    sp_ref = get_sp();
    t_total_start = cycles();
  };

  if (first_run) {
    first_run = false;
    strcpy(buf, "AI");
  } else {
    read_line(buf, sizeof(buf));
  }
  ser.println(buf);

  if (buf[0] == 'A') {
    // ---------- mode otomatis: A<cmd><slot>[|<hex>] ----------
    auto_mode = true;
    uint8_t slot = buf[2] - '0';
    switch (buf[1]) {
      case 'I': para[0] = 0; break;
      case 'R': command = 0x44; para[0] = slot; break;
      case 'W':
      case 'S': {
        bool sign = (buf[1] == 'S');
        if (sign) start_measure();
        uint32_t t0 = cycles();
        const char *hex = strchr(buf, '|');
        if (!hex || !parse_hex(hex + 1, para + 1, 32)) { ser.println("-1"); return; }
        para[0] = slot; param_len = 33;
        command = sign ? 0xA5 : 0x96;
        dly     = sign ? 130 : 60;
        lat.t_parse_us = cyc_to_us(cycles() - t0);
        break;
      }
      case 'U': {
        start_measure();
        uint32_t t0 = cycles();
        para[0] = slot; param_len = 33; command = 0xA5; dly = 130;
        for (int i = 1; i < 33; i++) para[i] = random(0x100);
        lat.t_parse_us = cyc_to_us(cycles() - t0);
        break;
      }
      case 'M': {
        // AM<slot>|<pesan>: SHA-256(pesan) di host -> challenge 32 byte -> tanda tangan DS28E38
        const char *msg = strchr(buf, '|');
        if (!msg) { ser.println("-1"); return; }
        msg++;
        start_measure();
        uint32_t t0 = cycles();
        sha256_msg(msg, strlen(msg), para + 1);
        lat.t_hash_us = cyc_to_us(cycles() - t0);
        para[0] = slot; param_len = 33; command = 0xA5; dly = 130;
        break;
      }
      case 'L':
        if      (buf[2] == 'R') { red_timeout   = millis(); ser.println("1"); }
        else if (buf[2] == 'G') { green_timeout = millis(); ser.println("1"); }
        else ser.println("-1");
        return;
      default: ser.println("-1"); return;
    }
  } else {
    // ---------- mode manual: <delay>|<len> <cmd>|<p0> <p1> ... (hex) ----------
    auto_mode = false;
    char *tok1 = strtok(buf, "|");
    if (tok1) dly = atoi(tok1);
    char *comm = strtok(NULL, "|");
    char *parm = strtok(NULL, "|");
    if (comm && parm) {
      char *t_len = strtok(comm, " ");
      char *t_cmd = strtok(NULL, " ");
      char *t_p0  = strtok(parm, " ");
      if (!t_len || !t_cmd || !t_p0) { ser.println("-1"); return; }
      param_len = strtol(t_len, NULL, 16) - 1;
      if (param_len < 1 || param_len > PARA_MAX) { ser.println("-1"); return; }
      command = strtol(t_cmd, NULL, 16);
      para[0] = strtol(t_p0, NULL, 16);
      for (int x = 1; x < param_len; x++) {
        char *t = strtok(NULL, " ");
        if (!t) { ser.println("-1"); return; }
        para[x] = strtol(t, NULL, 16);
      }
    }
  }

  digitalWrite(PIN_BUSY, LOW);

  uint8_t addr[8] = {0};
  ds.reset_search();
  if (!ds.search(addr) || OneWire::crc8(addr, 7) != addr[7]) { fail_reply(); return; }

  if (!auto_mode) {
    ser.print("ROM =");
    for (int i = 0; i < 8; i++) { ser.write(' '); ser.print(addr[i], HEX); }
  }

  delay(1);
  ds.reset();
  ds.select(addr);

  // ---------- kirim perintah (maks 2 percobaan) ----------
  uint32_t t = cycles();
  bool ok = ds_send(command, para, param_len) || ds_send(command, para, param_len);
  lat.t_send_us = cyc_to_us(cycles() - t);
  if (!ok) { fail_reply(); return; }

  // ---------- tunggu chip memproses ----------
  t = cycles();
  delay(dly);
  lat.t_wait_us = cyc_to_us(cycles() - t);

  // ---------- baca balasan (maks 2 percobaan) ----------
  uint8_t data[DS_DATA_MAX];
  int n = 0;
  t = cycles();
  ok = ds_read(data, &n) || ds_read(data, &n);
  lat.t_read_us = cyc_to_us(cycles() - t);
  if (!ok) { fail_reply(); return; }

  ds.reset();

  // Tanda tangan (0xA5) harus membawa minimal 64 byte data
  if (auto_mode && command == 0xA5 && n < 64) { fail_reply(); return; }

  if (do_measure) {
    lat.t_total_us = cyc_to_us(cycles() - t_total_start);
    uint32_t stack_op = sp_ref - stack_low_water();   // stack terdalam selama hash+sign
    RamStats ram_after = measure_ram();
    ser.printf("[STACK op] %lu bytes\n", (unsigned long)stack_op);
    print_ram_stats("SIGN", ram_after);
    ser.printf("[RAM delta] used %+ld bytes\n", (long)ram_after.used - (long)ram_before.used);
    report_latency(lat);
  }

  if (auto_mode) {
    switch (command) {
      case 0xAA: print_hex(addr, 8); ser.print("|"); print_hex(data, n); ser.println(); break;
      case 0x44: print_hex(data, n); ser.println(); break;
      case 0x96: ser.println("1"); break;
      case 0xA5:
        print_hex(para + 1, 32); ser.print("|");
        print_hex(data + 32, 32); print_hex(data, 32);   // urutan: paruh kedua lalu paruh pertama
        ser.println();
        break;
    }
  }
  digitalWrite(PIN_BUSY, HIGH);
}
