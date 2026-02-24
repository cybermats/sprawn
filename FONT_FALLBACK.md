# Font Fallback Support

## Overview

The Sprawn frontend implements automatic font fallback chaining to ensure comprehensive Unicode coverage, particularly for CJK (Chinese, Japanese, Korean) characters and other complex scripts.

## Architecture

### Primary + Fallback Chain

The `Font` class maintains:
- **Primary font**: The main font loaded for the viewing session
- **Fallback fonts**: An ordered list of backup fonts tried when primary lacks glyphs

When the primary font cannot render a character (returns `.notdef` glyph with ID 0), the fallback chain is consulted in order until a fallback font provides a valid glyph.

### Implementation Details

#### Adding Fallback Fonts

```cpp
void Font::add_fallback(const std::string& path);
```

- Loads each fallback font via FreeType independently
- Creates a HarfBuzz font object for shaping with that fallback
- Silently skips unavailable fonts (no hard errors)
- All fallbacks use the same pixel size as the primary font

#### Shaping with Fallbacks

The `shape()` method:

1. **Phase 1**: Shape entire text with primary font
2. **Phase 2**: If fallbacks exist, scan for `.notdef` glyphs (ID 0)
3. **Phase 3**: For each `.notdef` glyph:
   - Extract the corresponding text cluster (using HarfBuzz cluster info)
   - Try each fallback font in order
   - Accept the first fallback that produces a valid glyph (ID != 0)
   - Replace the `.notdef` glyph with the fallback result

#### Cluster-based Mapping

HarfBuzz provides cluster information to map glyphs back to source text:
- Identifies byte range in source text for each glyph
- Handles variable-width UTF-8 encoding automatically
- Enables precise fallback substitution without re-parsing

### Performance Considerations

- **Lazy fallback shaping**: Only missing glyphs trigger fallback font queries
- **Caching**: GlyphCache stores results; fallback glyphs cached identically to primary glyphs
- **Advance metrics**: Fallback glyph advances correctly update total line advance

## Demo Configuration

The demo application (`examples/demo.cpp`) includes a default fallback chain:
- Primary: DejaVu Sans (Latin, Greek, Cyrillic)
- Fallback 1: Noto Sans CJK (CJK coverage)
- Fallback 2: Noto Sans Arabic (Arabic, Farsi, Urdu)
- Fallback 3: Noto Sans Thai (Thai, Lao, Khmer)

Users can easily extend this chain by calling `add_fallback()` with additional font paths.

## Future Enhancements

1. **Font config file**: Load fallback chains from system font config (fontconfig on Linux)
2. **Automatic fallback discovery**: Use HarfBuzz charset intersection to smart-select fallbacks
3. **Texture atlas**: Combine fallback glyphs into shared atlas for better cache locality
4. **Fallback caching**: Remember which glyphs came from which fallback to avoid re-querying

## Testing

The demo renders:
- **Latin**: "The quick brown fox jumps over the lazy dog."
- **Greek**: "Γρήγορη καφές πηδώντας πάνω από τον τεμπέλη σκύλο."
- **Cyrillic**: "Быстрая коричневая лиса прыгает через ленивую собаку."
- **CJK**: "快速棕色狐狸跳过懒狗。" (Chinese), "素早い茶色のキツネが怠け者の犬を飛び越えます。" (Japanese), "빠른 갈색 여우가 게으른 개를 뛰어넘습니다." (Korean)
- **Arabic**: "الثعلب البني السريع يقفز فوق الكلب الكسول."
- **Thai**: "สุนัขจิ้งจอกสีน้ำตาลที่เร็วจะกระโดดข้ามสุนัขขี้เกียจ"

All rendered correctly via primary + fallback chain.
