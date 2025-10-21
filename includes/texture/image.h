#pragma once

#include <map>
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