#pragma once

#include <vulkan/vulkan_core.h>

// Generated files by glslang
#include <fullscreen_frag.h>
#include <fullscreen_vert.h>
#include <lit_pnc_vert.h>
#include <lit_pnc_frag.h>
#include <unlit_pc_frag.h>
#include <unlit_pc_vert.h>
#include <sky_frag.h>
#include <sky_vert.h>

namespace legs
{
class Shader
{
  public:
    Shader()                         = delete;
    ~Shader()                        = delete;
    Shader(const Shader&)            = delete;
    Shader(Shader&&)                 = delete;
    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&&)      = delete;

    constexpr static VkShaderModuleCreateInfo GetCreateInfo(const uint32_t spv[], const size_t size)
    {
        VkShaderModuleCreateInfo createInfo {};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = size;
        createInfo.pCode    = static_cast<const uint32_t*>(spv);
        return createInfo;
    }
};

// Usage:
// auto shader = LOAD_VULKAN_SPV(simple_frag);
// To load Shaders/simple_frag.frag (simple_frag.h)
#define LOAD_VULKAN_SPV(NAME) legs::Shader::GetCreateInfo(NAME, sizeof(NAME))
} // namespace legs
