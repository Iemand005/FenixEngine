#include "Drone.hpp"

#include "../Object.hpp"
#include "../Primitives.hpp"
#include "../Scene.hpp"
#include "../physics/PhysicsFactory.hpp"
#include "../physics/PhysicsObject.hpp"

#include <cmath>
#include <utility>

namespace fe {

Drone::~Drone() {
	Despawn();
}

bool Drone::Spawn(Scene& scene, const glm::vec3& position) {
	if (object_)
		return false;

	auto object = std::make_shared<Object>(Primitives::GenerateCube(
		{PlaneDirection::Front, PlaneDirection::Back,
		 PlaneDirection::Left, PlaneDirection::Right,
		 PlaneDirection::Top, PlaneDirection::Bottom},
		0.6f));
	object->name = "Drone";
	object->color = glm::vec3(1.0f, 0.45f, 0.0f);
	object->state.position = position;
	object->isStatic = true;

	scene_ = &scene;
	object_ = std::move(object);
	scene_->AddObject(object_);
	return true;
}

bool Drone::Enter(PhysicsFactory& physics) {
	if (!object_)
		return false;
	if (body_)
		return true;

	object_->SetPhysicsObject(physics.CreateSphereObject(0.3f, true));
	body_ = object_->physicsObject.get();
	if (!body_)
		return false;

	body_->SetPosition(object_->state.position);
	body_->SetRotation(object_->state.orientation);
	body_->SetLinearVelocity(glm::vec3(0.0f));
	body_->SetAngularVelocity(glm::vec3(0.0f));
	body_->SetDamping(settings_.linearDamping, settings_.angularDamping);
	object_->isStatic = false;
	return true;
}

void Drone::Exit() {
	if (!object_ || !body_)
		return;

	object_->SetPhysicsObject(nullptr);
	body_ = nullptr;
	object_->isStatic = true;
}

void Drone::Despawn() {
	Exit();
	if (scene_ && object_)
		scene_->RemoveObject(object_);
	object_.reset();
	scene_ = nullptr;
}

void Drone::Update(const glm::vec2& horizontalInput, float verticalInput, float yawRadians) {
	if (!body_)
		return;

	body_->SetDamping(settings_.linearDamping, settings_.angularDamping);
	const glm::vec3 forward(std::cos(yawRadians), 0.0f, std::sin(yawRadians));
	const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
	glm::vec3 horizontalForce = forward * horizontalInput.y + right * horizontalInput.x;
	const float magnitude = glm::length(horizontalForce);
	if (magnitude > 1.0f)
		horizontalForce /= magnitude;
	horizontalForce *= settings_.thrust;

	float verticalForce = verticalInput * settings_.lift;
	if (settings_.altitudeHold)
		verticalForce += settings_.gravityCompensation;

	body_->AddForce(horizontalForce + glm::vec3(0.0f, verticalForce, 0.0f));
}

bool Drone::IsSpawned() const {
	return static_cast<bool>(object_);
}

bool Drone::IsActive() const {
	return body_ != nullptr;
}

std::shared_ptr<Object> Drone::GetDroneObject() const {
	return object_;
}

PhysicsObject* Drone::GetBody() const {
	return body_;
}

Drone::Settings& Drone::GetSettings() {
	return settings_;
}

const Drone::Settings& Drone::GetSettings() const {
	return settings_;
}

}
