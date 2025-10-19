#ifndef EXN_DISABLE_VULKAN
#include <renderers/vulkan/renderer.h>

namespace eXngine::Renderers::Vulkan
{
    VkShaderModuleObject::VkShaderModuleObject(VkDevice device, VkShaderModule shader) : m_pDevice(device), m_pShader(shader) {}
    VkShaderModuleObject::~VkShaderModuleObject()
    {
        vkDestroyShaderModule(m_pDevice, m_pShader, nullptr);
    }

    VkPipelineShaderStageCreateInfo VkShaderModuleObject::GetStageCreateInfo(VkShaderModule module, const char* name, VkShaderStageFlagBits stage)
    {
        return {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage, // VK_SHADER_STAGE_VERTEX_BIT;
            .module = module,
            .pName = "main", //?
        };
    }

    void VkShaderModuleObject::SetPipeline(VkGraphicsPipeline * p)
    { 
        m_pPipeline = p; 
    }

    VkShaderStageFlagBits VkShaderModuleObject::GetStageFlagBits() const
    {
        switch (type)
        {
        case eXngine::Vertex:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case eXngine::Fragment:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case eXngine::Geometry:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case eXngine::Compute:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        case eXngine::TessellationControl:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case eXngine::TessellationEvaluation:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        default:
            return VK_SHADER_STAGE_ALL;
        }
    }
}
#endif // EXN_DISABLE_VULKAN