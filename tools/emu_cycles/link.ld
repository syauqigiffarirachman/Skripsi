/* Harness emulator: peta memori STM32F103C8 (Flash 128K dipetakan, RAM 20K = RAM nyata) */
MEMORY { FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 128K
         RAM  (rwx): ORIGIN = 0x20000000, LENGTH = 20K }
_estack = ORIGIN(RAM) + LENGTH(RAM);
ENTRY(Reset_Handler)
SECTIONS {
  .isr_vector : { KEEP(*(.isr_vector)) } > FLASH
  .text : { *(.text*) *(.rodata*) . = ALIGN(4); } > FLASH
  .ARM.exidx : { *(.ARM.exidx*) } > FLASH
  _sidata = LOADADDR(.data);
  .data : { _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT > FLASH
  .bss  : { _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM
  _end = .;
  /DISCARD/ : { *(.ARM.attributes) }
}
