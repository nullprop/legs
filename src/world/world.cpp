#include <memory>

#include <glm/ext/matrix_transform.hpp>

#include "../physics.hpp"

#include <legs/entity/sky.hpp>
#include <legs/geometry/icosphere.hpp>
#include <legs/log.hpp>
#include <legs/renderer/renderer.hpp>
#include <legs/world/world.hpp>

namespace legs
{

World::World(std::shared_ptr<Renderer> renderer) :
    m_renderer(renderer),
    m_physics(std::make_shared<Physics>())
{
    LOG_DEBUG("Creating World");
}

World::~World()
{
    LOG_DEBUG("Destroying World");
    m_physics.reset();
}

void World::Frame()
{
    for (auto ent : m_entities)
    {
        ent->OnFrame();
    }
}

void World::Tick()
{
    {
        m_physics->Update();

        std::scoped_lock worldLock {m_worldMutex};
        for (auto ent : m_entities)
        {
            ent->OnTick();
        }
    }
}

void World::Render()
{
    if (m_sky != nullptr)
    {
        m_sky->Render(m_renderer);
    }

    for (auto ent : m_entities)
    {
        if (auto meshEnt = std::static_pointer_cast<MeshEntity>(ent))
        {
            meshEnt->Render(m_renderer);
        }
    }
}

void World::AddEntity(std::shared_ptr<Entity> entity)
{
    m_entities.push_back(entity);
    entity->OnSpawn();
}

void World::RemoveEntity(std::shared_ptr<Entity> entity)
{
    for (auto it = m_entities.begin(); it != m_entities.end();)
    {
        if (*it == entity)
        {
            m_entities.erase(it);
            entity->OnDestroy();
            break;
        }
        it++;
    }
}
} // namespace legs
