
#pragma once

#include <memory>
#include <vector>

#include "PhysicsObject.hpp"
#include "PhysicsVehicle.hpp"
#include "../Vertex.hpp"

namespace fe {
	class IRenderDevice;

	class PhysicsFactory {
	public:

		struct Impl;
		std::unique_ptr<Impl> impl;

		PhysicsFactory(fe::IRenderDevice* renderDevice, bool enableDebugRenderer = true);
		~PhysicsFactory();


		std::vector<std::unique_ptr<PhysicsObject>> physicsObjects;


		void Update(double dt);

		ObjectState SyncToRender();

		std::unique_ptr<PhysicsObject> CreateObject(glm::vec3 size, bool dynamic = true, bool allowRotation = false);
		std::unique_ptr<PhysicsObject> CreateSphereObject(float radius, bool dynamic = true);
		std::unique_ptr<PhysicsObject> CreateObject(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);
		std::unique_ptr<PhysicsObject> CreateObject(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);

		void RemoveObject(std::unique_ptr<PhysicsObject> object);

		PhysicsVehicle* CreateVehicle(PhysicsObject* body, const std::vector<PhysicsVehicle::WheelConfig>& wheels);

		void EnableGravity();
		void DisableGravity();
		void SetGravity(const glm::vec3& gravity);
		void RenderDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

		void Bind(PhysicsObject *obj);
	};
}
