| start.S - Amiga image header and entry. Entered by amiboot in supervisor mode,
| SR=0x2700, MMU and caches off, %a0 = struct amiboot_info *, %sp = stack_top.
        .section .text.hdr,"ax"
        .globl  _start
_start:
        .ascii  "AMIL"                  | amikern_hdr.magic
        .long   1                       | version
        .long   __image_size            | bytes in the file
        .long   __bss_size              | bytes to clear after it
        .long   entry - _start          | entry offset
        .long   8192                    | load alignment

        .text
entry:
        move.w  #0x2700,%sr
        move.l  16(%a0),%sp             | stack_top
        lea     vector_table(%pc),%a1
        movec   %a1,%vbr
        move.l  %a0,-(%sp)
        bsr     kmain
        bra     cpu_halt
