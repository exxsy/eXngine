#pragma once

#include <algorithm>

namespace eXngine::Types
{
    struct eXcolor
    {
        unsigned char r, g, b, a;

        constexpr eXcolor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255)
            : r(red), g(green), b(blue), a(alpha) {}

        constexpr unsigned int toInt() const
        {
            return (static_cast<unsigned int>(r) << 16) |
                   (static_cast<unsigned int>(g) << 8) |
                   static_cast<unsigned int>(b);
        }

        constexpr unsigned int toHex() const
        {
            return toInt();
        }
    };

    constexpr eXcolor operator""_rgb(unsigned long long value)
    {
        unsigned int b = value % 1000;
        unsigned int g = (value / 1000) % 1000;
        unsigned int r = (value / 1000000) % 1000;

        // Clamp to 0-255
        r = std::clamp(r, 0U, 255U); // (r > 255) ? 255 : r;
        g = std::clamp(g, 0U, 255U);
        b = std::clamp(b, 0U, 255U);

        return eXcolor(static_cast<unsigned char>(r),
                       static_cast<unsigned char>(g),
                       static_cast<unsigned char>(b), 255);
    }

    constexpr eXcolor operator""_rgba(unsigned long long value)
    {
        unsigned int a = value % 1000;
        unsigned int b = (value / 1000) % 1000;
        unsigned int g = (value / 1000000) % 1000;
        unsigned int r = (value / 1000000000) % 1000;

        r = std::clamp(r, 0U, 255U);
        g = std::clamp(g, 0U, 255U);
        b = std::clamp(b, 0U, 255U);
        a = std::clamp(a, 0U, 255U);

        return eXcolor(static_cast<unsigned char>(r),
                       static_cast<unsigned char>(g),
                       static_cast<unsigned char>(b),
                       static_cast<unsigned char>(a));
    }

    constexpr eXcolor operator""_hex(unsigned long long value)
    {
        unsigned char r = (value >> 16) & 0xFF;
        unsigned char g = (value >> 8) & 0xFF;
        unsigned char b = value & 0xFF;

        return eXcolor(r, g, b, 255);
    }

    constexpr eXcolor operator""_hexa(unsigned long long value)
    {
        unsigned char a = (value >> 24) & 0xFF;
        unsigned char r = (value >> 16) & 0xFF;
        unsigned char g = (value >> 8) & 0xFF;
        unsigned char b = value & 0xFF;

        return eXcolor(r, g, b, a);
    }
}