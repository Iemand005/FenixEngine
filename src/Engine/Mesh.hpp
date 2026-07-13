
#pragma once

#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "Vertex.hpp"
#include "physics/PhysicsObject.hpp"
#include "WawaDir.hpp"
#include "ImageLoader.hpp"

#include "Graphics/IGPUBuffers.hpp"
#include "Graphics/IGPUTexture.hpp"
#include "Graphics/IRenderDevice.hpp"
#include "Graphics/OpenGLGPUBuffers.hpp"
#include "Graphics/OpenGLGPUTexture.hpp"


namespace fe {

	template <typename VertexType = Vertex>
	class Mesh {
		unsigned int indexCount;

	public:
		std::vector<VertexType> vertices;
		std::vector<unsigned int> indices;
		glm::mat4 modelMatrix;

		std::unique_ptr<PhysicsObject> physicsObject = nullptr;

		TextureScaling scaling = TextureScaling::Linear;

		std::unique_ptr<IGPUBuffers> gpuBuffers = nullptr;
		std::unique_ptr<IGPUTexture> gpuTexture = nullptr;

		bool hasTransparency = false;

		IRenderDevice* device_ = nullptr;

		std::string pendingTexturePath;
		TextureScaling pendingTextureScaling = TextureScaling::Linear;
		bool hasPendingTexture = false;

		Mesh() {}

		Mesh(std::vector<VertexType> vertices, std::vector<unsigned int> indices) {
			this->vertices = vertices;
			this->indices = indices;
			this->indexCount = indices.size();
			modelMatrix = glm::mat4(1.0f);
		}

		Mesh(std::string objFilePath, std::string textureFilePath) {
			modelMatrix = glm::mat4(1.0f);

			if (!loadObj(objFilePath) || !loadTexture(textureFilePath)) {
				std::cerr << "Failed to load model or texture" << std::endl;
				return;
			}
		}

		Mesh& operator=(const Mesh& other) {
					if (this != &other) {
							indexCount = other.indexCount;
							vertices = other.vertices;
							indices = other.indices;
							modelMatrix = other.modelMatrix;
							physicsObject = other.physicsObject ? other.physicsObject->Clone() : nullptr;
							gpuBuffers = nullptr;
							gpuTexture = nullptr;
							device_ = other.device_;
							hasPendingTexture = other.hasPendingTexture;
							pendingTexturePath = other.pendingTexturePath;
							pendingTextureScaling = other.pendingTextureScaling;
							if (device_ && !vertices.empty() && !indices.empty()) init();
							if (device_ && hasPendingTexture) {
								gpuTexture = device_->CreateGPUTexture();
								if (gpuTexture) device_->UploadTexture(gpuTexture.get(), pendingTexturePath, pendingTextureScaling);
								hasPendingTexture = false;
							}
					}
					return *this;
			}

		Mesh(Mesh&&) = default;
		Mesh& operator=(Mesh&&) = default;

		Mesh(const Mesh& other)
				: indexCount(other.indexCount),
					vertices(other.vertices),
					indices(other.indices),
					modelMatrix(other.modelMatrix),
					physicsObject(other.physicsObject ? other.physicsObject->Clone() : nullptr),
					device_(other.device_),
					hasPendingTexture(other.hasPendingTexture),
					pendingTexturePath(other.pendingTexturePath),
					pendingTextureScaling(other.pendingTextureScaling) {
			if (device_ && !vertices.empty() && !indices.empty()) init();
			if (device_ && hasPendingTexture) {
				gpuTexture = device_->CreateGPUTexture();
				if (gpuTexture) device_->UploadTexture(gpuTexture.get(), pendingTexturePath, pendingTextureScaling);
				hasPendingTexture = false;
			}
		}

		void SetDevice(IRenderDevice* d) {
			device_ = d;
			if (!device_) return;

			if (!gpuBuffers && !vertices.empty() && !indices.empty()) init();

			if (hasPendingTexture) {
				gpuTexture = device_->CreateGPUTexture();
				if (gpuTexture) {
					device_->UploadTexture(gpuTexture.get(), pendingTexturePath, pendingTextureScaling);
				}
				hasPendingTexture = false;
			}
		}

		void init() {
			if (device_) {
				gpuBuffers = device_->CreateGPUBuffers();
				if (gpuBuffers) {
					device_->UploadBuffers(gpuBuffers.get(),
						vertices.data(), sizeof(VertexType), vertices.size(),
						indices.data(), static_cast<uint32_t>(indices.size()),
						VertexType::getLayout());
				}
			} else {
				auto glBuffers = std::make_unique<OpenGLGPUBuffers>();
				glBuffers->upload(vertices, indices);
				gpuBuffers = std::move(glBuffers);
			}
		}

		bool loadObj(std::string objFilePath);

		bool loadTexture(std::string textureFilePath, TextureScaling newScaling = TextureScaling::Linear) {
			if (device_) {
				gpuTexture = device_->CreateGPUTexture();
				if (gpuTexture) {
					device_->UploadTexture(gpuTexture.get(), textureFilePath, newScaling);
					return true;
				}
				return false;
			}
			pendingTexturePath = textureFilePath;
			pendingTextureScaling = newScaling;
			hasPendingTexture = true;
			return true;
		}

		glm::mat4 getModelMatrix() { return modelMatrix; }

		void SetPhysicsObject(std::unique_ptr<PhysicsObject> physicsObject) { this->physicsObject = std::move(physicsObject); }

		std::vector<Vertex> GetVertices() { return vertices; }
	};

}  // namespace fe
