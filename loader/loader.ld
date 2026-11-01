/* Loader: single contiguous hunk; must be position independent (PC-relative only). */
OUTPUT_ARCH(m68k)
ENTRY(_start)
SECTIONS {
  . = 0;
  .text : { KEEP(*(.text.start)) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data : { *(.data .data.*) }
  . = ALIGN(4);
  __bss_start = .;
  .bss : { *(.bss .bss.*) *(COMMON) }
  . = ALIGN(4);
  _end = .;
  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame) }
}
