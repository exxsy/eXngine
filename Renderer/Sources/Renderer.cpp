#include <Renderers/AbstractRenderer.h>

eXngine::Renderers::Renderer::Renderer(const char* name, Size sz) : m_szName(const_cast<char*>(name)), m_szFrameBufferSize(sz)
{
}

void eXngine::Renderers::Renderer::SetFrameBufferResize(bool state)
{
	this->m_bFrameBufferResized = state;
}

void eXngine::Renderers::Renderer::SetFrameBufferSize(Size sz)
{
    this->m_szFrameBufferSize = sz;
}
