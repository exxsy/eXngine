#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <eXngine.h>
#include <assets/atlas.h>

namespace eXngine::Assets
{
    // Unicode code points First..Last (inclusive) to bake into a font's atlas.
    struct GlyphRange
    {
        EXUINT32 First = 0, Last = 0;
    };

    // Basic Latin, Latin-1 Supplement and Latin Extended-A: English, Turkish
    // (ç ğ ı İ ö ş ü) and most other European languages.
    EXNEXPORT std::vector<GlyphRange> GetLatinGlyphRanges();

    struct FontSettings
    {
        // Distance from the highest ascender to the lowest descender, in pixels. The glyphs
        // are rendered at this size: bigger looks sharper up close, but needs more texture.
        EXFLOAT PixelHeight = 48.0f;
        std::vector<GlyphRange> Ranges = GetLatinGlyphRanges();
        // Drawn for code points the font (or Ranges) does not have.
        EXUINT32 FallbackCodepoint = '?';
        // Empty pixels between the glyphs in the atlas.
        EXINT32 Padding = 2;
        // Font of a collection (.ttc) to use.
        EXINT32 FaceIndex = 0;
    };

    // One baked character. Positions are in pixels, relative to the pen on the baseline,
    // y growing downwards (like the image rows).
    struct Glyph
    {
        EXUINT32 Codepoint = 0;
        // Index of the glyph inside the font file, for kerning.
        EXINT32 Index = 0;
        // How far the pen moves after this glyph.
        EXFLOAT Advance = 0.0f;
        // Top-left corner of the bitmap, seen from the pen.
        EXFLOAT OffsetX = 0.0f, OffsetY = 0.0f;
        // Size of the bitmap: 0 for blank glyphs such as the space, which only advance.
        EXINT32 Width = 0, Height = 0;
        // Where the bitmap is in the font's atlas.
        EXFLOAT U0 = 0.0f, V0 = 0.0f, U1 = 0.0f, V1 = 0.0f;
    };

    enum class TextAlign
    {
        Left,
        Center,
        Right,
    };

    struct TextLayoutOptions
    {
        TextAlign Align = TextAlign::Left;
        // Lines wider than this many pixels break at the last space (or inside a word that
        // is too long on its own). 0 only breaks at '\n'.
        EXFLOAT MaxWidth = 0.0f;
        // Multiplies the distance between two baselines.
        EXFLOAT LineSpacing = 1.0f;
        EXBOOL Kerning = true;
        // A tab advances as far as this many spaces.
        EXINT32 TabSize = 4;
    };

    // One glyph rectangle of a laid out text: pixels, y down, (0, 0) being the top-left
    // corner of the text's bounding box.
    struct GlyphQuad
    {
        EXFLOAT X0 = 0.0f, Y0 = 0.0f, X1 = 0.0f, Y1 = 0.0f;
        EXFLOAT U0 = 0.0f, V0 = 0.0f, U1 = 0.0f, V1 = 0.0f;
    };

    struct TextLayout
    {
        std::vector<GlyphQuad> Quads;
        // Bounding box of the text, in pixels.
        EXFLOAT Width = 0.0f, Height = 0.0f;
        EXINT32 LineCount = 0;
    };

    // A TrueType/OpenType font, rendered once at a fixed size into a TextureAtlas
    // ("bitmap font"). Layout() turns UTF-8 text into textured rectangles, one per glyph,
    // that sample the atlas.
    //   Font font;
    //   font.LoadFromFile("C:/Windows/Fonts/segoeui.ttf", {.PixelHeight = 32.0f});
    //   const TextLayout layout = font.Layout("Merhaba Dünya!");
    //   // upload font.GetAtlas().GetPixels() once, then draw layout.Quads
    // Only load fonts you trust: the parser does not guard against malicious files.
    class EXNEXPORT Font
    {
    public:
        Font();
        ~Font();
        Font(Font &&) noexcept;
        Font &operator=(Font &&) noexcept;
        Font(const Font &) = delete;
        Font &operator=(const Font &) = delete;

        EXBOOL LoadFromFile(const std::string &path, const FontSettings & = {});
        EXBOOL LoadFromMemory(std::vector<EXUINT8> fontFile, const FontSettings & = {});
        EXBOOL IsLoaded() const { return m_pImpl != EXN_NULL_HANDLE; }

        // The fallback glyph for a code point that was not baked, null if there is none.
        const Glyph *GetGlyph(EXUINT32 codepoint) const;
        // Extra distance between two glyphs, e.g. negative for "AV". In pixels.
        EXFLOAT GetKerning(const Glyph &left, const Glyph &right) const;

        TextLayout Layout(const std::string &utf8, const TextLayoutOptions & = {}) const;

        const FontSettings &GetSettings() const { return m_Settings; }
        const TextureAtlas &GetAtlas() const { return m_Atlas; }
        const std::unordered_map<EXUINT32, Glyph> &GetGlyphs() const { return m_Glyphs; }

        // Vertical metrics in pixels; Descent is negative (below the baseline).
        EXFLOAT GetAscent() const { return m_fAscent; }
        EXFLOAT GetDescent() const { return m_fDescent; }
        EXFLOAT GetLineGap() const { return m_fLineGap; }
        // Distance between two baselines.
        EXFLOAT GetLineHeight() const { return m_fAscent - m_fDescent + m_fLineGap; }

    private:
        // The parsed font file (stb_truetype), kept for kerning.
        struct Impl;

        std::unique_ptr<Impl> m_pImpl;
        FontSettings m_Settings;
        TextureAtlas m_Atlas;
        std::unordered_map<EXUINT32, Glyph> m_Glyphs;
        EXFLOAT m_fAscent = 0.0f, m_fDescent = 0.0f, m_fLineGap = 0.0f;
    };

    // Reads the code point starting at offset and moves offset past it. Malformed
    // sequences give U+FFFD and skip one byte.
    EXNEXPORT EXUINT32 DecodeUtf8(const std::string &text, size_t &offset);
}
