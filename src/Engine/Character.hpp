
#include "bases.h"
#include "Object.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace fe {

class Character : public Object {
public:
	float moveSpeed = 5.0f;
	float jumpSpeed = 7.0f;
	glm::vec3 pendingMovement{};
	bool pendingJump = false;

	Character() {
		this->name = "Character";
	}

	void Move(Direction direction, Camera* camera) {
		if (!camera) return;

		glm::vec3 horizontalFront = glm::normalize(glm::vec3(camera->front.x, 0.0f, camera->front.z));
		glm::vec3 right = glm::normalize(glm::cross(horizontalFront, camera->up));

		switch (direction) {
			case Forwards: pendingMovement += horizontalFront; break;
			case Backwards: pendingMovement -= horizontalFront; break;
			case Left: pendingMovement -= right; break;
			case Right: pendingMovement += right; break;
			case Up: pendingJump = true; break;
			case Down: pendingMovement -= camera->up; break;
		}
	}

	void Update(double deltaTime) override {
		Object::Update(deltaTime);

		if (this->physicsObject) {
			glm::vec3 targetVelocity(0.0f);
			if (glm::length2(pendingMovement) > 0.0001f) {
				targetVelocity = glm::normalize(pendingMovement) * moveSpeed;
			}
			targetVelocity.y = this->state.velocity.y;
			this->physicsObject->SetLinearVelocity(targetVelocity);
			pendingMovement = glm::vec3(0.0f);

			if (pendingJump) {
				this->physicsObject->AddLinearVelocity(glm::vec3(0.0f, jumpSpeed, 0.0f));
				pendingJump = false;
			}
			return;
		}

		if (glm::length2(pendingMovement) > 0.0001f) {
			this->state.position += glm::normalize(pendingMovement) * moveSpeed * static_cast<float>(deltaTime);
		}
		pendingMovement = glm::vec3(0.0f);
		if (pendingJump) {
			this->state.position += glm::vec3(0.0f, jumpSpeed * static_cast<float>(deltaTime), 0.0f);
			pendingJump = false;
		}
	}
};

}
