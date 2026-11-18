#!/usr/bin/env bash
# Recompile the game's executable into the AOT corpus.
#
#   scripts/generate.sh [build_dir]
#
# Needs an executable prepared by `TrickLogic2Native --install <image.iso>`,
# which writes EBOOT.ELF into the per-user data directory, or a game directory
# named by TXL2_GAME_DIR. Output: generated/, which is never committed.
#
# Generating takes minutes; compiling what it produces takes much longer. Read
# PortableKit's docs/BRINGING_UP_A_GAME.md first: the interpreter runs the
# game without any of this.
set -euo pipefail

repo_dir="$(cd "$(dirname "$0")/.." && pwd)"
build_dir="${1:-$repo_dir/out}"

elf="${TXL2_EBOOT:-}"
if [[ -z "$elf" ]]; then
    for candidate in \
        "${TXL2_GAME_DIR:-}/EBOOT.ELF" \
        "${TXL2_DATA_DIR:-}/EBOOT.ELF" \
        "$HOME/Library/Application Support/TrickLogic2/UCJS10105/EBOOT.ELF" \
        "$HOME/.local/share/TrickLogic2/UCJS10105/EBOOT.ELF"; do
        if [[ -f "$candidate" ]]; then elf="$candidate"; break; fi
    done
fi
if [[ ! -f "$elf" ]]; then
    echo "error: no prepared executable found." >&2
    echo "Run '$build_dir/bin/TrickLogic2Native --install <image.iso>' first," >&2
    echo "or set TXL2_EBOOT to the decrypted EBOOT.ELF." >&2
    exit 1
fi

cmake --build "$build_dir" --target psp_analyze psp_recomp
mkdir -p "$repo_dir/analysis" "$repo_dir/generated"
"$build_dir/portablekit/psp_analyze" "$elf" "$repo_dir/analysis/report.json"
# Regenerated in place: psp_recomp rewrites only units whose text changed and
# removes units that no longer exist, so unchanged units keep their timestamps.
#
# The game's C++ static initializers sit inside what the section table calls
# .data: 19 functions at 0x089167F4-0x08918AAC (end exclusive), with their
# entry table at 0x08918AAC-0x08918AF8. The startup code at 0x088EAD3C walks
# that table (loop at 0x088EAC60) before main; the functions build global
# objects and register their destructors through 0x088EAC3C. The Japanese
# release's executable has them in the same place: the translation changed
# only data. The analyser only takes .text as code, so the range is named here. Without it
# the corpus stops at 0x089167F4 under PSPRECOMP_NO_INTERPRETER=1.
"$build_dir/portablekit/psp_recomp" "$elf" --auto "$repo_dir/generated" \
    --code 0x089167F4-0x08918AAC

echo
echo "What the recompiler could not lower:"
grep -rho 'unsupported(0x[0-9A-Fa-f]*u, 0x[0-9A-Fa-f]*u, "[^"]*"' "$repo_dir/generated"/*.cpp \
    | sed 's/.*"\(.*\)"/\1/' | sort | uniq -c | sort -rn || true
