#include <vector>
#include <memory>

struct Vertex { /* ... */ };

class IGPUBuffers {
public:
    virtual ~IGPUBuffers() = default;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::unique_ptr<IGPUBuffers> gpuBuffers = nullptr;
};
