#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Object.hpp"
#include "ShaderProgram.hpp"

namespace fe {
class Camera {
 private:
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::mat4 viewMatrix;
  unsigned int frustumVAO = 0, frustumVBO = 0;
  std::vector<glm::vec3> frustumVertices;

 public:
  float fov, aspect, nearDist, farDist;
  glm::mat4 projectionMatrix;

  Camera() {}

  Camera(float nearDist, float farDist) : nearDist(nearDist), farDist(farDist) {};

  Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float fov, float aspect, float nearDist, float farDist);

  void setAspect(float aspect) {
    this->aspect = aspect;
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearDist, farDist);
  }

  void setPos(const glm::vec3& pos) {
    this->position = pos;
    viewMatrix = glm::lookAt(position, position + front, up);
  }
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
  glm::mat4 getViewMatrix() const { return viewMatrix; }
  glm::mat4 getProjectionMatrix() const { return projectionMatrix; }
  void render(ShaderProgram& shader) const {
    if (frustumVAO == 0) return;
    shader.use();
    glm::mat4 model = glm::inverse(viewMatrix);
    shader.setMat4("model", model);
    glBindVertexArray(frustumVAO);
    glDrawArrays(GL_LINES, 0, frustumVertices.size());
    glBindVertexArray(0);
  }
};

}