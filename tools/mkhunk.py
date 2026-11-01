#!/usr/bin/env python3
"""Wrap a flat, position-independent m68k binary into a minimal AmigaDOS hunk executable.

Usage: mkhunk.py [--bss BYTES] input.bin output.exe

Produces HUNK_HEADER / HUNK_CODE / HUNK_END with no relocations, so the input must be
position independent (PC-relative references only, or absolute references to fixed
addresses such as ExecBase at 4 or the custom chip registers).
"""
import struct
import sys

HUNK_HEADER = 0x3F3
HUNK_CODE = 0x3E9
HUNK_END = 0x3F2


def main(argv):
    bss = 0
    if len(argv) >= 3 and argv[1] == "--bss":
        bss = int(argv[2], 0)
        argv = argv[:1] + argv[3:]
    if len(argv) != 3:
        sys.exit(__doc__)
    with open(argv[1], "rb") as f:
        code = f.read()
    while len(code) % 4:
        code += b"\0"
    longs = len(code) // 4
    # the header's size entry covers code plus bss so LoadSeg allocates room for both
    out = struct.pack(">IIIIII", HUNK_HEADER, 0, 1, 0, 0, longs + (bss + 3) // 4)
    out += struct.pack(">II", HUNK_CODE, longs) + code
    out += struct.pack(">I", HUNK_END)
    with open(argv[2], "wb") as f:
        f.write(out)
    print(f"{argv[2]}: {len(code)} bytes of code, {len(out)} bytes total")


if __name__ == "__main__":
    main(sys.argv)
