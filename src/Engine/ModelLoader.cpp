#include "ModelLoader.hpp"

#include <cgltf.h>

using namespace fe;


Object ModelLoader::LoadMesh(std::string fileName) {
	Object result;

	cgltf_options options = {};
	cgltf_data* data = nullptr;
	cgltf_result parse_result = cgltf_parse_file(&options, fileName.c_str(), &data);

	return std::move(result);
}