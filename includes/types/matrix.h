#pragma once

namespace eXngine::Types
{
    template <typename T, std::size_t N>
    struct EXNEXPORT eXmat : public glm::mat<N, N, T, glm::packed_highp>
    {
    };

    struct EXNEXPORT eXmat4 : public eXmat<EXFLOAT, 4>
    {
        eXmat4() : eXmat<EXFLOAT, 4>() {}
        eXmat4(EXFLOAT diagonal) : eXmat<EXFLOAT, 4>()
        {
            for (std::size_t i = 0; i < 4; i++)
                (*this)[i][i] = diagonal;
        }
        eXmat4(const EXMATH::mat4 &other) : eXmat<EXFLOAT, 4>()
        {
            for (std::size_t i = 0; i < 4; i++)
                for (std::size_t j = 0; j < 4; j++)
                    (*this)[i][j] = other[i][j];
        }

        eXmat4 operator=(const EXMATH::mat4 &other)
        {
            return eXmat4(other);
        }
    };
}