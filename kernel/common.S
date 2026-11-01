| common.S - exception glue and helpers shared by all platforms. Position independent.
        .text
| Generic exception entry: hands the frame to C. Vector number comes from the frame.
        .globl  exc_generic
exc_generic:
        movem.l %d0-%d1/%a0-%a1,-(%sp)
        lea     16(%sp),%a0
        move.l  %a0,-(%sp)
        bsr     exc_c_handler
        addq.l  #4,%sp
        movem.l (%sp)+,%d0-%d1/%a0-%a1
        rte

        .globl  get_pc                  | unsigned long get_pc(void): address after the call
get_pc:
        move.l  (%sp),%d0
        rts

        .globl  cpu_halt                | void cpu_halt(void): never returns
cpu_halt:
        stop    #0x2700
        bra     cpu_halt

        .bss
        .balign 1024
        .globl  vector_table
vector_table:
        .space  256*4
