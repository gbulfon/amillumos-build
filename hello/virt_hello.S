| virt_hello.S - bare-metal hello for QEMU's m68k "virt" machine.
| Writes a string to the goldfish-tty PUT_CHAR register, then halts the
| machine through the virt-ctrl device so QEMU exits.
| Memory map (hw/m68k/virt.c): goldfish-tty at 0xff008000, virt-ctrl at 0xff009000.

#define GF_TTY_BASE     0xff008000
#define GF_TTY_PUT_CHAR 0x00
#define VIRT_CTRL_BASE  0xff009000
#define VIRT_CTRL_CMD   0x04
#define VIRT_CMD_HALT   2

        .text
        .globl  _start
_start:
        lea     GF_TTY_BASE, %a0
        lea     msg(%pc), %a1
1:      move.b  (%a1)+, %d0
        beq     2f
        move.l  %d0, GF_TTY_PUT_CHAR(%a0)
        bra     1b
2:      lea     VIRT_CTRL_BASE, %a0
        move.l  #VIRT_CMD_HALT, VIRT_CTRL_CMD(%a0)
3:      stop    #0x2700
        bra     3b

msg:    .asciz  "Hello from Amillumos via goldfish-tty\r\n"
        .align  2
