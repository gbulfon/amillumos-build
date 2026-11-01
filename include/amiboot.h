/*
 * amiboot.h - boot protocol between the AmigaOS-side loader (amiboot) and the kernel.
 * Everything is big-endian 32-bit. The loader builds an amiboot_info in fast RAM and
 * jumps to the kernel entry in supervisor mode, interrupts masked, MMU and caches off,
 * with %a0 = pointer to the amiboot_info and %sp = stack_top.
 */
#ifndef AMIBOOT_H
#define AMIBOOT_H

#define AMIBOOT_MAGIC      0x414D494CUL   /* "AMIL" */
#define AMIBOOT_VERSION    1
#define AMIBOOT_MAX_MEM    16
#define AMIBOOT_CMDLINE    256

/* Flat kernel image header, at offset 0 of the image file. */
#define AMIKERN_MAGIC      0x414D494CUL   /* "AMIL" */
struct amikern_hdr {
	unsigned long magic;        /* AMIKERN_MAGIC */
	unsigned long version;      /* 1 */
	unsigned long image_size;   /* bytes in the file (text+rodata+data) */
	unsigned long bss_size;     /* bytes to zero after the image */
	unsigned long entry;        /* entry point offset from image start */
	unsigned long align;        /* required load alignment (power of two) */
};

struct amiboot_mem {
	unsigned long lower;        /* first byte */
	unsigned long upper;        /* last byte + 1 */
	unsigned long attrs;        /* exec MEMF_* attributes of the MemHeader */
};

struct amiboot_info {
	unsigned long magic;        /* AMIBOOT_MAGIC */
	unsigned long version;      /* AMIBOOT_VERSION */
	unsigned long load_addr;    /* physical address of the kernel image */
	unsigned long load_size;    /* image_size + bss_size */
	unsigned long stack_top;    /* initial supervisor stack (grows down) */
	unsigned long attn_flags;   /* ExecBase->AttnFlags (CPU/FPU type) */
	unsigned long nmem;
	struct amiboot_mem mem[AMIBOOT_MAX_MEM];
	char cmdline[AMIBOOT_CMDLINE];
};

#endif /* AMIBOOT_H */
