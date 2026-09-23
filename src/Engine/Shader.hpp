
#pragma once

#if defined(__ANDROID__)
#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>
#elif defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <filesystem>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#if defined(__ANDROID__) && __has_include(<android/log.h>)
#include <android/log.h>
#endif

#include "WawaDir.hpp"
#include "Log.hpp"

namespace fe {


  enum ShaderType : GLenum {
    VertexShaderType = GL_VERTEX_SHADER,
    FragmentShaderType = GL_FRAGMENT_SHADER
  };

  class Shader {
  public:
    unsigned int id;

    std::string shaderText;

    Shader(GLenum shaderType) {
      id = glCreateShader(shaderType);
    }

    Shader(std::string text, GLenum shaderType) : Shader(text, static_cast<ShaderType>(shaderType)) {}

    Shader(std::string text, ShaderType shaderType) : Shader(shaderType) {
      if (!LoadShaderFile(text))
        LoadText(text);
    }

    static Shader Vertex(std::string text) { return Shader(text, ShaderType::VertexShaderType); }
    static Shader Fragment(std::string text) { return Shader(text, ShaderType::FragmentShaderType); }

  bool ErrorCheck() {
    GLint success, length;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);

    if (!success) {
      std::string log(length, '\0');
      glGetShaderInfoLog(id, length, NULL, log.data());
      std::cout << "SHADER COMPILE ERROR:\n" << log << std::endl;
      //throw std::runtime_error(log);
      std::cerr << log;
      return false;
    }
    return true;
  }

  bool LoadText(std::string shaderText) {
    const GLchar* shaderString = shaderText.c_str();
    glShaderSource(id, 1, &shaderString, NULL);
    glCompileShader(id);
    return ErrorCheck();
  }

	bool LoadShaderFile(std::string fileName) {
		std::ifstream file(fileName.c_str());

		if (!file.is_open()) {
			std::filesystem::path cwd = std::filesystem::current_path();
			std::cerr << "Failed to open file: " << fileName << " In: " << cwd << std::endl;
			fe::LogWarning("Shader LoadShaderFile failed to open: %s (cwd=%s)", fileName.c_str(), cwd.string().c_str());

			std::string exeDir = GetExecutableDirectory();
			std::string path2 = exeDir + "/" + fileName;// TODO: this slash thing not corss latfromr!
			file.open(path2);
			if (!file.is_open())
			{
              std::cout << "NO LOADY OH NO " << path2 << std::endl;
        return false;
				// return true;
			}
      std::cout << "Loaded from exe dir: " << path2 << std::endl;

			// return false;
		}

		shaderText.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		file.close();

		return LoadText(shaderText);
	}

  void deleteShader() { glDeleteShader(this->id); }

  void attachToProgram(unsigned int programId) { glAttachShader(programId, this->id); }
};
}
