#include "Camera.hpp"

namespace gps {

    // Constructorul
    Camera::Camera(glm::vec3 cameraPosition)
        : cameraPosition(cameraPosition) {
        cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        cameraUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    // Matricea de vizualizare pentru camera izometrica
    glm::mat4 Camera::getViewMatrix() const {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    // Matricea de proiectie ortografica
    glm::mat4 Camera::getProjectionMatrix(float left, float right, float bottom, float top, float near, float far) {
        return glm::ortho(left, right, bottom, top, near, far);
    }

    void Camera::centerOn(const glm::vec3& worldPoint) {
        glm::vec3 delta = worldPoint - cameraTarget;
        delta.y = 0.0f;
        cameraPosition += delta;
        cameraTarget   += delta;
    }

    // Move the camera in 2D space
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        // Calculate movement vectors for isometric plane
        glm::vec3 forward = glm::normalize(glm::vec3(1.0f, 0.0f, -1.0f)); // Isometric forward
        glm::vec3 right = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));    // Isometric right

        switch (direction) {
        case MOVE_UP:
            cameraPosition += forward * speed;
            cameraTarget += forward * speed;
            break;
        case MOVE_DOWN:
            cameraPosition -= forward * speed;
            cameraTarget -= forward * speed;
            break;
        case MOVE_RIGHT:
            cameraPosition += right * speed;
            cameraTarget += right * speed;
            break;
        case MOVE_LEFT:
            cameraPosition -= right * speed;
            cameraTarget -= right * speed;
            break;
        }
    }

}
