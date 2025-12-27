#pragma once
#include <glad/glad.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef OBJ_LOADER
#include <OBJ_LOADER.h>
#endif

#include <stb_image.h>

namespace fe {


struct AABB {
  glm::vec3 min, max;
};

struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TextureCoordinate;

  Vertex() {}

  Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v) {
    this->Position = glm::vec3(x, y, z);
    this->Normal = glm::vec3(nx, ny, nz);
    this->TextureCoordinate = glm::vec2(u, v);
  }
};

struct Timer {
  std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

 public:
  double lastTime = 0.0;
  double deltaTime = 0.0;

  Timer() { this->reset(); }

  void update() {
    double currentTime = getTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
  }

  void reset() {
    startTime = std::chrono::high_resolution_clock::now();
  }

  double getTime() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - startTime;
    return elapsed.count();
  }
};

class Shader {
 public:
  unsigned int Id;

  std::string shaderText;

  Shader(std::string fileName, GLenum shaderType) {
    if (!loadShaderFile(fileName)) return;

    this->Id = glCreateShader(shaderType);

    const char* shaderString = shaderText.c_str();
    glShaderSource(this->Id, 1, &shaderString, NULL);
    glCompileShader(this->Id);
  }

  bool loadShaderFile(std::string fileName) {
    std::ifstream file(fileName.c_str());

    if (!file.is_open()) {
      std::cerr << "Failed to open file." << std::endl;
      return false;
    }

    shaderText.assign((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

    file.close();

    return true;
  }

  void deleteShader() { glDeleteShader(this->Id); }

  void attachToProgram(unsigned int programId) {
    glAttachShader(programId, this->Id);
  }
};

class ShaderProgram {
 public:
  unsigned int Id;

  int modelLoc;
  int viewLoc;
  int projectionLoc;
  int texLoc;

  ShaderProgram(std::string vertexShaderFile, std::string fragmentShaderFile)
      : ShaderProgram(Shader(vertexShaderFile, GL_VERTEX_SHADER),
                      Shader(fragmentShaderFile, GL_FRAGMENT_SHADER)) {}

  ShaderProgram(Shader& vertexShader, Shader& fragmentShader) {
    Id = glCreateProgram();

    vertexShader.attachToProgram(Id);
    fragmentShader.attachToProgram(Id);

    glLinkProgram(Id);

    vertexShader.deleteShader();
    fragmentShader.deleteShader();

    modelLoc = glGetUniformLocation(this->Id, "model");
    viewLoc = glGetUniformLocation(this->Id, "view");
    projectionLoc = glGetUniformLocation(this->Id, "projection");
    texLoc = glGetUniformLocation(this->Id, "ourTexture");
    glUniform1i(texLoc, 0);
  }

  void use() { glUseProgram(this->Id); }

  void setMat4(const std::string& name, const glm::mat4& mat) const {
    glUniformMatrix4fv(glGetUniformLocation(this->Id, name.c_str()), 1,
                       GL_FALSE, &mat[0][0]);
  }
};

class Mesh {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  unsigned int indexCount;

  unsigned int VAO = 0;
  unsigned int VBO = 0;
  unsigned int EBO = 0;
  unsigned int texture = 0;

 public:
  glm::mat4 modelMatrix;

  Mesh() {}

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices) {
    this->vertices = vertices;
    this->indices = indices;
    this->indexCount = indices.size();
    modelMatrix = glm::mat4(1.0f);
    init();
  }

  Mesh(std::string objFilePath, std::string textureFilePath) {
    modelMatrix = glm::mat4(1.0f);

    if (!loadObj(objFilePath) || !loadTexture(textureFilePath)) {
      std::cerr << "Failed to load model or texture" << std::endl;
      return;
    }

    init();
  }

  void init() {
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    this->VAO = VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_STATIC_DRAW);
    this->VBO = VBO;

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int),
                 indices.data(), GL_STATIC_DRAW);
    this->EBO = EBO;

    int vertexStride = sizeof(Vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride,
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride,
                          (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
  }

  bool loadObj(std::string objFilePath);

  bool loadTextureFile(std::string textureFilePath, int& width, int& height,
                       int& nrChannels, unsigned char*& data) {
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(textureFilePath.c_str(), &width, &height, &nrChannels, 0);
    if (!data) {
      std::cerr << "Failed to load texture" << std::endl;
      return false;
    }
    return true;
  }

  bool loadTexture(std::string textureFilePath = NULL) {
    // if (textureFilePath == NULL)
    //   return false;
    int width, height, nrChannels;
    unsigned char* data;
    if (!loadTextureFile(textureFilePath, width, height, nrChannels, data))
      return false;

    glGenTextures(1, &this->texture);
    glBindTexture(GL_TEXTURE_2D, this->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
  }

  glm::mat4 getModelMatrix() { return this->modelMatrix; }

  void render(ShaderProgram& shader) {
    this->render(shader, this->getModelMatrix());
  }

  void prepareRender(ShaderProgram& shader) {
    if (VAO == 0) return;
    shader.use();
    glBindVertexArray(this->VAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);
  }

  void draw() {
    glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
  }

  void render(ShaderProgram& shader, glm::mat4 modelMatrix) {
    this->prepareRender(shader);
    shader.setMat4("model", modelMatrix);

    this->draw();
  }

  void renderInstanced(ShaderProgram& shader,
                       const std::vector<glm::mat4>& modelMatrices) {
    this->prepareRender(shader);

    for (const auto& modelMatrix : modelMatrices) {
      shader.setMat4("model", modelMatrix);
      this->draw();
    }
  }

  std::vector<Vertex> getVertices() { return this->vertices; }
};

class Object {
 private:
 public:
  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 velocity;
  glm::vec3 acceleration;

  glm::vec3 scale;
  glm::mat4 modelMatrix;

  std::vector<Mesh> meshes;

  bool isStatic = false;

  bool needsUpdate = true;

  bool touchedGround = false;

  bool touchedOtherObject = false;

  AABB aabb;

  unsigned int boundingBoxVAO = 0, boundingBoxVBO = 0;

  std::vector<glm::vec3> boundingBoxVertices;

  // bool needsUpdate = true;

  Object() {
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    scale = glm::vec3(1.0f, 1.0f, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    meshes = std::vector<Mesh>();
  }

  Object(Mesh mesh) : Object() {
    meshes.push_back(mesh);
    calculateBoundingBox();
  }

  Object(std::string objFilePath, float scale = 1.0f) : Object() {
    loadOBJ(objFilePath, scale);
  }

  bool loadOBJ(std::string path, float scale = 1.0f);

  void applyForce(const glm::vec3& force) {
    this->acceleration = this->acceleration + force;
    this->needsUpdate = true;
  }

  void applyAcceleration(const glm::vec3& acceleration) {
    this->velocity = this->velocity + acceleration;
    this->needsUpdate = true;
  }

  void applyVelocity(const glm::vec3& vel) {
    this->velocity = vel;
    this->needsUpdate = true;
  }

  void applyGravity(const glm::vec3& gravity, double deltaTime) {
    this->applyAcceleration(gravity * glm::vec3(deltaTime));
  }

  void update(double deltaTime) {
    // if (isStatic || !needsUpdate)
    //   return;
    this->applyAcceleration(this->acceleration);
    this->position = this->position +
                     this->velocity * static_cast<float>(deltaTime) +
                     glm::radians(0.0001f);
    this->acceleration = glm::vec3(0.0f);
  }

  void calculateBoundingBox() {
    glm::vec3 min = glm::vec3(FLT_MAX), max = glm::vec3(-FLT_MAX);
    for (auto& mesh : meshes) {
      for (auto& v : mesh.getVertices()) {
        min = glm::min(min, v.Position);
        max = glm::max(max, v.Position);
      }
    }
    this->aabb.min = min;
    this->aabb.max = max;
    boundingBoxVertices = {
        // bottom
        glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z), glm::vec3(min.x, min.y, max.z),
        glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, min.y, min.z),
        // top
        glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z),
        glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z),
        glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, max.y, min.z),
        // sides
        glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z),
        glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z),
        glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z)};
    glGenVertexArrays(1, &boundingBoxVAO);
    glBindVertexArray(boundingBoxVAO);
    glGenBuffers(1, &boundingBoxVBO);
    glBindBuffer(GL_ARRAY_BUFFER, boundingBoxVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 boundingBoxVertices.size() * sizeof(glm::vec3),
                 boundingBoxVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                          (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
  }

  bool intersects(const Object& other) const {
    glm::vec3 thisMin = this->position + this->aabb.min * this->scale;
    glm::vec3 thisMax = this->position + this->aabb.max * this->scale;
    glm::vec3 otherMin = other.position + other.aabb.min * other.scale;
    glm::vec3 otherMax = other.position + other.aabb.max * other.scale;
    return (thisMin.x <= otherMax.x && thisMax.x >= otherMin.x) &&
           (thisMin.y <= otherMax.y && thisMax.y >= otherMin.y) &&
           (thisMin.z <= otherMax.z && thisMax.z >= otherMin.z);
  }

  glm::mat4 getModelMatrix() {
    glm::mat4 model = glm::translate(this->modelMatrix, this->position);
    model = glm::rotate(model, glm::radians(this->rotation.y),
                        glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(this->rotation.x),
                        glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, this->scale);
    return model;
  }

  void render(ShaderProgram& shader) {
    for (auto& mesh : meshes) mesh.render(shader, this->getModelMatrix());
    if (boundingBoxVAO && touchedOtherObject) {
      shader.use();
      shader.setMat4("model", this->getModelMatrix());
      glBindVertexArray(boundingBoxVAO);
      glDrawArrays(GL_LINES, 0, boundingBoxVertices.size());
      glBindVertexArray(0);
    }
  }

  std::shared_ptr<Object> clone() const {
    auto newObj = std::make_shared<Object>();
    newObj->meshes = this->meshes;
    newObj->scale = this->scale;
    newObj->modelMatrix = this->modelMatrix;
    return newObj;
  }

  void lookAt(const glm::vec3& target) {
    glm::vec3 direction = glm::normalize(target - this->position);
    float pitch = glm::degrees(asin(direction.y));
    float yaw = glm::degrees(atan2(direction.z, direction.x));

    this->rotation.x = pitch;
    this->rotation.y = -yaw + 90.0f;
  }
};

class Character : public Object {
 public:
};

class Camera {
 private:
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::mat4 viewMatrix;
  unsigned int frustumVAO = 0, frustumVBO = 0;
  std::vector<glm::vec3> frustumVertices;

 public:
  float fov, aspect, nearDist, farDist;
  glm::mat4 projectionMatrix;

  Camera() {}

  Camera(float nearDist, float farDist)
      : nearDist(nearDist), farDist(farDist) {};

  Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float fov,
         float aspect, float nearDist, float farDist);

  void setAspect(float aspect) {
    this->aspect = aspect;
    projectionMatrix =
        glm::perspective(glm::radians(fov), aspect, nearDist, farDist);
  }

  void setPos(const glm::vec3& pos) {
    this->position = pos;
    viewMatrix = glm::lookAt(position, position + front, up);
  }
  void setFront(const glm::vec3& front) {
    this->front = front;
    updateView(position, front, up);
  }

  void update(glm::vec3 position, glm::quat orientation, glm::vec4 fov) {
    updateView(position, orientation);
    updateProjection(fov);
  }

  void updateView(glm::vec3 position, glm::vec3 front, glm::vec3 up) {
    viewMatrix = glm::lookAt(position, position + front, up);
  }

  void updateView(glm::vec3 position, glm::quat orientation) {
    updateView(position, orientation * glm::vec3(0.0f, 0.0f, -1.0f), orientation * glm::vec3(0.0f, 1.0f, 0.0f));
  }

  void updateProjection(float fov, float aspect) {
    projectionMatrix =
        glm::perspective(glm::radians(fov), aspect, nearDist, farDist);
  }

  void updateProjection(glm::vec4 fov) {
    // fov = glm::vec4(tan(fov.x), tan(fov.y), tan(fov.z), tan(fov.w));
    fov = glm::tan(fov) * nearDist;
    projectionMatrix = glm::frustum(fov.x, fov.y, fov.z, fov.w, nearDist, farDist);
  }
  glm::mat4 getViewMatrix() const { return viewMatrix; }
  glm::mat4 getProjectionMatrix() const { return projectionMatrix; }
  void render(ShaderProgram& shader) const {
    if (frustumVAO == 0) return;
    shader.use();
    glm::mat4 model = glm::inverse(viewMatrix);
    shader.setMat4("model", model);
    glBindVertexArray(frustumVAO);
    glDrawArrays(GL_LINES, 0, frustumVertices.size());
    glBindVertexArray(0);
  }
};

class Scene {
 private:
  std::vector<std::shared_ptr<Object>> objects;
  glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
  Timer timer;

 public:
  Scene() {
    objects = std::vector<std::shared_ptr<Object>>();
    this->enableDepthTest();
    this->enableFaceCulling();
  }

  void enableDepthTest() { glEnable(GL_DEPTH_TEST); }

  void enableFaceCulling() { glEnable(GL_CULL_FACE); }

  void addModel(std::shared_ptr<Object> object) { objects.push_back(object); }

  void prepareRender(ShaderProgram& shader, const Camera& camera) {
    this->clear();
    shader.use();
    shader.setMat4("view", camera.getViewMatrix());
    shader.setMat4("projection", camera.getProjectionMatrix());
  }

  void clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

  void render(ShaderProgram& shader, const Camera& camera, int width, int height);

  void render(ShaderProgram& shader, const Camera& camera) {
    this->prepareRender(shader, camera);

    for (auto& model : objects) model->render(shader);

    camera.render(shader);

    this->endRender();
  }

  void endRender() { glBindVertexArray(0); }

  void update() {
    timer.update();
    for (auto& object : objects) {
      object->applyGravity(gravity, timer.deltaTime);
      object->update(timer.deltaTime);
    }
    resolveCollisions();
  }

  void resolveCollisions() {
    for (auto& object : objects) {
      if (object->position.y < 0.0f) {
        object->position.y = 0.0f;
        object->velocity.y *= -0.1f;

        object->touchedGround = true;

        if (object->velocity.y < 0.01f && object->velocity.y > -0.01f)
          object->needsUpdate = false;
      } else {
        object->touchedGround = false;
      }

      bool foundTouch = false;
      for (auto& otherObject : objects) {
        if (otherObject == object) continue;

        if (object->intersects(*otherObject)) {
          if (!object->touchedOtherObject)
            std::cout << "Intersected" << std::endl;
          object->touchedOtherObject = true;
          otherObject->touchedOtherObject = true;
          foundTouch = true;
          break;
        }
      }

      if (!foundTouch) {
        object->touchedOtherObject = false;
      }
    }
  }

  std::vector<std::shared_ptr<Object>>& getModels() { return objects; }

  double getDeltaTime() { return timer.deltaTime; }

  void resize(int width, int height) {
    glViewport(0, 0, width, height);
  }
};

}  // namespace fe