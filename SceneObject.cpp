#include "SceneObject.hpp"
#include "Component.hpp"
#include "Model3D.hpp"
#include <iostream>
#include <algorithm>

namespace gps {

    SceneObject::SceneObject(int id, const std::string& name)
        : m_id(id)
		, m_tag("default")
        , m_name(name)
        , m_active(true)
        , m_model(nullptr)
        , m_localCenter(0.0f)
        , m_localRadius(1.0f)
        , m_worldCenter(0.0f)
        , m_worldRadius(1.0f)
        , m_collisionRadius(0.0f)
    {
    }

    SceneObject::~SceneObject() {
        // Cleanup components
        for (auto& component : m_components) {
            if (component) {
                component->OnDestroy();
            }
        }
    }

    void SceneObject::Update(float deltaTime) {
        if (!m_active) return;

        // Update toate componentele active
        for (auto& component : m_components) {
            if (component && component->IsActive()) {
                component->Update(deltaTime);
            }
        }
        UpdateWorldBounds();
    }

    void SceneObject::UpdateWorldBounds() {
        // Transforma centrul local in world space
        const glm::mat4& modelMatrix = m_transform.GetModelMatrix();
        glm::vec4 worldCenterVec4 = modelMatrix * glm::vec4(m_localCenter, 1.0f);
        m_worldCenter = glm::vec3(worldCenterVec4);

        // Folosim scale-ul maxim din cele 3 axe
        glm::vec3 scaleVec = m_transform.GetScale();
        float maxScale = std::max(scaleVec.x, std::max(scaleVec.y, scaleVec.z));
        m_worldRadius = m_localRadius * maxScale;
    }

}