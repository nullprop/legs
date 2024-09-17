#pragma once

#include <memory>
#include <type_traits>

#include <glm/vec3.hpp>

#include <legs/renderer/common.hpp>

namespace legs
{
struct Vertex_P
{
    glm::vec3 position;
};

struct Vertex_P_C
{
    glm::vec3 position;
    glm::vec3 color;
};

struct Vertex_P_N_C
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

// For fullscreen triangle
struct VertexEmpty
{
};

typedef uint32_t Index;

template<typename T>
consteval VkVertexInputBindingDescription GetBindingDescription()
{
    VkVertexInputBindingDescription desc {};
    desc.binding   = 0;
    desc.stride    = sizeof(T);
    desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return desc;
}

template<typename T>
constexpr std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> desc {};

    // TODO: reflection

    if constexpr (std::is_same_v<T, Vertex_P>)
    {
        desc.push_back({
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(T, position),
        });
    }
    else if constexpr (std::is_same_v<T, Vertex_P_C>)
    {
        desc.push_back({
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(T, position),
        });
        desc.push_back({
            .location = 1,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(T, color),
        });
    }
    else if constexpr (std::is_same_v<T, Vertex_P_N_C>)
    {
        desc.push_back({
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(T, position),
        });
        desc.push_back({
            .location = 1,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(T, normal),
        });
        desc.push_back({
            .location = 2,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof(T, color),
        });
    }
    else if constexpr (std::is_same_v<T, VertexEmpty>)
    {
        // nada
    }
    else
    {
        static_assert(false, "Unhandled type for attribute descriptions");
    }

    return desc;
}
} // namespace legs
