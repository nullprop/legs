#include <cstdint>
#include <cstring>
#include <stdexcept>

#include <legs/log.hpp>
#include <legs/renderer/common.hpp>
#include <legs/renderer/instance.hpp>

namespace legs
{
constexpr static VKAPI_ATTR VkBool32 VKAPI_CALL _VkDebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData*/
)
{
    auto level = LogLevel::Debug;
    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            level = LogLevel::Debug;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            level = LogLevel::Info;
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            level = LogLevel::Warn;
            break;
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            level = LogLevel::Error;
            break;
    }

    _LOG(level, "validation layer: {}", pCallbackData->pMessage);

    return VK_FALSE;
}

constexpr static VkResult _CreateDebugUtilsMessengerEXT(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks*              pAllocator,
    VkDebugUtilsMessengerEXT*                 pDebugMessenger
)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT
    )vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

constexpr static void _DestroyDebugUtilsMessengerEXT(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     debugMessenger,
    const VkAllocationCallbacks* pAllocator
)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT
    )vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

Instance::Instance(std::shared_ptr<Window> window) : m_window(window)
{
    LOG_DEBUG("Creating Instance");

    VkApplicationInfo appInfo {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "legs";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "legs";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_3;

    if (m_enableValidationLayers && !ValidationLayersSupported())
    {
        throw std::runtime_error("Validation layers not available");
    }

    auto extensions = GetRequiredExtensions();

    VkInstanceCreateInfo createInfo {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (m_enableValidationLayers)
    {
        createInfo.enabledLayerCount   = static_cast<uint32_t>(m_validationLayers.size());
        createInfo.ppEnabledLayerNames = m_validationLayers.data();

        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext             = nullptr;
    }

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_vkInstance), "Failed to create instance");

    if (m_enableValidationLayers)
    {
        SetupDebugMessenger();
    }

    CreateSurface();
}

Instance::~Instance()
{
    LOG_DEBUG("Destroying Instance");

    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);

    if (m_enableValidationLayers)
    {
        _DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
    }

    vkDestroyInstance(m_vkInstance, nullptr);
}

void Instance::SetWindow(std::shared_ptr<Window> window)
{
    m_window = window;
    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
    CreateSurface();
}

std::vector<const char*> Instance::GetRequiredExtensions()
{
    unsigned int extensionCount = 0;
    if (!m_window->GetExtensions(&extensionCount, nullptr))
    {
        throw std::runtime_error("Failed to get vulkan extensions from window");
    }
    std::vector<const char*> extensions(extensionCount);
    if (!m_window->GetExtensions(&extensionCount, extensions.data()))
    {
        throw std::runtime_error("Failed to get vulkan extensions from window");
    }

    if (m_enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool Instance::ValidationLayersSupported()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const auto* layerName : m_validationLayers)
    {
        auto layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

void constexpr Instance::PopulateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo
)
{
    createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                 | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                             | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = _VkDebugMessengerCallback;
    createInfo.pUserData       = nullptr;
    createInfo.flags           = 0;
    createInfo.pNext           = nullptr;
}

void Instance::SetupDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo {};
    PopulateDebugMessengerCreateInfo(createInfo);

    VK_CHECK(
        _CreateDebugUtilsMessengerEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugMessenger),
        "Failed to set up debug messenger"
    );
}

void Instance::CreateSurface()
{
    auto result = m_window->CreateSurface(m_vkInstance, &m_vkSurface);
    if (!result)
    {
        throw std::runtime_error("Failed to create vulkan surface");
    }
}
} // namespace legs
