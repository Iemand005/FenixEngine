#define STB_IMAGE_IMPLEMENTATION
#define OBJ_LOADER
#include "engine.h"

#include "OBJ_Loader.h"

namespace fe {

bool Mesh::loadObj(std::string objFilePath) {
  objl::Loader objectLoader;

  bool success = objectLoader.LoadFile(objFilePath);
  if (!success) return false;

  this->vertices = std::vector<Vertex>(objectLoader.LoadedVertices.size());

  for (int i = 0; i < this->vertices.size(); i++) {
    objl::Vertex v = objectLoader.LoadedVertices[i];
    this->vertices[i] = Vertex(v.Position.X, v.Position.Y, v.Position.Z, v.Normal.X, v.Normal.Y, v.Normal.Z, v.TextureCoordinate.X, v.TextureCoordinate.Y);
  }

  this->indices = std::vector<unsigned int>(objectLoader.LoadedIndices.size());

  for (size_t i = 0; i < this->indices.size(); i++) this->indices[i] = objectLoader.LoadedIndices[i];

  return true;
}

bool Object::LoadObj(std::string path, float scale) {
  objl::Loader objectLoader;

  bool success = objectLoader.LoadFile(path);
  if (!success) return false;

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

    Mesh mesh(vertices, indices);
    mesh.loadTexture(loadedMesh.MeshMaterial.map_Kd);

    this->meshes.push_back(mesh);
  }

  this->state.scale = glm::vec3(scale);

  return true;
}

}  // namespace fe