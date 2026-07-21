#include "ModelLoader.hpp"

// #include <cgltf.h>
// #include "../../cgltf/cgltf.h"
#include "../../external/cgltf/cgltf.h"

using namespace fe;


std::shared_ptr<Object> ModelLoader::LoadMesh(std::string fileName) {
	std::shared_ptr<Object> result = std::make_shared<Object>();

	cgltf_options options = {};
	cgltf_data* data = nullptr;
	cgltf_result parse_result = cgltf_parse_file(&options, fileName.c_str(), &data);

	// return std::move(result);
	return result;
}