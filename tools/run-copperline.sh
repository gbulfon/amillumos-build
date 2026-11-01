#!/bin/bash
# Run an Amiga executable headless under Copperline and assert a marker on the serial output.
# Usage: tools/run-copperline.sh PROG MARKER [EMULATED_SECONDS]
# Exit 0 when MARKER appears on the Paula serial output, 1 otherwise. Log kept in build/last-run.log.
# RETURNS=1 : the program returns to AmigaDOS; use --exit-on-return (Copperline >= 1.1) instead
#             of the emulated-time deadline. Kernels never return, so the default is the deadline.
set -u
PROG=${1:?program}; MARKER=${2:?marker}; SECS=${3:-15}
HERE=$(cd "$(dirname "$0")/.." && pwd)
COPPERLINE=${COPPERLINE:-/Applications/Copperline.app/Contents/MacOS/copperline}
CONFIG=${CONFIG:-$HERE/emu/copperline/a4000.toml}
LOG=$HERE/build/last-run.log
TMPPNG=$(mktemp -t amillumos-end).png
if [ "${RETURNS:-0}" = 1 ]; then
    "$COPPERLINE" --config "$CONFIG" --noaudio --run "$PROG" --exit-on-return --screenshot-after "$SECS" "$TMPPNG" >"$LOG" 2>&1
else
    "$COPPERLINE" --config "$CONFIG" --noaudio --run "$PROG" --screenshot-after "$SECS" "$TMPPNG" >"$LOG" 2>&1
fi
rc=$?
rm -f "$TMPPNG"
if grep -a -q -- "$MARKER" "$LOG"; then
    echo "PASS: '$MARKER' seen on serial (copperline rc=$rc)"
    exit 0
fi
echo "FAIL: '$MARKER' not seen (copperline rc=$rc); last lines of $LOG:" >&2
grep -a -v '^\[' "$LOG" | tail -20 >&2
exit 1
