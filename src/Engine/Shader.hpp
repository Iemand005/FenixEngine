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
  unsigned int id;

  std::string shaderText;

  Shader(GLenum shaderType) {
    id = glCreateShader(shaderType);
  }

  Shader(std::string fileName, GLenum shaderType) : Shader(shaderType) {
    if (!loadShaderFile(fileName)) return;

  }

  bool ErrorCheck() {
    GLint success, length;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

    if (!success || length > 1) {
      std::string log(length, '\0');
      glGetShaderInfoLog(id, length, NULL, log.data());
      std::cout << "SHADER COMPILE ERROR:\n" << log << std::endl;
      return false;
    }
    return true;
  }

  bool LoadText(std::string shaderText) {
    const GLchar* shaderString = shaderText.c_str();
    glShaderSource(id, 1, &shaderString, NULL);
    glCompileShader(id);
    return yuErrorCheck();
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

    LoadText(shaderText);

    return true;
  }

  void deleteShader() { glDeleteShader(this->id); }

  void attachToProgram(unsigned int programId) { glAttachShader(programId, this->id); }
};
}
