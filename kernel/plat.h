/* plat.h - what the hello kernel needs from a platform (plain functions: GCC 16's m68k
 * backend ICEs on calls through function pointers in const structs with -mpcrel). */
#ifndef PLAT_H
#define PLAT_H
typedef unsigned long  ulong;
typedef unsigned short ushort;
typedef unsigned char  uchar;

extern const char plat_name[];
extern const ulong plat_tt[2];		/* transparent translation registers, 0 = unused */
extern const ulong plat_fault_test_addr;	/* an address nothing maps */
void plat_early_init(void);
void plat_putc(ulong c);
void plat_flush(void);
void plat_map_platform(void);		/* identity-map chip RAM, I/O, ROM via page tables */
void plat_halt(void) __attribute__((noreturn));

/* main.c services available to platforms */
void map_range(ulong va, ulong pa, ulong len, ulong cm, ulong flags);
#define CM_WRITETHROUGH  0
#define CM_COPYBACK      1
#define CM_NOCACHE_SER   2
#define CM_NOCACHE       3
#define PD_WP            0x4
#define TTR(base, mask, cm) \
	(((ulong)(base) << 24) | ((ulong)(mask) << 16) | (1UL << 15) | (2UL << 13) | ((ulong)(cm) << 5))

/* common.S */
void exc_generic(void);
ulong get_pc(void);
void cpu_halt(void) __attribute__((noreturn));
extern ulong vector_table[256];
#endif
