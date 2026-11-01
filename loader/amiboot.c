/*
 * amiboot.c - AmigaOS-side loader for Amillumos kernels.
 *
 * Usage (from an AmigaDOS shell or copperline --run):  amiboot [kernel.img]
 *
 * Reads a flat kernel image (struct amikern_hdr at offset 0), places it in fast RAM
 * at the alignment the header asks for, zeroes its bss, collects exec's memory list
 * into a struct amiboot_info, then enters supervisor mode and jumps to the kernel
 * with the MMU and caches off. Progress goes to the Paula serial port so that it is
 * visible on the emulator's host console.
 */
#include "../include/amiboot.h"

typedef unsigned long  ulong;
typedef unsigned short ushort;
typedef unsigned char  uchar;

/* start.S */
void *exec_OpenLibrary(const char *, ulong);
void  exec_CloseLibrary(void *);
void *exec_AllocMem(ulong, ulong);
void  exec_FreeMem(void *, ulong);
void  exec_Forbid(void);
void  exec_Permit(void);
void  exec_Disable(void);
void  exec_Supervisor(void (*)(void));
void *exec_base(void);
long  dos_Open(void *, const char *, long);
long  dos_Close(void *, long);
long  dos_Read(void *, long, void *, long);
long  dos_Seek(void *, long, long, long);
long  dos_IoErr(void *);
void  boot_trampoline(void);
extern struct amiboot_info *boot_info_ptr;
extern ulong boot_entry_off;

#define MEMF_PUBLIC   0x0001UL
#define MEMF_CHIP     0x0002UL
#define MEMF_FAST     0x0004UL
#define MEMF_CLEAR    0x10000UL
#define MODE_OLDFILE  1005
#define OFFSET_BEGINNING (-1)
#define OFFSET_END     1
#define AFF_68040     (1 << 3)

/* exec structures, only the fields we read (offsets from the NDK). */
/* exec structures are 2-byte packed; we build with -malign-int, so say so explicitly. */
struct Node { struct Node *ln_Succ, *ln_Pred; uchar ln_Type, ln_Pri; char *ln_Name; }
    __attribute__((packed));
struct List { struct Node *lh_Head, *lh_Tail, *lh_TailPred; uchar lh_Type, l_pad; }
    __attribute__((packed));
struct MemHeader {
	struct Node mh_Node; ushort mh_Attributes; void *mh_First;
	void *mh_Lower, *mh_Upper; ulong mh_Free;
} __attribute__((packed));
#define EXEC_ATTNFLAGS(eb)  (*(ushort *)((char *)(eb) + 0x128))
#define EXEC_MEMLIST(eb)    ((struct List *)((char *)(eb) + 0x142))

/* ---- Paula serial output, polled ---- */
#define CUSTOM   ((volatile ushort *)0xDFF000)
#define SERDAT   (0x030 / 2)
#define SERPER   (0x032 / 2)
#define SERDATR  (0x018 / 2)

static void ser_putc(char c)
{
	while (!(CUSTOM[SERDATR] & (1 << 13)))
		;
	CUSTOM[SERDAT] = 0x100 | (uchar)c;
}
static void ser_puts(const char *s)
{
	for (; *s; s++) {
		if (*s == '\n')
			ser_putc('\r');
		ser_putc(*s);
	}
}
static void ser_puthex(ulong v)
{
	static const char hex[] = "0123456789abcdef";
	int i;
	ser_puts("0x");
	for (i = 28; i >= 0; i -= 4)
		ser_putc(hex[(v >> i) & 15]);
}
static void ser_putdec(ulong v)
{
	char buf[12]; int i = 11;
	buf[i] = 0;
	do { buf[--i] = '0' + v % 10; v /= 10; } while (v);
	ser_puts(buf + i);
}
static void ser_flush(void)
{
	while (!(CUSTOM[SERDATR] & (1 << 12)))
		;
}

static ulong align_up(ulong v, ulong a) { return (v + a - 1) & ~(a - 1); }

static long read_fully(void *dosbase, long fh, void *buf, long len)
{
	char *p = buf;
	while (len > 0) {
		long n = dos_Read(dosbase, fh, p, len);
		if (n <= 0) {
			ser_puts("amiboot: Read("); ser_puthex((ulong)p); ser_puts(", ");
			ser_putdec(len); ser_puts(") returned "); ser_putdec((ulong)n);
			ser_puts(" IoErr "); ser_putdec(dos_IoErr(dosbase)); ser_puts("\n");
			return -1;
		}
		p += n; len -= n;
	}
	return 0;
}

long loader_main(const char *args, ulong arglen)
{
	char name[64];
	void *dosbase; long fh, fsize;
	struct amikern_hdr hdr;
	struct amiboot_info *bi;
	void *exec = exec_base();
	ulong base, raw, total, i, to_read;
	char *dst;
	struct MemHeader *mh;

	CUSTOM[SERPER] = 368;	/* 9600 baud, 8N1 */
	ser_puts("\namiboot: Amillumos loader, build " __DATE__ "\n");

	/* kernel file name: first word of the command line, default kernel.img */
	for (i = 0; i < arglen && i < sizeof(name) - 1 && args[i] > ' '; i++)
		name[i] = args[i];
	name[i] = 0;
	if (i == 0) {
		const char *d = "kernel.img";
		for (i = 0; d[i]; i++) name[i] = d[i];
		name[i] = 0;
	}

	if (!(EXEC_ATTNFLAGS(exec) & AFF_68040)) {
		ser_puts("amiboot: this is not a 68040 (AttnFlags=");
		ser_puthex(EXEC_ATTNFLAGS(exec)); ser_puts("), giving up\n");
		return 20;
	}

	dosbase = exec_OpenLibrary("dos.library", 36);
	if (!dosbase) { ser_puts("amiboot: no dos.library v36\n"); return 20; }

	fh = dos_Open(dosbase, name, MODE_OLDFILE);
	if (!fh) { ser_puts("amiboot: cannot open "); ser_puts(name); ser_puts("\n"); return 20; }
	dos_Seek(dosbase, fh, 0, OFFSET_END);
	fsize = dos_Seek(dosbase, fh, 0, OFFSET_BEGINNING);
	if (fsize < (long)sizeof(hdr) || read_fully(dosbase, fh, &hdr, sizeof(hdr)) < 0) {
		ser_puts("amiboot: short read of header\n"); return 20;
	}
	if (hdr.magic != AMIKERN_MAGIC || hdr.version != 1) {
		ser_puts("amiboot: bad kernel header magic "); ser_puthex(hdr.magic); ser_puts("\n");
		return 20;
	}
	ser_puts("amiboot: "); ser_puts(name); ser_puts(": image ");
	ser_putdec(hdr.image_size); ser_puts(" bytes, bss ");
	ser_putdec(hdr.bss_size); ser_puts(" bytes, entry +");
	ser_puthex(hdr.entry); ser_puts(", align "); ser_puthex(hdr.align); ser_puts("\n");

	/* place the image: any fast RAM, aligned as requested, plus a 64 KB stack above it */
	total = align_up(hdr.image_size + hdr.bss_size, 4096) + 65536;
	raw = (ulong)exec_AllocMem(total + hdr.align, MEMF_FAST | MEMF_PUBLIC | MEMF_CLEAR);
	if (!raw) { ser_puts("amiboot: AllocMem failed\n"); return 20; }
	base = align_up(raw, hdr.align);
	dst = (char *)base;
	ser_puts("amiboot: AllocMem gave "); ser_puthex(raw); ser_puts(", image base ");
	ser_puthex(base); ser_puts("\n");
	/* the file may stop short of image_size (alignment padding); zero whatever is missing */
	to_read = (ulong)fsize < hdr.image_size ? (ulong)fsize : hdr.image_size;
	dos_Seek(dosbase, fh, 0, OFFSET_BEGINNING);
	if (read_fully(dosbase, fh, dst, to_read) < 0) {
		ser_puts("amiboot: read of image failed\n"); return 20;
	}
	dos_Close(dosbase, fh);
	for (i = to_read; i < hdr.image_size + hdr.bss_size; i++)
		dst[i] = 0;

	/* boot information block */
	bi = exec_AllocMem(sizeof(*bi), MEMF_FAST | MEMF_PUBLIC | MEMF_CLEAR);
	if (!bi) { ser_puts("amiboot: AllocMem(info) failed\n"); return 20; }
	bi->magic = AMIBOOT_MAGIC;
	bi->version = AMIBOOT_VERSION;
	bi->load_addr = base;
	bi->load_size = hdr.image_size + hdr.bss_size;
	bi->stack_top = base + total - 16;
	bi->attn_flags = EXEC_ATTNFLAGS(exec);
	for (i = 0; i < AMIBOOT_CMDLINE - 1 && i < arglen && args[i] >= ' '; i++)
		bi->cmdline[i] = args[i];
	bi->cmdline[i] = 0;

	exec_Forbid();
	for (mh = (struct MemHeader *)EXEC_MEMLIST(exec)->lh_Head;
	    mh->mh_Node.ln_Succ && bi->nmem < AMIBOOT_MAX_MEM;
	    mh = (struct MemHeader *)mh->mh_Node.ln_Succ) {
		bi->mem[bi->nmem].lower = (ulong)mh->mh_Lower;
		bi->mem[bi->nmem].upper = (ulong)mh->mh_Upper;
		bi->mem[bi->nmem].attrs = mh->mh_Attributes;
		bi->nmem++;
	}
	exec_Permit();

	ser_puts("amiboot: memory map from exec:\n");
	for (i = 0; i < bi->nmem; i++) {
		ser_puts("  "); ser_puthex(bi->mem[i].lower); ser_puts(" - ");
		ser_puthex(bi->mem[i].upper); ser_puts("  ");
		ser_puts((bi->mem[i].attrs & MEMF_CHIP) ? "chip" : "fast"); ser_puts("\n");
	}
	ser_puts("amiboot: kernel at "); ser_puthex(base); ser_puts(", stack top ");
	ser_puthex(bi->stack_top); ser_puts(", info at "); ser_puthex((ulong)bi);
	ser_puts("\namiboot: gdb: add-symbol-file kernel.elf -o "); ser_puthex(base);
	ser_puts("\namiboot: leaving AmigaOS, jumping to kernel\n");
	ser_flush();

	boot_info_ptr = bi;
	boot_entry_off = hdr.entry;
	exec_Disable();
	exec_Supervisor(boot_trampoline);
	/* not reached */
	ser_puts("amiboot: returned from kernel?!\n");
	return 20;
}
