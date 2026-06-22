#pragma once
#include <glm/glm.hpp>

namespace fe {

struct Vertex {
 public:
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 uv;

  Vertex() {}

  Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
    this->position = glm::vec3(x, y, z);
    this->normal = glm::vec3(nx, ny, nz);
    this->uv = glm::vec2(u, v);
  }
};

}