#pragma once

#include <glad/glad.h>
#include <vector>

#include "IGPUBuffers.hpp"

class OpenGLGPUBuffers : public IGPUBuffers {
public:
	unsigned int vao = 0, vbo = 0, ebo = 0;

public:

	OpenGLGPUBuffers() = default;

	~OpenGLGPUBuffers() override {
        if (ebo != 0) glDeleteBuffers(1, &ebo);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (vao != 0) glDeleteVertexArrays(1, &vao);
    }

	void bind() const override {
        glBindVertexArray(vao);
    }

	unsigned int getVAO() const { return vao; }

	template<typename VertexType>
	void upload(const std::vector<VertexType>& vertices, const std::vector<unsigned int>& indices) {
		indexCount = static_cast<int>(indices.size());

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexType), vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

		auto layout = VertexType::getLayout();
		for (const auto& attr : layout) {
			glVertexAttribPointer(attr.location, attr.components, GL_FLOAT, GL_FALSE, sizeof(VertexType), (void*)attr.offset);
			glEnableVertexAttribArray(attr.location);
		}

		glBindVertexArray(0);
	}
};