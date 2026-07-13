#include <glad/glad.h>

#include "Game.hpp"

using namespace fe;

void fe::Game::ToggleWireframe(bool enabled) {
	if (enabled) EnableWireframe();
	else DisableWireframe();
}

void Game::BindFrameBuffer(int bufferIndex) {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferIndex);
}

void Game::CheckErrors() {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
		std::cerr << "OpenGL error: " << err << std::endl;
}
