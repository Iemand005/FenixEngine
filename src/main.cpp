#pragma once
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

#include "engine.h"

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

class Window
{
public:
  int width;
  int height;
  GLFWwindow *window;
  std::unique_ptr<fe::Scene> scene;
  std::unique_ptr<fe::Camera> playerCamera;
  fe::ShaderProgram *shader;
  fe::FPSCounter fpsCounter;

  std::shared_ptr<fe::Character> player;

  std::vector<std::shared_ptr<fe::Character>> npcs;

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

    this->scene = std::make_unique<fe::Scene>();
    this->shader = new fe::ShaderProgram("VertexShader.glsl", "FragmentShader.glsl");
    this->playerCamera = std::make_unique<fe::Camera>(cameraPos, cameraFront, cameraUp, fov, (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

    startMouseCapture();

    this->npcs = std::vector<std::shared_ptr<fe::Character>>();

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
    this->player = std::static_pointer_cast<fe::Character>(loadOBJ("resources/models/citizen.obj", 0.1f));

    auto obj2 = std::static_pointer_cast<fe::Character>(loadOBJ("resources/models/citizen.obj", 0.1f));
    obj2->position = glm::vec3(5.0f, 0.0f, 0.0f);

    obj2->meshes[0].loadTexture("resources/textures/chau_zombfacemap.png");
    obj2->meshes[1].loadTexture("resources/textures/citizenzomb_sheet_reference.png");

    obj2->lookAt(this->player->position);

    npcs.push_back(obj2);
    spawnZombies(10);
  }

  void spawnZombies(int count = 10)
  {
    const float minDistance = 20.0f;
    const float minDistanceSq = minDistance * minDistance;

    auto zombieTemplate = std::static_pointer_cast<fe::Character>(loadOBJButDontAdd("resources/models/citizen.obj", 0.1f));
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

      auto npc = std::static_pointer_cast<fe::Character>(zombieTemplate->clone());
      npc->position = glm::vec3(x, 0.0f, z);

      npc->meshes[0].loadTexture("resources/textures/chau_zombfacemap.png");
      npc->meshes[1].loadTexture("resources/textures/citizenzomb_sheet_reference.png");

      this->scene->addModel(npc);

      npcs.push_back(npc);
    }
  }

  std::shared_ptr<fe::Object> loadOBJ(std::string path, float scale = 1.0f)
  {
    std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>(path, scale);
    this->scene->addModel(model);
    return model;
  }

  std::shared_ptr<fe::Object> loadOBJButDontAdd(std::string path, float scale = 1.0f)
  {
    return std::make_shared<fe::Object>(path, scale);
  }

  bool loadStaticOBJ(std::string path, float scale = 1.0f)
  {
    std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>(path, scale);
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

  

  void redraw()
  {
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
      std::shared_ptr<fe::Object> newObj = this->player->clone();
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
    fe::Object *model = this->player.get();
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