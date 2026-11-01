CROSS   ?= m68k-elf-
CC      = $(CROSS)gcc
LD      = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump
OUT     ?= ../build
CFLAGS  = -m68040 -malign-int -mpcrel -fno-pic -ffreestanding -fno-builtin -nostdlib \
          -fno-jump-tables -fno-asynchronous-unwind-tables -fno-stack-protector \
          -fomit-frame-pointer -O2 -Wall -Wextra -Wno-unused-parameter

OBJS = $(OUT)/loader/start.o $(OUT)/loader/amiboot.o

all: $(OUT)/amiboot.exe

$(OUT)/loader/%.o: %.S
	@mkdir -p $(OUT)/loader
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/loader/%.o: %.c ../include/amiboot.h
	@mkdir -p $(OUT)/loader
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/amiboot.elf: $(OBJS) loader.ld
	$(LD) -T loader.ld -q -o $@ $(OBJS)
	@# any remaining absolute relocation would break the relocation-free hunk
	@if $(OBJDUMP) -r -j .text -j .rodata -j .data $@ | grep -E 'R_68K_(32|16|8)\b' | grep -v '\.debug' | grep -q .; then \
	    echo "ERROR: absolute relocations in loader:"; $(OBJDUMP) -r -j .text -j .rodata -j .data $@ | grep -E 'R_68K_(32|16|8)\b'; exit 1; fi

$(OUT)/amiboot.bin: $(OUT)/amiboot.elf
	$(OBJCOPY) -O binary $< $@

$(OUT)/amiboot.exe: $(OUT)/amiboot.bin $(OUT)/amiboot.elf
	b=$$($(CROSS)nm $(OUT)/amiboot.elf | awk '/ __bss_start$$/{print $$1}'); \
	e=$$($(CROSS)nm $(OUT)/amiboot.elf | awk '/ _end$$/{print $$1}'); \
	python3 ../tools/mkhunk.py --bss $$((0x$$e - 0x$$b)) $< $@

clean:
	rm -rf $(OUT)/loader $(OUT)/amiboot.*
.PHONY: all clean
