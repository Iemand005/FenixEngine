
#pragma once

#include <memory>

#include "PhysicsObject.hpp"

namespace fe{

	class PhysicsFactory {
	public:

		struct Impl;
		std::unique_ptr<Impl> impl;

		PhysicsFactory();
		~PhysicsFactory();


		std::vector<std::unique_ptr<PhysicsObject>> physicsObjects;


		void Update(double dt);

		ObjectState SyncToRender();

		std::unique_ptr<PhysicsObject> CreateObject(glm::vec3 size, bool dynamic = true);
		std::unique_ptr<PhysicsObject> CreateObject(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);
		
		void RemoveObject(std::unique_ptr<PhysicsObject> object);

		void EnableGravity();
		void DisableGravity();
		void RenderDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

		void Bind(PhysicsObject *obj);
	};
}
