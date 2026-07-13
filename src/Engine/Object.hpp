#pragma once
#include <glad/glad.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "bases.h"
#include "Mesh.hpp"
#include "MeshArray.hpp"
#include "ShaderProgram.hpp"
#include "physics/PhysicsEngine.hpp"


//#ifndef STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
//#endif  // !STB_IMAGE_IMPLEMENTATION

//#define OBJ_LOADER
//#include "OBJ_Loader.h"

namespace fe {

		/*struct ObjectState {

};*/

class Object {
private:
public:
	ObjectState state{};
	glm::mat4 modelMatrix;

	std::vector<Mesh<>> meshes;
	std::vector<MeshArray> meshArrays;

	bool isStatic = false;

	bool touchedGround = false;

	bool touchedOtherObject = false;

	unsigned int boundingBoxVAO = 0, boundingBoxVBO = 0;

	std::vector<glm::vec3> boundingBoxVertices;

	std::unique_ptr<fe::PhysicsObject> physicsObject = nullptr;

	std::string sourcePath;

	std::string name = "unkle";

	std::shared_ptr<ShaderProgram> shader = nullptr;

	Object() {
		//acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
		state.scale = glm::vec3(1.0f);
		meshes = std::vector<Mesh<>>();
		meshArrays = std::vector<MeshArray>();
	}

	Object(Mesh<> mesh) : Object() {
		if (mesh.physicsObject) {
			this->physicsObject = std::move(mesh.physicsObject);
		}
		meshes.push_back(std::move(mesh));
	}

	Object(MeshArray mesh) : Object() {
		if (mesh.physicsObject)
			this->physicsObject = std::move(mesh.physicsObject);
		meshArrays.emplace_back(std::move(mesh));
	}

	Object(std::string objFilePath, float scale = 1.0f) : Object() {
		LoadObj(objFilePath, scale);
		std::filesystem::path path(objFilePath);
		name = path.filename().string();
		sourcePath = objFilePath;
	}

	Object(std::string objFilePath, ObjectState state) : Object() { LoadObj(objFilePath);
		this->state = state;
		sourcePath = objFilePath;
	}

	bool LoadObj(std::string path, float scale = 1.0f);

	void SetPhysicsObject(std::unique_ptr<PhysicsObject> physicsObject) { this->physicsObject = std::move(physicsObject); }

	virtual void Update(double deltaTime) {
		if (this->physicsObject) {
			auto state = this->physicsObject->SyncToRender();
			this->state = state;
		}
	}

	glm::mat4 GetModelMatrix() {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), this->state.position);
		model = glm::rotate(model, glm::radians(this->state.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(this->state.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, this->state.scale);
		return model;
	}

	void Render(ShaderProgram& shader) {
		for (auto& mesh : meshes) mesh.Render(shader, this->GetModelMatrix());
		for (auto& mesh : meshArrays) mesh.Render(shader, this->GetModelMatrix());
		if (boundingBoxVAO && touchedOtherObject) {
			shader.Use();
			shader.SetMat4("model", this->GetModelMatrix());
			glBindVertexArray(boundingBoxVAO);
			glDrawArrays(GL_LINES, 0, boundingBoxVertices.size());
			glBindVertexArray(0);
		}
	}

	std::shared_ptr<Object> Clone() const {
		auto newObj = std::make_shared<Object>();
		newObj->meshes = this->meshes;
		// newObj->meshes.clear();
		// newObj->meshes.reserve(meshes.size());
		// for (const auto& m : meshes) {
		// 	newObj->meshes.push_back(m.Clone());
		// }
		// newObj->meshArrays = this->meshArrays;
		newObj->meshArrays.clear();
		newObj->meshArrays.reserve(meshArrays.size());
		for (const auto& m : meshArrays) {
			newObj->meshArrays.push_back(m.Clone());
		}
		newObj->state.scale = this->state.scale;
		return newObj;
	}

		void LookAt(const glm::vec3& target) {
		glm::vec3 direction = glm::normalize(target - this->state.position);
		float pitch = glm::degrees(asin(direction.y));
		float yaw = glm::degrees(atan2(direction.x, direction.z));
		this->state.rotation.x = pitch;
		this->state.rotation.y = yaw - 180;
	}

	std::string GetName() {
		return name;
	}
};

}
