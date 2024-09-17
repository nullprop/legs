#pragma once

#include <memory>
#include <stdexcept>

#include <imgui_impl_vulkan.h>

#include <legs/entity/camera.hpp>
#include <legs/renderer/buffer.hpp>
#include <legs/renderer/common.hpp>
#include <legs/renderer/descriptor_set.hpp>
#include <legs/renderer/device.hpp>
#include <legs/renderer/instance.hpp>
#include <legs/renderer/pipeline.hpp>
#include <legs/renderer/ubo.hpp>

namespace legs
{
enum RenderPipeline
{
    INVALID,
    GEO_P_C,
    GEO_P_N_C,
    FULLSCREEN,
    SKY,
};

class Renderer
{
  public:
    Renderer(std::shared_ptr<Window> window);
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer(Renderer&&)                 = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&)      = delete;

    void SetWindow(std::shared_ptr<Window> window);

    void ResetViewport()
    {
        m_device.ResetViewport();
    }

    void SetViewport(SRect rect)
    {
        m_device.SetViewport(rect);
    }

    void  ClearViewport();
    void  Resize();
    float GetAspect();
    void  Begin();
    void  Submit();
    void  Present();
    void  UpdateUBO();
    void  WaitForIdle();

    std::shared_ptr<UniformBufferObject> GetUBO()
    {
        return m_ubo;
    }

    void* GetCommandBuffer()
    {
        return GetVkCommandBuffer();
    };

    void GetImGuiInfo(ImGuiCreationInfo& info);

    VkCommandBuffer GetVkCommandBuffer() const
    {
        return m_device.GetCommandBuffer();
    }

    VkCommandBuffer GetTemporaryCommandBuffer()
    {
        return m_device.GetTemporaryCommandBuffer();
    }

    void SubmitTemporaryCommandBuffer(VkCommandBuffer buffer)
    {
        m_device.SubmitTemporaryCommandBuffer(buffer);
    }

    void CreateBuffer(
        std::shared_ptr<Buffer>& buffer,
        BufferType               bufferType,
        void*                    data,
        uint32_t                 elementSize,
        uint32_t                 elementCount
    )
    {
        auto hostBuffer =
            std::make_shared<Buffer>(bufferType, HostBuffer, elementSize, elementCount);
        auto deviceBuffer =
            std::make_shared<Buffer>(bufferType, DeviceBuffer, elementSize, elementCount);
        hostBuffer->Write(data, elementSize * elementCount);
        auto tempBuffer = GetTemporaryCommandBuffer();
        hostBuffer->CopyToDevice(tempBuffer, static_pointer_cast<Buffer>(deviceBuffer));
        SubmitTemporaryCommandBuffer(tempBuffer);
        buffer = static_pointer_cast<Buffer>(deviceBuffer);
    }

    void DrawWithBuffers(std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer)
    {
        auto commandBuffer = GetCommandBuffer();
        if (commandBuffer != nullptr)
        {
            vertexBuffer->Bind(commandBuffer);
            indexBuffer->Bind(commandBuffer);
            indexBuffer->Draw(commandBuffer);
            m_frameBuffers.push_back(vertexBuffer);
            m_frameBuffers.push_back(indexBuffer);
        }
    }

    void BindPipeline(RenderPipeline pipe)
    {
        auto commandBuffer = m_device.GetCommandBuffer();
        auto currentFrame  = m_device.GetCurrentFrame();

        switch (pipe)
        {
            case GEO_P_C:
            {
                m_testPipeline->Bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentFrame);
                break;
            }

            case GEO_P_N_C:
            {
                m_geoPNCPipeline
                    ->Bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentFrame);
                break;
            }

            case FULLSCREEN:
            {
                m_fullscreenPipeline
                    ->Bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentFrame);
                break;
            }

            case SKY:
            {
                m_skyPipeline->Bind(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentFrame);
                break;
            }

            default:
            {
                throw std::runtime_error("unknown pipeline");
            }
        }
    }

  private:
    VkShaderModule& CreateShaderModule(VkShaderModuleCreateInfo createInfo);
    constexpr VkPipelineShaderStageCreateInfo FillShaderStageCreateInfo(
        VkShaderModule&       module,
        VkShaderStageFlagBits stage
    );

    Instance m_instance;
    Device   m_device;

    std::shared_ptr<DescriptorSet> m_descriptorSet;
    std::vector<VkShaderModule>    m_vkShaderModules;

    std::shared_ptr<Pipeline<Vertex_P_C>>   m_testPipeline;
    std::shared_ptr<Pipeline<Vertex_P_N_C>> m_geoPNCPipeline;
    std::shared_ptr<Pipeline<VertexEmpty>>  m_fullscreenPipeline;
    std::shared_ptr<Pipeline<Vertex_P>>     m_skyPipeline;

    std::shared_ptr<UniformBufferObject> m_ubo;

    // Hold so we don't call Buffer destructor
    // while still in use by command buffer.
    std::vector<std::shared_ptr<Buffer>> m_frameBuffers;
};
} // namespace legs
