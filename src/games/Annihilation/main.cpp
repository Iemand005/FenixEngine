#include "Annihilation.hpp"
// #include "../../engine/physics/BasicDebugRenderer.hpp"

int main() {

  Annihilation game(800, 600);
  // game.DisableVSync();
  
  glm::vec3 cameraOffset = glm::vec3(0.0f, 6.5f, 0.0f);

  while (!game.ShouldClose()) {
//     std::cout << "DEBUG: Before glfwPollEvents()" << std::endl;
// std::cout << "DEBUG: Window pointer: " << game.window << std::endl;
    glfwPollEvents();

    if (game.player->touchedGround) {
      game.canJump = true;
    }
    game.ProcessInput();
    game.player->state.rotation.y = -game.yaw + 90.0f;
    glm::vec3 pos = game.player->state.position + cameraOffset;
    // game.cameraPos = pos - game.cameraFront * 5.0f;
    game.camera->SetPos(pos - game.camera->front * 5.0f);

    game.camera->setFront(glm::normalize(pos - game.camera->GetPos()));

    if (game.isConnectedToServer) game.client->sendPosition(game.player->state.position, game.player->state.rotation);

    for (auto& npc : game.npcs) {
      npc->LookAt(pos * glm::vec3(1.0f, 0.0f, 1.0f));/*
      npc->ApplyVelocity(glm::normalize(pos - npc->state.position) * glm::vec3(1.0f, 0.0f, 1.0f) * 0.2f * (float)game.getDeltaTime());
      npc->needsUpdate = true;*/
    }
    for (auto& npc : game.npcs) {
      // if (game.player->intersects(*npc)) {
      //   std::cout << "Player intersects with NPC" << std::endl;
      //   std::cout << "YOU FUCKING DIED!!!!" << std::endl;
      //   // client.sendPing();
      // }
    }
    game.Update();
    game.Redraw();


  }

  game.Destroy();
  return 0;
}