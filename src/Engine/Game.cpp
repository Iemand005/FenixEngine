#include <memory>

#include "Game.hpp"

using namespace fe;

struct Game::Impl {
#ifndef EXCLUDE_JOLT
	std::unique_ptr<PhysicsFactory> physicsEngine = nullptr;
#endif
};

Game::~Game() = default;

Game::Game() : Renderer() {}

Game::Game(GLADloadproc loadProc) : Renderer(loadProc) {};

Game::Game(int width, int height, bool skipInit, bool showWindow) : Renderer(width, height, skipInit, !showWindow) {
	Init();
}

PhysicsFactory *Game::GetPhysicsEngine() {
#ifndef EXCLUDE_JOLT

	return impl->physicsEngine.get();
#endif
}

void Game::Init() {
	impl = std::make_unique<Game::Impl>();

	SetClearColor(0.0F, 0.0F, 0.0f);

#ifndef EXCLUDE_JOLT
	impl->physicsEngine = std::make_unique<PhysicsFactory>();
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
	if (impl->physicsEngine) impl->physicsEngine->Update(deltaTime);
#endif
}