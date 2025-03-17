#include "CameraISO.hpp"

CameraISO::CameraISO(float left, float right, float bottom, float top, float near, float far) {
    projection = glm::ortho(left, right, bottom, top, near, far);
    cameraPosition = glm::vec3(100.0f, 150.0f, 100.0f); // Elevated position
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    // Define isometric axes (45-degree projection)
    isoX = glm::normalize(glm::vec3(1.0f, 0.0f, -1.0f));
    isoZ = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));
}

void CameraISO::SetPosition(const glm::vec3& position) { cameraPosition = position; }
void CameraISO::SetTarget(const glm::vec3& target) { cameraTarget = target; }
void CameraISO::SetUpVector(const glm::vec3& up) { upVector = up; }

void CameraISO::MoveForward(float speed) { cameraPosition += isoZ * speed; }
void CameraISO::MoveBackward(float speed) { cameraPosition -= isoZ * speed; }
void CameraISO::MoveLeft(float speed) { cameraPosition -= isoX * speed; }
void CameraISO::MoveRight(float speed) { cameraPosition += isoX * speed; }

glm::mat4 CameraISO::GetViewMatrix() const {
    return glm::lookAt(cameraPosition, cameraTarget, upVector);
}

glm::mat4 CameraISO::GetProjectionMatrix() const { return projection; }
glm::mat4 CameraISO::GetMVPMatrix(const glm::mat4& model) const {
    return projection * GetViewMatrix() * model;
}