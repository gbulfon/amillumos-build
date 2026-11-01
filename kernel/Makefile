CROSS   ?= m68k-elf-
CC      = $(CROSS)gcc
LD      = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump
OUT     ?= ../build
CFLAGS  = -m68040 -malign-int -mpcrel -fno-pic -ffreestanding -fno-builtin -nostdlib \
          -fno-jump-tables -fno-asynchronous-unwind-tables -fno-stack-protector \
          -fomit-frame-pointer -O2 -g -Wall -Wextra -Wno-unused-parameter

AMIGA_OBJS = $(OUT)/kernel/start.o $(OUT)/kernel/common.o $(OUT)/kernel/main.o $(OUT)/kernel/plat_amiga.o
VIRT_OBJS  = $(OUT)/kernel/start_virt.o $(OUT)/kernel/common.o $(OUT)/kernel/main.o $(OUT)/kernel/plat_virt.o

all: $(OUT)/kernel.img $(OUT)/kernel-virt.elf

$(OUT)/kernel/%.o: %.S
	@mkdir -p $(OUT)/kernel
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/kernel/%.o: %.c ../include/amiboot.h plat.h
	@mkdir -p $(OUT)/kernel
	$(CC) $(CFLAGS) -c $< -o $@

define check_relocs
	@if $(OBJDUMP) -r -j .text -j .rodata -j .data $(1) | grep -E 'R_68K_(32|16|8)\b' | grep -vE '__image_size|__bss_size' | grep -q .; then \
	    echo "ERROR: absolute relocations in $(1):"; $(OBJDUMP) -r -j .text -j .rodata -j .data $(1) | grep -E 'R_68K_(32|16|8)\b'; exit 1; fi
endef

$(OUT)/kernel.elf: $(AMIGA_OBJS) kernel.ld
	$(LD) -T kernel.ld -q --defsym BASE=0 -o $@ $(AMIGA_OBJS)
	$(call check_relocs,$@)

$(OUT)/kernel.img: $(OUT)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@$(CROSS)size $<

# QEMU loads the ELF where it says; keep it clear of the vector area at 0.
$(OUT)/kernel-virt.elf: $(VIRT_OBJS) kernel.ld
	$(LD) -T kernel.ld -q --defsym BASE=0x10000 -o $@ $(VIRT_OBJS)
	$(call check_relocs,$@)
	@$(CROSS)size $@

clean:
	rm -rf $(OUT)/kernel $(OUT)/kernel.* $(OUT)/kernel-virt.elf
.PHONY: all clean
