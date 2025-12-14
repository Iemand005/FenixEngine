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

int loadShaderFile(char **shaderText, const char *fileName)
{

  std::ifstream file(fileName);

  if (!file.is_open())
  {
    std::cerr << "Failed to open file." << std::endl;
    return -1;
  }

  std::string line;
  std::string vertexShaderSource;
  while (std::getline(file, line))
  {
    vertexShaderSource += line + "\n";
  }

  const char *shaderCode = vertexShaderSource.c_str();
  *shaderText = new char[vertexShaderSource.length() + 1];
  std::strcpy(*shaderText, shaderCode);

  file.close();

  return 0;
}

int drawImGui()
{
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
  glfwInit();
  const char *glsl_version = "#version 330 core";
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

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

  // float vertices[] = {

  //     0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,   // top right
  //     0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f,  // bottom right
  //     -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, // bottom left
  //     -0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f,  // top left

  //     0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f,   // top right
  //     0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // bottom right
  //     -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // bottom left
  //     -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f   // top left
  // };

  // Vertex vertices[] = {
  //     // Front face
  //     Vertex(0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),   // top right
  //     Vertex(0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f),  // bottom right
  //     Vertex(-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f), // bottom left
  //     Vertex(-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f),  // top left

  //     // Back face
  //     Vertex(0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f),   // top right
  //     Vertex(0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),  // bottom right
  //     Vertex(-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f), // bottom left
  //     Vertex(-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f)   // top left
  // };

  objl::Loader objectLoader;

  bool success = objectLoader.LoadFile("resources/models/Headz.obj");

  int vertexCount = objectLoader.LoadedVertices.size();

  Vertex *vertices = new Vertex[vertexCount];

  for (int i = 0; i < vertexCount; i++)
  {
    objl::Vertex v = objectLoader.LoadedVertices[i];
    Vertex vertex = Vertex(v.Position.X, v.Position.Y, v.Position.Z,
                           v.Normal.X, v.Normal.Y, v.Normal.Z,
                           v.TextureCoordinate.X, v.TextureCoordinate.Y);
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
  if (!data) {
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

  char *vertexShaderSource;
  loadShaderFile(&vertexShaderSource, "VertexShader.glsl");

  // Create and compile vertex shader
  unsigned int vertexShader;
  vertexShader = glCreateShader(GL_VERTEX_SHADER);

  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  // Compile fragment shader
  char *fragmentShaderSource;
  loadShaderFile(&fragmentShaderSource, "FragmentShader.glsl");

  unsigned int fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);

  // Create shader program
  unsigned int shaderProgram;
  shaderProgram = glCreateProgram();

  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  unsigned int VBO;
  glGenBuffers(1, &VBO);

  unsigned int VAO;
  glGenVertexArrays(1, &VAO);

  unsigned int EBO;
  glGenBuffers(1, &EBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(int), indices, GL_STATIC_DRAW);

  int vertexStride = sizeof(Vertex);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, vertexStride, (void *)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

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

  glUseProgram(shaderProgram);
  int modelLoc = glGetUniformLocation(shaderProgram, "model");
  int viewLoc = glGetUniformLocation(shaderProgram, "view");
  int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
  int texLoc = glGetUniformLocation(shaderProgram, "ourTexture");
  glUniform1i(texLoc, 0);

  glEnable(GL_CULL_FACE);
  // glCullFace(GL_FRONT);

  while (!glfwWindowShouldClose(window))
  {
    processInput(window);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 model = glm::mat4(1.0f);

    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.5f, 1.0f, 0.0f));

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(VAO);

    // glBindVertexArray(VAO);
    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    drawImGui();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}