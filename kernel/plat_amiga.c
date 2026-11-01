/* plat_amiga.c - Amiga 4000: Paula serial console, chip RAM and I/O map, TTRs for
 * the CPU-slot (0x08000000-0x0FFFFFFF) and Zorro III (0x40000000-0x7FFFFFFF) RAM. */
#include "plat.h"

#define CUSTOM   ((volatile ushort *)0xDFF000)
#define SERDAT   (0x030 / 2)
#define SERPER   (0x032 / 2)
#define SERDATR  (0x018 / 2)
#define INTENA   (0x09A / 2)
#define INTREQ   (0x09C / 2)

void plat_putc(ulong c)
{
	while (!(CUSTOM[SERDATR] & (1 << 13)))
		;
	CUSTOM[SERDAT] = 0x100 | (uchar)c;
}
void plat_flush(void)
{
	while (!(CUSTOM[SERDATR] & (1 << 12)))
		;
}
void plat_map_platform(void)
{
	map_range(0x00000000, 0x00000000, 0x00200000, CM_WRITETHROUGH, 0);	/* chip RAM */
	map_range(0x00200000, 0x00200000, 0x00D80000, CM_NOCACHE_SER, 0);	/* Z2, CIA, custom, IDE, RTC */
	map_range(0x00F80000, 0x00F80000, 0x00080000, CM_WRITETHROUGH, PD_WP);	/* Kickstart */
}
void plat_halt(void)
{
	plat_flush();
	cpu_halt();
}

const char plat_name[] = "amiga4000";
const ulong plat_tt[2] = { TTR(0x08, 0x07, CM_WRITETHROUGH), TTR(0x40, 0x3F, CM_WRITETHROUGH) };
const ulong plat_fault_test_addr = 0x20000000;

void plat_early_init(void)
{
	CUSTOM[INTENA] = 0x7FFF; CUSTOM[INTREQ] = 0x7FFF;	/* no chipset interrupts */
	CUSTOM[SERPER] = 368;					/* 9600 8N1 */
}
