#pragma once

#include "bases.h"

#include "Object.hpp"
#include "Camera.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>

namespace fe {

class Character : public Object {
public:
	float moveSpeed = 5.0f;
	float jumpSpeed = 8.0f;
	float jumpHeightWhenGravityDisabled = 0.15f;
	float groundCheckDistance = 0.15f;
	glm::vec3 pendingMovement{};
	bool pendingJump = false;
	bool jumpTriggered = false;
	bool gravityEnabled = true;
	bool isGrounded = false;

	Character() {
		this->name = "Character";
	}

	bool IsGrounded() const {
		return isGrounded;
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
			case Up: {
				if (gravityEnabled)
					pendingJump = true;
				else pendingMovement += camera->up; break;
				break;
			}
			case Down: pendingMovement -= camera->up; break;
		}
	}

	void Update(double deltaTime) override {
		ObjectBase::Update(deltaTime);
		isGrounded = false;

		if (this->physicsObject) {
			isGrounded = true; // TODO: use JPH contact listener for proper ground check

			glm::vec3 targetVelocity(0.0f);
			if (glm::length2(pendingMovement) > 0.0001f) {
				targetVelocity = glm::normalize(pendingMovement) * moveSpeed;
			}

			if (pendingJump && isGrounded && !jumpTriggered) {
				targetVelocity.y = jumpSpeed;
				jumpTriggered = true;
			} else if (gravityEnabled) {
				targetVelocity.y = this->state.velocity.y;
			}
			if (!pendingJump && jumpTriggered)
				jumpTriggered = false;

			this->physicsObject->SetLinearVelocity(targetVelocity);
			pendingMovement = glm::vec3(0.0f);
			pendingJump = false;
			return;
		}

		if (glm::length2(pendingMovement) > 0.0001f) {
			this->state.position += glm::normalize(pendingMovement) * moveSpeed * static_cast<float>(deltaTime);
		}
		pendingMovement = glm::vec3(0.0f);
		if (pendingJump && isGrounded) {
			this->state.position += glm::vec3(0.0f, jumpHeightWhenGravityDisabled, 0.0f);
			pendingJump = false;
		}
		jumpTriggered = false;
	}
};

}
