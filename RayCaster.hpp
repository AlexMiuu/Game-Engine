#pragma once

#include <glm/glm.hpp>
#include "Model3D.hpp"

namespace gps {

    // A simple Ray struct in gps namespace
    struct Ray {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    // 1) Unprojects mouse coords -> a Ray in world space
    Ray computeRay(
        float mouseX,
        float mouseY,
        float windowWidth,
        float windowHeight,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosWorld
    );

    // 2) Ray-sphere intersection
    bool RayIntersectsSphere(
        const Ray& ray,
        const glm::vec3& center,
        float radius,
        float& outDistance
    );

    // 3) Ray-triangle intersection (Möller–Trumbore)
    bool RayIntersectsTriangle(
        const Ray& ray,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& outT
    );

    // 4) Ray-model intersection in world space
    bool RayIntersectsModelWithMatrix(
        const Ray& ray,
        const gps::Model3D& model,
        const glm::mat4& modelMatrix,
        float& outT,
        glm::vec3& outPoint
    );

} // end namespace gps
