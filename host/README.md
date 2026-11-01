# host/ — the illumos x86 side

Everything that runs on the build host, laid out like richlowe/arm64-gate:

| Path | Role |
|---|---|
| `Makefile` | Sun-make orchestration: toolchain, sysroot, gate builds (stamps in `stamps/`) |
| `env/m68k` | bldenv/nightly environment, native (x86) and cross (`-T m68k`) |
| `bin/host-check.sh` | preflight: compilers, gmp headers, interpreters, packages, disk |
| `patches/` | toolchain patches (GCC `m68k-*-solaris2*` target config goes here) |
| `amillumos-gate/` | clone of the gate fork, branch `amiga` (gitignored) |
| `build/`, `src/`, `stamps/` | toolchain sources and outputs (gitignored) |

The step-by-step procedure is `../docs/host-setup.md`.
