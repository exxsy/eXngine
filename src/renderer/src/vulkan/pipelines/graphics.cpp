#ifndef EXN_DISABLE_VULKAN
#include <renderers/vulkan/renderer.h>
#include <renderers/vulkan/pipelines/graphics.h>
namespace eXngine::Renderers::Vulkan
{
    void VkGraphicsPipeline::SetExtent(VkExtent2D extent)
    {
		m_pExtent = &extent;

        if (m_pExtent != EXN_NULL_HANDLE && m_Viewports.size() <= 1 && m_Scissors.size() <= 1)
        {
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(m_pExtent->width);
            viewport.height = static_cast<float>(m_pExtent->height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = *m_pExtent;

			if (m_Viewports.size() == 1)
				m_Viewports.clear();

            if (m_Viewports.empty())
                m_Viewports.push_back({
                    .x = 0.0f,            
                    .y = 0.0f,            
                    .width = static_cast<float>(m_pExtent->width),
                    .height = static_cast<float>(m_pExtent->height),
                    .minDepth = 0.0f,
                    .maxDepth = 1.0f,
                });

            if (m_Scissors.size() == 1)
				m_Scissors.clear();

            if (m_Scissors.empty())
                m_Scissors.push_back({
                    .offset = { 0, 0 },
                    .extent = *m_pExtent,
			    });
        }
    }

    void VkGraphicsPipeline::AddViewport(VkViewport viewport)
    {
		m_Viewports.push_back(viewport);
    }

    void VkGraphicsPipeline::AddScissor(VkRect2D rect)
    {
		m_Scissors.push_back(rect);
    }

    void VkGraphicsPipeline::SetDynamicStates(std::vector<VkDynamicState> s)
    {
		m_DynamicStates = s;
    }
}
#endif // EXN_DISABLE_VULKAN