#include "Foxcraft.hpp"

using namespace std;
using namespace glm;

int main() {

  auto game = make_unique<Foxcraft>(800, 600);
  
  auto cameraOffset = vec3(0.0f, 6.5f, 0.0f);

  while (!game->ShouldClose()) {
    glfwPollEvents();

    if (game->player->touchedGround) {
      game->canJump = true;
    }
    game->ProcessInput();
    game->player->rotation.y = -game->yaw + 90.0f;
    glm::vec3 pos = game->player->position + cameraOffset;
    game->cameraPos = pos - game->cameraFront * 5.0f;
    game->camera->SetPos(game->cameraPos);

    // game.playerCamera->setAspect((float)game.width / (float)game.height);
    // window.playerCamera->setPos(cameraPos);

    game->camera->setFront(glm::normalize(pos - game->cameraPos));

    if (game->isConnectedToServer) game->client->sendPosition(game->player->position, game->player->rotation);

    for (auto& npc : game->npcs) {
      npc->LookAt(pos * glm::vec3(1.0f, 0.0f, 1.0f));
      npc->applyVelocity(glm::normalize(pos - npc->position) * glm::vec3(1.0f, 0.0f, 1.0f) * 0.2f * (float)game->getDeltaTime());
      npc->needsUpdate = true;
    }
    for (auto& npc : game->npcs) {
      if (game->player->intersects(*npc)) {
        std::cout << "Player intersects with NPC" << std::endl;
        std::cout << "YOU FUCKING DIED!!!!" << std::endl;
        // client.sendPing();
      }
    }
    game->update();
    game->redraw();
  }

  game->destroy();
  return 0;
}