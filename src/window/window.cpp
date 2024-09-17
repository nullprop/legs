#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <SDL.h>
#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>
#include <SDL_stdinc.h>
#include <SDL_video.h>
#include <SDL_vulkan.h>

#include <legs/log.hpp>
#include "legs/window/window.hpp"

namespace legs
{
Window::Window(std::shared_ptr<InputSettings> inputSettings) : m_inputSettings(inputSettings)
{
    LOG_INFO(
        "Creating window, SDL version: {}.{}.{}",
        SDL_MAJOR_VERSION,
        SDL_MINOR_VERSION,
        SDL_PATCHLEVEL
    );

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        LOG_ERROR("SDL could not initialize. SDL_Error: {}", SDL_GetError());
        throw("Failed to create Engine");
    }

    m_window = SDL_CreateWindow(
        "legs",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        1280,
        720,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );

    if (m_window == nullptr)
    {
        LOG_ERROR("Window could not be created. SDL_Error: {}", SDL_GetError());
        throw("Failed to create window");
    }

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(m_window, &wmi))
    {
        LOG_ERROR("SDL_SysWMinfo could not be retrieved. SDL_Error: {}", SDL_GetError());
        throw("Failed to create window");
    }

    LOG_DEBUG("Window created");
}

Window::~Window()
{
    LOG_INFO("Destroying window");
    if (m_window != nullptr)
    {
        SDL_DestroyWindow(m_window);
    }
    SDL_Quit();
}

void Window::SetTitle(const char* title)
{
    SDL_SetWindowTitle(m_window, title);
}

void Window::SetMouseGrab(bool grab)
{
    SDL_SetRelativeMouseMode(grab ? SDL_TRUE : SDL_FALSE);
}

bool Window::IsMouseGrabbed()
{
    return SDL_GetRelativeMouseMode() == SDL_TRUE;
}

void Window::AggregateInput(WindowInput& input)
{
    ImGuiIO& io             = ImGui::GetIO();
    bool     handleKeyboard = true;
    bool     handleMouse    = true;

    for (SDL_Event event; SDL_PollEvent(&event) != 0;)
    {
        if (event.type == SDL_QUIT)
        {
            input.wantsQuit = true;
            break;
        }

        ImGui_ImplSDL2_ProcessEvent(&event);
        handleKeyboard = !io.WantCaptureKeyboard;
        handleMouse    = !io.WantCaptureMouse;

        if (!handleMouse)
        {
            SetMouseGrab(false);
        }
        if (!IsMouseGrabbed())
        {
            handleKeyboard = false;
        }

        if (event.type == SDL_WINDOWEVENT)
        {
            switch (event.window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    input.wantsResize = true;
                    break;

                default:
                    break;
            }
        }

        if (handleKeyboard)
        {
            if (event.type == SDL_KEYDOWN)
            {
                auto key = m_inputSettings->GetKeyFromSDL(event.key.keysym.scancode);
                input.KeyDown(key);
            }

            if (event.type == SDL_KEYUP)
            {
                auto key = m_inputSettings->GetKeyFromSDL(event.key.keysym.scancode);
                input.KeyUp(key);
            }
        }

        if (handleMouse)
        {
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                switch (event.button.button)
                {
                    case 1:
                        SetMouseGrab(true);
                        break;

                    default:
                        break;
                }
            }

            if (IsMouseGrabbed())
            {
                if (event.type == SDL_MOUSEMOTION)
                {
                    input.mouse.x += event.motion.xrel;
                    input.mouse.y += event.motion.yrel;
                }
                if (event.type == SDL_MOUSEWHEEL)
                {
                    input.scroll.x += event.motion.xrel;
                    input.scroll.y += event.motion.yrel;
                }
            }
        }
    }
}

void Window::GetFramebufferSize(int* width, int* height)
{
    SDL_Vulkan_GetDrawableSize(m_window, width, height);
}

bool Window::CreateSurface(VkInstance instance, VkSurfaceKHR* surface)
{
    auto result = SDL_Vulkan_CreateSurface(m_window, instance, surface);
    return result == SDL_TRUE;
}

bool Window::GetExtensions(unsigned int* pCount, const char** pNames)
{
    auto result = SDL_Vulkan_GetInstanceExtensions(m_window, pCount, pNames);
    return result == SDL_TRUE;
}

bool Window::IsMinimized()
{
    return SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED;
}

unsigned int Window::GetRefreshRate()
{
    auto display = SDL_GetWindowDisplayIndex(m_window);
    if (display >= 0)
    {
        SDL_DisplayMode mode = {};
        if (SDL_GetDesktopDisplayMode(0, &mode) == 0)
        {
            return static_cast<unsigned int>(mode.refresh_rate);
        }
    }

    LOG_ERROR("Failed to get display mode, {}", SDL_GetError());
    return 60;
}
} // namespace legs
