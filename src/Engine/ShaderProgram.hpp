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

#include "Shader.hpp"

namespace fe {


class ShaderProgram {
  unsigned int id;

  int modelLoc;
  int viewLoc;
  int projectionLoc;
  int texLoc;

  Shader vertexShader = Shader(GL_VERTEX_SHADER);
  Shader fragmentShader = Shader(GL_FRAGMENT_SHADER);

 public:

  ShaderProgram() {
    id = glCreateProgram();
  }

  ShaderProgram(Shader vertexShader, Shader fragmentShader) : ShaderProgram() {

    this->vertexShader = vertexShader;
    this->fragmentShader = fragmentShader;

    AttachShaders();
    LinkShaders();
  }

  ShaderProgram(std::string vertexShaderFile, std::string fragmentShaderFile)
      : ShaderProgram(Shader(vertexShaderFile, GL_VERTEX_SHADER), Shader(fragmentShaderFile, GL_FRAGMENT_SHADER)) {}

  bool LoadShaderTexts(std::string vertexShaderText, std::string fragmentShaderText) {
    if (!vertexShader.LoadText(vertexShaderText)) return false; // TODO: Use error throw insteadto pass message;
    if (!fragmentShader.LoadText(fragmentShaderText)) return false;
    AttachShaders();
    LinkShaders();
    return ErrorCheck();
  }

  void AttachShaders() {
    vertexShader.attachToProgram(id);
    fragmentShader.attachToProgram(id);
  }

  void LinkShaders() {
    glLinkProgram(id);

    modelLoc = glGetUniformLocation(id, "model");
    viewLoc = glGetUniformLocation(id, "view");
    projectionLoc = glGetUniformLocation(id, "projection");
    texLoc = glGetUniformLocation(id, "ourTexture");

    glUniform1i(texLoc, 0);
  }

  bool ErrorCheck() {
    GLint ok = 0, length = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &ok);
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
    if (!ok || length > 1) {
        std::string log(length, '\0');
        glGetProgramInfoLog(id, length, nullptr, log.data());
        fprintf(stderr, "Program link log:\n%s\n", log.c_str());
        throw std::runtime_error(log);
        return false;
    }
    return true;
  }

  void Use() { glUseProgram(id); }

  void SetMat4(const std::string& name, const glm::mat4& mat) const { glUniformMatrix4fv(glGetUniformLocation(this->id, name.c_str()), 1, GL_FALSE, &mat[0][0]); }
  void SetVec3(const std::string& name, const glm::vec3& vec) const { glUniform3f(glGetUniformLocation(this->id, name.c_str()), vec.x, vec.y, vec.z); }
};
}

