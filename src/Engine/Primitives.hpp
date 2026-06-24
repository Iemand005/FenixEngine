#pragma once
#include "Mesh.hpp"

namespace fe::Primitives {

	inline Mesh GenerateCube(float size = 1.0f) {
		float h = size * 0.5f;

		std::vector<Vertex> vertices = {
			// +X face
			Vertex( h,-h,-h,  1,0,0,  0,0),
			Vertex( h, h,-h,  1,0,0,  0,1),
			Vertex( h, h, h,  1,0,0,  1,1),
			Vertex( h,-h, h,  1,0,0,  1,0),

			// -X face
			Vertex(-h,-h, h, -1,0,0,  0,0),
			Vertex(-h, h, h, -1,0,0,  0,1),
			Vertex(-h, h,-h, -1,0,0,  1,1),
			Vertex(-h,-h,-h, -1,0,0,  1,0),

			// +Y face
			Vertex(-h, h,-h,  0,1,0,  0,0),
			Vertex(-h, h, h,  0,1,0,  0,1),
			Vertex( h, h, h,  0,1,0,  1,1),
			Vertex( h, h,-h,  0,1,0,  1,0),

			// -Y face
			Vertex(-h,-h, h,  0,-1,0, 0,0),
			Vertex(-h,-h,-h,  0,-1,0, 0,1),
			Vertex( h,-h,-h,  0,-1,0, 1,1),
			Vertex( h,-h, h,  0,-1,0, 1,0),

			// +Z face
			Vertex(-h,-h, h,  0,0,1,  0,0),
			Vertex( h,-h, h,  0,0,1,  0,1),
			Vertex( h, h, h,  0,0,1,  1,1),
			Vertex(-h, h, h,  0,0,1,  1,0),

			// -Z face
			Vertex( h,-h,-h,  0,0,-1, 0,0),
			Vertex(-h,-h,-h,  0,0,-1, 0,1),
			Vertex(-h, h,-h,  0,0,-1, 1,1),
			Vertex( h, h,-h,  0,0,-1, 1,0),
		};

		std::vector<uint32_t> indices = {
			0, 1, 2,    0, 2, 3,     // +X
			4, 5, 6,    4, 6, 7,     // -X
			8, 9, 10,   8, 10, 11,   // +Y
			12, 13, 14, 12, 14, 15,  // -Y
			16, 17, 18, 16, 18, 19,  // +Z
			20, 21, 22, 20, 22, 23,  // -Z
		};

		return Mesh(vertices, indices);
	}

	inline Mesh GeneratePlane(float width = 1.0f, float height = 1.0f) {
    float w = width * 0.5f;
    float h = height * 0.5f;
    
    std::vector<Vertex> vertices = {
			Vertex(-w, 0, -h,  0, 1, 0,  0, 0),
			Vertex( w, 0, -h,  0, 1, 0,  1, 0),
			Vertex( w, 0,  h,  0, 1, 0,  1, 1),
			Vertex(-w, 0,  h,  0, 1, 0,  0, 1),
    };
    
    std::vector<uint32_t> indices = {
			0, 1, 2,
			0, 2, 3,
    };
    
    return Mesh(vertices, indices);
	O}

} // namespace fe::Primitives