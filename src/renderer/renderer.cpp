#include <imgui_impl_vulkan.h>
#include <memory>

#include <legs/log.hpp>
#include <legs/renderer/mesh_data.hpp>
#include <legs/renderer/renderer.hpp>
#include <legs/renderer/shader.hpp>

namespace legs
{

#define MAX_FRAMES_IN_FLIGHT 2

Renderer::Renderer(std::shared_ptr<Window> window) :
    m_instance(window),
    m_device(m_instance, MAX_FRAMES_IN_FLIGHT)
{
    LOG_INFO("Creating Renderer");

    auto uboBuffers = std::vector<std::shared_ptr<Buffer>>();
    for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        uboBuffers.push_back(
            std::make_shared<Buffer>(UniformBuffer, HostBuffer, sizeof(UniformBufferObject), 1)
        );
    }
    m_descriptorSet = std::make_shared<DescriptorSet>(m_device, uboBuffers);

    // TODO: abstract away all the shader + pipeline setup
    auto moduleSimpleFrag = CreateShaderModule(LOAD_VULKAN_SPV(unlit_pc_frag));
    auto moduleSimpleVert = CreateShaderModule(LOAD_VULKAN_SPV(unlit_pc_vert));
    auto stageSimpleFrag =
        FillShaderStageCreateInfo(moduleSimpleFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
    auto stageSimpleVert = FillShaderStageCreateInfo(moduleSimpleVert, VK_SHADER_STAGE_VERTEX_BIT);
    std::vector simpleStages {stageSimpleFrag, stageSimpleVert};

    m_testPipeline =
        std::make_shared<Pipeline<Vertex_P_C>>(m_device, m_descriptorSet, simpleStages);

    auto moduleGeoPNCFrag = CreateShaderModule(LOAD_VULKAN_SPV(lit_pnc_frag));
    auto moduleGeoPNCVert = CreateShaderModule(LOAD_VULKAN_SPV(lit_pnc_vert));
    auto stageGeoPNCFrag =
        FillShaderStageCreateInfo(moduleGeoPNCFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
    auto stageGeoPNCVert = FillShaderStageCreateInfo(moduleGeoPNCVert, VK_SHADER_STAGE_VERTEX_BIT);
    std::vector geoPNCStages {stageGeoPNCFrag, stageGeoPNCVert};

    m_geoPNCPipeline =
        std::make_shared<Pipeline<Vertex_P_N_C>>(m_device, m_descriptorSet, geoPNCStages);

    auto moduleFullscreenFrag = CreateShaderModule(LOAD_VULKAN_SPV(fullscreen_frag));
    auto moduleFullscreenVert = CreateShaderModule(LOAD_VULKAN_SPV(fullscreen_vert));
    auto stageFullscreenFrag =
        FillShaderStageCreateInfo(moduleFullscreenFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
    auto stageFullscreenVert =
        FillShaderStageCreateInfo(moduleFullscreenVert, VK_SHADER_STAGE_VERTEX_BIT);
    std::vector fullscreenStages {stageFullscreenFrag, stageFullscreenVert};

    m_fullscreenPipeline = std::make_shared<Pipeline<VertexEmpty>>(
        m_device,
        m_descriptorSet,
        fullscreenStages,
        false,
        false
    );

    auto moduleSkyFrag = CreateShaderModule(LOAD_VULKAN_SPV(sky_frag));
    auto moduleSkyVert = CreateShaderModule(LOAD_VULKAN_SPV(sky_vert));
    auto stageSkyFrag  = FillShaderStageCreateInfo(moduleSkyFrag, VK_SHADER_STAGE_FRAGMENT_BIT);
    auto stageSkyVert  = FillShaderStageCreateInfo(moduleSkyVert, VK_SHADER_STAGE_VERTEX_BIT);
    std::vector skyStages {stageSkyFrag, stageSkyVert};

    m_skyPipeline = std::make_shared<Pipeline<Vertex_P>>(m_device, m_descriptorSet, skyStages);

    m_ubo = std::make_shared<UniformBufferObject>();
}

Renderer::~Renderer()
{
    LOG_INFO("Destroying Renderer");

    vkDeviceWaitIdle(m_device.GetVkDevice());

    m_testPipeline.reset();
    m_geoPNCPipeline.reset();
    m_fullscreenPipeline.reset();
    m_skyPipeline.reset();

    m_descriptorSet.reset();

    for (auto& module : m_vkShaderModules)
    {
        vkDestroyShaderModule(m_device.GetVkDevice(), module, nullptr);
    }

    m_frameBuffers.clear();
}

void Renderer::SetWindow(std::shared_ptr<Window> window)
{
    LOG_INFO("Setting window");
    m_instance.SetWindow(window);
    Resize();
}

void Renderer::ClearViewport()
{
    auto commandBuffer = m_device.GetCommandBuffer();
    BindPipeline(RenderPipeline::FULLSCREEN);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
}

void Renderer::Resize()
{
    LOG_INFO("Resizing");
    m_device.ResizeFramebuffer();
}

float Renderer::GetAspect()
{
    return m_device.GetSwapchainAspect();
}

void Renderer::Begin()
{
    m_device.Begin();
}

void Renderer::Submit()
{
    m_device.Submit();

    // Not ideal but guarantees chunk buffers aren't freed too early.
    m_device.WaitForGraphicsIdle();
    m_frameBuffers.clear();
}

void Renderer::Present()
{
    m_device.Present();
}

void Renderer::UpdateUBO()
{
    auto currentFrame = m_device.GetCurrentFrame();
    m_descriptorSet->UpdateUBO(currentFrame, m_ubo);
}

void Renderer::WaitForIdle()
{
    vkDeviceWaitIdle(m_device.GetVkDevice());
}

void Renderer::GetImGuiInfo(ImGuiCreationInfo& info)
{
    info.colorFormat     = m_device.GetSwapchainImageFormat();
    VkFormat depthFormat = m_device.GetDepthFormat();

    info.pipelineCreateInfo       = {};
    info.pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    info.pipelineCreateInfo.pNext = VK_NULL_HANDLE;
    info.pipelineCreateInfo.colorAttachmentCount    = 1;
    info.pipelineCreateInfo.pColorAttachmentFormats = &info.colorFormat;
    info.pipelineCreateInfo.depthAttachmentFormat   = depthFormat;
    info.pipelineCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

    info.imGuiInfo                             = {};
    info.imGuiInfo.Instance                    = m_instance.GetVkInstance();
    info.imGuiInfo.PhysicalDevice              = m_device.GetVkPhysicalDevice();
    info.imGuiInfo.Device                      = m_device.GetVkDevice();
    info.imGuiInfo.QueueFamily                 = m_device.GetGraphicsQueueIndex();
    info.imGuiInfo.Queue                       = m_device.GetGraphicsQueue();
    info.imGuiInfo.PipelineCache               = VK_NULL_HANDLE; // TODO
    info.imGuiInfo.DescriptorPool              = m_device.GetImGuiDescriptorPool();
    info.imGuiInfo.UseDynamicRendering         = true;
    info.imGuiInfo.PipelineRenderingCreateInfo = info.pipelineCreateInfo;
    info.imGuiInfo.MinImageCount               = MAX_FRAMES_IN_FLIGHT;
    info.imGuiInfo.ImageCount                  = MAX_FRAMES_IN_FLIGHT;
    info.imGuiInfo.MSAASamples                 = VK_SAMPLE_COUNT_1_BIT;
    info.imGuiInfo.Allocator                   = nullptr; // TODO vma?
    info.imGuiInfo.CheckVkResultFn             = ImGuiVkCheck;
}

VkShaderModule& Renderer::CreateShaderModule(VkShaderModuleCreateInfo createInfo)
{
    VkShaderModule module;
    VK_CHECK(
        vkCreateShaderModule(m_device.GetVkDevice(), &createInfo, nullptr, &module),
        "Failed to create shader module"
    );
    return m_vkShaderModules.emplace_back(module);
}

constexpr VkPipelineShaderStageCreateInfo Renderer::FillShaderStageCreateInfo(
    VkShaderModule&       module,
    VkShaderStageFlagBits stage
)
{
    VkPipelineShaderStageCreateInfo createInfo {};
    createInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage  = stage;
    createInfo.module = module;
    createInfo.pName  = "main";
    return createInfo;
}
} // namespace legs
