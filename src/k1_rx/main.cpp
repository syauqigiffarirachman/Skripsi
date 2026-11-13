// ============================================================================
// PENERIMA FRAME DARI ATTACKER - TAHAP 1 (build from base) - STM32F103C8 (host)
//
// Fungsi tahap ini SATU: membuktikan frame 36 B dari attacker sampai di USART2 (PA3) host
// dan utuh. Tidak ada ECDSA, DMA, WFI, atau jendela 60 s (itu tahap berikutnya).
//
// Jalur: attacker PA9 -> (resistor seri ~1 kOhm) -> PA3 host (USART2 RX), ground bersama.
// Frame (format sama dengan src/attacker/attacker.cpp):
//   [0]=0xA5  [1..2]=seq uint16 LE  [3..34]=payload 32 B  [35]=XOR byte [0..34]
//
// Perintah (USB CDC 115200, akhiri newline):
//   S   cetak statistik:  ok, bad(checksum/sinkron salah), hilang(selisih seq), seq terakhir
//   Z   nol-kan statistik
// Laju attacker 1 atau 10 frame/detik; USB hanya dipakai saat diminta (hindari membanjiri
// antrean RX USB 192 B: bug HardFault core 4.21).
// Build: pio run -e k1_rx   |   Upload: pio run -e k1_rx -t upload (ST-Link terpasang)
// ============================================================================
#include <Arduino.h>

#define ser  SerialUSB
#define link Serial2            // USART2: PA3 = RX dari attacker, PA2 = TX (tidak dipakai)

static const uint32_t LINK_BAUD = 115200;     // harus sama dengan HOST_BAUD attacker
static const uint8_t  FRAME_SOF = 0xA5;
static const uint8_t  FRAME_LEN = 36;

static uint8_t  buf[FRAME_LEN];
static uint8_t  fill = 0;                     // jumlah byte yang sudah terkumpul di buf

static uint32_t n_ok = 0, n_bad = 0, n_lost = 0;
static uint16_t last_seq = 0;
static bool     have_seq = false;

static bool frame_valid() {
  uint8_t x = 0;
  for (uint8_t i = 0; i < FRAME_LEN - 1; i++) x ^= buf[i];
  return buf[0] == FRAME_SOF && x == buf[FRAME_LEN - 1];
}

// Buang byte terdepan lalu geser ke SOF berikutnya (resinkronisasi bila ada byte hilang/rusak).
static void resync() {
  uint8_t i = 1;
  while (i < fill && buf[i] != FRAME_SOF) i++;
  uint8_t keep = fill - i;
  for (uint8_t k = 0; k < keep; k++) buf[k] = buf[i + k];
  fill = keep;
}

static void on_byte(uint8_t b) {
  if (fill == 0 && b != FRAME_SOF) return;    // tunggu penanda awal
  buf[fill++] = b;
  if (fill < FRAME_LEN) return;

  if (frame_valid()) {
    uint16_t seq = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8);
    if (have_seq) n_lost += (uint16_t)(seq - (uint16_t)(last_seq + 1));   // aman terhadap limpahan
    last_seq = seq; have_seq = true;
    n_ok++;
    fill = 0;
  } else {
    n_bad++;
    resync();
  }
}

static void cmd_status() {
  ser.printf("ok=%lu bad=%lu hilang=%lu seq_terakhir=%u t_ms=%lu\n",
             (unsigned long)n_ok, (unsigned long)n_bad, (unsigned long)n_lost,
             (unsigned)last_seq, (unsigned long)millis());
}

void setup() {
  ser.begin(115200);
  link.begin(LINK_BAUD);
  for (uint32_t t = millis(); !ser && (millis() - t) < 10000; ) { }
  ser.println("K1 RX tahap 1 (penerima frame USART2 PA3) siap. Perintah: S | Z");
}

void loop() {
  while (link.available() > 0) on_byte((uint8_t)link.read());

  if (ser.available() > 0) {
    char c = ser.read();
    if (c == 'S') cmd_status();
    else if (c == 'Z') { n_ok = n_bad = n_lost = 0; have_seq = false; fill = 0; ser.println("nol"); }
  }
}
