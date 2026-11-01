# Host setup: fork the gate, borrow arm64-gate's plumbing, build the toolchain

Step-by-step for M0 on the illumos x86 build host. Written 2026-11-01 from the sources named
in each step; nothing here has run on the host yet, so expect to adjust paths on the first pass.
The files referred to live in `host/` of this repo.

Layout on the host (mirrors richlowe/arm64-gate: gate inside the build repo, toolchain next to it,
env file reaches the toolchain through `$CODEMGR_WS/../build`):

    ~/amillumos/amillumos-build/            this repo
        host/Makefile  host/env/m68k  host/bin/host-check.sh  host/patches/
        host/amillumos-gate/                illumos-gate fork, branch amiga  (= CODEMGR_WS)
        host/build/cross/                   cross binutils, gcc, illumos tools proto (prefix)
        host/build/sysroot/                 m68k headers, later libc
        host/src/gcc  host/src/binutils-gdb
        host/stamps/

## Facts checked while writing this

- Fork point `755ccbcfa27de6aa07ebf138548b68dc6a9ae02d` = "14018 pcieadm save-cfgspace usage
  incorrect", 2021-08-12. Its child `86ef0a63e1c` is 13939, the ILP32-removal commit whose diff is
  the checklist of `_ILP32` sites in `uts/common`.
- **The CLASS=32 kernel build knobs are still present at the fork point.** 8685 (2018) removed
  `DEF_BUILDS32`, `ALL_BUILDS32`, `CLASS_OBJ32`, `OBJS_DIR_OBJ32` from `uts/Makefile.uts` and the
  `obj32`/`debug32` rules from `uts/Makefile.targ`, but at `755ccbc` those lines are back
  (`Makefile.uts` lines 65-91, `Makefile.targ` lines 278-286; only the fallback `CLASS_ = 64`
  stayed). Nothing from 8685's diff has to be re-added; `uts/m68k/Makefile.m68k` simply sets
  `DEF_BUILDS = $(DEF_BUILDS32)` and `ALL_BUILDS = $(ALL_BUILDS32)` where `Makefile.intel`
  line 104 says 64. The rest of 8685 is mdb/cmd 32-bit Makefiles and manifests, irrelevant here.
- Compilers at the fork point: `Makefile.master` primary `gcc7` at `/usr/gcc/7`; GCC 10 is a known
  shadow since 13812 (2021-02-14), so `-_gcc10=` flags exist. `env/m68k` picks GCC 7 if the host
  has it, else GCC 10, and sets no shadow compilers. The variable `CW_NO_SHADOW` does not exist in
  this gate; "no shadows" is simply an unset `SHADOW_CCS`.
- `bldenv` exports `MAKEFLAGS=e`, so environment variables override `Makefile.master` defaults.
- At the fork point `bldenv`/`nightly` have no `-T`; `Makefile.tools` and `Makefile.master` use
  `$(MACH)` for the tools proto and `$(ONBLD_TOOLS)/bin/$(MACH)/cw`. richlowe added the cross
  plumbing in richlowe/illumos-gate (branch `arm64-gate`):
  - `c14727bb0` 2023-03-16 "nightly, bldenv: accept -T <target>" (bldenv, nightly, nightly.1onbld)
  - `38aa9e73c` 2023-03-23 "bldenv: make target clear"
  - `cb6653583` 2022-11-12 "Initial AArch64 port in an illumos-y build environment": the
    `NATIVE_MACH`/`CROSS_BLD` machinery in `Makefile.master` (48 `NATIVE_MACH` uses today),
    `Makefile.tools` (`TOOLS_PROTO=.../root_$(NATIVE_MACH)-nd`), plus the whole aarch64 port.
    Only the build-system hunks are wanted; the `cw.c` rewrite there (699 changed lines, drops
    the Studio `-xarch` tables) is not needed because m68k CPU flags are passed as `-_gcc=` which
    `cw` forwards untouched.
- Toolchain sources: GCC = `illumos/gcc` branch `il-10_4_0` (what `/usr/gcc/10` is built from;
  carries `__illumos__`, `-msave-args`, libc SSP). The aarch64 target configuration to copy
  for `m68k-*-solaris2*` is in richlowe/gcc: `e09303585` (config.gcc, config.host,
  `gcc/config/aarch64/sol2.h`, `t-sol2`, `libgcc/config.host`), `af5b6a6c6` (the real
  `sol2.h`, 406 lines), `8177a00c0` (no GNU ld), `11887da00` (`libgcc/config/aarch64/t-sol2`),
  `5926f2d9a` (libgcc with Solaris CRTs). GCC 10.4 has no `m68k-*-solaris*` or `-sysv4` target
  left (`gcc/config.gcc` lines 2334-2404: elf, netbsdelf, openbsd, uclinux, linux, rtems).
  binutils = richlowe/binutils-gdb `illumos-arm64-2-43-1` (2.43.1 + commit `942136298` which also
  carries gnulib fixes for the illumos host). The m68k ELF target needs no binutils patch.
- GitHub: `gh` on the Mac is logged in as `gbulfon`.

## Step 0: preflight on the host

    ./host/bin/host-check.sh

Read off: which GCC exists (7 or 10 at `/usr/gcc/N` or `/opt/gcc-N`), where `gmp.h` is (that
directory is `GMPINCDIR`), python3 minor versions, perl version, free disk (want 25 GB).

## Step 1: packages

OpenIndiana:

    sudo pkg install build-essential developer/versioning/git library/gmp library/mpfr \
        library/mpc text/texinfo developer/build/gnu-make archiver/gnu-tar

OmniOS (r151046 or later):

    sudo pkg install pkg:/developer/illumos-tools pkg:/developer/gcc10 pkg:/library/gmp \
        pkg:/library/mpfr pkg:/library/mpc pkg:/ooce/text/texinfo pkg:/developer/build/gnu-make

Another distribution (XStreamOS): the same set by whatever names it uses; the hard requirements
are GCC 10 (to build GCC 10.4 and the gate's tools), git, gmake, rsync, gmp/mpfr/mpc headers,
texinfo (binutils/gdb docs), python3 and perl.

## Step 2: create the two repositories (from the Mac)

    gh repo fork illumos/illumos-gate --fork-name amillumos-gate --clone=false --default-branch-only

    cd ~/Claude/Amillumos
    git init -b main
    git add .
    git commit -m "Amillumos: loader, hello kernel, emulator scripts, host build layout"
    gh repo create amillumos-build --private --source=. --remote=origin --push

`.gitignore` already excludes `build/`, `host/amillumos-gate/`, `host/build/`, `host/src/`,
`host/stamps/`. Make the fork private too if preferred (`gh repo edit gbulfon/amillumos-gate --visibility private`).

## Step 3: clone and branch on the host

    mkdir -p ~/amillumos && cd ~/amillumos
    git clone https://github.com/gbulfon/amillumos-build.git
    cd amillumos-build/host
    git clone https://github.com/gbulfon/amillumos-gate.git          # ~1.5 GB
    cd amillumos-gate
    git remote add upstream https://github.com/illumos/illumos-gate.git
    git remote add arm64 https://github.com/richlowe/illumos-gate.git
    git fetch arm64 arm64-gate                                        # for cherry-picks and git show
    git checkout -b amiga 755ccbcfa27de6aa07ebf138548b68dc6a9ae02d
    git push -u origin amiga
    gh repo edit gbulfon/amillumos-gate --default-branch amiga        # gh on either machine

Check:

    git log -1 --format='%h %ad %s' --date=short        # 755ccbcfa 2021-08-12 14018 pcieadm ...
    git show --stat 86ef0a63e1c | tail -1                # 13939: the _ILP32 checklist
    grep -n 'DEF_BUILDS32\|CLASS_OBJ32' usr/src/uts/Makefile.uts   # present

## Step 4: smoke-test the 2021 gate on this host, unmodified

Builds the tools and the x86 headers natively with `env/m68k` (MACH=i386 here), about 15 minutes:

    cd ~/amillumos/amillumos-build/host
    make native-tools
    ls amillumos-gate/usr/src/tools/proto/root_i386-nd/opt/onbld/bin/i386/   # cw ctfconvert ctfmerge ...
    ls amillumos-gate/proto/root_i386/usr/include/sys/isa_defs.h

Likely snags, in order of probability: `GNUC_ROOT` wrong (edit the search list in `env/m68k`);
python 3.12+ versus 2021 onbld scripts (set `PYTHON3_VERSION=3.9` or `3.11` in `env/m68k` if
installed); smatch under `usr/src/tools/smatch` failing to build with a newer host GCC (remove
`smatch` from `SUBDIRS` in `usr/src/tools/Makefile` for now, it is a shadow compiler we do not
use). Fix, re-run `make native-tools` after `rm stamps/native-tools-stamp`. Commit the fixes on
`amiga` as "build: make the 2021 gate's tools build on a 2026 host".

## Step 5: borrow the cross-build plumbing (gate commits 1 to 3)

    cd ~/amillumos/amillumos-build/host/amillumos-gate

    # 1. -T <target> for bldenv and nightly
    git cherry-pick c14727bb0 38aa9e73c
    #    conflicts are expected (two years of bldenv/nightly drift); keep our side plus the -T hunks,
    #    then: git add -A && git cherry-pick --continue

    # 2. NATIVE_MACH / CROSS_BLD in the make infrastructure; take only the build-system files
    git show cb6653583 -- usr/src/Makefile.master usr/src/Makefile.master.64 \
        usr/src/tools/Makefile.tools usr/src/Makefile usr/src/Targetdirs > /tmp/cross-infra.diff
    git apply --3way --reject /tmp/cross-infra.diff
    #    then edit: every "aarch64" hunk becomes the m68k row of step 6, all
    #    $(ONBLD_TOOLS)/bin/$(MACH) become $(NATIVE_MACH), CW= and BUILD_LD= included.
    #    Reference for the final shape: git show arm64/arm64-gate:usr/src/Makefile.master | grep -n NATIVE_MACH

    # 3. verify native still builds, then commit
    rm ../stamps/native-tools-stamp && (cd .. && make native-tools)
    git commit -am "build: cross-build plumbing from arm64-gate (-T, NATIVE_MACH, CROSS_BLD)"

## Step 6: the first m68k commits (gate commits 4 to 7)

Goal of this step is only `make -T m68k sgs` succeeding, which installs the m68k headers. Each
item is one commit.

1. `usr/src/Makefile.master`, next to the sparc/i386 rows (arm64-gate's aarch64 rows at
   `grep -n '^aarch64_' usr/src/Makefile.master` on the `arm64` remote are the template):

        m68k_BUILD64=        $(POUND_SIGN)
        m68k_BUILD32=
        m68k_MACH64=         m68k
        m68k_MACH32=         m68k
        m68k_XARCH=
        m68k_CFLAGS=         $(m68k_XARCH) -_gcc=-m68040 -_gcc=-malign-int
        m68k_COPTFLAG=       -O
        m68k_CCFLAGS=        $(m68k_XARCH) -_gcc=-m68040 -_gcc=-malign-int $(CCERRWARN)
        m68k_CCOPTFLAG=      -O
        m68k_STAND_FLAGS=    -_gcc=-ffreestanding
        m68k_GNUC_ROOT=      $(CROSS)
        m68k_PRIMARY_CC=     gcc10,$(m68k_GNUC_ROOT)/bin/m68k-unknown-solaris2.11-gcc,gnu
        m68k_PRIMARY_CCC=    gcc10,$(m68k_GNUC_ROOT)/bin/m68k-unknown-solaris2.11-g++,gnu
        m68k_HAVE_CLOSED_BINS= $(POUND_SIGN)
        m68k_C_PICFLAGS=     -fpic
        m68k_C_BIGPICFLAGS=  -fPIC
        m68k_CC_PICFLAGS=    -_gcc=-fpic
        m68k_NATIVE_MACH64=  m68k
        m68k_ELFWRAP_TARGET= -ztarget=m68k
        m68k_LD_TARGET=      $(MACH)
        m68k_GNU_ROOT=       $(CROSS)
        m68k_PERL_MACH=      m68k
        CTF_FLAGS_m68k=      $(CCGDEBUG) $(CSTD)

   and turn the `MACH64_1`/`MACH32_1` substitutions (lines 320-324 at the fork point) into the
   table form `MACH64= $($(MACH)_MACH64)` that arm64-gate uses.
2. `usr/src/uts/Makefile`: `m68k_ARCHITECTURES = amiga m68k` and `m68k_PARALLEL = $(m68k_ARCHITECTURES)`
   beside the sparc/i386 lines (38-57), and add `$(m68k_ARCHITECTURES)` to `XRDIRS`.
3. `usr/src/uts/common/sys/isa_defs.h`: before the final `#error "ISA not supported"` (line 488):

        #elif defined(__mc68000__)
        #define _BIG_ENDIAN
        #define _ILP32
        #define _LONG_LONG_HTOL
        #define _BIT_FIELDS_HTOL
        #define _STACK_GROWS_DOWNWARD
        #define _CHAR_IS_SIGNED
        #define _CHAR_ALIGNMENT 1
        #define _SHORT_ALIGNMENT 2
        #define _INT_ALIGNMENT 4
        #define _LONG_ALIGNMENT 4
        #define _LONG_LONG_ALIGNMENT 4
        #define _DOUBLE_ALIGNMENT 4
        #define _DOUBLE_COMPLEX_ALIGNMENT 4
        #define _LONG_DOUBLE_ALIGNMENT 4
        #define _LONG_DOUBLE_COMPLEX_ALIGNMENT 4
        #define _POINTER_ALIGNMENT 4
        #define _MAX_ALIGNMENT 4
        #define _ALIGNMENT_REQUIRED 1
        #define _LONG_LONG_ALIGNMENT_32 _LONG_LONG_ALIGNMENT
        #define _PSM_MODULES
        #define _DONT_USE_1275_GENERIC_NAMES

   `-malign-int` is what makes `_INT_ALIGNMENT 4` true; the plain m68k ABI aligns int to 2. The
   i386 block's `_SUNOS_VTOC_16`, `_FIRMWARE_NEEDS_FDISK`, `_DMA_USES_PHYSADDR`, `_RTC_CONFIG`
   are decided when cmlb and the Gayle driver are ported (Amiga disks use RDB, not fdisk);
   `_HAVE_CPUID_INSN` and `_MULTI_DATAMODEL` stay out.
4. New directories, copied from the intel/i86pc files named and trimmed to headers only:
   - `usr/src/uts/m68k/Makefile`, `Makefile.m68k` (from `uts/intel/Makefile.intel`; set
     `DEF_BUILDS = $(DEF_BUILDS32)`, `ALL_BUILDS = $(ALL_BUILDS32)`, `CFLAGS_XARCH_32 =`,
     `ASFLAGS_XARCH_32 =`), `Makefile.files` (empty module lists), `Makefile.rules`,
     `sys/Makefile` with the first headers (`isa_defs` lives in common; start with `machtypes.h`,
     `asm_linkage.h`, `machlock.h`, `regset.h`, `frame.h`, `stack.h`, `trap.h`, `psw.h`,
     `pcb.h`, `ucontext.h`, `fp.h`, `elf_M68K.h` in `common/sys`).
   - `usr/src/uts/amiga/Makefile`, `Makefile.amiga` (from `uts/i86pc/Makefile.i86pc`,
     `PLATFORM = amiga`), `Makefile.files`, `Makefile.rules`, `sys/Makefile`.
   Everything else (`ml/`, `os/`, `vm/`, `unix/`) comes with M1.

Then:

    cd ~/amillumos/amillumos-build/host
    make sgs
    ls build/sysroot/usr/include/sys/isa_defs.h build/cross/opt/onbld/bin/i386/cw

## Step 7: the cross toolchain

    cd ~/amillumos/amillumos-build/host
    make download                      # src/gcc (il-10_4_0), src/binutils-gdb (2.43.1 + illumos host fixes)
    make binutils                      # build/cross/bin/m68k-unknown-elf-{as,ld,objdump,gdb}
    make stage0-gcc                    # freestanding m68k-unknown-elf GCC 10.4, C only, ~20 min
    make check-align                   # the -malign-int ICE: present in 10.4 or not?

`check-align` answers next-step 2 of PLAN.md. If GCC 10.4 also crashes, the bug is ours to
reduce and report (gcc-bugs, and a patch in `host/patches/`); if not, Homebrew's GCC 16 is the
odd one and the plan stands.

7b. The real target. Write `host/patches/gcc-m68k-solaris2.patch` against `il-10_4_0` with:

- `gcc/config.gcc`: a `m68k-*-solaris2*` case near the `m68k-*-elf*` one (line 2334), with
  `tm_file="${tm_file} dbxelf.h elfos.h m68k/m68k-none.h m68k/m68kelf.h sol2.h m68k/sol2.h"`,
  `tmake_file="m68k/t-m68kelf t-sol2 m68k/t-sol2"`, `tm_defines` setting the default CPU to
  68040, `use_gcc_stdint=wrap`, and `extra_options="${extra_options} sol2.opt"`.
- `gcc/config/m68k/sol2.h` modelled on `richlowe/gcc:gcc/config/aarch64/sol2.h` after
  `af5b6a6c6` and `8177a00c0`: `TARGET_OS_CPP_BUILTINS`, link spec for illumos `ld` (no GNU ld),
  startfile/endfile with the Solaris CRTs, `-mcpu` defaults, TLS via `__m68k_read_tp`.
- `gcc/config/m68k/t-sol2`, `libgcc/config.host` (`m68k-*-solaris2*` with
  `tmake_file="$tmake_file t-crtstuff-pic t-libgcc-pic t-eh-dw2-dip t-slibgcc t-slibgcc-sld ..."`),
  `libgcc/config/m68k/t-sol2`.

Then `make boot-gcc` (needs the sysroot headers from step 6; links through
`build/cross/opt/onbld/bin/amd64/ld`, the illumos linker from the tools proto, which still lacks
the m68k backend at this point, so `boot-gcc`'s own link tests will fail until `sgs/libld`
learns `EM_68K`; the compiler and `libgcc.a` install regardless, and the kernel is linked by the
gate's `ld` once that exists).

## Step 8: first cross invocation of the kernel tree

    cd ~/amillumos/amillumos-build/host/amillumos-gate
    ./usr/src/tools/scripts/bldenv -T m68k ../env/m68k 'cd usr/src/uts/amiga && make'

It will stop at the first missing file. That list, worked through in order, is milestone M1.

## Order of commits on `amiga`, summarised

| # | Commit | Step |
|---|---|---|
| 1 | build: make the 2021 gate's tools build on a 2026 host | 4 |
| 2 | build: cross-build plumbing from arm64-gate (-T, NATIVE_MACH, CROSS_BLD) | 5 |
| 3 | m68k: Makefile.master rows | 6.1 |
| 4 | m68k: uts/Makefile architectures | 6.2 |
| 5 | m68k: isa_defs.h | 6.3 |
| 6 | m68k, amiga: uts header-only skeletons | 6.4 |
| 7 | (toolchain, in amillumos-build): gcc-m68k-solaris2.patch | 7b |
