#include <iostream>

#include "PhysicsCharacter.hpp"

#ifndef EXCLUDE_JOLT

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Geometry/Plane.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
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
	JPH::Vec3 desiredHorizontal{0.0f, 0.0f, 0.0f};
	bool wantsJump = false;
	float jumpSpeed = 7.0f;
	float height = 1.0f;
	float centerOffset = 0.5f;
#endif
};

PhysicsCharacter::PhysicsCharacter() = default;

PhysicsCharacter::~PhysicsCharacter() = default;

int PhysicsCharacter::Initialize(JPH::PhysicsSystem* physicsSystem, JPH::TempAllocator* tempAllocator,
	float height, float radius, const glm::vec3& centerPosition, float maxSlopeAngleDeg) {
#ifndef EXCLUDE_JOLT
	if (physicsSystem == nullptr || tempAllocator == nullptr || height < 0.001f || radius < 0.001f)
		return -1;

	impl = std::make_unique<Impl>();
	impl->physicsSystem = physicsSystem;
	impl->tempAllocator = tempAllocator;
	impl->height = height;
	impl->centerOffset = 0.5f * height;

	// Build a capsule that is exactly `height` long with flat-ish top/bottom at
	// (0,0,0). Jolt expects the bottom of the character shape on the origin, so
	// the capsule is shifted up by (half height + radius).
	float capsuleHalfHeight = 0.5f * height - radius;
	auto shape = RotatedTranslatedShapeSettings(
		Vec3(0.0f, impl->centerOffset, 0.0f), Quat::sIdentity(),
		new CapsuleShape(capsuleHalfHeight, radius)).Create();
	if (shape.HasError())
	{
		std::cerr << "PhysicsCharacter: failed to create capsule shape: " << shape.GetError() << std::endl;
		return -1;
	}

	Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
	settings->mShape = shape.Get();
	// Only contacts that touch the lower part of the capsule count as
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

void PhysicsCharacter::SetInput(const glm::vec3& desiredHorizontalVelocity, bool wantsJump) {
#ifndef EXCLUDE_JOLT
	if (!impl) return;
	impl->desiredHorizontal = Vec3(desiredHorizontalVelocity.x, 0.0f, desiredHorizontalVelocity.z);
	impl->wantsJump = wantsJump;
#endif
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

	const float dt = static_cast<float>(deltaTime);
	const Vec3 up = Vec3::sAxisY();
	const Vec3 gravity = impl->physicsSystem->GetGravity();

	// Determine new velocity, mirroring CharacterVirtualTest::HandleInput so
	// that the character stops cleanly against walls and never pops through.
	const Vec3 currentVelocity = impl->character->GetLinearVelocity();
	const Vec3 currentVerticalVelocity = up * currentVelocity.Dot(up);
	const Vec3 groundVelocity = impl->character->GetGroundVelocity();
	const bool movingTowardsGround = (currentVerticalVelocity.GetY() - groundVelocity.GetY()) < 0.1f;

	Vec3 newVelocity;
	if (impl->character->GetGroundState() == CharacterVirtual::EGroundState::OnGround && movingTowardsGround)
	{
		newVelocity = groundVelocity;
		if (impl->wantsJump)
			newVelocity += impl->jumpSpeed * up;
	}
	else
	{
		newVelocity = currentVerticalVelocity;
	}

	// Apply gravity to the vertical velocity.
	newVelocity += gravity * dt;

	// Horizontal input: on the ground use the desired velocity, in the air keep
	// the momentum the player already has.
	if (impl->character->IsSupported())
		newVelocity += impl->desiredHorizontal;
	else
		newVelocity += currentVelocity - currentVerticalVelocity;

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