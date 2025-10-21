#pragma once

#include <map>
#include <eXngine.h>
#include <functional>
#include <cassert>
#include <string>
#include <utils/utils.h>

// TODO: Add your own texture loading strategy, because why not!?
#ifdef EXN_TEXTURE_STRATEGY_STBI
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#define EXN_LOAD_TEXTURE(...) stbi_load(__VA_ARGS__)
#define EXN_FREE_TEXTURE(...) stbi_image_free(__VA_ARGS__)
#else
#error "No texture loading strategy defined. Please define EXN_TEXTURE_STRATEGY_STBI to use stb_image."
#endif

namespace eXngine::Images
{
    enum ImageColorFormat : int
    {
        DEFAULT,
        G = 1,
        GA = 2,
        RGB = 3,
        RGBA = 4,
        COUNT
    };

    struct Image
    {
    public:
        EXUINT16 width, height;
        EXUINT8* data;
        EXSIZE size;

        ~Image();
    };

    class EXNEXPORT ImageManager
    {
        EXN_SINGLETON(ImageManager, textureManager)
    protected:
        std::map<EXUINT32, Image*> m_sImages;

    public:
        bool LoadImageFromFile(const char *, const char *, ImageColorFormat);
        bool CreateImage(const char*, const void *, EXUINT32, EXUINT32, ImageColorFormat);
        void DeleteImage(EXUINT32);
        Image* GetImage(EXUINT32);

        ~ImageManager();
    };
}