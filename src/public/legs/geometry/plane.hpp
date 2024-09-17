#pragma once

#include <glm/vec3.hpp>

#include <legs/renderer/mesh_data.hpp>

namespace legs
{
static const std::array<glm::vec3, 4> planeBaseVertices = {
    glm::vec3 {-0.5f, -0.5f, 0.0f},
    glm::vec3 { 0.5f, -0.5f, 0.0f},
    glm::vec3 { 0.5f,  0.5f, 0.0f},
    glm::vec3 {-0.5f,  0.5f, 0.0f},
};
static const std::array<Index, 6> planeBaseIndices = {0, 1, 2, 2, 3, 0};

struct SPlane
{
    std::array<glm::vec3, 4> vertices;
    std::array<Index, 6>     indices;

    SPlane(glm::vec3 position, float scale = 1.0f)
    {
        indices = planeBaseIndices;
        for (unsigned int i = 0; i < 4; i++)
        {
            vertices[i] = position + planeBaseVertices[i] * scale;
        }
    }
};
}; // namespace legs
