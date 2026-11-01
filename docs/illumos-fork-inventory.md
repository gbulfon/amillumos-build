# illumos-gate fork inventory for an m68k/Amiga port

Research date: 2026-10-31. Web-only, read-only (GitHub, api.github.com, illumos.org, flag-day
page). Measured at the January 2018 snapshot `141fdc8` (parent of 8685, the 32-bit x86 kernel
removal). **Decision taken afterwards:** the fork point is `755ccbcfa27de6aa07ebf138548b68dc6a9ae02d`
(2021-08-16, parent of 13939 "it's time the 32bit intel kernel was (x)86'd"), because 8685
removed only Makefiles while the ILP32 kernel source survived until 13939, and GCC 7 became the
primary compiler in between (9996, 2019-06-17). The directory shapes and sizes below are the
same at both points; the GCC 4.4.4 remarks in section 4 apply only to the 2018 snapshot.

All paths relative to `usr/src/` in illumos-gate unless noted. Byte sizes from the GitHub API;
~30-35 bytes/line for C.

## 1. Fork points

| Item | Value |
|---|---|
| 32-bit kernel build removal | `95cb2e31c87e3febe7315833e075c1b16451e44c` - "8685 uts and mdb: do not build 32bit kernel" (Toomas Soome; committer Dan McDonald). 330 files, +97/-4526. Issue 8685 opened 2017-09-26, closed 2018-01-25. |
| Flag day | illumos.org/docs/developers/flagdays entry 2018-01-25 "HEADS UP: Removal of 32-bit x86 kernel" (Topicbox `Tbe0c8f87eef5a3f2-M62fb2690bf9ab15b373ac252`; body NOT VERIFIED). |
| Parent of 8685 | `141fdc8db8933e5c2d4fe6a0f5ee25c59f0a0231` - "8987 bootadm: add bootfile fallback to unix". |
| ILP32 kernel source removal | `86ef0a63e1cfa5dc98606efef379365acca98063` - "13939 it's time the 32bit intel kernel was (x)86'd" (Richard Lowe, committed 2021-08-16), +984/-4551, mostly `uts/common` (`__i386` guards -> `__x86`, arc.c heap-arena caps, param.c, busra, cmlb, drivers) plus removal of 32-bit mdb Makefiles. |
| **Chosen fork point** | **`755ccbcfa27de6aa07ebf138548b68dc6a9ae02d`** (parent of 13939). |
| Compilers | GCC 7 primary: 9996 `8123ce6` 2019-06-17. GCC 10 primary: 14421 `69cb22e` 2023-04-12. |

What 8685 touched (all 330 files inspected): no `uts/**` C/asm source. Only `uts/Makefile.uts`,
`uts/intel/Makefile.intel`, `uts/i86pc/Makefile.i86pc`, `uts/i86xpv/Makefile.i86xpv`
(DEF_BUILDS/ALL_BUILDS lose `$(..._BUILDS32)`), `Targetdirs`, `exception_lists/{closed-bins,packaging}`;
deletion of ~75 `cmd/mdb/intel/ia32/<mod>/Makefile`, `cmd/mdb/intel/ia32/{kmdb,libstand,libstandctf}`
ia32 files, `cmd/mdb/{i86pc,i86xpv,intel}/modules/*/ia32/Makefile`,
`cmd/{modload,pbind,prtconf,psrset,swap,sysdef,whodo}/i386/Makefile`; ~200 `pkg/manifests/*.mf`
edits. Its reverse is the recipe for re-adding the CLASS=32 machinery at `755ccbc`.

## 2. Inventory

### 2a. `uts/` top level
Dirs `common i86pc i86xpv intel sfmmu sparc sun sun4 sun4u sun4v`; files `Makefile Makefile.targ Makefile.uts README req.flg`.
- `uts/Makefile` (~210 lines): `sparc_ARCHITECTURES = sun4v sun4u sparc`, `i386_ARCHITECTURES = i86pc i86xpv intel`, `i386_PREREQ = intel`, `sparc_PREREQ = sun4`. New ISA adds `m68k_ARCHITECTURES = amiga m68kvirt m68k`.
- `uts/Makefile.uts` (~550 lines): `CLASS = $(CLASS_$(BUILD_TYPE))` (`CLASS_OBJ32=32`, `CLASS_DBG32=32`, `CLASS_OBJ64=64`), `MODEL_32=ilp32`, `CC = $(CC_$(MACH)_$(CLASS))`, `SUBDIR64_sparc=sparcv9`, `SUBDIR64_i386=amd64`, `CTFMERGE_GUDIR_sparc=sun4u`, `CTFMERGE_GUDIR_i386=intel`, `LINTFLAGS_<mach>_<class>`. Every per-MACH table needs an `m68k` row.
- `Makefile.psm` (~240 lines): `ROOT_PSM_DIR=$(ROOT)/platform/$(PLATFORM)`, `USR_PSM_DIR`, `ROOT_PSM_KERN_DIR=$(ROOT_PSM_KERN_DIR_$(CLASS))` - ISA-neutral.
- `uts/README`: three-level layout (ISA dir / platform dir / leaf module dir); `Makefile.files` = object lists + include paths, `Makefile.rules` = compile rules, `Makefile.<platform>` = defaults, `Makefile.uts`/`Makefile.targ` shared.

### 2b. ISA level: `uts/intel` vs `uts/sparc` (template for `uts/m68k`)
| | `uts/intel` | `uts/sparc` |
|---|---|---|
| Top | `Makefile`, `Makefile.files` 7.4K, `Makefile.intel` 16K, `Makefile.rules` 14K, `Makefile.targ` + ~200 module leaf dirs | `Makefile`, `Makefile.files` 2.8K, `Makefile.rules` 4.1K, `Makefile.sparc` 13K, `Makefile.targ` + ~150 module dirs |
| Sub-ISA | `ia32/{krtld,ml,os,promif,sys,syscall}`, `amd64/{krtld,ml,sys}` | `v9/{fpu,ml,os,sys,syscall,vm}` |
| Assembly | `ia32/ml` 13 files 374 KB: copy.s 68K, i86_subr.s 82K, exception.s 34K, lock_prim.s 37K, modstubs.s 47K, swtch.s 25K, float.s 20K, ddi_i86_asm.s 30K, sseblk.s, ovbcopy.s, desctbls_asm.s, hypersubr.s, ia32.il | `v9/ml` 6 files: crt.s, ddi_v9_asm.s, float.s, lock_prim.s, sparcv9_subr.s, syscall_trap.s |
| C machdep | `ia32/os` 12 files 259 KB: archdep.c 40K, ddi_i86.c 45K, desctbls.c 35K, syscall.c 36K, sundep.c 26K, sendsig.c 21K, fpu.c 21K, sysi86.c 21K, cpc_subr.c, copy_subr.c, comm_page_util.c, bootdev.c | `sparc/os` 21 files 148 KB: syscall.c 31K, polled_io.c, bootdev.c, cpr_sparc.c, bootops.c, archdep.c, iscsi_boot.c, ddi_arch.c, obpsym*.c, door_support.c, bitmap_arch.c, sundep.c; `v9/os`: simulator.c, v9dep.c, xregs.c |
| Syscall glue | `ia32/syscall`: getcontext.c, lwp_private.c | `v9/syscall` same |
| promif | `ia32/promif`: prom_enter.c, prom_exit.c, prom_panic.c (stubs; x86 has no PROM) | real OBP client in `psm/promif/ieee1275` |
| Inline hdrs | `intel/asm`: atomic.h bitmap.h byteorder.h clock.h cpu.h cpuvar.h htable.h mmu.h sunddi.h thread.h | none (SPARC uses .il) |
| ISA headers | `intel/sys`: 73+ files + acpi/fm/hotplug (archsystm.h asm_linkage.h controlregs.h cpu.h frame.h fp.h machlock.h machsig.h machtypes.h regset.h reg.h stack.h trap.h psw.h pcb.h ucontext.h synch32.h vmparam.h fasttrap_isa.h kdi_regs.h kdi_machimpl.h ddi_isa.h bootconf.h bootsvcs.h ...) | `sparc/sys`: 32+ files + fm/cpu, fpu (same set plus fsr.h utrap.h simulate.h cmpregs.h) |
| krtld | `ia32/krtld`: doreloc.c 8.8K, kobj_reloc.c 8.8K, kobj_isa.c 3K, kobj_convrelstr.c 2.3K, kobj_boot.c 1.9K, kobj_crt.s 1.8K | `sparc/krtld`: doreloc.c 23K, kobj_reloc.c 10.6K, kobj_isa.c, kobj_convrelstr.c, mapfile |
| DTrace | `intel/dtrace`: dtrace_asm.s 8.6K, dtrace_isa.c 16.7K, fasttrap_isa.c 45K, fbt.c 20K, sdt.c 13K + .conf | `sparc/dtrace`: dtrace_asm.s 14K, dtrace_isa.c 24K, fasttrap_isa.c 44K, fbt.c 50K, sdt.c 12K |
| kdi | `intel/kdi`: kdi_idt.c, kdi_idthdl.s, kdi_offsets.in, `ia32/kdi_asm.s`, `amd64/` | `sfmmu/ml/sfmmu_kdi.s`, sun4u mach_kdi files |
| Class knobs | `Makefile.intel`: `DEF_BUILDS = $(DEF_BUILDS64) $(DEF_BUILDS32)`, `ALL_BUILDS = $(ALL_BUILDS64) $(ALL_BUILDS32)`, `INLINES_32=$(UTSBASE)/intel/ia32/ml/ia32.il`, `CFLAGS_XARCH_32=$(i386_CFLAGS)`, `ASFLAGS_XARCH_32=$(i386_ASFLAGS)`, `COPTFLAG_32`, `DEBUG_DEFS_DBG32`; only class-split module `DRV_KMODS_32 += dnet` | 64-only (`DEF_BUILDS = $(DEF_BUILDS64)`, `CC_XARCH=-m64 -xarch=sparcvis`) |

Shared krtld `uts/common/krtld`: 10 files, 159 KB (kobj.c 112K, bootrd.c, kobj_bootflags.c, kobj_kdi.c, kobj_lm.c, kobj_stubs.c, kobj_subr.c, reloc.h, reloc_defs.h). `kobj.c` keys on `_OBP` (SPARC: `BOP_MOUNTROOT()`, `bop_temp_alloc()`), `!_OBP` (x86: `kobj_boot_mountroot()` ramdisk + `synthetic_bootaux()`), `_DBOOT`, `_RELSEG`, `__sparc` (STT_SPARC_REGISTER, redzone), `MODDIR_SUFFIX` (ISA subdir lookup). Comment: "on x86, we always boot with a ramdisk".

Shared DTrace `uts/common/dtrace`: dtrace.c 447K, fasttrap.c 63K, sdt_subr.c 55K, dcpc.c 32K, profile.c, lockstat.c, systrace.c. Platform hooks: `i86pc/os/dtrace_subr.c` 9.8K / `sun4/os/dtrace_subr.c` 5.7K.

### 2c. Platform level: `uts/i86pc` vs `uts/sun4` + `uts/sun4u` (template for `uts/amiga`)
`uts/i86pc` (42 dirs; `Makefile.i86pc` 9K ~330 lines, `Makefile.files` 6.3K, `Makefile.rules` 15K, `Makefile.workarounds`; `PLATFORM=i86pc`, `IMPLEMENTATIONS=i86hvm`, `MACHINE_DEFS=-D$(PLATFORM) -D_MACHDEP`, `PLATFORM_OFFSETS_32=$(UTSBASE)/$(PLATFORM)/ml/mach_offsets.in`, `KDI_OFFSETS_SRC=$(UTSBASE)/intel/kdi/kdi_offsets.in`):
- `ml/` 17 files 208 KB: locore.s 52K, syscall_asm.s 20K (32-bit), syscall_asm_amd64.s 37K, mpcore.s 15K, cpr_wakecode.s 23K, bios_call_src.s 10K, fb_swtch_src.s 9K, interrupt.s 5K, fast_trap_asm.s 5K, kdi_subr.s, comm_page.s, notes.s, genassym.c 6.5K, offsets.in 7.7K, mach_offsets.in 3.7K, ia32.il, amd64.il. `SPECIAL_OBJS_32 = locore.o fast_trap_asm.o interrupt.o syscall_asm.o`.
- `os/` 50 files + cpupm/, 1.19 MB: cpuid.c 136K, lgrpplat.c 99K, startup.c 88K, ddi_impl.c 75K, fakebop.c 67K (boot properties), trap.c 64K, intr.c 56K, mp_startup.c 52K, mp_machdep.c 48K, cmi_hw.c 48K, fastboot.c 42K, machdep.c 35K, microcode.c 31K, cpr_impl.c 27K, pmem.c 25K, timestamp.c 23K, cmi.c 22K, ibft.c 20K, x_call.c 19K, mp_pc.c 17K, cms.c 15K, mlsetup.c 14K, memscrub.c 14K, cpuid_subr.c 14K, dtrace_subr.c 9.8K, mp_implfuncs.c, pci_cfgspace.c, pci_cfgacc_x86.c, fpu_subr.c, biosdisk.c 7.7K, pci_* (mech1, mech1_amd, mech2, neptune, orion, bios), memnode.c, xpv_platform.c, ssp.c, smb_dev.c, graphics.c, mach_kdi.c, instr_size.c, ppage.c, mp_call.c, hold_page.c, dvma.c, acpi_stubs.c, mem_config_arch.c, mach_sysconfig.c.
- `vm/` 14 files 371 KB (x86 HAT): hat_i86.c 107K, vm_machdep.c 98K, htable.c 60K, hment.c 18K, i86_mmu.c 13K, kboot_mmu.c 12K, hat_kdi.c 8K, vm_dep.h 19K, htable.h 11K, hat_pte.h 9K, hat_i86.h 8K, hment.h, kboot_mmu.h.
- `dboot/` 11 files 101 KB (dboot_startkern.c 64K, dboot_multiboot2.c 8.5K, dboot_grub.s 8.5K, dboot_elfload.c 4.8K, dboot_printf.c, dboot_xen.s, dboot_asm.s, headers). `boot/` 9 files 58 KB (boot_console.c 21K serial/VGA early console, boot_keyboard.c 11K, boot_keyboard_table.c, boot_vga.c, boot_mmu.c, boot_gdt.s, boot_serial.h).
- `sys/` 74+ headers (machparam.h machsystm.h machcpuvar.h machthread.h machclock.h mach_mmu.h pc_mmu.h vm_machparam.h machprivregs.h psm*.h apic*.h apix*.h x_call.h xc_levels.h rm_platter.h smp_impldefs.h fastboot*.h clock.h tsc.h hpet*.h boot_console.h debug_info.h comm_page.h ...).
- `io/` 37+ (rootnex.c, isa.c, cbe.c, consplat.c, hardclk.c, microfind.c, todpc_subr.c, hrtimers.c, mp_platform_common.c, mp_platform_misc.c, cpudrv_mach.c, immu*.c, ppm_plat.c + dirs acpi acpi_drv amd_iommu apix dr fipe gfx_private ioat pci pciex pcplusmp ppm psm tzmon xsvc).
- `conf/`: Mapfile, Mapfile.amd64, Mapfile.bios, Mapfile.fb_swtch, confunix.c. `unix/Makefile`: `MAPFILE_32=$(MAPFILE)`, `MAPFILE_64=$(MAPFILE).amd64`, `GENUNIX_DIR=../../intel/$(GENUNIX)`, `CTFEXTRAOBJS=vers.o comm_page_ctf.o`, `ROOTMODULE=$(ROOT_PSM_KERN_DIR)/$(UNIX)` (= `/platform/i86pc/kernel/unix` for class 32).
- Platform modules: cpu/, pcplusmp/ apix/ uppc/ (interrupt PSMs), rootnex/ isa/ npe/ pci/ pcie/ pci-ide/, acpi_drv/ acpidev/ acpinex/ acpippm/ acpica, amd_iommu/, amd_opteron/ generic_cpu/ authenticamd/ genuineintel/ (CPU_KMODS), cpudrv/ ppm/ tzmon/ fipe/ ioat/ xsvc/ gfx_private/ cpc/ cpr/ dr/ drmach_acpi/ consconfig_dacf/, genassym/, i86hvm/.

`uts/sun4` (shared by sun4u/sun4v): `brand conf cpu io ml os sys vm` + `Makefile Makefile.files Makefile.rules`.
- `ml/` 11 files 145 KB (interrupt.s 65K, swtch.s 17K, subr_asm.s 12K, ip_ocsum.s 12K, copy.s 10K, offsets.in 10.6K, proc_init.s, locore.s 3.9K, xc.s, cpc_hwreg.s, genconst.c).
- `os/` 27 files 461 KB (startup.c 97K, trap.c 52K, ddi_impl.c 49K, visinstr.c 47K, intr.c 25K, x_call.c 25K, machdep.c 23K, mp_startup.c 19K, prom_subr.c 17K, mlsetup.c 14K, memlist.c 14K, dmv.c 10.5K, cpu_states.c, lgrpplat.c, memnode.c, iommutsb.c, mp_states.c, dtrace_subr.c, dvma.c, forthdebug.c, bus_func.c, mp_call.c, bcmp.c, mach_sysconfig.c, cpupm_mach.c, platmod.c, mem_config_arch.c).
- `vm/` 4 files 93 KB (vm_dep.c 27K, vm_dep.h 30K, sfmmu.c 31K, mach_kpm.h). `sys/` 38 headers. `io/`: cbe.c, ebus.c, ivintr.c, pcicfg.c, rootnex.c, su_driver.c (serial), tod.c, trapstat.c + efcode/fpc/px. `conf/`: Mapfile, confunix.c, genalign.s.

`uts/sun4u` (105+ dirs): `Makefile.sun4u` 13K ~380 lines (`PLATFORM=sun4u`, `PROMIF=ieee1275`, `PSMBASE=$(UTSBASE)/../psm`, `MACHINE_DEFS=-D$(PLATFORM) -D_MACHDEP -DSFMMU`, `DEF_BUILDS=$(DEF_BUILDS64)`; includes `sun4/Makefile.files`, `$(PLATFORM)/Makefile.files`, `sfmmu/Makefile.files`, `sparc/v9/Makefile.files`, `sparc/Makefile.files`, `sun/Makefile.files`, `$(SRC)/psm/promif/$(PROMIF)/common/Makefile.files`, `$(SRC)/psm/promif/$(PROMIF)/$(PLATFORM)/Makefile.files`, `common/Makefile.files`, `Makefile.uts`, `Makefile.workarounds`; 28 IMPLEMENTATIONS). `ml/` 11 files 196 KB (trap_table.s 92K, mach_locore.s 39K, wbuf.s 11K, mach_subr_asm.s 11K, mach_copy.s 10.5K, mach_interrupt.s 8.5K, mach_xc.s, cpr_resume_setup.s, memscrub_asm.s, mc-us3_asm.s, mach_offsets.in 9K). `os/` 15 files 251 KB (cpr_impl.c 50K, memscrub.c 38K, fillsysinfo.c 33K, plat_ecc_unum.c 32K, mach_startup.c 15K, ppage.c 14.5K, mach_cpu_states.c 14K, plat_ecc_dimm.c 11K, mach_ddi_impl.c 10K, mach_trap.c 9K, ecc.c 8K, cmp.c 7K, mach_mp_startup.c, cpc_subr.c, mach_mp_states.c). `sys/` 84+ headers. `io/` 53 files + i2c/pci/pciex/ppm/px. `unix/`, `genunix/`, `platmod/` are Makefile-only leaves; unix links against genunix + platmod + CPU lib via `LIBOPTS`, installs `$(ROOT_PSM_KERN_DIR)/unix` plus symlink `$(ROOT_PSM_KERN_DIR_32)/unix -> $(SUBDIR64)/unix`. CORE_OBJS (atomic.o bootops.o cmp.o cpc_hwreg.o cpc_subr.o cpupm.o mach_cpu_states.o mach_ddi_impl.o ecc.o fillsysinfo.o forthdebug.o hardclk.o hat_sfmmu.o hat_kdi.o iscsi_boot.o mach_copy.o mach_kpm.o mach_mp_startup.o mach_mp_states.o mach_sfmmu.o mach_startup.o mach_subr_asm.o mach_trap.o mach_vm_dep.o mach_xc.o mem_cage.o mem_config.o memlist_new.o memscrub.o memscrub_asm.o ppage.o sfmmu_kdi.o swtch.o); SPECIAL_OBJS (trap_table.o locore.o mach_locore.o sfmmu_asm.o mach_sfmmu_asm.o interrupt.o mach_interrupt.o wbuf.o).

### 2d. MMU layer
- x86: `uts/i86pc/vm` (14 files, 371 KB): hat_i86.c + htable.c + hment.c implement the HAT over hardware page tables; `i86pc/sys/{mach_mmu.h,pc_mmu.h,vm_machparam.h}`; `i86pc/os/startup.c` (KVA layout, kernelheap); dboot + kboot_mmu.c for early mappings.
- SPARC: `uts/sfmmu` (`vm/`: hat_sfmmu.c 417 KB ~12k lines, hat_sfmmu.h 87K, hat_kdi.c; `ml/`: sfmmu_asm.s 132K, sfmmu_kdi.s 8K) + `uts/sun4/vm` + `uts/sun4u/vm` (mach_sfmmu.c, mach_vm_dep.c) - software TLB-miss design.
- 68040 has a hardware table walk (3-level, 4K or 8K pages): use the x86 hat_i86/htable/hment structure as template, not sfmmu. Expect new `uts/amiga/vm/{hat_m68k.c,htable.c,hment.c,vm_machdep.c,kboot_mmu.c}` of ~300 KB.

### 2e. `usr/src/common` (shared kernel+user ISA code)
`common/atomic/{i386,amd64,sparc,sparcv9}/atomic.s` + `atomic.c` (C fallback) - one file per ISA; 68040 has CAS/CAS2 so `atomic.s` can be real. `common/dis/i386/dis_tables.{c,h}` (x86 decoder shared by libdisasm and kernel `instr_size.c`); SPARC decoder lives in `lib/libdisasm/common/dis_sparc*.c`. Also `common/util`, `common/bignum`, `common/crypto` (optional per-ISA asm), `common/elfcap`, `common/ctf`, `common/dtrace`. 40 dirs total.

### 2f. Boot: `usr/src/boot` (x86) vs `usr/src/psm` (SPARC/OBP)
- `boot/`: FreeBSD loader snapshot "Current snapshot from freebsd revision 296191" (Mar 2016, Toomas Soome). `sys/boot/{i386 (boot0 boot0sio boot2 btx cdboot gptzfsboot libi386 loader pmbr pxeldr common), efi, common (51+: load_elf32/64.c, load_elf_obj.c, multiboot2.c, reloc_elf*.c, self_reloc.c, part.c, gpt.c, disk.c, bcache.c, interp*.c, ufsread.c), zfs, libstand, ficl, forth, userboot, usb, ofw, uboot, fdt, arm, arm64, sparc64 (unbuilt)}`, `lib/{libc,libstand,libz}`, `include`, `Makefile.version`. Hands off to `uts/i86pc/dboot` via multiboot/multiboot2 with the boot archive (ramdisk) in memory.
- `psm/`: `Makefile`, `Makefile.psm.64`, `promif/ieee1275/{common (35 prom_*.c 1275 client-interface wrappers), sun4 (18: prom_alloc.c prom_map.c prom_mmu.c prom_mem.c prom_set_traptable.c prom_init.c prom_cpuctl.c prom_idprom.c prom_macaddr.c ...), sun4u, sun4v, README.promif}`, `stand/{boot, bootblks, bootlst, cpr, lib, sys, README}`. `stand/boot/{common (heap_kmem.c readfile.c), sparc/common (25: boot_plat.c bootops.c bootprop.c bootflags.c ramdisk.c inetboot.c wanboot.c wbcli.c nfsconf.c sun4u_srt0.s sun4u_machdep.c sun4u_memlist.c sun4v_machdep.c sun4x_standalloc.c sun4dep.c machdep.c sparcv9_subr.s sparc.il mapfile), sparcv9/{sun4,sun4u,sun4v}}` = inetboot/wanboot network standalone. `stand/bootblks/{common (boot.fth rd.fth util.fth mkbb.sh), zfs/{common,i386,sparc}, ufs, hsfs, Makefile.1275}` = Forth bootblocks that read the boot archive. `stand/lib/{boot/{sparc,sparcv9}, names, promif/sparcv9}`; `stand/cpr`. `usr/src/stand/lib/{fakeboot,fs,inet,sa,sock,tcp,tcpstubs,xdr}` = standalone NFS/TCP library.
- SPARC disk-boot flow (medium confidence): OBP -> Forth bootblk reads `/platform/sun4u/boot_archive` and exposes it as a ramdisk -> OBP ELF-loads unix -> krtld `_OBP` branch calls `BOP_MOUNTROOT()` to read modules via OBP. No dboot/loader stage on SPARC.

### 2g. libc per ISA (`lib/libc`)
Top: `amd64 capabilities common/sys i386 i386_hwcap1 i386_hwcap2 i386_hwcap3 inc port sparc sparcv9` + Makefile, Makefile.targ, README.
- `i386/`: `crt/` (_rtboot.s _rtld.c alias_boot.h cerror.s cerror64.s); `gen/` 42 files (setjmp.s siglongjmp.c makectxt.c _stack_grow.s _getsp.s i386_data.s memccpy/memchr/memcmp/memcpy/memset.s strcat/strchr/strcpy/strlen/strncat/strncmp/strncpy/strrchr.s strcmp.c wschr/wslen/wsncmp.s byteorder.s byteorder64.c _div64.s _mul64.s _divdi3.s ladd/lsub/lmul/ldivide/lshiftl/lsign.s lock.s alloca.s abs.s cuexit.s ecvt.c endian.c siginfolst.c sync_instruction_memory.c); `sys/` 19 (syscall.s door.s forkx.s forkallx.s vforkx.s getcontext.s __getcontext.s lwp_private.s _lwp_mutex_unlock.s gettimeofday.s __clock_gettime.c __clock_gettime_sys.s uname.s nuname.s sysi86.s uadmin.c __uadmin.s ptrace.c xstat.c); `threads/` (asm_subr.s machdep.c tls_get_addr.s i386.il); `fp/` 35 (x87 control word + complex arithmetic); `unwind/unwind_frame.s`; `inc/SYS.h` (syscall trap macros); `etc/`; genassym.c; offsets.in; `Makefile.com` ~1,200 lines (CRTOBJS=cerror.o cerror64.o, DYNOBJS=_rtbootld.o, FPOBJS ~29, FPASMOBJS 5, GENOBJS, SYSOBJS, THREADSMACHOBJS=machdep.o, THREADSASMOBJS=asm_subr.o, UNWINDMACHOBJS=unwind.o, UNWINDASMOBJS=unwind_frame.o, ATOMICOBJS=atomic.o from `common/atomic/$(MACH)`; assym.h from offsets.in + genassym; `-e __rtboot`, `-z rtldinfo=tls_rtldinfo`, `-zdtrace=dtrace_data`).
- `sparc/`: same shape (crt adds _ftou.c hwmuldiv.s stret.s; gen 37 files incl. smt_pause.s swapctxt.c getctxt.c _xregs_clrptr.c).
- `capabilities/`: SPARC-only hwcap libraries (sun4u, sun4u-opl, sun4u-us3-hwcap1, sun4u-us3-hwcap2, sun4u-us3/common, sun4v, sun4v-hwcap1, sun4v-hwcap2); x86 hwcap variants are `libc/i386_hwcap{1,2,3}`. Not needed for m68k.
- `lib/crt/{common (common-crt.c values-Xa/Xc/Xs/Xt/xpg4/xpg6.c), i386 (crti.s crtn.s fsr.s mach-crt1.s), sparc, sparcv9, amd64}`.

### 2h. Link-editor (`cmd/sgs`, 46 subdirs)
- `libld/common` (44 files): machrel.intel.c, machrel.amd.c, machrel.sparc.c, machsym.sparc.{c,h}, relocate.c, unwind.c, ldmachdep.c, libld.{intel,sparc}.msg; `libld/{i386,amd64,sparc,sparcv9,common}`. `Makefile.com` (~180 lines): `G_MACHOBJS32 = doreloc_sparc_32.o doreloc_x86_32.o`, `G_MACHOBJS64 = doreloc_sparc_64.o doreloc_x86_64.o`, `L_SPARC_MACHOBJS32 = machrel.sparc32.o machsym.sparc32.o`, `L_X86_MACHOBJS64 = machrel.amd64.o`, `KRTLD_I386 = $(SRCBASE)/uts/$(VAR_PLAT_i386)/krtld`, `KRTLD_SPARC`, `-DDO_RELOC_LIBLD -D_ELF64` - ld(1) compiles the kernel's `uts/<isa>/krtld/doreloc.c` directly; one doreloc.c serves krtld, ld.so.1 and ld. ld is multi-target (builds all ISA back-ends).
- `rtld`: `common/` 43 files (ISA-neutral: analyze.c elf.c dlfcns.c tls.c setup.c ... mapfile-order-gcc); `i386/` (Makefile _setup.c boot.s boot_elf.s caller.s i386_elf.c); `sparc/` (+ boot_a.out.s common_sparc.c sparc_a.out.c sparc_elf.c); `amd64`, `sparcv9`, `mdbmod`; rtld.{intel,sparc}{,32,64}.msg. `Makefile.com` (~145 lines) pulls `$(G_MACHOBJS:%.o=$(SRCBASE)/uts/$(PLAT)/krtld/%.c)`.
- `libconv/common`: relocate_{i386,amd64,sparc}.c + .msg, symbols_sparc_machelf.c, arch.c, cap.c, cap_machelf.c, corenote.c, dynamic.c, elf.c, phdr.c, sections.c - all three ISAs compiled unconditionally (COMOBJS).
- `cmd/sgs/include/{i386/machdep_x86.h, sparc/, machdep.h, _machelf.h, libld.h, rtld.h ...}`; `uts/common/sys/elf_{386,SPARC,amd64}.h`.
- New ISA = `machrel.m68k.c` + `uts/m68k/krtld/doreloc.c` + `include/m68k/machdep_m68k.h` + `libconv/common/relocate_m68k.c` + `elf_M68K.h` + `rtld/m68k/{boot.s,boot_elf.s,caller.s,m68k_elf.c,_setup.c}` + elfcap/libconv table entries.

### 2i. Debuggers / tracing / proc
- `cmd/mdb` (Makefile.kmdb, Makefile.libstand, Makefile.libstandctf, Makefile.module ...): `common/{mdb, kmdb (44 files + kctl: kmdb_dpi.c kmdb_kdi.c kmdb_kvm.c kmdb_main.c kmdb_promif.c kaif_start.c ...), libstand}`; `intel/{amd64, ia32 (71 per-module Makefile dirs; all deleted by 8685), kmdb (16: kaif.c/h kmdb_dpi_isadep.c kmdb_fault_isadep.c kmdb_kdi_isadep.c kmdb_promif_isadep.c kvm_isadep.c kvm_cpu_amd.c kvm_cpu_p4.c kmdb_context_off.in kmdb_asmutil.h), libstand, mdb (kvm_ia32dep.c kvm_amd64dep.c kvm_isadep.c mdb_ia32util.c mdb_amd64util.c proc_ia32dep.c proc_amd64dep.c mdb_kreg.h mdb_kreg_impl.h mdb_isautil.h), modules}`; `sparc/{v7, v9, kmdb (20: kaif.c kaif_activate.c kaif_start_isadep.c kmdb_asmutil.s kmdb_makecontext.c ...), mdb, modules}`; `i86pc/{modules,Makefile.i86pc}`, `i86xpv`, `sun4u`, `sun4v`.
- `lib/libdisasm/{common (dis_i386.c dis_sparc.c dis_sparc.h dis_sparc_fmt.c dis_sparc_instr.c dis_s390x.c libdisasm.c libdisasm_impl.h), i386, amd64, sparc, sparcv9}`. No m68k decoder anywhere in the gate.
- `lib/libproc/{common, i386 (Makefile Pisadep.c), amd64, sparc (Pisadep.c), sparcv9}`; `lib/libc_db/{common,i386,amd64,sparc,sparcv9}`; `lib/libdtrace/{common, i386 (dt_isadep.c regs.d.in regs.sed.in), amd64, sparc, sparcv9}`; `lib/libsaveargs` (amd64 only).
- regset/core ABI: `uts/<isa>/sys/{regset.h,ucontext.h,procfs_isa.h,old_procfs.h}`.

### 2j. `uts/common/sys/isa_defs.h` (~400 lines)
Keys on `__x86_64||__amd64` -> `__i386||__i386__` -> `__sparc||__sparcv9||__sparc__` (`__sparcv8`/`__sparcv9` sub-blocks) else `#error "ISA not supported"`. No PowerPC remnant. Sets: `_LITTLE_ENDIAN/_BIG_ENDIAN`, `_STACK_GROWS_DOWNWARD`, `_LONG_LONG_LTOH/HTOL`, `_BIT_FIELDS_LTOH/HTOL`, `_IEEE_754`, `_CHAR_IS_SIGNED`, `_ALIGNMENT_REQUIRED` (0 on i386, 1 elsewhere), `_LONG_ALIGNMENT/_POINTER_ALIGNMENT/_MAX_ALIGNMENT`, `_ILP32`/`_LP64` (+`_I32LPx` under `_KERNEL`), `_MULTI_DATAMODEL` (64-bit only), `_SUNOS_VTOC_8|16`, `_DMA_USES_PHYSADDR|VIRTADDR`, `_FIRMWARE_NEEDS_FDISK|_NO_FDISK_PRESENT`, `_PSM_MODULES`, `_RTC_CONFIG`, `_SOFT_HOSTID`, `_DONT_USE_1275_GENERIC_NAMES`, `_HAVE_CPUID_INSN`, `_HAVE_TEM_FIRMWARE`, `_OBP`, `__i386_COMPAT`. Also defines `__x86`/`__amd64` itself. m68k block: `_BIG_ENDIAN`, `_ILP32`, `_LONG_LONG_HTOL`, `_BIT_FIELDS_HTOL`, `_STACK_GROWS_DOWNWARD`, `_CHAR_IS_SIGNED`, `_IEEE_754`, `_ALIGNMENT_REQUIRED 1`, `_LONG_ALIGNMENT 4` (needs -malign-int), no `_OBP`, no `_MULTI_DATAMODEL`.

### 2k. `uts/common/os` and `Makefile.master` knobs
`uts/common/os`: 174 files, ~5-6 MB (main.c 17K, cpu.c 95K, lgrp.c 119K, mem_config.c 83K, dumpsubr.c 80K, clock.c 75K, modctl.c, modconf.c, modsubr.c, modsysfile.c, sysent.c, panic.c, timer.c, vmem.c, kmem.c, sunddi.c, sunndi.c, sunpci.c, swapgeneric.c, vfs_conf.c, zone.c ...). No ISA-named files; ISA dependencies via `#ifdef` and machdep headers. `uts/common/sys`: several hundred headers (NOT COUNTED).
`Makefile.master`: MACH from env ("per uname -p"); `MACH64_1=$(MACH:sparc=sparcv9)`, `MACH64=$(MACH64_1:i386=amd64)`; `MACH32_1=$(MACH:sparc=sparcv7)`, `MACH32=$(MACH32_1:i386=i86)`; `NATIVE_MACH=$(MACH:amd64=i386)`; `sparc_XARCH=-m32`, `sparcv9_XARCH=-m64`, `i386_XARCH=-m32`, `amd64_XARCH=-m64 -Ui386 -U__i386`; `sparc_BUILD64=` / `i386_BUILD64=` empty (= 64-bit IS built), `BUILD64=$($(MACH)_BUILD64)`; `SPARC_BLD`/`INTEL_BLD`; `<mach>_CC = $(ONBLD_TOOLS)/bin/$(MACH)/cw -_gcc`; `<mach>_LINT = $(SPRO_ROOT)/sunstudio12.1/bin/lint`; `ELFWRAP_FLAGS64=-64`; `COMPILE.b=$(ELFWRAP) $(ELFWRAP_FLAGS$(CLASS))`. m68k needs `m68k_*` siblings for every `sparc_`/`i386_` pair and `m68k_BUILD64=$(POUND_SIGN)`; the BUILD64-off path has been unused since sparcv7 removal (rot risk).
`tools/cw/cw.c` (CW_VERSION 1.30 in 2018): `do_shadow = (getenv("CW_NO_SHADOW") ? 0 : 1)`; `CW_SHADOW_SERIAL`; `CW_CC_DIR`/`CW_GCC_DIR`/`CW_CC`/`CW_GCC` overrides; `xtbl` flag translation table only `#if defined(__x86) / #elif defined(__sparc)`; `-m32/-m64` handling in `do_gcc`; no gcc version check.

## 3. Hard requirements vs deferrable for "serial console shell on ramdisk root"

MUST EXIST (new code modelled on the x86 ILP32 path):
1. `Makefile.master`, `uts/Makefile.uts`, `uts/Makefile`, `Makefile.psm`, `Makefile.lib`/`Makefile.cmd` ISA dirs, `Targetdirs`: m68k rows; `m68k_ARCHITECTURES = amiga m68kvirt m68k`; `m68k_PREREQ = m68k`; `m68k_BUILD64=$(POUND_SIGN)`; cw xtbl branch; `CW_NO_SHADOW=1`; re-add CLASS=32 knobs removed by 8685.
2. `isa_defs.h` block; `uts/m68k/sys/` ~60-70 headers cloned from intel/sys + ia32 (asm_linkage.h machlock.h machtypes.h regset.h frame.h stack.h trap.h psw/sr.h pcb.h ucontext.h fp.h machsig.h archsystm.h synch32.h vmparam.h privregs.h kdi_regs.h stub procfs_isa.h old_procfs.h elf_M68K.h ddi_isa.h fasttrap_isa.h stub).
3. `uts/m68k/ml/` (copy.s, m68k_subr.s, lock_prim.s, swtch.s, trap/exception entry, modstubs.s, float.s for 68040 FPU save/restore) and `uts/m68k/os/` (archdep.c sundep.c syscall.c sendsig.c fpu.c ddi_m68k.c cpc stubs copy_subr.c bootdev.c): 250-600 KB new code (x86 equivalents: 374 KB asm + 259 KB C).
4. `uts/m68k/krtld/{doreloc.c,kobj_reloc.c,kobj_isa.c,kobj_convrelstr.c,kobj_boot.c,kobj_crt.s}` (R_68K_*; x86 doreloc.c is only 8.8K) - consumed by ld(1) and ld.so.1 too.
5. `uts/amiga/`: Makefile.amiga, Makefile.files/.rules/.workarounds, `ml/` (locore.s interrupt.s syscall_asm.s offsets.in mach_offsets.in genassym.c), `os/` (startup.c mlsetup.c machdep.c trap.c intr.c ddi_impl.c single-CPU mp_* stubs, trivial x_call.c, timestamp/hardclk via CIA timers (Amiga) or goldfish timer (QEMU), fakebop-equivalent boot properties, memlist, dtrace_subr.c stub, mach_sysconfig.c, mem_config_arch.c), `vm/` (68040 HAT: hat_m68k.c htable.c hment.c vm_machdep.c kboot_mmu.c vm_dep.h), `sys/` (machparam.h machsystm.h machcpuvar.h machthread.h machclock.h vm_machparam.h x_call.h xc_levels.h mach_mmu.h machprivregs.h), `conf/{Mapfile,confunix.c}`, `unix/Makefile`, rootnex, cbe (clock backend), serial console driver in `uts/amiga/io/` (Paula serial on Amiga; goldfish-tty on QEMU virt) wired into consconfig/cn, `boot/` early console.
6. Boot handoff: a tiny native stage (QEMU `-kernel`; AmigaOS/AROS-side loader on Amiga) that enters unix with memlist + boot archive address, then the `!_OBP` krtld ramdisk path (`kobj_boot_mountroot()`) with bootops/fakebop property emulation. boot_archive tooling (`cmd/boot/bootadm`, `cmd/boot/scripts`) is ISA-neutral in format. Porting `uts/i86pc/dboot` + `usr/src/boot` is NOT planned.
7. Userland ISA glue: `common/atomic/m68k/atomic.s`, `lib/libc/m68k/{crt,gen,sys,threads,fp,unwind,inc/SYS.h,Makefile.com,genassym.c,offsets.in}`, `lib/crt/m68k/{crti.s,crtn.s,mach-crt1.s}`, `cmd/sgs/{libld/common/machrel.m68k.c, include/m68k/machdep_m68k.h, rtld/m68k/{boot.s,boot_elf.s,caller.s,m68k_elf.c,_setup.c}, libconv/common/relocate_m68k.c + .msg}`, `uts/common/sys/elf_M68K.h` - unavoidable because ld(1)/ld.so.1 must emit/process R_68K relocs even for the kernel build; libc needed for init/sh.
8. Kernel modules for ramdisk root: genunix, specfs, ufs (or bootfs + ramdisk `uts/common/io/ramdisk.c`), procfs, tmpfs, fifofs, devfs/dev, sockfs, cn/consconfig, ldterm/ttcompat, ptm/pts, TS/TS_DPTBL, elfexec, intpexec, doorfs, minimal kcf/crypto, random - all ISA-neutral C under uts/common, needing ~100 leaf `uts/m68k/<mod>/Makefile` copies of `uts/intel/<mod>/Makefile`.

DEFERRABLE: DTrace (`uts/m68k/dtrace/*`, `lib/libdtrace/m68k`; fasttrap/fbt need instruction decoding - stub `dtrace_subr.c`), kmdb/kaif (`cmd/mdb/m68k/kmdb`, `uts/m68k/kdi`), mdb `kvm_isadep`/`proc_isadep` (stubs suffice to link mdb), libdisasm (no m68k decoder exists), libproc/libc_db Pisadep (pstack/truss/gcore only), libc/capabilities (SPARC hwcap), cpc/pcbe, cpr, lgrp, mem_config, any amd64/sparcv9 dirs (ILP32-only ISA: no `$(MACH64)` subtrees at all), `usr/src/boot` loader, ZFS (milestone M4), platform IMPLEMENTATIONS, i86hvm-style sub-platforms.

## 4. Gotchas: ILP32 kernel state

- ZFS on the 32-bit kernel was still built and shipped in 2018 (`uts/intel/zfs/Makefile` had no class override; 8685 had to strip 32-bit `kernel/fs/zfs` paths from `system-file-system-zfs.mf`). Explicit ILP32 knobs in ZFS: `arc.c` `arc_memory_throttle()` `#if defined(__i386)` caps available memory by `vmem_size(heap_arena, VMEM_FREE)`; `arc_init()` caps `arc_meta_limit` to `vmem_size(heap_arena, VMEM_ALLOC|VMEM_FREE)/2`. 13939 removed these in 2021; re-add them under `_ILP32` at the fork. The 32-bit kernel heap (~1 GB of KVA minus segkp/segmap, set up in `i86pc/os/startup.c`) is the practical ARC limit. Peter Tribble (2015): 32-bit kernel "effectively untested because almost nobody uses it". NOT VERIFIED: whether a 32-bit ZFS root actually booted in 2018 distros (OpenIndiana dropped 32-bit kernels at 2017.04; OmniOS called i386 "effectively unusable" in Feb 2016; Tribblix 0m20 was the last with meaningful 32-bit support).
- 64-bit-only modules at the 2018 point: the only class-split line was `DRV_KMODS_32 += dnet`; nvme, bnxe, cxgbe, i40e, xhci, ixgbe, ioat, dtrace, zfs Makefiles had no class override - essentially the whole module set compiled as ILP32. Byte order was never exercised in x86 code; expect latent little-endian assumptions in x86-origin drivers (virtio vioblk/vioif in `uts/common/io/virtio` use ddi_get* accessors, probably OK - verify). SPARC is the only big-endian reference and `uts/sun/io` holds the shared Sun drivers (zs/su serial lineage).
- Alignment: all illumos ISAs have `_LONG_ALIGNMENT >= 4`; the default m68k GCC ABI aligns int/long/pointers to 2 bytes. Build with `-malign-int` (or struct/CTF/sizeof assumptions break); 68040 traps only on misaligned instruction fetch.
- Compiler at the 2018 point: GCC default (`__SUNC=#`), `GCC_ROOT=/opt/gcc/4.4.4` (illumos-patched gcc 4.4.4 "il"), Studio shadow unless `CW_NO_SHADOW`, lint hard-wired to sunstudio12.1, stabs debug format. GCC 7 shadow arrived 2018-11-27 ("GCC 7 shadow compiler; no more lint"); GCC 7 primary 2019-06-17; GCC 10 primary 2023-04-12. This is why the fork moved to Aug 2021.
- `_MULTI_DATAMODEL` is off in ILP32 (no 32-on-64 compat), but MODDIR_SUFFIX/SUBDIR64 and the `kernel/<mod>` vs `kernel/amd64/<mod>` install logic in Makefile.uts, Makefile.psm and Targetdirs need the 32-bit-only layout that 8685 deleted; read the 8685 diffs to `Targetdirs`, `exception_lists/packaging` and the four Makefile.* for exactly what to restore.
- No OBP/IEEE1275 on Amiga/QEMU virt: SPARC promif/bootblks/inetboot machinery is not reusable; use the x86 fakebop/bootops property model and the `!_OBP` krtld path. QEMU m68k virt passes a bootinfo list (not FDT); Amiga Kickstart is not a 1275 client interface.
- CTF: `ctfconvert`/`ctfmerge` in `$(ONBLD_TOOLS)` (libctf/libelf) must understand EM_68K ELF and relocs; stub-able (no CTF) for first boot, mandatory for `mdb -k` later.
- Hostid/TOD/RTC: x86 uses `_SOFT_HOSTID`/`_RTC_CONFIG` and `todpc_subr.c`; the A4000 has an RP5C01 RTC (Copperline models it) - needs a small tod driver; QEMU virt has goldfish-rtc.

## Sources
- https://illumos.org/docs/developers/flagdays/
- https://github.com/illumos/illumos-gate/commit/95cb2e31c87e3febe7315833e075c1b16451e44c
- https://github.com/illumos/illumos-gate/commit/141fdc8db8933e5c2d4fe6a0f5ee25c59f0a0231
- https://github.com/illumos/illumos-gate/commit/86ef0a63e1cfa5dc98606efef379365acca98063
- https://www.illumos.org/issues/8685
- https://tribblix.org/32bit.html
- https://ptribble.blogspot.com/2015/09/the-32-bit-dilemma.html
- https://openindiana.org/pipermail/openindiana-discuss/2024-January/026610.html
- https://omnios.org/ml-archive/2016-February/006419.html
- https://omnios.org/ml-archive/2015-May/004974.html

## Critical files (read these first at `755ccbc`)
- usr/src/Makefile.master (MACH/XARCH/BUILD64/cw knobs)
- usr/src/uts/Makefile.uts (CLASS/MACH toolchain tables)
- usr/src/uts/i86pc/Makefile.i86pc (platform template)
- usr/src/uts/common/sys/isa_defs.h
- usr/src/uts/common/krtld/kobj.c (boot path: `_OBP` vs ramdisk)
- the diff of 8685 (`95cb2e3`) and of 13939 (`86ef0a6`)
