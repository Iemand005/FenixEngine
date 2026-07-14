#pragma once
#define WIN32_LEAN_AND_MEAN
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
#include "ShaderProgram.hpp"
#include "physics/PhysicsEngine.hpp"


//#ifndef STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
//#endif  // !STB_IMAGE_IMPLEMENTATION

//#define OBJ_LOADER
//#include "OBJ_Loader.h"

namespace fe {

class ObjectBase {
public:
	ObjectState state{};
	glm::mat4 modelMatrix{1.0f};

	bool isStatic = false;
	bool touchedGround = false;
	bool touchedOtherObject = false;

	std::unique_ptr<PhysicsObject> physicsObject = nullptr;
	std::string sourcePath;
	std::string name = "unkle";
	std::shared_ptr<ShaderProgram> shader = nullptr;

	ObjectBase() { state.scale = glm::vec3(1.0f); }
	virtual ~ObjectBase() = default;

	virtual void Update(double deltaTime) {
		if (this->physicsObject) {
			auto s = this->physicsObject->SyncToRender();
			this->state = s;
		}
	}

	virtual void Render(IRenderDevice* device) {}

	virtual size_t GetMeshCount() const { return 0; }
	virtual size_t GetTotalVertexCount() const { return 0; }

	glm::mat4 GetModelMatrix() {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), this->state.position);
		model = glm::rotate(model, glm::radians(this->state.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(this->state.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, this->state.scale);
		return model;
	}

	void SetPhysicsObject(std::unique_ptr<PhysicsObject> obj) { physicsObject = std::move(obj); }
};

template <typename VertexType = Vertex>
class Object : public ObjectBase {
public:
	std::vector<Mesh<VertexType>> meshes;

	unsigned int boundingBoxVAO = 0, boundingBoxVBO = 0;
	std::vector<glm::vec3> boundingBoxVertices;

	Object() : ObjectBase() {}

	Object(Mesh<VertexType> mesh) : ObjectBase() {
		if (mesh.physicsObject) {
			this->physicsObject = std::move(mesh.physicsObject);
		}
		meshes.push_back(std::move(mesh));
	}

	Object(std::string objFilePath, float scale = 1.0f) : ObjectBase() {
		LoadObj(objFilePath, scale);
		std::filesystem::path path(objFilePath);
		name = path.filename().string();
		sourcePath = objFilePath;
	}

	Object(std::string objFilePath, ObjectState state) : ObjectBase() { LoadObj(objFilePath);
		this->state = state;
		sourcePath = objFilePath;
	}

	size_t GetMeshCount() const override { return meshes.size(); }
	size_t GetTotalVertexCount() const override {
		size_t total = 0;
		for (const auto& m : meshes) total += m.vertices.size();
		return total;
	}

	void Render(IRenderDevice* device) override {
		for (auto& mesh : meshes) {
			mesh.SetDevice(device);
			device->DrawMesh(mesh.gpuBuffers.get(), mesh.gpuTexture.get());
		}
	}

	bool LoadObj(std::string path, float scale = 1.0f);

	std::shared_ptr<Object> Clone() const {
		auto newObj = std::make_shared<Object>();
		newObj->meshes.clear();
		newObj->meshes.reserve(meshes.size());
		for (const auto& m : meshes) {
			newObj->meshes.push_back(m.Clone());
		}
		newObj->state = this->state;
		newObj->state.scale = this->state.scale;
		newObj->name = this->name;
		newObj->sourcePath = this->sourcePath;
		newObj->isStatic = this->isStatic;
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
