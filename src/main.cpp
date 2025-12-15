#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui\imgui.h"
#include "imgui\imgui_impl_glfw.h"
#include "imgui\imgui_impl_opengl3.h"

#include "OBJ_Loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

float fov = 45.0f;

float lastX = 400, lastY = 300;

float yaw = -90.0f;
float pitch = 0.0f;

int windowWidth = 800.0f;
int windowHeight = 600.0f;

struct Vector2
{
  float X;
  float Y;

  Vector2()
  {
    X = 0.0f;
    Y = 0.0f;
  }

  Vector2(float X, float Y)
  {
    this->X = X;
    this->Y = Y;
  }
  bool operator==(const Vector2 &other) const
  {
    return (this->X == other.X && this->Y == other.Y);
  }
  bool operator!=(const Vector2 &other) const
  {
    return !(this->X == other.X && this->Y == other.Y);
  }
  Vector2 operator+(const Vector2 &right) const
  {
    return Vector2(this->X + right.X, this->Y + right.Y);
  }
  Vector2 operator-(const Vector2 &right) const
  {
    return Vector2(this->X - right.X, this->Y - right.Y);
  }
  Vector2 operator*(const float &other) const
  {
    return Vector2(this->X * other, this->Y * other);
  }
};

struct Vector3
{
  float X;
  float Y;
  float Z;

  Vector3()
  {
    X = 0.0f;
    Y = 0.0f;
    Z = 0.0f;
  }

  Vector3(float X, float Y, float Z)
  {
    this->X = X;
    this->Y = Y;
    this->Z = Z;
  }

  bool operator==(const Vector3 &other) const
  {
    return (this->X == other.X && this->Y == other.Y && this->Z == other.Z);
  }

  bool operator!=(const Vector3 &other) const
  {
    return !(this->X == other.X && this->Y == other.Y && this->Z == other.Z);
  }
  Vector3 operator+(const Vector3 &right) const
  {
    return Vector3(this->X + right.X, this->Y + right.Y, this->Z + right.Z);
  }
  Vector3 operator-(const Vector3 &right) const
  {
    return Vector3(this->X - right.X, this->Y - right.Y, this->Z - right.Z);
  }
  Vector3 operator*(const float &other) const
  {
    return Vector3(this->X * other, this->Y * other, this->Z * other);
  }
  Vector3 operator/(const float &other) const
  {
    return Vector3(this->X / other, this->Y / other, this->Z / other);
  }
};

struct Vertex
{
  Vector3 Position;
  Vector3 Normal;
  Vector2 TextureCoordinate;

  Vertex() {}

  Vertex(float X, float Y, float Z, float NX, float NY, float NZ, float U, float V)
  {
    this->Position = Vector3(X, Y, Z);
    this->Normal = Vector3(NX, NY, NZ);
    this->TextureCoordinate = Vector2(U, V);
  }
};

void mouseCallback(GLFWwindow *window, double xpos, double ypos)
{
  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
  lastX = xpos;
  lastY = ypos;

  const float sensitivity = 0.1f;
  xoffset *= sensitivity;
  yoffset *= sensitivity;

  yaw += xoffset;
  pitch += yoffset;

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

void scrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
  fov -= (float)yoffset;
  if (fov < 1.0f)
    fov = 1.0f;
  if (fov > 45.0f)
    fov = 45.0f;
}

void startMouseCapture(GLFWwindow *window)
{
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void stopMouseCapture(GLFWwindow *window)
{
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glfwSetCursorPosCallback(window, NULL);
}

void enableWireframeMode()
{
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void disableWireframeMode()
{
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void processInput(GLFWwindow *window)
{
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    stopMouseCapture(window);
  ;
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    startMouseCapture(window);

  const float cameraSpeed = 0.005f; // adjust accordingly
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    cameraPos += cameraSpeed * cameraFront;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    cameraPos -= cameraSpeed * cameraFront;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    enableWireframeMode();
  if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
    disableWireframeMode();
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

  ShaderProgram(Shader *vertexShader, Shader *fragmentShader)
  {
    Id = glCreateProgram();

    vertexShader->attachToProgram(Id);
    fragmentShader->attachToProgram(Id);
    glLinkProgram(Id);

    vertexShader->deleteShader();
    fragmentShader->deleteShader();

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

class Model
{
  Vertex *vertices;
  unsigned int *indices;

  unsigned int vertexCount;
  unsigned int indexCount;
  
  unsigned int VAO = 0;
  unsigned int VBO = 0;
  unsigned int EBO = 0;
  unsigned int texture = 0;

public:
  Model() {}

  Model(std::string objFilePath, std::string textureFilePath)
  {
    loadObj(objFilePath);
    loadTexture(textureFilePath);

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    this->VAO = VAO;

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    this->VBO = VBO;

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(int), indices, GL_STATIC_DRAW);
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

    this->vertexCount = objectLoader.LoadedVertices.size();
    this->vertices = new Vertex[this->vertexCount];

    for (int i = 0; i < this->vertexCount; i++)
    {
      objl::Vertex v = objectLoader.LoadedVertices[i];
      this->vertices[i] = Vertex(v.Position.X, v.Position.Y, v.Position.Z, v.Normal.X, v.Normal.Y, v.Normal.Z, v.TextureCoordinate.X, v.TextureCoordinate.Y);
    }

    this->indexCount = objectLoader.LoadedIndices.size();
    this->indices = new unsigned int[this->indexCount];

    for (size_t i = 0; i < objectLoader.LoadedIndices.size(); i++)
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

  bool loadTexture(std::string textureFilePath)
  {
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

  void render(ShaderProgram &shader)
  {
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.5f, 1.0f, 0.0f));

    shader.setMat4("model", model);

    shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glBindVertexArray(this->VAO);
    glDrawElements(GL_TRIANGLES, this->indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
};

class Camera
{
private:
  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 up;
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;

public:
  Camera(glm::vec3 position, glm::vec3 front, glm::vec3 up, float fov, float aspect, float near, float far) : position(position), front{front}, up{up}
  {
    viewMatrix = glm::lookAt(position, position + front, up);
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, near, far);
  }
  glm::mat4 getViewMatrix() const
  {
    return viewMatrix;
  }
  glm::mat4 getProjectionMatrix() const
  {
    return projectionMatrix;
  }
};

class Scene
{
private:
  std::vector<std::unique_ptr<Model>> models;

public:
  void addModel(std::unique_ptr<Model> model)
  {
    models.push_back(std::move(model));
  }

  void render(ShaderProgram &shader, const Camera &camera)
  {
    // Set view/projection matrices (same for all models)
    shader.use();
    shader.setMat4("view", camera.getViewMatrix());
    shader.setMat4("projection", camera.getProjectionMatrix());

    for (auto &model : models)
      model->render(shader);
  }
};

int drawImGui()
{
  return 0;
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("Window");
  ImGui::Text("Hello, World!");
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  return 0;
}

int main()
{

  // for (uint64_t n1 = 1, n2 = 1, r = 0;; n1 = n2, n2 = r) std::cout << (r = n1 + n2) << std::endl;

  glfwInit();
  const char *glsl_version = "#version 330 core";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  printf("Hello, TestEngine!\n");

  GLFWwindow *window = glfwCreateWindow(800, 600, "TestEngine", NULL, NULL);
  if (window == NULL)
  {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
  {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  glViewport(0, 0, 800, 600);

  objl::Loader objectLoader;

  bool success = objectLoader.LoadFile("resources/models/Headz.obj");

  int vertexCount = objectLoader.LoadedVertices.size();

  Vertex *vertices = new Vertex[vertexCount];

  for (int i = 0; i < vertexCount; i++)
  {
    objl::Vertex v = objectLoader.LoadedVertices[i];
    Vertex vertex = Vertex(v.Position.X, v.Position.Y, v.Position.Z, v.Normal.X, v.Normal.Y, v.Normal.Z, v.TextureCoordinate.X, v.TextureCoordinate.Y);
    vertices[i] = vertex;
  }

  unsigned int indexCount = objectLoader.LoadedIndices.size();
  unsigned int *indices = new unsigned int[indexCount];

  for (size_t i = 0; i < objectLoader.LoadedIndices.size(); i++)
  {
    indices[i] = objectLoader.LoadedIndices[i];
  }

  stbi_set_flip_vertically_on_load(true);
  int width, height, nrChannels;
  unsigned char *data = stbi_load("resources/textures/Terminatrix_Head.png", &width, &height, &nrChannels, 0);
  if (!data)
  {
    std::cerr << "Failed to load texture" << std::endl;
    return -1;
  }

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  stbi_image_free(data);

  Shader vertexShader("VertexShader.glsl", GL_VERTEX_SHADER);
  Shader fragmentShader("FragmentShader.glsl", GL_FRAGMENT_SHADER);

  ShaderProgram shaderProgram(&vertexShader, &fragmentShader);

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);
  glBindVertexArray(VAO);

  unsigned int VBO;
  glGenBuffers(1, &VBO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

  unsigned int EBO;
  glGenBuffers(1, &EBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(int), indices, GL_STATIC_DRAW);

  int vertexStride = sizeof(Vertex);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride, (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);

  glEnable(GL_DEPTH_TEST);
  // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  glm::vec3 cameraDirection = glm::normalize(cameraPos - cameraTarget);

  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSetScrollCallback(window, scrollCallback);
  startMouseCapture(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // glUseProgram(shaderProgram.Id);
  shaderProgram.use();

  Scene scene;
  std::unique_ptr<Model> model = std::make_unique<Model>("resources/models/Headz.obj", "resources/textures/Terminatrix_Head.png");
  scene.addModel(std::move(model));
  // int modelLoc = glGetUniformLocation(shaderProgram.Id, "model");
  // int viewLoc = glGetUniformLocation(shaderProgram.Id, "view");
  // int projectionLoc = glGetUniformLocation(shaderProgram.Id, "projection");
  // int texLoc = glGetUniformLocation(shaderProgram.Id, "ourTexture");
  // glUniform1i(texLoc, 0);

  // glEnable(GL_CULL_FACE);
  // glCullFace(GL_FRONT);

  Camera camera(cameraPos, cameraFront, cameraUp, fov, (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glm::mat4 model = glm::mat4(1.0f);

    // model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.5f, 1.0f, 0.0f));

    // glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // glm::mat4 projection = glm::perspective(glm::radians(fov), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    // glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    // glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    // glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // glBindTexture(GL_TEXTURE_2D, texture);
    // glBindVertexArray(VAO);

    // glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    // glBindVertexArray(0);

    scene.render(shaderProgram, Camera(cameraPos, cameraFront, cameraUp, fov, (float)windowWidth / (float)windowHeight, 0.1f, 100.0f));

    drawImGui();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}