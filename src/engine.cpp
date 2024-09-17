#include <functional>
#include <memory>
#include <stop_token>
#include <thread>

#include <imgui.h>

#include "physics.hpp"

#include "legs/engine.hpp"
#include "legs/log.hpp"
#include "legs/time.hpp"
#include "legs/window/window.hpp"

namespace legs
{
Engine::Engine()
{
    LOG_INFO("Creating Engine");

    m_inputSettings = std::make_shared<InputSettings>();
    m_window        = std::make_shared<Window>(m_inputSettings);
    m_renderer      = std::make_shared<Renderer>(m_window);

    int width;
    int height;
    m_window->GetFramebufferSize(&width, &height);
    m_camera = std::make_shared<Camera>(width, height);

    m_ui = std::make_unique<UI>(m_window, m_renderer);

    Physics::Register();
    m_world = std::make_shared<World>(m_renderer);

    m_window->SetMouseGrab(true);

    auto fps = m_window->GetRefreshRate();
    LOG_DEBUG("Setting framerate to {}", fps);
    Time::SetFrameRate(fps);

    Time::SetStart();

    m_frameInput.Clear();
    m_tickInput.Clear();

    m_tickThread   = std::jthread {std::bind_front(&Engine::TickThread, this)};
    m_renderThread = std::jthread {std::bind_front(&Engine::RenderThread, this)};
}

Engine::~Engine()
{
    LOG_INFO("Destroying Engine");

    LOG_DEBUG("Requesting TickThread stop");
    m_tickThread.request_stop();
    m_threadTickSemaphore.release();
    m_tickThread.join();

    LOG_DEBUG("Requesting RenderThread stop");
    m_renderThread.request_stop();
    m_threadFrameSemaphore.release();
    m_renderThread.join();

    LOG_DEBUG("Waiting for renderer idle");
    m_renderer->WaitForIdle();
}

int Engine::Run()
{
    m_mainFrameSemaphore.release();
    m_mainTickSemaphore.release();

    Time::UpdateTickDelta();
    Time::UpdateFrameDelta();

    const double sleepThreshold = 0.0001;
    while (true)
    {
        auto toTick = Time::TimeToEngineTick();
        if (toTick <= 0)
        {
            if (!Tick())
            {
                LOG_INFO("Engine::Tick exit");
                break;
            }
            toTick = Time::TickInterval;
        }

        auto toFrame = Time::TimeToEngineFrame();
        if (toFrame <= 0)
        {
            Frame();
            toFrame = Time::FrameInterval;
        }

        const auto lowest = std::min(toTick, toFrame);
        if (lowest < sleepThreshold)
        {
            std::this_thread::yield();
        }
        else
        {
            std::this_thread::sleep_for(Time::Duration(lowest - sleepThreshold));
        }
    }

    return 0;
}

void Engine::Frame()
{
    // Renderer still busy?
    auto acquired = m_mainFrameSemaphore.try_acquire();
    if (!acquired)
    {
        return;
    }

    Time::UpdateFrameDelta();

    UpdateInput();

    for (auto system : m_systems)
    {
        system->OnFrame();
    }

    if (m_frameInput.wantsResize)
    {
        int width;
        int height;
        m_window->GetFramebufferSize(&width, &height);
        m_camera->UpdateViewport(width, height);
        m_renderer->Resize();
    }

    if (m_world != nullptr)
    {

        m_world->Frame();
    }

    if (m_frameInput.HasKey(Key::KEY_MOUSE_GRAB))
    {
        m_window->SetMouseGrab(!m_window->IsMouseGrabbed());
        m_frameInput.Clear(true);
    }

    if (m_frameInput.HasKey(Key::KEY_WINDOW_DEBUG))
    {
        m_ui->ToggleWindow(UIWindow::DEBUG);
        m_frameInput.KeyUp(Key::KEY_WINDOW_DEBUG);
    }

    if (m_frameInput.HasKey(Key::KEY_WINDOW_DEMO))
    {
        m_window->SetMouseGrab(false);
        m_frameInput.Clear();
        m_ui->ToggleWindow(UIWindow::DEMO);
    }

    m_frameInput.Clear();

    // Allow render thread to run.
    m_threadFrameSemaphore.release();
}

bool Engine::Tick()
{
    if (m_tickInput.wantsQuit)
    {
        return false;
    }

    // Tick thread still busy?
    auto acquired = m_mainTickSemaphore.try_acquire();
    if (!acquired)
    {
        return true;
    }

    const auto delta         = Time::TimeSinceEngineTick();
    const auto slowThreshold = Time::TickInterval * 2.0;
    if (delta > slowThreshold)
    {
        LOG_WARN("Tick thread ran slow: {:.2f}ms", 1000 * Time::TimeSinceEngineTick());
    }

    Time::UpdateTickDelta();

    // Allow tick thread to run.
    m_threadTickSemaphore.release();

    return true;
}

void Engine::UpdateInput()
{
    m_window->AggregateInput(m_frameInput);
    m_tickInput.Aggregate(m_frameInput);
}

void Engine::TickThread(const std::stop_token token)
{
    LOG_INFO("Enter TickThread");

    while (!token.stop_requested())
    {
        // Wait for main thread.
        m_threadTickSemaphore.acquire();

        for (auto system : m_systems)
        {
            system->OnTick();
        }

        if (m_world != nullptr)
        {
            m_world->Tick();
        }

        m_tickInput.Clear();

        // Let main thread know we are done.
        m_mainTickSemaphore.release();
    }
}

void Engine::RenderThread(const std::stop_token token)
{
    LOG_INFO("Enter RenderThread");

    while (!token.stop_requested())
    {
        // Wait for main thread.
        m_threadFrameSemaphore.acquire();

        Time::StartRender();

        if (m_window->IsMinimized())
        {
            Time::StopRender();
            m_mainFrameSemaphore.release();
            continue;
        }

        m_renderer->Begin();

        auto ubo = m_renderer->GetUBO();

        ubo->sunColor = {};
        ubo->sunDir   = {};

        if (m_world != nullptr)
        {
            if (auto sky = m_world->GetSky())
            {
                ubo->sunDir   = sky->SunDirection;
                ubo->sunColor = sky->SunColor;
            }
        }

        if (m_camera != nullptr)
        {
            ubo->SetCamera(m_camera);
        }

        m_renderer->UpdateUBO();

        if (m_world != nullptr)
        {
            m_world->Render();
        }

        m_ui->Render();

        m_renderer->Submit();

        m_renderer->Present();

        Time::StopRender();

        // Let main thread know we are done.
        m_mainFrameSemaphore.release();
    }

    LOG_INFO("Exit RenderThread");
}
} // namespace legs
