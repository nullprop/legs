#pragma once

#include <memory>
#include <string>

#include <legs/components/transform.hpp>

namespace legs
{
class Entity
{
  public:
    Entity() : Name(""), Transform(std::make_shared<STransform>()) {};
    virtual ~Entity() = default;

    Entity(const Entity&)            = delete;
    Entity(Entity&&)                 = delete;
    Entity& operator=(const Entity&) = delete;
    Entity& operator=(Entity&&)      = delete;

    virtual void OnSpawn() {};
    virtual void OnDestroy() {};
    virtual void OnFrame() {};
    virtual void OnTick() {};

    virtual void SetPosition(glm::vec3 pos)
    {
        Transform->position = pos;
    }

    virtual void SetRotation(glm::quat rot)
    {
        Transform->rotation.quaternion = rot;
    }

    virtual void SetVelocity(glm::vec3 vel)
    {
        Transform->velocity = vel;
    }

    virtual void SetAngularVelocity(glm::vec3 vel)
    {
        Transform->angularVelocity = vel;
    }

    virtual std::shared_ptr<STransform> GetTransform()
    {
        return Transform;
    }

    virtual glm::vec3 GetPosition()
    {
        return Transform->position;
    }

    virtual glm::quat GetRotation()
    {
        return Transform->rotation.quaternion;
    }

    virtual glm::vec3 GetVelocity()
    {
        return Transform->velocity;
    }

    virtual glm::vec3 GetAngularVelocity()
    {
        return Transform->angularVelocity;
    }

  protected:
    std::string                 Name;
    std::shared_ptr<STransform> Transform;
};
}; // namespace legs
