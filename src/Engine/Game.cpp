#include <memory>

#include "Game.hpp"
#include "ModelLoader.hpp"

using namespace fe;

struct Game::Impl {
#ifndef EXCLUDE_JOLT
	std::unique_ptr<PhysicsFactory> PhysicsFactory = nullptr;
#endif
};

Game::~Game() = default;

Game::Game() : Renderer() {
	Init();
}

Game::Game(GLADloadproc loadProc) : Renderer(loadProc) {
	Init();
};

Game::Game(int width, int height, bool skipInit, bool showWindow) : Renderer(width, height, skipInit, !showWindow) {
	Init();
}

Game::Game(RendererOptions options) : Renderer(options) {
	Init();
}


PhysicsFactory *Game::GetPhysicsFactory() {
#ifndef EXCLUDE_JOLT
	return impl->PhysicsFactory.get();
#endif
}

void Game::Init() {
	impl = std::make_unique<Game::Impl>();

	// SetClearColor moved to after NewWindow() call

	#ifndef EXCLUDE_JOLT
	impl->PhysicsFactory = std::make_unique<PhysicsFactory>(renderDevice.get(), !this->useVulkan);
	#endif
	
	//LoadShaders("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");
	
	this->scene = std::make_unique<fe::Scene>();
	this->camera = std::make_unique<fe::Camera>(60.0f, 0.1f, 1000.0f);
	
	this->scene->SetLight();
	
	// Renderer::Init();
	InitUI();
}

void Game::LoadModel(const std::string& path) {
	auto obj = ModelLoader::LoadModel(path);
	if (obj) {
		scene->AddObject(obj);
	}
}

void Game::UpdatePhysics(double deltaTime) {
#ifndef EXCLUDE_JOLT
	if (impl->PhysicsFactory) impl->PhysicsFactory->Update(deltaTime);
#endif
}