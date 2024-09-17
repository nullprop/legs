#pragma once

#include <memory>
#include <stdexcept>

#include <legs/jolt_pch.hpp>

#include <legs/components/transform.hpp>

namespace legs
{
namespace Layers
{
static constexpr JPH::ObjectLayer NON_MOVING = 0;
static constexpr JPH::ObjectLayer MOVING     = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}; // namespace Layers

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2)
        const override
    {
        switch (inObject1)
        {
            case Layers::NON_MOVING:
                return inObject2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

class ICollider
{
  public:
    ICollider() = default;

    virtual ~ICollider() = default;

    ICollider(const ICollider&)            = default;
    ICollider(ICollider&&)                 = default;
    ICollider& operator=(const ICollider&) = default;
    ICollider& operator=(ICollider&&)      = default;

    void CreateBody(std::shared_ptr<STransform> trans)
    {
        JPH::ShapeSettings::ShapeResult shapeResult = ShapeSettings->Create();
        if (shapeResult.HasError())
        {
            throw std::runtime_error(std::format("Jolt: {}", shapeResult.GetError()));
        }
        JPH::ShapeRefC shape = shapeResult.Get();

        CreationSettings = JPH::BodyCreationSettings(
            shape,
            JPH::RVec3(trans->position.x, trans->position.y, trans->position.z),
            JPH::Quat(
                trans->rotation.quaternion.x,
                trans->rotation.quaternion.y,
                trans->rotation.quaternion.z,
                trans->rotation.quaternion.w
            ),
            MotionType,
            Layer
        );
    }

    JPH::EMotionType                    MotionType;
    JPH::ObjectLayer                    Layer;
    JPH::BodyCreationSettings           CreationSettings;
    std::shared_ptr<JPH::ShapeSettings> ShapeSettings;
};

class BoxCollider final : public ICollider
{
  public:
    BoxCollider(
        JPH::EMotionType            motionType,
        JPH::ObjectLayer            layer,
        std::shared_ptr<STransform> trans,
        glm::vec3                   size
    )
    {
        MotionType    = motionType;
        Layer         = layer;
        ShapeSettings = std::make_shared<JPH::BoxShapeSettings>(JPH::Vec3(size.x, size.y, size.z));
        CreateBody(trans);
    }

    ~BoxCollider()
    {
    }
};

class SphereCollider final : public ICollider
{
  public:
    SphereCollider(
        JPH::EMotionType            motionType,
        JPH::ObjectLayer            layer,
        std::shared_ptr<STransform> trans,
        float                       radius
    )
    {
        MotionType    = motionType;
        Layer         = layer;
        ShapeSettings = std::make_shared<JPH::SphereShapeSettings>(radius);
        CreateBody(trans);
    }

    ~SphereCollider()
    {
    }
};
}; // namespace legs
