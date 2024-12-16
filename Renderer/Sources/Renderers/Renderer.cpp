#include "../../Renderers/Renderer.h"

eXngine::Renderers::Renderer::Renderer(const char* name) : m_szName(const_cast<char*>(name))
{
}

void eXngine::Renderers::Renderer::SetFrameBufferResize(bool state)
{
	this->m_bFrameBufferResized = state;
}
