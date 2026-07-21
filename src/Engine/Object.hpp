#pragma once
#define WIN32_LEAN_AND_MEAN
#include <glad/glad.h>

#include <cstdio>
#include <memory>
#include <string>
#include <filesystem>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "bases.h"
#include "IMesh.hpp"
#include "ShaderProgram.hpp"
#include "Graphics/IRenderDevice.hpp"

namespace fe {

template<typename VertexType> class Mesh;

class ObjectBase {
public:
	ObjectState state{};
	glm::mat4 modelMatrix{1.0f};
	float boundingRadius = 0.0f;
	glm::vec3 boundingCenterOffset{0.0f};

	bool isStatic = false;
	bool touchedGround = false;
	bool touchedOtherObject = false;

	glm::vec3 color{1.0f, 1.0f, 1.0f};

	std::unique_ptr<PhysicsObject> physicsObject = nullptr;
	std::string sourcePath;
	std::string name = "unkle";
	std::shared_ptr<ShaderProgram> shader = nullptr;

	ObjectBase() { state.scale = glm::vec3(1.0f); }
	virtual ~ObjectBase() = default;

	virtual void Update(double deltaTime) {
		if (this->physicsObject && !this->isStatic) {
			auto s = this->physicsObject->SyncToRender();
			this->state = s;
		}
	}

	virtual void Render(IRenderDevice* device) {}

	virtual size_t GetMeshCount() const { return 0; }
	virtual size_t GetTotalVertexCount() const { return 0; }

	glm::mat4 GetModelMatrix() {
		glm::mat4 model = glm::translate(glm::mat4(1.0f), this->state.position);
		if (glm::length(this->state.orientation - glm::quat(1, 0, 0, 0)) > 0.001f) {
			model = model * glm::mat4_cast(this->state.orientation);
		} else {
			model = glm::rotate(model, glm::radians(this->state.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(this->state.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		}
		model = glm::scale(model, this->state.scale);
		return model;
	}

	void SetPhysicsObject(std::unique_ptr<PhysicsObject> obj) { physicsObject = std::move(obj); }
};

class Object : public ObjectBase {
public:
	std::vector<std::unique_ptr<IMesh>> meshes;
	Object* parent = nullptr;
    std::vector<std::unique_ptr<Object>> children;

	unsigned int boundingBoxVAO = 0, boundingBoxVBO = 0;
	std::vector<glm::vec3> boundingBoxVertices;

	Object() : ObjectBase() {}

	template<typename VertexType>
	explicit Object(Mesh<VertexType> mesh) : ObjectBase() {
		PushMesh(std::move(mesh));
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

	Object* GetParent() const { return parent; }
	const std::vector<std::unique_ptr<Object>>& GetChildren() const { return children; }

	Object* AddChild(std::unique_ptr<Object> child) {
        child->parent = this;
        children.push_back(std::move(child));
        return children.back().get();
    }

    void RemoveChild(Object* child) {
        auto it = std::find_if(children.begin(), children.end(),
            [child](const std::unique_ptr<Object>& c) { return c.get() == child; });
        if (it != children.end()) {
            (*it)->parent = nullptr;
            children.erase(it);
        }
    }

	template<typename VertexType>
	void PushMesh(Mesh<VertexType>&& mesh) {
		if (mesh.physicsObject) {
			this->physicsObject = std::move(mesh.physicsObject);
		}
		meshes.push_back(std::make_unique<Mesh<VertexType>>(std::move(mesh)));
	}

	template<typename VertexType>
	Mesh<VertexType>& EmplaceMesh(std::vector<VertexType> vertices, std::vector<unsigned int> indices) {
		auto mesh = std::make_unique<Mesh<VertexType>>(std::move(vertices), std::move(indices));
		auto& ref = *mesh;
		meshes.push_back(std::move(mesh));
		return ref;
	}

	size_t GetMeshCount() const override { return meshes.size(); }
	size_t GetTotalVertexCount() const override {
		size_t total = 0;
		for (const auto& m : meshes) total += m->GetVertexCount();
		return total;
	}

	void Render(IRenderDevice* device) override {
		for (auto& mesh : meshes) {
			mesh->SetDevice(device);
			device->DrawMesh(mesh->GetGPUBuffers(), mesh->GetGPUTexture());
		}
	}

	bool LoadObj(std::string path, float scale = 1.0f);

	std::shared_ptr<Object> Clone() const {
		auto newObj = std::make_shared<Object>();
		newObj->meshes.reserve(meshes.size());
		for (const auto& m : meshes) {
			newObj->meshes.push_back(m->Clone());
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

// Explicit template instantiation for common types
#include "Mesh.hpp"
