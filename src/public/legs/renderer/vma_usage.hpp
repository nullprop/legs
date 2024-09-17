#pragma once

#include <vk_mem_alloc.h>

namespace legs
{
extern void CreateAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device);

extern void DestroyAllocator();

extern VmaTotalStatistics GetAllocatorTotalStatistics();

extern VmaAllocator g_vma;
} // namespace legs
