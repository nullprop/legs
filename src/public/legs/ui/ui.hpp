#pragma once

#include <memory>

#include <glm/vec2.hpp>

#include <legs/renderer/renderer.hpp>
#include <legs/window/window.hpp>

namespace legs
{

enum class UIWindow
{
    DEBUG,
    DEMO,
    MAX
};

struct UIState
{
    bool showWindow[static_cast<unsigned int>(UIWindow::MAX)];
};

class UI
{
  public:
    UI()                     = delete;
    UI(const UI&)            = delete;
    UI(UI&&)                 = delete;
    UI& operator=(const UI&) = delete;
    UI& operator=(UI&&)      = delete;

    UI(std::shared_ptr<Window> window, std::shared_ptr<Renderer> renderer);
    ~UI();

    void ToggleWindow(UIWindow window)
    {
        auto index                = static_cast<unsigned int>(window);
        m_state.showWindow[index] = !m_state.showWindow[index];
    }

    void Render();

  private:
    void DebugWindow();
    void DemoWindow();

    std::shared_ptr<Window>   m_window;
    std::shared_ptr<Renderer> m_renderer;
    ImGuiCreationInfo         m_info;
    UIState                   m_state;
};
}; // namespace legs
