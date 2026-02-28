#include <sprawn/frontend/text_layout.h>
#include "glyph_atlas.h"

#include <cstdint>
#include <string_view>

namespace sprawn {

namespace {

// Decode one UTF-8 codepoint from [pos, end). Advances pos past the sequence.
// Returns replacement char U+FFFD on invalid bytes.
uint32_t decode_utf8(const char*& pos, const char* end) {
    auto b = [](const char* p) { return static_cast<uint8_t>(*p); };

    if (pos >= end) return 0;

    uint8_t c = b(pos);
    uint32_t cp;
    int extra;

    if (c < 0x80)        { cp = c;           extra = 0; }
    else if (c < 0xC0)   { ++pos; return 0xFFFD; }  // continuation byte
    else if (c < 0xE0)   { cp = c & 0x1F;   extra = 1; }
    else if (c < 0xF0)   { cp = c & 0x0F;   extra = 2; }
    else if (c < 0xF8)   { cp = c & 0x07;   extra = 3; }
    else                 { ++pos; return 0xFFFD; }

    ++pos;
    for (int i = 0; i < extra; ++i) {
        if (pos >= end || (b(pos) & 0xC0) != 0x80) return 0xFFFD;
        cp = (cp << 6) | (b(pos) & 0x3F);
        ++pos;
    }
    return cp;
}

} // namespace

TextLayout::TextLayout(GlyphAtlas& atlas, int line_height, int ascent)
    : atlas_(atlas), line_height_(line_height), ascent_(ascent)
{}

GlyphRun TextLayout::shape_line(std::string_view utf8) {
    GlyphRun run;
    run.total_width = 0;

    const char* pos = utf8.data();
    const char* end = pos + utf8.size();

    while (pos < end) {
        const char* seq_start = pos;
        uint32_t cp = decode_utf8(pos, end);

        // Skip \r and \n
        if (cp == '\r' || cp == '\n') continue;
        // Skip NUL
        if (cp == 0) continue;

        GlyphEntry ge;
        ge.codepoint = cp;
        ge.x         = run.total_width;
        ge.cluster   = static_cast<int>(seq_start - utf8.data());

        const AtlasGlyph* ag = atlas_.get_or_add(cp);
        if (ag)
            run.total_width += ag->advance_x;

        run.glyphs.push_back(ge);
    }

    return run;
}

void TextLayout::draw_run(Renderer& r, const GlyphRun& run, int x, int y,
                          Color tint)
{
    int baseline_y = y + ascent_;

    for (const GlyphEntry& ge : run.glyphs) {
        const AtlasGlyph* ag = atlas_.get_or_add(ge.codepoint);
        if (!ag || ag->rect.w == 0 || ag->rect.h == 0)
            continue;

        SDL_Rect src = ag->rect;
        SDL_Rect dst;
        dst.x = x + ge.x + ag->bearing_x;
        dst.y = baseline_y - ag->bearing_y;
        dst.w = ag->rect.w;
        dst.h = ag->rect.h;

        r.blit(atlas_.texture(), src, dst, tint);
    }
}

int TextLayout::x_for_column(const GlyphRun& run, size_t col) const {
    if (col == 0 || run.glyphs.empty()) return 0;

    if (col >= run.glyphs.size()) {
        // Past the end: use last glyph x + its advance
        const GlyphEntry& last = run.glyphs.back();
        const AtlasGlyph* ag   = atlas_.get_or_add(last.codepoint);
        int adv = ag ? ag->advance_x : 0;
        return last.x + adv;
    }

    return run.glyphs[col].x;
}

size_t TextLayout::column_for_x(const GlyphRun& run, int x) const {
    if (run.glyphs.empty() || x <= 0) return 0;

    for (size_t i = 0; i + 1 < run.glyphs.size(); ++i) {
        int mid = (run.glyphs[i].x + run.glyphs[i + 1].x) / 2;
        if (x <= mid) return i;
    }

    // Check if x is beyond the last glyph midpoint
    if (!run.glyphs.empty()) {
        const GlyphEntry& last = run.glyphs.back();
        const AtlasGlyph* ag   = atlas_.get_or_add(last.codepoint);
        int adv  = ag ? ag->advance_x : 0;
        int mid  = last.x + adv / 2;
        if (x > mid) return run.glyphs.size(); // past end
    }

    return run.glyphs.size() > 0 ? run.glyphs.size() - 1 : 0;
}

} // namespace sprawn
