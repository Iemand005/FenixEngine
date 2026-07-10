
#pragma once
#include <glad/glad.h>

#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

// #include <stb_image.h>

#include "ShaderProgram.hpp"
#include "VertexArray.hpp"
#include "physics/PhysicsObject.hpp"
#include "WawaDir.hpp"



namespace fe {

	class MeshArray {
		unsigned int indexCount;

		unsigned int vao = 0;
		unsigned int VBO = 0;
		unsigned int EBO = 0;
		unsigned int texture = 0;

	public:
		std::vector<VertexArray> vertices;
		std::vector<unsigned int> indices;
		glm::mat4 modelMatrix;

		std::unique_ptr<PhysicsObject> physicsObject = nullptr;

		TextureScaling scaling = TextureScaling::Linear;

		bool hasTransparency = false;

		MeshArray() {}

		MeshArray(std::vector<VertexArray> vertices, std::vector<unsigned int> indices, bool copyToGPU = true) : vertices(std::move(vertices)), indices(std::move(indices)) {
			this->indexCount = this->indices.size();
			modelMatrix = glm::mat4(1.0f);
			if (copyToGPU) CopyToGPU();
		}

		

		MeshArray(MeshArray&&) = default;
		MeshArray(const MeshArray&) = delete;
    	MeshArray& operator=(const MeshArray&) = delete;
		MeshArray& operator=(MeshArray&& other) noexcept {
			if (this != &other) {
				RemoveFromGPU(); // clean up whatever *this* currently owns

				vertices = std::move(other.vertices);
				indices  = std::move(other.indices);
				vao = other.vao;
				VBO = other.VBO;
				EBO = other.EBO;

				other.vao = 0;
				other.VBO = 0;
				other.EBO = 0;
			}
			return *this;
		}

		// MeshArray(const MeshArray& other) : indexCount(other.indexCount), vao(other.vao), VBO(other.VBO), EBO(other.EBO), texture(other.texture), vertices(other.vertices), indices(other.indices), modelMatrix(other.modelMatrix), physicsObject(other.physicsObject ? other.physicsObject->Clone() : nullptr) {}

		// q``q	1	1		1``			`
		~MeshArray() {
			RemoveFromGPU();
		}

		void CopyToGPU() {
			unsigned int vao, VBO, EBO;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			this->vao = vao;

			glGenBuffers(1, &VBO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexArray), vertices.data(), GL_STATIC_DRAW);
			this->VBO = VBO;

			glGenBuffers(1, &EBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);
			this->EBO = EBO;

			int vertexStride = sizeof(VertexArray);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)(6 * sizeof(float)));
			glEnableVertexAttribArray(2);

			glBindVertexArray(0);
		}

		void RemoveFromGPU() {
			if (vao) {
				glDeleteVertexArrays(1, &vao); vao = 0;
			}
			if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
			if (EBO) { glDeleteBuffers(1, &EBO); EBO = 0; }
		}

		void FreeCpuData() {
			vertices.clear();
			vertices.shrink_to_fit();
			indices.clear();
			indices.shrink_to_fit();
		}

		MeshArray Clone() const {
			MeshArray copy;
			copy.vertices = vertices;
			copy.indices = indices;
			copy.CopyToGPU();
			return copy;
		}

		bool loadObj(std::string objFilePath);

		static std::unordered_map<std::string, unsigned int>& GetTextureArrayCache() {
			static std::unordered_map<std::string, unsigned int> cache;
			return cache;
		}

		bool loadTextureFile(std::string textureFilePath, int& width, int& height, int& nrChannels, std::vector<unsigned char>& data) {
			auto image = fe::ImageLoader::Load(textureFilePath);
			if (image.pixels.empty()) {
				return false;
			}

			width = image.width;
			height = image.height;
			nrChannels = image.channels;
			data = std::move(image.pixels);
			return true;
		}

		bool loadTextureArray(const std::vector<std::string>& textureFilePaths, TextureScaling newScaling = TextureScaling::Nearest) {
			if (textureFilePaths.empty()) return false;

			std::string cacheKey;
			for (const auto& texturePath : textureFilePaths) {
				cacheKey += texturePath;
				cacheKey += '\0';
			}
			cacheKey += std::to_string(static_cast<int>(newScaling));

			auto& cache = GetTextureArrayCache();
			auto cachedIt = cache.find(cacheKey);
			if (cachedIt != cache.end()) {
				this->texture = cachedIt->second;
				return true;
			}

			int baseWidth, baseHeight, nrChannels;
			std::vector<unsigned char> firstData;
			if (!loadTextureFile(textureFilePaths[0], baseWidth, baseHeight, nrChannels, firstData)) {
				return false;
			}
			
			GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
			GLint internalFormat = (nrChannels == 4) ? GL_RGBA8 : GL_RGB8;
			int layerCount = static_cast<int>(textureFilePaths.size());

			glGenTextures(1, &this->texture);
			glBindTexture(GL_TEXTURE_2D_ARRAY, this->texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, baseWidth, baseHeight, layerCount, 0, format, GL_UNSIGNED_BYTE, nullptr);

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, baseWidth, baseHeight, 1, format, GL_UNSIGNED_BYTE, firstData.data());

			for (int i = 1; i < layerCount; ++i) {
				int width, height, channels;
				std::vector<unsigned char> data;
				if (loadTextureFile(textureFilePaths[i], width, height, channels, data)) {
					if (width == baseWidth && height == baseHeight && channels == nrChannels) {
						glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, baseWidth, baseHeight, 1, format, GL_UNSIGNED_BYTE, data.data());
					}
				}
			}

			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

			GLint texScaling = (newScaling == TextureScaling::Nearest) ? GL_NEAREST : GL_LINEAR;
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, texScaling); 
			glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, texScaling);

			cache[cacheKey] = this->texture;
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			return true;
		}


		glm::mat4 getModelMatrix() { return modelMatrix; }

		void Render(ShaderProgram& shader) { Render(shader, this->getModelMatrix()); }

		void PrepareRender(ShaderProgram& shader) {
			if (vao == 0) return;
			shader.Use();
			glBindVertexArray(vao);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, texture);

			if (hasTransparency) {
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
				glDepthMask(GL_FALSE); // Don't depth mask transparent objects but might be useful to be able to turn this off?
			}
		}

		void EndRender() {
			if (hasTransparency) {
				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
			}
		}

		void Draw() { glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0); }

		void Render(ShaderProgram& shader, glm::mat4 modelMatrix) {
			PrepareRender(shader);
			shader.SetMat4("model", modelMatrix);
			Draw();
			EndRender();
		}

		void RenderInstanced(ShaderProgram& shader, const std::vector<glm::mat4>& modelMatrices) {
			PrepareRender(shader);

			for (const auto& modelMatrix : modelMatrices) {
				shader.SetMat4("model", modelMatrix);
				Draw();
			}

			EndRender();
		}

		void SetPhysicsObject(std::unique_ptr<PhysicsObject> physicsObject) { this->physicsObject = std::move(physicsObject); }

		std::vector<VertexArray> GetVertices() { return vertices; }
	};

}  // namespace fe
