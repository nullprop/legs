#include "vk_mem_alloc.h"
#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000 //  1.3

#include <legs/renderer/common.hpp>
#include <legs/renderer/vma_usage.hpp>

namespace legs
{
VmaAllocator g_vma;

void CreateAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
    VmaAllocatorCreateInfo createInfo {};
    createInfo.instance         = instance;
    createInfo.physicalDevice   = physicalDevice;
    createInfo.device           = device;
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    VK_CHECK(vmaCreateAllocator(&createInfo, &g_vma), "Failed to crate vulkan allocator");
}

void DestroyAllocator()
{
    vmaDestroyAllocator(g_vma);
}

VmaTotalStatistics GetAllocatorTotalStatistics()
{
    VmaTotalStatistics stats {};
    vmaCalculateStatistics(g_vma, &stats);
    return stats;
}
} // namespace legs
