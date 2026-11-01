/*
 * main.c - Amillumos hello kernel for the Amiga 4000 / 68040.
 *
 * Proves the bring-up loop before any illumos code exists:
 *   1. we are running where amiboot put us, in supervisor mode, interrupts masked;
 *   2. the Paula serial port works as a console;
 *   3. 68040 page tables + transparent translation registers can be built and enabled;
 *   4. the image can be reached through a second, "kernel" virtual mapping;
 *   5. an access fault reaches our vector table and can be resumed.
 */
#include "../include/amiboot.h"
#include "plat.h"

void plat_early_init(void);
#define halt() (plat->halt())

/* ------------------------------------------------------------ console */
static void putc_(char c) { plat->putc(c); }
static void puts_(const char *s)
{
	for (; *s; s++) {
		if (*s == '\n')
			putc_('\r');
		putc_(*s);
	}
}
static void puthex(ulong v)
{
	static const char hex[] = "0123456789abcdef";
	int i;
	puts_("0x");
	for (i = 28; i >= 0; i -= 4)
		putc_(hex[(v >> i) & 15]);
}
static void putdec(ulong v)
{
	char buf[12]; int i = 11;
	buf[i] = 0;
	do { buf[--i] = '0' + v % 10; v /= 10; } while (v);
	puts_(buf + i);
}
static void flush_(void) { plat->flush(); }

/* ------------------------------------------------------------ 68040 MMU */
#define PAGE_SIZE   4096UL
#define PAGE_MASK   (~(PAGE_SIZE - 1))

#define UDT_RESIDENT 0x3      /* table descriptor: pointer valid */
#define PDT_RESIDENT 0x1      /* page descriptor: page valid */
#define PD_S         0x80     /* supervisor only */

static ulong root_table[128] __attribute__((aligned(512)));
/* Page-table pool: carved out of free RAM at run time (keeping it in .bss would push
 * PC-relative references past the 16-bit displacement -mpcrel allows). */
#define POOL_BYTES  (256UL * 1024)
static ulong *table_pool;
static ulong pool_used;		/* in longwords */

/* Does a platform TTR cover this physical address? (base[31:24], mask[23:16], E[15]) */
static int in_ttr(ulong pa)
{
	int i;
	for (i = 0; i < 2; i++) {
		ulong tt = plat->tt[i];
		if ((tt & (1UL << 15)) && ((((pa >> 24) ^ (tt >> 24)) & ~(tt >> 16) & 0xFF) == 0))
			return 1;
	}
	return 0;
}

static ulong *alloc_table(ulong entries)
{
	ulong *t;
	while (pool_used % entries)		/* keep tables naturally aligned */
		pool_used++;
	if (pool_used + entries > POOL_BYTES / sizeof(ulong)) {
		puts_("panic: page table pool exhausted\n");
		halt();
	}
	t = &table_pool[pool_used];
	pool_used += entries;
	return t;
}

static void map_page(ulong va, ulong pa, ulong cm, ulong flags)
{
	ulong ri = (va >> 25) & 0x7F, pi = (va >> 18) & 0x7F, gi = (va >> 12) & 0x3F;
	ulong *ptr, *pg;

	if (!(root_table[ri] & UDT_RESIDENT))
		root_table[ri] = (ulong)alloc_table(128) | UDT_RESIDENT;
	ptr = (ulong *)(root_table[ri] & ~0x1FFUL);
	if (!(ptr[pi] & UDT_RESIDENT))
		ptr[pi] = (ulong)alloc_table(64) | UDT_RESIDENT;
	pg = (ulong *)(ptr[pi] & ~0xFFUL);
	pg[gi] = (pa & PAGE_MASK) | (cm << 5) | flags | PDT_RESIDENT;
}

void map_range(ulong va, ulong pa, ulong len, ulong cm, ulong flags)
{
	ulong end = (va + len + PAGE_SIZE - 1) & PAGE_MASK;
	for (va &= PAGE_MASK, pa &= PAGE_MASK; va < end; va += PAGE_SIZE, pa += PAGE_SIZE)
		map_page(va, pa, cm, flags);
}

static void mmu_enable(void)
{
	ulong root = (ulong)root_table;
	ulong tt0 = plat->tt[0], tt1 = plat->tt[1];
	asm volatile(
	    "movec %0,%%srp\n\t"
	    "movec %0,%%urp\n\t"
	    "movec %1,%%itt0\n\t"
	    "movec %1,%%dtt0\n\t"
	    "movec %2,%%itt1\n\t"
	    "movec %2,%%dtt1\n\t"
	    "cinva %%bc\n\t"
	    "pflusha\n\t"
	    "movec %3,%%tc\n\t"		/* E=1, 4 KB pages */
	    "nop\n\t"
	    "movec %4,%%cacr\n\t"	/* enable instruction and data caches */
	    "nop"
	    : : "d"(root), "d"(tt0), "d"(tt1), "d"(0x8000UL), "d"(0x80008000UL) : "memory");
}

static ulong mmu_ptest(ulong va)
{
	ulong mmusr;
	asm volatile("ptestr (%1)\n\tmovec %%mmusr,%0" : "=d"(mmusr) : "a"(va) : "memory");
	return mmusr;
}

/* ------------------------------------------------------------ exceptions */
struct fault_frame {			/* 68040 format 7 access error frame */
	ushort sr; ulong pc; ushort fv; ulong ea;
	ushort ssw, wb3s, wb2s, wb1s; ulong fa;
	ulong wb3a, wb3d, wb2a, wb2d, wb1a, wb1d, pd1, pd2, pd3;
} __attribute__((packed));

static volatile ulong fault_resume;	/* PC to resume at after an expected fault */
static volatile ulong fault_count;
static volatile ulong last_fault_addr;

void exc_c_handler(struct fault_frame *f)
{
	ulong vec = (f->fv & 0xFFF) >> 2;

	if (vec == 2 && fault_resume) {
		last_fault_addr = f->fa;
		fault_count++;
		f->pc = fault_resume;
		f->ssw &= 0x0FFF;		/* drop CP/CU/CT/CM continuation bits */
		f->wb1s &= ~0x80; f->wb2s &= ~0x80; f->wb3s &= ~0x80;
		return;
	}
	puts_("\npanic: exception vector "); putdec(vec);
	puts_(" at pc "); puthex(f->pc); puts_(" sr "); puthex(f->sr);
	if ((f->fv >> 12) == 7) {
		puts_("\n  access error: fa "); puthex(f->fa);
		puts_(" ea "); puthex(f->ea); puts_(" ssw "); puthex(f->ssw);
	}
	puts_("\n");
	flush_();
	halt();
}

/* ------------------------------------------------------------ stage 2: runs via the alias */
#define KERNEL_VBASE 0xC0000000UL

static void __attribute__((noinline)) stage2(void)
{
	ulong pc = get_pc(), v;

	puts_("stage2: running at pc "); puthex(pc);
	puts_(pc >= KERNEL_VBASE ? " (kernel virtual alias)\n" : " (NOT the alias?!)\n");
	puts_("stage2: ptest "); puthex(pc); puts_(" -> mmusr "); puthex(mmu_ptest(pc)); puts_("\n");

	/* an access to an address no table or TTR covers must fault and resume here */
	fault_count = 0;
	asm volatile(
	    "lea 1f(%%pc),%%a1\n\t"
	    "lea fault_resume(%%pc),%%a0\n\t"
	    "move.l %%a1,(%%a0)\n\t"
	    "move.l %1,%%a1\n\t"
	    "move.l (%%a1),%0\n"
	    "1:\n\t"
	    "lea fault_resume(%%pc),%%a0\n\t"
	    "clr.l (%%a0)"
	    : "=d"(v) : "d"(plat->fault_test_addr) : "a0", "a1", "memory");
	if (fault_count == 1 && last_fault_addr == plat->fault_test_addr) {
		puts_("stage2: access fault at "); puthex(last_fault_addr);
		puts_(" caught and resumed: fault ok\n");
	}
	else {
		puts_("stage2: expected one fault, got "); putdec(fault_count);
		puts_(" (fa "); puthex(last_fault_addr); puts_(")\n");
	}
	puts_("stage2: all done, halting. HALT\n");
	flush_();
	halt();
}

/* ------------------------------------------------------------ main */
void kmain(struct amiboot_info *bi)
{
	ulong i, here = get_pc(), lo, hi, len, alias_off;
	void (*alias_stage2)(void);

	plat_early_init();
	puts_("\nAmillumos hello kernel, build " __DATE__ " " __TIME__ ", platform ");
	puts_(plat->name); puts_("\n");
	puts_("kernel: pc "); puthex(here); puts_(" sp ");
	{ ulong sp; asm("move.l %%sp,%0" : "=d"(sp)); puthex(sp); }
	puts_(" bootinfo "); puthex((ulong)bi); puts_("\n");

	if (bi->magic != AMIBOOT_MAGIC) { puts_("kernel: bad bootinfo magic\n"); halt(); }
	puts_("kernel: loaded at "); puthex(bi->load_addr); puts_(" size ");
	putdec(bi->load_size); puts_(" attn "); puthex(bi->attn_flags);
	puts_(" cmdline '"); puts_(bi->cmdline); puts_("'\n");
	for (i = 0; i < bi->nmem; i++) {
		puts_("kernel: mem "); puthex(bi->mem[i].lower); puts_("-");
		puthex(bi->mem[i].upper); puts_((bi->mem[i].attrs & 2) ? " chip" : " fast");
		puts_(in_ttr(bi->mem[i].lower) ? " (TTR)\n" : " (page tables)\n");
	}

	for (i = 0; i < 256; i++)
		vector_table[i] = (ulong)exc_generic;

	/* page-table pool: top of the largest fast range that does not hold us */
	table_pool = 0;
	for (i = 0; i < bi->nmem; i++) {
		ulong top, cand;
		if (bi->mem[i].attrs & 2)
			continue;
		top = bi->mem[i].upper & PAGE_MASK;
		cand = top - POOL_BYTES;
		if (cand < bi->mem[i].lower)
			continue;
		if (cand < bi->stack_top + 16 && top > bi->load_addr)
			continue;			/* overlaps image/stack */
		if (cand < (ulong)bi + sizeof(*bi) && top > (ulong)bi)
			continue;			/* overlaps bootinfo */
		if (!table_pool || (top - cand) >= POOL_BYTES)
			table_pool = (ulong *)cand;
	}
	if (!table_pool) { puts_("kernel: no room for page tables\n"); halt(); }
	for (i = 0; i < POOL_BYTES / sizeof(ulong); i++)
		table_pool[i] = 0;
	puts_("kernel: page table pool at "); puthex((ulong)table_pool); puts_("\n");

	/* platform: chip RAM, I/O, ROM */
	plat->map_platform();
	/* identity map every fast memory range not already covered by a TTR */
	for (i = 0; i < bi->nmem; i++) {
		lo = bi->mem[i].lower & PAGE_MASK;
		hi = (bi->mem[i].upper + PAGE_SIZE - 1) & PAGE_MASK;
		if (in_ttr(lo) || (bi->mem[i].attrs & 2))
			continue;
		map_range(lo, lo, hi - lo, CM_WRITETHROUGH, 0);
	}
	/* the kernel's own virtual home */
	len = (bi->load_size + PAGE_SIZE - 1) & PAGE_MASK;
	map_range(KERNEL_VBASE, bi->load_addr, len, CM_WRITETHROUGH, 0);
	puts_("kernel: page tables built, "); putdec(pool_used * 4 / 1024);
	puts_(" KB used, root at "); puthex((ulong)root_table); puts_("\n");
	flush_();

	mmu_enable();
	puts_("kernel: MMU enabled (TC=0x8000, 4 KB pages), still alive at pc ");
	puthex(get_pc()); puts_("\n");
	puts_("kernel: ptest kernel alias -> mmusr "); puthex(mmu_ptest(KERNEL_VBASE)); puts_("\n");
	puts_("kernel: ptest unmapped "); puthex(plat->fault_test_addr); puts_(" -> mmusr ");
	puthex(mmu_ptest(plat->fault_test_addr)); puts_("\n");

	alias_off = (ulong)stage2 - bi->load_addr;
	alias_stage2 = (void (*)(void))(KERNEL_VBASE + alias_off);
	puts_("kernel: jumping to stage2 through the alias at "); puthex((ulong)alias_stage2); puts_("\n");
	flush_();
	alias_stage2();
	halt();
}
