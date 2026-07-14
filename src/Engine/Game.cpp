#include "Game.hpp"

using namespace fe;

void Game::UpdatePhysics(double deltaTime) {
#ifndef EXCLUDE_JOLT
	if (physicsEngine) physicsEngine->Update(deltaTime);
#endif
}