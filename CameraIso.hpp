#pragma once
#ifndef CAMERA_ISO_HPP
#define CAMERA_ISO_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraISO {
public:
    CameraISO(float left, float right, float bottom, float top, float near, float far);

    void SetPosition(const glm::vec3& position);
    void SetTarget(const glm::vec3& target);
    void SetUpVector(const glm::vec3& up);

    void MoveForward(float speed);
    void MoveBackward(float speed);
    void MoveLeft(float speed);
    void MoveRight(float speed);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetMVPMatrix(const glm::mat4& model) const;

    glm::vec3 GetPosition() const { return cameraPosition; }

private:
    glm::mat4 projection;
    glm::vec3 cameraPosition;
    glm::vec3 cameraTarget;
    glm::vec3 upVector;

    // Isometric movement axes
    glm::vec3 isoX;
    glm::vec3 isoZ;
};

#endif