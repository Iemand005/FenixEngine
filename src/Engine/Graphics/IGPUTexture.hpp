#include <string>
#include <vector>
#include <memory>

enum class TextureScaling { Nearest, Linear };

class IGPUTexture {
public:
    virtual ~IGPUTexture() = default;
};

class Texture {
public:
    std::unique_ptr<IGPUTexture> gpuTexture = nullptr;
};
