/* plat_virt.c - QEMU m68k "virt" machine: goldfish-tty console, virt-ctrl power-off,
 * Linux bootinfo records converted into an amiboot_info. RAM (from 0) is covered by
 * one TTR; the goldfish/virtio MMIO window at 0xff000000 goes through page tables. */
#include "plat.h"
#include "../include/amiboot.h"

#define BI_LAST            0x0000
#define BI_MEMCHUNK        0x0005
#define BI_COMMAND_LINE    0x0007
#define BI_VIRT_GF_TTY_BASE 0x8003
#define BI_VIRT_CTRL_BASE  0x8005

static volatile ulong *gf_tty = (volatile ulong *)0xff008000;
static volatile ulong *virt_ctrl = (volatile ulong *)0xff009000;
static struct amiboot_info bi;
extern char _start[], _end[];
void kmain(struct amiboot_info *);

void plat_putc(ulong c) { gf_tty[0] = (uchar)c; }
void plat_flush(void) { }
void plat_map_platform(void)
{
	map_range(0xff000000, 0xff000000, 0x00020000, CM_NOCACHE_SER, 0);	/* goldfish + virtio */
}
void plat_halt(void)
{
	virt_ctrl[1] = 2;	/* VIRT_CMD_HALT: QEMU exits */
	cpu_halt();
}

const char plat_name[] = "qemu-virt";
const ulong plat_tt[2] = { TTR(0x00, 0x3F, CM_WRITETHROUGH), 0 };	/* RAM 0x00000000-0x3FFFFFFF */
const ulong plat_fault_test_addr = 0xA0000000;

void plat_early_init(void) { }

/* called from start_virt.S with the address just past the image */
void virt_entry(char *image_end)
{
	ushort *rec = (ushort *)(((ulong)image_end + 1) & ~1UL);
	ulong sp;
	int i;

	asm("move.l %%sp,%0" : "=d"(sp));
	bi.magic = AMIBOOT_MAGIC; bi.version = AMIBOOT_VERSION;
	bi.load_addr = (ulong)_start; bi.load_size = (ulong)(_end - _start);
	bi.stack_top = sp;
	for (;;) {
		ushort tag = rec[0], size = rec[1];
		ulong *data = (ulong *)(rec + 2);
		if (tag == BI_LAST || size < 4)
			break;
		switch (tag) {
		case BI_MEMCHUNK:
			if (bi.nmem < AMIBOOT_MAX_MEM) {
				bi.mem[bi.nmem].lower = data[0];
				bi.mem[bi.nmem].upper = data[0] + data[1];
				bi.mem[bi.nmem].attrs = 4;	/* MEMF_FAST */
				bi.nmem++;
			}
			break;
		case BI_COMMAND_LINE:
			for (i = 0; i < AMIBOOT_CMDLINE - 1 && ((char *)data)[i]; i++)
				bi.cmdline[i] = ((char *)data)[i];
			bi.cmdline[i] = 0;
			break;
		case BI_VIRT_GF_TTY_BASE:
			gf_tty = (volatile ulong *)data[0];
			break;
		case BI_VIRT_CTRL_BASE:
			virt_ctrl = (volatile ulong *)data[0];
			break;
		}
		rec = (ushort *)((char *)rec + size);
	}
	kmain(&bi);
	plat_halt();
}
