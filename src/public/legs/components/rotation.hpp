#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace legs
{
struct SRotation
{
    SRotation()
    {
        euler      = {};
        quaternion = glm::identity<glm::quat>();
    }

    void UpdateQuaternion()
    {
        quaternion = glm::angleAxis(glm::radians(euler.z), glm::vec3 {0.0f, 0.0f, 1.0f})
                     * glm::angleAxis(glm::radians(euler.x), glm::vec3 {1.0f, 0.0f, 0.0f});
    }

    glm::vec3 euler;
    glm::quat quaternion;
};
} // namespace legs
