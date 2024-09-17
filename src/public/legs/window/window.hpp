#pragma once

#include <memory>

#include <SDL_syswm.h>
#include <vulkan/vulkan_core.h>

#include <legs/window/input.hpp>

namespace legs
{
class Window
{
  public:
    Window(std::shared_ptr<InputSettings> inputSettings);
    ~Window();

    void SetTitle(const char* title);
    void SetMouseGrab(bool grab);
    bool IsMouseGrabbed();

    void AggregateInput(WindowInput& input);

    void GetFramebufferSize(int* width, int* height);

    bool CreateSurface(VkInstance instance, VkSurfaceKHR* surface);

    bool GetExtensions(unsigned int* pCount, const char** pNames);

    bool IsMinimized();

    unsigned int GetRefreshRate();

    SDL_Window* GetSDLWindow() const
    {
        return m_window;
    }

  private:
    SDL_Window*                    m_window;
    std::shared_ptr<InputSettings> m_inputSettings;
};
} // namespace legs
