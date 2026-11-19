# Building

The README has the basic build. PortableKit's [BUILDING.md](../portablekit/docs/BUILDING.md)
covers the rest in general: what you need installed, where the time goes and how to
work on the code without a full build each time. This page is what is particular to
this port.

## The game's code

`scripts/generate.sh` generates the code from the executable `--install` prepared;
`TXL2_EBOOT` names another one. Either Season 2 edition works: their code is the same
byte for byte, and the code generated from them differs only in the hash of the
executable it records.

## The Steam Deck

The Linux build can be cross-compiled on macOS with [zig](https://ziglang.org/),
against the Deck's own SDL3 and Vulkan loader. `<deck>` is anything `ssh` takes for the
Deck, `<sysroot>` a folder for its libraries and `<build>` the build folder:

```bash
portablekit/tools/make_linux_sysroot.sh <deck> <sysroot>
cmake -S . -B <build> -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=portablekit/cmake/toolchains/zig-linux-x86_64.cmake \
      -DPORTABLEKIT_LINUX_SYSROOT=<sysroot>/usr \
      -DPORTABLEKIT_FFMPEG=bundled
cmake --build <build> -j2
```

It uses the same `generated/` as the native build. Copy `<build>/bin` (the executable
and `lib/`) to the Deck, then set the game up there:

```bash
./TrickLogic2Native --install /path/to/season2.iso --in-place
./TrickLogic2Native --add-disc /path/to/season1.iso
```

## Season 1 and the disc overlay

`--add-disc` writes Season 1's cases where Season 2 looks for downloaded episodes,
`<data>/ms0/PSP/GAME/NPJG00025/<case>.edat`. The Japanese releases of the two seasons
share their fonts, so with both of them the cases are copied as they are. When either
disc is a translation that redrew its fonts, the cases' text is moved to new codes and
a patch of Season 2's `common.bin` adds Season 1's glyphs at those codes
(`host/season1_import.hpp` has the details).

The patch goes to PortableKit's disc overlay rather than a rebuilt disc image:
`<data>/disc_overlay/PSP_GAME/USRDIR/data/common.bin.pkpatch` changes byte ranges of
the disc's file as the game reads it. It records the size and SHA-256 of the
`common.bin` it was made from, and a patch that does not match the disc in use is left
out with a warning; adding Season 1 again after changing the Season 2 image makes a new
one. `TXL2_NO_DISC_OVERLAY=1` turns the overlay off.

## Testing

Bound every run with `timeout`, and give a test its own data folder so it never
touches your saves:

```bash
TXL2_DATA_DIR=<test folder> timeout 300 out/bin/TrickLogic2Native --install season2.chd
PSPRECOMP_NO_INTERPRETER=1 TXL2_DATA_DIR=<test folder> timeout 120 out/bin/TrickLogic2Native
```

`PSPRECOMP_NO_INTERPRETER=1` checks that only recompiled code runs.
`TXL2_SCREENSHOT_DIR` captures the game's frames and `TXL2_INPUT_SCRIPT` drives the
controls; PortableKit's [AGENTS.md](../portablekit/AGENTS.md) lists the rest.
