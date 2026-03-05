//
// InputManager.cpp
// Implementarea sistemului de input
//

#include "InputManager.hpp"
#include <algorithm>
#include <iostream>

namespace gps {

    void InputManager::Update() {
        // Salveazã starea din frame-ul anterior
        m_keysLastFrame = m_keys;
        m_mouseButtonsLastFrame = m_mouseButtons;
        m_mousePositionLast = m_mousePosition;

        // Reseteazã scroll (e un event, nu state persistent)
        m_mouseScroll = 0.0f;
    }

    void InputManager::Reset() {
        m_keys.fill(false);
        m_keysLastFrame.fill(false);
        m_mouseButtons.fill(false);
        m_mouseButtonsLastFrame.fill(false);
        m_mouseScroll = 0.0f;
        m_mousePosition = glm::vec2(0.0f);
        m_mousePositionLast = glm::vec2(0.0f);
    }

    // ===========================
    // KEYBOARD
    // ===========================

    bool InputManager::IsKeyPressed(int key) const {
        if (!IsValidKey(key)) return false;
        return m_keys[key];
    }

    bool InputManager::IsKeyJustPressed(int key) const {
        if (!IsValidKey(key)) return false;
        return m_keys[key] && !m_keysLastFrame[key];
    }

    bool InputManager::IsKeyJustReleased(int key) const {
        if (!IsValidKey(key)) return false;
        return !m_keys[key] && m_keysLastFrame[key];
    }

    // ===========================
    // MOUSE BUTTONS
    // ===========================

    bool InputManager::IsMouseButtonPressed(int button) const {
        if (!IsValidButton(button)) return false;
        return m_mouseButtons[button];
    }

    bool InputManager::IsMouseButtonJustPressed(int button) const {
        if (!IsValidButton(button)) return false;
        return m_mouseButtons[button] && !m_mouseButtonsLastFrame[button];
    }

    bool InputManager::IsMouseButtonJustReleased(int button) const {
        if (!IsValidButton(button)) return false;
        return !m_mouseButtons[button] && m_mouseButtonsLastFrame[button];
    }

    // ===========================
    // CALLBACKS
    // ===========================

    void InputManager::OnKeyEvent(int key, int scancode, int action, int mods) {
        if (!IsValidKey(key)) return;

        if (action == GLFW_PRESS) {
            m_keys[key] = true;
        }
        else if (action == GLFW_RELEASE) {
            m_keys[key] = false;
        }
        // GLFW_REPEAT - ignorãm, IsKeyPressed() va returna true continuu
    }

    void InputManager::OnMouseButton(int button, int action, int mods) {
        if (!IsValidButton(button)) return;

        if (action == GLFW_PRESS) {
            m_mouseButtons[button] = true;
        }
        else if (action == GLFW_RELEASE) {
            m_mouseButtons[button] = false;
        }
    }

    void InputManager::OnMouseMove(double xpos, double ypos) {
        m_mousePosition.x = static_cast<float>(xpos);
        m_mousePosition.y = static_cast<float>(ypos);
    }

    void InputManager::OnMouseScroll(double xoffset, double yoffset) {
        m_mouseScroll = static_cast<float>(yoffset);
    }

} // namespace gps