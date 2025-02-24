#include "RayCaster.hpp"
#include "Model3D.hpp"  // <-- needed to handle Model3D
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <limits>
#include <cfloat>  // for FLT_MAX


namespace gps {
  
   Ray computeRay(
        float mouseX,
        float mouseY,
        float windowWidth,
        float windowHeight,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosWorld
    )
    {
        // Convert mouse coords to NDC
        float xNdc = (2.0f * mouseX) / windowWidth - 1.0f;
        float yNdc = 1.0f - (2.0f * mouseY) / windowHeight;
        float zNdc = -1.0f; // near plane

        glm::vec4 clipPos(xNdc, yNdc, zNdc, 1.0f);

        // Inverse projection -> view-space
        glm::mat4 invProj = glm::inverse(projection);
        glm::vec4 viewPos = invProj * clipPos;
        viewPos /= viewPos.w; // perspective divide

        // Inverse view -> world-space
        glm::mat4 invView = glm::inverse(view);
        glm::vec4 worldPos = invView * viewPos;

        Ray ray;
        ray.origin = cameraPosWorld;
        ray.direction = glm::normalize(glm::vec3(worldPos) - ray.origin);
        return ray;
    }

    bool RayIntersectsSphere(
        const Ray& ray,
        const glm::vec3& center,
        float radius,
        float& outDistance
    )
    {
        glm::vec3 oc = ray.origin - center;
        float a = glm::dot(ray.direction, ray.direction);  // ~1 if normalized
        float b = 2.0f * glm::dot(oc, ray.direction);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f)
            return false;

        float sqrtDisc = sqrt(discriminant);
        float t1 = (-b - sqrtDisc) / (2.0f * a);
        float t2 = (-b + sqrtDisc) / (2.0f * a);

        // Find the smallest positive root
        float t = (t1 < t2) ? t1 : t2;
        if (t < 0.0f) {
            t = (t == t1) ? t2 : t1;
            if (t < 0.0f)
                return false;
        }
        outDistance = t;
        return true;
    }
    // --------------------------------------------------------------
    // NEW FUNCTION #1: Ray-Triangle intersection (Möller–Trumbore)
    // --------------------------------------------------------------
    bool RayIntersectsTriangle(
        const Ray& ray,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& outT
    )
    {
        const float EPSILON = 1e-6f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);
        if (fabs(a) < EPSILON) {
            // Ray is parallel to this triangle
            return false;
        }
        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) {
            return false;
        }
        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);
        if (v < 0.0f || (u + v) > 1.0f) {
            return false;
        }
        float t = f * glm::dot(edge2, q);
        if (t > EPSILON) {
            outT = t;
            return true;
        }
        return false;
    }

    // --------------------------------------------------------------
    // NEW FUNCTION #2: Ray-Model intersection in WORLD SPACE
    // (We transform each local triangle by modelMatrix.)
    // --------------------------------------------------------------
    bool RayIntersectsModelWithMatrix(
        const Ray& ray,
        const gps::Model3D& model,
        const glm::mat4& modelMatrix,
        float& outT,
        glm::vec3& outPoint
    )
    {
        float closestT = FLT_MAX;
        bool  hitAny = false;

        // 1) Access submeshes. (Requires Model3D::getMeshes() to be public.)
        const auto& meshList = model.getMeshes();

        // 2) Loop over each mesh
        for (auto& mesh : meshList) {
            const auto& verts = mesh.vertices;
            const auto& indices = mesh.indices;

            // 3) For each triangle in the index buffer
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                glm::vec3 v0 = verts[indices[i + 0]].Position;
                glm::vec3 v1 = verts[indices[i + 1]].Position;
                glm::vec3 v2 = verts[indices[i + 2]].Position;

                // transform local -> world
                glm::vec4 wv0 = modelMatrix * glm::vec4(v0, 1.0f);
                glm::vec4 wv1 = modelMatrix * glm::vec4(v1, 1.0f);
                glm::vec4 wv2 = modelMatrix * glm::vec4(v2, 1.0f);

                float t;
                if (RayIntersectsTriangle(
                    ray,
                    glm::vec3(wv0), glm::vec3(wv1), glm::vec3(wv2),
                    t))
                {
                    // keep the closest positive intersection
                    if (t < closestT && t > 0.0f) {
                        closestT = t;
                        hitAny = true;
                    }
                }
            }
        }

        if (hitAny) {
            outT = closestT;
            outPoint = ray.origin + closestT * ray.direction;
        }
        return hitAny;
    }

}
