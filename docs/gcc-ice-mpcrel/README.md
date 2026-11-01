GCC 16.2.0 (Homebrew m68k-elf-gcc) internal compiler error with -mpcrel:
`internal compiler error: in emit_move_insn, at expr.cc:4637` when calling a function
through a pointer held in a `const struct` reached via a global pointer (`plat->putc(c)`).
Flags: -m68040 -malign-int -mpcrel -O2 -ffreestanding. Reproducer: main.i (preprocessed).
Worked around in the kernel by using plain per-platform functions instead of a struct of
function pointers. To report at https://gcc.gnu.org/bugzilla/ (target m68k).
