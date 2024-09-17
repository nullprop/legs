#pragma once

#include <memory>

#include <legs/jolt_pch.hpp>

#include <legs/components/transform.hpp>

namespace legs
{
class IPhysics
{
  public:
    IPhysics()          = default;
    virtual ~IPhysics() = default;

    IPhysics(const IPhysics&)            = delete;
    IPhysics(IPhysics&&)                 = delete;
    IPhysics& operator=(const IPhysics&) = delete;
    IPhysics& operator=(IPhysics&&)      = delete;

    virtual void Update()   = 0;
    virtual void Optimize() = 0;

    virtual JPH::BodyID CreateBody(JPH::BodyCreationSettings settings) = 0;
    virtual void        AddBody(JPH::BodyID id)                        = 0;
    virtual void        RemoveBody(JPH::BodyID id)                     = 0;
    virtual void        DestroyBody(JPH::BodyID id)                    = 0;

    virtual void GetBodyTransform(JPH::BodyID id, std::shared_ptr<STransform> trans) = 0;
    virtual void SetBodyTransform(JPH::BodyID id, std::shared_ptr<STransform> trans) = 0;

    virtual void SetBodyPosition(JPH::BodyID id, glm::vec3 pos)        = 0;
    virtual void SetBodyRotation(JPH::BodyID id, glm::quat rot)        = 0;
    virtual void SetBodyVelocity(JPH::BodyID id, glm::vec3 vel)        = 0;
    virtual void SetBodyAngularVelocity(JPH::BodyID id, glm::vec3 vel) = 0;
};
}; // namespace legs
