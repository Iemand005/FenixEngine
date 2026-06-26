
#pragma once
#include <glad/glad.h>

#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <array>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.hpp"
#include "Timer.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"
#include "physics/PhysicsEngine.hpp"

namespace fe {

static constexpr int kMaxPointLights = 8;

struct PointLight {
  glm::vec3 position{0.0f};
  glm::vec3 color{1.0f};
  float intensity{1.0f};
  float radius{10.0f};
};

class Scene {
  private:
  std::vector<std::shared_ptr<Object>> objects;
  glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
  Timer timer;

  std::array<PointLight, kMaxPointLights> pointLights{};
  int lightCount = 1;

  glm::mat4 lastViewMatrix = glm::mat4(1.0f);
  glm::mat4 lastProjectionMatrix = glm::mat4(1.0f);
  bool hasCameraMatrices = false;

  GLuint gizmoProgram = 0;
  GLuint gizmoVAO = 0;
  GLuint gizmoVBO = 0;
  GLint gizmoModelLoc = -1;
  GLint gizmoViewLoc = -1;
  GLint gizmoProjectionLoc = -1;
  GLint gizmoColorLoc = -1;
  bool gizmoRendererReady = false;

  static GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
      GLint logLength = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
      std::string log(static_cast<size_t>(std::max(logLength, 1)), '\0');
      glGetShaderInfoLog(shader, logLength, nullptr, log.data());
      glDeleteShader(shader);
      throw std::runtime_error(log.empty() ? "Failed to compile gizmo shader" : log);
    }

    return shader;
  }

  static bool CheckProgramLink(GLuint program, std::string& logOut) {
    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
      GLint logLength = 0;
      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
      logOut.assign(static_cast<size_t>(std::max(logLength, 1)), '\0');
      glGetProgramInfoLog(program, logLength, nullptr, logOut.data());
      return false;
    }
    logOut.clear();
    return true;
  }

  void DrawGizmoLines(const std::vector<glm::vec3>& vertices, GLenum mode, const glm::vec3& color, float lineWidth = 2.0f) {
    if (vertices.empty() || !hasCameraMatrices) return;

    EnsureGizmoRenderer();

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);

    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLfloat previousLineWidth = 1.0f;
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);

    glDisable(GL_DEPTH_TEST);

    glUseProgram(gizmoProgram);
    const glm::mat4 model(1.0f);
    glUniformMatrix4fv(gizmoModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(gizmoViewLoc, 1, GL_FALSE, glm::value_ptr(lastViewMatrix));
    glUniformMatrix4fv(gizmoProjectionLoc, 1, GL_FALSE, glm::value_ptr(lastProjectionMatrix));
    glUniform3f(gizmoColorLoc, color.r, color.g, color.b);

    glBindVertexArray(gizmoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gizmoVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STREAM_DRAW);
    glLineWidth(lineWidth);
    glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size()));
    glLineWidth(previousLineWidth);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glUseProgram(static_cast<GLuint>(previousProgram));
    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
  }

  void EnsureGizmoRenderer() {
    if (gizmoRendererReady) return;

    static const char* vertexShaderSource = R"(#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
  gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    static const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;

uniform vec3 uColor;

void main() {
  FragColor = vec4(uColor, 1.0);
}
)";

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    gizmoProgram = glCreateProgram();
    glAttachShader(gizmoProgram, vertexShader);
    glAttachShader(gizmoProgram, fragmentShader);
    glLinkProgram(gizmoProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    std::string linkLog;
    if (!CheckProgramLink(gizmoProgram, linkLog)) {
      glDeleteProgram(gizmoProgram);
      gizmoProgram = 0;
      throw std::runtime_error(linkLog.empty() ? "Failed to link gizmo shader" : linkLog);
    }

    gizmoModelLoc = glGetUniformLocation(gizmoProgram, "model");
    gizmoViewLoc = glGetUniformLocation(gizmoProgram, "view");
    gizmoProjectionLoc = glGetUniformLocation(gizmoProgram, "projection");
    gizmoColorLoc = glGetUniformLocation(gizmoProgram, "uColor");

    glGenVertexArrays(1, &gizmoVAO);
    glGenBuffers(1, &gizmoVBO);

    glBindVertexArray(gizmoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gizmoVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), nullptr, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    gizmoRendererReady = true;
  }

 public:
  Scene() {
    objects = std::vector<std::shared_ptr<Object>>();
    this->EnableDepthTest();
    this->EnableFaceCulling();
  }

  ~Scene() {
    if (gizmoVBO) glDeleteBuffers(1, &gizmoVBO);
    if (gizmoVAO) glDeleteVertexArrays(1, &gizmoVAO);
    if (gizmoProgram) glDeleteProgram(gizmoProgram);
  }

  void Render(ShaderProgram shader, Camera const& camera, int width, int height) {
    this->Resize(width, height);
    this->Render(shader, camera);
  }
  
  void Render(ShaderProgram shader, Camera const& camera) {
    this->PrepareRender(shader, camera);

    for (auto& model : objects) {
      if (model->shader)
        model->Render(*(model->shader.get()));
      else model->Render(shader);
    }

    this->EndRender();
  }

  void EnableDepthTest() { glEnable(GL_DEPTH_TEST); }
  void DisableDepthTest() { glDisable(GL_DEPTH_TEST); }
  void EnableFaceCulling() { glEnable(GL_CULL_FACE); }
  void DisableFaceCulling() { glDisable(GL_CULL_FACE); }


  std::vector<std::shared_ptr<Object>>& GetObjects() { return objects; }

  std::vector<std::shared_ptr<Object>> GetFilteredObjects(std::shared_ptr<Object> exclude) const {
    std::vector<std::shared_ptr<Object>> filtered;
    std::copy_if(objects.begin(), objects.end(), std::back_inserter(filtered), [exclude](const std::shared_ptr<Object>& obj) {
      return obj != exclude;
    });
    return filtered;
}

  void ClearObjects() { objects.clear(); }

  void AddObject(std::shared_ptr<Object> object) { objects.push_back(object); }
  std::shared_ptr<Object> AddObject(Mesh mesh) {
    auto obj = std::make_shared<Object>(mesh);
    objects.push_back(obj);
    return obj;
  }

  bool RemoveObject(std::shared_ptr<Object> object) {
    auto it = std::find(objects.begin(), objects.end(), object);
    if (it != objects.end()) {
      objects.erase(it);
      return true;
    }
    return false;
  }

  void AddLight() {
    ++lightCount;
  }

  void RemoveLight() {
    --lightCount;
  }

  void SetLight(int index = 0) {
    pointLights[index].position = glm::vec3(3.0f, 3.0f, 3.0f);
    pointLights[index].color = glm::vec3(1.0f);
    pointLights[index].intensity = 1.0f;
    pointLights[index].radius = 10.0f;
  }

  int GetLightCount() {
    return lightCount;
  }

  PointLight *GetLights() {
    return pointLights.data();
  }

  std::array<PointLight, kMaxPointLights> GetLightArray() {
    return pointLights;
  }

  void PrepareRender(ShaderProgram shader, Camera const& camera) {
    this->Clear();
    shader.Use();
    shader.SetMat4("view", camera.GetViewMatrix());
    shader.SetMat4("projection", camera.GetProjectionMatrix());

    lastViewMatrix = camera.GetViewMatrix();
    lastProjectionMatrix = camera.GetProjectionMatrix();
    hasCameraMatrices = true;
  }

  void Clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }


  void EndRender() { glBindVertexArray(0); }

  double Update() {
    auto deltaTime = timer.update();
    for (auto& object : objects) {
      object->Update(deltaTime);
    }
    ResolveCollisions();
    return deltaTime;
  }

  void ResolveCollisions() {
    for (auto& object : objects) {
      if (object->state.position.y < -10.0f) {
        auto pos = object->state.position;
        pos.y = 10;
        if (!object->physicsObject) continue;
        object->physicsObject->SetPosition(pos);
        object->physicsObject->SetLinearVelocity(glm::vec3(0.0f));
      }
    }
  }

  double GetDeltaTime() { return timer.deltaTime; }

  void Resize(int width, int height) { glViewport(0, 0, width, height); }

  void DrawCircle(float radius, int segments) {
    DrawCircle(glm::vec3(0.0f), radius, segments, glm::vec3(0.0f), glm::vec3(0.95f, 0.80f, 0.15f));
  }

  void DrawCircle(const glm::vec3& position, float radius, int segments) {
    DrawCircle(position, radius, segments, glm::vec3(0.0f), glm::vec3(0.95f, 0.80f, 0.15f));
  }

  void DrawCircle(
      const glm::vec3& position,
      float radius,
      int segments,
      const glm::vec3& rotationDegrees,
      const glm::vec3& color) {
    if (radius <= 0.0f) return;
    segments = std::max(segments, 3);
    if (!hasCameraMatrices) return;

    EnsureGizmoRenderer();

    glm::mat4 rotation(1.0f);
    rotation = glm::rotate(rotation, glm::radians(rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotation;

    std::vector<glm::vec3> circleVertices;
    circleVertices.reserve(static_cast<size_t>(segments));

    constexpr float kTwoPi = 6.28318530717958647692f;
    for (int i = 0; i < segments; ++i) {
      float t = kTwoPi * (static_cast<float>(i) / static_cast<float>(segments));
      glm::vec3 localPoint(std::cos(t) * radius, 0.0f, std::sin(t) * radius);
      circleVertices.emplace_back(glm::vec3(transform * glm::vec4(localPoint, 1.0f)));
    }
    DrawGizmoLines(circleVertices, GL_LINE_LOOP, color);
  }

  void DrawArrow(
      const glm::vec3& from,
      const glm::vec3& to,
      const glm::vec3& color = glm::vec3(0.95f, 0.80f, 0.15f),
      float headLengthScale = 0.20f,
      float headRadiusScale = 0.12f) {
    if (!hasCameraMatrices) return;

    glm::vec3 direction = to - from;
    float length = glm::length(direction);
    if (length <= 0.0001f) return;

    glm::vec3 forward = direction / length;
    float headLength = std::clamp(length * headLengthScale, 0.0f, length * 0.95f);
    float headRadius = std::clamp(length * headRadiusScale, 0.0f, headLength * 0.75f);
    if (headRadius <= 0.0001f) {
      headRadius = length * 0.05f;
    }

    glm::vec3 shaftEnd = to - forward * headLength;

    glm::vec3 reference = std::abs(forward.y) < 0.999f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 right = glm::cross(forward, reference);
    if (glm::length(right) <= 0.0001f) {
      reference = glm::vec3(0.0f, 0.0f, 1.0f);
      right = glm::cross(forward, reference);
    }
    right = glm::normalize(right);
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    std::array<glm::vec3, 4> headRing = {
        shaftEnd + right * headRadius + up * headRadius,
        shaftEnd - right * headRadius + up * headRadius,
        shaftEnd - right * headRadius - up * headRadius,
        shaftEnd + right * headRadius - up * headRadius};

    std::vector<glm::vec3> arrowVertices;
    arrowVertices.reserve(18);
    arrowVertices.push_back(from);
    arrowVertices.push_back(shaftEnd);

    for (const auto& point : headRing) {
      arrowVertices.push_back(to);
      arrowVertices.push_back(point);
    }

    for (size_t i = 0; i < headRing.size(); ++i) {
      arrowVertices.push_back(headRing[i]);
      arrowVertices.push_back(headRing[(i + 1) % headRing.size()]);
    }

    DrawGizmoLines(arrowVertices, GL_LINES, color);
  }

  void DrawArrow(
      const glm::vec3& origin,
      const glm::vec3& direction,
      float length,
      const glm::vec3& color = glm::vec3(0.95f, 0.80f, 0.15f),
      float headLengthScale = 0.10f,
      float headRadiusScale = 0.5f) {
    if (length <= 0.0f) return;

    float directionLength = glm::length(direction);
    if (directionLength <= 0.0001f) return;

    DrawArrow(
        origin,
        origin + (direction / directionLength) * length,
        color,
        headLengthScale,
        headRadiusScale);
  }
};

} 
