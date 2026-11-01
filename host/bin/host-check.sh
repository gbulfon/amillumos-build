#!/bin/ksh
#
# Preflight for the Amillumos build host (illumos x86). Prints what the
# host/Makefile and host/env/m68k assume, so they can be adjusted before the
# first build. Read-only.
#
print "== system"
uname -a
head -1 /etc/release 2>/dev/null
print "zone: $(zonename 2>/dev/null)"
print
print "== compilers (need GCC 7 or 10 for the 2021 gate; 10 for building GCC 10.4)"
for g in /usr/gcc/*/bin/gcc /opt/gcc-*/bin/gcc; do
	[[ -x $g ]] && print "  $g: $($g -dumpfullversion 2>/dev/null || $g -dumpversion)"
done
print
print "== gmp/mpfr/mpc headers (GMPINCDIR for host/Makefile is the directory holding gmp.h)"
find /usr/include /opt/ooce/include -maxdepth 2 \( -name gmp.h -o -name mpfr.h -o -name mpc.h \) 2>/dev/null | sed 's/^/  /'
print
print "== interpreters and tools"
for t in git gmake dmake make rsync gtar python3 perl; do
	p=$(whence $t); print "  $t: ${p:-MISSING}"
done
ls /usr/bin/python3.* 2>/dev/null | sed 's/^/  /'
print "  perl: $(perl -MConfig -e 'print $Config{version}' 2>/dev/null)"
print
print "== packages (either distro; absent names just print nothing)"
pkg list -H build-essential developer/illumos-tools developer/build/onbld \
    developer/gcc-7 developer/gcc-10 developer/gcc7 developer/gcc10 \
    library/gmp library/mpfr library/mpc text/texinfo ooce/text/texinfo \
    developer/build/gnu-make developer/versioning/git 2>/dev/null | sed 's/^/  /'
print
print "== disk (gate clone ~2 GB, tools+headers ~3 GB, two GCC builds ~6 GB; want 25 GB free)"
df -h "$HOME" | tail -1 | sed 's/^/  /'
print "  zfs root: $(zfs list -H -o name / 2>/dev/null)"
print
print "== onbld already on the host? (not required; the gate builds its own)"
ls -d /opt/onbld 2>/dev/null | sed 's/^/  /'
