# TrickLogic2

A native port of *Trick × Logic Season 2* (PSP, UCJS-10105) for macOS and Linux,
including the Steam Deck. It is built on [PortableKit](https://github.com/TeamGDB/PortableKit),
which recompiles the game's PSP code to C++ ahead of time and supplies the PSP system around it.

[简体中文](README.zh-Hans.md)

**No part of the game is in this repository.** You need your own disc image. The
port reads it, prepares the game's executable from it, and recompiles nothing at
the player's end.

## What it plays

| Disc | Edition | As |
| --- | --- | --- |
| *Trick × Logic Season 2* (UCJS-10105) | The Japanese release, or its Simplified Chinese translation | The game |
| *Trick × Logic Season 1* (UCJS-10097) | The Japanese release, or a translation of it | An add-on: its four cases join Season 2's shelf |

Disc images can be an `.iso` or a CHD made from one with `chdman createdvd`. A CHD
is read as it is, with nothing unpacked to disk.

The two Season 2 editions share one build: the translation changed only text and
art, not the game's code, so the code recompiled from one runs the other. Season 1's
cases are built at setup from the player's own Season 1 disc; a translated disc's
text keeps that disc's own font.

## Building

```bash
git clone --recursive <this repository>
cd TrickLogic2
cmake -S . -B out -G Ninja
cmake --build out -j2
```

The first build has no recompiled code and runs the game under PortableKit's
interpreter, which is slow. To recompile the game, prepare the executable from your
disc, generate the code and build again:

```bash
out/bin/TrickLogic2Native --install /path/to/season2.iso
scripts/generate.sh
cmake --build out -j2
```

The game's code is about 60 units and compiles in under ten minutes at `-j2` on an
Apple Silicon Mac. Keep `-j2`: the units are large and need memory.
[docs/BUILDING.md](docs/BUILDING.md) has the details, including cross-compiling for the
Steam Deck.

## Playing

On the first start, setup asks for the Season 2 disc image, checks it and prepares the
game. It then offers to add Season 1, which you can skip. The same can be done from a
terminal:

```bash
TrickLogic2Native --install season2.chd           # add --in-place to use the image where it is
TrickLogic2Native --add-disc season1.chd          # Season 1's cases, now or later
```

| Key | Button |
| --- | --- |
| Z, Space | ○ (confirm) |
| X, Backspace | × (cancel) |
| C | △ |
| V | □ |
| Q, Page Up / E, Page Down | L / R |
| Enter | START (skips a movie) |
| Tab | SELECT |
| Arrows, WASD | D-pad |
| Esc | The port's menu |

A gamepad works as a PSP's controls; L3+R3 opens the menu.

Game data, saves and settings are kept in `~/Library/Application Support/TrickLogic2/UCJS10105`
on macOS and `~/.local/share/TrickLogic2/UCJS10105` on Linux. `TXL2_DATA_DIR` names
another folder. PortableKit's switches all take the `TXL2_` prefix; see its
[README](https://github.com/TeamGDB/PortableKit).

## Legal

This project is not affiliated with Sony Interactive Entertainment or with the
translators of the game. *Trick × Logic* is a trademark of its owners. This repository
contains none of the game's code or data; play it with discs you own.

The port is under the [MIT licence](LICENSE). PortableKit and the components it
bundles keep their own licences.
