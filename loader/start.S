| start.S - entry of the amiboot loader (AmigaDOS executable) and the exec/dos stubs.
| All code is position independent: the image is wrapped by tools/mkhunk.py without
| relocations. C ABI (m68k-elf gcc): args on the stack, scalars return in %d0, pointers
| in %a0 (and %d0). %d2-%d7/%a2-%a6 are callee-saved.

        .section .text.start,"ax"
        .globl  _start
_start:
        movem.l %d2-%d7/%a2-%a6,-(%sp)
        move.l  %a0,%a2                 | command line pointer
        move.l  %d0,%d2                 | command line length
        | zero .bss (LoadSeg does not guarantee cleared memory)
        lea     __bss_start(%pc),%a0
        lea     _end(%pc),%a1
1:      cmp.l   %a0,%a1
        bls     2f
        clr.l   (%a0)+
        bra     1b
2:      move.l  %d2,-(%sp)
        move.l  %a2,-(%sp)
        bsr     loader_main
        addq.l  #8,%sp
        movem.l (%sp)+,%d2-%d7/%a2-%a6
        rts

        .text
| ---- exec.library ----
        .globl  exec_OpenLibrary        | void *exec_OpenLibrary(const char *, unsigned long)
exec_OpenLibrary:
        move.l  %a6,-(%sp)
        move.l  8(%sp),%a1
        move.l  12(%sp),%d0
        move.l  4.w,%a6
        jsr     -552(%a6)
        move.l  (%sp)+,%a6
        move.l  %d0,%a0
        rts
        .globl  exec_CloseLibrary       | void exec_CloseLibrary(void *)
exec_CloseLibrary:
        move.l  %a6,-(%sp)
        move.l  8(%sp),%a1
        move.l  4.w,%a6
        jsr     -414(%a6)
        move.l  (%sp)+,%a6
        rts
        .globl  exec_AllocMem           | void *exec_AllocMem(unsigned long size, unsigned long attrs)
exec_AllocMem:
        move.l  %a6,-(%sp)
        move.l  8(%sp),%d0
        move.l  12(%sp),%d1
        move.l  4.w,%a6
        jsr     -198(%a6)
        move.l  (%sp)+,%a6
        move.l  %d0,%a0
        rts
        .globl  exec_FreeMem            | void exec_FreeMem(void *, unsigned long)
exec_FreeMem:
        move.l  %a6,-(%sp)
        move.l  8(%sp),%a1
        move.l  12(%sp),%d0
        move.l  4.w,%a6
        jsr     -210(%a6)
        move.l  (%sp)+,%a6
        rts
        .globl  exec_Forbid
exec_Forbid:
        move.l  %a6,-(%sp)
        move.l  4.w,%a6
        jsr     -132(%a6)
        move.l  (%sp)+,%a6
        rts
        .globl  exec_Permit
exec_Permit:
        move.l  %a6,-(%sp)
        move.l  4.w,%a6
        jsr     -138(%a6)
        move.l  (%sp)+,%a6
        rts
        .globl  exec_Disable
exec_Disable:
        move.l  %a6,-(%sp)
        move.l  4.w,%a6
        jsr     -120(%a6)
        move.l  (%sp)+,%a6
        rts
        .globl  exec_Supervisor         | void exec_Supervisor(void (*fn)(void))  (never returns for us)
exec_Supervisor:
        move.l  %a5,-(%sp)
        move.l  %a6,-(%sp)
        move.l  12(%sp),%a5
        move.l  4.w,%a6
        jsr     -30(%a6)
        move.l  (%sp)+,%a6
        move.l  (%sp)+,%a5
        rts
        .globl  exec_base               | void *exec_base(void)
exec_base:
        move.l  4.w,%a0
        move.l  %a0,%d0
        rts

| ---- dos.library (base passed explicitly) ----
        .globl  dos_Open                | long dos_Open(void *dosbase, const char *name, long mode)
dos_Open:
        move.l  %a6,-(%sp)
        move.l  %d2,-(%sp)
        move.l  12(%sp),%a6
        move.l  16(%sp),%d1
        move.l  20(%sp),%d2
        jsr     -30(%a6)
        move.l  (%sp)+,%d2
        move.l  (%sp)+,%a6
        rts
        .globl  dos_Close               | long dos_Close(void *dosbase, long fh)
dos_Close:
        move.l  %a6,-(%sp)
        move.l  8(%sp),%a6
        move.l  12(%sp),%d1
        jsr     -36(%a6)
        move.l  (%sp)+,%a6
        rts
        .globl  dos_Read                | long dos_Read(void *dosbase, long fh, void *buf, long len)
dos_Read:
        move.l  %a6,-(%sp)
        movem.l %d2-%d3,-(%sp)
        move.l  16(%sp),%a6
        move.l  20(%sp),%d1
        move.l  24(%sp),%d2
        move.l  28(%sp),%d3
        jsr     -42(%a6)
        movem.l (%sp)+,%d2-%d3
        move.l  (%sp)+,%a6
        rts
        .globl  dos_Seek                | long dos_Seek(void *dosbase, long fh, long pos, long mode)
dos_Seek:
        move.l  %a6,-(%sp)
        movem.l %d2-%d3,-(%sp)
        move.l  16(%sp),%a6
        move.l  20(%sp),%d1
        move.l  24(%sp),%d2
        move.l  28(%sp),%d3
        jsr     -66(%a6)
        movem.l (%sp)+,%d2-%d3
        move.l  (%sp)+,%a6
        rts

| ---- the point of no return ----
| Runs in supervisor mode via exec Supervisor(). Masks interrupts, moves to the kernel
| stack, pushes and disables the caches, turns the MMU off and jumps to the kernel.
        .globl  boot_trampoline
boot_trampoline:
        move.w  #0x2700,%sr
        lea     boot_info_ptr(%pc),%a0
        move.l  (%a0),%a0               | a0 = struct amiboot_info *
        move.l  16(%a0),%sp             | stack_top
        move.l  8(%a0),%a1              | load_addr
        lea     boot_entry_off(%pc),%a2
        add.l   (%a2),%a1               | a1 = entry address
        | kill chipset interrupts (INTENA) and pending requests (INTREQ)
        lea     0xDFF000,%a3
        move.w  #0x7FFF,0x9A(%a3)
        move.w  #0x7FFF,0x9C(%a3)
        | caches: push dirty data, invalidate, then disable
        nop
        cpusha  %bc
        moveq   #0,%d0
        movec   %d0,%cacr
        cinva   %bc
        | MMU off: TC, transparent translations, flush ATC
        movec   %d0,%tc
        movec   %d0,%itt0
        movec   %d0,%itt1
        movec   %d0,%dtt0
        movec   %d0,%dtt1
        pflusha
        nop
        jmp     (%a1)

        .data
        .globl  boot_info_ptr, boot_entry_off
boot_info_ptr:  .long 0
boot_entry_off: .long 0

        .text
        .globl  dos_IoErr               | long dos_IoErr(void *dosbase)
dos_IoErr:
        move.l  %a6,-(%sp)
        move.l  8(%sp),%a6
        jsr     -132(%a6)
        move.l  (%sp)+,%a6
        rts
