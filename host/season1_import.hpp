#pragma once

#include "profile.hpp"

namespace tricklogic2 {

// Builds Season 1's cases for Season 2 from the player's two discs, whatever
// edition (the Japanese release, a translation) each one is.
//
// The Japanese releases of both seasons have the same three fonts, so when
// the two discs' fonts match, Season 1's case archives are copied unchanged
// to where Season 2 looks for downloaded episodes,
// ms0:/PSP/GAME/NPJG00025/<case>.edat. Otherwise one of them is an edition
// that redrew its fonts, and:
//
//  - Season 2's three fonts (in data/common.bin) gain every glyph of Season
//    1's, at codes Season 2 leaves empty: the i-th Season 1 code becomes the
//    i-th double-byte code with a lead byte of 0xEB-0xFC. Season 1's kana and
//    kanji are moved, not redrawn, so their ink lines up as Season 2's does.
//    The result is a disc patch of common.bin: the fonts appended, three index
//    records changed.
//  - Each case archive's scripts have their text codes moved to match, and
//    the archive is written where Season 2 looks for downloaded episodes,
//    ms0:/PSP/GAME/NPJG00025/<case>.edat.
//
// Throws install::InstallError when either disc is not laid out as expected.
void build_season1(const portablekit::AddOnBuild &build);

} // namespace tricklogic2
