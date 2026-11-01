# Amillumos

illumos on an Amiga 4000 with a 68040, emulator-first. See `PLAN.md` for the full plan and
`docs/illumos-fork-inventory.md` for the illumos-gate research behind it.

## Status (2026-11-01): step 1, the bring-up loop, works

| Piece | What it does | Verified on |
|---|---|---|
| `loader/` `amiboot` | AmigaDOS program: loads a flat kernel image into fast RAM, collects exec's memory list into a `struct amiboot_info`, enters supervisor mode, turns the MMU and caches off and jumps to the kernel | Copperline 1.1.0, Kickstart 3.1 A4000 |
| `kernel/` hello kernel | Paula serial console, 68040 page tables (4 KB, 3-level) plus two transparent-translation registers, enables the MMU, runs itself through a 0xC0000000 alias, takes and resumes an access fault | Copperline A4000, QEMU `virt` |
| `kernel/plat_*.c` | platform layer: Amiga (Paula, chip RAM/IO map) and QEMU virt (goldfish-tty, bootinfo records, virt-ctrl power-off) | both |
| `tools/mkhunk.py` | wraps a position-independent flat binary into an AmigaDOS hunk executable | |
| `tools/run-copperline.sh`, `tools/run-qemu.sh` | headless runs that grep the serial console for a marker; the seed of CI | |

## Host side (illumos x86)

`host/` holds the build-host layout (env file, Sun-make orchestration, preflight script) and
`docs/host-setup.md` the step-by-step procedure to fork illumos-gate at `755ccbc`, borrow
arm64-gate's cross-build plumbing and build the m68k toolchain.

## Build and test

Requirements on the Mac: Homebrew `m68k-elf-gcc` and `m68k-elf-binutils`, `qemu`, `gdb`, Copperline 1.1.0 or newer,
a Kickstart 3.1 A4000 ROM (path in `emu/copperline/a4000.toml`).

    make          # build/serial_hello.exe, build/amiboot.exe, build/kernel.img, build/kernel-virt.elf
    make test     # Copperline: serial test + loader + kernel; QEMU virt: kernel

Run the Amiga side by hand:

    copperline --config emu/copperline/a4000.toml --run build/amiboot.exe --screenshot-after 20 /tmp/end.png

QEMU by hand:

    qemu-system-m68k -M virt -cpu m68040 -m 1G -nographic -no-reboot -kernel build/kernel-virt.elf

Debugging: run Copperline with `--no-jit --gdb 2345 ...`, then
`gdb -ex 'set architecture m68k' -ex 'target remote :2345'`. The machine starts paused; set a
breakpoint in the loader first (for example `boot_trampoline`, hunk base is printed by the stub),
and only then set kernel breakpoints, because the loader copies the image over any breakpoint
set earlier. Under `--gdb` the kernel lands at 0x08024000, otherwise at 0x0801c000. Verified with
Copperline 1.1.0 and gdb 18.1: breakpoints and `stepi` work with the MMU on, and the format-7
access-error frame is readable on the stack. Two stub limitations found:

- with the JIT enabled, breakpoints never fire and gdb reports "Ignoring packet error" on the
  stop replies; use `--no-jit` for debugging sessions (to report upstream);
- memory reads use physical addresses, not the guest's MMU translation (reading 0xC0000000
  returns bus-error fill, not the kernel alias); a feature request upstream.

## Boot protocol

`include/amiboot.h`: flat image with a 24-byte `amikern_hdr` (magic "AMIL", sizes, entry
offset, alignment); the kernel is entered with `%a0` pointing to an `amiboot_info`
(load address, stack top, exec memory list, AttnFlags, command line). All kernel code is
position independent (`-mpcrel`, no relocations), so the loader may place it anywhere.

## Known toolchain issue

Homebrew m68k-elf GCC 16.2.0 crashes (`internal compiler error: in emit_move_insn`) on any call
to an external function with arguments when `-malign-int` is given. Reproducer and notes in
`docs/gcc-ice-mpcrel/`. The hello kernel currently builds because its external calls are few;
the real port will use its own cross GCC.
