#include "Camera.hpp"

namespace gps {

    // Constructor: Sets initial position for isometric view
    Camera::Camera(glm::vec3 cameraPosition)
        : cameraPosition(cameraPosition) {
        // Initialize camera vectors for RTS-style view
        cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
        cameraUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    // Return the isometric view matrix
    glm::mat4 Camera::getViewMatrix() {
        // Create the view matrix with RTS-style isometric angles
        glm::mat4 view = glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);

        // Apply RTS-style rotation (45 degrees around Y, 60 degrees around X)
        float rtsAngleX =  60.0f;  // More top-down view typical for RTS games
        float rtsAngleY = 45.0f;  // Standard 45-degree rotation

        view = glm::rotate(view, glm::radians(rtsAngleX), glm::vec3(1.0f, 0.0f, 0.0f));
        view = glm::rotate(view, glm::radians(rtsAngleY), glm::vec3(0.0f, 1.0f, 0.0f));

        return view;
    }

    // Return the orthographic projection matrix
    glm::mat4 Camera::getProjectionMatrix(float left, float right, float bottom, float top, float near, float far) {
        return glm::ortho(left, right, bottom, top, near, far);
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
