#pragma once

#include <memory>
#include <mutex>

#include <legs/iphysics.hpp>

#include <legs/entity/mesh_entity.hpp>
#include <legs/entity/sky.hpp>

namespace legs
{
class World
{
  public:
    World() = delete;
    World(std::shared_ptr<Renderer> renderer);
    ~World();

    World(const World&)            = delete;
    World(World&&)                 = delete;
    World& operator=(const World&) = delete;
    World& operator=(World&&)      = delete;

    void Frame();
    void Tick();
    void Render();

    void AddEntity(std::shared_ptr<Entity> entity);
    void RemoveEntity(std::shared_ptr<Entity> entity);

    void SetSky(std::shared_ptr<Sky> sky)
    {
        m_sky = sky;
    }

    std::shared_ptr<Sky> GetSky()
    {
        return m_sky;
    }

    std::shared_ptr<IPhysics> GetPhysics()
    {
        return m_physics;
    }

  private:
    std::mutex m_worldMutex;

    std::shared_ptr<Renderer> m_renderer;

    std::vector<std::shared_ptr<Entity>> m_entities;

    std::shared_ptr<Sky> m_sky;

    std::shared_ptr<IPhysics> m_physics;
};
} // namespace legs
