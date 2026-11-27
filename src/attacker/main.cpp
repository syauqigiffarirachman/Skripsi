// =====================================================================================
// NODE PEMBANGKIT PERMINTAAN AUTENTIKASI - STM32F030C8T6 (board ke-2 dari 3)
// Tahap 1 (build from base): kirim frame pada laju tetap memakai pewaktu hardware (TIM3).
// Fungsi tahap ini SATU: pembangkitan + pengiriman frame. Belum ada handshake "papan siap"
// maupun penerimaan balasan host; itu tahap berikutnya setelah tahap ini stabil.
//
// Pin (pengkabelan fisik sejak 12 Nov: UART1 sudah disambung ke papan host, jadi CH340
// dipindah ke UART2 -- KEBALIKAN dari tahap 1 awal):
//   USART1 PA9(TX)/PA10(RX) -> host : jalur frame (tahap ini hanya memakai arah TX attacker).
//                              PA9(TX) attacker -> PA3(RX) host via resistor seri ~1 kOhm
//                              (sesuai Tabel 3.3 paper), ground bersama.
//   USART2 PA2(TX)/PA3(RX)  -> CH340: konsol kontrol 115200 8N1, dan jalur bootloader upload.
//   Bootloader ROM STM32F0 mendukung USART1 maupun USART2 (AN2606/AN3155), jadi upload lewat
//   CH340 di USART2 tetap berfungsi.
//
// Perintah konsol (satu karakter, tanpa Enter):
//   a = mulai mode A (1 frame/detik)    b = mulai mode B (10 frame/detik)
//   x = berhenti                         s = cetak status                f = cetak 1 frame (hex)
//
// Format frame (36 byte, dikirim kontigu agar deteksi idle-line di penerima
// menganggapnya satu frame / satu bangun CPU):
//   [0]     = 0xA5 (penanda awal frame)
//   [1..2]  = nomor urut, uint16 little-endian
//   [3..34] = isi tetap 32 byte (pengganti tantangan tetap pada tahap ini)
//   [35]    = XOR byte [0..34], untuk pemeriksaan sederhana di penerima
// Nomor urut dipakai penerima untuk menghitung frame yang terlewat.
// =====================================================================================
#include <Arduino.h>
#include <HardwareTimer.h>

// PENTING (bug tahap 1, diperbaiki): pada varian board F030C8T generik ini, objek default
// "Serial" adalah makro alias ke Serial2 (SERIAL_UART_INSTANCE=2 di variant_generic.h).
// Jangan pernah memakai "Serial" polos di file ini — selalu pakai Serial1/Serial2 eksplisit
// agar tidak diam-diam berebut periferal yang sama. Pemetaan di bawah ini mengikuti
// pengkabelan FISIK saat ini (lihat komentar pin di atas): CH340 di USART2, host di USART1.
#define Console  Serial2    // USART2 PA2(TX)/PA3(RX)  -> CH340
#define HostLink Serial1    // USART1 PA9(TX)/PA10(RX) -> host

// Format frame (FRAME_SOF, FRAME_PAYLOAD_LEN, FRAME_LEN, FRAME_BAUD) didefinisikan SATU kali di
// lib/bench_proto/proto_attacker_host.h, dipakai bersama penerima di host.
#include <proto_attacker_host.h>

static const uint32_t CONSOLE_BAUD  = 115200;

static HardwareTimer  Ticker(TIM3);         // pewaktu hardware: ritme pengiriman frame

static uint8_t  g_frame[FRAME_LEN];
static uint16_t g_seq = 0;

// Diubah di dalam ISR pewaktu, dibaca di loop() - ditandai volatile.
static volatile bool g_running   = false;
static volatile bool g_sendFlag  = false;
static volatile uint32_t g_tickCount = 0;
// Ticker.refresh() memaksa update event yg langsung memicu interrupt begitu timer
// (re)start, sebelum periode pertama benar-benar lewat (terbukti lewat uji millis()).
// Tick palsu pertama ini dibuang agar frame pertama benar-benar terkirim setelah 1 periode.
static volatile bool g_skipFirstTick = false;

enum Mode : uint8_t { MODE_IDLE = 0, MODE_A = 1, MODE_B = 2, MODE_C = 3 };
static Mode g_mode = MODE_IDLE;

// Mode C (27 Nov 2026, skenario kapasitas rho = 1): periode bebas dalam mikrodetik, perintah "p<periode_us>" + Enter.
// Periode = latensi tanda tangan rata-rata arsitektur yang diserang (dikoreksi galat jam HSI attacker oleh PC).
static const uint32_t PERIOD_MIN_US = 1000UL;       // 1 ms (1000/s): batas bawah wajar untuk frame 36 B @115200 (~3,1 ms)
static const uint32_t PERIOD_MAX_US = 10000000UL;   // 10 s
static uint32_t g_period_us = 0;
static bool     g_pEntry = false;                   // sedang mengetik "p<angka>"
static uint32_t g_pVal = 0;
static uint8_t  g_pDigits = 0;
static uint32_t g_startMillis = 0;   // waktu mulai mode, dipakai utk hitung laju asli di board

// --- Pembangun frame -----------------------------------------------------------------
static void buildFrame(uint16_t seq) {
  g_frame[0] = FRAME_SOF;
  g_frame[1] = (uint8_t)(seq & 0xFF);
  g_frame[2] = (uint8_t)(seq >> 8);
  for (uint8_t i = 0; i < FRAME_PAYLOAD_LEN; i++) {
    g_frame[3 + i] = (uint8_t)(0x30 + i);   // isi tetap, mudah diperiksa saat debug
  }
  uint8_t x = 0;
  for (uint8_t i = 0; i < FRAME_LEN - 1; i++) x ^= g_frame[i];
  g_frame[FRAME_LEN - 1] = x;
}

// --- ISR pewaktu hardware: HANYA menaikkan penanda, tidak mengirim UART di sini ------
static void onTick() {
  if (g_skipFirstTick) {      // buang tick palsu dari update event paksa saat start
    g_skipFirstTick = false;
    return;
  }
  g_tickCount++;
  g_sendFlag = true;
}

static void timerStart(uint32_t freqHz) {
  Ticker.pause();
  Ticker.setOverflow(freqHz, HERTZ_FORMAT);
  Ticker.attachInterrupt(onTick);
  Ticker.refresh();
  Ticker.resume();
}

// Mode C: HardwareTimer memilih prescaler sendiri (TIM3 16-bit), resolusi < 2 us untuk periode 100 ms - 1 s.
static void timerStartUs(uint32_t periodUs) {
  Ticker.pause();
  Ticker.setOverflow(periodUs, MICROSEC_FORMAT);
  Ticker.attachInterrupt(onTick);
  Ticker.refresh();
  Ticker.resume();
}

static void timerStop() {
  Ticker.pause();
}

static void modeStart(Mode m) {
  g_mode = m;
  g_seq = 0;
  g_tickCount = 0;
  g_sendFlag = false;
  g_skipFirstTick = true;
  g_startMillis = millis();
  uint32_t freq = (m == MODE_B) ? 10 : 1;   // skenario A: 1/s, skenario B: 10/s
  timerStart(freq);
  g_running = true;
  Console.print("[attacker] mulai mode ");
  Console.print(m == MODE_B ? "B (10/s)" : "A (1/s)");
  Console.println();
}

// Mode C dibuat terpisah dari modeStart() agar jalur mode A/B tidak berubah sama sekali.
static void modeStartPeriod(uint32_t periodUs) {
  g_mode = MODE_C;
  g_period_us = periodUs;
  g_seq = 0;
  g_tickCount = 0;
  g_sendFlag = false;
  g_skipFirstTick = true;
  g_startMillis = millis();
  timerStartUs(periodUs);
  g_running = true;
  Console.print("[attacker] mulai mode C (");
  Console.print(periodUs);
  Console.println(" us)");
}

// Menutup pengetikan "p<angka>": mulai mode C bila angka dalam rentang, selain itu tolak tanpa mengubah mode.
static void periodEntryFinish() {
  g_pEntry = false;
  if (g_pDigits > 0 && g_pVal >= PERIOD_MIN_US && g_pVal <= PERIOD_MAX_US) {
    modeStartPeriod(g_pVal);
  } else {
    Console.println("[attacker] periode ditolak (1000..10000000 us)");
  }
}

static void modeStop() {
  timerStop();
  g_running = false;
  g_mode = MODE_IDLE;
  Console.println("[attacker] berhenti");
}

static void printFrameHex() {
  buildFrame(g_seq);
  for (uint8_t i = 0; i < FRAME_LEN; i++) {
    if (g_frame[i] < 0x10) Console.print('0');
    Console.print(g_frame[i], HEX);
    Console.print(' ');
  }
  Console.println();
}

static void printStatus() {
  Console.print("[attacker] mode=");
  Console.print(g_mode == MODE_A ? "A" : (g_mode == MODE_B ? "B" : (g_mode == MODE_C ? "C" : "idle")));
  Console.print(" berjalan=");
  Console.print(g_running ? "ya" : "tidak");
  Console.print(" seq=");
  Console.print(g_seq);
  Console.print(" t_ms=");   // waktu sejak mode dimulai, diukur di board (ground truth)
  if (g_mode == MODE_C) {
    Console.print(g_running ? (millis() - g_startMillis) : 0);
    Console.print(" periode_us=");
    Console.println(g_period_us);
  } else {
    Console.println(g_running ? (millis() - g_startMillis) : 0);
  }
}

static void printHelp() {
  Console.println(F("a = mode A (1/s) | b = mode B (10/s) | p<us>+Enter = mode C (periode bebas) | x = berhenti | s = status | f = cetak frame"));
}

void setup() {
  Console.begin(CONSOLE_BAUD);    // USART2 lewat CH340
  HostLink.begin(FRAME_BAUD);     // USART1 ke host
  delay(50);
  Console.println();
  Console.println(F("=== Node pembangkit permintaan - tahap 1 (pewaktu TIM3) ==="));
  printHelp();
#ifdef ATTACKER_AUTOSTART_HZ
  // Sejak 13 Nov konsol CH340 tidak dipakai (upload+daya lewat ST-Link/USB-B), jadi tidak ada
  // cara mengirim 'a'/'b'. Build flag -DATTACKER_AUTOSTART_HZ=1 (mode A) atau =10 (mode B)
  // memulai pengiriman otomatis saat boot. Tanpa flag ini perilaku tidak berubah.
  modeStart(ATTACKER_AUTOSTART_HZ >= 10 ? MODE_B : MODE_A);
#endif
}

void loop() {
  // Kirim frame bila pewaktu sudah menandai giliran (dieksekusi di loop, bukan di ISR)
  if (g_sendFlag) {
    g_sendFlag = false;
    buildFrame(g_seq);
    HostLink.write(g_frame, FRAME_LEN);
    g_seq++;
  }

  if (Console.available()) {
    char c = (char)Console.read();
    if (g_pEntry) {                                   // sedang mengetik "p<angka>"
      if (c >= '0' && c <= '9' && g_pDigits < 8) {
        g_pVal = g_pVal * 10 + (uint32_t)(c - '0');
        g_pDigits++;
        return;
      }
      if (c == '\r' || c == '\n') { periodEntryFinish(); return; }
      g_pEntry = false;                               // karakter lain: batalkan, lalu proses seperti biasa
      Console.println("[attacker] periode dibatalkan");
    }
    switch (c) {
      case 'p': case 'P': g_pEntry = true; g_pVal = 0; g_pDigits = 0; break;
      case 'a': case 'A': modeStart(MODE_A); break;
      case 'b': case 'B': modeStart(MODE_B); break;
      case 'x': case 'X': modeStop();        break;
      case 's': case 'S': printStatus();     break;
      case 'f': case 'F': printFrameHex();   break;
      case '\r': case '\n': break;
      default: printHelp(); break;
    }
  }
}
