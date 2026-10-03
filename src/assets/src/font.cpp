#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>

#include <assets/font.h>

// stb_truetype comes with Dear ImGui (3rdparty/imgui/imstb_truetype.h). STBTT_STATIC keeps
// its functions private to this file, so they cannot clash with ImGui's own copy.
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif
#include <imstb_truetype.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace eXngine::Assets
{
    struct Font::Impl
    {
        // stbtt_fontinfo points into the file: both live and move together.
        std::vector<EXUINT8> File;
        stbtt_fontinfo Info{};
        EXFLOAT Scale = 0.0f;
    };

    std::vector<GlyphRange> GetLatinGlyphRanges()
    {
        return {
            {0x0020, 0x007E}, // Basic Latin
            {0x00A0, 0x00FF}, // Latin-1 Supplement: ç ö ü ...
            {0x0100, 0x017F}, // Latin Extended-A: ğ ı İ ş ...
        };
    }

    EXUINT32 DecodeUtf8(const std::string &text, size_t &offset)
    {
        constexpr EXUINT32 replacement = 0xFFFD;
        const auto byte = [&](size_t i)
        { return static_cast<EXUINT8>(text[i]); };

        const EXUINT8 first = byte(offset);

        // The number of bytes is in the high bits of the first one: 0xxxxxxx, 110xxxxx,
        // 1110xxxx or 11110xxx; every following byte is 10xxxxxx.
        EXINT32 length = 0;
        EXUINT32 codepoint = 0;

        if (first < 0x80)
            length = 1, codepoint = first;
        else if ((first & 0xE0) == 0xC0)
            length = 2, codepoint = first & 0x1F;
        else if ((first & 0xF0) == 0xE0)
            length = 3, codepoint = first & 0x0F;
        else if ((first & 0xF8) == 0xF0)
            length = 4, codepoint = first & 0x07;
        else
        {
            ++offset;
            return replacement;
        }

        if (offset + length > text.size())
        {
            ++offset;
            return replacement;
        }

        for (EXINT32 i = 1; i < length; ++i)
        {
            const EXUINT8 next = byte(offset + i);

            if ((next & 0xC0) != 0x80)
            {
                ++offset;
                return replacement;
            }

            codepoint = (codepoint << 6) | (next & 0x3F);
        }

        // Overlong forms and surrogates are not valid UTF-8.
        constexpr EXUINT32 smallest[] = {0, 0, 0x80, 0x800, 0x10000};

        if (codepoint < smallest[length] || codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF))
        {
            ++offset;
            return replacement;
        }

        offset += length;
        return codepoint;
    }

    Font::Font() = default;
    Font::~Font() = default;
    Font::Font(Font &&) noexcept = default;
    Font &Font::operator=(Font &&) noexcept = default;

    EXBOOL Font::LoadFromFile(const std::string &path, const FontSettings &settings)
    {
        std::ifstream file(path, std::ios::binary);

        if (!file.is_open())
        {
            EX_WARNING("Font: cannot open '%s'.", path.c_str());
            return false;
        }

        std::vector<EXUINT8> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        if (!LoadFromMemory(std::move(data), settings))
        {
            EX_WARNING("Font: '%s' is not a font stb_truetype can read.", path.c_str());
            return false;
        }

        return true;
    }

    EXBOOL Font::LoadFromMemory(std::vector<EXUINT8> fontFile, const FontSettings &settings)
    {
        auto impl = std::make_unique<Impl>();
        impl->File = std::move(fontFile);

        if (impl->File.empty() || settings.PixelHeight <= 0.0f)
            return false;

        const EXINT32 offset = stbtt_GetFontOffsetForIndex(impl->File.data(), settings.FaceIndex);

        if (offset < 0 || !stbtt_InitFont(&impl->Info, impl->File.data(), offset))
            return false;

        // Font files store everything in "font units"; this scale turns them into pixels.
        impl->Scale = stbtt_ScaleForPixelHeight(&impl->Info, settings.PixelHeight);

        EXINT32 ascent = 0, descent = 0, lineGap = 0;
        stbtt_GetFontVMetrics(&impl->Info, &ascent, &descent, &lineGap);

        TextureAtlasSettings atlasSettings;
        atlasSettings.Padding = settings.Padding;
        // The padding around a glyph must stay transparent.
        atlasSettings.ExtrudeEdges = false;

        TextureAtlas atlas(atlasSettings);
        std::unordered_map<EXUINT32, Glyph> glyphs;
        std::vector<EXUINT8> bitmap;

        const auto bake = [&](EXUINT32 codepoint)
        {
            if (glyphs.contains(codepoint))
                return;

            const EXINT32 index = stbtt_FindGlyphIndex(&impl->Info, static_cast<EXINT32>(codepoint));

            // Index 0 is the "missing glyph" box: only the fallback may use it.
            if (index == 0 && codepoint != settings.FallbackCodepoint)
                return;

            EXINT32 advance = 0, leftSideBearing = 0, x0 = 0, y0 = 0, x1 = 0, y1 = 0;
            stbtt_GetGlyphHMetrics(&impl->Info, index, &advance, &leftSideBearing);
            stbtt_GetGlyphBitmapBox(&impl->Info, index, impl->Scale, impl->Scale, &x0, &y0, &x1, &y1);

            Glyph glyph;
            glyph.Codepoint = codepoint;
            glyph.Index = index;
            glyph.Advance = advance * impl->Scale;
            glyph.OffsetX = static_cast<EXFLOAT>(x0);
            glyph.OffsetY = static_cast<EXFLOAT>(y0);
            glyph.Width = x1 - x0;
            glyph.Height = y1 - y0;

            if (glyph.Width > 0 && glyph.Height > 0)
            {
                // An 8-bit coverage image: how much of every pixel the outline covers.
                bitmap.assign(static_cast<size_t>(glyph.Width) * glyph.Height, 0);
                stbtt_MakeGlyphBitmap(&impl->Info, bitmap.data(), glyph.Width, glyph.Height, glyph.Width, impl->Scale, impl->Scale, index);
                atlas.AddImage(std::to_string(codepoint), bitmap.data(), glyph.Width, glyph.Height, 1);
            }

            glyphs.emplace(codepoint, glyph);
        };

        for (const auto &range : settings.Ranges)
        {
            for (EXUINT32 codepoint = range.First; codepoint <= range.Last && codepoint <= 0x10FFFF; ++codepoint)
                bake(codepoint);
        }

        bake(settings.FallbackCodepoint);

        if (!atlas.Build())
            return false;

        for (auto &[codepoint, glyph] : glyphs)
        {
            if (const AtlasRegion *region = atlas.GetRegion(std::to_string(codepoint)))
            {
                glyph.U0 = region->U0;
                glyph.V0 = region->V0;
                glyph.U1 = region->U1;
                glyph.V1 = region->V1;
            }
        }

        m_pImpl = std::move(impl);
        m_Settings = settings;
        m_Atlas = std::move(atlas);
        m_Glyphs = std::move(glyphs);
        m_fAscent = ascent * m_pImpl->Scale;
        m_fDescent = descent * m_pImpl->Scale;
        m_fLineGap = lineGap * m_pImpl->Scale;

        return true;
    }

    const Glyph *Font::GetGlyph(EXUINT32 codepoint) const
    {
        auto it = m_Glyphs.find(codepoint);

        if (it == m_Glyphs.end())
            it = m_Glyphs.find(m_Settings.FallbackCodepoint);

        return it != m_Glyphs.end() ? &it->second : EXN_NULL_HANDLE;
    }

    EXFLOAT Font::GetKerning(const Glyph &left, const Glyph &right) const
    {
        if (m_pImpl == EXN_NULL_HANDLE)
            return 0.0f;

        return stbtt_GetGlyphKernAdvance(&m_pImpl->Info, left.Index, right.Index) * m_pImpl->Scale;
    }

    TextLayout Font::Layout(const std::string &utf8, const TextLayoutOptions &options) const
    {
        TextLayout layout;

        if (!IsLoaded())
            return layout;

        std::vector<EXUINT32> codepoints;
        codepoints.reserve(utf8.size());

        for (size_t offset = 0; offset < utf8.size();)
        {
            const EXUINT32 codepoint = DecodeUtf8(utf8, offset);

            if (codepoint != '\r')
                codepoints.push_back(codepoint);
        }

        const Glyph *space = GetGlyph(' ');
        const EXFLOAT tabAdvance = space != EXN_NULL_HANDLE ? space->Advance * std::max(options.TabSize, 1) : 0.0f;

        // How far the pen moves for codepoints[i], given the glyph before it.
        const auto advance = [&](const Glyph *previous, size_t i, const Glyph *glyph)
        {
            if (glyph == EXN_NULL_HANDLE)
                return 0.0f;

            if (codepoints[i] == '\t')
                return tabAdvance;

            const EXFLOAT kerning = options.Kerning && previous != EXN_NULL_HANDLE ? GetKerning(*previous, *glyph) : 0.0f;
            return kerning + glyph->Advance;
        };

        // Width of codepoints[begin, end) without trailing spaces, which alignment ignores.
        const auto measure = [&](size_t begin, size_t end)
        {
            while (end > begin && (codepoints[end - 1] == ' ' || codepoints[end - 1] == '\t'))
                --end;

            EXFLOAT width = 0.0f;
            const Glyph *previous = EXN_NULL_HANDLE;

            for (size_t i = begin; i < end; ++i)
            {
                const Glyph *glyph = GetGlyph(codepoints[i]);
                width += advance(previous, i, glyph);
                previous = glyph;
            }

            return width;
        };

        // 1. Split into lines, at '\n' and where a line gets wider than MaxWidth.
        struct Line
        {
            size_t Begin, End;
        };

        std::vector<Line> lines;
        size_t lineBegin = 0, lastSpace = std::string::npos;
        EXFLOAT pen = 0.0f;
        const Glyph *previous = EXN_NULL_HANDLE;

        for (size_t i = 0; i < codepoints.size(); ++i)
        {
            if (codepoints[i] == '\n')
            {
                lines.push_back({lineBegin, i});
                lineBegin = i + 1;
                lastSpace = std::string::npos;
                pen = 0.0f;
                previous = EXN_NULL_HANDLE;
                continue;
            }

            const Glyph *glyph = GetGlyph(codepoints[i]);
            const EXBOOL blank = codepoints[i] == ' ' || codepoints[i] == '\t';

            if (options.MaxWidth > 0.0f && !blank && i > lineBegin && pen + advance(previous, i, glyph) > options.MaxWidth)
            {
                if (lastSpace != std::string::npos)
                {
                    // Word wrap: the space becomes the line break, the word moves down.
                    lines.push_back({lineBegin, lastSpace});
                    lineBegin = lastSpace + 1;
                }
                else
                {
                    // A single word wider than the line: break inside it.
                    lines.push_back({lineBegin, i});
                    lineBegin = i;
                }

                lastSpace = std::string::npos;
                pen = measure(lineBegin, i);
                previous = i > lineBegin ? GetGlyph(codepoints[i - 1]) : EXN_NULL_HANDLE;
            }

            if (blank)
                lastSpace = i;

            pen += advance(previous, i, glyph);
            previous = glyph;
        }

        lines.push_back({lineBegin, codepoints.size()});

        // 2. Place the glyphs of every line on its baseline.
        const EXFLOAT lineAdvance = GetLineHeight() * options.LineSpacing;

        for (const auto &line : lines)
            layout.Width = std::max(layout.Width, measure(line.Begin, line.End));

        for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
        {
            const Line &line = lines[lineIndex];
            const EXFLOAT free = layout.Width - measure(line.Begin, line.End);
            const EXFLOAT baseline = m_fAscent + lineIndex * lineAdvance;

            pen = options.Align == TextAlign::Center ? free * 0.5f : options.Align == TextAlign::Right ? free
                                                                                                       : 0.0f;
            previous = EXN_NULL_HANDLE;

            for (size_t i = line.Begin; i < line.End; ++i)
            {
                const Glyph *glyph = GetGlyph(codepoints[i]);

                if (glyph == EXN_NULL_HANDLE)
                    continue;

                const EXFLOAT step = advance(previous, i, glyph);
                const EXFLOAT kerning = codepoints[i] == '\t' ? 0.0f : step - glyph->Advance;

                if (glyph->Width > 0 && codepoints[i] != '\t')
                {
                    // Glyph bitmaps are whole pixels: a rounded pen keeps them sharp at 1:1.
                    GlyphQuad quad;
                    quad.X0 = std::round(pen + kerning) + glyph->OffsetX;
                    quad.Y0 = std::round(baseline) + glyph->OffsetY;
                    quad.X1 = quad.X0 + glyph->Width;
                    quad.Y1 = quad.Y0 + glyph->Height;
                    quad.U0 = glyph->U0;
                    quad.V0 = glyph->V0;
                    quad.U1 = glyph->U1;
                    quad.V1 = glyph->V1;

                    layout.Quads.push_back(quad);
                }

                pen += step;
                previous = glyph;
            }
        }

        layout.LineCount = static_cast<EXINT32>(lines.size());
        layout.Height = m_fAscent - m_fDescent + (layout.LineCount - 1) * lineAdvance;

        return layout;
    }
}
