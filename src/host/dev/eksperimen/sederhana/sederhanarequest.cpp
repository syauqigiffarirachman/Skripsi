// STM32F103C8 <-> DS28E38 (OneWire, PA15). Serial USB 115200, akhiri dengan Enter.
//   id            -> ROM ID chip
//   ttd <digest>  -> tanda tangan r||s (128 hex) dari digest SHA-256 (64 hex)
//
// Kecepatan pembuatan tanda tangan (ECDSA P-256), hasil benchmark di data/ (STM32 48 MHz):
//   - Hardware (DS28E38): ±224 ms per tanda tangan, rinciannya kirim ±24 ms,
//     chip menghitung ±129 ms, baca hasil ±46 ms.
//   - Software murni (micro-ecc di STM32): ±260 ms per tanda tangan.
//   Jadi chip hanya sedikit lebih cepat (±1,2x), keunggulan utamanya kunci privat tidak
//   pernah keluar dari chip, bukan kecepatan.
// Optimasi "ttd" (sebelumnya menunggu tetap 520 ms -> ±0,6 detik per tanda tangan):
//   1. Tidak lagi menunggu tetap. Chip mengirim 1 terus selama sibuk (±107 ms untuk ECDSA),
//      jadi bus diintip per BIT dan hasil dibaca begitu chip selesai (batas aman tetap 520 ms).
//   2. ROM ID disimpan (tidak dicari ulang tiap perintah, hemat ±25 ms); dicari ulang otomatis
//      kalau gagal.
//   Hasil terukur (ttd lewat USB, dari kirim perintah sampai balasan diterima, 48 MHz):
//   ±190 ms per tanda tangan, stabil (tiap tanda tangan diverifikasi di PC). Lebih cepat dari
//   software murni (±260 ms) dan ±3x lebih cepat dari sebelum optimasi.
//   Sisa waktu: chip menghitung ±107 ms (tetap), kirim ±24 ms + baca ±40 ms lewat OneWire
//   kecepatan standar. Overdrive sudah dicoba tetapi chip tidak membalas, jadi tidak dipakai.
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
static const char *xpc(uint8_t cmd, const uint8_t *par, int plen, int awal_ms, int tunggu_ms,
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

    // Selama sibuk bus terbaca 1 terus. Setelah selesai chip mengirim: 0xFF (dummy), lalu byte
    // panjang yang bit-0-nya 1 (0x41 untuk tanda tangan) dan bit-1-nya 0. Jadi bit 0 pertama
    // yang terbaca = bit-1 byte panjang. Intip per BIT (bukan per byte) agar sejajar dengan
    // aliran data chip. Chip butuh ±107 ms; intip dimulai di awal_ms, batas waktu tunggu_ms.
    uint32_t t0 = millis();
    delay(awal_ms);
    bool siap = false;
    while ((uint32_t)(millis() - t0) < (uint32_t)tunggu_ms) {
      if (ds.read_bit() == 0) { siap = true; break; }
      delayMicroseconds(200);
    }
    if (!siap) { err = "chip tidak selesai"; continue; }
    uint8_t len = 0x01;                                 // bit-0 = 1, bit-1 = 0 (baru terbaca)
    for (int i = 2; i < 8; i++) len |= (uint8_t)(ds.read_bit() << i);
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

static bool g_rom_ok = false;                           // ROM sudah dikenal (hemat ±25 ms)

static void proses(char *baris) {
  char *cmd = strtok(baris, " ");
  if (!cmd) return;

  if (!strcmp(cmd, "id")) {
    g_rom_ok = cari_chip();                             // "id" selalu cari ulang
    if (!g_rom_ok) { ser.println("GAGAL: DS28E38 tidak ditemukan"); return; }
    print_hex(g_rom, 8); ser.println();
  } else if (!strcmp(cmd, "ttd")) {
    char *a = strtok(nullptr, " ");
    uint8_t par[33] = {0}, sig[64];                     // par[0] = halaman 0
    if (!a || !parse_hex(a, par + 1, 32)) { ser.println("Pakai: ttd <64 hex>"); return; }
    const char *err = "?";
    for (int coba = 0; coba < 2; coba++) {
      if (!g_rom_ok && !(g_rom_ok = cari_chip())) { err = "DS28E38 tidak ditemukan"; break; }
      err = xpc(0xA5, par, 33, 80, 520, sig, 64);
      if (!err) break;
      g_rom_ok = false;                                 // gagal -> cari ulang ROM lalu coba lagi
    }
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
