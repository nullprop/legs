#pragma once

#include <vulkan/vulkan_core.h>

#include <legs/renderer/buffer.hpp>
#include <legs/renderer/device.hpp>
#include <legs/renderer/mesh_data.hpp>
#include <legs/renderer/ubo.hpp>

namespace legs
{
class DescriptorSet
{
  public:
    DescriptorSet() = delete;
    DescriptorSet(const Device& device, std::vector<std::shared_ptr<Buffer>> uboBuffers);
    ~DescriptorSet();

    DescriptorSet(const DescriptorSet&)            = delete;
    DescriptorSet(DescriptorSet&&)                 = default;
    DescriptorSet& operator=(const DescriptorSet&) = delete;
    DescriptorSet& operator=(DescriptorSet&&)      = delete;

    void UpdateUBO(uint32_t frameIndex, const std::shared_ptr<UniformBufferObject> ubo);

    void Bind(
        VkCommandBuffer     commandBuffer,
        VkPipelineBindPoint bindPoint,
        VkPipelineLayout    pipelineLayout,
        uint32_t            frameIndex
    );

    std::vector<VkDescriptorSetLayout> GetLayouts() const
    {
        return m_vkLayouts;
    }

  private:
    const Device&                        m_device;
    std::vector<VkDescriptorSetLayout>   m_vkLayouts;
    std::vector<VkDescriptorSet>         m_vkSets;
    std::vector<std::shared_ptr<Buffer>> m_uniformBuffers;
    std::vector<void*>                   m_ubosMappedMemory;
};
} // namespace legs
