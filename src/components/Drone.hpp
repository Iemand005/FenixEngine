#pragma once

#include <memory>

#include <glm/glm.hpp>

namespace fe {

class Object;
class PhysicsFactory;
class PhysicsObject;
class Scene;

class Drone {
public:
	struct Settings {
		float thrust = 30.0f;
		float lift = 18.0f;
		float gravityCompensation = 20.0f;
		float linearDamping = 1.2f;
		float angularDamping = 4.0f;
		bool altitudeHold = true;
	};

	Drone() = default;
	~Drone();

	Drone(const Drone&) = delete;
	Drone& operator=(const Drone&) = delete;

	bool Spawn(Scene& scene, const glm::vec3& position);
	bool Enter(PhysicsFactory& physics);
	void Exit();
	void Despawn();

	void Update(const glm::vec2& horizontalInput, float verticalInput, float yawRadians);

	bool IsSpawned() const;
	bool IsActive() const;
	std::shared_ptr<Object> GetDroneObject() const;
	PhysicsObject* GetBody() const;
	Settings& GetSettings();
	const Settings& GetSettings() const;

private:
	Scene* scene_ = nullptr;
	std::shared_ptr<Object> object_;
	PhysicsObject* body_ = nullptr;
	Settings settings_;
};

}
