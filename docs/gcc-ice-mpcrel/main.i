# 0 "main.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "main.c"
# 11 "main.c"
# 1 "../include/amiboot.h" 1
# 17 "../include/amiboot.h"
struct amikern_hdr {
 unsigned long magic;
 unsigned long version;
 unsigned long image_size;
 unsigned long bss_size;
 unsigned long entry;
 unsigned long align;
};

struct amiboot_mem {
 unsigned long lower;
 unsigned long upper;
 unsigned long attrs;
};

struct amiboot_info {
 unsigned long magic;
 unsigned long version;
 unsigned long load_addr;
 unsigned long load_size;
 unsigned long stack_top;
 unsigned long attn_flags;
 unsigned long nmem;
 struct amiboot_mem mem[16];
 char cmdline[256];
};
# 12 "main.c" 2
# 1 "plat.h" 1



typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;

struct plat {
 const char *name;
 void (*putc)(char);
 void (*flush)(void);
 void (*map_platform)(void);
 void (*halt)(void) __attribute__((noreturn));
 ulong tt[2];
 ulong fault_test_addr;
};
extern const struct plat *plat;


void map_range(ulong va, ulong pa, ulong len, ulong cm, ulong flags);
# 30 "plat.h"
void exc_generic(void);
ulong get_pc(void);
void cpu_halt(void) __attribute__((noreturn));
extern ulong vector_table[256];
# 13 "main.c" 2

void plat_early_init(void);



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
# 52 "main.c"
static ulong root_table[128] __attribute__((aligned(512)));



static ulong *table_pool;
static ulong pool_used;


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
 while (pool_used % entries)
  pool_used++;
 if (pool_used + entries > (256UL * 1024) / sizeof(ulong)) {
  puts_("panic: page table pool exhausted\n");
  (plat->halt());
 }
 t = &table_pool[pool_used];
 pool_used += entries;
 return t;
}

static void map_page(ulong va, ulong pa, ulong cm, ulong flags)
{
 ulong ri = (va >> 25) & 0x7F, pi = (va >> 18) & 0x7F, gi = (va >> 12) & 0x3F;
 ulong *ptr, *pg;

 if (!(root_table[ri] & 0x3))
  root_table[ri] = (ulong)alloc_table(128) | 0x3;
 ptr = (ulong *)(root_table[ri] & ~0x1FFUL);
 if (!(ptr[pi] & 0x3))
  ptr[pi] = (ulong)alloc_table(64) | 0x3;
 pg = (ulong *)(ptr[pi] & ~0xFFUL);
 pg[gi] = (pa & (~(4096UL - 1))) | (cm << 5) | flags | 0x1;
}

void map_range(ulong va, ulong pa, ulong len, ulong cm, ulong flags)
{
 ulong end = (va + len + 4096UL - 1) & (~(4096UL - 1));
 for (va &= (~(4096UL - 1)), pa &= (~(4096UL - 1)); va < end; va += 4096UL, pa += 4096UL)
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
     "movec %3,%%tc\n\t"
     "nop\n\t"
     "movec %4,%%cacr\n\t"
     "nop"
     : : "d"(root), "d"(tt0), "d"(tt1), "d"(0x8000UL), "d"(0x80008000UL) : "memory");
}

static ulong mmu_ptest(ulong va)
{
 ulong mmusr;
 asm volatile("ptestr (%1)\n\tmovec %%mmusr,%0" : "=d"(mmusr) : "a"(va) : "memory");
 return mmusr;
}


struct fault_frame {
 ushort sr; ulong pc; ushort fv; ulong ea;
 ushort ssw, wb3s, wb2s, wb1s; ulong fa;
 ulong wb3a, wb3d, wb2a, wb2d, wb1a, wb1d, pd1, pd2, pd3;
} __attribute__((packed));

static volatile ulong fault_resume;
static volatile ulong fault_count;
static volatile ulong last_fault_addr;

void exc_c_handler(struct fault_frame *f)
{
 ulong vec = (f->fv & 0xFFF) >> 2;

 if (vec == 2 && fault_resume) {
  last_fault_addr = f->fa;
  fault_count++;
  f->pc = fault_resume;
  f->ssw &= 0x0FFF;
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
 (plat->halt());
}




static void __attribute__((noinline)) stage2(void)
{
 ulong pc = get_pc(), v;

 puts_("stage2: running at pc "); puthex(pc);
 puts_(pc >= 0xC0000000UL ? " (kernel virtual alias)\n" : " (NOT the alias?!)\n");
 puts_("stage2: ptest "); puthex(pc); puts_(" -> mmusr "); puthex(mmu_ptest(pc)); puts_("\n");


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
 (plat->halt());
}


void kmain(struct amiboot_info *bi)
{
 ulong i, here = get_pc(), lo, hi, len, alias_off;
 void (*alias_stage2)(void);

 plat_early_init();
 puts_("\nAmillumos hello kernel, build " "Nov  1 2026" " " "12:24:22" ", platform ");
 puts_(plat->name); puts_("\n");
 puts_("kernel: pc "); puthex(here); puts_(" sp ");
 { ulong sp; asm("move.l %%sp,%0" : "=d"(sp)); puthex(sp); }
 puts_(" bootinfo "); puthex((ulong)bi); puts_("\n");

 if (bi->magic != 0x414D494CUL) { puts_("kernel: bad bootinfo magic\n"); (plat->halt()); }
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


 table_pool = 0;
 for (i = 0; i < bi->nmem; i++) {
  ulong top, cand;
  if (bi->mem[i].attrs & 2)
   continue;
  top = bi->mem[i].upper & (~(4096UL - 1));
  cand = top - (256UL * 1024);
  if (cand < bi->mem[i].lower)
   continue;
  if (cand < bi->stack_top + 16 && top > bi->load_addr)
   continue;
  if (cand < (ulong)bi + sizeof(*bi) && top > (ulong)bi)
   continue;
  if (!table_pool || (top - cand) >= (256UL * 1024))
   table_pool = (ulong *)cand;
 }
 if (!table_pool) { puts_("kernel: no room for page tables\n"); (plat->halt()); }
 for (i = 0; i < (256UL * 1024) / sizeof(ulong); i++)
  table_pool[i] = 0;
 puts_("kernel: page table pool at "); puthex((ulong)table_pool); puts_("\n");


 plat->map_platform();

 for (i = 0; i < bi->nmem; i++) {
  lo = bi->mem[i].lower & (~(4096UL - 1));
  hi = (bi->mem[i].upper + 4096UL - 1) & (~(4096UL - 1));
  if (in_ttr(lo) || (bi->mem[i].attrs & 2))
   continue;
  map_range(lo, lo, hi - lo, 0, 0);
 }

 len = (bi->load_size + 4096UL - 1) & (~(4096UL - 1));
 map_range(0xC0000000UL, bi->load_addr, len, 0, 0);
 puts_("kernel: page tables built, "); putdec(pool_used * 4 / 1024);
 puts_(" KB used, root at "); puthex((ulong)root_table); puts_("\n");
 flush_();

 mmu_enable();
 puts_("kernel: MMU enabled (TC=0x8000, 4 KB pages), still alive at pc ");
 puthex(get_pc()); puts_("\n");
 puts_("kernel: ptest kernel alias -> mmusr "); puthex(mmu_ptest(0xC0000000UL)); puts_("\n");
 puts_("kernel: ptest unmapped "); puthex(plat->fault_test_addr); puts_(" -> mmusr ");
 puthex(mmu_ptest(plat->fault_test_addr)); puts_("\n");

 alias_off = (ulong)stage2 - bi->load_addr;
 alias_stage2 = (void (*)(void))(0xC0000000UL + alias_off);
 puts_("kernel: jumping to stage2 through the alias at "); puthex((ulong)alias_stage2); puts_("\n");
 flush_();
 alias_stage2();
 (plat->halt());
}
