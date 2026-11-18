// Season 1's cases for Season 2, built at setup from the player's own discs.
// See season1_import.hpp for what is built; this file is how.
//
// Formats, as read off both games' data:
//
//  SECTPACK archive (data/*.bin): byte 15 is the sector the data starts at.
//    The index before it holds records "./<name>\0" followed by u16 id
//    (bit 15: the entry is LZ-compressed), u16 first sector (counted from the
//    data start) and u16 sector count.
//  LZ: u32 size, u32 compressed size, then groups of a flag byte (most
//    significant bit first; 1 = a literal byte) and eight items. A reference
//    is two bytes: (distance >> 8) << 4 | (length - 3), distance & 0xFF;
//    distance 1-4096 (4096 stored as 0), length 3-18.
//  Font (SIR0): "SIR0", u32 root, u32 pointer list. Glyph records sorted by
//    code, each u16 code, a 14-byte header and a 4-bit bitmap, padded to four
//    bytes with 0xAA. The root is u32 count, u32 0, the records' offsets. The
//    pointer list is the deltas between the file's pointer slots (4, 8 and
//    each offset in the root) as 7-bit groups, high first, 0x80 on all but
//    the last, ending in 0.
//    Header bytes after the code: 2 width, 3 height, 4 x offset (signed),
//    5 y offset, 7 y offset + height (horizontal text); 10 x offset (signed)
//    and 11 y offset (vertical text).
//  Script (PRCS): "PRCS", 12 bytes, then instructions u8 opcode, u32 length,
//    payload. Text is NUL-terminated Shift-JIS inside payloads.

#include "season1_import.hpp"

#include "install/installer.hpp"
#include "kernel/disc_patch.hpp"
#include "kernel/iso_image.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace tricklogic2 {
namespace {

using portablekit::IsoImage;
using portablekit::install::InstallError;
using Bytes = std::vector<std::uint8_t>;

constexpr std::uint32_t kSector = 2048u;
constexpr const char *kCommon = "PSP_GAME/USRDIR/data/common.bin";
// The cases Season 2 offers as downloads, by their archive's name on Season
// 1's disc (data/<name>.bin) and in the download folder (<name>.edat).
constexpr const char *kCases[] = {"NF", "NF_A", "FW", "FW_A", "KM", "KM_A", "SI", "SI_A"};
constexpr const char *kDownloadFolder = "PSP/GAME/NPJG00025";

struct FontEntry {
    const char *name;
    bool line_up;  // move the glyphs' ink (not the ruby font's)
};
constexpr FontEntry kFonts[] = {
    {"script/Font/NovelFontList.dat", true},
    {"script/Font/AdvFontList.dat", true},
    {"script/Font/NovelRubyFontList.dat", false},
};

// Where Season 1's codes go: leads 0xEB-0xFC, every Shift-JIS trail byte.
constexpr std::uint8_t kFirstLead = 0xEBu;
constexpr std::size_t kLeads = 0xFCu - kFirstLead + 1u;
constexpr std::size_t kTrails = 188u;  // 0x40-0x7E, 0x80-0xFC

std::uint16_t le16(const Bytes &d, std::size_t at) { return static_cast<std::uint16_t>(d[at] | (d[at + 1u] << 8u)); }
std::uint32_t le32(const Bytes &d, std::size_t at) {
    return d[at] | (d[at + 1u] << 8u) | (d[at + 2u] << 16u) | (static_cast<std::uint32_t>(d[at + 3u]) << 24u);
}
void put16(Bytes &d, std::size_t at, std::uint32_t v) {
    d[at] = static_cast<std::uint8_t>(v);
    d[at + 1u] = static_cast<std::uint8_t>(v >> 8u);
}
void put32(Bytes &d, std::size_t at, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) d[at + static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(v >> (8 * i));
}
void push32(Bytes &d, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) d.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}
std::size_t sectors(std::size_t bytes) { return (bytes + kSector - 1u) / kSector; }
void pad_to_sector(Bytes &d) { d.resize(sectors(d.size()) * kSector, 0u); }

// --- Disc files ---------------------------------------------------------------

struct DiscFile {
    std::uint64_t start{};  // image offset
    std::uint32_t size{};
};

DiscFile find_file(IsoImage &iso, const std::string &path, const char *disc) {
    const auto entry = iso.find(path);
    if (!entry || entry->directory)
        throw InstallError(std::string(disc) + " has no " + path + ": it is not an edition this build knows.");
    return {static_cast<std::uint64_t>(entry->lba) * kSector, entry->size};
}

Bytes read_range(IsoImage &iso, std::uint64_t at, std::size_t size) {
    Bytes out(size);
    if (iso.read(at, out) != size) throw InstallError("Reading the disc image failed.");
    return out;
}

// --- SECTPACK -------------------------------------------------------------------

struct PackEntry {
    std::string name;
    std::uint16_t id{};
    std::uint64_t offset{};  // in the archive
    std::uint64_t size{};    // whole sectors
    std::size_t record{};    // where its u16 id is in the index
};

std::uint32_t pack_data_start(const Bytes &header) { return header.size() > 15u ? header[15] : 0u; }

std::vector<PackEntry> pack_entries(const Bytes &index, std::uint32_t data_start) {
    std::vector<PackEntry> entries;
    std::map<std::string, std::size_t> by_name;
    const auto printable = [](std::uint8_t c) { return c >= 0x20u && c <= 0x7Eu; };
    std::size_t i = 0;
    while (i + 2u < index.size()) {
        if (index[i] == '.' && index[i + 1u] == '/') {
            std::size_t j = i + 2u;
            while (j < index.size() && printable(index[j])) ++j;
            if (j > i + 2u && j < index.size() && index[j] == 0u && j + 7u <= index.size()) {
                PackEntry entry;
                entry.name.assign(index.begin() + static_cast<std::ptrdiff_t>(i + 2u),
                                  index.begin() + static_cast<std::ptrdiff_t>(j));
                entry.record = j + 1u;
                entry.id = le16(index, entry.record);
                entry.offset = (static_cast<std::uint64_t>(data_start) + le16(index, entry.record + 2u)) * kSector;
                entry.size = static_cast<std::uint64_t>(le16(index, entry.record + 4u)) * kSector;
                if (const auto found = by_name.find(entry.name); found != by_name.end())
                    entries[found->second] = entry;
                else {
                    by_name.emplace(entry.name, entries.size());
                    entries.push_back(entry);
                }
                i = j + 1u;
                continue;
            }
        }
        ++i;
    }
    return entries;
}

const PackEntry &pack_entry(const std::vector<PackEntry> &entries, const std::string &name, const std::string &where) {
    for (const PackEntry &entry : entries)
        if (entry.name == name) return entry;
    throw InstallError(where + " has no " + name + ": it is not an edition this build knows.");
}

// --- LZ -------------------------------------------------------------------------

Bytes lz_decompress(const Bytes &d) {
    if (d.size() < 8u) throw InstallError("A compressed file on the disc is cut short.");
    const std::uint32_t size = le32(d, 0u);
    const std::size_t end = std::min<std::size_t>(d.size(), 8u + le32(d, 4u));
    Bytes out;
    out.reserve(size);
    std::size_t i = 8u;
    while (i < end && out.size() < size) {
        const std::uint8_t flags = d[i++];
        for (int k = 0; k < 8; ++k) {
            if (i >= end || out.size() >= size) break;
            if ((flags >> (7 - k)) & 1u) {
                out.push_back(d[i++]);
            } else {
                if (i + 1u >= end) {
                    i = end;
                    break;
                }
                const std::uint8_t b0 = d[i];
                const std::uint8_t b1 = d[i + 1u];
                i += 2u;
                std::size_t distance = ((b0 & 0xF0u) << 4u) | b1;
                if (distance == 0u) distance = 4096u;
                const std::size_t length = (b0 & 0xFu) + 3u;
                for (std::size_t n = 0; n < length; ++n)
                    out.push_back(out.size() >= distance ? out[out.size() - distance] : 0u);
            }
        }
    }
    return out;
}

// Greedy, longest match among the last 64 places the next three bytes were
// seen, nearest first.
Bytes lz_compress(const Bytes &data) {
    const std::size_t n = data.size();
    Bytes out;
    push32(out, static_cast<std::uint32_t>(n));
    push32(out, 0u);
    std::unordered_map<std::uint32_t, std::int64_t> head;
    std::vector<std::int64_t> previous(n, -1);
    const auto key = [&data](std::size_t p) {
        return static_cast<std::uint32_t>(data[p]) << 16u | static_cast<std::uint32_t>(data[p + 1u]) << 8u | data[p + 2u];
    };
    const auto add = [&](std::size_t p) {
        if (p + 3u > n) return;
        auto [it, inserted] = head.try_emplace(key(p), static_cast<std::int64_t>(p));
        if (!inserted) {
            previous[p] = it->second;
            it->second = static_cast<std::int64_t>(p);
        }
    };
    std::size_t i = 0;
    while (i < n) {
        const std::size_t flag_at = out.size();
        out.push_back(0u);
        std::uint8_t flags = 0u;
        for (int k = 0; k < 8; ++k) {
            if (i >= n) break;
            std::size_t best_length = 0u;
            std::size_t best_distance = 0u;
            if (i + 3u <= n) {
                const auto found = head.find(key(i));
                std::int64_t p = found == head.end() ? -1 : found->second;
                for (int tries = 0; p >= 0 && tries < 64; ++tries, p = previous[static_cast<std::size_t>(p)]) {
                    const std::size_t distance = i - static_cast<std::size_t>(p);
                    if (distance > 4096u) break;
                    std::size_t length = 3u;
                    while (length < 18u && i + length < n && data[static_cast<std::size_t>(p) + length] == data[i + length])
                        ++length;
                    if (length > best_length) {
                        best_length = length;
                        best_distance = distance;
                    }
                    if (length == 18u) break;
                }
            }
            if (best_length >= 3u) {
                out.push_back(static_cast<std::uint8_t>(((best_distance >> 8u) & 0xFu) << 4u | (best_length - 3u)));
                out.push_back(static_cast<std::uint8_t>(best_distance & 0xFFu));
                for (std::size_t p = i; p < i + best_length; ++p) add(p);
                i += best_length;
            } else {
                flags = static_cast<std::uint8_t>(flags | (1u << (7 - k)));
                out.push_back(data[i]);
                add(i);
                ++i;
            }
        }
        out[flag_at] = flags;
    }
    put32(out, 4u, static_cast<std::uint32_t>(out.size() - 8u));
    return out;
}

Bytes entry_data(const Bytes &raw, std::uint16_t id) { return (id & 0x8000u) != 0u ? lz_decompress(raw) : raw; }

// --- Fonts ----------------------------------------------------------------------

struct Font {
    Bytes prefix;  // the bytes before the first record
    // Each record from its code to the next record (padding included), in
    // the order of the root's offsets.
    std::vector<std::pair<std::uint16_t, Bytes>> records;
};

Font read_font(const Bytes &d, const std::string &what) {
    if (d.size() < 16u || std::memcmp(d.data(), "SIR0", 4u) != 0) throw InstallError(what + " is not a font this build knows.");
    const std::uint32_t root = le32(d, 4u);
    if (root + 8u > d.size()) throw InstallError(what + " is cut short.");
    const std::uint32_t count = le32(d, root);
    if (root + 8u + std::size_t{count} * 4u > d.size() || count == 0u) throw InstallError(what + " is cut short.");
    std::vector<std::uint32_t> offsets(count);
    for (std::uint32_t i = 0; i < count; ++i) offsets[i] = le32(d, root + 8u + i * 4u);
    std::vector<std::uint32_t> ends(offsets);
    ends.push_back(root);
    std::sort(ends.begin(), ends.end());
    ends.erase(std::unique(ends.begin(), ends.end()), ends.end());
    Font font;
    const std::uint32_t first = *std::min_element(offsets.begin(), offsets.end());
    if (first < 2u || first > root) throw InstallError(what + " is not a font this build knows.");
    font.prefix.assign(d.begin(), d.begin() + first);
    for (const std::uint32_t at : offsets) {
        const auto next = std::upper_bound(ends.begin(), ends.end(), at);
        if (next == ends.end() || *next > d.size() || *next < at + 16u) throw InstallError(what + " is not a font this build knows.");
        font.records.emplace_back(le16(d, at), Bytes(d.begin() + at, d.begin() + *next));
    }
    return font;
}

Bytes varint(std::uint32_t v) {
    Bytes groups{static_cast<std::uint8_t>(v & 0x7Fu)};
    for (v >>= 7u; v != 0u; v >>= 7u) groups.push_back(static_cast<std::uint8_t>(0x80u | (v & 0x7Fu)));
    return {groups.rbegin(), groups.rend()};
}

Bytes write_font(const Bytes &prefix, const std::map<std::uint16_t, Bytes> &records) {
    Bytes body(prefix);
    std::vector<std::uint32_t> offsets;
    for (const auto &[code, record] : records) {
        offsets.push_back(static_cast<std::uint32_t>(body.size()));
        const std::size_t at = body.size();
        body.insert(body.end(), record.begin(), record.end());
        put16(body, at, code);
        while (body.size() % 4u != 0u) body.push_back(0xAAu);
    }
    while (body.size() % 16u != 0u) body.push_back(0u);
    const auto root = static_cast<std::uint32_t>(body.size());
    push32(body, static_cast<std::uint32_t>(offsets.size()));
    push32(body, 0u);
    for (const std::uint32_t offset : offsets) push32(body, offset);
    while (body.size() % 16u != 0u) body.push_back(0xAAu);
    const auto list = static_cast<std::uint32_t>(body.size());
    std::uint32_t previous = 0u;
    const auto slot = [&](std::uint32_t at) {
        const Bytes delta = varint(at - previous);
        body.insert(body.end(), delta.begin(), delta.end());
        previous = at;
    };
    slot(4u);
    slot(8u);
    for (std::size_t i = 0; i < offsets.size(); ++i) slot(root + 8u + static_cast<std::uint32_t>(i) * 4u);
    body.push_back(0u);
    while (body.size() % 16u != 0u) body.push_back(0xAAu);
    std::memcpy(body.data(), "SIR0", 4u);
    put32(body, 4u, root);
    put32(body, 8u, list);
    return body;
}

// --- Lining up Season 1's glyphs ------------------------------------------------
//
// Season 1's bitmaps carry 0-3 blank or faint rows and columns around the ink
// that their offsets do not make up for, so a line of them wobbles. Each kana
// and kanji is moved so its ink sits where most of the font's does: its ink's
// bottom on one line across, its ink's centre on one column down. A row or
// column counts as ink by its total (at least 25 of 15 per pixel), so the
// faint anti-aliasing fringe some glyphs carry does not move the edge.
// Bitmaps are not touched.

struct Ink {
    double column{};  // centre
    double row{};     // centre
    int last_row{};
};

std::optional<Ink> ink(const Bytes &record) {
    // The header starts after the code.
    const std::size_t header = 2u;
    const int width = record[header + 2u];
    const int height = record[header + 3u];
    if (width == 0 || height == 0) return std::nullopt;
    const std::size_t stride = static_cast<std::size_t>(width + 1) / 2u;
    std::vector<int> columns(static_cast<std::size_t>(width), 0);
    std::vector<int> rows(static_cast<std::size_t>(height), 0);
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const std::size_t at = header + 14u + static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x) / 2u;
            const std::uint8_t byte = at < record.size() ? record[at] : 0u;
            const int value = (x & 1) != 0 ? byte >> 4u : byte & 0xFu;
            columns[static_cast<std::size_t>(x)] += value;
            rows[static_cast<std::size_t>(y)] += value;
        }
    int first_column = -1, last_column = -1, first_row = -1, last_row = -1;
    for (int x = 0; x < width; ++x)
        if (columns[static_cast<std::size_t>(x)] >= 25) {
            if (first_column < 0) first_column = x;
            last_column = x;
        }
    for (int y = 0; y < height; ++y)
        if (rows[static_cast<std::size_t>(y)] >= 25) {
            if (first_row < 0) first_row = y;
            last_row = y;
        }
    if (first_column < 0 || first_row < 0) return std::nullopt;
    return Ink{(first_column + last_column) / 2.0, (first_row + last_row) / 2.0, last_row};
}

int s8(std::uint8_t v) { return v > 127u ? v - 256 : v; }
bool is_kanji(std::uint16_t code) { return code >= 0x889Fu; }
bool is_fullwidth(std::uint16_t code) { return (code >= 0x829Fu && code <= 0x8396u) || code >= 0x889Fu; }

// Counts values, remembering the order they were first seen in, which
// settles ties.
template <typename T>
class Tally {
public:
    void add(T value) {
        for (auto &[seen, count] : counts_)
            if (seen == value) {
                ++count;
                return;
            }
        counts_.emplace_back(value, 1);
    }
    // The n most common, ties in first-seen order.
    [[nodiscard]] std::vector<T> most_common(std::size_t n) const {
        auto sorted = counts_;
        std::stable_sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
        std::vector<T> out;
        for (std::size_t i = 0; i < sorted.size() && i < n; ++i) out.push_back(sorted[i].first);
        return out;
    }

private:
    std::vector<std::pair<T, int>> counts_;
};

double mean(const std::vector<double> &values) {
    if (values.empty()) throw InstallError("A Season 1 font has no kanji.");
    double sum = 0.0;
    for (const double v : values) sum += v;
    return sum / static_cast<double>(values.size());
}

void line_up(Font &font) {
    std::vector<std::optional<Ink>> inks;
    inks.reserve(font.records.size());
    for (const auto &[code, record] : font.records) inks.push_back(ink(record));
    Tally<double> hx, vx, vy;
    Tally<int> bottoms;
    for (std::size_t i = 0; i < font.records.size(); ++i) {
        const auto &[code, record] = font.records[i];
        if (!is_kanji(code) || !inks[i]) continue;
        hx.add(s8(record[6]) + inks[i]->column);
        vx.add(s8(record[12]) + inks[i]->column);
        vy.add(s8(record[13]) + inks[i]->row);
        bottoms.add(record[7] + inks[i]->last_row);
    }
    // The mean of the two most common half-pixel positions, so rounding
    // splits evenly.
    const double across_x = mean(hx.most_common(2));
    const double down_x = mean(vx.most_common(2));
    const double down_y = mean(vy.most_common(2));
    const auto bottom = bottoms.most_common(1);
    if (bottom.empty()) throw InstallError("A Season 1 font has no kanji.");
    const auto byte = [](long v) {
        if (v < 0 || v > 255) throw InstallError("A Season 1 glyph does not fit Season 2's layout.");
        return static_cast<std::uint8_t>(v);
    };
    for (std::size_t i = 0; i < font.records.size(); ++i) {
        auto &[code, record] = font.records[i];
        if (!is_fullwidth(code) || !inks[i]) continue;
        const Ink &k = *inks[i];
        // Across: one centre column, one bottom line.
        record[6] = static_cast<std::uint8_t>(static_cast<long>(std::floor(across_x - k.column + 0.5)) & 0xFF);
        record[7] = byte(std::max(0, bottom.front() - k.last_row));
        record[9] = byte(record[7] + record[5]);
        // Down: one centre.
        record[12] = static_cast<std::uint8_t>(static_cast<long>(std::floor(down_x - k.column + 0.5)) & 0xFF);
        record[13] = byte(std::max(0L, static_cast<long>(std::floor(down_y - k.row + 0.5))));
    }
}

// --- Scripts --------------------------------------------------------------------

bool is_lead(std::uint8_t b) { return (b >= 0x81u && b <= 0x9Fu) || (b >= 0xE0u && b <= 0xFCu); }
bool is_trail(std::uint8_t b) { return b >= 0x40u && b <= 0xFCu && b != 0x7Fu; }
bool is_text(std::uint8_t b) { return b == 0x0Au || (b >= 0x20u && b <= 0x7Eu) || (b >= 0xA1u && b <= 0xDFu); }

// Moves a script's text codes. False: the script is not laid out as expected
// and is left as it is.
bool remap_script(Bytes &d, const std::unordered_map<std::uint16_t, std::uint16_t> &index, std::size_t &unknown) {
    std::vector<std::pair<std::size_t, std::size_t>> payloads;
    std::size_t p = 16u;
    while (p + 5u <= d.size()) {
        const std::size_t length = le32(d, p + 1u);
        if (p + 5u + length > d.size()) break;
        payloads.emplace_back(p + 5u, length);
        p += 5u + length;
    }
    if (p != d.size()) return false;
    for (const auto &[start, length] : payloads) {
        const std::size_t end = start + length;
        // NUL-terminated runs holding at least one double-byte code and
        // nothing but text, found before any is changed.
        std::vector<std::size_t> pairs;
        for (std::size_t i = start; i < end;) {
            std::size_t j = i;
            std::vector<std::size_t> run;
            bool text = true;
            while (j < end && d[j] != 0u) {
                if (is_lead(d[j]) && j + 1u < end && is_trail(d[j + 1u])) {
                    run.push_back(j);
                    j += 2u;
                } else {
                    text = text && is_text(d[j]);
                    ++j;
                }
            }
            if (text && !run.empty() && j < end && (i == start || d[i - 1u] == 0u))
                pairs.insert(pairs.end(), run.begin(), run.end());
            i = j + 1u;
        }
        for (const std::size_t at : pairs) {
            const auto found = index.find(static_cast<std::uint16_t>(d[at] << 8u | d[at + 1u]));
            if (found == index.end()) {
                ++unknown;
                continue;
            }
            d[at] = static_cast<std::uint8_t>(found->second >> 8u);
            d[at + 1u] = static_cast<std::uint8_t>(found->second & 0xFFu);
        }
    }
    return true;
}

// The archive with some entries replaced, laid out again in the order of
// their data.
Bytes repack(const Bytes &archive, const std::map<std::string, Bytes> &replace) {
    const std::uint32_t start = pack_data_start(archive);
    const Bytes index(archive.begin(), archive.begin() + std::min<std::size_t>(archive.size(), std::size_t{start} * kSector));
    std::vector<PackEntry> entries = pack_entries(index, start);
    std::stable_sort(entries.begin(), entries.end(), [](const PackEntry &a, const PackEntry &b) { return a.offset < b.offset; });
    Bytes out(index);
    std::uint32_t at = 0u;
    for (const PackEntry &entry : entries) {
        Bytes data;
        if (const auto found = replace.find(entry.name); found != replace.end()) {
            data = found->second;
            pad_to_sector(data);
        } else {
            const std::size_t from = std::min<std::size_t>(archive.size(), entry.offset);
            const std::size_t to = std::min<std::size_t>(archive.size(), entry.offset + entry.size);
            data.assign(archive.begin() + static_cast<std::ptrdiff_t>(from), archive.begin() + static_cast<std::ptrdiff_t>(to));
        }
        const std::size_t count = data.size() / kSector;
        if (at + count > 0xFFFFu) throw InstallError("A Season 1 case does not fit its archive once converted.");
        put16(out, entry.record, entry.id);
        put16(out, entry.record + 2u, at);
        put16(out, entry.record + 4u, static_cast<std::uint32_t>(count));
        out.insert(out.end(), data.begin(), data.end());
        at += static_cast<std::uint32_t>(count);
    }
    return out;
}

void write_file(const std::filesystem::path &path, const Bytes &data) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    out.close();
    if (!out) throw InstallError("Could not write " + path.string() + ".");
}

// A whole SECTPACK's index (the sectors before its data).
Bytes read_pack_index(IsoImage &iso, const DiscFile &file) {
    const Bytes first = read_range(iso, file.start, std::min<std::size_t>(kSector, file.size));
    const std::uint32_t start = pack_data_start(first);
    if (start == 0u || std::size_t{start} * kSector > file.size) throw InstallError("common.bin is not an archive this build knows.");
    return read_range(iso, file.start, std::size_t{start} * kSector);
}

Bytes read_pack_entry(IsoImage &iso, const DiscFile &file, const PackEntry &entry) {
    if (entry.offset + entry.size > file.size) throw InstallError("common.bin is cut short.");
    return entry_data(read_range(iso, file.start + entry.offset, static_cast<std::size_t>(entry.size)), entry.id);
}

} // namespace

void build_season1(const portablekit::AddOnBuild &build) {
    const char *const s1 = "The Season 1 disc";
    const char *const s2 = "The Season 2 disc";
    build.progress("Reading the fonts", 0u, 0u);

    // Both games' fonts.
    const DiscFile common2 = find_file(build.game_disc, kCommon, s2);
    const DiscFile common1 = find_file(build.add_on, kCommon, s1);
    const Bytes index2 = read_pack_index(build.game_disc, common2);
    const Bytes index1 = read_pack_index(build.add_on, common1);
    const auto entries2 = pack_entries(index2, pack_data_start(index2));
    const auto entries1 = pack_entries(index1, pack_data_start(index1));
    std::vector<Font> fonts2, fonts1;
    for (const FontEntry &font : kFonts) {
        fonts2.push_back(read_font(read_pack_entry(build.game_disc, common2, pack_entry(entries2, font.name, s2)),
                                   std::string(s2) + "'s " + font.name));
        fonts1.push_back(read_font(read_pack_entry(build.add_on, common1, pack_entry(entries1, font.name, s1)),
                                   std::string(s1) + "'s " + font.name));
    }

    // Season 1's codes, each to a code Season 2 leaves empty.
    std::vector<std::uint16_t> codes;
    for (const Font &font : fonts1)
        for (const auto &[code, record] : font.records)
            if (code >= 0x8140u) codes.push_back(code);
    std::sort(codes.begin(), codes.end());
    codes.erase(std::unique(codes.begin(), codes.end()), codes.end());
    if (codes.size() > kLeads * kTrails) throw InstallError("Season 1's fonts have more glyphs than Season 2 has room for.");
    std::unordered_map<std::uint16_t, std::uint16_t> index;
    for (std::size_t i = 0; i < codes.size(); ++i) {
        const std::size_t trail = i % kTrails;
        const auto trail_byte = static_cast<std::uint8_t>(trail < 63u ? 0x40u + trail : 0x80u + (trail - 63u));
        index.emplace(codes[i], static_cast<std::uint16_t>((kFirstLead + i / kTrails) << 8u | trail_byte));
    }

    // The fonts, merged.
    build.progress("Merging the fonts", 0u, 0u);
    std::vector<Bytes> merged;
    for (std::size_t f = 0; f < std::size(kFonts); ++f) {
        std::map<std::uint16_t, Bytes> records;
        for (const auto &[code, record] : fonts2[f].records) {
            if ((code >> 8u) >= kFirstLead)
                throw InstallError("This edition of Season 2 already uses the font codes Season 1 would take.");
            records[code] = record;
        }
        if (kFonts[f].line_up) line_up(fonts1[f]);
        for (const auto &[code, record] : fonts1[f].records)
            if (code >= 0x8140u) records[index.at(code)] = record;
        merged.push_back(write_font(fonts2[f].prefix, records));
    }

    // common.bin: the fonts appended after its data, their index records
    // pointed at them.
    const std::uint64_t data_start = std::uint64_t{pack_data_start(index2)} * kSector;
    const std::uint64_t appended_at = sectors(common2.size) * kSector;
    std::uint64_t next_sector = (appended_at - data_start) / kSector;
    portablekit::disc_patch::Patch patch;
    patch.base_size = common2.size;
    Bytes tail;
    for (std::size_t f = 0; f < std::size(kFonts); ++f) {
        const PackEntry &entry = pack_entry(entries2, kFonts[f].name, s2);
        Bytes data = (entry.id & 0x8000u) != 0u ? lz_compress(merged[f]) : merged[f];
        pad_to_sector(data);
        const std::size_t count = data.size() / kSector;
        if (next_sector + count > 0xFFFFu) throw InstallError("Season 2's common.bin has no room for the merged fonts.");
        Bytes record(6u);
        put16(record, 0u, entry.id);
        put16(record, 2u, static_cast<std::uint32_t>(next_sector));
        put16(record, 4u, static_cast<std::uint32_t>(count));
        patch.ranges.push_back({entry.record, record});
        tail.insert(tail.end(), data.begin(), data.end());
        next_sector += count;
    }
    patch.size = appended_at + tail.size();
    patch.ranges.push_back({appended_at, std::move(tail)});
    build.progress("Checking Season 2's common.bin", 0u, common2.size);
    patch.base_sha256 = portablekit::disc_patch::hash_base(
        common2.size, [&build, &common2](std::uint64_t at, std::uint8_t *out, std::size_t n) {
            const std::size_t got = build.game_disc.read(common2.start + at, std::span<std::uint8_t>(out, n));
            build.progress("Checking Season 2's common.bin", at + got, common2.size);
            return got;
        });
    portablekit::disc_patch::write(build.disc_overlay / (std::string(kCommon) + portablekit::disc_patch::kSuffix), patch);

    // The cases.
    std::size_t unknown = 0u;
    std::size_t done = 0u;
    for (const char *name : kCases) {
        build.progress("Converting the cases", done++, std::size(kCases));
        const DiscFile file = find_file(build.add_on, std::string("PSP_GAME/USRDIR/data/") + name + ".bin", s1);
        const Bytes archive = read_range(build.add_on, file.start, file.size);
        const std::uint32_t start = pack_data_start(archive);
        const Bytes header(archive.begin(), archive.begin() + std::min<std::size_t>(archive.size(), std::size_t{start} * kSector));
        std::map<std::string, Bytes> replace;
        for (const PackEntry &entry : pack_entries(header, start)) {
            if (entry.name.size() < 4u || entry.name.compare(entry.name.size() - 4u, 4u, ".bin") != 0) continue;
            if (entry.offset + entry.size > archive.size()) throw InstallError(std::string(s1) + "'s " + name + ".bin is cut short.");
            const Bytes raw(archive.begin() + static_cast<std::ptrdiff_t>(entry.offset),
                            archive.begin() + static_cast<std::ptrdiff_t>(entry.offset + entry.size));
            Bytes script = entry_data(raw, entry.id);
            if (script.size() < 4u || std::memcmp(script.data(), "PRCS", 4u) != 0) continue;
            if (!remap_script(script, index, unknown)) {
                std::cerr << "[season1] " << name << "/" << entry.name << ": not a script this build can read, left as it is\n";
                continue;
            }
            replace.emplace(entry.name, (entry.id & 0x8000u) != 0u ? lz_compress(script) : script);
        }
        write_file(build.memory_stick / kDownloadFolder / (std::string(name) + ".edat"), repack(archive, replace));
    }
    build.progress("Converting the cases", done, std::size(kCases));
    if (unknown != 0u) std::cerr << "[season1] " << unknown << " text codes have no glyph in Season 1's fonts\n";
    std::cout << "[season1] " << codes.size() << " glyphs added, " << std::size(kCases) << " cases converted\n";
}

} // namespace tricklogic2
