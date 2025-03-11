#include "Camera.hpp"

namespace gps {

    // Constructor: Sets initial position for isometric view
    Camera::Camera(glm::vec3 cameraPosition)
        : cameraPosition(cameraPosition) {
        cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        cameraUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    // Return the isometric view matrix
    glm::mat4 Camera::getViewMatrix() {
        // Apply an isometric tilt (30-45 degrees)
        glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
        view = glm::rotate(view, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Tilt down
        view = glm::rotate(view, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate for isometric angle
        return view;
    }

    // Return the orthographic projection matrix
    glm::mat4 Camera::getProjectionMatrix(float left, float right, float bottom, float top, float near, float far) {
        return glm::ortho(left, right, bottom, top, near, far);
    }

    // Move the camera in 2D space
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_UP) {
            cameraPosition.y += speed;
        }
        else if (direction == MOVE_DOWN) {
            cameraPosition.y -= speed;
        }
        else if (direction == MOVE_LEFT) {
            cameraPosition.x -= speed;
        }
        else if (direction == MOVE_RIGHT) {
            cameraPosition.x += speed;
        }
    }

}
