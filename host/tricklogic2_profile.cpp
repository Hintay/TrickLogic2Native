// Everything PortableKit has to be told about Trick x Logic Season 2
// (UCJS-10105) with the Simplified Chinese fan translation applied. The
// framework reads the rest out of the game's own executable.
//
// Read off the player's disc image, not recalled: the disc id and title from
// PSP_GAME/PARAM.SFO, the hash from PSP_GAME/SYSDIR/EBOOT.BIN, and the load
// image's size from the ELF's program header.
//
// The translation replaces EBOOT.BIN with the plain (already decrypted) ELF,
// and BOOT.BIN on that disc is all zeroes, so there is nothing to decrypt: the
// installer accepts a plain EBOOT.BIN whose SHA-256 is executable_sha256 as
// it is.
//
// The Japanese release is an edition of the same disc id (kEditions). Its
// EBOOT.BIN is encrypted, and decrypts to an executable laid out exactly like
// the translation's, with the same code: the translation changed 5558 bytes,
// all in .data. A corpus generated from it is the translation's corpus apart
// from the hash it records, so the one corpus runs both.

#include "profile.hpp"
#include "season1_import.hpp"

#include "psprecomp/elf32.hpp"

namespace portablekit {
namespace {

// The game lists its saves as gameName "NPJG00025" (the download release's
// id, which the disc release reuses), saveName "SAVE??", file SN7SAVE.BIN
// (first boot, "[savedata] LIST"). The episode files it probes for on the
// memory stick also live under ms0:/PSP/GAME/NPJG00025/.
constexpr SaveFolder kSaveFolders[] = {
    {"NPJG00025", "Game data", true},
};

// Season 2 plays Season 1's cases as downloaded episodes. They are built at
// setup from the player's Season 1 disc (season1_import.hpp), in whatever
// edition it is; nothing of either game is shipped.
constexpr const char *kSeason1DiscIds[] = {"UCJS10097"};
constexpr AddOnDisc kAddOnDiscs[] = {
    {"season1", "Trick x Logic Season 1",
     "Season 1's four cases join Season 2's shelf, built from your Season 1 disc in its own font.", kSeason1DiscIds,
     tricklogic2::build_season1},
};

// The Japanese release, as Sony pressed it. Its EBOOT.BIN's "~PSP" tag is
// 0xD9160BF0, which selects decryption_key below (the published key table's
// entry for that tag). BOOT.BIN on this disc is all zeroes too.
constexpr ProfileVariant kEditions[] = {
    {.key = "ja",
     .name = "the Japanese release",
     .encrypted_executable_sha256 = "fd7678eff182802c7af76a9c9f23125931403c499f4911de665542495fcd80e7",
     .executable_sha256 = "1e617f9320a7813db85cdbe38c80cc9095dffe6b33bd629de506bc489ba0c025",
     .same_code_as_profile = true,
     // Japanese save dialogs, like the console it was made for.
     .dialog_language = 0u},
};

constexpr ProfileText kText[] = {
    {"zh-Hans", "Season 1's four cases join Season 2's shelf, built from your Season 1 disc in its own font.",
     "从你的一代光盘生成一代的四个案件，加入二代的书架，文字使用一代自己的字体。"},
    {"zh-Hans", "Reading the fonts", "正在读取字库"},
    {"zh-Hans", "Merging the fonts", "正在合并字库"},
    {"zh-Hans", "Checking Season 2's common.bin", "正在校验二代的 common.bin"},
    {"zh-Hans", "Converting the cases", "正在转换案件"},
};

// The keyboard for a text adventure: one hand on the bottom row, as in most
// PC visual novels and PSP emulators. Z or Space confirms (circle, the
// Japanese confirm), X or Backspace cancels (cross), C and V are the other
// two face buttons, Q and E (or Page Up / Page Down) the shoulders. Enter
// stays START, which skips a movie. The D-pad is on the arrows and on WASD;
// the stick is left unbound, since the menus read the D-pad. Esc stays the
// port's own menu, and the mouse is left free.
void keyboard_defaults(input::Bindings &b, bool &mouse) {
    using input::Action;
    const auto set = [&](Action action, const char *first, const char *second = nullptr) {
        b[static_cast<std::size_t>(action)] = {input::from_name(first),
                                               second != nullptr ? input::from_name(second) : input::kNone};
    };
    for (input::Slots &slots : b) slots = {input::kNone, input::kNone};
    set(Action::Circle, "Z", "Space");
    set(Action::Cross, "X", "Backspace");
    set(Action::Triangle, "C");
    set(Action::Square, "V");
    set(Action::L, "Q", "PageUp");
    set(Action::R, "E", "PageDown");
    set(Action::Start, "Enter");
    set(Action::Select, "Tab");
    set(Action::Up, "Up", "W");
    set(Action::Down, "Down", "S");
    set(Action::Left, "Left", "A");
    set(Action::Right, "Right", "D");
    mouse = false;
}

} // namespace

const GameProfile &game() {
    static const GameProfile profile{
        .app_name = "TrickLogic2Native",
        .project_name = "TrickLogic2",
        .env_prefix = "TXL2",
        .data_organization = "TrickLogic2",
        .data_application = "UCJS10105",

        .disc_id = "UCJS10105",
        .disc_id_display = "UCJS-10105",
        .game_title = "Trick x Logic Season 2",
        .executable_path_on_disc = "PSP_GAME/SYSDIR/EBOOT.BIN",
        .param_sfo_path_on_disc = "PSP_GAME/PARAM.SFO",
        .release_name = "the Japanese release or its Simplified Chinese translation",
        // The translated disc carries the plain ELF as EBOOT.BIN, so the
        // profile's own executable has no encrypted form. Its SHA-256 is what
        // the corpus is built from.
        .encrypted_executable_sha256 = "",
        .executable_sha256 = "c25cd25356b80d17413d83045860e94174f165818d6a4b753010e6c6ffc14e0b",
        // The key the Japanese release's EBOOT.BIN needs (kEditions).
        .decryption_tag = 0xD9160BF0u,
        .decryption_key = {0x83, 0x83, 0xF1, 0x37, 0x53, 0xD0, 0xBE, 0xFC, 0x8D, 0xA7, 0x32, 0x52, 0x46, 0x0A, 0xC2,
                           0xC2},
        .variants = kEditions,

        // A relocatable executable (ELF type 0xFFA0) with one segment of
        // 0x144A580 bytes in memory (0x116190 in the file), so it fits the
        // 32 MiB of a PSP-1000 at the usual user base.
        .load_base = psprecomp::kDefaultPspUserLoadBase,
        .guest_ram_bytes = 32u * 1024u * 1024u,
        .boot_path = "disc0:/PSP_GAME/SYSDIR/EBOOT.BIN",
        // The section table has no overlay slots. What it loads at run time
        // is the system libraries in PSP_GAME/USRDIR/module (libfont,
        // libpsmfplayer, psmf), which the framework implements as HLE.
        .overlay_slots = {},

        .add_on_discs = kAddOnDiscs,
        .text = kText,

        .save_game_name = "NPJG00025",
        .save_folders = kSaveFolders,

        // A guess: the disc id. The game imports sceNetAdhoc*, but nothing
        // about its ad hoc play is known yet.
        .adhoc_product_code = "UCJS10105",

        .keyboard_defaults = keyboard_defaults,

        // A Japanese console (the defaults): Japanese, and circle confirms.
        .system_language = 0u,
        .confirm_button = 0u,
        // The translation's language for the save list; the game itself
        // still sees the Japanese console it was made for. The Japanese
        // release keeps Japanese (kEditions).
        .dialog_language = 11u,
    };
    return profile;
}

} // namespace portablekit
