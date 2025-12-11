#pragma once

#include <types/color.h>

using namespace eXngine::Types;

namespace eXngine::Renderers
{
    enum ViewportRenderFlags : EXUINT32
    {
        VIEWPORT_RENDER_FLAG_NONE = 0,
        VIEWPORT_RENDER_FLAG_CLEAR_COLOR = 1 << 0,
        VIEWPORT_RENDER_FLAG_CLEAR_DEPTH = 1 << 1,
        VIEWPORT_RENDER_FLAG_CLEAR_STENCIL = 1 << 2,
    };

    class EXNEXPORT eXviewport
    {
    public:
        EXN_PROPERTY(EXUINT32, RenderFlags, RenderFlags, VIEWPORT_RENDER_FLAG_CLEAR_COLOR | VIEWPORT_RENDER_FLAG_CLEAR_DEPTH);
        EXN_PROPERTY(eXcolor, ClearColor, ClearColor, eXcolor(0, 0, 0, 255));
        EXN_PROPERTY(EXFLOAT, ClearDepth, ClearDepth, 1.0f);
        EXN_PROPERTY(EXUINT32, ClearStencil, ClearStencil, 0);
        EXN_PROPERTY(EXUINT32, X, X, 0);
        EXN_PROPERTY(EXUINT32, Y, Y, 0);
        EXN_PROPERTY(EXUINT32, Width, Width, 0);
        EXN_PROPERTY(EXUINT32, Height, Height, 0);

    public:
        inline EXFLOAT GetAspectRatio() const
        {
            if (m_Height == 0)
                return 1.0f;

            return static_cast<EXFLOAT>(m_Width) / static_cast<EXFLOAT>(m_Height);
        }
    };
}