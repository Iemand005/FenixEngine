#pragma once

#include "Object.hpp"

namespace fe {
	class ModelLoader {
		static std::shared_ptr<Object> LoadMesh(std::string fileName);
	};
}