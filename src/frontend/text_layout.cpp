#include <sprawn/frontend/text_layout.h>
#include "font_chain.h"
#include "glyph_atlas.h"

#include <hb.h>
#include <unicode/ubidi.h>
#include <unicode/ustring.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

namespace sprawn {

namespace {

// Convert a byte offset in a UTF-8 string to a codepoint (column) index.
size_t byte_offset_to_col(std::string_view utf8, int byte_off) {
    size_t col = 0;
    size_t i = 0;
    size_t target = static_cast<size_t>(byte_off);
    if (target > utf8.size()) target = utf8.size();
    while (i < target && i < utf8.size()) {
        uint8_t b = static_cast<uint8_t>(utf8[i]);
        if      (b < 0x80) i += 1;
        else if (b < 0xE0) i += 2;
        else if (b < 0xF0) i += 3;
        else               i += 4;
        ++col;
    }
    return col;
}

// Convert a codepoint (column) index to a byte offset in a UTF-8 string.
size_t col_to_byte_offset(std::string_view utf8, size_t col) {
    size_t i = 0;
    for (size_t n = 0; n < col && i < utf8.size(); ++n) {
        uint8_t b = static_cast<uint8_t>(utf8[i]);
        if      (b < 0x80) i += 1;
        else if (b < 0xE0) i += 2;
        else if (b < 0xF0) i += 3;
        else               i += 4;
    }
    return i;
}

// Check if the UTF-8 string might contain RTL or complex scripts.
// Returns true if any byte >= 0xD6 (start of Hebrew block U+0590 = 0xD6 0x90).
bool might_need_bidi(std::string_view utf8) {
    for (size_t i = 0; i < utf8.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(utf8[i]);
        // Two-byte sequences for U+0590+ start with 0xD6 or higher lead byte.
        // Three/four-byte sequences always have lead >= 0xE0.
        // We check for >= 0xD6 as a fast heuristic.
        if (b >= 0xD6 && b != 0xFF)
            return true;
    }
    return false;
}

// Shape a single directional run with HarfBuzz and append results to out.
// If max_width_px > 0, stop shaping when x_accum exceeds it. Returns true if truncated.
bool shape_run(FontChain& fonts, GlyphAtlas& atlas,
               std::string_view utf8,     // full line
               int run_start,             // byte offset of run in utf8
               int run_length,            // byte length
               hb_direction_t direction,
               int& x_accum,
               std::vector<GlyphEntry>& out,
               int max_width_px = 0)
{
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, utf8.data(), static_cast<int>(utf8.size()),
                       run_start, run_length);
    hb_buffer_set_direction(buf, direction);
    hb_buffer_guess_segment_properties(buf);

    hb_shape(fonts.primary().hb_font(), buf, nullptr, 0);

    unsigned glyph_count = 0;
    hb_glyph_info_t*     infos = hb_buffer_get_glyph_infos(buf, &glyph_count);
    hb_glyph_position_t* pos   = hb_buffer_get_glyph_positions(buf, &glyph_count);

    // First pass: collect glyphs, detect .notdef (glyph_id == 0) for fallback
    for (unsigned i = 0; i < glyph_count; ++i) {
        uint32_t gid = infos[i].codepoint;
        uint8_t  font_idx = 0;

        if (gid == 0) {
            // Try fallback fonts for this cluster's codepoint
            int cluster_byte = static_cast<int>(infos[i].cluster);
            // Decode the codepoint at this cluster position
            const char* p = utf8.data() + cluster_byte;
            const char* end = utf8.data() + utf8.size();
            uint32_t cp = 0;
            if (p < end) {
                uint8_t b = static_cast<uint8_t>(*p);
                if (b < 0x80) {
                    cp = b;
                } else if (b < 0xE0) {
                    cp = b & 0x1F;
                    if (p + 1 < end) cp = (cp << 6) | (static_cast<uint8_t>(p[1]) & 0x3F);
                } else if (b < 0xF0) {
                    cp = b & 0x0F;
                    if (p + 1 < end) cp = (cp << 6) | (static_cast<uint8_t>(p[1]) & 0x3F);
                    if (p + 2 < end) cp = (cp << 6) | (static_cast<uint8_t>(p[2]) & 0x3F);
                } else {
                    cp = b & 0x07;
                    if (p + 1 < end) cp = (cp << 6) | (static_cast<uint8_t>(p[1]) & 0x3F);
                    if (p + 2 < end) cp = (cp << 6) | (static_cast<uint8_t>(p[2]) & 0x3F);
                    if (p + 3 < end) cp = (cp << 6) | (static_cast<uint8_t>(p[3]) & 0x3F);
                }
            }

            auto [fi, fallback_gid] = fonts.resolve(cp);
            if (fallback_gid != 0) {
                // Re-shape this single cluster with the fallback font
                hb_buffer_t* fb_buf = hb_buffer_create();
                hb_buffer_add_utf8(fb_buf, utf8.data(), static_cast<int>(utf8.size()),
                                   cluster_byte,
                                   // length of this cluster: distance to next cluster or run end
                                   (i + 1 < glyph_count)
                                       ? static_cast<int>(infos[i + 1].cluster) - cluster_byte
                                       : run_start + run_length - cluster_byte);
                hb_buffer_set_direction(fb_buf, direction);
                hb_buffer_guess_segment_properties(fb_buf);
                hb_shape(fonts.font(fi).hb_font(), fb_buf, nullptr, 0);

                unsigned fb_count = 0;
                hb_glyph_info_t*     fb_infos = hb_buffer_get_glyph_infos(fb_buf, &fb_count);
                hb_glyph_position_t* fb_pos   = hb_buffer_get_glyph_positions(fb_buf, &fb_count);

                double scale = fonts.font(fi).bitmap_scale();
                for (unsigned j = 0; j < fb_count; ++j) {
                    GlyphEntry ge;
                    ge.glyph_id   = fb_infos[j].codepoint;
                    ge.font_index = fi;
                    ge.x          = x_accum + static_cast<int>((fb_pos[j].x_offset >> 6) * scale);
                    ge.y_offset   = static_cast<int>((fb_pos[j].y_offset >> 6) * scale);
                    ge.cluster    = static_cast<int>(fb_infos[j].cluster);
                    out.push_back(ge);
                    x_accum += static_cast<int>((fb_pos[j].x_advance >> 6) * scale);
                }

                hb_buffer_destroy(fb_buf);
                continue;
            }
        }

        GlyphEntry ge;
        ge.glyph_id   = gid;
        ge.font_index = font_idx;
        ge.x          = x_accum + (pos[i].x_offset >> 6);
        ge.y_offset   = pos[i].y_offset >> 6;
        ge.cluster    = static_cast<int>(infos[i].cluster);
        out.push_back(ge);
        x_accum += pos[i].x_advance >> 6;

        if (max_width_px > 0 && x_accum > max_width_px) {
            hb_buffer_destroy(buf);
            return true;
        }
    }

    hb_buffer_destroy(buf);
    return false;
}

} // namespace

TextLayout::TextLayout(GlyphAtlas& atlas, FontChain& fonts, float dpi_scale)
    : atlas_(atlas), fonts_(fonts), dpi_scale_(dpi_scale),
      line_height_(static_cast<int>(fonts.line_height() / dpi_scale + 0.5f)),
      ascent_(static_cast<int>(fonts.ascent() / dpi_scale + 0.5f))
{}

void TextLayout::reset(float dpi_scale) {
    dpi_scale_ = dpi_scale;
    line_height_ = static_cast<int>(fonts_.line_height() / dpi_scale_ + 0.5f);
    ascent_ = static_cast<int>(fonts_.ascent() / dpi_scale_ + 0.5f);
}

GlyphRun TextLayout::shape_line(std::string_view utf8, int max_width_px) {
    GlyphRun run;
    run.total_width = 0;

    if (utf8.empty()) return run;

    // Strip trailing \r\n for shaping
    while (!utf8.empty() && (utf8.back() == '\r' || utf8.back() == '\n'))
        utf8.remove_suffix(1);

    if (utf8.empty()) return run;

    // Convert logical limit to physical pixels for comparison with x_accum
    int phys_limit = (max_width_px > 0) ? static_cast<int>(max_width_px * dpi_scale_) : 0;

    int x_accum = 0;

    if (!might_need_bidi(utf8)) {
        // Fast path: pure LTR, single HarfBuzz run
        run.truncated = shape_run(fonts_, atlas_, utf8, 0, static_cast<int>(utf8.size()),
                  HB_DIRECTION_LTR, x_accum, run.glyphs, phys_limit);
    } else {
        // ICU BiDi path
        // Convert UTF-8 to UTF-16 for ICU
        int32_t u16_len = 0;
        UErrorCode err = U_ZERO_ERROR;
        u_strFromUTF8(nullptr, 0, &u16_len,
                      utf8.data(), static_cast<int32_t>(utf8.size()), &err);
        err = U_ZERO_ERROR; // reset buffer overflow error

        std::vector<UChar> u16(u16_len + 1);
        u_strFromUTF8(u16.data(), u16_len + 1, &u16_len,
                      utf8.data(), static_cast<int32_t>(utf8.size()), &err);

        if (U_FAILURE(err)) {
            // Fallback to LTR on conversion failure
            shape_run(fonts_, atlas_, utf8, 0, static_cast<int>(utf8.size()),
                      HB_DIRECTION_LTR, x_accum, run.glyphs);
            run.total_width = x_accum;
            return run;
        }

        UBiDi* bidi = ubidi_open();
        ubidi_setPara(bidi, u16.data(), u16_len, UBIDI_DEFAULT_LTR, nullptr, &err);

        if (U_FAILURE(err)) {
            ubidi_close(bidi);
            shape_run(fonts_, atlas_, utf8, 0, static_cast<int>(utf8.size()),
                      HB_DIRECTION_LTR, x_accum, run.glyphs);
            run.total_width = x_accum;
            return run;
        }

        int32_t run_count = ubidi_countRuns(bidi, &err);

        // Build a UTF-16 offset → UTF-8 byte offset map
        // We need this to convert BiDi run positions (UTF-16) to byte offsets
        std::vector<int32_t> u16_to_u8(u16_len + 1);
        {
            int32_t u8i = 0, u16i = 0;
            const char* p = utf8.data();
            const char* end = utf8.data() + utf8.size();
            while (p < end && u16i <= u16_len) {
                u16_to_u8[u16i] = u8i;
                uint8_t b = static_cast<uint8_t>(*p);
                int u8_seq_len;
                int u16_seq_len;
                if (b < 0x80)        { u8_seq_len = 1; u16_seq_len = 1; }
                else if (b < 0xE0)   { u8_seq_len = 2; u16_seq_len = 1; }
                else if (b < 0xF0)   { u8_seq_len = 3; u16_seq_len = 1; }
                else                 { u8_seq_len = 4; u16_seq_len = 2; } // surrogate pair
                // Fill intermediate UTF-16 indices (for surrogate pairs)
                for (int k = 1; k < u16_seq_len && u16i + k <= u16_len; ++k)
                    u16_to_u8[u16i + k] = u8i;
                p += u8_seq_len;
                u8i += u8_seq_len;
                u16i += u16_seq_len;
            }
            // Sentinel at end
            if (u16i <= u16_len)
                u16_to_u8[u16i] = u8i;
        }

        for (int32_t i = 0; i < run_count; ++i) {
            int32_t logical_start = 0, length = 0;
            UBiDiDirection dir = ubidi_getVisualRun(bidi, i, &logical_start, &length);

            // Convert UTF-16 positions to UTF-8 byte offsets
            int32_t u8_start = u16_to_u8[logical_start];
            int32_t u8_end   = (logical_start + length <= u16_len)
                                   ? u16_to_u8[logical_start + length]
                                   : static_cast<int32_t>(utf8.size());

            hb_direction_t hb_dir = (dir == UBIDI_RTL) ? HB_DIRECTION_RTL
                                                        : HB_DIRECTION_LTR;

            shape_run(fonts_, atlas_, utf8, u8_start, u8_end - u8_start,
                      hb_dir, x_accum, run.glyphs);
        }

        ubidi_close(bidi);
    }

    run.total_width = x_accum;
    return run;
}

void TextLayout::draw_run(Renderer& r, const GlyphRun& run, int x, int y,
                          Color tint)
{
    int baseline_y = y + ascent_;
    float inv = 1.0f / dpi_scale_;

    for (const GlyphEntry& ge : run.glyphs) {
        const AtlasGlyph* ag = atlas_.get_or_add(ge.glyph_id, ge.font_index);
        if (!ag || ag->rect.w == 0 || ag->rect.h == 0)
            continue;

        SDL_Rect src = ag->rect;
        SDL_Rect dst;
        dst.x = x + static_cast<int>(ge.x * inv) + static_cast<int>(ag->bearing_x * inv);
        dst.y = baseline_y - static_cast<int>(ag->bearing_y * inv) - static_cast<int>(ge.y_offset * inv);
        dst.w = static_cast<int>(ag->rect.w * inv);
        dst.h = static_cast<int>(ag->rect.h * inv);

        r.blit(atlas_.texture(), src, dst, tint);
    }
}

int TextLayout::x_for_column(const GlyphRun& run, std::string_view utf8,
                             size_t col) const
{
    if (col == 0 || run.glyphs.empty()) return 0;

    float inv = 1.0f / dpi_scale_;

    // Convert column (codepoint index) to byte offset
    size_t target_byte = col_to_byte_offset(utf8, col);

    // Find the first glyph at or past this byte offset
    for (size_t i = 0; i < run.glyphs.size(); ++i) {
        if (static_cast<size_t>(run.glyphs[i].cluster) >= target_byte) {
            return static_cast<int>(run.glyphs[i].x * inv);
        }
    }

    // Past the end: use total_width
    return static_cast<int>(run.total_width * inv);
}

size_t TextLayout::column_for_x(const GlyphRun& run, std::string_view utf8,
                                int x) const
{
    if (run.glyphs.empty() || x <= 0) return 0;

    // Convert logical x to physical for comparison with GlyphEntry positions
    int phys_x = static_cast<int>(x * dpi_scale_);

    // Find nearest glyph by x position (physical coords)
    for (size_t i = 0; i + 1 < run.glyphs.size(); ++i) {
        int mid = (run.glyphs[i].x + run.glyphs[i + 1].x) / 2;
        if (phys_x <= mid) {
            return byte_offset_to_col(utf8, run.glyphs[i].cluster);
        }
    }

    // Check if past the last glyph
    if (!run.glyphs.empty()) {
        const GlyphEntry& last = run.glyphs.back();
        const AtlasGlyph* ag = atlas_.get_or_add(last.glyph_id, last.font_index);
        int adv = ag ? ag->advance_x : 0;
        int mid = last.x + adv / 2;
        if (phys_x > mid) {
            // Past end — return total codepoint count
            return byte_offset_to_col(utf8, static_cast<int>(utf8.size()));
        }
    }

    // Last glyph
    return byte_offset_to_col(utf8, run.glyphs.back().cluster);
}

} // namespace sprawn
