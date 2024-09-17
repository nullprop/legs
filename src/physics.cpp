#include <cmath>
#include <cstdarg>
#include <iostream>
#include <thread>

#include <legs/time.hpp>

#include "physics.hpp"

namespace legs
{

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char* fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, list);
    va_end(list);

    LOG_ERROR("JOLT TRACE: {}", buffer);
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(
    const char* inExpression,
    const char* inMessage,
    const char* inFile,
    uint        inLine
)
{
    Log::Print(inFile, inLine, inExpression, LogLevel::Error, inMessage);

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS

void Physics::Register()
{
    // Register allocation hook. In this example we'll just let Jolt use malloc / free but you can
    // override these if you want (see Memory.h). This needs to be done before any other Jolt
    // function is called.
    JPH::RegisterDefaultAllocator();

    // Install trace and assert callbacks
    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

    // Create a factory, this class is responsible for creating instances of classes based on their
    // name or hash and is mainly used for deserialization of saved data. It is not directly used in
    // this example but still required.
    JPH::Factory::sInstance = new JPH::Factory();

    // Register all physics types with the factory and install their collision handlers with the
    // CollisionDispatch class. If you have your own custom shape types you probably need to
    // register their handlers with the CollisionDispatch before calling this function. If you
    // implement your own default material (PhysicsMaterial::sDefault) make sure to initialize it
    // before this function or else this function will create one for you.
    JPH::RegisterTypes();
}

Physics::Physics() :
    m_tempAllocator(10 * 1024 * 1024),
    m_jobSystem(
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        static_cast<int>(std::thread::hardware_concurrency()) - 1
    ),
    m_maxDeltaTime(1.0f / 60.0f)
{
    // This is the max amount of rigid bodies that you can add to the physics system. If you try to
    // add more you'll get an error. Note: This value is low because this is a simple test. For a
    // real project use something in the order of 65536.
    const uint cMaxBodies = 1024;

    // This determines how many mutexes to allocate to protect rigid bodies from concurrent access.
    // Set it to 0 for the default settings.
    const uint cNumBodyMutexes = 0;

    // This is the max amount of body pairs that can be queued at any time (the broad phase will
    // detect overlapping body pairs based on their bounding boxes and will insert them into a queue
    // for the narrowphase). If you make this buffer too small the queue will fill up and the broad
    // phase jobs will start to do narrow phase work. This is slightly less efficient. Note: This
    // value is low because this is a simple test. For a real project use something in the order of
    // 65536.
    const uint cMaxBodyPairs = 1024;

    // This is the maximum size of the contact constraint buffer. If more contacts (collisions
    // between bodies) are detected than this number then these contacts will be ignored and bodies
    // will start interpenetrating / fall through the world. Note: This value is low because this is
    // a simple test. For a real project use something in the order of 10240.
    const uint cMaxContactConstraints = 1024;

    // Now we can create the actual physics system.
    m_physicsSystem.Init(
        cMaxBodies,
        cNumBodyMutexes,
        cMaxBodyPairs,
        cMaxContactConstraints,
        m_broadPhaseLayerInterface,
        m_objectVsBroadphaseLayerFilter,
        m_objectVsObjectLayerFilter
    );

    // A body activation listener gets notified when bodies activate and go to sleep
    // Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    m_physicsSystem.SetBodyActivationListener(&m_bodyActivationListener);

    // A contact listener gets notified when bodies (are about to) collide, and when they separate
    // again. Note that this is called from a job so whatever you do here needs to be thread safe.
    // Registering one is entirely optional.
    m_physicsSystem.SetContactListener(&m_contactListener);
}

Physics::~Physics()
{
    // Unregisters all types with the factory and cleans up the default material
    JPH::UnregisterTypes();

    // Destroy the factory
    if (JPH::Factory::sInstance != nullptr)
    {
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }
}

void Physics::Optimize()
{
    m_physicsSystem.OptimizeBroadPhase();
}

void Physics::Update()
{
    const unsigned int steps = std::ceil(Time::DeltaTick / m_maxDeltaTime);
    m_physicsSystem.Update(Time::DeltaTick, steps, &m_tempAllocator, &m_jobSystem);
}

JPH::BodyID Physics::CreateBody(JPH::BodyCreationSettings settings)
{
    auto body = m_physicsSystem.GetBodyInterface().CreateBody(settings);
    if (body != nullptr)
    {
        return body->GetID();
    }
    return JPH::BodyID(JPH::BodyID::cInvalidBodyID);
}

void Physics::AddBody(JPH::BodyID id)
{
    m_physicsSystem.GetBodyInterface().AddBody(id, JPH::EActivation::Activate);
}

void Physics::RemoveBody(JPH::BodyID id)
{
    m_physicsSystem.GetBodyInterface().RemoveBody(id);
}

void Physics::DestroyBody(JPH::BodyID id)
{
    m_physicsSystem.GetBodyInterface().DestroyBody(id);
}

void Physics::GetBodyTransform(JPH::BodyID id, std::shared_ptr<STransform> trans)
{
    JPH::RVec3 joltPos;
    JPH::Quat  joltRot;

    m_physicsSystem.GetBodyInterface().GetPositionAndRotation(id, joltPos, joltRot);

    trans->position.x = joltPos.GetX();
    trans->position.y = joltPos.GetY();
    trans->position.z = joltPos.GetZ();

    trans->rotation.quaternion.x = joltRot.GetX();
    trans->rotation.quaternion.y = joltRot.GetY();
    trans->rotation.quaternion.z = joltRot.GetZ();
    trans->rotation.quaternion.w = joltRot.GetW();
}

void Physics::SetBodyTransform(JPH::BodyID id, std::shared_ptr<STransform> trans)
{
    JPH::RVec3 joltPos = {trans->position.x, trans->position.y, trans->position.z};
    JPH::Quat  joltRot = {
        trans->rotation.quaternion.x,
        trans->rotation.quaternion.y,
        trans->rotation.quaternion.w,
        trans->rotation.quaternion.z
    };

    m_physicsSystem.GetBodyInterface()
        .SetPositionAndRotation(id, joltPos, joltRot, JPH::EActivation::Activate);
}

void Physics::SetBodyPosition(JPH::BodyID id, glm::vec3 pos)
{
    JPH::RVec3 joltPos = {pos.x, pos.y, pos.z};
    m_physicsSystem.GetBodyInterface().SetPosition(id, joltPos, JPH::EActivation::Activate);
}

void Physics::SetBodyRotation(JPH::BodyID id, glm::quat rot)
{
    JPH::Quat joltRot = {rot.x, rot.y, rot.w, rot.z};
    m_physicsSystem.GetBodyInterface().SetRotation(id, joltRot, JPH::EActivation::Activate);
}

void Physics::SetBodyVelocity(JPH::BodyID id, glm::vec3 vel)
{
    JPH::RVec3 joltVel = {vel.x, vel.y, vel.z};
    m_physicsSystem.GetBodyInterface().SetPosition(id, joltVel, JPH::EActivation::Activate);
}

void Physics::SetBodyAngularVelocity(JPH::BodyID id, glm::vec3 vel)
{
    JPH::RVec3 joltVel = {vel.x, vel.y, vel.z};
    m_physicsSystem.GetBodyInterface().SetPosition(id, joltVel, JPH::EActivation::Activate);
}
}; // namespace legs
