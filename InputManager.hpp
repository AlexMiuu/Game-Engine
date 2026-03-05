//
// InputManager.hpp
// Sistem centralizat pentru gestionarea input-ului
//
// Înlocuie?te: pressedKeys[], callback-uri globale, state împrã?tiat
// Cu: Interfa?ã curatã, singleton, query-uri simple
//

#pragma once

#include <glm/glm.hpp>
#include <array>
#include <GLFW/glfw3.h>

namespace gps {

    /**
     * @brief Singleton pentru gestionarea input-ului de la tastaturã ?i mouse
     *
     * Aceastã clasã centralizeazã tot input-ul ?i oferã o interfa?ã simplã
     * pentru a verifica starea tastelor ?i mouse-ului.
     *
     * Exemple de utilizare:
     * @code
     * // În game loop
     * if (InputManager::Instance().IsKeyPressed(GLFW_KEY_W)) {
     *     camera->MoveForward(deltaTime);
     * }
     *
     * if (InputManager::Instance().IsMouseButtonJustPressed(GLFW_MOUSE_BUTTON_LEFT)) {
     *     StartSelection();
     * }
     *
     * glm::vec2 mousePos = InputManager::Instance().GetMousePosition();
     * @endcode
     */
    class InputManager {
    public:
        /**
         * @brief Ob?ine instan?a singleton
         */
        static InputManager& Instance() {
            static InputManager instance;
            return instance;
        }

        // Prevent copying
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

        // ===========================
        // PUBLIC API
        // ===========================

        /**
         * @brief Actualizeazã starea input-ului
         * Trebuie apelat la începutul fiecãrui frame
         */
        void Update();

        // ===========================
        // KEYBOARD INPUT
        // ===========================

        /**
         * @brief Verificã dacã o tastã este apãsatã în frame-ul curent
         * @param key Codul tastei (GLFW_KEY_*)
         * @return true dacã tasta e apãsatã
         */
        bool IsKeyPressed(int key) const;

        /**
         * @brief Verificã dacã o tastã tocmai a fost apãsatã (trigger)
         * @param key Codul tastei
         * @return true doar în frame-ul în care tasta a fost apãsatã
         */
        bool IsKeyJustPressed(int key) const;

        /**
         * @brief Verificã dacã o tastã tocmai a fost eliberatã
         * @param key Codul tastei
         * @return true doar în frame-ul în care tasta a fost eliberatã
         */
        bool IsKeyJustReleased(int key) const;

        // ===========================
        // MOUSE BUTTONS
        // ===========================

        /**
         * @brief Verificã dacã un buton de mouse e apãsat
         * @param button Codul butonului (GLFW_MOUSE_BUTTON_*)
         */
        bool IsMouseButtonPressed(int button) const;

        /**
         * @brief Verificã dacã un buton de mouse tocmai a fost apãsat
         */
        bool IsMouseButtonJustPressed(int button) const;

        /**
         * @brief Verificã dacã un buton de mouse tocmai a fost eliberat
         */
        bool IsMouseButtonJustReleased(int button) const;

        // ===========================
        // MOUSE POSITION
        // ===========================

        /**
         * @brief Ob?ine pozi?ia curentã a mouse-ului pe ecran
         * @return Pozi?ia în coordonate de ecran (pixeli)
         */
        glm::vec2 GetMousePosition() const { return m_mousePosition; }

        /**
         * @brief Ob?ine delta-ul de mi?care a mouse-ului
         * @return Diferen?a de pozi?ie fa?ã de frame-ul anterior
         */
        glm::vec2 GetMouseDelta() const {
            return m_mousePosition - m_mousePositionLast;
        }

        /**
         * @brief Ob?ine valoarea scroll-ului mouse-ului
         * @return Valoarea scroll-ului din frame-ul curent
         */
        float GetMouseScroll() const { return m_mouseScroll; }

        // ===========================
        // INTERNAL (called by Window callbacks)
        // ===========================

        /**
         * @brief Callback intern pentru evenimente de tastaturã
         * NU apela manual! Folosit de Window class
         */
        void OnKeyEvent(int key, int scancode, int action, int mods);

        /**
         * @brief Callback intern pentru butoane de mouse
         */
        void OnMouseButton(int button, int action, int mods);

        /**
         * @brief Callback intern pentru mi?care mouse
         */
        void OnMouseMove(double xpos, double ypos);

        /**
         * @brief Callback intern pentru scroll
         */
        void OnMouseScroll(double xoffset, double yoffset);

        // ===========================
        // UTILITY
        // ===========================

        /**
         * @brief Reseteazã starea input-ului
         */
        void Reset();

        /**
         * @brief Activeazã/dezactiveazã capturarea mouse-ului
         * @param capture Dacã true, mouse-ul e locked în fereastrã
         */
        void SetMouseCaptured(bool capture);

        /**
         * @brief Verificã dacã mouse-ul e capturat
         */
        bool IsMouseCaptured() const { return m_mouseCaptured; }

    private:
        InputManager() = default;

        // ===========================
        // KEYBOARD STATE
        // ===========================
        std::array<bool, 1024> m_keys;           // Starea curentã
        std::array<bool, 1024> m_keysLastFrame;  // Starea din frame-ul anterior

        // ===========================
        // MOUSE STATE
        // ===========================
        std::array<bool, 8> m_mouseButtons;           // Butoane curente
        std::array<bool, 8> m_mouseButtonsLastFrame;  // Butoane frame anterior

        glm::vec2 m_mousePosition;      // Pozi?ia curentã
        glm::vec2 m_mousePositionLast;  // Pozi?ia din frame-ul anterior
        float m_mouseScroll;            // Scroll value

        bool m_mouseCaptured = false;

        // ===========================
        // HELPERS
        // ===========================

        /**
         * @brief Verificã dacã un key code e valid
         */
        bool IsValidKey(int key) const {
            return key >= 0 && key < 1024;
        }

        /**
         * @brief Verificã dacã un button code e valid
         */
        bool IsValidButton(int button) const {
            return button >= 0 && button < 8;
        }
    };

} // namespace gps

/*
 * ============================================================
 * ?? EXEMPLE DE UTILIZARE
 * ============================================================
 *
 * 1. VERIFICARE TASTÃ CONTINUÃ (held down)
 * ------------------------------------------
 * if (InputManager::Instance().IsKeyPressed(GLFW_KEY_W)) {
 *     camera->MoveForward(deltaTime);
 * }
 *
 * 2. TRIGGER (doar în frame-ul când se apasã)
 * --------------------------------------------
 * if (InputManager::Instance().IsKeyJustPressed(GLFW_KEY_SPACE)) {
 *     player->Jump();
 * }
 *
 * 3. MOUSE BUTTON
 * ----------------
 * if (InputManager::Instance().IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
 *     DrawSelectionBox();
 * }
 *
 * 4. MOUSE POSITION
 * ------------------
 * glm::vec2 mousePos = InputManager::Instance().GetMousePosition();
 * glm::vec3 worldPos = ScreenToWorld(mousePos);
 *
 * 5. MOUSE DELTA (pentru camera rotation)
 * -----------------------------------------
 * glm::vec2 delta = InputManager::Instance().GetMouseDelta();
 * camera->Rotate(delta.x * sensitivity, delta.y * sensitivity);
 *
 * 6. SCROLL (pentru zoom)
 * ------------------------
 * float scroll = InputManager::Instance().GetMouseScroll();
 * if (scroll != 0.0f) {
 *     camera->Zoom(scroll);
 * }
 *
 * ============================================================
 * ?? BENEFICII vs. CODUL VECHI
 * ============================================================
 *
 * ÎNAINTE (în main.cpp):
 * ```cpp
 * bool pressedKeys[1024];  // Global!
 *
 * void keyCallback(GLFWwindow* window, int key, ...) {
 *     if (action == GLFW_PRESS) {
 *         pressedKeys[key] = true;
 *     }
 *     // Logicã complexã aici...
 * }
 *
 * void processMovement() {
 *     if (pressedKeys[GLFW_KEY_W]) {
 *         // ...
 *     }
 * }
 * ```
 *
 * DUPÃ (cu InputManager):
 * ```cpp
 * if (InputManager::Instance().IsKeyPressed(GLFW_KEY_W)) {
 *     // Simplu ?i clar!
 * }
 * ```
 *
 * Avantaje:
 * ? Encapsulare perfectã
 * ? Interfa?ã intuitivã
 * ? Suport pentru "just pressed" / "just released"
 * ? Mouse delta automat
 * ? Validare automatã a codurilor
 * ? Testabil (mock pentru unit tests)
 *
 * ============================================================
 */