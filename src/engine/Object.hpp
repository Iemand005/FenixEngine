#pragma once
#include <glad/glad.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "bases.h"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "physics/PhysicsEngine.hpp"


//#ifndef STB_IMAGE_IMPLEMENTATION
//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
//#endif  // !STB_IMAGE_IMPLEMENTATION

//#define OBJ_LOADER
//#include "OBJ_Loader.h"

namespace fe {

    /*struct ObjectState {

};*/

class Object {
 private:
 public:
  ObjectState state{};
  //glm::vec3 position;
  //glm::vec3 rotation;
  //glm::vec3 velocity;
  //glm::vec3 acceleration;

  glm::mat4 modelMatrix;

  std::vector<Mesh> meshes;

  bool isStatic = false;

  bool touchedGround = false;

  bool touchedOtherObject = false;

  unsigned int boundingBoxVAO = 0, boundingBoxVBO = 0;

  std::vector<glm::vec3> boundingBoxVertices;

  std::unique_ptr<fe::PhysicsObject> physicsObject = nullptr;

  std::string sourcePath;

  // bool needsUpdate = true;

  Object() {
    //acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    state.scale = glm::vec3(1.0f, 1.0f, 1.0f);
    meshes = std::vector<Mesh>();
  }

  Object(Mesh mesh) : Object() {
    meshes.push_back(mesh);
  }

  Object(std::string objFilePath, float scale = 1.0f) : Object() { loadOBJ(objFilePath, scale);
  sourcePath = objFilePath;
  }

  bool loadOBJ(std::string path, float scale = 1.0f)
  /*{
    objl::Loader objectLoader;

    bool success = objectLoader.LoadFile(path);
    if (!success) return false;

    for (auto& loadedMesh : objectLoader.LoadedMeshes) {
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
  }*/;

  void SetPhysicsObject(std::unique_ptr<PhysicsObject> physicsObject) { this->physicsObject = std::move(physicsObject); }

  void Update(double deltaTime) {
    if (this->physicsObject) {
      auto state = this->physicsObject->SyncToRender();
      this->state = state;
     /* this->velocity = state.velocity;
      this->rotation = state.rotation;*/
    }
  }

  glm::mat4 GetModelMatrix() {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), this->state.position);
    model = glm::rotate(model, glm::radians(this->state.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(this->state.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, this->state.scale);
    return model;
  }

  void render(ShaderProgram& shader) {
    for (auto& mesh : meshes) mesh.render(shader, this->GetModelMatrix());
    if (boundingBoxVAO && touchedOtherObject) {
      shader.Use();
      shader.SetMat4("model", this->GetModelMatrix());
      glBindVertexArray(boundingBoxVAO);
      glDrawArrays(GL_LINES, 0, boundingBoxVertices.size());
      glBindVertexArray(0);
    }
  }

  std::shared_ptr<Object> Clone() const {
    auto newObj = std::make_shared<Object>();
    newObj->meshes = this->meshes;
    newObj->state.scale = this->state.scale;
    return newObj;
  }

  void lookAt(const glm::vec3& target) {
    glm::vec3 direction = glm::normalize(target - this->state.position);
    float pitch = glm::degrees(asin(direction.y));
    float yaw = glm::degrees(atan2(direction.z, direction.x));

    this->state.rotation.x = pitch;
    this->state.rotation.y = -yaw + 90.0f;
  }
};

}