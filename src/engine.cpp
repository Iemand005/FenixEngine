#define STB_IMAGE_IMPLEMENTATION
#include "engine.h"

namespace fe {


  Camera::Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float fov, float aspect, float nearDist, float farDist) : position(position), front{front}, up{up}, fov(fov), aspect(aspect), nearDist(nearDist), farDist(farDist)
    {
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
          // near plane
          nearTopLeft, nearTopRight,
          nearTopRight, nearBottomRight,
          nearBottomRight, nearBottomLeft,
          nearBottomLeft, nearTopLeft,
          // far plane
          farTopLeft, farTopRight,
          farTopRight, farBottomRight,
          farBottomRight, farBottomLeft,
          farBottomLeft, farTopLeft,
          // sides
          nearTopLeft, farTopLeft,
          nearTopRight, farTopRight,
          nearBottomRight, farBottomRight,
          nearBottomLeft, farBottomLeft};

      glGenVertexArrays(1, &frustumVAO);
      glBindVertexArray(frustumVAO);
      glGenBuffers(1, &frustumVBO);
      glBindBuffer(GL_ARRAY_BUFFER, frustumVBO);
      glBufferData(GL_ARRAY_BUFFER, frustumVertices.size() * sizeof(glm::vec3), frustumVertices.data(), GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
      glEnableVertexAttribArray(0);
      glBindVertexArray(0);
    }
}