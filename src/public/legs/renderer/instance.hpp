#pragma once

#include <memory>
#include <vector>

#include <legs/window/window.hpp>

namespace legs
{
class Instance
{
  public:
    Instance(std::shared_ptr<Window> window);
    ~Instance();

    Instance(const Instance&)            = delete;
    Instance(Instance&&)                 = delete;
    Instance& operator=(const Instance&) = delete;
    Instance& operator=(Instance&&)      = delete;

    VkInstance GetVkInstance() const
    {
        return m_vkInstance;
    }

    VkSurfaceKHR GetSurface() const
    {
        return m_vkSurface;
    }

    void GetFramebufferSize(int* width, int* height) const
    {
        m_window->GetFramebufferSize(width, height);
    }

    void SetWindow(std::shared_ptr<Window> window);

  private:
    bool                     ValidationLayersSupported();
    std::vector<const char*> GetRequiredExtensions();
    void constexpr PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void SetupDebugMessenger();
    void CreateSurface();

    std::shared_ptr<Window> m_window;

    VkInstance               m_vkInstance;
    VkDebugUtilsMessengerEXT m_vkDebugMessenger;
    VkSurfaceKHR             m_vkSurface;

    const std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
    constexpr static bool m_enableValidationLayers = false;
#else
    constexpr static bool m_enableValidationLayers = true;
#endif
};
} // namespace legs
