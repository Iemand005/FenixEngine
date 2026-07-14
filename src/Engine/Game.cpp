#include "Game.hpp"

using namespace fe;

void Game::UpdatePhysics(double deltaTime) {
	if (physicsEngine) physicsEngine->Update(deltaTime);
}