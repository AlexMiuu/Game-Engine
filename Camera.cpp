#include "Camera.hpp"
#include <glm/gtc/matrix_transform.hpp> // For glm::lookAt
#include <glm/gtc/constants.hpp>        // For mathematical constants

namespace gps {

    // Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp)
        : cameraPosition(cameraPosition), cameraTarget(cameraTarget), cameraUpDirection(cameraUp)
    {
        // Calculate the initial front direction based on target and position
        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);

        // Calculate the right direction as the cross product of front and up
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));

        // Re-calculate the up direction to ensure orthogonality
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

    // Return the view matrix using glm::lookAt
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraPosition + cameraFrontDirection, cameraUpDirection);
    }

    // Update the camera position based on move direction
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD) {
            cameraPosition += speed * cameraFrontDirection;
        }
        else if (direction == MOVE_BACKWARD) {
            cameraPosition -= speed * cameraFrontDirection;
        }
        else if (direction == MOVE_RIGHT) {
            cameraPosition += speed * cameraRightDirection;
        }
        else if (direction == MOVE_LEFT) {
            cameraPosition -= speed * cameraRightDirection;
        }
    }

    // Update the camera front direction based on yaw and pitch
    void Camera::rotate(float pitch, float yaw) {
        // Calculate the new front vector based on yaw and pitch
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        // Normalize and set the new front direction
        cameraFrontDirection = glm::normalize(front);

        // Recalculate the right and up directions to maintain orthogonality
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
        cameraUpDirection = glm::normalize(glm::cross(cameraRightDirection, cameraFrontDirection));
    }

} // namespace gps
