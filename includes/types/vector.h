#pragma once

namespace eXngine::Types
{
    template <typename T, std::size_t N>
    struct eXvec : public glm::vec<N, T, glm::defaultp>
    {
    // public:
    //     T data[N];
    //     constexpr std::size_t length() const noexcept { return N; }
    //     T &operator[](std::size_t i) { return data[i]; }
    //     const T &operator[](std::size_t i) const { return data[i]; }
    };
}