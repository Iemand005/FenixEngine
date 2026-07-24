#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "ImageLoader.hpp"
#include "physics/PhysicsObject.hpp"
#include "Graphics/IGPUBuffers.hpp"
#include "Graphics/IGPUTexture.hpp"

namespace fe {

class IRenderDevice;

class IMesh {
public:
    virtual ~IMesh() = default;

    virtual void SetDevice(IRenderDevice* d) = 0;
    virtual void CopyToGPU() = 0;
    virtual void RemoveFromGPU() = 0;
    virtual void FreeCpuData() = 0;

    virtual std::unique_ptr<IMesh> Clone() const = 0;

    virtual size_t GetVertexCount() const = 0;
    virtual size_t GetIndexCount() const = 0;
    virtual const std::vector<unsigned int>& GetIndices() const = 0;

    virtual IGPUBuffers* GetGPUBuffers() const = 0;
    virtual IGPUTexture* GetGPUTexture() const = 0;

    virtual IGPUBuffers* GetGPUBuffersFor(IRenderDevice* dev) { return GetGPUBuffers(); }
    virtual IGPUTexture* GetGPUTextureFor(IRenderDevice* dev) { return GetGPUTexture(); }

    virtual bool loadTexture(const std::string& textureFilePath, TextureScaling scaling = TextureScaling::Linear) = 0;
    virtual bool loadTexture(const ImageData& image, TextureScaling scaling = TextureScaling::Linear) = 0;
    virtual bool loadTextureArray(const std::vector<std::string>& textureFilePaths, TextureScaling scaling = TextureScaling::Linear) = 0;

    virtual glm::vec4 GetColor() const = 0;
    virtual void SetColor(const glm::vec4& c) = 0;

    virtual bool GetHasTransparency() const = 0;
    virtual void SetHasTransparency(bool v) = 0;

    virtual void SetPhysicsObject(std::unique_ptr<PhysicsObject> obj) = 0;

    virtual void GetPositions(std::vector<glm::vec3>& out) const = 0;
};

} // namespace fe
