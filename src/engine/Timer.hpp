
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

namespace fe {


struct Timer {
  std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

 public:
  double lastTime = 0.0;
  double deltaTime = 0.0;

  Timer() { this->reset(); }

  double update() {
    double currentTime = getTime();
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
    return deltaTime;
  }

  void reset() { startTime = std::chrono::high_resolution_clock::now(); }

  double getTime() {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = now - startTime;
    return elapsed.count();
  }
};

}