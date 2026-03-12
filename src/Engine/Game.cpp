#include <glad/glad.h>

#include "Game.hpp"

using namespace fe;

void Game::InitGL() {
  glEnable(GL_MULTISAMPLE);
}

void Game::SetClearColor(float r, float g, float b, float a) {
  glClearColor(r, g, b, a);
}

void Game::EnableWireframe() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}
void Game::DisableWireframe() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void fe::Game::ToggleWireframe(bool enabled) {
  if (enabled) EnableWireframe();
  else DisableWireframe();
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
