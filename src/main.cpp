#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define USE_IMGUI 1

#if USE_IMGUI
#include "imgui\imgui.h"
#include "imgui\imgui_impl_glfw.h"
#include "imgui\imgui_impl_opengl3.h"
#endif

#include "OBJ_Loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

float fov = 45.0f;

float lastX = 0, lastY = 0;

float yaw = -90.0f;
float pitch = 0.0f;

int windowWidth = 800.0f;
int windowHeight = 600.0f;

// struct Vector2
// {
//   float X;
//   float Y;

//   Vector2()
//   {
//     X = 0.0f;
//     Y = 0.0f;
//   }

//   Vector2(float X, float Y)
//   {
//     this->X = X;
//     this->Y = Y;
//   }
//   bool operator==(const Vector2 &other) const
//   {
//     return (this->X == other.X && this->Y == other.Y);
//   }
//   bool operator!=(const Vector2 &other) const
//   {
//     return !(this->X == other.X && this->Y == other.Y);
//   }
//   Vector2 operator+(const Vector2 &right) const
//   {
//     return Vector2(this->X + right.X, this->Y + right.Y);
//   }
//   Vector2 operator-(const Vector2 &right) const
//   {
//     return Vector2(this->X - right.X, this->Y - right.Y);
//   }
//   Vector2 operator*(const float &other) const
//   {
//     return Vector2(this->X * other, this->Y * other);
//   }
// };

// struct Vector3
// {
//   float X;
//   float Y;
//   float Z;

//   Vector3()
//   {
//     X = 0.0f;
//     Y = 0.0f;
//     Z = 0.0f;
//   }

//   Vector3(float X, float Y, float Z)
//   {
//     this->X = X;
//     this->Y = Y;
//     this->Z = Z;
//   }

//   bool operator==(const Vector3 &other) const
//   {
//     return (this->X == other.X && this->Y == other.Y && this->Z == other.Z);
//   }

//   bool operator!=(const Vector3 &other) const
//   {
//     return !(this->X == other.X && this->Y == other.Y && this->Z == other.Z);
//   }
//   Vector3 operator+(const Vector3 &right) const
//   {
//     return Vector3(this->X + right.X, this->Y + right.Y, this->Z + right.Z);
//   }
//   Vector3 operator-(const Vector3 &right) const
//   {
//     return Vector3(this->X - right.X, this->Y - right.Y, this->Z - right.Z);
//   }
//   Vector3 operator*(const float &other) const
//   {
//     return Vector3(this->X * other, this->Y * other, this->Z * other);
//   }
//   Vector3 operator/(const float &other) const
//   {
//     return Vector3(this->X / other, this->Y / other, this->Z / other);
//   }
// };

struct AABB
{
  glm::vec3 min, max;
};

struct Vertex
{
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TextureCoordinate;

  Vertex() {}

  Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v)
  {
    this->Position = glm::vec3(x, y, z);
    this->Normal = glm::vec3(nx, ny, nz);
    this->TextureCoordinate = glm::vec2(u, v);
  }
};

struct FPSCounter
{
  double lastTime = 0.0;
  double frameTime = 0.0;

  void update()
  {
    double currentTime = glfwGetTime();
    frameTime = currentTime - lastTime;
    lastTime = currentTime;
  }
};

struct Timer
{
  double lastTime = 0.0;
  double deltaTime = 0.0;

  void update()
  {
    double currentTime = glfwGetTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
  }
};

// void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
//     // Forward to ImGui FIRST
//     ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

//     // Only process your own input if ImGui doesn't want it
//     ImGuiIO& io = ImGui::GetIO();
//     if (!io.WantCaptureMouse) {
//         // Your mouse handling code
//     }
// }

void mouseCallback(GLFWwindow *window, double xPos, double yPos)
{
  // ImGui_ImplGlfw_CursorPosCallback(window, xPos, yPos);
  float xOffset = xPos - lastX;
  float yOffset = lastY - yPos;
  if (lastX == 0 && lastY == 0)
  {
    xOffset = 0;
    yOffset = 0;
  }
  lastX = xPos;
  lastY = yPos;

  const float sensitivity = 0.1f;
  xOffset *= sensitivity;
  yOffset *= sensitivity;

  yaw += xOffset;
  pitch += yOffset;

  if (pitch > 89.0f)
    pitch = 89.0f;
  if (pitch < -89.0f)
    pitch = -89.0f;

  glm::vec3 direction;
  direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
  direction.y = sin(glm::radians(pitch));
  direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
  cameraFront = glm::normalize(direction);
}

void scrollCallback(GLFWwindow *window, double xoffset, double yOffset)
{
  fov -= (float)yOffset;
  if (fov < 1.0f)
    fov = 1.0f;
  if (fov > 45.0f)
    fov = 45.0f;
}

void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
  windowWidth = width;
  windowHeight = height;
  glViewport(0, 0, width, height);
}

class Shader
{
public:
  unsigned int Id;

  std::string shaderText;

  Shader(std::string fileName, GLenum shaderType)
  {
    if (!loadShaderFile(fileName))
      return;

    this->Id = glCreateShader(shaderType);

    const char *shaderString = shaderText.c_str();
    glShaderSource(this->Id, 1, &shaderString, NULL);
    glCompileShader(this->Id);
  }

  bool loadShaderFile(std::string fileName)
  {

    std::ifstream file(fileName.c_str());

    if (!file.is_open())
    {
      std::cerr << "Failed to open file." << std::endl;
      return false;
    }

    shaderText.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    file.close();

    return true;
  }

  void deleteShader()
  {
    glDeleteShader(this->Id);
  }

  void attachToProgram(unsigned int programId)
  {
    glAttachShader(programId, this->Id);
  }
};

class ShaderProgram
{
public:
  unsigned int Id;

  int modelLoc;
  int viewLoc;
  int projectionLoc;
  int texLoc;

  ShaderProgram(std::string vertexShaderFile, std::string fragmentShaderFile)
  {
    Shader vertexShader(vertexShaderFile, GL_VERTEX_SHADER);
    Shader fragmentShader(fragmentShaderFile, GL_FRAGMENT_SHADER);

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

  ShaderProgram(Shader &vertexShader, Shader &fragmentShader)
  {
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

  void use()
  {
    glUseProgram(this->Id);
  }

  void setMat4(const std::string &name, const glm::mat4 &mat) const
  {
    glUniformMatrix4fv(glGetUniformLocation(this->Id, name.c_str()), 1, GL_FALSE, &mat[0][0]);
  }
};

class Mesh
{
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

  Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
  {
    this->vertices = vertices;
    this->indices = indices;
    this->indexCount = indices.size();
    modelMatrix = glm::mat4(1.0f);
    init();
  }

  Mesh(std::string objFilePath, std::string textureFilePath)
  {
    modelMatrix = glm::mat4(1.0f);

    if (!loadObj(objFilePath) || !loadTexture(textureFilePath))
    {
      std::cerr << "Failed to load model or texture" << std::endl;
      return;
    }

    init();
  }

  void init()
  {
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    this->VAO = VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    this->VBO = VBO;

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);
    this->EBO = EBO;

    int vertexStride = sizeof(Vertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
  }

  bool loadObj(std::string objFilePath)
  {
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

  bool loadTextureFile(std::string textureFilePath, int &width, int &height, int &nrChannels, unsigned char *&data)
  {
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(textureFilePath.c_str(), &width, &height, &nrChannels, 0);
    if (!data)
    {
      std::cerr << "Failed to load texture" << std::endl;
      return false;
    }
    return true;
  }

  bool loadTexture(std::string textureFilePath = NULL)
  {
    // if (textureFilePath == NULL)
    //   return false;
    int width, height, nrChannels;
    unsigned char *data;
    if (!loadTextureFile(textureFilePath, width, height, nrChannels, data))
      return false;

    glGenTextures(1, &this->texture);
    glBindTexture(GL_TEXTURE_2D, this->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
  }

  glm::mat4 getModelMatrix()
  {
    return this->modelMatrix;
  }

  void render(ShaderProgram &shader)
  {
    this->render(shader, this->getModelMatrix());
  }

  void prepareRender(ShaderProgram &shader)
  {
    if (VAO == 0)
      return;
    shader.use();
    glBindVertexArray(this->VAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);
  }

  void draw()
  {
    glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
  }

  void render(ShaderProgram &shader, glm::mat4 modelMatrix)
  {
    this->prepareRender(shader);
    shader.setMat4("model", modelMatrix);

    this->draw();
  }

  void renderInstanced(ShaderProgram &shader, const std::vector<glm::mat4> &modelMatrices)
  {
    this->prepareRender(shader);

    for (const auto &modelMatrix : modelMatrices)
    {
      shader.setMat4("model", modelMatrix);
      this->draw();
    }
  }

  std::vector<Vertex> getVertices()
  {
    return this->vertices;
  }
};

class Object
{
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

  Object()
  {
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
    scale = glm::vec3(1.0f, 1.0f, 1.0f);
    modelMatrix = glm::mat4(1.0f);
    meshes = std::vector<Mesh>();
  }

  Object(Mesh mesh) : Object()
  {
    meshes.push_back(mesh);
    calculateBoundingBox();
  }

  Object(std::string objFilePath, float scale = 1.0f) : Object()
  {
    loadOBJ(objFilePath, scale);
  }

  bool loadOBJ(std::string path, float scale = 1.0f)
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

  void applyForce(const glm::vec3 &force)
  {
    this->acceleration = this->acceleration + force;
    this->needsUpdate = true;
  }

  void applyAcceleration(const glm::vec3 &acceleration)
  {
    this->velocity = this->velocity + acceleration;
    this->needsUpdate = true;
  }

  void applyVelocity(const glm::vec3 &vel)
  {
    this->velocity = vel;
    this->needsUpdate = true;
  }

  void applyGravity(const glm::vec3 &gravity, double deltaTime)
  {
    this->applyAcceleration(gravity * glm::vec3(deltaTime));
  }

  void update(double deltaTime)
  {
    // if (isStatic || !needsUpdate)
    //   return;
    this->applyAcceleration(this->acceleration);
    this->position = this->position + this->velocity * static_cast<float>(deltaTime) + glm::radians(0.0001f);
    this->acceleration = glm::vec3(0.0f);
  }

  void calculateBoundingBox()
  {
    glm::vec3 min = glm::vec3(FLT_MAX), max = glm::vec3(-FLT_MAX);
    for (auto &mesh : meshes)
    {
      for (auto &v : mesh.getVertices())
      {
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
    glBufferData(GL_ARRAY_BUFFER, boundingBoxVertices.size() * sizeof(glm::vec3), boundingBoxVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
  }

  bool intersects(const Object &other) const
  {
    glm::vec3 thisMin = this->position + this->aabb.min * this->scale;
    glm::vec3 thisMax = this->position + this->aabb.max * this->scale;
    glm::vec3 otherMin = other.position + other.aabb.min * other.scale;
    glm::vec3 otherMax = other.position + other.aabb.max * other.scale;
    return (thisMin.x <= otherMax.x && thisMax.x >= otherMin.x) &&
           (thisMin.y <= otherMax.y && thisMax.y >= otherMin.y) &&
           (thisMin.z <= otherMax.z && thisMax.z >= otherMin.z);
  }

  glm::mat4 getModelMatrix()
  {
    glm::mat4 model = glm::translate(this->modelMatrix, this->position);
    model = glm::rotate(model, glm::radians(this->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(this->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, this->scale);
    return model;
  }

  void render(ShaderProgram &shader)
  {
    for (auto &mesh : meshes)
      mesh.render(shader, this->getModelMatrix());
    if (boundingBoxVAO && touchedOtherObject)
    {
      shader.use();
      shader.setMat4("model", this->getModelMatrix());
      glBindVertexArray(boundingBoxVAO);
      glDrawArrays(GL_LINES, 0, boundingBoxVertices.size());
      glBindVertexArray(0);
    }
  }

  std::shared_ptr<Object> clone() const
  {
    auto newObj = std::make_shared<Object>();
    newObj->meshes = this->meshes;
    newObj->scale = this->scale;
    newObj->modelMatrix = this->modelMatrix;
    return newObj;
  }

  void lookAt(const glm::vec3 &target)
  {
    glm::vec3 direction = glm::normalize(target - this->position);
    float pitch = glm::degrees(asin(direction.y));
    float yaw = glm::degrees(atan2(direction.z, direction.x));

    this->rotation.x = pitch;
    this->rotation.y = -yaw + 90.0f;
  }
};

class Character : public Object
{
public:
};

class Camera
{
private:
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
  float fov, aspect, nearDist, farDist;
  unsigned int frustumVAO = 0, frustumVBO = 0;
  std::vector<glm::vec3> frustumVertices;

public:
  Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float fov, float aspect, float near, float far) : position(position), front{front}, up{up}, fov(fov), aspect(aspect), nearDist(near), farDist(far)
  {
    viewMatrix = glm::lookAt(position, position + front, up);
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, near, far);

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
        nearBottomLeft, farBottomLeft};

    glGenVertexArrays(1, &frustumVAO);
    glBindVertexArray(frustumVAO);
    glGenBuffers(1, &frustumVBO);
    glBindBuffer(GL_ARRAY_BUFFER, frustumVBO);
    glBufferData(GL_ARRAY_BUFFER, frustumVertices.size() * sizeof(glm::vec3), frustumVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
  }

  void setAspect(float aspect)
  {
    this->aspect = aspect;
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, nearDist, farDist);
  }

  void setPos(const glm::vec3 &pos)
  {
    this->position = pos;
    viewMatrix = glm::lookAt(position, position + front, up);
  }
  void setFront(const glm::vec3 &front)
  {
    this->front = front;
    viewMatrix = glm::lookAt(position, position + front, up);
  }
  glm::mat4 getViewMatrix() const
  {
    return viewMatrix;
  }
  glm::mat4 getProjectionMatrix() const
  {
    return projectionMatrix;
  }
  void render(ShaderProgram &shader) const
  {
    if (frustumVAO == 0)
      return;
    shader.use();
    glm::mat4 model = glm::inverse(viewMatrix);
    shader.setMat4("model", model);
    glBindVertexArray(frustumVAO);
    glDrawArrays(GL_LINES, 0, frustumVertices.size());
    glBindVertexArray(0);
  }
};

class Scene
{
private:
  std::vector<std::shared_ptr<Object>> objects;
  glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
  Timer timer;

public:
  Scene()
  {
    objects = std::vector<std::shared_ptr<Object>>();
  }

  void addModel(std::shared_ptr<Object> object)
  {
    objects.push_back(object);
  }

  void prepareRender(ShaderProgram &shader, const Camera &camera)
  {
    shader.use();
    shader.setMat4("view", camera.getViewMatrix());
    shader.setMat4("projection", camera.getProjectionMatrix());
  }

  void render(ShaderProgram &shader, const Camera &camera)
  {
    this->prepareRender(shader, camera);

    for (auto &model : objects)
      model->render(shader);

    camera.render(shader);

    this->endRender();
  }

  void endRender()
  {
    glBindVertexArray(0);
  }

  void update()
  {
    timer.update();
    for (auto &object : objects)
    {
      object->applyGravity(gravity, timer.deltaTime);
      object->update(timer.deltaTime);
    }
    resolveCollisions();
  }

  void resolveCollisions()
  {
    for (auto &object : objects)
    {
      if (object->position.y < 0.0f)
      {
        object->position.y = 0.0f;
        object->velocity.y *= -0.1f;

        object->touchedGround = true;

        if (object->velocity.y < 0.01f && object->velocity.y > -0.01f)
          object->needsUpdate = false;
      }
      else
      {
        object->touchedGround = false;
      }

      bool foundTouch = false;
      for (auto &otherObject : objects)
      {
        if (otherObject == object)
          continue;

        if (object->intersects(*otherObject))
        {
          if (!object->touchedOtherObject)
            std::cout << "Yooo I hit someone bro";
          object->touchedOtherObject = true;
          otherObject->touchedOtherObject = true;
          foundTouch = true;
          break;
        }
      }

      if (!foundTouch)
      {
        object->touchedOtherObject = false;
      }
    }
  }

  const std::vector<std::shared_ptr<Object>> &getModels() const
  {
    return objects;
  }

  double getDeltaTime()
  {
    return timer.deltaTime;
  }
};

class Window
{
public:
  int width;
  int height;
  GLFWwindow *window;
  std::unique_ptr<Scene> scene;
  std::unique_ptr<Camera> playerCamera;
  ShaderProgram *shader;
  FPSCounter fpsCounter;

  std::shared_ptr<Character> player;

  std::vector<std::shared_ptr<Character>> npcs;

  double lastUpdateTime = 0.0f;

  bool canJump = true;

  Window(int width, int height) : width(width), height(height)
  {
    if (!initGlfw())
      return;
    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    // glCullFace(GL_FRONT);

    this->setClearColor(0.1f, 0.4f, 1.0f, 1.0f);

    this->scene = std::make_unique<Scene>();
    this->shader = new ShaderProgram("VertexShader.glsl", "FragmentShader.glsl");
    this->playerCamera = std::make_unique<Camera>(cameraPos, cameraFront, cameraUp, fov, (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    startMouseCapture();

    this->npcs = std::vector<std::shared_ptr<Character>>();

#if USE_IMGUI
    const char *glsl_version = "#version 330 core";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
#endif

    loadModels();
  }

  bool initGlfw()
  {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    this->window = glfwCreateWindow(width, height, "FoxEngine", NULL, NULL);
    if (window == NULL)
    {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return false;
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(0); // Enable vsync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return false;
    }

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, scrollCallback);
    // glfwSetMouseButtonCallback(window, mouseButtonCallback);
    return true;
  }

  void loadModels()
  {
    loadStaticOBJ("resources/models/collisiontest.obj");
    this->player = std::static_pointer_cast<Character>(loadOBJ("resources/models/citizen.obj", 0.1f));

    // loadOBJ("resources/models/citizen.obj", 0.1f);
    auto obj2 = std::static_pointer_cast<Character>(loadOBJ("resources/models/citizen.obj", 0.1f));
    obj2->position = glm::vec3(5.0f, 0.0f, 0.0f);

    obj2->meshes[0].loadTexture("resources/textures/chau_zombfacemap.png");
    obj2->meshes[1].loadTexture("resources/textures/citizenzomb_sheet_reference.png");

    // Look at player
    obj2->lookAt(this->player->position);

    npcs.push_back(obj2);
    spawnZombies(10);
    // for (int i = 0; i < 10; i++) {
    //   for (int j = 0; j < 5; j++) {
    //   auto object = obj2->clone();
    //   object->position = glm::vec3(i * 2.0f, j * 5.0f, 0.0f);
    // }
    // }
    // this->player->position = cameraPos - cameraFront * 5.0f;
  }

  void spawnZombies(int count = 10)
  {
    const float minDistance = 20.0f;
    const float minDistanceSq = minDistance * minDistance;

    auto zombieTemplate = std::static_pointer_cast<Character>(loadOBJButDontAdd("resources/models/citizen.obj", 0.1f));
    for (int i = 0; i < count; i++)
    {
      float x = static_cast<float>(rand() % 100 - 50);
      float z = static_cast<float>(rand() % 100 - 50);

      float dx = x - player->position.x;
      float dz = z - player->position.z;
      float distanceSq = dx * dx + dz * dz;

      if (distanceSq < minDistanceSq) {
        x += minDistanceSq;
        z += minDistanceSq;
      }

      auto npc = std::static_pointer_cast<Character>(zombieTemplate->clone());
      npc->position = glm::vec3(x, 0.0f, z);

      npc->meshes[0].loadTexture("resources/textures/chau_zombfacemap.png");
      npc->meshes[1].loadTexture("resources/textures/citizenzomb_sheet_reference.png");

      this->scene->addModel(npc);

      npcs.push_back(npc);
    }
  }

  std::shared_ptr<Object> loadOBJ(std::string path, float scale = 1.0f)
  {
    std::shared_ptr<Object> model = std::make_shared<Object>(path, scale);
    this->scene->addModel(model);
    return model;
  }

  std::shared_ptr<Object> loadOBJButDontAdd(std::string path, float scale = 1.0f)
  {
    return std::make_shared<Object>(path, scale);
  }

  bool loadStaticOBJ(std::string path, float scale = 1.0f)
  {
    std::shared_ptr<Object> model = std::make_shared<Object>(path, scale);
    model->isStatic = true;
    model->needsUpdate = false;
    this->scene->addModel(model);

    return true;
  }

  double getDeltaTime()
  {
    return glfwGetTime();
  }

  void setClearColor(float r, float g, float b, float a)
  {
    glClearColor(r, g, b, a);
  }

  void clear()
  {
    // glClearColor(0.1f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void redraw()
  {
    this->clear();

    scene->render(*(this->shader), *(this->playerCamera));

#if USE_IMGUI
    fpsCounter.update();
    drawImGui();
#endif

    glfwSwapBuffers(this->window);
    glfwPollEvents();
  }

  void update()
  {
    scene->update();

    // this->player->position = cameraPos + cameraFront * 2.0f + glm::vec3(0.0f, -6.5f, 0.0f);
    // this->player->rotation.y = yaw / 360.0f + 90.0f;
  }

  void processInput()
  {
    double deltaTime = scene->getDeltaTime();

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      stopMouseCapture();
    if (ImGui::GetIO().WantCaptureMouse)
    {
      stopMouseCapture();
    }
    else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
      startMouseCapture();
    }

    const float cameraSpeed = 10.0f * deltaTime;
    glm::vec3 horizontalFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(horizontalFront, cameraUp));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      this->player->position += cameraSpeed * horizontalFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      this->player->position -= cameraSpeed * horizontalFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      this->player->position -= right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      this->player->position += right * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
      this->player->position += cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
      this->player->position -= cameraUp * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && canJump)
    {

      // this->player->acceleration.y = 10.0f;
      this->player->applyForce(glm::vec3(0.0f, 10.0f, 0.0f));
      canJump = false;
    }
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
    {
      std::shared_ptr<Object> newObj = this->player->clone();
      newObj->position = this->player->position + horizontalFront * 2.0f;
      glm::vec3 dir = glm::normalize(this->player->position - newObj->position);
      newObj->rotation.y = glm::degrees(atan2(dir.z, dir.x)) - 90.0f;
      newObj->rotation.x = 0.0f;
      this->scene->addModel(newObj);
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
      enableWireframeMode();
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
      disableWireframeMode();
  }

  void enableWireframeMode()
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  }

  void disableWireframeMode()
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  }

  void startMouseCapture()
  {
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  void stopMouseCapture()
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(window, NULL);
  }

  bool shouldClose()
  {
    return glfwWindowShouldClose(this->window);
  }

  void destroy()
  {
    glfwDestroyWindow(this->window);
    glfwTerminate();
  }

#if USE_IMGUI
  int drawImGui()
  {
    glDisable(GL_DEPTH_TEST);

    ImGui::Begin("Window");
    ImGui::SetWindowFocus();
    ImGui::Text("Hello, World!");
    ImGui::Text("FPS %.1f", fpsCounter.frameTime > 0.0 ? 1.0 / fpsCounter.frameTime : 0.0);
    ImGui::Text("Objects: %zu", this->scene->getModels().size());
    size_t totalVertices = 0;
    for (auto &obj : this->scene->getModels())
      for (auto &mesh : obj->meshes)
        totalVertices += mesh.getVertices().size();
    ImGui::Text("Vertices: %zu", totalVertices);
    size_t needsUpdateCount = 0;
    for (auto &obj : this->scene->getModels())
    {
      if (obj->needsUpdate)
        needsUpdateCount++;
    }
    ImGui::Text("Needs Update: %zu", needsUpdateCount);
    if (ImGui::Button("Start", ImVec2(50, 20)))
    {
      std::cout << "Button clicked!" << std::endl;
    }
    Object *model = this->player.get();
    ImGui::SliderFloat3("Position", &model->position.x, -10.0f, 10.0f);
    for (size_t i = 0; i < this->npcs.size(); ++i)
    {
      ImGui::Text("NPC %zu", i);
      ImGui::SliderFloat3(("Position##npc" + std::to_string(i)).c_str(), &this->npcs[i]->position.x, -10.0f, 10.0f);
      ImGui::SliderFloat3(("Rotation##npc" + std::to_string(i)).c_str(), &this->npcs[i]->rotation.x, -180.0f, 180.0f);
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glEnable(GL_DEPTH_TEST);
    return 0;
  }
#endif
};

int main()
{
  Window window(800, 600);

  glm::vec3 playerHeight = glm::vec3(0.0f, 6.5f, 0.0f);

  while (!window.shouldClose())
  {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (window.player->touchedGround)
    {
      window.canJump = true;
    }
    window.processInput();
    window.player->rotation.y = -yaw + 90.0f;
    glm::vec3 pos = window.player->position + playerHeight;
    cameraPos = pos - cameraFront * 5.0f;
    window.playerCamera->setPos(cameraPos);
    cameraTarget = pos;

    window.playerCamera->setAspect((float)windowWidth / (float)windowHeight);
    // window.playerCamera->setPos(cameraPos);

    window.playerCamera->setFront(glm::normalize(cameraTarget - cameraPos));

    for (auto &npc : window.npcs)
    {
      npc->lookAt(pos * glm::vec3(1.0f, 0.0f, 1.0f));
      npc->applyVelocity(glm::normalize(pos - npc->position) * glm::vec3(1.0f, 0.0f, 1.0f) * 0.2f * glm::vec3(window.getDeltaTime()));
      npc->needsUpdate = true;
    }
    for (auto &npc : window.npcs)
    {
      if (window.player->intersects(*npc))
      {
        std::cout << "Player intersects with NPC" << std::endl;
        std::cout << "YOU FUCKING DIED!!!!" << std::endl;
      }
    }
    window.redraw();
    window.update();
  }

  window.destroy();
  return 0;
}