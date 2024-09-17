#pragma once

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include <legs/components/rotation.hpp>

namespace legs
{
struct STransform
{
    glm::vec3 position;
    SRotation rotation;
    glm::vec3 velocity;
    glm::vec3 angularVelocity;

    glm::mat4x4 GetModelMatrix()
    {
        auto model = glm::identity<glm::mat4x4>();
        model      = glm::translate(model, position);
        model      = glm::rotate(model, glm::radians(rotation.euler.x), glm::vec3 {1, 0, 0});
        model      = glm::rotate(model, glm::radians(rotation.euler.y), glm::vec3 {0, 1, 0});
        model      = glm::rotate(model, glm::radians(rotation.euler.z), glm::vec3 {0, 0, 1});
        return model;
    }

    glm::vec3 Forward()
    {
        return rotation.quaternion * glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 Right()
    {
        return rotation.quaternion * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 Up()
    {
        return rotation.quaternion * glm::vec3(0.0f, 0.0f, 1.0f);
    }
};
} // namespace legs
