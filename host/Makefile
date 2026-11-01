#
# Amillumos host-side orchestration, modelled on richlowe/arm64-gate/Makefile.
# For illumos make(1S) (/usr/bin/make, dmake) on the x86 build host, not GNU make:
#
#	cd amillumos-build/host
#	make download        clone gcc (illumos/gcc il-10_4_0) and binutils-gdb
#	make binutils        m68k-unknown-elf as/ld/objdump/gdb into build/cross
#	make stage0-gcc      bare m68k-unknown-elf GCC 10.4 (C only, no headers)
#	make check-align     compile the -malign-int ICE reproducer with stage0
#	make native-tools    smoke test: build the 2021 gate's tools natively
#	make sgs             cross: bldtools + m68k headers -> build/sysroot
#	make boot-gcc        m68k-unknown-solaris2.11 GCC against the sysroot
#	make illumos         nightly -T m68k (much later)
#	make clean | clobber
#
# Written on the Mac from the arm64-gate recipes; expect to fix paths on the
# first host run. Every step leaves a stamp in stamps/ so it is not repeated.
#
$(NOSUCH) GNU_MAKE_CANNOT_PARSE_THIS +=

PWD:sh=		pwd
SRCS=		$(PWD)/src
BUILDS=		$(PWD)/build
CROSS=		$(BUILDS)/cross
SYSROOT=	$(BUILDS)/sysroot
STAMPS=		$(PWD)/stamps
GATE=		$(PWD)/amillumos-gate
ENV=		$(PWD)/env/m68k
REPRO=		$(PWD)/../docs/gcc-ice-mpcrel

MAX_JOBS=	8
# OmniOS keeps gmp.h in /usr/include/gmp, OpenIndiana in /usr/include.
# Override on the command line: make GMPINCDIR=/usr/include stage0-gcc
GMPINCDIR=	/usr/include/gmp

TRIPLE=		m68k-unknown-solaris2.11
ELFTRIPLE=	m68k-unknown-elf

GCC_REPO=	https://github.com/illumos/gcc
GCC_BRANCH=	il-10_4_0
# richlowe's branch is upstream 2.43.1 plus one commit that also carries the
# gnulib fixes needed to build gdb on an illumos host.
BINUTILS_REPO=	https://github.com/richlowe/binutils-gdb
BINUTILS_BRANCH=	illumos-arm64-2-43-1

BLDENV=		$(GATE)/usr/src/tools/scripts/bldenv
NIGHTLY=	$(GATE)/usr/src/tools/scripts/nightly

# Common GCC configure options (arm64-gate COMMON_GCC_OPTS, m68k values).
COMMON_GCC_OPTS= \
	--with-gmp-include=$(GMPINCDIR) \
	--prefix=$(CROSS) \
	--with-cpu=m68040 \
	--disable-multilib \
	--enable-c99 \
	--disable-libquadmath \
	--disable-libmudflap \
	--disable-libgomp \
	--disable-decimal-float \
	--disable-libitm \
	--disable-libsanitizer \
	--disable-libvtv \
	--disable-libcilkrts \
	--disable-nls \
	--with-system-zlib \
	--enable-__cxa-atexit \
	--with-gnu-as \
	--with-as=$(CROSS)/bin/$(ELFTRIPLE)-as

all:
	@echo "targets: download binutils stage0-gcc check-align native-tools sgs boot-gcc illumos clean clobber"

download: download-gcc download-binutils-gdb

# the m68k-*-solaris2* target configuration is applied here when it exists
# (docs/host-setup.md, step 7b)
download-gcc: FRC
	mkdir -p $(SRCS); test -d $(SRCS)/gcc || \
	git clone --depth 1 -b $(GCC_BRANCH) $(GCC_REPO) $(SRCS)/gcc
	test ! -f $(PWD)/patches/gcc-m68k-solaris2.patch || \
	(cd $(SRCS)/gcc && git apply --check $(PWD)/patches/gcc-m68k-solaris2.patch 2>/dev/null && \
	 git apply $(PWD)/patches/gcc-m68k-solaris2.patch || true)

download-binutils-gdb: FRC
	mkdir -p $(SRCS); test -d $(SRCS)/binutils-gdb || \
	git clone --depth 1 -b $(BINUTILS_BRANCH) $(BINUTILS_REPO) $(SRCS)/binutils-gdb

#
# binutils + gdb for the generic m68k ELF target. illumos' own ld links the
# product; GNU ld is only used by stage0 (freestanding tests) and gdb debugs
# the emulators' stubs. Adding m68k-*-solaris2* to bfd/gas is a later nicety.
#
binutils: $(STAMPS)/binutils-stamp
$(STAMPS)/binutils-stamp:
	mkdir -p $(STAMPS) $(BUILDS)/binutils-gdb && \
	(cd $(BUILDS)/binutils-gdb && \
	$(SRCS)/binutils-gdb/configure \
	    CFLAGS='-std=gnu17 -O2' \
	    --target=$(ELFTRIPLE) \
	    --with-gmp-include=$(GMPINCDIR) \
	    --prefix=$(CROSS) \
	    --disable-werror \
	    --disable-gprofng \
	    --disable-sim \
	    --disable-nls && \
	gmake -j $(MAX_JOBS) CPPFLAGS+='-I/usr/xpg4/include -I$(GMPINCDIR)' && \
	gmake -j $(MAX_JOBS) install) && \
	touch $@

#
# stage0: freestanding C compiler, no target headers or libc. Enough for the
# kernel and for settling the -malign-int question on GCC 10.4.
#
stage0-gcc: $(STAMPS)/stage0-gcc-stamp
$(STAMPS)/stage0-gcc-stamp: $(STAMPS)/binutils-stamp
	mkdir -p $(BUILDS)/stage0-gcc && \
	(cd $(BUILDS)/stage0-gcc && \
	$(SRCS)/gcc/configure $(COMMON_GCC_OPTS) \
	    --target=$(ELFTRIPLE) \
	    --enable-languages=c \
	    --without-headers \
	    --with-newlib \
	    --disable-shared \
	    --disable-threads \
	    --disable-libssp \
	    --disable-libatomic \
	    --disable-libstdcxx \
	    --with-gnu-ld \
	    --with-ld=$(CROSS)/bin/$(ELFTRIPLE)-ld && \
	gmake -j $(MAX_JOBS) all-gcc all-target-libgcc && \
	gmake -j $(MAX_JOBS) install-gcc install-target-libgcc) && \
	touch $@

# The Homebrew GCC 16 ICE (docs/gcc-ice-mpcrel): does GCC 10.4 have it?
check-align: $(STAMPS)/stage0-gcc-stamp
	$(CROSS)/bin/$(ELFTRIPLE)-gcc --version | head -1
	$(CROSS)/bin/$(ELFTRIPLE)-gcc -O2 -m68040 -malign-int -mpcrel -fno-pic \
	    -ffreestanding -fno-builtin -nostdlib -Wall -c -o $(BUILDS)/align-test.o \
	    $(REPRO)/main.c && echo "-malign-int: OK on GCC 10.4"
	$(CROSS)/bin/$(ELFTRIPLE)-gcc -O2 -m68040 -malign-int -ffreestanding -c \
	    -o $(BUILDS)/align-test2.o $(REPRO)/main.c && echo "-malign-int (no -mpcrel): OK"

#
# Smoke test of the fork point on this host: tools and x86 headers, natively,
# without any Amillumos change to the gate. ~15 min. Output in
# amillumos-gate/usr/src/tools/proto/root_i386-nd and amillumos-gate/proto/root_i386.
#
native-tools: $(STAMPS)/native-tools-stamp
$(STAMPS)/native-tools-stamp:
	mkdir -p $(STAMPS) && \
	(cd $(GATE) && \
	 $(BLDENV) $(ENV) 'cd usr/src && make -j $(MAX_JOBS) bldtools sgs') && \
	touch $@

#
# Cross: native tools + m68k headers into the sysroot. Needs the cross-build
# plumbing (step 5) and the first m68k gate commits (step 6).
#
sysroot: $(STAMPS)/sysroot-stamp
$(STAMPS)/sysroot-stamp:
	mkdir -p $(STAMPS) $(SYSROOT)/usr/include $(SYSROOT)/usr/lib $(SYSROOT)/lib && \
	touch $@

sgs: $(STAMPS)/sgs-stamp
$(STAMPS)/sgs-stamp: $(STAMPS)/sysroot-stamp
	(cd $(GATE) && \
	 $(BLDENV) -T m68k $(ENV) 'cd usr/src && make -j $(MAX_JOBS) bldtools sgs' && \
	 rsync -a usr/src/tools/proto/root_i386-nd/ $(CROSS)/ && \
	 rsync -a proto/root_m68k/usr/include/ $(SYSROOT)/usr/include/) && \
	touch $@

#
# boot-gcc: the real target triple, headers from the sysroot, no shared libgcc,
# no libstdc++ (there is no libc yet). Links with illumos ld from the tools proto.
#
boot-gcc: $(STAMPS)/boot-gcc-stamp
$(STAMPS)/boot-gcc-stamp: $(STAMPS)/sgs-stamp $(STAMPS)/binutils-stamp
	mkdir -p $(BUILDS)/boot-gcc && \
	(cd $(BUILDS)/boot-gcc && \
	$(SRCS)/gcc/configure $(COMMON_GCC_OPTS) \
	    --target=$(TRIPLE) \
	    --enable-languages=c,c++ \
	    --with-build-sysroot=$(SYSROOT) \
	    --with-headers=$(SYSROOT)/usr/include \
	    --disable-shared \
	    --disable-libstdcxx \
	    --disable-libatomic \
	    --without-gnu-ld \
	    --with-ld=$(CROSS)/opt/onbld/bin/amd64/ld && \
	gmake -j $(MAX_JOBS) && \
	gmake -j $(MAX_JOBS) install && \
	rm -fr $(CROSS)/lib/gcc/$(TRIPLE)/10.4.0/include-fixed) && \
	touch $@

illumos: $(STAMPS)/boot-gcc-stamp
	(cd $(GATE) && $(NIGHTLY) -T m68k $(ENV))

clean:
	rm -rf $(BUILDS) $(STAMPS)

clobber: clean
	rm -rf $(SRCS)

FRC:
