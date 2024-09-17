#include <algorithm>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan_core.h>

#include <legs/log.hpp>
#include <legs/renderer/common.hpp>
#include <legs/renderer/device.hpp>

namespace legs
{
constexpr static void _vkCmdBeginRenderingKHR(
    VkInstance             instance,
    VkCommandBuffer        commandBuffer,
    const VkRenderingInfo* renderingInfo
)
{
    auto func =
        (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
    if (func != nullptr)
    {
        func(commandBuffer, renderingInfo);
    }
    else
    {
        throw std::runtime_error("Failed to find vkCmdBeginRenderingKHR address");
    }
}

constexpr static void _vkCmdEndRenderingKHR(VkInstance instance, VkCommandBuffer commandBuffer)
{
    auto func = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");
    if (func != nullptr)
    {
        func(commandBuffer);
    }
    else
    {
        throw std::runtime_error("Failed to find vkCmdEndRenderingKHR address");
    }
}

Device::Device(const Instance& instance, uint32_t maxFramesInFlight) :
    m_instance(instance),
    m_maxFramesInFlight(maxFramesInFlight)
{
    LOG_INFO("Creating Device");

    PickPhysicalDevice();
    CreateLogicalDevice();

    CreateAllocator(m_instance.GetVkInstance(), m_vkPhysicalDevice, m_vkDevice);

    CreateSwapchain();
    CreateImageViews();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
    CreateDescriptorPools();
}

Device::~Device()
{
    LOG_INFO("Destroying Device");

    vkDeviceWaitIdle(m_vkDevice);

    vkDestroyDescriptorPool(m_vkDevice, m_vkUboDescriptorPool, nullptr);
    vkDestroyDescriptorPool(m_vkDevice, m_vkImGuiDescriptorPool, nullptr);

    for (auto& fence : m_vkInFlightFences)
    {
        vkDestroyFence(m_vkDevice, fence, nullptr);
    }
    for (auto& semaphore : m_vkImageSemaphores)
    {
        vkDestroySemaphore(m_vkDevice, semaphore, nullptr);
    }
    for (auto& semaphore : m_vkRenderSemaphores)
    {
        vkDestroySemaphore(m_vkDevice, semaphore, nullptr);
    }

    vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);

    DestroySwapchain();

    DestroyAllocator();

    vkDestroyDevice(m_vkDevice, nullptr);
}

void Device::Begin()
{
    VK_CHECK(
        vkWaitForFences(m_vkDevice, 1, &m_vkInFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX),
        "Failed waiting for in flight fence"
    );

    auto imageResult = vkAcquireNextImageKHR(
        m_vkDevice,
        m_vkSwapchain,
        UINT64_MAX,
        m_vkImageSemaphores[m_currentFrame],
        VK_NULL_HANDLE,
        &m_currentImageIndex
    );
    if (imageResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapchain();
        return;
    }
    else if (imageResult != VK_SUCCESS && imageResult != VK_SUBOPTIMAL_KHR)
    {

        throw std::runtime_error("Failed to acquire next swapchain image");
    }

    VK_CHECK(
        vkResetFences(m_vkDevice, 1, &m_vkInFlightFences[m_currentFrame]),
        "Failed to reset in flight fence"
    );

    VK_CHECK(
        vkResetCommandBuffer(m_vkCommandBuffers[m_currentFrame], 0),
        "Failed to reset command buffer"
    );

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK(
        vkBeginCommandBuffer(m_vkCommandBuffers[m_currentFrame], &beginInfo),
        "Failed to begin command buffer"
    );

    TransitionImageLayout(
        m_vkCommandBuffers[m_currentFrame],
        m_vkSwapchainImages[m_currentImageIndex],
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    );

    VkClearValue clearColor {};
    clearColor.color = {
        {0.0f, 0.0f, 0.0f, 1.0f}
    };
    VkClearValue clearDepth {};
    clearDepth.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfoKHR colorAttachment {};
    colorAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.clearValue  = clearColor;
    colorAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.imageView   = m_vkSwapchainImageViews[m_currentImageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;

    VkRenderingAttachmentInfoKHR depthAttachment {};
    depthAttachment.sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.clearValue  = clearDepth;
    depthAttachment.loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.imageView   = m_vkDepthImageView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;

    VkRect2D renderArea {};
    renderArea.extent = m_vkSwapchainExtent;
    renderArea.offset = {0, 0};

    VkRenderingInfo renderingInfo {};
    renderingInfo.sType                = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea           = renderArea;
    renderingInfo.viewMask             = 0;
    renderingInfo.layerCount           = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments    = &colorAttachment;
    renderingInfo.pDepthAttachment     = &depthAttachment;
    renderingInfo.pStencilAttachment   = nullptr;
    renderingInfo.flags                = 0;

    _vkCmdBeginRenderingKHR(
        m_instance.GetVkInstance(),
        m_vkCommandBuffers[m_currentFrame],
        &renderingInfo
    );

    ResetViewport();
}

void Device::ResetViewport()
{
    VkViewport viewport {};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(m_vkSwapchainExtent.width);
    viewport.height   = static_cast<float>(m_vkSwapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_vkCommandBuffers[m_currentFrame], 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = {0, 0};
    scissor.extent = m_vkSwapchainExtent;
    vkCmdSetScissor(m_vkCommandBuffers[m_currentFrame], 0, 1, &scissor);
}

void Device::SetViewport(SRect rect)
{
    VkViewport viewport {};
    viewport.x        = static_cast<float>(rect.offset.x);
    viewport.y        = static_cast<float>(rect.offset.y);
    viewport.width    = static_cast<float>(rect.size.x);
    viewport.height   = static_cast<float>(rect.size.y);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_vkCommandBuffers[m_currentFrame], 0, 1, &viewport);

    VkRect2D scissor {};
    scissor.offset = {rect.offset.x, rect.offset.y};
    scissor.extent = {static_cast<uint32_t>(rect.size.x), static_cast<uint32_t>(rect.size.y)};
    vkCmdSetScissor(m_vkCommandBuffers[m_currentFrame], 0, 1, &scissor);
}

void Device::Submit()
{
    _vkCmdEndRenderingKHR(m_instance.GetVkInstance(), m_vkCommandBuffers[m_currentFrame]);

    TransitionImageLayout(
        m_vkCommandBuffers[m_currentFrame],
        m_vkSwapchainImages[m_currentImageIndex],
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    );

    VK_CHECK(
        vkEndCommandBuffer(m_vkCommandBuffers[m_currentFrame]),
        "Failed to end command buffer"
    );

    VkSemaphore          waitSemaphores[]   = {m_vkImageSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore          signalSemaphores[] = {m_vkRenderSemaphores[m_currentFrame]};

    VkSubmitInfo submitInfo {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &m_vkCommandBuffers[m_currentFrame];
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    VK_CHECK(
        vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, m_vkInFlightFences[m_currentFrame]),
        "Failed to submit queue"
    );
}

void Device::Present()
{
    VkSemaphore    signalSemaphores[] = {m_vkRenderSemaphores[m_currentFrame]};
    VkSwapchainKHR swapchains[]       = {m_vkSwapchain};

    VkPresentInfoKHR presentInfo {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &m_currentImageIndex;

    auto presentResult = vkQueuePresentKHR(m_vkPresentQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR
        || m_frameBufferResized)
    {
        m_frameBufferResized = false;
        RecreateSwapchain();
    }
    else if (presentResult != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present queue");
    }

    m_currentFrame = (m_currentFrame + 1) % m_maxFramesInFlight;
}

void Device::WaitForGraphicsIdle()
{
    vkQueueWaitIdle(m_vkGraphicsQueue);
}

VkCommandBuffer Device::GetTemporaryCommandBuffer()
{
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // TODO: use separate pool
    allocInfo.commandPool        = m_vkCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer {};
    VK_CHECK(
        vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &commandBuffer),
        "Failed to allocate temporary command buffer"
    );

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(
        vkBeginCommandBuffer(commandBuffer, &beginInfo),
        "Failed to begin temporary command buffer"
    );

    return commandBuffer;
}

void Device::SubmitTemporaryCommandBuffer(VkCommandBuffer commandBuffer)
{
    VK_CHECK(vkEndCommandBuffer(commandBuffer), "Failed to end temporary command buffer");

    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    VK_CHECK(
        vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE),
        "Failed to submit temporary command buffer"
    );
    // TODO: wait all copies with fences?
    WaitForGraphicsIdle();

    vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &commandBuffer);
}

void Device::RecreateSwapchain()
{
    vkDeviceWaitIdle(m_vkDevice);

    DestroySwapchain();

    CreateSwapchain();
    CreateImageViews();
}

void Device::DestroySwapchain()
{
    for (auto view : m_vkSwapchainImageViews)
    {
        vkDestroyImageView(m_vkDevice, view, nullptr);
    }

    vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr);

    vkDestroyImageView(m_vkDevice, m_vkDepthImageView, nullptr);
    vmaDestroyImage(g_vma, m_vkDepthImage, m_vmaDepthAllocation);
}

void Device::PickPhysicalDevice()
{
    LOG_DEBUG("Picking physical device");

    m_vkPhysicalDevice   = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance.GetVkInstance(), &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("No GPUs found with  support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance.GetVkInstance(), &deviceCount, devices.data());

    for (const auto& device : devices)
    {
        if (IsDeviceSuitable(device))
        {
            m_vkPhysicalDevice = device;
            break;
        }
    }

    if (m_vkPhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable GPU for vulkan");
    }
}

bool Device::IsDeviceSuitable(const VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties {};
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature {};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.pNext = nullptr;

    VkPhysicalDeviceFeatures2 deviceFeatures {};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &dynamicRenderingFeature;

    vkGetPhysicalDeviceFeatures2(device, &deviceFeatures);

    auto dynamicRenderingSupported = dynamicRenderingFeature.dynamicRendering == VK_TRUE;

    auto featuresSupported = dynamicRenderingSupported;

    auto familyIndices       = FindQueueFamilies(device);
    auto extensionsSupported = CheckDeviceExtensionSupport(device);

    auto swapchainAdequate = false;
    if (extensionsSupported)
    {
        auto swapchainSupport = QuerySwapchainSupport(device);
        swapchainAdequate =
            !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
    }

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
           && familyIndices.IsComplete() && featuresSupported && extensionsSupported
           && swapchainAdequate;
}

QueueFamilyIndices Device::FindQueueFamilies(const VkPhysicalDevice device)
{
    QueueFamilyIndices familyIndices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    uint32_t i = 0;
    for (const auto& family : queueFamilies)
    {
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            familyIndices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_instance.GetSurface(), &presentSupport);

        if (presentSupport)
        {
            familyIndices.presentFamily = i;
        }

        if (familyIndices.IsComplete())
        {
            break;
        }

        i++;
    }

    return familyIndices;
}

bool Device::CheckDeviceExtensionSupport(const VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        device,
        nullptr,
        &extensionCount,
        availableExtensions.data()
    );

    std::set<std::string> uniqueRequired(m_requiredExtensions.begin(), m_requiredExtensions.end());
    for (const auto& extension : availableExtensions)
    {
        uniqueRequired.erase(extension.extensionName);
    }

    return uniqueRequired.empty();
}

SwapchainSupportDetails Device::QuerySwapchainSupport(const VkPhysicalDevice device)
{
    SwapchainSupportDetails details {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        device,
        m_instance.GetSurface(),
        &details.capabilities
    );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_instance.GetSurface(), &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device,
            m_instance.GetSurface(),
            &formatCount,
            details.formats.data()
        );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device,
        m_instance.GetSurface(),
        &presentModeCount,
        nullptr
    );
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device,
            m_instance.GetSurface(),
            &presentModeCount,
            details.presentModes.data()
        );
    }

    return details;
}

constexpr VkSurfaceFormatKHR Device::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& available
)
{
    for (const auto& fmt : available)
    {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB
            && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return fmt;
        }
    }

    return available[0];
}

constexpr VkPresentModeKHR Device::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& available
)
{
    for (const auto& mode : available)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

constexpr VkExtent2D Device::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width;
    int height;
    m_instance.GetFramebufferSize(&width, &height);

    VkExtent2D extent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
    };
    extent.width = std::clamp(
        extent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width
    );
    extent.height = std::clamp(
        extent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height
    );

    return extent;
}

void Device::CreateSwapchain()
{
    auto support       = QuerySwapchainSupport(m_vkPhysicalDevice);
    auto surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
    auto presentMode   = ChooseSwapPresentMode(support.presentModes);
    auto extent        = ChooseSwapExtent(support.capabilities);
    auto imageCount    = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
    {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = m_instance.GetSurface();
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform     = support.capabilities.currentTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = presentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    auto     families        = FindQueueFamilies(m_vkPhysicalDevice);
    uint32_t familyIndices[] = {families.graphicsFamily.value(), families.presentFamily.value()};

    if (families.graphicsFamily != families.presentFamily)
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = familyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VK_CHECK(
        vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_vkSwapchain),
        "Failed to create swapchain"
    );

    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &imageCount, nullptr);
    m_vkSwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &imageCount, m_vkSwapchainImages.data());

    m_vkSwapchainImageFormat = surfaceFormat.format;
    m_vkSwapchainExtent      = extent;

    CreateImage(
        &m_vkDepthImage,
        &m_vmaDepthAllocation,
        VK_IMAGE_TYPE_2D,
        GetDepthFormat(),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        extent.width,
        extent.height
    );
}

void Device::CreateImage(
    VkImage*          image,
    VmaAllocation*    imageAllocation,
    VkImageType       imageType,
    VkFormat          format,
    VkImageUsageFlags usage,
    uint32_t          width,
    uint32_t          height
)
{
    VkImageCreateInfo createInfo {};
    createInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType     = imageType;
    createInfo.format        = format;
    createInfo.extent.width  = width;
    createInfo.extent.height = height;
    createInfo.extent.depth  = 1;
    createInfo.mipLevels     = 1;
    createInfo.arrayLayers   = 1;
    createInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage         = usage;
    createInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    createInfo.flags         = 0;

    VmaAllocationCreateInfo allocInfo {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    vmaCreateImage(g_vma, &createInfo, &allocInfo, image, imageAllocation, nullptr);
}

void Device::CreateImageView(
    VkImage            image,
    VkImageView*       imageView,
    VkImageViewType    viewType,
    VkFormat           format,
    VkImageAspectFlags aspect
)
{
    VkImageViewCreateInfo createInfo {};
    createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image                           = image;
    createInfo.viewType                        = viewType;
    createInfo.format                          = format;
    createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask     = aspect;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    VK_CHECK(
        vkCreateImageView(m_vkDevice, &createInfo, nullptr, imageView),
        "Failed to create image view"
    );
}

void Device::CreateImageViews()
{
    m_vkSwapchainImageViews.resize(m_vkSwapchainImages.size());
    for (size_t i = 0; i < m_vkSwapchainImages.size(); i++)
    {
        CreateImageView(
            m_vkSwapchainImages[i],
            &m_vkSwapchainImageViews[i],
            VK_IMAGE_VIEW_TYPE_2D,
            m_vkSwapchainImageFormat,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
    }

    CreateImageView(
        m_vkDepthImage,
        &m_vkDepthImageView,
        VK_IMAGE_VIEW_TYPE_2D,
        GetDepthFormat(),
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
}

void Device::CreateLogicalDevice()
{
    LOG_DEBUG("Creating logical device");

    QueueFamilyIndices familyIndices = FindQueueFamilies(m_vkPhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t>                   uniqueQueueFamilies = {
        familyIndices.graphicsFamily.value(),
        familyIndices.presentFamily.value()
    };

    auto queuePriority = 1.0f;
    for (uint32_t familyIndex : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = familyIndex;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature {};
    dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature.dynamicRendering = VK_TRUE;
    dynamicRenderingFeature.pNext            = nullptr;

    VkPhysicalDeviceFeatures2 deviceFeatures {};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &dynamicRenderingFeature;

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures        = nullptr; // handled in pNext
    deviceCreateInfo.pNext                   = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(m_requiredExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = m_requiredExtensions.data();

    VK_CHECK(
        vkCreateDevice(m_vkPhysicalDevice, &deviceCreateInfo, nullptr, &m_vkDevice),
        "Failed to create logical  device"
    );

    m_vkGraphicsQueueIndex = familyIndices.graphicsFamily.value();
    m_vkPresentQueueIndex  = familyIndices.presentFamily.value();
    vkGetDeviceQueue(m_vkDevice, m_vkGraphicsQueueIndex, 0, &m_vkGraphicsQueue);
    vkGetDeviceQueue(m_vkDevice, m_vkPresentQueueIndex, 0, &m_vkPresentQueue);
}

void Device::CreateCommandPool()
{
    auto familyIndices = FindQueueFamilies(m_vkPhysicalDevice);

    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = familyIndices.graphicsFamily.value();

    VK_CHECK(
        vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool),
        "Failed to create command pool"
    );
}

void Device::CreateCommandBuffers()
{
    m_vkCommandBuffers.resize(m_maxFramesInFlight);

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = m_vkCommandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_maxFramesInFlight;

    VK_CHECK(
        vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkCommandBuffers.data()),
        "Failed to allocate command buffers"
    );
}

void Device::CreateSyncObjects()
{
    m_vkImageSemaphores.resize(m_maxFramesInFlight);
    m_vkRenderSemaphores.resize(m_maxFramesInFlight);
    m_vkInFlightFences.resize(m_maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < m_maxFramesInFlight; i++)
    {
        VK_CHECK(
            vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkImageSemaphores[i]),
            "Failed to create image semaphore"
        );
        VK_CHECK(
            vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkRenderSemaphores[i]),
            "Failed to create render semaphore"
        );
        VK_CHECK(
            vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_vkInFlightFences[i]),
            "Failed to create in flight fence"
        );
    }
}

void Device::CreateDescriptorPools()
{
    VkDescriptorPoolSize uboPoolSize {};
    uboPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboPoolSize.descriptorCount = m_maxFramesInFlight;

    VkDescriptorPoolCreateInfo uboPoolInfo {};
    uboPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    uboPoolInfo.poolSizeCount = 1;
    uboPoolInfo.pPoolSizes    = &uboPoolSize;
    uboPoolInfo.maxSets       = m_maxFramesInFlight;

    VK_CHECK(
        vkCreateDescriptorPool(m_vkDevice, &uboPoolInfo, nullptr, &m_vkUboDescriptorPool),
        "Failed to create ubo descriptor pool"
    );

    VkDescriptorPoolSize imGuiPoolSize {};
    imGuiPoolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    imGuiPoolSize.descriptorCount = m_maxFramesInFlight;

    VkDescriptorPoolCreateInfo imGuiPoolInfo {};
    imGuiPoolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    imGuiPoolInfo.poolSizeCount = 1;
    imGuiPoolInfo.pPoolSizes    = &imGuiPoolSize;
    imGuiPoolInfo.maxSets       = m_maxFramesInFlight;
    imGuiPoolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VK_CHECK(
        vkCreateDescriptorPool(m_vkDevice, &imGuiPoolInfo, nullptr, &m_vkImGuiDescriptorPool),
        "Failed to create ImGui descriptor pool"
    );
}
} // namespace legs
