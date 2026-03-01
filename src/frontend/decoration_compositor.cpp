#include <sprawn/frontend/decoration_compositor.h>

#include <algorithm>
#include <vector>

namespace sprawn {

std::vector<StyledSpan> DecorationCompositor::flatten(
    const LineDecoration& deco, int line_byte_len,
    const TextStyle& default_style)
{
    if (line_byte_len <= 0)
        return {};

    // Collect valid spans (clamped to line bounds, non-zero length)
    std::vector<const StyledSpan*> valid;
    for (const auto& span : deco.spans) {
        int s = std::max(span.byte_start, 0);
        int e = std::min(span.byte_end, line_byte_len);
        if (s < e)
            valid.push_back(&span);
    }

    // No decoration spans → single default span
    if (valid.empty()) {
        StyledSpan ds;
        ds.byte_start = 0;
        ds.byte_end = line_byte_len;
        ds.style = default_style;
        ds.priority = 0;
        return {ds};
    }

    // Collect boundary points
    std::vector<int> boundaries;
    boundaries.push_back(0);
    boundaries.push_back(line_byte_len);
    for (const auto* span : valid) {
        int s = std::max(span->byte_start, 0);
        int e = std::min(span->byte_end, line_byte_len);
        boundaries.push_back(s);
        boundaries.push_back(e);
    }

    // Sort and deduplicate
    std::sort(boundaries.begin(), boundaries.end());
    boundaries.erase(std::unique(boundaries.begin(), boundaries.end()),
                     boundaries.end());

    // Build output spans for each sub-interval
    std::vector<StyledSpan> result;
    result.reserve(boundaries.size() - 1);

    for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
        int seg_start = boundaries[i];
        int seg_end = boundaries[i + 1];
        if (seg_start >= seg_end)
            continue;

        // Find all covering spans for this segment
        // A span covers if its clamped range includes [seg_start, seg_end)
        TextStyle composite = default_style;
        int best_priority = -1;
        int best_bg_priority = -1;
        int best_underline_priority = -1;
        bool any_underline = false;

        for (const auto* span : valid) {
            int s = std::max(span->byte_start, 0);
            int e = std::min(span->byte_end, line_byte_len);
            if (s > seg_start || e < seg_end)
                continue;

            // This span covers the segment
            int p = span->priority;

            // fg: highest priority wins
            if (p > best_priority) {
                best_priority = p;
                composite.fg = span->style.fg;
                composite.bold = span->style.bold;
                composite.italic = span->style.italic;
            }

            // bg: highest priority with bg.a > 0 wins
            if (span->style.bg.a > 0 && p > best_bg_priority) {
                best_bg_priority = p;
                composite.bg = span->style.bg;
            }

            // underline: union semantics
            if (span->style.underline) {
                any_underline = true;
                if (p > best_underline_priority) {
                    best_underline_priority = p;
                    composite.underline_color = span->style.underline_color;
                }
            }
        }

        composite.underline = any_underline;

        StyledSpan out;
        out.byte_start = seg_start;
        out.byte_end = seg_end;
        out.style = composite;
        out.priority = best_priority >= 0 ? best_priority : 0;
        result.push_back(out);
    }

    return result;
}

} // namespace sprawn
