#pragma once

#include <algorithm>

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <legs/components/transform.hpp>
#include <legs/entity/entity.hpp>
#include <legs/time.hpp>
#include <legs/window/input.hpp>

namespace legs
{
#define CAM_NEAR 0.1f
#define CAM_FAR  1000.0f

class Camera : public Entity
{
  public:
    Camera(int width, int height) : Entity()
    {
        fov  = 60.0f;
        near = CAM_NEAR;
        far  = CAM_FAR;
        UpdateViewport(width, height);
        UpdateMatrices();
    }

    virtual ~Camera() {};

    void UpdateMatrices()
    {
        Transform->rotation.UpdateQuaternion();
        view = glm::lookAt(
            Transform->position,
            Transform->position + Transform->Forward(),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        proj = glm::perspective(glm::radians(fov), aspect, near, far);
        proj[1][1] *= -1; // OpenGL Y flip
    }

    void UpdateViewport(int width, int height)
    {
        aspect     = static_cast<float>(width) / static_cast<float>(height);
        viewport.x = static_cast<float>(width);
        viewport.y = static_cast<float>(height);
        viewport.z = CAM_NEAR;
        viewport.w = CAM_FAR;
    }

    float     fov;
    float     aspect;
    float     near;
    float     far;
    glm::vec4 viewport;
    glm::mat4 view;
    glm::mat4 proj;
};

class NoclipCamera : public Camera
{
  public:
    NoclipCamera(int width, int height) : Camera(width, height)
    {
        MoveSpeed   = 1.0f;
        Sensitivity = 2.0f;
    }

    void HandleInput(WindowInput input)
    {
        if (input.mouse.x != 0)
        {
            Transform->rotation.euler.z -= 0.022f * 3.14f * static_cast<float>(input.mouse.x);
            while (Transform->rotation.euler.z < -180.0f)
            {
                Transform->rotation.euler.z += 360.0f;
            }
            while (Transform->rotation.euler.z > 180.0f)
            {
                Transform->rotation.euler.z -= 360.0f;
            }
        }
        if (input.mouse.y != 0)
        {
            Transform->rotation.euler.x -= 0.022f * 3.14f * static_cast<float>(input.mouse.y);
            Transform->rotation.euler.x = std::clamp(Transform->rotation.euler.x, -89.0f, 89.0f);
        }

        if (input.scroll.y > 0)
        {
            MoveSpeed *= 2;
        }
        else if (input.scroll.y < 0)
        {
            MoveSpeed /= 2;
        }

        if (input.HasKey(Key::KEY_MOVE_FORWARD))
        {
            Transform->position +=
                Transform->Forward() * MoveSpeed * static_cast<float>(Time::DeltaFrame);
        }
        if (input.HasKey(Key::KEY_MOVE_BACK))
        {
            Transform->position -=
                Transform->Forward() * MoveSpeed * static_cast<float>(Time::DeltaFrame);
        }
        if (input.HasKey(Key::KEY_MOVE_RIGHT))
        {
            Transform->position +=
                Transform->Right() * MoveSpeed * static_cast<float>(Time::DeltaFrame);
        }
        if (input.HasKey(Key::KEY_MOVE_LEFT))
        {
            Transform->position -=
                Transform->Right() * MoveSpeed * static_cast<float>(Time::DeltaFrame);
        }
        if (input.HasKey(Key::KEY_MOVE_UP))
        {
            Transform->position +=
                glm::vec3(0, 0, 1) * MoveSpeed * static_cast<float>(Time::DeltaFrame);
        }
        if (input.HasKey(Key::KEY_MOVE_DOWN))
        {
            Transform->position -=
                glm::vec3(0, 0, 1) * MoveSpeed * static_cast<float>(Time::DeltaFrame);
        }

        UpdateMatrices();
    }

    float MoveSpeed;
    float Sensitivity;
};
} // namespace legs
