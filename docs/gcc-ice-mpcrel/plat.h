/* plat.h - what the hello kernel needs from a platform. */
#ifndef PLAT_H
#define PLAT_H
typedef unsigned long  ulong;
typedef unsigned short ushort;
typedef unsigned char  uchar;

struct plat {
	const char *name;
	void (*putc)(char);
	void (*flush)(void);
	void (*map_platform)(void);	/* identity-map chip RAM, I/O, ROM via page tables */
	void (*halt)(void) __attribute__((noreturn));
	ulong tt[2];			/* transparent translation registers, 0 = unused */
	ulong fault_test_addr;		/* an address nothing maps */
};
extern const struct plat *plat;

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
