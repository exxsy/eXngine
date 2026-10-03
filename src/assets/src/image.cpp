
#include <texture/image.h>

// TODO: Add your own image loading strategy, because why not!?
#ifdef EXN_IMAGE_STRATEGY_STBI
#ifndef EXN_IMAGE_STRATEGY_STBI_H
#define EXN_IMAGE_STRATEGY_STBI_H
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define EXN_LOAD_IMAGE(...) stbi_load(__VA_ARGS__)
#define EXN_FREE_IMAGE(...) stbi_image_free(__VA_ARGS__)
#endif
#else
#error "No image loading strategy defined. Please define EXN_IMAGE_STRATEGY_STBI to use stb_image."
#endif


extern EXINT32 hash(const std::string&);

namespace eXngine::Images
{
    bool ImageManager::LoadImageFromFile(const char * key, const char *path, ImageColorFormat bytes = ImageColorFormat::RGBA)
    {
        if (!path)
            return false;

        EXINT32 width = 0, height = 0, channels = 0;
		EXUINT8* data = nullptr;

#ifdef EXN_IMAGE_STRATEGY_STBI
        data = EXN_LOAD_IMAGE(path, &width, &height, &channels, bytes);
#endif

        if (data == EXN_NULL_HANDLE)
        {
            EX_WARNING("Failed to load image '%s': %s", path, stbi_failure_reason());
            return false;
        }

        const bool result = CreateImage(key, data, width, height, bytes);

#ifdef EXN_IMAGE_STRATEGY_STBI
        if (data) EXN_FREE_IMAGE(data);
#endif

        return result;
    }

    bool ImageManager::CreateImage(const char * key, const void *data, EXUINT32 width, EXUINT32 height, ImageColorFormat bytes = ImageColorFormat::RGBA)
    {
        if (!data || width == 0 || height == 0)
            return false;

        Image *texture = new Image();
        texture->width = width;
        texture->height = height;
        texture->size = static_cast<size_t>(width) * static_cast<size_t>(height) * bytes;
        texture->data = new EXUINT8[texture->size];

		memcpy_s(texture->data, texture->size, data, texture->size);

		const auto textureId = hash(std::string(key));

        if (m_sImages.find(textureId) != m_sImages.end())
        {
            delete texture;
            return false;
		}

        m_sImages.emplace(textureId, texture);

        return true;
    }

    bool DecodeImageFile(const char *path, std::vector<EXUINT8> &rgba, EXINT32 &width, EXINT32 &height)
    {
        EXINT32 channels = 0;
        EXUINT8 *data = path != EXN_NULL_HANDLE ? EXN_LOAD_IMAGE(path, &width, &height, &channels, STBI_rgb_alpha) : EXN_NULL_HANDLE;

        if (data == EXN_NULL_HANDLE)
            return false;

        rgba.assign(data, data + static_cast<size_t>(width) * height * 4);
        EXN_FREE_IMAGE(data);

        return true;
    }

    void ImageManager::DeleteImage(EXUINT32 textureId)
    {
        auto it = m_sImages.find(textureId);
        if (it != m_sImages.end())
        {
            delete it->second;
            m_sImages.erase(it);
        }
    }

    Image *ImageManager::GetImage(EXUINT32 textureId)
    {
        auto it = m_sImages.find(textureId);
        if (it != m_sImages.end())
        {
            return it->second;
        }
        return nullptr;
    }

    ImageManager::~ImageManager()
    {
        for (auto &pair : m_sImages)
        {
            delete pair.second;
        }

        m_sImages.clear();
	}

    Image::~Image()
    {
        // CreateImage copies the pixels into new[]: stb_image's buffer is already freed.
        delete[] data;
        data = EXN_NULL_HANDLE;
    }
}