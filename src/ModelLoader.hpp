#pragma once

#include <memory>
#include <string>

#include "Object.hpp"

namespace fe {
	class ModelLoader {
	public:
		static std::shared_ptr<Object> LoadModel(const std::string& fileName);
	};
}
