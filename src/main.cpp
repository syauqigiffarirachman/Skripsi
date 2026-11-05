#include <OneWire.h>
#include <stdarg.h> // Dibutuhkan untuk fungsi serPrintf

// Serial lewat USB CDC (USB-C: PA11/PA12), muncul sebagai port COM di laptop
#define serial SerialUSB
#define ser serial

// [PERBAIKAN 2] Menghilangkan tanda kurung kurawal {} dan titik koma ; agar tidak merusak if-else
#define RED(red) digitalWrite(PB6,red)  
#define GREEN(green) digitalWrite(PB5,green)

#define RED_TIME 5000
#define GREEN_TIME 500

OneWire ds(PA15);
bool begi = 1;
bool auto_mode = 0;

unsigned long red_timeot = 0;
unsigned long green_timeot = 0;

// ====================================================================
// [PERBAIKAN 3] FUNGSI BANTUAN PRINTF UNTUK STM32
// ====================================================================
void serPrintf(const char *format, ...) {
  char buf[128]; // Buffer untuk menampung teks sementara
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args); // Menggabungkan format dan variabel
  va_end(args);
  serial.print(buf); // Mengirimkan teks utuh ke Serial
}

// ============================================================
// RAM & LATENCY MONITORING  (set 0 untuk mematikan seluruh pengukuran)
// ============================================================
#define ENABLE_MEASURE 1
#define STACK_PAINT    0xA5A5A5A5UL
#define STACK_MARGIN   512          // jarak aman di atas heap & di bawah SP saat paint

extern "C" char *sbrk(int i);
extern char _sdata, _end, _estack;  // simbol linker (awal RAM, akhir .bss, puncak stack)

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

static inline uint32_t get_sp() {
  uint32_t sp;
  __asm__ volatile ("mov %0, sp" : "=r" (sp));
  return sp;
}

static uint32_t *g_paint_lo = nullptr;   // batas bawah area stack yang dicat

// Isi area antara heap dan stack dengan pola, dipanggil sekali di setup()
void stack_paint() {
  uint32_t *lo = (uint32_t *)(((uint32_t)sbrk(0) + STACK_MARGIN + 3) & ~3UL);
  uint32_t *hi = (uint32_t *)((get_sp() - STACK_MARGIN) & ~3UL);
  if (lo >= hi) return;
  g_paint_lo = lo;
  for (uint32_t *p = lo; p < hi; p++) *p = STACK_PAINT;
}

// Cari alamat terendah yang sudah tertimpa stack (hanya dipanggil saat laporan)
static uint32_t stack_peak_bytes() {
  uint32_t top = (uint32_t)&_estack;
  if (!g_paint_lo) return top - get_sp();
  uint32_t *p = g_paint_lo;
  uint32_t *sp = (uint32_t *)get_sp();
  while (p < sp && *p == STACK_PAINT) p++;
  return top - (uint32_t)p;
}

RamStats measure_ram() {
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

void print_ram_stats(const char *label, const RamStats &s) {
  serial.printf("[RAM %s] total=%lu static=%lu heap=%lu stack=%lu (peak %lu) "
             "used=%lu (%lu%%) free=%lu (min %lu) bytes\n",
             label, s.total, s.statik, s.heap, s.stack_now, s.stack_peak,
             s.used, (s.used * 100UL) / s.total, s.free_now, s.free_min);
}

typedef struct {
  uint32_t t_parse_us;       // waktu parsing / pembentukan parameter
  uint32_t t_send_us;        // ds_send (termasuk retry)
  uint32_t t_wait_us;        // delay tunggu chip
  uint32_t t_read_us;        // ds_read (termasuk retry)
  uint32_t t_total_us;       // end-to-end (perintah diterima -> hasil siap)
} LatencyStats;

typedef struct {
  uint32_t count, min_us, max_us, avg_us;
} LatencyAgg;

static LatencyAgg g_agg = {0, 0xFFFFFFFFUL, 0, 0};

void update_latency_stats(uint32_t total_us) {
  g_agg.count++;
  if (total_us < g_agg.min_us) g_agg.min_us = total_us;
  if (total_us > g_agg.max_us) g_agg.max_us = total_us;
  g_agg.avg_us += ((int32_t)(total_us - g_agg.avg_us)) / (int32_t)g_agg.count;
}

void print_latency_report(const LatencyStats &l) {
  serial.printf("[LAT #%lu] parse=%lu send=%lu wait=%lu read=%lu total=%lu us | "
             "min=%lu max=%lu avg=%lu us\n",
             g_agg.count, l.t_parse_us, l.t_send_us, l.t_wait_us, l.t_read_us,
             l.t_total_us, g_agg.min_us, g_agg.max_us, g_agg.avg_us);
}
// ============================================================


static const uint16_t crctableMaxim[256] ={
  0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0,
  0x0280, 0xC241, 0xC601, 0x06C0, 0x0780, 0xC741,
  0x0500, 0xC5C1, 0xC481, 0x0440, 0xCC01, 0x0CC0,
  0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
  0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0,
  0x0880, 0xC841, 0xD801, 0x18C0, 0x1980, 0xD941,
  0x1B00, 0xDBC1, 0xDA81, 0x1A40, 0x1E00, 0xDEC1,
  0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
  0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0,
  0x1680, 0xD641, 0xD201, 0x12C0, 0x1380, 0xD341,
  0x1100, 0xD1C1, 0xD081, 0x1040, 0xF001, 0x30C0,
  0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
  0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0,
  0x3480, 0xF441, 0x3C00, 0xFCC1, 0xFD81, 0x3D40,
  0xFF01, 0x3FC0, 0x3E80, 0xFE41, 0xFA01, 0x3AC0,
  0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
  0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0,
  0x2A80, 0xEA41, 0xEE01, 0x2EC0, 0x2F80, 0xEF41,
  0x2D00, 0xEDC1, 0xEC81, 0x2C40, 0xE401, 0x24C0,
  0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
  0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0,
  0x2080, 0xE041, 0xA001, 0x60C0, 0x6180, 0xA141,
  0x6300, 0xA3C1, 0xA281, 0x6240, 0x6600, 0xA6C1,
  0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
  0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0,
  0x6E80, 0xAE41, 0xAA01, 0x6AC0, 0x6B80, 0xAB41,
  0x6900, 0xA9C1, 0xA881, 0x6840, 0x7800, 0xB8C1,
  0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
  0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1,
  0xBC81, 0x7C40, 0xB401, 0x74C0, 0x7580, 0xB541,
  0x7700, 0xB7C1, 0xB681, 0x7640, 0x7200, 0xB2C1,
  0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
  0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0,
  0x5280, 0x9241, 0x9601, 0x56C0, 0x5780, 0x9741,
  0x5500, 0x95C1, 0x9481, 0x5440, 0x9C01, 0x5CC0,
  0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
  0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0,
  0x5880, 0x9841, 0x8801, 0x48C0, 0x4980, 0x8941,
  0x4B00, 0x8BC1, 0x8A81, 0x4A40, 0x4E00, 0x8EC1,
  0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
  0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0,
  0x4680, 0x8641, 0x8201, 0x42C0, 0x4380, 0x8341,
  0x4100, 0x81C1, 0x8081, 0x4040
};

uint16_t CalculateCRC16( uint16_t crc, const void *c_ptr, int len)    
{
  const uint8_t *c = (uint8_t *)c_ptr;
  while (len--)
    crc = (crc >> 8 & 0xFF) ^ crctableMaxim[(crc ^ *c++) & 0xFF];

  crc ^= 0xFFFF;
  crc &= 0xFFFF;
  return crc;
}


bool ds_send(byte command, byte *param, int param_len){
  byte crc[2];
  byte data_send[param_len + 3];
  
  ds.write(0x66,1);
  ds.write(param_len + 1,1);
  ds.write(command,1);
  if(!auto_mode) serPrintf("\nData send:\n %02X %02X %02X\n", 0x66, param_len + 1, command);
  
  for(int i = 0; i<param_len; i++){
    ds.write(param[i],1);
    data_send[i+3] = param[i];
    if(!auto_mode) serPrintf(" %02X", param[i]);
  }
  
  crc[0] = ds.read(); crc[1] = ds.read();
  ds.write(0xAA, 1);
  data_send[0] = 0x66; data_send[1] = (param_len + 1); data_send[2] = command;

  uint16_t calc_crc;
  uint16_t comb_crc = (crc[1]<<8) | (crc[0]); 
  calc_crc = CalculateCRC16(0x0000, data_send, param_len +3);

  if(calc_crc == comb_crc){
    if(!auto_mode) serPrintf("\nCRC SUCCESS:\n %04X [%04X]", comb_crc, calc_crc);
    return true;
  }

  if(!auto_mode) serPrintf("\nCRC FAILED:\n %04X [%04X]", comb_crc, calc_crc);
  return false;
}

#define DS_DATA_MAX 256   // ukuran buffer data[] di loop()

bool ds_read(byte *dat, int *len){
  byte dummy = ds.read();
  *len = ds.read() + 2;
  byte result = ds.read();
  // len-3 byte data harus muat di buffer data[] milik pemanggil (DS_DATA_MAX)
  if(*len < 3 || *len >= 0xFE || (*len - 3) > DS_DATA_MAX){
    if(!auto_mode) ser.println("LENGTH ERROR");
    return false;
  }
  
  if(!auto_mode) serPrintf("\n\nData recieve:\n %02X %02X %02X\n ", dummy, *len-2, result);
  
  byte dat_rec[*len+2];
  byte crc[2] = {0, 0};

  int is = 0;
  for(int i = 0; i<*len; i++){
    if(i < *len-3){
      dat[i] = ds.read(); dat_rec[i+2] = dat[i];
      if(!auto_mode) serPrintf("%02X ", dat[i]);
    }else{
      byte b = ds.read();
      if(is < 2) crc[is] = b;   // byte ke-3 dibaca tapi tidak disimpan
      is++;
    }
  }

  dat_rec[0] =  *len-2; dat_rec[1] =  result;
  
  uint16_t calc_crc;
  uint16_t comb_crc = (crc[1]<<8) | (crc[0]); 
  calc_crc = CalculateCRC16(0x0000, dat_rec, *len-1);

  if(calc_crc == comb_crc){
    if(!auto_mode) serPrintf("\nCRC SUCCES:\n %04X [%04X]\n", comb_crc, calc_crc);
    return true;
  }
  if(!auto_mode) serPrintf("\nCRC FAILED:\n %04X [%04X]\n", comb_crc, calc_crc);
  return false;
}


void setup(void){
  pinMode(PB5,OUTPUT);
  pinMode(PB6,OUTPUT);
  pinMode(PC13,OUTPUT);
  digitalWrite(PC13,1);
  RED(1);
  GREEN(0);  
  
  delay(3000); 
  serial.begin(115200);
  // Tunggu laptop membuka port COM (maks 10 detik) agar "START!!" tidak hilang
  { uint32_t t_wait = millis(); while(!serial && (millis() - t_wait) < 10000) { } }
  serial.println("START!!");
#if ENABLE_MEASURE
  randomSeed(analogRead(PA0) ^ micros());
  stack_paint();
  print_ram_stats("BASELINE", measure_ram());
#endif
}

void loop(void) {
  byte addr[8];
  int dly = 30;
  byte command = 0xAA;
  byte para[50] = {0x00};
  byte param_len = 1;
  char buf[255];
  bool done = 0;
  int str_in = 0;
  bool fail = 0;

  LatencyStats lat = {0, 0, 0, 0, 0};
  RamStats ram_before = {0};
  bool do_measure = false;
  uint32_t t_total_start = 0;

  if(!begi){
    while(!done){
      if((millis()-red_timeot)<RED_TIME) RED(1);
      else RED(0);
      
      if((millis()-green_timeot)<GREEN_TIME) GREEN(1);
      else GREEN(0);
      
      if(ser.available()>0){
        if(str_in < 254) {
          buf[str_in] = ser.read();
          if(buf[str_in] == '\r') continue;   // abaikan CR (terminal CRLF)
          if(buf[str_in] == '\n'){
            buf[str_in] = (char) 0;
            done = 1;
            break;
          }
          str_in++;
        } else {
          buf[254] = '\0'; 
          done = 1;
          break;
        }
      }
    }
  }else{
    begi = 0;
    buf[0] = 'A';
    buf[1] = 'I';
    buf[2] = '\0';
  }
  
  ser.println(buf);

  if(buf[0] == 'A'){
    auto_mode = 1;
    switch(buf[1]){
      case 'I':{
        dly = 30; param_len = 0x1; command = 0xAA; para[0] = 0; break;
      }
      case 'R':{
        dly = 30; param_len = 0x1; command = 0x44; para[0] = buf[2]-uint8_t('0'); break;
      }
      case 'W':{
        dly = 60; param_len = 0x21; command = 0x96;
        strtok(buf,"|"); char *param = strtok(NULL,"|");
        if (param == NULL || strlen(param) < 64) { ser.println("-1"); return; }
        {
            char buff[3]; int u = 0; para[0] = buf[2]-uint8_t('0');
            for(int i=1; i<33; i++){
              buff[0] = param[u++]; buff[1] = param[u++]; buff[2] = char(0);
              para[i] = strtol(buff, NULL, 16);
            }
        }
        break;
      }
      case 'S':{
#if ENABLE_MEASURE
        do_measure    = true;
        t_total_start = micros();
        ram_before    = measure_ram();
#endif
        uint32_t t0 = micros();
        dly = 130; param_len = 0x21; command = 0xA5;
        strtok(buf,"|"); char *param = strtok(NULL,"|");
        if (param == NULL || strlen(param) < 64) { ser.println("-1"); return; }
        {
            char buff[3]; int u = 0; para[0] = buf[2]-uint8_t('0');
            for(int i=1; i<33; i++){
              buff[0] = param[u++]; buff[1] = param[u++]; buff[2] = char(0);
              para[i] = strtol(buff, NULL, 16);
            }
        }
        lat.t_parse_us = micros() - t0;
        break;
      }
      case 'U':{
#if ENABLE_MEASURE
        do_measure    = true;
        t_total_start = micros();
        ram_before    = measure_ram();
#endif
        uint32_t t0 = micros();
        dly = 130; param_len = 0x21; command = 0xA5;
        para[0] = buf[2]-uint8_t('0');
        for(int i=1; i<33; i++) para[i] = random(0x00,0x100);
        lat.t_parse_us = micros() - t0;
        break;
      }
      case 'L':{
        if(buf[2] == 'R') { red_timeot = millis(); ser.println("1"); }
        else if(buf[2] == 'G') { green_timeot = millis(); ser.println("1"); }
        else ser.println("-1");
        return;
      }
      default: ser.println("-1"); return;
    }
  }else{
    auto_mode = 0;
    char *tok1 = strtok(buf, "|");
    if(tok1 != NULL) dly = atoi(tok1);
    char *comm = strtok(NULL, "|");
    char *parm = strtok(NULL, "|");
    if (comm != NULL && parm != NULL) {
        char *t_len = strtok(comm, " ");
        char *t_cmd = strtok(NULL, " ");
        char *t_p0  = strtok(parm, " ");
        if (t_len == NULL || t_cmd == NULL || t_p0 == NULL) { ser.println("-1"); return; }
        param_len = strtol(t_len, NULL, 16) - 1;
        if (param_len < 1 || param_len > (int)sizeof(para)) { ser.println("-1"); return; }
        command = strtol(t_cmd, NULL, 16);
        para[0] = strtol(t_p0, NULL, 16);
        for(int x=1; x<param_len; x++){
          char *t = strtok(NULL, " ");
          if (t == NULL) { ser.println("-1"); return; }
          para[x] = strtol(t, NULL, 16);
        }
    }
  }

  digitalWrite(PC13, LOW);

  ds.reset_search();
  memset(addr, 0, sizeof(addr));
  if (!ds.search(addr) || OneWire::crc8(addr, 7) != addr[7]) {
      ser.println("-1"); digitalWrite(PC13, HIGH); return;
  }

  if(!auto_mode){
    ser.print("ROM =");
    for(int i = 0; i < 8; i++) { ser.write(' '); ser.print(addr[i], HEX); }
  }
  
  delay(1);
  ds.reset();
  
  ds.select(addr);

  // ---------- ukur ds_send ----------
  uint32_t t_send_start = micros();
  for(int i=0; i<2; i++){
    if(ds_send(command, para, param_len)){ fail = false; break; }
    else fail = true;
  }
  if(do_measure) lat.t_send_us = micros() - t_send_start;

  if(fail){ ser.println("-1"); digitalWrite(PC13, HIGH); return; }

  // ---------- ukur delay tunggu chip ----------
  uint32_t t_wait_start = micros();
  delay(dly);
  if(do_measure) lat.t_wait_us = micros() - t_wait_start;

  // ---------- ukur ds_read ----------
  byte data[DS_DATA_MAX];
  int len;
  uint32_t t_read_start = micros();
  for(int i=0; i<2; i++){
    if(ds_read(data,&len)){ fail = false; break; }
    else fail = true;
  }
  if(do_measure) lat.t_read_us = micros() - t_read_start;

  if(fail){ ser.println("-1"); digitalWrite(PC13, HIGH); return; }

  ds.reset();

  // Tanda tangan (0xA5) harus membawa minimal 64 byte data
  if(auto_mode && command == 0xA5 && (len - 3) < 64){
    ser.println("-1"); digitalWrite(PC13, HIGH); return;
  }

  // ---------- hitung total & cetak laporan ----------
  if(do_measure){
    lat.t_total_us = micros() - t_total_start;
    update_latency_stats(lat.t_total_us);
    RamStats ram_after = measure_ram();
    print_ram_stats("SIGN", ram_after);
    serial.printf("[RAM delta] used %+ld bytes\n", (long)ram_after.used - (long)ram_before.used);
    print_latency_report(lat);
  }

  if(auto_mode){
    switch(command){
      case 0xAA:{
        for(int i = 0; i < 8; i++) serPrintf("%02X",addr[i]);
        ser.print("|");
        for(int i = 0; i < len-3; i++) serPrintf("%02X",data[i]);
        ser.println(); break;
      }
      case 0x44:{
        for(int i = 0; i < len-3; i++) serPrintf("%02X",data[i]);
        ser.println(); break;
      }
      case 0x96:{ ser.println("1"); break; }
      case 0xA5:{
        for(int i = 1; i<33; i++) serPrintf("%02X",para[i]);
        ser.print("|");
        for(int i = 32; i<64; i++) serPrintf("%02X",data[i]);
        for(int i = 0; i<32; i++) serPrintf("%02X",data[i]);
        ser.println();
        break;
      }
    }
  }
  digitalWrite(PC13, HIGH);
}


