#include <vector>
#include <string>
#include <fstream>
#include <cassert>

namespace eXngine::Utils
{
    class File
    {
    public:
        static std::vector<char> Read(const std::string& filename)
        {
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            assert(file.is_open());

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);
            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
            return buffer;
        }
    };
}

