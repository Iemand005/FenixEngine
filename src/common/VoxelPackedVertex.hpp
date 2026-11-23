#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include <Vertex.hpp>

#pragma pack(push, 1)
struct VoxelPackedVertex {
    int16_t x, y, z;
    uint8_t normalLayer; // bits: nnnlllll (normal=3, layer=5)

    static std::vector<fe::VertexAttribute> getLayout() {
        return {
            { 0, 3, 0, fe::VertexAttribType::Short },
            { 1, 1, 6, fe::VertexAttribType::UByte },
        };
    }
};
#pragma pack(pop)
static_assert(sizeof(VoxelPackedVertex) == 7, "VoxelPackedVertex must be 7 bytes");

namespace fe {
    template<>
    struct VertexTraits<VoxelPackedVertex> {
        static glm::vec3 getPosition(const VoxelPackedVertex& v) {
            return glm::vec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
        }
    };
}
