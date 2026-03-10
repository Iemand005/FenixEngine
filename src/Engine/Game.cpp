#include <glad/glad.h>

#include "Game.hpp"

using namespace fe;

void Game::GLInit() {
  // glEnable(GL_DEPTH_TEST);
  // glEnable(GL_CULL_FACE);
  glEnable(GL_MULTISAMPLE);
}

void Game::SetClearColor(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
}

void Game::EnableWireframeMode() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}
void Game::DisableWireframeMode() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

Game::Game(GLADloadproc loadProc) {
  if (!gladLoadGLLoader(loadProc)) {
      std::cerr << "Failed to load OpenGL functions (GLAD)";
  }
}