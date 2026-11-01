| serial_hello.S - AmigaDOS executable that writes a string straight to the
| Paula serial port (SERPER/SERDAT), bypassing the OS. Used to verify that the
| emulator's serial output reaches the host. Position independent, no relocs.

#define CUSTOM   0xDFF000
#define SERDAT   0x030
#define SERPER   0x032
#define SERDATR  0x018
#define TBE_BIT  13
#define TSRE_BIT 12
| SERPER = (3546895 / baud) - 1 on PAL, 3579545 on NTSC; 9600 baud -> 368/372.
#define BAUD_DIV 368

        .text
        .globl  _start
_start:
        lea     CUSTOM, %a0
        move.w  #BAUD_DIV, SERPER(%a0)
        lea     msg(%pc), %a1
1:      moveq   #0, %d0
        move.b  (%a1)+, %d0
        beq     3f
2:      move.w  SERDATR(%a0), %d1
        btst    #TBE_BIT, %d1
        beq     2b
        or.w    #0x0100, %d0            | stop bit
        move.w  %d0, SERDAT(%a0)
        bra     1b
3:      move.w  SERDATR(%a0), %d1       | wait for the shift register to drain
        btst    #TSRE_BIT, %d1
        beq     3b
        moveq   #0, %d0                 | return code 0
        rts

msg:    .asciz  "Hello from Amillumos via Paula serial\r\n"
        .align  2
