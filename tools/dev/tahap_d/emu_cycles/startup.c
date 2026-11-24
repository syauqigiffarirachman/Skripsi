// Startup + dukungan newlib minimal + penanda & pengukur stack untuk emulator.
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
extern uint32_t _estack, _sidata, _sdata, _edata, _sbss, _ebss, _end;
extern int main(void);

void Reset_Handler(void) {
  uint32_t *s = &_sidata, *d = &_sdata;
  while (d < &_edata) *d++ = *s++;
  for (d = &_sbss; d < &_ebss; ) *d++ = 0;
  main();
  extern void finish(void); finish();
}
__attribute__((section(".isr_vector"), used))
const uint32_t vectors[2] = { (uint32_t)&_estack, (uint32_t)Reset_Handler };

// Anggaran RAM identik firmware: stack <= STACK_BYTES dari puncak RAM; heap = sisanya.
#define STACK_BYTES 8192
#define PAINT 0xA5A5A5A5u
static char *heap_end = 0;
static char *heap_limit(void) { return (char *)&_estack - STACK_BYTES; }
void *_sbrk(int incr) {
  if (!heap_end) heap_end = (char *)&_end;
  char *prev = heap_end;
  if (heap_end + incr > heap_limit()) { errno = ENOMEM; return (void *)-1; }
  heap_end += incr; return prev;
}
uint32_t heap_used_sbrk(void) { return (uint32_t)(heap_end ? heap_end - (char *)&_end : 0); }

__attribute__((noinline)) void marker_begin(void) { __asm volatile(""); }
__attribute__((noinline)) void marker_end(void)   { __asm volatile(""); }
__attribute__((noinline)) void finish(void)       { for (;;) { } }

void stack_paint(void) {
  uint32_t sp; __asm volatile("mov %0, sp" : "=r"(sp));
  for (uint32_t *p = (uint32_t *)heap_limit(); p < (uint32_t *)(sp - 64); p++) *p = PAINT;
}
uint32_t stack_used(void) {
  uint32_t *p = (uint32_t *)heap_limit();
  while (p < (uint32_t *)&_estack && *p == PAINT) p++;
  return (uint32_t)&_estack - (uint32_t)p;
}
// syscall stub lain yang mungkin ditarik newlib-nano
int _close(int f){return -1;} int _fstat(int f,void*s){return 0;} int _isatty(int f){return 1;}
int _lseek(int f,int o,int w){return 0;} int _read(int f,char*b,int n){return 0;} int _write(int f,char*b,int n){return n;}
void _exit(int c){ for(;;){} } int _kill(int p,int s){return -1;} int _getpid(void){return 1;}
