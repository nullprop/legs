#pragma once

#include <memory>

#include <legs/entity/camera.hpp>
#include <legs/entity/mesh_entity.hpp>
#include <legs/geometry/icosphere.hpp>
#include <legs/renderer/buffer.hpp>
#include <legs/renderer/mesh_data.hpp>
#include <legs/renderer/renderer.hpp>

namespace legs
{
class Sky : public MeshEntity
{
  public:
    Sky(std::shared_ptr<Renderer> renderer) : MeshEntity()
    {
        auto icosphere = SIcosphere({}, CAM_FAR * 0.95f, 1, true);

        std::vector<Vertex_P> vertices;
        vertices.reserve(icosphere.positions.size());
        for (unsigned int i = 0; i < icosphere.positions.size(); i++)
        {
            vertices.push_back({icosphere.positions[i]});
        }

        renderer->CreateBuffer(
            m_vertexBuffer,
            VertexBuffer,
            vertices.data(),
            sizeof(Vertex_P),
            static_cast<uint32_t>(vertices.size())
        );
        renderer->CreateBuffer(
            m_indexBuffer,
            IndexBuffer,
            icosphere.indices.data(),
            sizeof(Index),
            static_cast<uint32_t>(icosphere.indices.size())
        );

        m_pipeline = RenderPipeline::SKY;

        SunDirection = glm::vec3(0, 0, 1.0f);
        SunColor     = glm::vec3(0.5f, 0.5f, 0.5f);
    }

    void Render(std::shared_ptr<Renderer> renderer) override
    {
        renderer->GetUBO()->sunDir   = SunDirection;
        renderer->GetUBO()->sunColor = SunColor;
        MeshEntity::Render(renderer);
    }

    glm::vec3 SunDirection;
    glm::vec3 SunColor;
};
} // namespace legs
