# Third-party notices

TrickLogic2 is distributed under the MIT License (`TrickLogic2-LICENSE.txt`). It is built on PortableKit, also under the MIT License (`PortableKit-LICENSE.txt`, <https://github.com/TeamGDB/PortableKit>). Its released builds also contain the third-party software listed here, each under its own license. The license texts are in the same directory as this file: `TrickLogic2.app/Contents/Resources/licenses/` on macOS, `licenses/` beside the program on Linux.

TrickLogic2 ships none of the game's files: no disc image, art, text, sound, movie or save. You provide the disc images of games you own; what is prepared from them (the game's executable, Season 1's cases) stays on your computer and is for your own use only.

## Compiled into the program

### Dear ImGui 1.92.9b

The in-game menu and the setup screens. Copyright (c) 2014-2026 Omar Cornut. MIT License: `DearImGui-LICENSE.txt`. <https://github.com/ocornut/imgui>

### tiny-AES-c

AES for preparing the executable from the disc image and for save data. Commit `23856752fbd139da0b8ca6e471a13d5bcc99a08d`, released into the public domain under the Unlicense: `tiny-AES-c-UNLICENSE.txt`. <https://github.com/kokke/tiny-AES-c>

### stb_truetype 1.26

Draws the game's text. Copyright (c) 2017 Sean Barrett. Used under the public-domain dedication it offers as an alternative to the MIT License:

> This is free and unencumbered software released into the public domain. Anyone is free to copy, modify, publish, use, compile, sell, or distribute this software, either in source code form or as a compiled binary, for any purpose, commercial or non-commercial, and by any means.

<https://github.com/nothings/stb>

### stb_image 2.30 and stb_image_write 1.16

Read and write the PNG images of HD texture packs. Copyright (c) 2017 Sean Barrett. Used under the same public-domain dedication as stb_truetype above. <https://github.com/nothings/stb>

### xxHash 0.8.3

Hashes textures for HD texture packs. Copyright (c) 2012-2021 Yann Collet. BSD 2-Clause License: `xxHash-LICENSE.txt`. <https://github.com/Cyan4973/xxHash>

### libchdr

Reads disc images in CHD format. Copyright Romain Tisserand. BSD 3-Clause License: `libchdr-LICENSE.txt`. <https://github.com/rtissera/libchdr>

It brings the decoders a CHD may use, whose notices are in `libchdr-NOTICES.txt`: miniz 3.1.2 (MIT), the Zstandard 1.5.7 decompressor (BSD), LZMA SDK 26.02 (public domain) and dr_flac (public domain or MIT-0).

## Shipped as separate libraries

These are dynamically linked shared libraries: in `TrickLogic2.app/Contents/Frameworks/` on macOS, in `lib/` beside the program on Linux. They are unmodified builds of the upstream releases below. You may replace them with your own builds of the same or a compatible version.

### SDL3 3.4.16

Window, input and audio output. Copyright (C) 1997-2026 Sam Lantinga. zlib License: `SDL3-LICENSE.txt`.

- Library: `libSDL3.0.dylib` on macOS, `libSDL3.so.0` on Linux, built from the source below
- Source: <https://github.com/libsdl-org/SDL/releases/download/release-3.4.16/SDL3-3.4.16.tar.gz> (SHA-256 `7322236cd12090c3eb40b9728be4d49c76f66ad17d04369584d4ecad5cf77c68`)

### FFmpeg 7.1.5

Decodes the game's ATRAC3 and ATRAC3plus audio and their H.264 movies. FFmpeg is licensed under the GNU Lesser General Public License, version 2.1 or later: `FFmpeg-COPYING.LGPLv2.1.txt`, with the source and configuration also noted in `FFmpeg-SOURCE.txt`. This build contains only FFmpeg's LGPL parts and nothing non-free. <https://ffmpeg.org/>

- Libraries: `libavcodec.61.dylib` and `libavutil.59.dylib` on macOS, `libavcodec.so.61` and `libavutil.so.59` on Linux
- Source: <https://ffmpeg.org/releases/ffmpeg-7.1.5.tar.xz> (SHA-256 `de668509caf9e35e3cd162473441fdb29538c6d96ed080292b3cf9e6fc5d558f`), not modified
- Configured with:

  ```text
  ./configure --prefix=<prefix> --enable-shared --disable-static --disable-programs --disable-doc --disable-avdevice --disable-avformat --disable-avfilter --disable-swscale --disable-swresample --disable-network --disable-autodetect --disable-everything --enable-decoder=atrac3,atrac3p,h264 --disable-x86asm --disable-debug
  ```

- On macOS also with `--install-name-dir=@rpath`, which only sets where the libraries expect each other.
- Built by the build itself, `cmake/FFmpeg.cmake` in PortableKit (<https://github.com/TeamGDB/PortableKit>), which pins the version, checksum and configuration above.

**Source offer.** The exact FFmpeg source archive above is published on the same release page as every TrickLogic2 build that contains it. For at least three years after we distribute a build, we will also provide that source to anyone who asks through the project's issue tracker, <https://github.com/Hintay/TrickLogic2Native/issues>, at no more than the cost of providing it.

### Vulkan loader 1.4.357 (macOS only)

Finds and loads the Vulkan driver, MoltenVK. Copyright (c) 2015-2026 The Khronos Group Inc., Valve Corporation and LunarG, Inc. Apache License 2.0: `Vulkan-Loader-LICENSE.txt`. Built for macOS 13 and Apple Silicon, with its Vulkan-Headers of the same tag.

- Library: `libvulkan.1.dylib`
- Source: <https://github.com/KhronosGroup/Vulkan-Loader/archive/refs/tags/vulkan-sdk-1.4.357.0.tar.gz> (SHA-256 `54f2537df22313768da0317dda2abdaaab7711b4081c48c869a79db343d0ae70`)

### MoltenVK 1.4.2 (macOS only)

Vulkan on Apple's Metal. Copyright (c) 2015-2026 The Brenwill Workshop Ltd. Apache License 2.0: `MoltenVK-LICENSE.txt`. The Khronos release build; only its Apple Silicon (arm64) part is kept.

- Library: `libMoltenVK.dylib`, with its driver manifest in `TrickLogic2.app/Contents/Resources/vulkan/icd.d/`
- Release: <https://github.com/KhronosGroup/MoltenVK/releases/download/v1.4.2/MoltenVK-macos.tar> (SHA-256 `f95765a6229cb7b915990a2890ce12ebe36a730b021545d3d52ae69ce4c4024e`); source: <https://github.com/KhronosGroup/MoltenVK/tree/v1.4.2>

## Font

### Noto Sans CJK JP (Noto CJK Sans 2.004)

A fallback for text on systems without a CJK font: `TrickLogic2.app/Contents/Resources/fonts/NotoSansCJKjp-Regular.otf` on macOS, `fonts/NotoSansCJKjp-Regular.otf` on Linux. Copyright 2014-2021 Adobe (<http://www.adobe.com/>), with Reserved Font Name 'Source'. SIL Open Font License, version 1.1: `NotoSansCJK-OFL.txt`. <https://github.com/notofonts/noto-cjk>
