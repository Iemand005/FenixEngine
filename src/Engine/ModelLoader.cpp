#include "ModelLoader.hpp"

#include <iostream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define CGLTF_IMPLEMENTATION
#include "../../external/cgltf/cgltf.h"

using namespace fe;

static void ProcessGLTFNode(cgltf_node* node, Object* parent) {
	auto obj = std::make_unique<Object>();

	if (node->name) obj->name = node->name;
	else obj->name = "GLBNode";

	obj->state.position = glm::vec3(
		node->translation[0], node->translation[1], node->translation[2]);
	obj->state.orientation = glm::quat(
		node->rotation[3], node->rotation[0], node->rotation[1], node->rotation[2]);
	obj->state.scale = glm::vec3(
		node->scale[0], node->scale[1], node->scale[2]);

	if (node->mesh) {
		for (cgltf_size p = 0; p < node->mesh->primitives_count; ++p) {
			const cgltf_primitive* prim = &node->mesh->primitives[p];

			const cgltf_accessor* pos_accessor = cgltf_find_accessor(prim, cgltf_attribute_type_position, 0);
			const cgltf_accessor* norm_accessor = cgltf_find_accessor(prim, cgltf_attribute_type_normal, 0);
			const cgltf_accessor* uv_accessor = cgltf_find_accessor(prim, cgltf_attribute_type_texcoord, 0);

			if (!pos_accessor) continue;

			cgltf_size vertex_count = pos_accessor->count;
			std::vector<Vertex> vertices(vertex_count);

			for (cgltf_size i = 0; i < vertex_count; ++i) {
				glm::vec3 pos(0.0f), norm(0.0f, 1.0f, 0.0f);
				glm::vec2 uv(0.0f);

				cgltf_float pos_data[3];
				if (cgltf_accessor_read_float(pos_accessor, i, pos_data, 3)) {
					pos.x = pos_data[0];
					pos.y = pos_data[1];
					pos.z = pos_data[2];
				}

				if (norm_accessor) {
					cgltf_float norm_data[3];
					if (cgltf_accessor_read_float(norm_accessor, i, norm_data, 3)) {
						norm.x = norm_data[0];
						norm.y = norm_data[1];
						norm.z = norm_data[2];
					}
				}

				if (uv_accessor) {
					cgltf_float uv_data[2];
					if (cgltf_accessor_read_float(uv_accessor, i, uv_data, 2)) {
						uv.x = uv_data[0];
						uv.y = uv_data[1];
					}
				}

				vertices[i] = Vertex(pos.x, pos.y, pos.z, norm.x, norm.y, norm.z, uv.x, uv.y);
			}

			std::vector<unsigned int> indices;
			if (prim->indices) {
				indices.resize(prim->indices->count);
				for (cgltf_size i = 0; i < prim->indices->count; ++i) {
					indices[i] = static_cast<unsigned int>(cgltf_accessor_read_index(prim->indices, i));
				}
			} else {
				indices.resize(vertex_count);
				for (cgltf_size i = 0; i < vertex_count; ++i) {
					indices[i] = static_cast<unsigned int>(i);
				}
			}

			obj->EmplaceMesh<Vertex>(std::move(vertices), std::move(indices));
		}
	}

	Object* rawPtr = parent->AddChild(std::move(obj));

	for (cgltf_size i = 0; i < node->children_count; ++i) {
		ProcessGLTFNode(node->children[i], rawPtr);
	}
}

std::shared_ptr<Object> ModelLoader::LoadModel(const std::string& fileName) {
	auto root = std::make_shared<Object>();
	root->name = "GLB Root";

	cgltf_options options = {};
	cgltf_data* data = nullptr;

	cgltf_result parse_result = cgltf_parse_file(&options, fileName.c_str(), &data);
	if (parse_result != cgltf_result_success) {
		std::cerr << "[ModelLoader] Failed to parse file: " << fileName << " (error " << parse_result << ")" << std::endl;
		return root;
	}

	cgltf_result buffer_result = cgltf_load_buffers(&options, data, fileName.c_str());
	if (buffer_result != cgltf_result_success) {
		std::cerr << "[ModelLoader] Failed to load buffers for: " << fileName << std::endl;
		cgltf_free(data);
		return root;
	}

	if (data->scene) {
		for (cgltf_size i = 0; i < data->scene->nodes_count; ++i) {
			ProcessGLTFNode(data->scene->nodes[i], root.get());
		}
	} else {
		std::cerr << "[ModelLoader] No default scene found; loading meshes flat." << std::endl;
		for (cgltf_size m = 0; m < data->meshes_count; ++m) {
			const cgltf_mesh* mesh = &data->meshes[m];
			for (cgltf_size p = 0; p < mesh->primitives_count; ++p) {
				const cgltf_primitive* prim = &mesh->primitives[p];
				const cgltf_accessor* pos_accessor = cgltf_find_accessor(prim, cgltf_attribute_type_position, 0);
				const cgltf_accessor* norm_accessor = cgltf_find_accessor(prim, cgltf_attribute_type_normal, 0);
				const cgltf_accessor* uv_accessor = cgltf_find_accessor(prim, cgltf_attribute_type_texcoord, 0);
				if (!pos_accessor) continue;

				cgltf_size vertex_count = pos_accessor->count;
				std::vector<Vertex> vertices(vertex_count);
				for (cgltf_size i = 0; i < vertex_count; ++i) {
					glm::vec3 pos(0.0f), norm(0.0f, 1.0f, 0.0f);
					glm::vec2 uv(0.0f);
					cgltf_float pos_data[3];
					if (cgltf_accessor_read_float(pos_accessor, i, pos_data, 3)) {
						pos.x = pos_data[0]; pos.y = pos_data[1]; pos.z = pos_data[2];
					}
					if (norm_accessor) {
						cgltf_float norm_data[3];
						if (cgltf_accessor_read_float(norm_accessor, i, norm_data, 3)) {
							norm.x = norm_data[0]; norm.y = norm_data[1]; norm.z = norm_data[2];
						}
					}
					if (uv_accessor) {
						cgltf_float uv_data[2];
						if (cgltf_accessor_read_float(uv_accessor, i, uv_data, 2)) {
							uv.x = uv_data[0]; uv.y = uv_data[1];
						}
					}
					vertices[i] = Vertex(pos.x, pos.y, pos.z, norm.x, norm.y, norm.z, uv.x, uv.y);
				}
				std::vector<unsigned int> indices;
				if (prim->indices) {
					indices.resize(prim->indices->count);
					for (cgltf_size i = 0; i < prim->indices->count; ++i)
						indices[i] = static_cast<unsigned int>(cgltf_accessor_read_index(prim->indices, i));
				} else {
					indices.resize(vertex_count);
					for (cgltf_size i = 0; i < vertex_count; ++i)
						indices[i] = static_cast<unsigned int>(i);
				}
				root->EmplaceMesh<Vertex>(std::move(vertices), std::move(indices));
			}
		}
	}

	cgltf_free(data);
	return root;
}
