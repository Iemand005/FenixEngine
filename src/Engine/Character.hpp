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
	float jumpSpeed = 0.15f;
	float jumpHeightWhenGravityDisabled = 0.15f;
	float groundCheckDistance = 0.15f;
	glm::vec3 pendingMovement{};
	bool pendingJump = false;
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
		Object::Update(deltaTime);
		isGrounded = false;

		if (this->physicsObject) {
			glm::vec3 currentPos = this->physicsObject->GetPosition();
			glm::vec3 down = glm::vec3(0.0f, -1.0f, 0.0f);
			glm::vec3 probePos = currentPos + down * (1.0f + groundCheckDistance);
			isGrounded = currentPos.y <= groundCheckDistance + 0.01f;
			if (!isGrounded) {
				isGrounded = this->state.position.y <= groundCheckDistance + 0.01f;
			}
			isGrounded = true; // TODO: add JPH contact lsiterner and use that to determine if norma l of conatc t is up
			glm::vec3 targetVelocity(0.0f);
			if (glm::length2(pendingMovement) > 0.0001f) {
				targetVelocity = glm::normalize(pendingMovement) * moveSpeed;
			}
			targetVelocity.y = gravityEnabled ? this->state.velocity.y : 0.0f;
			this->physicsObject->SetLinearVelocity(targetVelocity);
			pendingMovement = glm::vec3(0.0f);

			if (pendingJump && isGrounded) {
				if (gravityEnabled) {
					this->physicsObject->AddLinearVelocity(glm::vec3(0.0f, jumpSpeed, 0.0f));
					// this->physicsObject->SetLinearVelocity
				} else {
					this->physicsObject->AddPosition(glm::vec3(0.0f, jumpHeightWhenGravityDisabled, 0.0f));
					this->state.position += glm::vec3(0.0f, jumpHeightWhenGravityDisabled, 0.0f);
				}
				pendingJump = false;
			}
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
	}
};

}
