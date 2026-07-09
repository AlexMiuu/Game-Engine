//
// Component.cpp
// Implementare pentru clasa de baz� Component
//

#include "Component.hpp"
#include "SceneObject.hpp"

namespace gps {

    void Component::Update(float deltaTime) {
        //each component implements its own update logic, so the base class does not do anything
    }

    void Component::OnDestroy() {
    }


}