
#pragma once

#include "IRenderDevice.hpp"
#include "OpenGLGPUBuffers.hpp"
#include "OpenGLGPUTexture.hpp"

namespace fe {

class OpenGLRenderDevice : public IRenderDevice {

	void Init(fe::IWindow *window) override {

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

	void UploadBuffers(IGPUBuffers* buffers,
		const void* vertices, size_t vertexStride, size_t vertexCount,
		const uint32_t* indices, uint32_t indexCount) override {

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

		glBindVertexArray(0);
	}

	void DrawMesh(const IGPUBuffers* buffers, const IGPUTexture* texture = nullptr) override {
		if (!buffers) return;

		const auto* glBuffers = static_cast<const OpenGLGPUBuffers*>(buffers);

		glBuffers->bind(); 

		if (texture) {
			const auto* glTexture = static_cast<const OpenGLGPUTexture*>(texture);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, glTexture->textureId);
		}

		glDrawElements(GL_TRIANGLES, glBuffers->indexCount, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		if (texture) {
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void UploadTexture(IGPUTexture* texture,
		const std::string& path, TextureScaling scaling = TextureScaling::Linear) override {
		if (!texture) return;
		auto* glTexture = static_cast<OpenGLGPUTexture*>(texture);
		glTexture->load(path, scaling);
	}


	void SubmitFrame() override {
		glFlush();
	}
};

}
