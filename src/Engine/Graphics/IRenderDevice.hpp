
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../window/IWindow.hpp"
#include "IGPUTexture.hpp"
#include "IGPUBuffers.hpp"
#include "../ImageLoader.hpp"
#include "../Vertex.hpp"

namespace fe {

	enum class GizmoDrawMode { Lines, LineLoop };

	class IRenderDevice {
	public:
		virtual ~IRenderDevice() = default;
		virtual void Init(IWindow *window = nullptr) = 0;
		virtual void Clear() = 0;
		virtual void SetClearColor(float r, float g, float b, float a = 1) = 0;
		virtual void Resize(int width, int height) = 0;
		
		virtual void BeginFrame() = 0;
		virtual void DrawMesh(const IGPUBuffers* buffers, const IGPUTexture* texture = nullptr) = 0;
		virtual void SubmitFrame() = 0;

		virtual void RegisterWindow(IWindow* window) {}
		virtual void UnregisterWindow(const IWindow* window) {}

		virtual std::unique_ptr<IGPUBuffers> CreateGPUBuffers() { return nullptr; }
		virtual std::unique_ptr<IGPUTexture> CreateGPUTexture() { return nullptr; }

		virtual void UploadBuffers(IGPUBuffers* buffers,
			const void* vertices, size_t vertexStride, size_t vertexCount,
			const uint32_t* indices, uint32_t indexCount,
			const std::vector<VertexAttribute>& layout = {}) {}

		virtual void UploadTexture(IGPUTexture* texture,
			const std::string& path, TextureScaling scaling = TextureScaling::Linear) {}

		virtual void UploadTexture(IGPUTexture* texture,
			const ImageData& image, TextureScaling scaling = TextureScaling::Linear) {}

		virtual void UploadTextureArray(IGPUTexture* texture,
			const std::vector<std::string>& paths, TextureScaling scaling = TextureScaling::Linear) {}

		virtual void SetMat4(const char* name, const glm::mat4& value) {}
		virtual void SetVec3(const char* name, const glm::vec3& value) {}
		virtual void SetInt(const char* name, int value) {}
		virtual void SetFloat(const char* name, float value) {}

		virtual void SetFrontFace(bool ccw) {}
		virtual void SetTransparentMode(bool enabled) {}
		virtual bool ReadDepthBuffer(std::vector<float>& outDepths, int& outW, int& outH) { return false; }

		virtual void EnableWireframe() {}
		virtual void DisableWireframe() {}
		virtual void BindFramebuffer(int bufferIndex) {}

		virtual uint64_t CreateFramebuffer(uint64_t nativeImage, uint32_t w, uint32_t h, uint32_t layer = 0, uint64_t depthFormat = 0, uint64_t colorFormat = 0) { return 0; }
		virtual void DestroyFramebuffer(uint64_t fb) {}
		virtual void BeginVRFrame() {}
		virtual void BeginEyeFrame(uint64_t fb, uint32_t w, uint32_t h) {}
		virtual void EndEyeFrame() {}
		virtual void EndVRFrame() {}

		virtual void BeginExternalFrame(uint64_t fb, uint32_t w, uint32_t h) {}
		virtual void EndExternalFrame() {}
		virtual uint64_t GetSwapchainFormat() const { return 0; }

		virtual uint64_t CreateColorAttachment(uint32_t w, uint32_t h) { return 0; }
		virtual void DestroyColorAttachment(uint64_t image) {}

		virtual bool IsVulkan() const { return false; }

		virtual void* UploadToImGui(const unsigned char* rgba, int w, int h) { return nullptr; }

		virtual void DrawDebugLines(const float* vertices, int vertexCount, const glm::mat4& view, const glm::mat4& proj) {}
		virtual void DrawDebugTriangles(const float* vertices, int vertexCount, const glm::mat4& view, const glm::mat4& proj) {}

		virtual void DrawGizmoLines(const float* vertices, int vertexCount, GizmoDrawMode mode, const glm::vec3& color, float lineWidth, const glm::mat4& view, const glm::mat4& proj) {}

		virtual const char* GetDeviceName() const = 0;
	};
}
