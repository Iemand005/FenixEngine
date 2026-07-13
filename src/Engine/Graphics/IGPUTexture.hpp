#pragma once

#include <string>
#include <vector>
#include <memory>


namespace fe {
    enum class TextureScaling { Nearest, Linear };

    class IGPUTexture {
    public:
        virtual ~IGPUTexture() = default;

        virtual bool load(const std::string& textureFilePath, TextureScaling scaling = TextureScaling::Linear) = 0;
    };

    class Texture {
    public:
        std::unique_ptr<IGPUTexture> gpuTexture = nullptr;
    };

}
