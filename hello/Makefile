# Build the serial test executable for the Amiga side.
CROSS   ?= m68k-elf-
AS      = $(CROSS)gcc
LD      = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy
OUT     ?= ../build

all: $(OUT)/serial_hello.exe

$(OUT)/serial_hello.o: serial_hello.S
	@mkdir -p $(OUT)
	$(AS) -m68040 -x assembler-with-cpp -c $< -o $@

$(OUT)/serial_hello.elf: $(OUT)/serial_hello.o
	$(LD) -Ttext=0 -e _start -o $@ $<

$(OUT)/serial_hello.bin: $(OUT)/serial_hello.elf
	$(OBJCOPY) -O binary $< $@

$(OUT)/serial_hello.exe: $(OUT)/serial_hello.bin
	python3 ../tools/mkhunk.py $< $@

clean:
	rm -f $(OUT)/serial_hello.*

.PHONY: all clean

# QEMU virt target
all: $(OUT)/virt_hello.elf

$(OUT)/virt_hello.o: virt_hello.S
	@mkdir -p $(OUT)
	$(AS) -m68040 -x assembler-with-cpp -c $< -o $@

$(OUT)/virt_hello.elf: $(OUT)/virt_hello.o
	$(LD) -Ttext=0x1000 -e _start -o $@ $<
