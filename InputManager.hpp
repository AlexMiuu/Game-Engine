#pragma once

#include <glm/glm.hpp>
#include <array>
#include <GLFW/glfw3.h>

namespace gps {

    //Input centralizat pt gestiunea inputului
    class InputManager {
    public:
        // Instanta singleton
        static InputManager& Instance() {
            static InputManager instance;
            return instance;
        }

        // Prevent copying
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        void Update();

        bool IsKeyPressed(int key) const;
        bool IsKeyJustPressed(int key) const;

        bool IsKeyJustReleased(int key) const;

        bool IsMouseButtonPressed(int button) const;
        bool IsMouseButtonJustPressed(int button) const;
        bool IsMouseButtonJustReleased(int button) const;

        glm::vec2 GetMousePosition() const { return m_mousePosition; }
        glm::vec2 GetMouseDelta() const {
            return m_mousePosition - m_mousePositionLast;
        }
        float GetMouseScroll() const { return m_mouseScroll; }

        // (called by Window callbacks)

        void OnKeyEvent(int key, int scancode, int action, int mods);
        void OnMouseButton(int button, int action, int mods);
        void OnMouseMove(double xpos, double ypos);
        void OnMouseScroll(double xoffset, double yoffset);

        void Reset();
        void SetMouseCaptured(bool capture);
        bool IsMouseCaptured() const { return m_mouseCaptured; }

    private:
        InputManager() = default;

        std::array<bool, 1024> m_keys;           // Starea curenta
        std::array<bool, 1024> m_keysLastFrame;  // Starea din frame-ul anterior

        std::array<bool, 8> m_mouseButtons;           // Butoane curente
        std::array<bool, 8> m_mouseButtonsLastFrame;  // Butoane frame anterior

        glm::vec2 m_mousePosition;      // Pozitia curenta
        glm::vec2 m_mousePositionLast;  // Pozitia din frame-ul anterior
        float m_mouseScroll;            // Scroll value

        bool m_mouseCaptured = false;

        bool IsValidKey(int key) const {
            return key >= 0 && key < 1024;
        }

        bool IsValidButton(int button) const {
            return button >= 0 && button < 8;
        }
    };

}
