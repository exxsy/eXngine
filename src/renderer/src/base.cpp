#include <renderers/base.h>
#include <chrono>

#pragma comment(lib, "eXngine.assets.lib")

namespace eXngine::Renderers
{
    BaseRenderer::BaseRenderer(const EXCHAR *name, Size sz) : m_szName(const_cast<EXCHAR *>(name)), m_szFrameBufferSize(sz), m_bFrameBufferResized(false)
    {
    }

    // BaseRenderer::BaseRenderer()// : AbstractRenderer(), m_szName(const_cast<EXCHAR *>("Renderer")), m_szFrameBufferSize(Size(0, 0)), m_bFrameBufferResized(false)
    // {
    // }

    FPSData BaseRenderer::GetFPS() const
    {
        return m_sFpsData;
    }

    void BaseRenderer::UpdateFPS()
    {
        auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

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

    Images::ImageManager *BaseRenderer::GetImageManager()
    {
        return Images::ImageManager::GetInstance();
    }
}
