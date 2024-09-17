#include <memory>

#include <legs/entry.hpp>

#include <legs/entity/physics_entity.hpp>
#include <legs/geometry/icosphere.hpp>
#include <legs/geometry/plane.hpp>
#include <legs/isystem.hpp>
#include <legs/log.hpp>
#include <legs/time.hpp>

using namespace legs;

class MySystem : public ISystem
{
  public:
    MySystem()
    {
        int width;
        int height;
        g_engine->GetWindow()->GetFramebufferSize(&width, &height);
        m_camera = std::make_shared<NoclipCamera>(width, height);
        m_camera->SetPosition({0.0f, -10.0f, 5.0f});
        g_engine->SetCamera(m_camera);

        auto renderer = g_engine->GetRenderer();
        auto world    = g_engine->GetWorld();

        // Create a plane
        std::shared_ptr<Buffer> planeVertexBuffer;
        std::shared_ptr<Buffer> planeIndexBuffer;

        auto testPlane     = SPlane({0.0f, 0.0f, 0.0f}, 20.0f);
        auto planeVertices = std::array<Vertex_P_C, 4>();
        for (unsigned int i = 0; i < 4; i++)
        {
            auto color = glm::vec3 {
                i == 0 ? 1.0f : 0.0f,
                i == 1 ? 1.0f : 0.0f,
                i == 2 ? 1.0f : 0.0f,
            };
            planeVertices[i] = {testPlane.vertices[i], color};
        }
        renderer->CreateBuffer(
            planeVertexBuffer,
            VertexBuffer,
            planeVertices.data(),
            sizeof(Vertex_P_C),
            static_cast<uint32_t>(planeVertices.size())
        );
        renderer->CreateBuffer(
            planeIndexBuffer,
            IndexBuffer,
            testPlane.indices.data(),
            sizeof(Index),
            static_cast<uint32_t>(testPlane.indices.size())
        );

        auto plane = std::make_shared<PhysicsEntity>();
        plane->SetBuffers(planeVertexBuffer, planeIndexBuffer);
        plane->SetPipeline(RenderPipeline::GEO_P_C);

        auto planeCollider = BoxCollider(
            JPH::EMotionType::Static,
            Layers::NON_MOVING,
            plane->GetTransform(),
            {20.0f, 20.0f, 0.1f}
        );
        plane->SetCollider(planeCollider);

        world->AddEntity(plane);

        // Create a sphere
        auto                    testSphere = SIcosphere(glm::vec3(0.0f, 0.0f, 10.0f), 0.5f, 1);
        std::shared_ptr<Buffer> sphereVertexBuffer;
        std::shared_ptr<Buffer> sphereIndexBuffer;

        std::vector<Vertex_P_N_C> sphereVertices;
        sphereVertices.reserve(testSphere.positions.size());
        for (unsigned int i = 0; i < testSphere.positions.size(); i++)
        {
            sphereVertices.push_back(
                {testSphere.positions[i], testSphere.normals[i], glm::vec3(0.5, 0.5, 0.5)}
            );
        }
        renderer->CreateBuffer(
            sphereVertexBuffer,
            VertexBuffer,
            sphereVertices.data(),
            sizeof(Vertex_P_N_C),
            static_cast<uint32_t>(sphereVertices.size())
        );
        renderer->CreateBuffer(
            sphereIndexBuffer,
            IndexBuffer,
            testSphere.indices.data(),
            sizeof(Index),
            static_cast<uint32_t>(testSphere.indices.size())
        );

        auto sphere = std::make_shared<PhysicsEntity>();
        sphere->SetBuffers(sphereVertexBuffer, sphereIndexBuffer);
        sphere->SetPipeline(RenderPipeline::GEO_P_N_C);

        auto sphereCollider =
            SphereCollider(JPH::EMotionType::Dynamic, Layers::MOVING, sphere->GetTransform(), 0.5f);
        sphere->SetCollider(sphereCollider);

        world->AddEntity(sphere);
    }

    ~MySystem()
    {
    }

    void OnFrame() override
    {
        m_camera->HandleInput(g_engine->GetFrameInput());
    }

  private:
    std::shared_ptr<NoclipCamera> m_camera;
};

int main(int argc, char** argv)
{
    Log::SetLogLevel(LogLevel::Debug);

    auto code = LEGS_Init(argc, argv);
    if (code < 0)
    {
        return code;
    }

    g_engine->GetWindow()->SetTitle("03_systems");

    g_engine->AddSystem(std::make_shared<MySystem>());

    return LEGS_Run();
}
