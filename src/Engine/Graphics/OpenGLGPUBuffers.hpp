#pragma once

#include "IGPUBuffers.hpp"

class OpenGLGPUBuffers : public IGPUBuffers {
	unsigned int vao = 0;
	unsigned int VBO = 0;
	unsigned int EBO = 0;
	unsigned int texture = 0;

public:
};