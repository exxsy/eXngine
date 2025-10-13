#include <renderers/renderer.h>
#include <chrono>

#pragma comment(lib, "eXngine.texture.lib")

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

	FPSData BaseRenderer::GetFPS() const
	{
		return m_sFpsData;
	}

    void BaseRenderer::UpdateFPS()
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        m_sFpsData.m_fDeltaTime = currentTime - m_sFpsData.m_fLastTime;
        m_sFpsData.m_fLastTime = static_cast<float>(currentTime);
        m_sFpsData.m_fFrameCount++;
        m_sFpsData.m_fFrameTimer += m_sFpsData.m_fDeltaTime;

        if (m_sFpsData.m_fFrameTimer >= 1.0f)
        {
            m_sFpsData.m_fFPS = static_cast<float>(m_sFpsData.m_fFrameCount);
            m_sFpsData.m_fAverageDeltaTime = m_sFpsData.m_fFrameTimer / static_cast<float>(m_sFpsData.m_fFrameCount);
            m_sFpsData.m_fFrameCount = 0;
            m_sFpsData.m_fFrameTimer = 0.0f;
        }
    }

    void BaseRenderer::SetOnCleanupHandler(OnCleanupHandler fn)
    {
        this->m_fOnCleanup = fn;
    }

	Images::ImageManager* BaseRenderer::GetImageManager()
    {
        return Images::ImageManager::GetInstance();
	}
}
