#pragma once

#include <stdexcept>

#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

namespace legs
{
#define VK_CHECK(RESULT, MSG)          \
    if (RESULT != VK_SUCCESS)          \
    {                                  \
        throw std::runtime_error(MSG); \
    }

constexpr static void ImGuiVkCheck(VkResult result)
{
    VK_CHECK(result, "ImGuiVkCheck failed");
}

// Hold color format so it doesn't get dropped from stack
struct ImGuiCreationInfo
{
    VkFormat                         colorFormat;
    VkPipelineRenderingCreateInfoKHR pipelineCreateInfo;
    ImGui_ImplVulkan_InitInfo        imGuiInfo;
};

constexpr static VkPipelineStageFlags GetPipelineStageFlags(const VkImageLayout imageLayout)
{
    switch (imageLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                   | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        default:
            throw std::runtime_error("Unhandled VkImageLayout to VkPipelineStageFlags conversion");
    }
}

constexpr static VkAccessFlags GetAccessFlags(const VkImageLayout imageLayout)
{
    switch (imageLayout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return 0;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_ACCESS_HOST_WRITE_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                   | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            return VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        default:
            throw std::runtime_error("Unhandled VkImageLayout to VkAccessFlags conversion");
    }
}

constexpr static void TransitionImageLayout(
    VkCommandBuffer                commandBuffer,
    VkImage                        image,
    VkImageLayout                  oldLayout,
    VkImageLayout                  newLayout,
    const VkImageSubresourceRange& subresourceRange,
    const VkPipelineStageFlags     srcStageMask,
    const VkPipelineStageFlags     dstStageMask,
    const VkAccessFlags            srcAccessMask,
    const VkAccessFlags            dstAccessMask
)
{
    VkImageMemoryBarrier barrier {};
    barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask       = srcAccessMask;
    barrier.dstAccessMask       = dstAccessMask;
    barrier.oldLayout           = oldLayout;
    barrier.newLayout           = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image               = image;
    barrier.subresourceRange    = subresourceRange;
    vkCmdPipelineBarrier(
        commandBuffer,
        srcStageMask,
        dstStageMask,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
}

constexpr static void TransitionImageLayout(
    VkCommandBuffer                commandBuffer,
    VkImage                        image,
    VkImageLayout                  oldLayout,
    VkImageLayout                  newLayout,
    const VkImageSubresourceRange& subresourceRange
)
{
    const VkPipelineStageFlags srcStageMask  = GetPipelineStageFlags(oldLayout);
    const VkPipelineStageFlags dstStageMask  = GetPipelineStageFlags(newLayout);
    const VkAccessFlags        srcAccessMask = GetAccessFlags(oldLayout);
    const VkAccessFlags        dstAccessMask = GetAccessFlags(newLayout);
    TransitionImageLayout(
        commandBuffer,
        image,
        oldLayout,
        newLayout,
        subresourceRange,
        srcStageMask,
        dstStageMask,
        srcAccessMask,
        dstAccessMask
    );
}

constexpr static void TransitionImageLayout(
    VkCommandBuffer commandBuffer,
    VkImage         image,
    VkImageLayout   oldLayout,
    VkImageLayout   newLayout
)
{
    VkImageSubresourceRange subRange {};
    subRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    subRange.baseMipLevel   = 0;
    subRange.levelCount     = 1;
    subRange.baseArrayLayer = 0;
    subRange.layerCount     = 1;
    TransitionImageLayout(commandBuffer, image, oldLayout, newLayout, subRange);
}
} // namespace legs
