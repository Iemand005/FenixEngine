
#pragma once

#include <iostream>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

#include "IRenderDevice.hpp"
#include "OpenGLGPUBuffers.hpp"
#include "OpenGLGPUTexture.hpp"

namespace fe {


class OpenGLRenderDevice : public IRenderDevice {
	GLuint defaultTextureId_ = 0;

	GLuint debugShaderProgram_ = 0;
	GLuint debugVAO_ = 0;
	GLuint debugVBO_ = 0;

	std::vector<IWindow*> registeredWindows_;
	IWindow* activeWindow_ = nullptr;

	void MakeCurrent() {
		if (activeWindow_) {
			activeWindow_->MakeCurrentGLContext();
		} else if (!registeredWindows_.empty()) {
			registeredWindows_.front()->MakeCurrentGLContext();
		}
	}

	void SetActiveWindow(IWindow* w) {
		activeWindow_ = w;
		MakeCurrent();
	}

	IWindow* GetActiveWindow() const { return activeWindow_; }
	const std::vector<IWindow*>& GetRegisteredWindows() const { return registeredWindows_; }

	void InitDebugRenderer()
	{
		if (debugShaderProgram_ != 0) return;

		const char* vertexShaderSource = R"(
			#version 330 core
			layout(location = 0) in vec3 aPos;
			layout(location = 1) in vec4 aColor;

			uniform mat4 uView;
			uniform mat4 uProjection;

			out vec4 vColor;

			void main()
			{
				vColor = aColor;
				gl_Position = uProjection * uView * vec4(aPos, 1.0);
			}
		)";

		const char* fragmentShaderSource = R"(
			#version 330 core
			in vec4 vColor;
			out vec4 FragColor;

			void main()
			{
				FragColor = vColor;
			}
		)";

		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
		glCompileShader(vertexShader);

		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
		glCompileShader(fragmentShader);

		debugShaderProgram_ = glCreateProgram();
		glAttachShader(debugShaderProgram_, vertexShader);
		glAttachShader(debugShaderProgram_, fragmentShader);
		glLinkProgram(debugShaderProgram_);

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);

		glGenVertexArrays(1, &debugVAO_);
		glGenBuffers(1, &debugVBO_);

		glBindVertexArray(debugVAO_);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO_);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 28, (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 28, (void*)12);
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void DrawDebugLines(const float* vertices, int vertexCount, const glm::mat4& view, const glm::mat4& proj) override
	{
		InitDebugRenderer();

		glUseProgram(debugShaderProgram_);
		glUniformMatrix4fv(glGetUniformLocation(debugShaderProgram_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(debugShaderProgram_, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));

		glBindVertexArray(debugVAO_);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO_);
		glBufferData(GL_ARRAY_BUFFER, vertexCount * 28, vertices, GL_STREAM_DRAW);
		glDrawArrays(GL_LINES, 0, vertexCount);
		glBindVertexArray(0);

		glUseProgram(0);
	}

	void DrawDebugTriangles(const float* vertices, int vertexCount, const glm::mat4& view, const glm::mat4& proj) override
	{
		InitDebugRenderer();

		glUseProgram(debugShaderProgram_);
		glUniformMatrix4fv(glGetUniformLocation(debugShaderProgram_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(debugShaderProgram_, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));

		glBindVertexArray(debugVAO_);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO_);
		glBufferData(GL_ARRAY_BUFFER, vertexCount * 28, vertices, GL_STREAM_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
		glBindVertexArray(0);

		glUseProgram(0);
	}

	void DrawGizmoLines(const float* vertices, int vertexCount, GizmoDrawMode mode, const glm::vec3& color, float lineWidth, const glm::mat4& view, const glm::mat4& proj) override
	{
		InitDebugRenderer();

		GLint previousProgram = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);

		GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
		GLfloat previousLineWidth = 1.0f;
		glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);

		glDisable(GL_DEPTH_TEST);

		glUseProgram(debugShaderProgram_);
		glUniformMatrix4fv(glGetUniformLocation(debugShaderProgram_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(debugShaderProgram_, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));

		static std::vector<float> temp;
		temp.resize(vertexCount * 7);
		for (int i = 0; i < vertexCount; ++i) {
			int src = i * 3;
			int dst = i * 7;
			temp[dst]     = vertices[src];
			temp[dst + 1] = vertices[src + 1];
			temp[dst + 2] = vertices[src + 2];
			temp[dst + 3] = color.r;
			temp[dst + 4] = color.g;
			temp[dst + 5] = color.b;
			temp[dst + 6] = 1.0f;
		}

		glBindVertexArray(debugVAO_);
		glBindBuffer(GL_ARRAY_BUFFER, debugVBO_);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(temp.size() * sizeof(float)), temp.data(), GL_STREAM_DRAW);

		GLenum glMode = (mode == GizmoDrawMode::LineLoop) ? GL_LINE_LOOP : GL_LINES;
		glLineWidth(lineWidth);
		glDrawArrays(glMode, 0, vertexCount);
		glLineWidth(previousLineWidth);

		glBindVertexArray(0);
		glUseProgram(static_cast<GLuint>(previousProgram));
		if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
	}

	void Init(IWindow *window) override {
		if (window) {
			window->MakeCurrentGLContext();
			registeredWindows_.push_back(window);
		}

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

	void RegisterWindow(IWindow* window) override {
		if (!window) return;
		for (auto* w : registeredWindows_) {
			if (w == window) return;
		}
		window->MakeCurrentGLContext();
		registeredWindows_.push_back(window);
	}

	void UnregisterWindow(const IWindow* window) override {
		registeredWindows_.erase(
			std::remove(registeredWindows_.begin(), registeredWindows_.end(), window),
			registeredWindows_.end());
	}

	void SetVec3(const char* name, const glm::vec3& value) override {
		MakeCurrent();
		GLint prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog) glUniform3f(glGetUniformLocation(prog, name), value.x, value.y, value.z);
	}

	void SetMat4(const char* name, const glm::mat4& value) override {
		MakeCurrent();
		GLint prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog) glUniformMatrix4fv(glGetUniformLocation(prog, name), 1, GL_FALSE, glm::value_ptr(value));
	}

	void SetInt(const char* name, int value) override {
		MakeCurrent();
		GLint prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog) glUniform1i(glGetUniformLocation(prog, name), value);
	}

	void SetFloat(const char* name, float value) override {
		MakeCurrent();
		GLint prog; glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
		if (prog) glUniform1f(glGetUniformLocation(prog, name), value);
	}

	void SetFrontFace(bool ccw) override {
		MakeCurrent();
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
		MakeCurrent();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void SetClearColor(float r, float g, float b, float a = 1) override {
		MakeCurrent();
		glClearColor(r, g, b, a);
	}

	void Resize(int width, int height) override {
		MakeCurrent();
		glViewport(0, 0, width, height);
	}

	std::vector<const IWindow*> GetWindows() override {
		std::vector<IWindow*> windows;
		windows.reserve(registeredWindows_.size());
		// return registeredWindows_;
		return {};
	}

	std::unique_ptr<IGPUBuffers> CreateGPUBuffers() override {
		return std::make_unique<OpenGLGPUBuffers>();
	}

	std::unique_ptr<IGPUTexture> CreateGPUTexture() override {
		return std::make_unique<OpenGLGPUTexture>();
	}

	void SetTransparentMode(bool enabled) override {
		MakeCurrent();
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
	void EnableDepthTest() override { glEnable(GL_DEPTH_TEST); }
	void DisableDepthTest() override { glDisable(GL_DEPTH_TEST); }
	void EnableFaceCulling() override { glEnable(GL_CULL_FACE); }
	void DisableFaceCulling() override { glDisable(GL_CULL_FACE); }

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
		MakeCurrent();
	}

	void DrawMesh(const IGPUBuffers* buffers, const IGPUTexture* texture = nullptr) override {
		if (!buffers) return;

		MakeCurrent();

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
		MakeCurrent();
		glFlush();
	}

	void SubmitFrame(const IWindow *window) override {
		window->MakeCurrentGLContext();
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

	void* UploadToImGui(const unsigned char* rgba, int w, int h) override {
		static GLuint tex = 0;
		static int prevW = 0, prevH = 0;
		if (!tex || w != prevW || h != prevH) {
			if (tex) glDeleteTextures(1, &tex);
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			prevW = w; prevH = h;
		}
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
		glBindTexture(GL_TEXTURE_2D, 0);
		return (void*)(uintptr_t)tex;
	}

	const char* GetDeviceName() const override {
		return reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	}

	bool IsVulkan() const override { return false; }

	size_t GetWindowCount() const override { return registeredWindows_.size(); }
};

}
