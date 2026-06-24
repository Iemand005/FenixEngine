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

void Game::BindFrameBuffer(int bufferIndex) {
  glBindFramebuffer(GL_FRAMEBUFFER, bufferIndex);
}

void Game::Redraw() {
	if (!scene || !camera || !shader) return;

	if (shader) {
		shader->Use();
		int count = scene->GetLightCount();
		auto pointLights = scene->GetLights();
		shader->SetInt("lightCount", count);
		for (int i = 0; i < count; ++i) {
			const auto& l = pointLights[i];
			shader->SetVec3("pointLights[" + std::to_string(i) + "].position", l.position);
			shader->SetVec3("pointLights[" + std::to_string(i) + "].color", l.color);
			shader->SetFloat("pointLights[" + std::to_string(i) + "].intensity", l.intensity);
			shader->SetFloat("pointLights[" + std::to_string(i) + "].radius", std::max(0.001f, l.radius));
		}
	}

	scene->Render(*this->shader, *this->camera.get());

	OnDraw();

	CheckErrors();

	glFlush();
	glFinish();

	fpsCounter.update();

	DrawUI();

	if (window) window->SwapBuffers();
}

void Game::CheckErrors() {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << "OpenGL error: " << err << std::endl;
	}
}