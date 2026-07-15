
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

		GLint eboAfterBind = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboAfterBind);

		glBindVertexArray(0);

		GLint eboAfterUnbind = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboAfterUnbind);

		std::cerr << "[UploadBuffers] vao=" << glBuffers->vao
				  << " vbo=" << glBuffers->vbo
				  << " ebo=" << glBuffers->ebo
				  << " verts=" << vertexCount
				  << " indices=" << indexCount
				  << " stride=" << vertexStride
				  << " eboWhileVAO=" << eboAfterBind
				  << " eboAfterUnbind=" << eboAfterUnbind
				  << std::endl;

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			std::cerr << "[GL ERROR] UploadBuffers: 0x" << std::hex << err << std::dec << std::endl;
		}
	}

	static void CheckAndLog(const char* label) {
		GLenum e;
		while ((e = glGetError()) != GL_NO_ERROR) {
			std::cerr << "[GL ERROR] " << label << " -> 0x" << std::hex << e << std::dec << std::endl;
		}
	}

	void DrawMesh(const IGPUBuffers* buffers, const IGPUTexture* texture = nullptr) override {
		if (!buffers) return;

		GLint currentProgram = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
		if (currentProgram == 0) {
			static bool logged = false;
			if (!logged) {
				std::cerr << "[GL] DrawMesh: no shader program bound, skipping draw" << std::endl;
				logged = true;
			}
			return;
		}

		const auto* glBuffers = static_cast<const OpenGLGPUBuffers*>(buffers);

		if (glBuffers->vao == 0) {
			std::cerr << "[GL] DrawMesh: vao is 0! indexCount=" << glBuffers->indexCount << std::endl;
			return;
		}

		while (glGetError() != GL_NO_ERROR) {}

		glBuffers->bind(); 
		CheckAndLog("after glBindVertexArray");

		GLint boundVAO = 0;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &boundVAO);

		GLint eboBound = 0;
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &eboBound);

		GLint programLinked = 0;
		glGetProgramiv(currentProgram, GL_LINK_STATUS, &programLinked);

		GLint vaoValid = (int)glIsVertexArray(static_cast<GLuint>(glBuffers->vao));

		if (boundVAO != static_cast<GLint>(glBuffers->vao) || eboBound == 0 || !programLinked || !vaoValid) {
			std::cerr << "[GL] DrawMesh STATE: vao=" << glBuffers->vao
					  << " boundVAO=" << boundVAO
					  << " ebo=" << eboBound
					  << " program=" << currentProgram
					  << " linked=" << programLinked
					  << " vaoValid=" << vaoValid
					  << " indexCount=" << glBuffers->indexCount
					  << std::endl;
		}

		if (texture) {
			const auto* glTexture = static_cast<const OpenGLGPUTexture*>(texture);
			glActiveTexture(GL_TEXTURE0);
			CheckAndLog("after glActiveTexture");
			if (glTexture->isTextureArray()) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, glTexture->textureId);
				CheckAndLog(("after glBindTexture(GL_TEXTURE_2D_ARRAY, " + std::to_string(glTexture->textureId) + ")").c_str());
			} else {
				glBindTexture(GL_TEXTURE_2D, glTexture->textureId);
				CheckAndLog(("after glBindTexture(GL_TEXTURE_2D, " + std::to_string(glTexture->textureId) + ")").c_str());
			}
		}

		glDrawElements(GL_TRIANGLES, glBuffers->indexCount, GL_UNSIGNED_INT, 0);
		CheckAndLog("after glDrawElements");

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
