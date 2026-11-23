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
	// Horizontal movement model, in Minecraft's per-tick (20 Hz) units:
	// acceleration is blocks/tick^2 and retention is the fraction of the
	// velocity that survives one tick of friction. Top speed is never set
	// directly, it falls out of the pair:
	//
	//     top speed (blocks/tick) = acceleration / (1 - retention)
	//
	// so on a normal block (retention 0.546) the defaults converge to
	// 0.216 blocks/tick (~4.32 m/s) walking and 0.281 (~5.61 m/s) sprinting.
	// The acceleration is intentionally separate from the retention so a
	// slippery surface (ice) can keep a sane walking speed while making the
	// player slide, exactly like Minecraft does.
	struct MovementSettings {
		float groundAcceleration = 0.098f;
		float airAcceleration = 0.0196f;
		// Block slipperiness * 0.91, so 0.6 * 0.91 for a normal block.
		float groundVelocityRetention = 0.546f;
		float airVelocityRetention = 0.91f;
	};

	struct Impl;

	PhysicsCharacter();
	~PhysicsCharacter();

	PhysicsCharacter(const PhysicsCharacter&) = delete;
	PhysicsCharacter& operator=(const PhysicsCharacter&) = delete;

	// Builds the capsule (or box when `rectangularHitbox` is true) shape (bottom
	// of the shape at the character origin) and the backing Jolt character.
	// `radius` is the capsule radius / half box width. Returns 0 on success,
	// -1 on failure.
	int Initialize(class JPH::PhysicsSystem* physicsSystem, class JPH::TempAllocator* tempAllocator,
		float height, float radius, const glm::vec3& centerPosition,
		bool rectangularHitbox = false, float maxSlopeAngleDeg = 50.0f);

	// Sets the direction the player is trying to move in (world space, unit
	// length, y is ignored) and whether a jump is requested this frame. The
	// magnitude scales the acceleration, so an analogue stick below 1.0
	// accelerates proportionally. How fast that ends up being is decided by
	// SetMovementSettings, not by this call. Consumed by the next Update().
	void SetInput(const glm::vec3& inputDirection, bool wantsJump);

	void SetMovementSettings(const MovementSettings& settings);
	const MovementSettings& GetMovementSettings() const;

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