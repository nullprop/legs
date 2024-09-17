#pragma once

#include <legs/collider.hpp>
#include <legs/iphysics.hpp>
#include <legs/log.hpp>

#include "job_system_thread_pool.hpp"

namespace legs
{
// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least
// want to have a layer for non-moving and moving objects to avoid having to update a tree full of
// static objects every frame. You can have a 1-on-1 mapping between object layers and broadphase
// layers (like in this case) but if you have many object layers you'll be creating many broad phase
// trees, which is not efficient. If you want to fine tune your broadphase layers define
// JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers
{
static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
static constexpr JPH::BroadPhaseLayer MOVING(1);
static constexpr uint                 NUM_LAYERS(2);
}; // namespace BroadPhaseLayers

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
  public:
    BPLayerInterfaceImpl()
    {
        // Create a mapping table from object to broad phase layer
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING]     = BroadPhaseLayers::MOVING;
    }

    virtual uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
        return mObjectToBroadPhase[inLayer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)inLayer)
        {
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
                return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
                return "MOVING";
            default:
                JPH_ASSERT(false);
                return "INVALID";
        }
    }
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

  private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
  public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2)
        const override
    {
        switch (inLayer1)
        {
            case Layers::NON_MOVING:
                return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                JPH_ASSERT(false);
                return false;
        }
    }
};

// An example contact listener
class MyContactListener : public JPH::ContactListener
{
  public:
    // See: ContactListener
    virtual JPH::ValidateResult OnContactValidate(
        const JPH::Body&               inBody1,
        const JPH::Body&               inBody2,
        JPH::RVec3Arg                  inBaseOffset,
        const JPH::CollideShapeResult& inCollisionResult
    ) override
    {
        // std::cout << "Contact validate callback" << std::endl;

        // Allows you to ignore a contact before it is created (using layers to not make objects
        // collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    virtual void OnContactAdded(
        const JPH::Body&            inBody1,
        const JPH::Body&            inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings&       ioSettings
    ) override
    {
        // std::cout << "A contact was added" << std::endl;
    }

    virtual void OnContactPersisted(
        const JPH::Body&            inBody1,
        const JPH::Body&            inBody2,
        const JPH::ContactManifold& inManifold,
        JPH::ContactSettings&       ioSettings
    ) override
    {
        // std::cout << "A contact was persisted" << std::endl;
    }

    virtual void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override
    {
        // std::cout << "A contact was removed" << std::endl;
    }
};

// An example activation listener
class MyBodyActivationListener : public JPH::BodyActivationListener
{
  public:
    virtual void OnBodyActivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData) override
    {
        // std::cout << "A body got activated" << std::endl;
    }

    virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, uint64_t inBodyUserData) override
    {
        // std::cout << "A body went to sleep" << std::endl;
    }
};

class Physics final : public IPhysics
{
  public:
    static void Register();

    Physics();
    ~Physics();

    Physics(const Physics&)            = delete;
    Physics(Physics&&)                 = delete;
    Physics& operator=(const Physics&) = delete;
    Physics& operator=(Physics&&)      = delete;

    void Optimize() override;
    void Update() override;

    JPH::BodyID CreateBody(JPH::BodyCreationSettings settings) override;
    void        AddBody(JPH::BodyID id) override;
    void        RemoveBody(JPH::BodyID id) override;
    void        DestroyBody(JPH::BodyID id) override;

    void GetBodyTransform(JPH::BodyID id, std::shared_ptr<STransform> trans) override;
    void SetBodyTransform(JPH::BodyID id, std::shared_ptr<STransform> trans) override;

    void SetBodyPosition(JPH::BodyID id, glm::vec3 pos) override;
    void SetBodyRotation(JPH::BodyID id, glm::quat rot) override;
    void SetBodyVelocity(JPH::BodyID id, glm::vec3 vel) override;
    void SetBodyAngularVelocity(JPH::BodyID id, glm::vec3 vel) override;

  private:
    JPH::PhysicsSystem                m_physicsSystem;
    JPH::TempAllocatorImpl            m_tempAllocator;
    JobSystemThreadPool               m_jobSystem;
    BPLayerInterfaceImpl              m_broadPhaseLayerInterface;
    ObjectVsBroadPhaseLayerFilterImpl m_objectVsBroadphaseLayerFilter;
    ObjectLayerPairFilterImpl         m_objectVsObjectLayerFilter;
    MyContactListener                 m_contactListener;
    MyBodyActivationListener          m_bodyActivationListener;
    float                             m_maxDeltaTime;
};
}; // namespace legs
