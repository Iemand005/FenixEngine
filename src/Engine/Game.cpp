#include "Game.hpp"

using namespace fe;


void Game::Init() {
	SetClearColor(0.0F, 0.0F, 0.0f);

#ifndef EXCLUDE_JOLT
	this->physicsEngine = std::make_unique<PhysicsFactory>();
#endif
	
	LoadShaders("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");
	
	this->scene = std::make_unique<fe::Scene>();
	this->camera = std::make_unique<fe::Camera>(60.0f, 0.1f, 100.0f);
	
	this->scene->SetLight();
	
	// Renderer::Init();
	InitUI();
}

void Game::UpdatePhysics(double deltaTime) {
#ifndef EXCLUDE_JOLT
	if (physicsEngine) physicsEngine->Update(deltaTime);
#endif
}