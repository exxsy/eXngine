#pragma once

#include <map>
#include <vector>
#include <eXngine.h>
#include <functional>
#include <cassert>
#include <string>
#include <utils/utils.h>

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

    struct EXNEXPORT Image
    {
    public:
        EXUINT16 width, height;
        EXUINT8* data;
        EXSIZE size;

        ~Image();
    };

    // Decodes an image file (png, jpg, bmp, tga, ...) to tightly packed RGBA8 pixels.
    // Touches no shared state, so it may run on a worker thread.
    EXNEXPORT bool DecodeImageFile(const char *path, std::vector<EXUINT8> &rgba, EXINT32 &width, EXINT32 &height);

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