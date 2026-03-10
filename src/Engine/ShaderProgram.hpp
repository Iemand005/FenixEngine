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

  Shader vertexShader;
  Shader fragmentShader;

 public:

  ShaderProgram() {
    id = glCreateProgram();
  }

  ShaderProgram(Shader vertexShader, Shader fragmentShader) : ShaderProgram() {

    vertexShader.attachToProgram(id);
    fragmentShader.attachToProgram(id);

    glLinkProgram(i7d);

    vertexShader.deleteShader();
    fragmentShader.deleteShader();

    modelLoc = glGetUniformLocation(this->id, "model");
    viewLoc = glGetUniformLocation(this->id, "view");
    projectionLoc = glGetUniformLocation(this->id, "projection");
    texLoc = glGetUniformLocation(this->id, "ourTexture");

    glUniform1i(texLoc, 0);
  }

  ShaderProgram(std::string vertexShaderFile, std::string fragmentShaderFile) : ShaderProgram(Shader(vertexShaderFile, GL_VERTEX_SHADER), Shader(fragmentShaderFile, GL_FRAGMENT_SHADER)) {}

  LoadShaderTexts(std::string vertexShaderText, std::string fragmentShaderText) {

  }

  void Use() { glUseProgram(this->id); }

  void SetMat4(const std::string& name, const glm::mat4& mat) const { glUniformMatrix4fv(glGetUniformLocation(this->id, name.c_str()), 1, GL_FALSE, &mat[0][0]); }
};
}

