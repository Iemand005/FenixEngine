#define STB_IMAGE_IMPLEMENTATION
#define OBJ_LOAdER
#include "engine.h"
#include "OBJ_Loader.h"


namespace fe {


  Camera::Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float fov, float aspect, float nearDist, float farDist) : position(position), front{front}, up{up}, fov(fov), aspect(aspect), nearDist(nearDist), farDist(farDist)
    {
      viewMatrix = glm::lookAt(position, position + front, up);
      projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearDist, farDist);

      // Compute frustum vertices
      glm::vec3 right = glm::normalize(glm::cross(front, up));
      float tanHalfFov = tan(glm::radians(fov / 2.0f));
      float nearHeight = 2 * tanHalfFov * nearDist;
      float farHeight = 2 * tanHalfFov * farDist;
      float nearWidth = nearHeight * aspect;
      float farWidth = farHeight * aspect;

      glm::vec3 nearCenter = front * nearDist;
      glm::vec3 farCenter = front * farDist;

      glm::vec3 nearTopLeft = nearCenter + up * (nearHeight / 2) - right * (nearWidth / 2);
      glm::vec3 nearTopRight = nearCenter + up * (nearHeight / 2) + right * (nearWidth / 2);
      glm::vec3 nearBottomLeft = nearCenter - up * (nearHeight / 2) - right * (nearWidth / 2);
      glm::vec3 nearBottomRight = nearCenter - up * (nearHeight / 2) + right * (nearWidth / 2);

      glm::vec3 farTopLeft = farCenter + up * (farHeight / 2) - right * (farWidth / 2);
      glm::vec3 farTopRight = farCenter + up * (farHeight / 2) + right * (farWidth / 2);
      glm::vec3 farBottomLeft = farCenter - up * (farHeight / 2) - right * (farWidth / 2);
      glm::vec3 farBottomRight = farCenter - up * (farHeight / 2) + right * (farWidth / 2);

      frustumVertices = {
          // near plane
          nearTopLeft, nearTopRight,
          nearTopRight, nearBottomRight,
          nearBottomRight, nearBottomLeft,
          nearBottomLeft, nearTopLeft,
          // far plane
          farTopLeft, farTopRight,
          farTopRight, farBottomRight,
          farBottomRight, farBottomLeft,
          farBottomLeft, farTopLeft,
          // sides
          nearTopLeft, farTopLeft,
          nearTopRight, farTopRight,
          nearBottomRight, farBottomRight,
          nearBottomLeft, farBottomLeft
        };

      glGenVertexArrays(1, &frustumVAO);
      glBindVertexArray(frustumVAO);
      glGenBuffers(1, &frustumVBO);
      glBindBuffer(GL_ARRAY_BUFFER, frustumVBO);
      glBufferData(GL_ARRAY_BUFFER, frustumVertices.size() * sizeof(glm::vec3), frustumVertices.data(), GL_STATIC_DRAW);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
      glEnableVertexAttribArray(0);
      glBindVertexArray(0);
    }

    bool Mesh::loadObj(std::string objFilePath) {
      objl::Loader objectLoader;

      bool success = objectLoader.LoadFile(objFilePath);
      if (!success)
        return false;

      this->vertices = std::vector<Vertex>(objectLoader.LoadedVertices.size());

      for (int i = 0; i < this->vertices.size(); i++)
      {
        objl::Vertex v = objectLoader.LoadedVertices[i];
        this->vertices[i] = Vertex(v.Position.X, v.Position.Y, v.Position.Z, v.Normal.X, v.Normal.Y, v.Normal.Z, v.TextureCoordinate.X, v.TextureCoordinate.Y);
      }

      this->indices = std::vector<unsigned int>(objectLoader.LoadedIndices.size());

      for (size_t i = 0; i < this->indices.size(); i++)
        this->indices[i] = objectLoader.LoadedIndices[i];

      return true;
    }

    bool Object::loadOBJ(std::string path, float scale)
    {
      objl::Loader objectLoader;

      bool success = objectLoader.LoadFile(path);
      if (!success)
        return false;

      for (auto &loadedMesh : objectLoader.LoadedMeshes)
      {
        std::cout << "Mesh Name: " << loadedMesh.MeshName << std::endl;
        std::cout << "Vertices: " << loadedMesh.Vertices.size() << std::endl;
        std::cout << "Indices: " << loadedMesh.Indices.size() << std::endl;

        auto vertices = std::vector<Vertex>(loadedMesh.Vertices.size());
        auto indices = std::vector<unsigned int>(loadedMesh.Indices.size());

        for (int i = 0; i < loadedMesh.Vertices.size(); i++)
        {
          objl::Vertex v = loadedMesh.Vertices[i];
          vertices[i] = Vertex(v.Position.X, v.Position.Y, v.Position.Z, v.Normal.X, v.Normal.Y, v.Normal.Z, v.TextureCoordinate.X, v.TextureCoordinate.Y);
        }

        for (size_t i = 0; i < indices.size(); i++)
          indices[i] = loadedMesh.Indices[i];

        Mesh mesh(vertices, indices);
        mesh.loadTexture(loadedMesh.MeshMaterial.map_Kd);

        this->meshes.push_back(mesh);
      }

      this->scale = glm::vec3(scale);

      calculateBoundingBox();

      return true;
    }


    void Scene::render(ShaderProgram &shader, const Camera &camera, int width, int height) {
      glViewport(0, 0, width, height);
      this->render(shader, camera);
    }
}