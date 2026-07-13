#pragma once

#include <glad/glad.h>


#include "IGPUBuffers.hpp"

class OpenGLGPUBuffers : public IGPUBuffers {
	unsigned int vao = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
	unsigned int texture = 0;

public:

	~OpenGLGPUBuffers() override {
        if (EBO != 0) glDeleteBuffers(1, &ebo);
        if (vbo != 0) glDeleteBuffers(1, &vbo);
        if (vao != 0) glDeleteVertexArrays(1, &vao);
    }

	void bind() const override {
        glBindVertexArray(vao); // Activeert de VAO en alle gekoppelde attributen
    }
};