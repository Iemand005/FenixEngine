#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Object.hpp"
#include "ShaderProgram.hpp"

namespace fe {
class Camera {
private:
	glm::vec3 position;
	glm::mat4 viewMatrix;
	unsigned int frustumVAO = 0, frustumVBO = 0;
	std::vector<glm::vec3> frustumVertices;
	
	public:
	glm::vec3 up;
	glm::vec3 front;
	float fov, aspect, nearDist, farDist;
	glm::mat4 projectionMatrix;

	float yaw = -90.0f;
	float pitch = 0.0f;

  Camera() {}

  Camera(float nearDist, float farDist) : nearDist(nearDist), farDist(farDist) {};

  Camera(float fov, float nearDist, float farDist) : Camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), fov, 1, nearDist, farDist) {};

  Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float fov, float aspect, float nearDist, float farDist) : front{front}, up{up}, fov(fov), aspect(aspect), nearDist(nearDist), farDist(farDist) 
  {
    this->position = position;
    viewMatrix = glm::lookAt(position, position + front, up);
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearDist, farDist);

    // Compute frustum vertices
    glm::vec3 right = glm::normalize(glm::cross(front, up));
    float tanHalfFov = tan(glm::radians(fov / 2.0f));
    float nearHeight = 2 * tanHalfFov * nearDist;
    float farHeight = 2 * tanHalfFov * farDist;
    float nearWidth = nearHeight * aspect;
    float farWidth = farHeight * aspect;

    glm::vec3 nearCenter = front * nearDist;
    glm::vec3 farCenter = front * farDist;

    glm::vec3 nearTopLeft = nearCenter + up * (nearHeight / 2) - right * (nearWidth / 2);
    glm::vec3 nearTopRight = nearCenter + up * (nearHeight / 2) + right * (nearWidth / 2);
    glm::vec3 nearBottomLeft = nearCenter - up * (nearHeight / 2) - right * (nearWidth / 2);
    glm::vec3 nearBottomRight = nearCenter - up * (nearHeight / 2) + right * (nearWidth / 2);

    glm::vec3 farTopLeft = farCenter + up * (farHeight / 2) - right * (farWidth / 2);
    glm::vec3 farTopRight = farCenter + up * (farHeight / 2) + right * (farWidth / 2);
    glm::vec3 farBottomLeft = farCenter - up * (farHeight / 2) - right * (farWidth / 2);
    glm::vec3 farBottomRight = farCenter - up * (farHeight / 2) + right * (farWidth / 2);

    frustumVertices = {
                       nearTopLeft, nearTopRight, nearTopRight, nearBottomRight, nearBottomRight, nearBottomLeft, nearBottomLeft, nearTopLeft,
                       farTopLeft, farTopRight, farTopRight, farBottomRight, farBottomRight, farBottomLeft, farBottomLeft, farTopLeft,
                       nearTopLeft, farTopLeft, nearTopRight, farTopRight, nearBottomRight, farBottomRight, nearBottomLeft, farBottomLeft};

    glGenVertexArrays(1, &frustumVAO);
    glBindVertexArray(frustumVAO);
    glGenBuffers(1, &frustumVBO);
    glBindBuffer(GL_ARRAY_BUFFER, frustumVBO);
    glBufferData(GL_ARRAY_BUFFER, frustumVertices.size() * sizeof(glm::vec3), frustumVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
  };

  void SetAspect(float aspect) {
    this->aspect = aspect;
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearDist, farDist);
  }

  void SetAspect(int wdith, int height) {
    SetAspect((float)wdith / (float)height);
  }

  void SetPos(const glm::vec3& pos) {
    this->position = pos;
    viewMatrix = glm::lookAt(position, position + front, up);
  }

	void LookAt(const glm::vec3& target) {
    glm::vec3 direction = glm::normalize(target - position);
    this->pitch = glm::degrees(asin(direction.y));
    this->yaw = glm::degrees(atan2(direction.z, direction.x));
    UpdateDirection();
	}

  glm::vec3 GetPos() const { return position; }
  void setFront(const glm::vec3& front) {
    this->front = front;
    updateView(position, front, up);
  }

  void update(glm::vec3 position, glm::quat orientation, glm::vec4 fov) {
    updateView(position, orientation);
    updateProjection(fov);
  }

  void updateView(glm::vec3 position, glm::vec3 front, glm::vec3 up) { viewMatrix = glm::lookAt(position, position + front, up); }

  void updateView(glm::vec3 position, glm::quat orientation) { updateView(position, orientation * glm::vec3(0.0f, 0.0f, -1.0f), orientation * glm::vec3(0.0f, 1.0f, 0.0f)); }

  void updateProjection(float fov, float aspect) { projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearDist, farDist); }

  void updateProjection(glm::vec4 fov) {
    // fov = glm::vec4(tan(fov.x), tan(fov.y), tan(fov.z), tan(fov.w));
    fov = glm::tan(fov) * nearDist;
    projectionMatrix = glm::frustum(fov.x, fov.y, fov.z, fov.w, nearDist, farDist);
  }

	void UpdateDirection()
	{
		glm::vec3 dir;
		dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		dir.y = sin(glm::radians(pitch));
		dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		front = glm::normalize(dir);
		updateView(position, front, up);
	}

  glm::mat4 GetViewMatrix() const { return viewMatrix; }
  glm::mat4 GetProjectionMatrix() const { return projectionMatrix; }

  void Move(Direction direction, float dt = 1.0f) {

    const float cameraSpeed = 10.100f;
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 right = glm::normalize(glm::cross(horizontalFront, up));
    glm::vec3 velocity{};
    switch (direction) {
      case Forwards: velocity += horizontalFront; break;
      case Backwards: velocity -= horizontalFront; break;
      case Left: velocity -= right; break;
      case Right: velocity += right; break;
      case Up: velocity += up; break;
      case Down: velocity -= up; break;
    }

    this->position += velocity * (cameraSpeed * dt);
    updateView(position, front, up);
  }
  
  void Render(ShaderProgram& shader) const {
    if (frustumVAO == 0) return;
    shader.Use();
    glm::mat4 model = glm::inverse(viewMatrix);
    shader.SetMat4("model", model);
    glBindVertexArray(frustumVAO);
    glDrawArrays(GL_LINES, 0, frustumVertices.size());
    glBindVertexArray(0);
  }
};

}
