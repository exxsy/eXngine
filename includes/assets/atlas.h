#pragma once

#include <map>
#include <string>
#include <vector>

#include <eXngine.h>
#include <utils/vertex.h>

namespace eXngine::Assets
{
    // Places rectangles into a fixed size area without overlaps, using the skyline
    // bottom-left method: the packer keeps the top edge ("skyline") of everything placed
    // so far as a list of horizontal segments, and puts every new rectangle where its top
    // ends up lowest. Rectangles are never rotated.
    //   AtlasPacker packer(256, 256);
    //   EXINT32 x, y;
    //   if (packer.Pack(30, 20, x, y)) ... // the rectangle goes to (x, y)
    // Packing tall rectangles first (see TextureAtlas::Build) leaves fewer gaps.
    class EXNEXPORT AtlasPacker
    {
    public:
        AtlasPacker() = default;
        AtlasPacker(EXINT32 width, EXINT32 height);

        // Forgets every placed rectangle.
        void Reset(EXINT32 width, EXINT32 height);
        // False when the rectangle does not fit anywhere anymore.
        EXBOOL Pack(EXINT32 width, EXINT32 height, EXINT32 &x, EXINT32 &y);

        EXINT32 GetWidth() const { return m_nWidth; }
        EXINT32 GetHeight() const { return m_nHeight; }
        // Share of the area covered by rectangles, 0..1.
        EXFLOAT GetOccupancy() const;

    private:
        struct Segment
        {
            EXINT32 X, Y, Width;
        };

        // Top of a width-wide rectangle whose left edge is at segment index, or -1.
        EXINT32 Fit(EXSIZE index, EXINT32 width, EXINT32 height) const;

        EXINT32 m_nWidth = 0, m_nHeight = 0;
        EXSIZE m_nUsedArea = 0;
        std::vector<Segment> m_Skyline;
    };

    // Where an image ended up in an atlas: pixel rectangle and texture coordinates.
    // UV0 is the top-left corner, UV1 the bottom-right one (v grows downwards, as in
    // the images and in the default shaders).
    struct AtlasRegion
    {
        std::string Name;
        EXINT32 X = 0, Y = 0, Width = 0, Height = 0;
        EXFLOAT U0 = 0.0f, V0 = 0.0f, U1 = 0.0f, V1 = 0.0f;
    };

    struct TextureAtlasSettings
    {
        // Empty pixels around every image, so linear filtering does not pull in the neighbours.
        EXINT32 Padding = 2;
        // Fills the padding with copies of the image's border pixels ("extrusion"): sprites
        // keep clean edges even when the sampler reads into the padding.
        EXBOOL ExtrudeEdges = true;
        // The atlas starts this big and doubles until everything fits, up to MaxSize.
        EXINT32 InitialSize = 256;
        EXINT32 MaxSize = 4096;
    };

    // Packs many small images into one big texture, so a single texture (and material)
    // can draw all of them: sprites, icons, the glyphs of a font, ...
    //   TextureAtlas atlas;
    //   atlas.AddImageFile("crate", "assets/textures/crate.png");
    //   atlas.AddImage("dot", pixels, 8, 8, 4);
    //   atlas.Build();
    //   renderer->CreateTexture("sprites", atlas.GetPixels().data(), atlas.GetWidth(), atlas.GetHeight());
    //   const AtlasRegion *crate = atlas.GetRegion("crate"); // UVs to draw the crate with
    // Images are kept, so more can be added and Build() called again (regions may move).
    // The pixels are RGBA8; one channel images become white with that channel as alpha.
    class EXNEXPORT TextureAtlas
    {
    public:
        TextureAtlas() = default;
        explicit TextureAtlas(const TextureAtlasSettings &settings) : m_Settings(settings) {}

        TextureAtlasSettings &GetSettings() { return m_Settings; }

        // channels: 1 (alpha/coverage), 2 (gray + alpha), 3 (RGB) or 4 (RGBA), tightly packed.
        // Adding a known name replaces that image.
        EXBOOL AddImage(const std::string &name, const EXUINT8 *pixels, EXINT32 width, EXINT32 height, EXINT32 channels);
        EXBOOL AddImageFile(const std::string &name, const std::string &path);
        EXBOOL RemoveImage(const std::string &name);
        void Clear();

        // Packs every image and draws the atlas. False when they do not fit into MaxSize;
        // the previous result is kept then.
        EXBOOL Build();
        // Images were added or removed since the last Build().
        EXBOOL IsDirty() const { return m_bDirty; }

        // Null for an unknown name, or before Build().
        const AtlasRegion *GetRegion(const std::string &name) const;
        const std::map<std::string, AtlasRegion> &GetRegions() const { return m_Regions; }
        EXSIZE GetImageCount() const { return static_cast<EXSIZE>(m_Images.size()); }

        const std::vector<EXUINT8> &GetPixels() const { return m_Pixels; }
        EXINT32 GetWidth() const { return m_nWidth; }
        EXINT32 GetHeight() const { return m_nHeight; }
        EXFLOAT GetOccupancy() const { return m_fOccupancy; }

    private:
        struct SourceImage
        {
            std::vector<EXUINT8> Pixels; // RGBA8
            EXINT32 Width = 0, Height = 0;
        };

        void Blit(const SourceImage &, EXINT32 x, EXINT32 y);

        TextureAtlasSettings m_Settings;
        std::map<std::string, SourceImage> m_Images;
        std::map<std::string, AtlasRegion> m_Regions;
        std::vector<EXUINT8> m_Pixels;
        EXINT32 m_nWidth = 0, m_nHeight = 0;
        EXFLOAT m_fOccupancy = 0.0f;
        EXBOOL m_bDirty = false;
    };

    // A rectangle showing an atlas region, centered on the pivot: pixelsPerUnit pixels of
    // the image make one unit, so sprites of one atlas keep their relative sizes.
    // pivot (0, 0) is the bottom-left corner of the sprite, (0.5, 0.5) its center.
    EXNEXPORT void BuildSpriteQuad(const AtlasRegion &, EXFLOAT pixelsPerUnit, std::vector<Utils::Vertex> &vertices,
                                   std::vector<EXUINT32> &indices, EXFLOAT pivotX = 0.5f, EXFLOAT pivotY = 0.5f);
}
