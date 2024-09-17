#pragma once

#include <memory>
#include <semaphore>
#include <stop_token>
#include <thread>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <legs/world/world.hpp>

#include <legs/isystem.hpp>
#include <legs/renderer/renderer.hpp>
#include <legs/ui/ui.hpp>
#include <legs/window/input.hpp>
#include <legs/window/window.hpp>

namespace legs
{
class Engine
{
  public:
    Engine();
    ~Engine();

    int Run();

    std::shared_ptr<Window> GetWindow() const
    {
        return m_window;
    }

    void AddSystem(std::shared_ptr<ISystem> system)
    {
        m_systems.push_back(system);
    }

    std::shared_ptr<Renderer> GetRenderer()
    {
        return m_renderer;
    }

    std::shared_ptr<World> GetWorld()
    {
        return m_world;
    }

    std::shared_ptr<Camera> GetCamera()
    {
        return m_camera;
    }

    void SetCamera(std::shared_ptr<Camera> camera)
    {
        m_camera = camera;
    }

    WindowInput GetFrameInput()
    {
        return m_frameInput;
    }

    WindowInput GetTickInput()
    {
        return m_tickInput;
    }

  private:
    void Frame();
    bool Tick();

    void UpdateInput();

    void TickThread(const std::stop_token token);
    void RenderThread(const std::stop_token token);

    std::shared_ptr<InputSettings> m_inputSettings;
    std::shared_ptr<Window>        m_window;
    std::shared_ptr<Camera>        m_camera;
    std::shared_ptr<Renderer>      m_renderer;
    std::shared_ptr<World>         m_world;
    std::unique_ptr<UI>            m_ui;

    WindowInput m_frameInput;
    WindowInput m_tickInput;

    std::jthread m_tickThread;
    std::jthread m_renderThread;

    std::binary_semaphore m_mainTickSemaphore {0};
    std::binary_semaphore m_threadTickSemaphore {0};

    std::binary_semaphore m_mainFrameSemaphore {0};
    std::binary_semaphore m_threadFrameSemaphore {0};

    std::vector<std::shared_ptr<ISystem>> m_systems;
};
} // namespace legs
