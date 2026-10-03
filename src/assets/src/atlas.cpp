#include <algorithm>
#include <cstring>

#include <assets/atlas.h>
#include <texture/image.h>

namespace eXngine::Assets
{
    AtlasPacker::AtlasPacker(EXINT32 width, EXINT32 height)
    {
        Reset(width, height);
    }

    void AtlasPacker::Reset(EXINT32 width, EXINT32 height)
    {
        m_nWidth = std::max(width, 0);
        m_nHeight = std::max(height, 0);
        m_nUsedArea = 0;

        // At first the skyline is the floor: one segment over the whole width.
        m_Skyline.clear();
        m_Skyline.push_back({0, 0, m_nWidth});
    }

    EXFLOAT AtlasPacker::GetOccupancy() const
    {
        const EXSIZE area = static_cast<EXSIZE>(m_nWidth) * m_nHeight;
        return area > 0 ? static_cast<EXFLOAT>(m_nUsedArea) / static_cast<EXFLOAT>(area) : 0.0f;
    }

    EXINT32 AtlasPacker::Fit(EXSIZE index, EXINT32 width, EXINT32 height) const
    {
        const EXINT32 x = m_Skyline[index].X;

        if (x + width > m_nWidth)
            return -1;

        // The rectangle rests on the highest segment below it.
        EXINT32 y = 0;
        EXINT32 widthLeft = width;

        for (EXSIZE i = index; widthLeft > 0 && i < static_cast<EXSIZE>(m_Skyline.size()); ++i)
        {
            y = std::max(y, m_Skyline[i].Y);

            if (y + height > m_nHeight)
                return -1;

            widthLeft -= m_Skyline[i].Width;
        }

        return y;
    }

    EXBOOL AtlasPacker::Pack(EXINT32 width, EXINT32 height, EXINT32 &x, EXINT32 &y)
    {
        if (width <= 0 || height <= 0)
            return false;

        EXSIZE bestIndex = 0;
        EXINT32 bestTop = m_nHeight + 1, bestWidth = m_nWidth + 1;
        EXBOOL found = false;

        // Bottom-left rule: the lowest top edge wins, a narrower segment breaks ties.
        for (EXSIZE i = 0; i < static_cast<EXSIZE>(m_Skyline.size()); ++i)
        {
            const EXINT32 fit = Fit(i, width, height);

            if (fit < 0)
                continue;

            if (fit + height < bestTop || (fit + height == bestTop && m_Skyline[i].Width < bestWidth))
            {
                bestIndex = i;
                bestTop = fit + height;
                bestWidth = m_Skyline[i].Width;
                x = m_Skyline[i].X;
                y = fit;
                found = true;
            }
        }

        if (!found)
            return false;

        // The rectangle's top becomes a new segment of the skyline...
        m_Skyline.insert(m_Skyline.begin() + bestIndex, {x, y + height, width});

        // ...hiding the segments (or the parts of them) below it.
        for (EXSIZE i = bestIndex + 1; i < static_cast<EXSIZE>(m_Skyline.size());)
        {
            const Segment &previous = m_Skyline[i - 1];
            Segment &segment = m_Skyline[i];
            const EXINT32 overlap = previous.X + previous.Width - segment.X;

            if (overlap <= 0)
                break;

            segment.X += overlap;
            segment.Width -= overlap;

            if (segment.Width > 0)
                break;

            m_Skyline.erase(m_Skyline.begin() + i);
        }

        // Neighbours at the same height become one segment.
        for (EXSIZE i = 0; i + 1 < static_cast<EXSIZE>(m_Skyline.size());)
        {
            if (m_Skyline[i].Y == m_Skyline[i + 1].Y)
            {
                m_Skyline[i].Width += m_Skyline[i + 1].Width;
                m_Skyline.erase(m_Skyline.begin() + i + 1);
            }
            else
            {
                ++i;
            }
        }

        m_nUsedArea += static_cast<EXSIZE>(width) * height;
        return true;
    }

    EXBOOL TextureAtlas::AddImage(const std::string &name, const EXUINT8 *pixels, EXINT32 width, EXINT32 height, EXINT32 channels)
    {
        if (name.empty() || pixels == EXN_NULL_HANDLE || width <= 0 || height <= 0 || channels < 1 || channels > 4)
        {
            EX_WARNING("TextureAtlas: invalid image '%s'.", name.c_str());
            return false;
        }

        SourceImage image;
        image.Width = width;
        image.Height = height;
        image.Pixels.resize(static_cast<size_t>(width) * height * 4);

        for (size_t i = 0, count = static_cast<size_t>(width) * height; i < count; ++i)
        {
            const EXUINT8 *source = &pixels[i * channels];
            EXUINT8 *target = &image.Pixels[i * 4];

            switch (channels)
            {
            case 1: // coverage, e.g. a glyph: white, so the vertex color tints it
                target[0] = target[1] = target[2] = 255;
                target[3] = source[0];
                break;
            case 2:
                target[0] = target[1] = target[2] = source[0];
                target[3] = source[1];
                break;
            case 3:
                memcpy(target, source, 3);
                target[3] = 255;
                break;
            default:
                memcpy(target, source, 4);
                break;
            }
        }

        m_Images[name] = std::move(image);
        m_bDirty = true;

        return true;
    }

    EXBOOL TextureAtlas::AddImageFile(const std::string &name, const std::string &path)
    {
        std::vector<EXUINT8> rgba;
        EXINT32 width = 0, height = 0;

        if (!Images::DecodeImageFile(path.c_str(), rgba, width, height))
        {
            EX_WARNING("TextureAtlas: cannot load image '%s'.", path.c_str());
            return false;
        }

        return AddImage(name, rgba.data(), width, height, 4);
    }

    EXBOOL TextureAtlas::RemoveImage(const std::string &name)
    {
        if (m_Images.erase(name) == 0)
            return false;

        m_bDirty = true;
        return true;
    }

    void TextureAtlas::Clear()
    {
        m_Images.clear();
        m_Regions.clear();
        m_Pixels.clear();
        m_nWidth = m_nHeight = 0;
        m_fOccupancy = 0.0f;
        m_bDirty = false;
    }

    EXBOOL TextureAtlas::Build()
    {
        const EXINT32 padding = std::max(m_Settings.Padding, 0);

        // Tall images first: rows of similar height waste the least space.
        std::vector<const std::pair<const std::string, SourceImage> *> order;
        order.reserve(m_Images.size());

        for (const auto &image : m_Images)
            order.push_back(&image);

        std::sort(order.begin(), order.end(), [](const auto *a, const auto *b)
                  { return a->second.Height != b->second.Height ? a->second.Height > b->second.Height
                                                                : a->second.Width > b->second.Width; });

        EXINT32 width = std::max(m_Settings.InitialSize, 1), height = width;
        AtlasPacker packer;
        std::vector<std::pair<EXINT32, EXINT32>> positions(order.size());

        for (;;)
        {
            packer.Reset(width, height);
            EXBOOL packed = true;

            for (size_t i = 0; i < order.size() && packed; ++i)
            {
                const SourceImage &image = order[i]->second;
                packed = packer.Pack(image.Width + padding * 2, image.Height + padding * 2, positions[i].first, positions[i].second);
            }

            if (packed)
                break;

            // Grows one side at a time: 256x256, 512x256, 512x512, ...
            if (width <= height)
                width *= 2;
            else
                height *= 2;

            if (width > m_Settings.MaxSize || height > m_Settings.MaxSize)
            {
                EX_WARNING("TextureAtlas: %lld images do not fit into %dx%d.", GetImageCount(), m_Settings.MaxSize, m_Settings.MaxSize);
                return false;
            }
        }

        m_nWidth = width;
        m_nHeight = height;
        m_fOccupancy = packer.GetOccupancy();
        m_Pixels.assign(static_cast<size_t>(width) * height * 4, 0);
        m_Regions.clear();

        for (size_t i = 0; i < order.size(); ++i)
        {
            const auto &[name, image] = *order[i];
            const EXINT32 x = positions[i].first + padding;
            const EXINT32 y = positions[i].second + padding;

            Blit(image, x, y);

            AtlasRegion region;
            region.Name = name;
            region.X = x;
            region.Y = y;
            region.Width = image.Width;
            region.Height = image.Height;
            region.U0 = static_cast<EXFLOAT>(x) / width;
            region.V0 = static_cast<EXFLOAT>(y) / height;
            region.U1 = static_cast<EXFLOAT>(x + image.Width) / width;
            region.V1 = static_cast<EXFLOAT>(y + image.Height) / height;

            m_Regions.emplace(name, region);
        }

        m_bDirty = false;
        return true;
    }

    void TextureAtlas::Blit(const SourceImage &image, EXINT32 x, EXINT32 y)
    {
        // With extrusion the copy reaches into the padding, repeating the border pixels.
        const EXINT32 border = m_Settings.ExtrudeEdges ? std::max(m_Settings.Padding, 0) : 0;

        for (EXINT32 row = -border; row < image.Height + border; ++row)
        {
            const EXINT32 sourceRow = std::clamp(row, 0, image.Height - 1);

            for (EXINT32 column = -border; column < image.Width + border; ++column)
            {
                const EXINT32 sourceColumn = std::clamp(column, 0, image.Width - 1);
                const EXUINT8 *source = &image.Pixels[(static_cast<size_t>(sourceRow) * image.Width + sourceColumn) * 4];
                EXUINT8 *target = &m_Pixels[(static_cast<size_t>(y + row) * m_nWidth + (x + column)) * 4];

                memcpy(target, source, 4);
            }
        }
    }

    const AtlasRegion *TextureAtlas::GetRegion(const std::string &name) const
    {
        const auto it = m_Regions.find(name);
        return it != m_Regions.end() ? &it->second : EXN_NULL_HANDLE;
    }

    void BuildSpriteQuad(const AtlasRegion &region, EXFLOAT pixelsPerUnit, std::vector<Utils::Vertex> &vertices,
                         std::vector<EXUINT32> &indices, EXFLOAT pivotX, EXFLOAT pivotY)
    {
        const EXFLOAT scale = pixelsPerUnit > 0.0f ? 1.0f / pixelsPerUnit : 1.0f;
        const EXFLOAT width = region.Width * scale, height = region.Height * scale;
        const EXFLOAT left = -pivotX * width, bottom = -pivotY * height;
        const EXFLOAT right = left + width, top = bottom + height;

        // Image rows run top to bottom, so the top of the sprite samples V0.
        vertices = {
            {.color = {1.0f, 1.0f, 1.0f}, .coordinates = {left, bottom, 0.0f}, .uv = {region.U0, region.V1}},
            {.color = {1.0f, 1.0f, 1.0f}, .coordinates = {right, bottom, 0.0f}, .uv = {region.U1, region.V1}},
            {.color = {1.0f, 1.0f, 1.0f}, .coordinates = {right, top, 0.0f}, .uv = {region.U1, region.V0}},
            {.color = {1.0f, 1.0f, 1.0f}, .coordinates = {left, top, 0.0f}, .uv = {region.U0, region.V0}},
        };
        indices = {0, 1, 2, 2, 3, 0};
    }
}
