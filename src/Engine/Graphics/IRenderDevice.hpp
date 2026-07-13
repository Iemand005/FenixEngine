
#pragma once

#include <memory>
#include <string>

#include "../window/IWindow.hpp"
#include "IGPUTexture.hpp"
#include "IGPUBuffers.hpp"

namespace fe {

	class IRenderDevice {
	public:
		virtual ~IRenderDevice() = default;
		virtual void Init(fe::IWindow *window) = 0;
		virtual void Clear() = 0;
		virtual void SetClearColor(float r, float g, float b, float a = 1) = 0;
		virtual void Resize(int width, int height) = 0;
		virtual void DrawMesh(const IGPUBuffers* buffers, const IGPUTexture* texture = nullptr) = 0;
		virtual void SubmitFrame() = 0;

		virtual std::unique_ptr<IGPUBuffers> CreateGPUBuffers() { return nullptr; }
		virtual std::unique_ptr<IGPUTexture> CreateGPUTexture() { return nullptr; }

		virtual void UploadBuffers(IGPUBuffers* buffers,
			const void* vertices, size_t vertexStride, size_t vertexCount,
			const uint32_t* indices, uint32_t indexCount) {}

		virtual void UploadTexture(IGPUTexture* texture,
			const std::string& path, TextureScaling scaling = TextureScaling::Linear) {}
	};
}
