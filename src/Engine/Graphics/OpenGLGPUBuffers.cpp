
#include <glad/glad.h>

#include "OpenGLGPUBuffers.hpp"

OpenGLGPUBuffers::~OpenGLGPUBuffers() {
	if (ebo != 0) glDeleteBuffers(1, &ebo);
	if (vbo != 0) glDeleteBuffers(1, &vbo);
	if (vao != 0) glDeleteVertexArrays(1, &vao);
}

void OpenGLGPUBuffers::bind() const {
	glBindVertexArray(vao);
}

GLenum OpenGLGPUBuffers::toGLType(fe::VertexAttribType type) {
	switch (type) {
		case fe::VertexAttribType::Short: return GL_SHORT;
		case fe::VertexAttribType::UByte: return GL_UNSIGNED_BYTE;
		default: return GL_FLOAT;
	}
}
