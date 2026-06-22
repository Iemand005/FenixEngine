#pragma once
#include "Mesh.hpp"

namespace fe::Primitives {

inline Mesh GenerateCube(float size = 1.0f) {
    float h = size * 0.5f;

    std::vector<Vertex> vertices = {
        {{ h,-h,-h}, {1,0,0}, {0,0}}, {{ h, h,-h}, {1,0,0}, {0,1}},
        {{ h, h, h}, {1,0,0}, {1,1}}, {{ h,-h, h}, {1,0,0}, {1,0}},
    };

    std::vector<uint32_t> indices = {
        0,1,2, 0,2,3,
    };

    return Mesh(vertices, indices);
}

} // namespace fe::Primitives