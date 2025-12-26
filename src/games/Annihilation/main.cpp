#pragma once

// #include "Annhihilation.hpp"
#include "Annihilation.hpp"



int main() {

  Game game(800, 600);
  
  glm::vec3 cameraOffset = glm::vec3(0.0f, 6.5f, 0.0f);

  while (!game.shouldClose()) {
    glfwPollEvents();

    if (game.player->touchedGround) {
      game.canJump = true;
    }
    game.processInput();
    game.player->rotation.y = -yaw + 90.0f;
    glm::vec3 pos = game.player->position + cameraOffset;
    cameraPos = pos - cameraFront * 5.0f;
    game.camera->setPos(cameraPos);

    // game.playerCamera->setAspect((float)game.width / (float)game.height);
    // window.playerCamera->setPos(cameraPos);

    game.camera->setFront(glm::normalize(pos - cameraPos));

    if (game.isConnectedToServer) game.client->sendPosition(game.player->position, game.player->rotation);

    for (auto& npc : game.npcs) {
      npc->lookAt(pos * glm::vec3(1.0f, 0.0f, 1.0f));
      npc->applyVelocity(glm::normalize(pos - npc->position) * glm::vec3(1.0f, 0.0f, 1.0f) * 0.2f * (float)game.getDeltaTime());
      npc->needsUpdate = true;
    }
    for (auto& npc : game.npcs) {
      if (game.player->intersects(*npc)) {
        std::cout << "Player intersects with NPC" << std::endl;
        std::cout << "YOU FUCKING DIED!!!!" << std::endl;
        // client.sendPing();
      }
    }
    game.update();
    game.redraw();
  }

  game.destroy();
  return 0;
}