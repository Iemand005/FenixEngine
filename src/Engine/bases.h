#pragma once
#include <glm/glm.hpp>

struct ObjectState {
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 velocity;
  glm::vec3 scale;
};

enum Direction {
  Forwards,
  Backwards,
  Left,
  Right,
  Up,
  Down
};
