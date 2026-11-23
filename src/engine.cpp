#define OBJ_LOADER
#include "engine.h"

#include <filesystem>

#include "OBJ_Loader.h"

#include "Object.hpp"
#include "Mesh.hpp"

namespace fe {

bool Object::LoadObj(std::string path, float scale) {
  objl::Loader objectLoader;

  bool success = objectLoader.LoadFile(path);
  if (!success) return false;

  const std::filesystem::path objPath(path);
  const std::filesystem::path objDir = objPath.parent_path();

  for (auto &loadedMesh : objectLoader.LoadedMeshes) {
    std::cout << "Mesh Name: " << loadedMesh.MeshName << std::endl;
    std::cout << "Vertices: " << loadedMesh.Vertices.size() << std::endl;
    std::cout << "Indices: " << loadedMesh.Indices.size() << std::endl;

    auto vertices = std::vector<Vertex>(loadedMesh.Vertices.size());
    auto indices = std::vector<unsigned int>(loadedMesh.Indices.size());

    for (int i = 0; i < loadedMesh.Vertices.size(); i++) {
      objl::Vertex v = loadedMesh.Vertices[i];
      vertices[i] = Vertex(v.Position.X, v.Position.Y, v.Position.Z, v.Normal.X, v.Normal.Y, v.Normal.Z, v.TextureCoordinate.X, v.TextureCoordinate.Y);
    }

    for (size_t i = 0; i < indices.size(); i++) indices[i] = loadedMesh.Indices[i];

    auto mesh = std::make_unique<Mesh<Vertex>>(vertices, indices);
    if (!loadedMesh.MeshMaterial.map_Kd.empty()) {
      std::filesystem::path texPath(loadedMesh.MeshMaterial.map_Kd);
      if (texPath.is_relative()) texPath = objDir / texPath;
      mesh->loadTexture(texPath.string());
    }

    this->meshes.push_back(std::move(mesh));
  }

  this->state.scale = glm::vec3(scale);
  modelMatrixDirty = true;

  return true;
}

}  // namespace fe
