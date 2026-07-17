
#pragma once

#include <iostream>

#include "IRenderDevice.hpp"
#include "OpenGLGPUBuffers.hpp"
#include "OpenGLGPUTexture.hpp"

namespace fe {


class OpenGLRenderDevice : public IRenderDevice {

	void Init(fe::IWindow *window) override {
// 11	
		EnableDepthTest();
		EnableFaceCulling();
	}

	void Clear() override {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void SetClearColor(float r, float g, float b, float a = 1) override {
		glClearColor(r, g, b, a);
	}

	void Resize(int width, int height) override {
		glViewport(0, 0, width, height);
	}

	std::unique_ptr<IGPUBuffers> CreateGPUBuffers() override {
		return std::make_unique<OpenGLGPUBuffers>();
	}

	std::unique_ptr<IGPUTexture> CreateGPUTexture() override {
		return std::make_unique<OpenGLGPUTexture>();
	}

	void EnableDepthTest() { glEnable(GL_DEPTH_TEST); }
	void DisableDepthTest() { glDisable(GL_DEPTH_TEST); }
	void EnableFaceCulling() { glEnable(GL_CULL_FACE); }
	void DisableFaceCulling() { glDisable(GL_CULL_FACE); }

	void UploadBuffers(IGPUBuffers* buffers,
		const void* vertices, size_t vertexStride, size_t vertexCount,
		const uint32_t* indices, uint32_t indexCount,
		const std::vector<fe::VertexAttribute>& layout = {}) override {

		if (!buffers) return;

		auto* glBuffers = static_cast<OpenGLGPUBuffers*>(buffers);
		glBuffers->indexCount = static_cast<int>(indexCount);

		glGenVertexArrays(1, &glBuffers->vao);
		glBindVertexArray(glBuffers->vao);

		glGenBuffers(1, &glBuffers->vbo);
		glBindBuffer(GL_ARRAY_BUFFER, glBuffers->vbo);
		glBufferData(GL_ARRAY_BUFFER, vertexStride * vertexCount, vertices, GL_STATIC_DRAW);

		glGenBuffers(1, &glBuffers->ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glBuffers->ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * indexCount, indices, GL_STATIC_DRAW);

		for (const auto& attr : layout) {
			glVertexAttribPointer(attr.location, attr.components, GL_FLOAT, GL_FALSE,
				static_cast<GLsizei>(vertexStride), (void*)attr.offset);
			glEnableVertexAttribArray(attr.location);
		}

		glBindVertexArray(0);

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			std::cerr << "[GL ERROR] UploadBuffers: 0x" << std::hex << err << std::dec << std::endl;
		}
	}

	void DrawMesh(const IGPUBuffers* buffers, const IGPUTexture* texture = nullptr) override {
		if (!buffers) return;

		GLint currentProgram = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
		if (currentProgram == 0) {
			return;
		}

		const auto* glBuffers = static_cast<const OpenGLGPUBuffers*>(buffers);

		if (glBuffers->vao == 0) {
			return;
		}

		glBuffers->bind(); 

		if (texture) {
			const auto* glTexture = static_cast<const OpenGLGPUTexture*>(texture);
			glActiveTexture(GL_TEXTURE0);
			if (glTexture->isTextureArray()) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, glTexture->textureId);
			} else {
				glBindTexture(GL_TEXTURE_2D, glTexture->textureId);
			}
		}

		glDrawElements(GL_TRIANGLES, glBuffers->indexCount, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		if (texture) {
			const auto* glTexture = static_cast<const OpenGLGPUTexture*>(texture);
			if (glTexture->isTextureArray()) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
			} else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	}

	void UploadTexture(IGPUTexture* texture,
		const std::string& path, TextureScaling scaling = TextureScaling::Linear) override {
		if (!texture) return;
		auto* glTexture = static_cast<OpenGLGPUTexture*>(texture);
		bool ok = glTexture->load(path, scaling);
		if (!ok) {
			std::cerr << "UploadTexture: failed to load " << path << std::endl;
		}
	}

	void UploadTextureArray(IGPUTexture* texture,
		const std::vector<std::string>& paths, TextureScaling scaling = TextureScaling::Linear) override {
		if (!texture) return;
		auto* glTexture = static_cast<OpenGLGPUTexture*>(texture);
		glTexture->loadTextureArray(paths, scaling);
	}


	void SubmitFrame() override {
		glFlush();
	}
};

}
