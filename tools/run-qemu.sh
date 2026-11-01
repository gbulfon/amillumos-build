#!/bin/bash
# Run a kernel ELF on QEMU's m68k virt machine and assert a marker on the console.
# Usage: tools/run-qemu.sh KERNEL.elf MARKER [TIMEOUT_SECONDS]
set -u
KERNEL=${1:?kernel}; MARKER=${2:?marker}; LIMIT=${3:-30}
HERE=$(cd "$(dirname "$0")/.." && pwd)
LOG=$HERE/build/last-qemu.log
qemu-system-m68k -M virt -cpu m68040 -m ${QEMU_MEM:-1G} -nographic -no-reboot -monitor none \
    -kernel "$KERNEL" -append "${QEMU_APPEND:-amillumos test}" >"$LOG" 2>&1 </dev/null &
QPID=$!
for ((i = 0; i < LIMIT * 10; i++)); do
    kill -0 $QPID 2>/dev/null || break
    sleep 0.1
done
if kill -0 $QPID 2>/dev/null; then kill $QPID; wait $QPID 2>/dev/null; echo "(qemu killed after ${LIMIT}s)" >>"$LOG"; fi
if grep -a -q -- "$MARKER" "$LOG"; then
    echo "PASS: '$MARKER' seen on goldfish-tty"
    exit 0
fi
echo "FAIL: '$MARKER' not seen; log $LOG:" >&2; tail -20 "$LOG" >&2
exit 1
