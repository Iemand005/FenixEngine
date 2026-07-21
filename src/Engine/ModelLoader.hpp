#pragma once

#include "Mesh.hpp"

namespace fe {
	class ModelFactory {
		static Mesh<> LoadMesh();
	};
}