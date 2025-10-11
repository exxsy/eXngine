#include <renderers/renderer.h>

namespace eXngine::Renderers
{
	BaseRenderer::BaseRenderer(const char* name, Size sz) : m_szName(const_cast<char*>(name)), m_szFrameBufferSize(sz) { }

	void BaseRenderer::SetFrameBufferResize(bool state)
	{
		this->m_bFrameBufferResized = state;
	}

	void BaseRenderer::SetFrameBufferSize(Size sz)
	{
		this->m_szFrameBufferSize = sz;
	}
}
