# Amillumos: illumos on an (emulated) Amiga 4000 / 68040

## Context

Goal as stated by Gabriele (2026-10-31): the **full illumos experience** (ZFS, SMF, DTrace, zones)
on an Amiga 4000 with a 68040, **emulator-only** for now with RAM in the GB range, Copperline as
the main target (WinUAE/FS-UAE and QEMU m68k as alternates). Fork illumos-gate at the last
commit that still carries an ILP32 kernel. **Solo hobby project.** Decided in Q&A: ZFS root is
the first "full" feature after multiuser; build host is an existing illumos x86 machine;
packaging is IPS; Copperline emulation gaps get fixed upstream, not worked around.

Why the premise is hard, and why it is still feasible:

- illumos-gate has had no 32-bit kernel since flag day 2018-01-25 (issue 8685, commit
  `95cb2e3`). A 68040 is ILP32 with a 32-bit physical bus. 8685 removed only Makefiles,
  mdb Makefiles and package manifests (+97/−4526). The ILP32 kernel *source* paths survived
  until issue 13939 (`86ef0a6`, committed 2021-08-16, Richard Lowe: "it's time the 32bit intel
  kernel was (x)86'd"), which replaced `__i386` guards with `__x86` and dropped 32-bit-only code
  in ~5.5k lines across `uts/common` (arc.c heap-arena caps, param.c, busra, cmlb, drivers).
- So there are two candidate fork points, not one:
  - `141fdc8` (2018-01-25, parent of 8685): 32-bit build machinery intact and exercised, but
    primary compiler is the patched GCC 4.4.4 (GCC 7 became primary only 2019-06-17, GCC 10 on
    2023-04-12). A new GCC target for m68k on 4.4.4 is a dead end.
  - **`755ccbc` (2021-08-16, parent of 13939): decided.** GCC 7 primary, GCC 10 shadow era,
    ZFS/SMF/zones with 3.5 more years of fixes, all ILP32 code paths in `uts/common` still
    present. Cost: re-add the CLASS=32 build knobs that 8685 deleted (small, and 8685's diff is
    the exact recipe), and accept that those paths were unbuilt for 3.5 years (we only use the
    `uts/common` ones; the x86-specific 32-bit code is irrelevant to m68k).
  - A third option, tracking current trunk with 13939 reverted, is rejected: every later
    LP64-only cleanup would need re-fixing forever.
- At either fork point ZFS, SMF, DTrace and zones were built for the ILP32 i386 kernel, so
  "full experience" on a 32-bit kernel is a long roadmap, not an impossibility. Peter Tribble's
  2015 note that the 32-bit kernel was "effectively untested" applies: expect latent bugs.
- Emulator-only removes the 16-128 MB hardware ceiling. Copperline's A4000 profile allows
  motherboard 64M + accelerator 128M + Zorro III up to 1G (~1.2 GB). QEMU `virt` allows ~3.2 GB.

## Verified facts used by this plan

| Fact | Source |
|---|---|
| 32-bit x86 kernel removed: issue 8685, commit `95cb2e31c87e3febe7315833e075c1b16451e44c`, parent `141fdc8db8933e5c2d4fe6a0f5ee25c59f0a0231` | github illumos-gate, illumos.org/issues/8685 |
| ILP32 kernel source removed: issue 13939, commit `86ef0a63e1cfa5dc98606efef379365acca98063`, parent `755ccbcfa27de6aa07ebf138548b68dc6a9ae02d`, +984/−4551 | github illumos-gate |
| GCC 7 primary: 9996 `8123ce6` 2019-06-17. GCC 10 primary: 14421 `69cb22e` 2023-04-12 | github illumos-gate |
| Copperline: Rust, GPL-3, A4000 profile (AGA, 68040@25, Gayle IDE at $DD2020); 68040 MMU with 3-level walk, 4K/8K pages, format-7 access errors, TTRs, URP/SRP; JIT works with MMU; `--gdb` stub; `[serial] mode = stdout/pty/tcp`; `[a2065] net = nat/bridge`; headless mode; `z3` up to 1G, `accelerator` up to 128M, `motherboard` up to 64M on A4000 | copperline.dev/docs (cpu, configuration, gdb, headless) |
| Copperline gaps: 68040 copyback cache "deliberately not modelled" (write-through); PTEST does not fill U/M/cache-mode bits; mid-instruction fault continuation not modelled; FPU "not chip-bit-exact"; 040 transcendentals trap to FPSP as on silicon | copperline.dev/docs/cpu |
| WinUAE: full 68030/040/060 MMU since 2.6.0 (2013), runs NetBSD/Linux/Amix; MMU mode incompatible with JIT | WinUAE release notes/help |
| QEMU `virt` m68k: default CPU 68040, ≤3.2 GB, goldfish tty/pic/rtc, 128 virtio-mmio slots, QEMU ≥6.0 | qemu hw/m68k/virt.c |
| Emu68/PiStorm: no MMU, not a target | Emu68 docs |
| Precedent for a new ISA with cross-build: richlowe/arm64-gate (illumos x86 host, cross gcc, QEMU virt, gdb attached to QEMU, no mdb/kmdb/dtrace yet) | github richlowe/arm64-gate |

## Inventory: what a new ISA/platform touches (measured at the 2018 fork; same shape in 2021)

Templates: ISA dir `uts/intel` (+`ia32`) and platform dir `uts/i86pc` are the right models
(hardware page-table walker, ramdisk boot, no OBP). `uts/sparc`/`sun4u`/`sfmmu` are the wrong
model (software TLB miss handler, OBP client interface, 64-bit only).

| Area | x86 reference size | m68k/amiga work |
|---|---|---|
| `usr/src/Makefile.master`, `uts/Makefile`, `uts/Makefile.uts`, `Makefile.psm`, `tools/cw/cw.c` | per-MACH tables (`sparc_`/`i386_` rows), `MACH64`, `BUILD64`, cw `xtbl` only for x86/sparc | add `m68k_*` rows, `m68k_BUILD64=$(POUND_SIGN)` (BUILD64-off path unused since sparcv7: expect rot), `m68k_ARCHITECTURES = amiga m68k`, cw branch; re-add CLASS=32 knobs from 8685's diff |
| `uts/common/sys/isa_defs.h` | ~400 lines, `#error` for unknown ISA | `__mc68000__` block: `_BIG_ENDIAN _ILP32 _LONG_LONG_HTOL _BIT_FIELDS_HTOL _STACK_GROWS_DOWNWARD _CHAR_IS_SIGNED`, no `_OBP`, no `_MULTI_DATAMODEL` |
| ISA headers `uts/intel/sys` + `ia32/sys` | ~75 headers | `uts/m68k/sys` ~60 headers (regset, frame, stack, trap, pcb, ucontext, fp, machlock, asm_linkage, archsystm, vmparam, procfs_isa, elf_M68K.h …) |
| ISA asm `intel/ia32/ml` | 13 files, 374 KB | `uts/m68k/ml`: copy.s, m68k_subr.s, lock_prim.s, swtch.s, trap entry, modstubs.s, float.s (FPU frames) |
| ISA C `intel/ia32/os` | 12 files, 259 KB | `uts/m68k/os`: archdep, sundep, syscall, sendsig, fpu (+FPSP), ddi_m68k, copy_subr |
| krtld relocs `intel/ia32/krtld` | 6 files, ~27 KB | `uts/m68k/krtld/doreloc.c` etc. (`R_68K_*`); the same `doreloc.c` is compiled into `ld` and `ld.so.1` |
| Platform `uts/i86pc` | `ml` 208 KB, `os` 1.19 MB (much is ACPI/MP/cpuid), `vm` 371 KB (HAT), `dboot` 101 KB, `boot` 58 KB, `sys` 74 hdrs | `uts/amiga`: `ml` (locore, interrupt, syscall_asm, offsets.in), `os` (startup, mlsetup, machdep, trap, intr, ddi_impl, memlist, fakebop-style bootprops, timestamp/hardclk via CIA, single-CPU mp stubs), `vm` (hat_m68k, htable, hment, vm_machdep, kboot_mmu; ~300 KB expected), `io` (rootnex, cbe, Paula serial, Gayle IDE, Zorro nexus, A2065), `conf`, `unix/Makefile`; plus `uts/m68kvirt` for QEMU sharing `uts/m68k` |
| `usr/src/common/atomic/<isa>/atomic.s` | one file per ISA | `m68k/atomic.s` on CAS/CAS2 |
| libc `lib/libc/i386` | crt, gen (42), sys (19), threads, fp (35), unwind, inc/SYS.h, Makefile.com ~1200 lines | `lib/libc/m68k` same shape; `lib/crt/m68k` |
| sgs | `libld/common/machrel.{intel,amd,sparc}.c`, `rtld/{i386,sparc}` (boot.s, boot_elf.s, caller.s, *_elf.c, _setup.c), `libconv/common/relocate_*.c`, `include/<isa>/machdep_*.h`, `uts/common/sys/elf_*.h` | `machrel.m68k.c`, `rtld/m68k`, `relocate_m68k.c`, `machdep_m68k.h`, `elf_M68K.h` |
| CTF tools (`libctf`, `ctfconvert`, `ctfmerge`) | know x86/SPARC ELF | teach `EM_68K`; can run without CTF for first boot |
| Deferrable | DTrace (`uts/<isa>/dtrace`: dtrace_asm.s, dtrace_isa.c, fasttrap_isa.c 45 KB, fbt.c, sdt.c; `lib/libdtrace/<isa>`), kmdb/kaif, mdb `kvm_isadep`/`proc_isadep`, libdisasm (no m68k decoder exists anywhere in the gate), libproc/libc_db `Pisadep.c`, cpc/pcbe, cpr, lgrp, `usr/src/boot` loader | per milestones M5+ |

Boot path to reuse: krtld's `!_OBP` branch (`kobj_boot_mountroot()` ramdisk root,
`synthetic_bootaux()`), `fakebop`-style boot properties. The x86 `dboot`/FreeBSD loader is
not ported; a native loader replaces it.

## Design decisions

1. **Fork point: `755ccbc` (parent of 13939, Aug 2021)**, branch `amiga` in a repo
   `amillumos-gate`. First commits: re-apply the CLASS=32 build machinery removed by 8685, and
   use 13939's diff as the checklist of ILP32 sites in `uts/common` (convert their `__i386`
   guards to `_ILP32`). (Confirmed by Gabriele on 2026-10-31; the 2018 fork `141fdc8` remains the fallback.)
   Checked 2026-11-01: `uts/Makefile.uts`/`Makefile.targ` at `755ccbc` still define the
   `obj32`/`debug32` classes, so only `uts/m68k/Makefile.m68k` has to select `DEF_BUILDS32`.
2. **ISA `m68k`, platforms `amiga` and `m68kvirt`**, laid out like `intel`/`i86pc`.
3. **ABI: SVR4 m68k ELF psABI** (`EM_68K`, `R_68K_*`), ILP32 only, no `$(MACH64)` subtrees
   ever. Compile with `-m68040 -malign-int` so `int`/`long`/pointer alignment is 4 as every
   illumos ISA assumes. TLS via the GNU m68k `__m68k_read_tp` model.
4. **Toolchain: upstream GCC 10 (matching the gate's GCC 10 shadow era) + GNU `as`** for
   m68k, new GCC target `m68k-illumos` modelled on arm64-gate's aarch64-illumos patches.
   illumos' own `ld`, `ld.so.1`, `krtld` get the m68k backend; no GNU ld in the product.
5. **Cross-build from the illumos x86 host** via an `amillumos-build` repo cloned from
   arm64-gate's structure: `env/m68k`, `make setup` (cross gcc/binutils, sysroot),
   `make illumos`, `make image`. Set `CW_NO_SHADOW`.
6. **Boot: loader run from AmigaOS/AROS**, like NetBSD's `loadbsd`: loads `unix` ELF +
   `boot_archive` into fast RAM, builds a memlist from exec's MemList, builds boot properties,
   disables the OS, enters supervisor with MMU off, jumps to `_start`. Same binary works on
   Copperline, WinUAE and real hardware. QEMU `virt` uses `-kernel` with a flat image and a
   tiny entry stub reading QEMU's bootinfo list.
7. **Primary emulator Copperline, QEMU `virt` second** (independent MMU implementation, more
   RAM, virtio for fast storage), WinUAE only as a pre-hardware check.
8. **Console = Paula serial** (`SERDAT`/`SERPER`) on Amiga, goldfish-tty on QEMU; early
   `prom_printf` equivalent is polled serial. Graphics deferred.
9. **Root: ramdisk → UFS on Gayle IDE → ZFS.**
10. **Debugging: emulator GDB stub + cross `m68k` gdb** with kernel symbols; kmdb/mdb later.
11. **FPU: 68040 FPSP** (Motorola, as in NetBSD `sys/arch/m68k/fpsp`) in its own kernel module
    for unimplemented FP instructions; separate license file.
12. **HAT `hat_m68k`** after `hat_i86.c`/`htable.c`: 3-level 68040 tables (7/7/6 bits at 4 KB),
    URP per process, SRP for kernel, full ATC flush on switch (no ASIDs; 64 entries), kernelbase
    split tuned for ~1 GB physical in ILP32 KVA; `arc.c` heap-arena caps from 13939 reinstated.
13. **Interrupts/clock**: Amiga levels 1-6 → PILs via SR IPL; CIA-B timer for `hz`;
    `gethrtime` from a CIA free-running timer (E-clock 709 kHz) initially.
14. **Packaging = IPS in two steps**: (a) x86 host publishes the cross-built gate to an IPS repo
    and builds images with `pkg image-create` + `pkg -R <root> install` (payloads are
    architecture-agnostic; this is how distro-constructor works); (b) Python + `pkg(1)` ported
    to m68k after ZFS so the Amiga can self-update with boot environments.

## Milestones

- **M0 Infrastructure**: repos; cross GCC/binutils `m68k-illumos`; Copperline A4000 config with
  1 GB and `serial=stdout`; QEMU virt config; CI script that boots and greps serial output.
- **M1 Kernel to shell**: loader; CLASS=32 machinery; `unix`+`genunix` linking with the m68k
  `ld` backend; krtld relocs; locore/trap/HAT; CIA timer; Paula serial + `consconfig`; ramdisk
  root with `init`, `sh`, `ld.so.1`, `libc`. Done when `sh` prompts on serial under Copperline
  and QEMU.
- **M2 Disk and network**: Gayle IDE (PIO) + UFS root installed from the ramdisk; A2065
  (Am7990) GLDv3 driver; TCP/IP; NFS; ssh in. virtio-blk/net on QEMU as a shortcut.
- **M3 Multiuser with SMF**: full 32-bit userland, doors, contracts, SMF, console-login,
  IPS publishing + host-side image build.
- **M4 ZFS root** (first "full" feature): ARC tuning for ILP32/1 GB; `beadm`; then Python +
  `pkg(1)` on m68k.
- **M5 DTrace**: `dtrace_isa.c`, profile/sdt, fbt (trap at entry), then fasttrap/pid (m68k
  instruction decoder + emulation; also unlocks libdisasm m68k and mdb).
- **M6 Zones** (native brand).
- **M7 Graphics console** (optional): Picasso II/CL-GD5426 or AGA framebuffer.
- **M8 Real hardware** (optional): 68040/060 accelerator, 128 MB. Expect copyback/DMA
  coherency bugs invisible under Copperline; validate under WinUAE first.

**Parallel track, Copperline fidelity**: file issues/PRs on CopperlineHQ/Copperline for gaps the
kernel exposes (copyback cache, PTEST U/M bits, fault continuation, Gayle/A2065 details). Keep
CDDL (Amillumos) and GPL-3 (Copperline) work in separate repos.

## Risks and honest costs

- Solo pace: M1 is realistically 1-2 years of evenings even with heavy AI assistance; M2-M3
  another year; ZFS, DTrace, zones after that. The arm64 port by a senior illumos engineer took
  years to reach a flaky multiuser boot.
- The ILP32 kernel paths in `uts/common` were unbuilt from 2018 to 2021 and untested long before
  that. Big-endian + ILP32 together have never been built in illumos (SPARC is LP64), so
  expect `#ifdef` rot in both directions.
- `ld.so.1` + `libc` m68k are on the critical path to a shell (no static libc in illumos).
- ZFS ARC in ILP32 KVA: the i386 heap arena was the practical limit; pools work but ARC is small.
- 32-bit `time_t` everywhere (2038).
- Copperline MMU/cache gaps may hide bugs; QEMU cross-checks CPU behaviour but not Amiga devices.
- GCC m68k backend quality for a large kernel codebase is unproven in this century outside
  Linux/NetBSD; expect to report a few compiler bugs.

## Verification

- Every milestone: Copperline headless run and QEMU run that boot the image, capture serial
  output and assert a marker (`SunOS Release`, then a `#` prompt); non-zero exit on timeout.
- M1: interactive `sh`; `uname -a` shows `m68k`/`amiga`; `ls /`, `ps`, `cat /etc/release`.
- M2: `newfs`/`mount` on IDE, `ifconfig` + `ping` via A2065 NAT, NFS mount, ssh login.
- M3: `svcs -xv` clean, boot to `console-login`, image built on host with `pkg -R` boots.
- M4: `zpool create` on IDE, ZFS root boot, `zpool scrub` clean, `beadm create` + reboot.
- M5: `dtrace -n 'syscall:::entry'`, then `fbt:::`, then `pid$target:::`.
- M6: `zonecfg`/`zoneadm install`/`zlogin`.

## Progress log

- **2026-11-01, step 1 done** (see README.md): `amiboot` loader (AmigaDOS, built with m68k-elf GCC
  via `tools/mkhunk.py`), hello kernel with 68040 page tables + TTRs, MMU on, 0xC0000000 alias,
  access fault caught and resumed. Passes headless on Copperline 0.16 (Kickstart 3.1 A4000,
  64M+128M+1G RAM, serial to stdout) and on QEMU `virt` (goldfish-tty, bootinfo records,
  virt-ctrl power-off). `make test` runs all three checks. GDB verified on Copperline 1.1.0 with `--no-jit`: breakpoints in loader and kernel,
  `stepi` with the MMU on, access-error frame readable. With the JIT on the stub's breakpoints never
  fire and gdb reports "Ignoring packet error" (upstream issue to file); the stub reads physical
  addresses only, no MMU translation (upstream feature request).
- Found: Homebrew m68k-elf GCC 16.2.0 ICEs on external calls with arguments under
  `-malign-int` (reproducer in `docs/gcc-ice-mpcrel/`). Copperline 1.1.0 (2026-10-31) adds
  `--exit-on-return`; dmg downloaded and verified but not installed (install by hand).
- **2026-11-01, host procedure written** (`docs/host-setup.md`, `host/`): env file for
  bldenv/nightly (native and `-T m68k`), Sun-make orchestration (download, binutils, stage0
  GCC 10.4 `m68k-unknown-elf`, `check-align` for the ICE, native tools smoke test, sgs/sysroot,
  boot-gcc), preflight script. Sources pinned: illumos/gcc `il-10_4_0`, richlowe/binutils-gdb
  `illumos-arm64-2-43-1`, arm64-gate commits `c14727bb0` `38aa9e73c` (`-T`) and `cb6653583`
  (`NATIVE_MACH`/`CROSS_BLD`), richlowe/gcc `e09303585` `af5b6a6c6` `8177a00c0` `11887da00`
  (aarch64 sol2 target config, the template for m68k). Not yet run on the host.

## Next steps (M0 continued)

1. ~~Install Copperline 1.1.0~~ done 2026-11-01; run script uses `--exit-on-return` for returning programs.
   File the two Copperline issues (gdb stub + JIT; MMU translation in the stub).
2. Decide the `-malign-int` question for the real toolchain (own cross GCC 10 build for
   `m68k-illumos`, where the ICE can be checked and, if present, reported/patched).
3. Create `amillumos-gate` (illumos-gate at `755ccbc`, branch `amiga`) and `amillumos-build`
   (arm64-gate layout) on the illumos x86 build host: exact procedure in `docs/host-setup.md`,
   files in `host/`. (Correction 2026-11-01: the CLASS=32 knobs of `uts/Makefile.uts` and
   `uts/Makefile.targ` are present at `755ccbc`; nothing from 8685 needs re-adding.)
4. Grow the loader into an ELF loader (PT_LOAD + symbol tables kept for krtld) and define the
   boot-property block illumos' `fakebop` path will consume.

## Original first steps (M0, kept for reference)

1. Create `amillumos-gate` from illumos-gate at `755ccbc`, branch `amiga`; create
   `amillumos-build` from arm64-gate's layout; write `README` with this plan.
2. Build cross binutils + GCC 10 for `m68k-illumos` (start from `m68k-elf`, add the
   illumos target config), confirm `-m68040 -malign-int` compiles `uts/common/os/main.c`.
3. Copperline: A4000 profile, `[cpu] model="68040" jit=true`, `[memory] motherboard="64M"
   accelerator="128M" z3="1G"`, `[serial] mode="stdout"`, `[a2065] net="nat"`; verify AROS
   boots and serial output reaches the host. Same for QEMU `-M virt -cpu m68040 -m 2G`.
4. Hello-kernel: a 200-line ELF that the AmigaOS loader loads, switches to supervisor,
   writes to `SERDAT`, enables the MMU with an identity map, and halts. This proves loader,
   serial, MMU and GDB attach before any illumos code is involved.
