#pragma once

#include <memory>
#include <glm/glm.hpp>

namespace JPH {
	class PhysicsSystem;
	class TempAllocator;
}

namespace fe {

// Move-and-slide character controller wrapped around Jolt's CharacterVirtual.
// Unlike a rigid body this controller slides along walls and never gets
// pushed through geometry, so it is suitable for a first person player.
class PhysicsCharacter {
public:
	struct Impl;

	PhysicsCharacter();
	~PhysicsCharacter();

	PhysicsCharacter(const PhysicsCharacter&) = delete;
	PhysicsCharacter& operator=(const PhysicsCharacter&) = delete;

	// Builds the capsule shape (bottom of the shape at the character origin)
	// and the backing Jolt character. Returns 0 on success, -1 on failure.
	int Initialize(class JPH::PhysicsSystem* physicsSystem, class JPH::TempAllocator* tempAllocator,
		float height, float radius, const glm::vec3& centerPosition, float maxSlopeAngleDeg = 50.0f);

	// Sets the desired horizontal velocity (world space, m/s) and whether a
	// jump is requested this frame. Consumed by the next Update() call.
	void SetInput(const glm::vec3& desiredHorizontalVelocity, bool wantsJump);

	void SetJumpSpeed(float jumpSpeed);
	float GetJumpSpeed() const;

	// Steps the character by one frame (applies gravity, slides along walls,
	// walks up small steps and sticks to the floor). Call every game update.
	void Update(double deltaTime);

	bool IsSupported() const;

	// Engine-space position of the character, treated as the shape *center*
	// (this matches the old box collider semantics and the +1.6 head offset).
	glm::vec3 GetPosition() const;
	void SetPosition(const glm::vec3& centerPosition);
	glm::vec3 GetFeetPosition() const;

	glm::vec3 GetLinearVelocity() const;
	void SetLinearVelocity(const glm::vec3& velocity);

	float GetHeight() const;
	// Distance from the feet to the center of the shape (height / 2).
	float GetCenterOffset() const;

private:
	std::unique_ptr<Impl> impl;
};

}  // namespace fe