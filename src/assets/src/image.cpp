
#include <texture/image.h>

extern EXINT32 hash(const std::string&);

namespace eXngine::Images
{
    bool ImageManager::LoadImageFromFile(const char * key, const char *path, ImageColorFormat bytes = ImageColorFormat::RGBA)
    {
        if (!path)
            return false;

        EXINT32 width = 0, height = 0, channels = 0;
		EXUINT8* data = nullptr;

#ifdef EXN_TEXTURE_STRATEGY_STBI
        data = EXN_LOAD_TEXTURE(path, &width, &height, &channels, bytes);
#endif

        assert(data);

        const bool result = CreateImage(key, data, width, height, bytes);

#ifdef EXN_TEXTURE_STRATEGY_STBI
        if (data) EXN_FREE_TEXTURE(data);
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
        if (data)
        {
#ifdef EXN_TEXTURE_STRATEGY_STBI
            stbi_image_free(data);
#endif

            delete data;
            data = EXN_NULL_HANDLE;
        }
    }
}