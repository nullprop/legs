#pragma once

#include <memory>
#include <string>

#include <legs/entity/entity.hpp>
#include <legs/renderer/buffer.hpp>
#include <legs/renderer/renderer.hpp>

namespace legs
{
class MeshEntity : public Entity
{
  public:
    MeshEntity() : Entity(), m_pipeline(RenderPipeline::INVALID)
    {
    }

    virtual ~MeshEntity() = default;

    MeshEntity(const MeshEntity&)            = delete;
    MeshEntity(MeshEntity&&)                 = delete;
    MeshEntity& operator=(const MeshEntity&) = delete;
    MeshEntity& operator=(MeshEntity&&)      = delete;

    virtual void OnSpawn() override
    {
        Entity::OnSpawn();
    }

    virtual void OnDestroy() override
    {
        Entity::OnDestroy();
    }

    virtual void OnFrame() override
    {
        Entity::OnFrame();
    }

    virtual void OnTick() override
    {
        Entity::OnTick();
    }

    virtual void SetBuffers(
        std::shared_ptr<Buffer> vertexBuffer,
        std::shared_ptr<Buffer> indexBuffer
    )
    {
        m_vertexBuffer = vertexBuffer;
        m_indexBuffer  = indexBuffer;
    }

    virtual void Render(std::shared_ptr<Renderer> renderer)
    {
        if (m_pipeline == RenderPipeline::INVALID)
        {
            return;
        }

        // TODO: Transform matrices
        renderer->BindPipeline(m_pipeline);
        renderer->DrawWithBuffers(m_vertexBuffer, m_indexBuffer);
    }

    virtual void SetPipeline(RenderPipeline pipeline)
    {
        m_pipeline = pipeline;
    }

  protected:
    RenderPipeline          m_pipeline;
    std::shared_ptr<Buffer> m_vertexBuffer;
    std::shared_ptr<Buffer> m_indexBuffer;
};
}; // namespace legs
