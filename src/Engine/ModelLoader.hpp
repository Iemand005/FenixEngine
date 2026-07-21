#pragma once

#include "Object.hpp"

namespace fe {
	class ModelLoader {
		static Object LoadMesh(std::string fileName);
	};
}