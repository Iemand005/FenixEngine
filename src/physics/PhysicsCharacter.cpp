#include <iostream>

#include "PhysicsCharacter.hpp"

#ifndef EXCLUDE_JOLT

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Geometry/Plane.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#endif

using namespace fe;

#ifndef EXCLUDE_JOLT
using namespace JPH;

// Object layer used by the static terrain bodies the player walks on. The
// engine uses one broadphase layer and all filters return true, so any layer
// would work; NON_MOVING keeps the character consistent with the world.
static constexpr ObjectLayer CHARACTER_OBJECT_LAYER = 0;
#endif

struct PhysicsCharacter::Impl {
#ifndef EXCLUDE_JOLT
	JPH::Ref<JPH::CharacterVirtual> character;
	JPH::PhysicsSystem* physicsSystem = nullptr;
	JPH::TempAllocator* tempAllocator = nullptr;
	JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
	JPH::Vec3 inputDirection{0.0f, 0.0f, 0.0f};
	bool wantsJump = false;
	float jumpSpeed = 8.5f;
	float height = 1.0f;
	float centerOffset = 0.5f;
	MovementSettings movement{};
#endif
};

PhysicsCharacter::PhysicsCharacter() = default;

PhysicsCharacter::~PhysicsCharacter() = default;

int PhysicsCharacter::Initialize(JPH::PhysicsSystem* physicsSystem, JPH::TempAllocator* tempAllocator,
	float height, float radius, const glm::vec3& centerPosition,
	bool rectangularHitbox, float maxSlopeAngleDeg) {
#ifndef EXCLUDE_JOLT
	if (physicsSystem == nullptr || tempAllocator == nullptr || height < 0.001f || radius < 0.001f)
		return -1;

	impl = std::make_unique<Impl>();
	impl->physicsSystem = physicsSystem;
	impl->tempAllocator = tempAllocator;
	impl->height = height;
	impl->centerOffset = 0.5f * height;

	// Build a shape that is exactly `height` tall with its bottom at (0,0,0).
	// Jolt expects the bottom of the character shape on the origin, so the
	// shape is shifted up by half the height.
	const Shape *baseShape = nullptr;
	if (rectangularHitbox)
		baseShape = new BoxShape(Vec3(radius, 0.5f * height, radius));
	else
		baseShape = new CapsuleShape(0.5f * height - radius, radius);

	auto shape = RotatedTranslatedShapeSettings(
		Vec3(0.0f, impl->centerOffset, 0.0f), Quat::sIdentity(), baseShape).Create();
	if (shape.HasError())
	{
		std::cerr << "PhysicsCharacter: failed to create character shape: " << shape.GetError() << std::endl;
		return -1;
	}

	Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
	settings->mShape = shape.Get();
	// Only contacts that touch the lower part of the shape count as
	// supporting the character (same as the Jolt samples).
	settings->mSupportingVolume = Plane(Vec3::sAxisY(), -radius);
	settings->mMaxSlopeAngle = maxSlopeAngleDeg * (JPH_PI / 180.0f);
	settings->mUp = Vec3::sAxisY();
	settings->mEnhancedInternalEdgeRemoval = true;
	settings->mBackFaceMode = EBackFaceMode::CollideWithBackFaces;
	settings->mPredictiveContactDistance = 0.1f;
	settings->mCharacterPadding = 0.02f;
	settings->mPenetrationRecoverySpeed = 1.0f;

	Vec3 feet(centerPosition.x, centerPosition.y - impl->centerOffset, centerPosition.z);
	impl->character = new CharacterVirtual(settings, feet, Quat::sIdentity(), 0, physicsSystem);

	return impl->character != nullptr ? 0 : -1;
#else
	return -1;
#endif
}

void PhysicsCharacter::SetInput(const glm::vec3& inputDirection, bool wantsJump) {
#ifndef EXCLUDE_JOLT
	if (!impl) return;
	impl->inputDirection = Vec3(inputDirection.x, 0.0f, inputDirection.z);
	impl->wantsJump = wantsJump;
#endif
}

void PhysicsCharacter::SetMovementSettings(const MovementSettings& settings) {
#ifndef EXCLUDE_JOLT
	if (!impl) return;
	impl->movement = settings;
#endif
}

const PhysicsCharacter::MovementSettings& PhysicsCharacter::GetMovementSettings() const {
	static const MovementSettings fallback{};
#ifndef EXCLUDE_JOLT
	if (impl) return impl->movement;
#endif
	return fallback;
}

void PhysicsCharacter::SetJumpSpeed(float jumpSpeed) {
#ifndef EXCLUDE_JOLT
	if (!impl) return;
	impl->jumpSpeed = jumpSpeed;
#endif
}

float PhysicsCharacter::GetJumpSpeed() const {
#ifndef EXCLUDE_JOLT
	if (impl) return impl->jumpSpeed;
#endif
	return 0.0f;
}

void PhysicsCharacter::Update(double deltaTime) {
#ifndef EXCLUDE_JOLT
	if (!impl || !impl->character || impl->physicsSystem == nullptr || impl->tempAllocator == nullptr)
		return;

	// Minecraft's movement runs on a fixed 20 Hz tick. The acceleration and
	// drag constants are authored per tick, so we scale them by however many
	// ticks this frame is worth; the resulting top speed is the same at 20, 60
	// or 240 fps. A stall (window drag, load, breakpoint) is clamped, because
	// integrating a second of movement in one step would launch the player
	// through the terrain.
	static constexpr double MOVEMENT_TICK_SECONDS = 0.05;
	static constexpr double MAX_DELTA_TIME = 0.1;
	const float dt = static_cast<float>(std::min(deltaTime, MAX_DELTA_TIME));
	const float ticks = dt / static_cast<float>(MOVEMENT_TICK_SECONDS);

	const Vec3 up = Vec3::sAxisY();
	const Vec3 gravity = impl->physicsSystem->GetGravity();
	const RVec3 startPosition = impl->character->GetPosition();

	// Determine new velocity, mirroring CharacterVirtualTest::HandleInput so
	// that the character stops cleanly against walls and never pops through.
	const Vec3 currentVelocity = impl->character->GetLinearVelocity();
	const Vec3 currentVerticalVelocity = up * currentVelocity.Dot(up);
	const Vec3 groundVelocity = impl->character->GetGroundVelocity();
	const bool movingTowardsGround = (currentVerticalVelocity.GetY() - groundVelocity.GetY()) < 0.1f;
	const bool onGround = impl->character->GetGroundState() == CharacterVirtual::EGroundState::OnGround;

	Vec3 verticalVelocity;
	if (onGround && movingTowardsGround)
	{
		verticalVelocity = up * groundVelocity.Dot(up);
		if (impl->wantsJump)
			verticalVelocity += impl->jumpSpeed * up;
	}
	else
	{
		verticalVelocity = currentVerticalVelocity;
	}

	// Apply gravity to the vertical velocity.
	verticalVelocity += gravity * dt;

	// Horizontal movement is acceleration plus drag, not "set velocity to the
	// walk speed". Each tick the velocity we ended last tick with is carried
	// forward, friction retains a fraction of it, and the input direction
	// adds a fixed amount on top:
	//
	//     v = v * retention + inputDirection * acceleration
	//
	// The terminal speed that falls out of that, acceleration / (1 -
	// retention), is what keeps normal walking around 4.32 m/s even though the
	// per-tick numbers look tiny. Friction is applied to the velocity only,
	// never to the input, which is why the input is added afterwards.
	const MovementSettings& movement = impl->movement;
	const bool supported = onGround && impl->character->IsSupported();
	const float retention = supported ? movement.groundVelocityRetention : movement.airVelocityRetention;
	const float acceleration = supported ? movement.groundAcceleration : movement.airAcceleration;
	// A moving platform carries the player itself; only the velocity the
	// player added on top of it should be dragged and accelerated.
	const Vec3 groundHorizontal = groundVelocity - up * groundVelocity.Dot(up);
	const Vec3 currentHorizontal = currentVelocity - up * currentVelocity.Dot(up);
	const Vec3 carriedVelocity = currentHorizontal - groundHorizontal;
	// Clamped so a pathologically long frame cannot invert the velocity.
	const float retainFactor = std::clamp(1.0f - (1.0f - retention) * ticks, 0.0f, 1.0f);
	const Vec3 horizontalVelocity = carriedVelocity * retainFactor + impl->inputDirection * (acceleration * ticks) + groundHorizontal;

	const Vec3 newVelocity = verticalVelocity + horizontalVelocity;
	impl->character->SetLinearVelocity(newVelocity);

	impl->character->ExtendedUpdate(
		dt,
		gravity,
		impl->updateSettings,
		impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(CHARACTER_OBJECT_LAYER),
		impl->physicsSystem->GetDefaultLayerFilter(CHARACTER_OBJECT_LAYER),
		{ },
		{ },
		*impl->tempAllocator);

	// ExtendedUpdate only blocks the position; it leaves the stored velocity
	// pointing into the obstacle. Drop the component of the velocity that
	// pushes into any surface we were actually touching when the frame started
	// (walls/floor the jump departs from) so we keep momentum loss there.
	Vec3 postVelocity = impl->character->GetLinearVelocity();
	for (const CharacterContact &contact : impl->character->GetActiveContacts())
		if (contact.mHadCollision && !contact.mWasDiscarded)
		{
			const Vec3 normal = contact.mSurfaceNormal;
			const float intoContact = postVelocity.Dot(normal);
			if (intoContact < 0.0f)
				postVelocity -= intoContact * normal;
		}

	// A ceiling hit found mid-sweep never lands in GetActiveContacts(), so the
	// loop above misses it. Catch it with the achieved movement instead: if we
	// asked to rise this frame but were blocked from rising, kill the upward
	// velocity so the player drops instead of pushing into the ceiling forever.
	const float requestedDeltaUp = newVelocity.Dot(up) * dt;
	const float achievedDeltaUp = static_cast<float>(impl->character->GetPosition().GetY() - startPosition.GetY());
	if (requestedDeltaUp > 0.05f && achievedDeltaUp < 0.75f * requestedDeltaUp)
		postVelocity = Vec3(postVelocity.GetX(), postVelocity.GetY() < 0.0f ? postVelocity.GetY() : 0.0f, postVelocity.GetZ());
	impl->character->SetLinearVelocity(postVelocity);

	impl->wantsJump = false;
#endif
}

bool PhysicsCharacter::IsSupported() const {
#ifndef EXCLUDE_JOLT
	if (impl && impl->character) return impl->character->IsSupported();
#endif
	return false;
}

glm::vec3 PhysicsCharacter::GetPosition() const {
#ifndef EXCLUDE_JOLT
	if (impl && impl->character)
	{
		const RVec3 feet = impl->character->GetPosition();
		return glm::vec3(static_cast<float>(feet.GetX()),
			static_cast<float>(feet.GetY() + impl->centerOffset),
			static_cast<float>(feet.GetZ()));
	}
#endif
	return glm::vec3(0.0f);
}

void PhysicsCharacter::SetPosition(const glm::vec3& centerPosition) {
#ifndef EXCLUDE_JOLT
	if (!impl || !impl->character) return;
	impl->character->SetPosition(RVec3(centerPosition.x,
		centerPosition.y - impl->centerOffset, centerPosition.z));
	impl->character->RefreshContacts(
		impl->physicsSystem->GetDefaultBroadPhaseLayerFilter(CHARACTER_OBJECT_LAYER),
		impl->physicsSystem->GetDefaultLayerFilter(CHARACTER_OBJECT_LAYER),
		{ }, { }, *impl->tempAllocator);
#endif
}

glm::vec3 PhysicsCharacter::GetFeetPosition() const {
#ifndef EXCLUDE_JOLT
	if (impl && impl->character)
	{
		const RVec3 feet = impl->character->GetPosition();
		return glm::vec3(static_cast<float>(feet.GetX()),
			static_cast<float>(feet.GetY()),
			static_cast<float>(feet.GetZ()));
	}
#endif
	return glm::vec3(0.0f);
}

glm::vec3 PhysicsCharacter::GetLinearVelocity() const {
#ifndef EXCLUDE_JOLT
	if (impl && impl->character)
	{
		const Vec3 v = impl->character->GetLinearVelocity();
		return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
	}
#endif
	return glm::vec3(0.0f);
}

void PhysicsCharacter::SetLinearVelocity(const glm::vec3& velocity) {
#ifndef EXCLUDE_JOLT
	if (!impl || !impl->character) return;
	impl->character->SetLinearVelocity(Vec3(velocity.x, velocity.y, velocity.z));
#endif
}

float PhysicsCharacter::GetHeight() const {
#ifndef EXCLUDE_JOLT
	if (impl) return impl->height;
#endif
	return 0.0f;
}

float PhysicsCharacter::GetCenterOffset() const {
#ifndef EXCLUDE_JOLT
	if (impl) return impl->centerOffset;
#endif
	return 0.0f;
}