
#pragma once
#include <glad/glad.h>

#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "ShaderProgram.hpp"
#include "Vertex.hpp"
#include "physics/PhysicsObject.hpp"

namespace fe {

	enum class TextureScaling {
		Linear,
		Nearest
	};

	class Mesh {
		unsigned int indexCount;

		unsigned int vao = 0;
		unsigned int VBO = 0;
		unsigned int EBO = 0;
		unsigned int texture = 0;

	public:
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		glm::mat4 modelMatrix;

		std::unique_ptr<PhysicsObject> physicsObject = nullptr;

		TextureScaling scaling = TextureScaling::Linear;

		bool hasTransparency = false;

		Mesh() {}

		Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
			this->vertices = vertices;
			this->indices = indices;
			this->indexCount = indices.size();
			modelMatrix = glm::mat4(1.0f);
			init();
		}

		Mesh(std::string objFilePath, std::string textureFilePath) {
			modelMatrix = glm::mat4(1.0f);

			if (!loadObj(objFilePath) || !loadTexture(textureFilePath)) {
				std::cerr << "Failed to load model or texture" << std::endl;
				return;
			}

			init();
		}

		Mesh& operator=(const Mesh& other) {
					if (this != &other) {
							indexCount = other.indexCount;
							vao = other.vao;
							VBO = other.VBO;
							EBO = other.EBO;
							texture = other.texture;
							vertices = other.vertices;
							indices = other.indices;
							modelMatrix = other.modelMatrix;
							physicsObject = other.physicsObject ? other.physicsObject->Clone() : nullptr;
					}
					return *this;
			}

		Mesh(Mesh&&) = default;
		Mesh& operator=(Mesh&&) = default;

		Mesh(const Mesh& other)
				: indexCount(other.indexCount),
					vao(other.vao),
					VBO(other.VBO),
					EBO(other.EBO),
					texture(other.texture),
					vertices(other.vertices),
					indices(other.indices),
					modelMatrix(other.modelMatrix),
					physicsObject(other.physicsObject ? other.physicsObject->Clone() : nullptr) {}

		void init() {
			unsigned int vao, VBO, EBO;
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			this->vao = vao;

			glGenBuffers(1, &VBO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
			this->VBO = VBO;

			glGenBuffers(1, &EBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);
			this->EBO = EBO;

			int vertexStride = sizeof(Vertex);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)0);
			glEnableVertexAttribArray(0);

			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)(3 * sizeof(float)));
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride, (void*)(6 * sizeof(float)));
			glEnableVertexAttribArray(2);

			glBindVertexArray(0);
		}

		bool loadObj(std::string objFilePath)
		/*{
			objl::Loader objectLoader;

			bool success = objectLoader.LoadFile(objFilePath);
			if (!success) return false;

			this->vertices = std::vector<Vertex>(objectLoader.LoadedVertices.size());

			for (int i = 0; i < this->vertices.size(); i++) {
				objl::Vertex v = objectLoader.LoadedVertices[i];
				this->vertices[i] = Vertex(v.Position.X, v.Position.Y, v.Position.Z, v.Normal.X, v.Normal.Y, v.Normal.Z, v.uv.X, v.uv.Y);
			}

			this->indices = std::vector<unsigned int>(objectLoader.LoadedIndices.size());

			for (size_t i = 0; i < this->indices.size(); i++) this->indices[i] = objectLoader.LoadedIndices[i];

			return true;
		}*/;

		bool loadTextureFile(std::string textureFilePath, int& width, int& height, int& nrChannels, unsigned char*& data) {
			stbi_set_flip_vertically_on_load(true);
			data = stbi_load(textureFilePath.c_str(), &width, &height, &nrChannels, 0);
			if (!data) {
				std::cerr << "Failed to load texture" << std::endl;
				return false;
			}
			return true;
		}

		bool loadTexture(std::string textureFilePath = NULL, TextureScaling newScaling = TextureScaling::Linear) {
			// if (textureFilePath == NULL)
			//   return false;
			int width, height, nrChannels;
			unsigned char* data;
			if (!loadTextureFile(textureFilePath, width, height, nrChannels, data)) return false;

			glGenTextures(1, &this->texture);
			glBindTexture(GL_TEXTURE_2D, this->texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			GLint texScaling = GL_LINEAR;
			if (newScaling == TextureScaling::Nearest) texScaling = GL_NEAREST;

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texScaling);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texScaling);

			GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			stbi_image_free(data);
			glBindTexture(GL_TEXTURE_2D, 0);
			return true;
		}

		glm::mat4 getModelMatrix() { return modelMatrix; }

		void Render(ShaderProgram& shader) { Render(shader, this->getModelMatrix()); }

		void PrepareRender(ShaderProgram& shader) {
			if (vao == 0) return;
			shader.Use();
			glBindVertexArray(vao);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);

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

		void SetPhysicsObject(std::unique_ptr<PhysicsObject> physicsObject) { physicsObject = std::move(physicsObject); }

		std::vector<Vertex> GetVertices() { return vertices; }
	};

}  // namespace fe