
#include "Object.hpp"

namespace fe {

enum Direction {
  Forwards,
  Backwards,
  Left,
  Right
};

class Character : public Object {
public:
  void Move(Direction direction, Camera* camera) {

    const float cameraSpeed = 0.0100f;
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(camera->front.x, 0.0f, camera->front.z));
    glm::vec3 right = glm::normalize(glm::cross(horizontalFront, camera->up));
    glm::vec3 velocity{};
    switch (direction) {
      case Forwards:
        velocity += horizontalFront;
        break;
      case Backwards:
        velocity -= horizontalFront;
        break;
      case Left:
        velocity -= right;
        break;
      case Right:
        velocity += right;
        break;
    }

    this->physicsObject->AddPosition(velocity * cameraSpeed);
  }
};

}