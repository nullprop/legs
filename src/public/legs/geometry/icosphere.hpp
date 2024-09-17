#pragma once

#include <cmath>
#include <glm/geometric.hpp>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <legs/log.hpp>
#include <legs/renderer/mesh_data.hpp>

namespace legs
{
static const glm::vec2                 baseSize = {0.525731112119133606f, 0.850650808352039932f};
static const std::array<glm::vec3, 12> basePositions = {
    glm::vec3 {-baseSize.x,        0.0f,  baseSize.y},
    glm::vec3 { baseSize.x,        0.0f,  baseSize.y},
    glm::vec3 {-baseSize.x,        0.0f, -baseSize.y},
    glm::vec3 { baseSize.x,        0.0f, -baseSize.y},

    glm::vec3 {       0.0f,  baseSize.y,  baseSize.x},
    glm::vec3 {       0.0f,  baseSize.y, -baseSize.x},
    glm::vec3 {       0.0f, -baseSize.y,  baseSize.x},
    glm::vec3 {       0.0f, -baseSize.y, -baseSize.x},

    glm::vec3 { baseSize.y,  baseSize.x,        0.0f},
    glm::vec3 {-baseSize.y,  baseSize.x,        0.0f},
    glm::vec3 { baseSize.y, -baseSize.x,        0.0f},
    glm::vec3 {-baseSize.y, -baseSize.x,        0.0f},
};
static const std::array<unsigned int, 60> baseIndices = {
    // clang-format off
    0,  1,  4,
    0,  4,  9,
    9,  4,  5,
    4,  8,  5,
    4,  1,  8,
    
    8,  1,  10,
    8,  10, 3,
    5,  8,  3,
    5,  3,  2,
    2,  3,  7,

    7,  3,  10,
    7,  10, 6,
    7,  6,  11,
    11, 6,  0,
    0,  6,  1,

    6,  10, 1,
    9,  11, 0,
    9,  2,  11,
    9,  5,  2,
    7,  11, 2
    // clang-format on
};
using EdgeMap = std::map<std::pair<Index, Index>, unsigned int>;

struct SIcosphere
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<Index>     indices;

    SIcosphere(glm::vec3 origin, float radius, unsigned int subdivisions, bool invert = false)
    {
        // Don't be silly...
        // 10k verts should be enough for anything
        const unsigned int maxDiv = 5;
        if (subdivisions > maxDiv)
        {
            LOG_WARN("Tried to create icosphere with subdiv {}, clamping", subdivisions);
            subdivisions = maxDiv;
        }

        auto numIndices = static_cast<size_t>(60 * std::pow(4, subdivisions));
        auto numVerts   = static_cast<size_t>(12 + 10 * (std::pow(4, subdivisions) - 1));

        positions.reserve(numVerts);
        indices.reserve(60);

        for (const auto& p : basePositions)
        {
            positions.push_back(p);
        }
        for (const auto& i : baseIndices)
        {
            indices.push_back(i);
        }

        for (unsigned int i = 0; i < subdivisions; i++)
        {
            Subdivide();
        }

        if (numVerts != positions.size())
        {
            LOG_ERROR(
                "Generated {} vertices for icosphere {}, expected {}",
                positions.size(),
                subdivisions,
                numVerts
            );
            numVerts = positions.size();
        }

        if (numIndices != indices.size())
        {
            LOG_ERROR(
                "Generated {} indices for icosphere, expected {}",
                indices.size(),
                numIndices
            );
            numIndices = indices.size();
        }

        normals.resize(numVerts);
        for (unsigned int i = 0; i < numVerts; i++)
        {
            normals[i]   = invert ? -positions[i] : positions[i];
            positions[i] = positions[i] * radius + origin;
        }

        if (invert)
        {
            for (unsigned int i = 0; i < numIndices; i += 3)
            {
                std::swap(indices[i], indices[i + 1]);
            }
        }
    }

    void Subdivide()
    {
        EdgeMap edgeMap;

        std::vector<Index> newIndices;
        newIndices.reserve(indices.size() * 4);

        for (unsigned int i = 0; i < indices.size(); i += 3)
        {
            unsigned int mid[3];
            for (unsigned int edge = 0; edge < 3; edge++)
            {
                const auto first  = i + edge;
                const auto second = i + ((edge + 1) % 3);
                mid[edge]         = EdgeVertex(edgeMap, indices[first], indices[second]);
            }

            newIndices.push_back(indices[i]);
            newIndices.push_back(mid[0]);
            newIndices.push_back(mid[2]);

            newIndices.push_back(indices[i + 1]);
            newIndices.push_back(mid[1]);
            newIndices.push_back(mid[0]);

            newIndices.push_back(indices[i + 2]);
            newIndices.push_back(mid[2]);
            newIndices.push_back(mid[1]);

            newIndices.push_back(mid[0]);
            newIndices.push_back(mid[1]);
            newIndices.push_back(mid[2]);
        }

        indices = newIndices;
    }

    unsigned int EdgeVertex(EdgeMap& edgeMap, Index first, Index second)
    {
        EdgeMap::key_type key(first, second);
        if (key.first > key.second)
        {
            std::swap(key.first, key.second);
        }

        const auto inserted = edgeMap.insert({key, positions.size()});
        if (inserted.second)
        {
            auto& edge0 = positions[first];
            auto& edge1 = positions[second];
            auto  pos   = glm::normalize(edge0 + edge1);
            positions.push_back(pos);
        }

        return inserted.first->second;
    }
};
}; // namespace legs
