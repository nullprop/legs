#pragma once

#include <memory>

#include <legs/collider.hpp>
#include <legs/iphysics.hpp>

#include <legs/entity/mesh_entity.hpp>
#include <legs/entry.hpp>

namespace legs
{
class PhysicsEntity : public MeshEntity
{
  public:
    PhysicsEntity() : MeshEntity()
    {
    }

    virtual ~PhysicsEntity() = default;

    PhysicsEntity(const PhysicsEntity&)            = delete;
    PhysicsEntity(PhysicsEntity&&)                 = delete;
    PhysicsEntity& operator=(const PhysicsEntity&) = delete;
    PhysicsEntity& operator=(PhysicsEntity&&)      = delete;

    virtual void OnSpawn() override
    {
        MeshEntity::OnSpawn();
        m_joltBody = g_engine->GetWorld()->GetPhysics()->CreateBody(m_collider.CreationSettings);
        g_engine->GetWorld()->GetPhysics()->AddBody(m_joltBody);
    }

    virtual void OnDestroy() override
    {
        MeshEntity::OnDestroy();
        g_engine->GetWorld()->GetPhysics()->RemoveBody(m_joltBody);
        g_engine->GetWorld()->GetPhysics()->DestroyBody(m_joltBody);
    }

    virtual void OnFrame() override
    {
        MeshEntity::OnFrame();
    }

    virtual void OnTick() override
    {
        MeshEntity::OnTick();
        g_engine->GetWorld()->GetPhysics()->GetBodyTransform(m_joltBody, Transform);
    }

    virtual void SetPosition(glm::vec3 pos) override
    {
        MeshEntity::SetPosition(pos);
        g_engine->GetWorld()->GetPhysics()->SetBodyPosition(m_joltBody, pos);
    }

    virtual void SetRotation(glm::quat rot) override
    {
        MeshEntity::SetRotation(rot);
        g_engine->GetWorld()->GetPhysics()->SetBodyRotation(m_joltBody, rot);
    }

    virtual void SetVelocity(glm::vec3 vel) override
    {
        MeshEntity::SetVelocity(vel);
        g_engine->GetWorld()->GetPhysics()->SetBodyVelocity(m_joltBody, vel);
    }

    virtual void SetAngularVelocity(glm::vec3 vel) override
    {
        MeshEntity::SetAngularVelocity(vel);
        g_engine->GetWorld()->GetPhysics()->SetBodyAngularVelocity(m_joltBody, vel);
    }

    virtual void SetCollider(ICollider collider)
    {
        m_collider = collider;
    }

  protected:
    JPH::BodyID m_joltBody;
    ICollider   m_collider;
};
}; // namespace legs
