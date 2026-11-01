| start_virt.S - entry for QEMU's m68k "virt" machine (-kernel kernel-virt.elf).
| QEMU starts us at the ELF entry in supervisor mode with no usable stack and a
| Linux-style bootinfo record list just past the end of the image.
        .section .text.hdr,"ax"
        .globl  _start
_start:
        move.w  #0x2700,%sr
        lea     _start(%pc),%a0
        lea     0x100000(%a0),%sp       | 1 MB above the image: free RAM
        lea     vector_table(%pc),%a1
        movec   %a1,%vbr
        lea     _end(%pc),%a0
        move.l  %a0,-(%sp)
        bsr     virt_entry              | builds an amiboot_info and calls kmain
        bra     cpu_halt
