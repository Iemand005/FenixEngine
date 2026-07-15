
#pragma once

#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

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

		std::vector<std::string> pendingTextureArrayPaths;
		TextureScaling pendingTextureArrayScaling = TextureScaling::Linear;
		bool hasPendingTextureArray = false;

		Mesh() {}

		Mesh(std::vector<VertexType> vertices, std::vector<unsigned int> indices) {
			this->vertices = std::move(vertices);
			this->indices = std::move(indices);
			this->indexCount = this->indices.size();
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
							hasTransparency = other.hasTransparency;

							hasPendingTexture = other.hasPendingTexture;
							pendingTexturePath = other.pendingTexturePath;
							pendingTextureScaling = other.pendingTextureScaling;

							hasPendingTextureArray = other.hasPendingTextureArray;
							pendingTextureArrayPaths = other.pendingTextureArrayPaths;
							pendingTextureArrayScaling = other.pendingTextureArrayScaling;

							if (device_ && !vertices.empty() && !indices.empty()) init();
							if (device_ && hasPendingTexture) {
								gpuTexture = device_->CreateGPUTexture();
								if (gpuTexture) device_->UploadTexture(gpuTexture.get(), pendingTexturePath, pendingTextureScaling);
								hasPendingTexture = false;
							}
							if (device_ && hasPendingTextureArray) {
								gpuTexture = device_->CreateGPUTexture();
								if (gpuTexture) device_->UploadTextureArray(gpuTexture.get(), pendingTextureArrayPaths, pendingTextureArrayScaling);
								hasPendingTextureArray = false;
							}
					}
					return *this;
			}

		Mesh(Mesh&& other) noexcept
			: indexCount(other.indexCount),
			  vertices(std::move(other.vertices)),
			  indices(std::move(other.indices)),
			  modelMatrix(other.modelMatrix),
			  physicsObject(std::move(other.physicsObject)),
			  scaling(other.scaling),
			  gpuBuffers(std::move(other.gpuBuffers)),
			  gpuTexture(std::move(other.gpuTexture)),
			  hasTransparency(other.hasTransparency),
			  device_(other.device_),
			  pendingTexturePath(std::move(other.pendingTexturePath)),
			  pendingTextureScaling(other.pendingTextureScaling),
			  hasPendingTexture(other.hasPendingTexture),
			  pendingTextureArrayPaths(std::move(other.pendingTextureArrayPaths)),
			  pendingTextureArrayScaling(other.pendingTextureArrayScaling),
			  hasPendingTextureArray(other.hasPendingTextureArray) {}

		Mesh& operator=(Mesh&& other) noexcept {
			if (this != &other) {
				indexCount = other.indexCount;
				vertices = std::move(other.vertices);
				indices = std::move(other.indices);
				modelMatrix = other.modelMatrix;
				physicsObject = std::move(other.physicsObject);
				scaling = other.scaling;
				gpuBuffers = std::move(other.gpuBuffers);
				gpuTexture = std::move(other.gpuTexture);
				hasTransparency = other.hasTransparency;
				device_ = other.device_;
				pendingTexturePath = std::move(other.pendingTexturePath);
				pendingTextureScaling = other.pendingTextureScaling;
				hasPendingTexture = other.hasPendingTexture;
				pendingTextureArrayPaths = std::move(other.pendingTextureArrayPaths);
				pendingTextureArrayScaling = other.pendingTextureArrayScaling;
				hasPendingTextureArray = other.hasPendingTextureArray;
			}
			return *this;
		}

		Mesh(const Mesh& other)
				: indexCount(other.indexCount),
					vertices(other.vertices),
					indices(other.indices),
					modelMatrix(other.modelMatrix),
					physicsObject(other.physicsObject ? other.physicsObject->Clone() : nullptr),
					device_(other.device_),
					hasTransparency(other.hasTransparency),
					hasPendingTexture(other.hasPendingTexture),
					pendingTexturePath(other.pendingTexturePath),
					pendingTextureScaling(other.pendingTextureScaling),
					hasPendingTextureArray(other.hasPendingTextureArray),
					pendingTextureArrayPaths(other.pendingTextureArrayPaths),
					pendingTextureArrayScaling(other.pendingTextureArrayScaling) {
			if (device_ && !vertices.empty() && !indices.empty()) init();
			if (device_ && hasPendingTexture) {
				gpuTexture = device_->CreateGPUTexture();
				if (gpuTexture) device_->UploadTexture(gpuTexture.get(), pendingTexturePath, pendingTextureScaling);
				hasPendingTexture = false;
			}
			if (device_ && hasPendingTextureArray) {
				gpuTexture = device_->CreateGPUTexture();
				if (gpuTexture) device_->UploadTextureArray(gpuTexture.get(), pendingTextureArrayPaths, pendingTextureArrayScaling);
				hasPendingTextureArray = false;
			}
		}

		void SetDevice(IRenderDevice* d) {
			device_ = d;
			if (!device_) return;

			if (!gpuBuffers && !vertices.empty() && !indices.empty()) {
				init();
			}

			if (hasPendingTextureArray) {
				gpuTexture = device_->CreateGPUTexture();
				if (gpuTexture) {
					device_->UploadTextureArray(gpuTexture.get(), pendingTextureArrayPaths, pendingTextureArrayScaling);
				}
				hasPendingTextureArray = false;
			}

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
				if constexpr (std::is_same_v<VertexType, VertexArray>)
					gpuBuffers->vertexFormat = VertexFormat::Array;
				else
					gpuBuffers->vertexFormat = VertexFormat::Standard;

				device_->UploadBuffers(gpuBuffers.get(),
					vertices.data(), sizeof(VertexType), vertices.size(),
					indices.data(), static_cast<uint32_t>(indices.size()),
					VertexType::getLayout());
			}
		} else {
			auto glBuffers = std::make_unique<OpenGLGPUBuffers>();
			glBuffers->upload(vertices, indices);
			if constexpr (std::is_same_v<VertexType, VertexArray>)
				glBuffers->vertexFormat = VertexFormat::Array;
			else
				glBuffers->vertexFormat = VertexFormat::Standard;
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

		bool loadTextureArray(const std::vector<std::string>& textureFilePaths, TextureScaling newScaling = TextureScaling::Linear) {
			if (textureFilePaths.empty()) return false;

			if (device_) {
				gpuTexture = device_->CreateGPUTexture();
				if (gpuTexture) {
					device_->UploadTextureArray(gpuTexture.get(), textureFilePaths, newScaling);
					return true;
				}
				return false;
			}

			pendingTextureArrayPaths = textureFilePaths;
			pendingTextureArrayScaling = newScaling;
			hasPendingTextureArray = true;
			return true;
		}

		void CopyToGPU() {
			init();
		}

		void RemoveFromGPU() {
			gpuBuffers.reset();
			gpuTexture.reset();
		}

		void FreeCpuData() {
			vertices.clear();
			vertices.shrink_to_fit();
			indices.clear();
			indices.shrink_to_fit();
		}

		Mesh Clone() const {
			Mesh copy;
			copy.vertices = vertices;
			copy.indices = indices;
			copy.indexCount = indexCount;
			copy.modelMatrix = modelMatrix;
			copy.hasTransparency = hasTransparency;
			copy.scaling = scaling;
			copy.device_ = device_;

			if (device_ && !vertices.empty() && !indices.empty()) {
				copy.init();
			}

			if (hasPendingTextureArray) {
				copy.pendingTextureArrayPaths = pendingTextureArrayPaths;
				copy.pendingTextureArrayScaling = pendingTextureArrayScaling;
				copy.hasPendingTextureArray = true;
				if (device_) {
					copy.gpuTexture = device_->CreateGPUTexture();
					if (copy.gpuTexture) {
						device_->UploadTextureArray(copy.gpuTexture.get(), pendingTextureArrayPaths, pendingTextureArrayScaling);
						copy.hasPendingTextureArray = false;
					}
				}
			} else if (hasPendingTexture) {
				copy.pendingTexturePath = pendingTexturePath;
				copy.pendingTextureScaling = pendingTextureScaling;
				copy.hasPendingTexture = true;
				if (device_) {
					copy.gpuTexture = device_->CreateGPUTexture();
					if (copy.gpuTexture) {
						device_->UploadTexture(copy.gpuTexture.get(), pendingTexturePath, pendingTextureScaling);
						copy.hasPendingTexture = false;
					}
				}
			}

			return copy;
		}

		glm::mat4 getModelMatrix() { return modelMatrix; }

		const std::vector<VertexType>& GetVertices() const { return vertices; }
		const std::vector<unsigned int>& GetIndices() const { return indices; }

		void SetPhysicsObject(std::unique_ptr<PhysicsObject> obj) { physicsObject = std::move(obj); }
	};

}