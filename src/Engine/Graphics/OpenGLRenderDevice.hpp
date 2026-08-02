
#pragma once

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

	void MakeCurrent();
	void SetActiveWindow(IWindow* w);
	IWindow* GetActiveWindow() const { return activeWindow_; }
	const std::vector<IWindow*>& GetRegisteredWindows() const { return registeredWindows_; }

	void InitDebugRenderer();

	void DrawDebugLines(const float* vertices, int vertexCount, const glm::mat4& view, const glm::mat4& proj) override;

	void DrawDebugTriangles(const float* vertices, int vertexCount, const glm::mat4& view, const glm::mat4& proj) override;

	void DrawGizmoLines(const float* vertices, int vertexCount, GizmoDrawMode mode, const glm::vec3& color, float lineWidth, const glm::mat4& view, const glm::mat4& proj) override;

	void Init(IWindow *window) override;
	void RegisterWindow(IWindow* window) override;
	void UnregisterWindow(const IWindow* window) override;
	void SetVec3(const char* name, const glm::vec3& value) override;
	void SetMat4(const char* name, const glm::mat4& value) override;
	void SetInt(const char* name, int value) override;
	void SetFloat(const char* name, float value) override;
	void SetFrontFace(bool ccw) override;
	void EnableWireframe() override;
	void DisableWireframe() override;
	void BindFramebuffer(int bufferIndex) override;
	void Clear() override;
	void SetClearColor(float r, float g, float b, float a = 1) override;
	void Resize(int width, int height) override;
	std::vector<const IWindow*> GetWindows() override;
	std::unique_ptr<IGPUBuffers> CreateGPUBuffers() override;
	std::unique_ptr<IGPUTexture> CreateGPUTexture() override;
	void SetTransparentMode(bool enabled) override;
	bool ReadDepthBuffer(std::vector<float>& outDepths, int& outW, int& outH) override;
	void EnableDepthTest() override;
	void DisableDepthTest() override;
	void EnableFaceCulling() override;
	void DisableFaceCulling() override;
	void UploadBuffers(IGPUBuffers* buffers,
		const void* vertices, size_t vertexStride, size_t vertexCount,
		const uint32_t* indices, uint32_t indexCount,
		const std::vector<fe::VertexAttribute>& layout = {}) override;
	void BeginFrame() override;
	void DrawMesh(const IGPUBuffers* buffers, const IGPUTexture* texture = nullptr) override;
	void UploadTexture(IGPUTexture* texture,
		const std::string& path, TextureScaling scaling = TextureScaling::Linear) override;
	void UploadTexture(IGPUTexture* texture,
		const ImageData& image, TextureScaling scaling = TextureScaling::Linear) override;
	void UploadTextureArray(IGPUTexture* texture,
		const std::vector<std::string>& paths, TextureScaling scaling = TextureScaling::Linear) override;
	void SubmitFrame() override;
	void SubmitFrame(const IWindow *window) override;
	uint64_t CreateFramebuffer(uint64_t nativeImage, uint32_t w, uint32_t h, uint32_t layer = 0, uint64_t depthFormat = 0, uint64_t colorFormat = 0) override;
	void DestroyFramebuffer(uint64_t fb) override;
	uint64_t CreateColorAttachment(uint32_t w, uint32_t h) override;
	void DestroyColorAttachment(uint64_t image) override;
	void BeginVRFrame() override;
	void BeginEyeFrame(uint64_t fb, uint32_t w, uint32_t h) override;
	void EndEyeFrame() override;
	void EndVRFrame() override;
	void BeginExternalFrame(uint64_t fb, uint32_t w, uint32_t h) override;
	void EndExternalFrame() override;
	uint64_t GetSwapchainFormat() const override;
	void* UploadToImGui(const unsigned char* rgba, int w, int h) override;
	const char* GetDeviceName() const override;
	bool IsVulkan() const override { return false; }
	size_t GetWindowCount() const override;
};

}
