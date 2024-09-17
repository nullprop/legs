#pragma once

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

#include <legs/entity/camera.hpp>

namespace legs
{
struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::mat4 mvp;
    alignas(16) glm::mat4 invModel;
    alignas(16) glm::mat4 invView;
    alignas(16) glm::mat4 invProj;
    alignas(16) glm::mat4 clipToWorld;
    alignas(16) glm::vec3 eye;

    alignas(16) glm::vec4 viewport;

    alignas(16) glm::vec3 sunDir;
    alignas(16) glm::vec3 sunColor;

    void SetCamera(const std::shared_ptr<Camera> cam)
    {
        model       = glm::identity<glm::mat4>();
        view        = cam->view;
        proj        = cam->proj;
        mvp         = proj * view * model;
        invModel    = glm::inverse(model);
        invView     = glm::inverse(view);
        invProj     = glm::inverse(proj);
        clipToWorld = glm::inverse(proj * view);
        eye         = cam->GetPosition();

        viewport = cam->viewport;
    }
};

}; // namespace legs
