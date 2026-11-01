# Amillumos top level. Targets: all, test, clean.
SUBDIRS = hello loader kernel

all:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d || exit 1; done

test: all
	RETURNS=1 tools/run-copperline.sh build/serial_hello.exe "Hello from Amillumos via Paula serial" 10
	tools/run-copperline.sh build/amiboot.exe "fault ok" 20
	tools/run-qemu.sh build/kernel-virt.elf "fault ok" 30

clean:
	@for d in $(SUBDIRS); do $(MAKE) -C $$d clean; done
.PHONY: all test clean
