//
// Component.cpp
// Implementare pentru clasa de bazã Component
//

#include "Component.hpp"
#include "SceneObject.hpp"

namespace gps {

    // Destructorul virtual este deja definit cu = default în .hpp
    // Dar pentru safety, îl putem redefini aici

    // Update - implementare implicitã (poate fi suprasã de subclase)
    void Component::Update(float deltaTime) {
        // Empty - fiecare tip de componentã va implementa propria versiune
    }

    // OnDestroy - implementare implicitã
    void Component::OnDestroy() {
        // Empty - pentru cleanup înainte de distrugere
    }

    // Getters/Setters sunt deja inline în .hpp, dar dacã vrei po?i sã le mu?i aici

} // namespace gps