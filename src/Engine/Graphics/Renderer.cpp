#include <glad/glad.h>

#include "Renderer.hpp"

using namespace fe;

void Renderer::EnableWireframe() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}
void Renderer::DisableWireframe() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::ToggleWireframe(bool enabled) {
  if (enabled) EnableWireframe();
  else DisableWireframe();
}

Renderer::Renderer(GLADloadproc loadProc) {
  if (!gladLoadGLLoader(loadProc)) {
    std::cerr << "Failed to load OpenGL functions (GLAD)";
  }
  this->SetClearColor(0.1f, 0.4f, 1.0f);
}

void Renderer::BindFrameBuffer(int bufferIndex) {
  glBindFramebuffer(GL_FRAMEBUFFER, bufferIndex);
}
