#pragma once
#include <glm/glm.hpp>

namespace fe {

struct VertexArray {
 public:
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 texCoord;

  VertexArray() {}

  VertexArray(float x, float y, float z, float nx, float ny, float nz, float u, float v, float layer) {
    this->position = glm::vec3(x, y, z);
    this->normal = glm::vec3(nx, ny, nz);
    this->texCoord = glm::vec3(u, v, layer);
  }
};

}