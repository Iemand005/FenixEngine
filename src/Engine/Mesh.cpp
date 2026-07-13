#pragma once

#include "Mesh.hpp"

bool fe::Mesh<>::loadObj(std::string objFilePath) {
//   objl::Loader objectLoader;

//   bool success = objectLoader.LoadFile(objFilePath);
//   if (!success) return false;

//   this->vertices = std::vector<Vertex>(objectLoader.LoadedVertices.size());
//   // TODO: this is a duplicate
//   for (int i = 0; i < this->vertices.size(); i++) {
//     objl::Vertex v = objectLoader.LoadedVertices[i];
//     this->vertices[i] = Vertex(v.Position.X, v.Position.Y, v.Position.Z, v.Normal.X, v.Normal.Y, v.Normal.Z, v.TextureCoordinate.X, v.TextureCoordinate.Y);
//   }

//   this->indices = std::vector<unsigned int>(objectLoader.LoadedIndices.size());

//   for (size_t i = 0; i < this->indices.size(); i++) this->indices[i] = objectLoader.LoadedIndices[i];

//   return true;
return false;
}