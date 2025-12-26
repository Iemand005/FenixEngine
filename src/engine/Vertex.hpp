#include <glm/glm.hpp>

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TextureCoordinate;

  Vertex() {}

  Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
    this->Position = glm::vec3(x, y, z);
    this->Normal = glm::vec3(nx, ny, nz);
    this->TextureCoordinate = glm::vec2(u, v);
  }
};