
#pragma once

#include <iostream>

#include "IRenderDevice.hpp"
#include "OpenGLGPUBuffers.hpp"
#include "OpenGLGPUTexture.hpp"

namespace fe {


class OpenGLRenderDevice : public IRenderDevice {
	GLuint defaultTextureId_ = 0;

	void Init(fe::IWindow *window) override {
		glGenTextures(1, &defaultTextureId_);
		glBindTexture(GL_TEXTURE_2D, defaultTextureId_);
		unsigned char whitePixel[4] = {255, 255, 255, 255};
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);

		EnableDepthTest();
		EnableFaceCulling();
	}

	void SetVec3(const char* name, const glm::vec3& value) override {
		GLint prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog) glUniform3f(glGetUniformLocation(prog, name), value.x, value.y, value.z);
	}

	void SetInt(const char* name, int value) override {
		GLint prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog) glUniform1i(glGetUniformLocation(prog, name), value);
	}

	void SetFloat(const char* name, float value) override {
		GLint prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog) glUniform1f(glGetUniformLocation(prog, name), value);
	}

	void SetFrontFace(bool ccw) override {
		glFrontFace(ccw ? GL_CCW : GL_CW);
	}

	void EnableWireframe() override {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	void DisableWireframe() override {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	void BindFramebuffer(int bufferIndex) override {
		glBindFramebuffer(GL_FRAMEBUFFER, bufferIndex);
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

	void SetTransparentMode(bool enabled) override {
		glDepthMask(enabled ? GL_FALSE : GL_TRUE);
	}

	bool ReadDepthBuffer(std::vector<float>& outDepths, int& outW, int& outH) override {
		GLint vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		int fbW = vp[2], fbH = vp[3];
		if (fbW <= 0 || fbH <= 0) return false;
		outW = std::min(fbW, 640);
		outH = fbH * outW / fbW;
		outDepths.resize(static_cast<size_t>(outW) * outH);
		glReadPixels(0, 0, outW, outH, GL_DEPTH_COMPONENT, GL_FLOAT, outDepths.data());
		return true;
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
			GLenum glType = OpenGLGPUBuffers::toGLType(attr.type);
			if (attr.type == fe::VertexAttribType::Float) {
				glVertexAttribPointer(attr.location, attr.components, glType, GL_FALSE,
					static_cast<GLsizei>(vertexStride), (void*)attr.offset);
			} else {
				glVertexAttribIPointer(attr.location, attr.components, glType,
					static_cast<GLsizei>(vertexStride), (void*)attr.offset);
			}
			glEnableVertexAttribArray(attr.location);
		}

		glBindVertexArray(0);

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			std::cerr << "[GL ERROR] UploadBuffers: 0x" << std::hex << err << std::dec << std::endl;
		}
	}

	void BeginFrame() override {

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

		glActiveTexture(GL_TEXTURE0);
		if (texture) {
			const auto* glTexture = static_cast<const OpenGLGPUTexture*>(texture);
			if (glTexture->isTextureArray()) {
				glBindTexture(GL_TEXTURE_2D_ARRAY, glTexture->textureId);
			} else {
				glBindTexture(GL_TEXTURE_2D, glTexture->textureId);
			}
		} else {
			glBindTexture(GL_TEXTURE_2D, defaultTextureId_);
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
		} else {
			glBindTexture(GL_TEXTURE_2D, 0);
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

	void UploadTexture(IGPUTexture* texture,
		const ImageData& image, TextureScaling scaling = TextureScaling::Linear) override {
		if (!texture) return;
		auto* glTexture = static_cast<OpenGLGPUTexture*>(texture);
		glTexture->upload(image, scaling);
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

	uint64_t CreateFramebuffer(uint64_t nativeImage, uint32_t w, uint32_t h, uint32_t layer = 0, uint64_t depthFormat = 0, uint64_t colorFormat = 0) override {
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, static_cast<GLuint>(nativeImage), 0, static_cast<GLint>(layer));

		GLuint depthRB = 0;
		if (depthFormat != 0) {
			glGenRenderbuffers(1, &depthRB);
			glBindRenderbuffer(GL_RENDERBUFFER, depthRB);
			glRenderbufferStorage(GL_RENDERBUFFER, static_cast<GLenum>(depthFormat), w, h);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRB);
		}

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			std::cerr << "OpenGL FBO incomplete: 0x" << std::hex << status << std::dec << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		uint64_t ret = fbo;
		if (depthRB != 0) {
			ret |= (static_cast<uint64_t>(depthRB) << 32);
		}
		return ret;
	}

	void DestroyFramebuffer(uint64_t fb) override {
		GLuint fbo   = static_cast<GLuint>(fb & 0xFFFFFFFF);
		GLuint depth = static_cast<GLuint>(fb >> 32);
		glDeleteFramebuffers(1, &fbo);
		if (depth != 0) glDeleteRenderbuffers(1, &depth);
	}

	uint64_t CreateColorAttachment(uint32_t w, uint32_t h) override {
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, static_cast<GLsizei>(w), static_cast<GLsizei>(h), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		return tex;
	}

	void DestroyColorAttachment(uint64_t image) override {
		GLuint tex = static_cast<GLuint>(image);
		glDeleteTextures(1, &tex);
	}

	void BeginVRFrame() override {}

	void BeginEyeFrame(uint64_t fb, uint32_t w, uint32_t h) override {
		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(fb));
		glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void EndEyeFrame() override {
		glFlush();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void EndVRFrame() override {}

	void BeginExternalFrame(uint64_t fb, uint32_t w, uint32_t h) override {
		BeginVRFrame();
		BeginEyeFrame(fb, w, h);
	}

	void EndExternalFrame() override {
		EndEyeFrame();
		EndVRFrame();
	}

	uint64_t GetSwapchainFormat() const override {
		return GL_RGBA8;
	}

	const char* GetDeviceName() const override {
		return reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	}
};

}
