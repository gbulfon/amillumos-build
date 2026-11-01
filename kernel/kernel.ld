/* Hello kernel: flat, position-independent image with an amikern_hdr at offset 0. */
OUTPUT_ARCH(m68k)
ENTRY(_start)
SECTIONS {
  . = BASE;
  .text : { KEEP(*(.text.hdr)) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data : { *(.data .data.*) }
  . = ALIGN(16);
  __bss_start = .;
  __image_size = __bss_start - _start;
  .bss : { *(.bss .bss.*) *(COMMON) }
  _end = .;			/* exactly the end of the loaded image: QEMU's bootinfo follows it */
  . = ALIGN(16);
  __bss_size = . - __bss_start;
  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame) }
}
