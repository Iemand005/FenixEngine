
#include "bases.h"
#include "Object.hpp"

namespace fe {

class Character : public Object {
public:
  Character() {
    this->name = "Character";
  }

  void Move(Direction direction, Camera* camera) {

    const float cameraSpeed = 0.0100f;
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(camera->front.x, 0.0f, camera->front.z));
    glm::vec3 right = glm::normalize(glm::cross(horizontalFront, camera->up));
    glm::vec3 velocity{};
    switch (direction) {
      case Forwards: velocity += horizontalFront; break;
      case Backwards: velocity -= horizontalFront; break;
      case Left: velocity -= right; break;
      case Right: velocity += right; break;
      case Up: velocity += camera->up; break;
      case Down: velocity -= camera->up; break;
    }

	if (!this->physicsObject)
      	this->state.position += velocity * cameraSpeed;
    else this->physicsObject->AddPosition(velocity * cameraSpeed);
  }
};

}
