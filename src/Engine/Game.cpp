#include <glad/glad.h>

#include "Game.hpp"

using namespace fe;

void Game::InitGL() {
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
  this->InitGL();
  this->SetClearColor(0.1f, 0.4f, 1.0f);
  Init();
}

void Game::BindFrameBuffer(int bufferIndex) {
  glBindFramebuffer(GL_FRAMEBUFFER, bufferIndex);
}
