// STM32F103C8 <-> DS28E38 (OneWire, PA15). Serial USB 115200, akhiri dengan Enter.
//   id            -> ROM ID chip
//   ttd <digest>  -> tanda tangan r||s (128 hex) dari digest SHA-256 (64 hex)
#include <Arduino.h>
#include <OneWire.h>

#define ser SerialUSB
OneWire ds(PA15);
static uint8_t g_rom[8];

static void print_hex(const uint8_t *p, int n) {
  for (int i = 0; i < n; i++) ser.printf("%02x", p[i]);
}

static bool parse_hex(const char *h, uint8_t *out, int n) {
  for (int i = 0; i < 2 * n; i++) if (!isxdigit((unsigned char)h[i])) return false;
  if (h[2 * n] != '\0') return false;
  for (int i = 0; i < n; i++) {
    char b[3] = {h[2 * i], h[2 * i + 1], 0};
    out[i] = (uint8_t)strtoul(b, nullptr, 16);
  }
  return true;
}

// ROM harus terbaca sama dua kali (hindari ROM palsu saat bus baru reset)
static bool cari_chip() {
  for (int coba = 0; coba < 4; coba++) {
    uint8_t a[8], b[8];
    ds.reset_search();
    if (!ds.search(a) || OneWire::crc8(a, 7) != a[7]) { delay(20); continue; }
    ds.reset_search();
    if (!ds.search(b) || memcmp(a, b, 8) != 0) { delay(20); continue; }
    memcpy(g_rom, a, 8);
    return true;
  }
  return false;
}

// Perintah DS28E38: kirim 0x66,panjang,cmd,param -> cek CRC -> 0xAA + tunggu -> baca hasil.
// Return nullptr jika sukses, atau teks kesalahan.
static const char *xpc(uint8_t cmd, const uint8_t *par, int plen, int tunggu_ms,
                       uint8_t *data, int ada) {
  const char *err = "?";
  for (int coba = 0; coba < 2; coba++) {
    ds.reset();
    ds.select(g_rom);

    uint8_t hdr[3] = {0x66, (uint8_t)(plen + 1), cmd};
    uint16_t crc = OneWire::crc16(hdr, 3, 0);
    crc = OneWire::crc16(par, plen, crc);
    for (int i = 0; i < 3; i++) ds.write(hdr[i], 1);
    for (int i = 0; i < plen; i++) ds.write(par[i], 1);
    uint8_t c[2];
    ds.read_bytes(c, 2);
    if ((uint16_t)~crc != (uint16_t)((c[1] << 8) | c[0])) { err = "CRC perintah salah"; continue; }

    ds.write(0xAA, 1);
    delay(tunggu_ms);

    ds.read();                                          // dummy
    uint8_t len = ds.read();
    if (len == 0 || len >= 0xFE || len > 70) { err = "panjang balasan salah"; continue; }
    uint8_t body[72];
    ds.read_bytes(body, len + 2);
    uint16_t rc = OneWire::crc16(&len, 1, 0);
    rc = OneWire::crc16(body, len, rc);
    if ((uint16_t)~rc != (uint16_t)((body[len + 1] << 8) | body[len])) { err = "CRC balasan salah"; continue; }

    if (body[0] != 0xAA) return "chip menolak perintah";
    if (len - 1 != ada) return "panjang data tidak sesuai";
    memcpy(data, body + 1, ada);
    return nullptr;
  }
  return err;
}

static void proses(char *baris) {
  char *cmd = strtok(baris, " ");
  if (!cmd) return;
  if (!cari_chip()) { ser.println("GAGAL: DS28E38 tidak ditemukan"); return; }

  if (!strcmp(cmd, "id")) {
    print_hex(g_rom, 8); ser.println();
  } else if (!strcmp(cmd, "ttd")) {
    char *a = strtok(nullptr, " ");
    uint8_t par[33] = {0}, sig[64];                     // par[0] = halaman 0
    if (!a || !parse_hex(a, par + 1, 32)) { ser.println("Pakai: ttd <64 hex>"); return; }
    const char *err = xpc(0xA5, par, 33, 520, sig, 64);
    if (err) { ser.printf("GAGAL: %s\n", err); return; }
    print_hex(sig + 32, 32);                            // r
    print_hex(sig, 32);                                 // s
    ser.println();
  } else {
    ser.println("Perintah: id | ttd <digest>");
  }
}

void setup() { ser.begin(115200); }

void loop() {
  static char buf[100];
  static int n = 0;
  while (ser.available() > 0) {
    char c = ser.read();
    if (c == '\r' || c == '\n') {
      if (n == 0) continue;                             // abaikan \n setelah \r
      ser.println();
      buf[n] = 0; n = 0; proses(buf);
    } else if (c >= 32 && n < (int)sizeof(buf) - 1) {
      buf[n++] = c; ser.write(c);                       // echo
    }
  }
}
