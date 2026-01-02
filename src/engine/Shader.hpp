#pragma once
#include <glad/glad.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
namespace fe {

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
      std::filesystem::path cwd = std::filesystem::current_path();
      std::cerr << "Failed to open file: " << fileName << " In: " << cwd << std::endl;
      return false;
    }

    shaderText.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    file.close();

    return true;
  }

  void deleteShader() { glDeleteShader(this->Id); }

  void attachToProgram(unsigned int programId) { glAttachShader(programId, this->Id); }
};
}