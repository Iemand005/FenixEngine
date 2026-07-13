
#pragma once

#include "../window/IWindow.hpp"
#include "IGPUTexture.hpp"

#include "../Mesh.hpp"

namespace fe {

    class IRenderDevice {
    public:
        virtual ~IRenderDevice() = default;
        virtual void Init(fe::IWindow *window) = 0;
        virtual void Clear() = 0;
        // virtual VertexBuffer* CreateVertexBuffer(void* data, size_t size) = 0;
        // virtual Texture* CreateTexture(const std::string& path) = 0;
        // virtual void DrawIndexed(uint32_t indexCount) = 0;
        virtual void DrawMesh(const IGPUBuffers* buffers, const IGPUTexture* texture = nullptr) = 0;
        virtual void SubmitFrame() = 0;
    };
}