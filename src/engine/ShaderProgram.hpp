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
 public:
  unsigned int Id;

  int modelLoc;
  int viewLoc;
  int projectionLoc;
  int texLoc;

  ShaderProgram(Shader vertexShader, Shader fragmentShader) {
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
  ShaderProgram(std::string vertexShaderFile, std::string fragmentShaderFile) : ShaderProgram(Shader(vertexShaderFile, GL_VERTEX_SHADER), Shader(fragmentShaderFile, GL_FRAGMENT_SHADER)) {}

  void Use() { glUseProgram(this->Id); }

  void SetMat4(const std::string& name, const glm::mat4& mat) const { glUniformMatrix4fv(glGetUniformLocation(this->Id, name.c_str()), 1, GL_FALSE, &mat[0][0]); }
};
}

