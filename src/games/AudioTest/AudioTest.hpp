#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "../../engine/EditableGame.hpp"

class AudioTest : public fe::EditableGame {
public:

	std::vector<std::string> messages;

	double lastUpdateTime = 0.0f;

	bool canJump = true;

	int mapIndex = 0;

	ImGuiIO io;

	AudioTest() : AudioTest(800, 640) {}

	AudioTest(int width, int height, bool vr = false) : fe::EditableGame(width, height, vr) {
		LoadModels();
	}

	void LoadModels() {
		auto map1 = LoadStaticOBJ("resources/models/collisiontest.obj");
		this->scene->AddObject(map1);
		this->maps.push_back(map1);

		this->maps.push_back(LoadStaticOBJ("resources/testmap/testmappy.obj", 5.0f));

		loadMap(0);

		this->player = std::make_shared<fe::Character>();
		this->scene->AddObject(player);

		this->player->SetPhysicsObject(physicsEngine->CreateObject(glm::vec3(1.0f, 1.0f, 1.0f)));
	}

	void ProcessInput() {
		SDL_Event event;
		fe::SDLWindow *window = (fe::SDLWindow*)this->window.get();
		while (window->PollSDLEvents(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			auto io = ImGui::GetIO();
			switch (event.type) {
				case SDL_EVENT_QUIT:
				// window->PrepareClose();
				break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (event.button.button == SDL_BUTTON_LEFT && !io.WantCaptureMouse) {
					window->StartMouseCapture();
				}
				break;
				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				// Get actual pixel dimensions
				break;
				case SDL_EVENT_MOUSE_MOTION:
				{
					float sensitivity = 0.1f;

					yaw   += event.motion.xrel * sensitivity;
					pitch -= event.motion.yrel * sensitivity;

					pitch = std::clamp(pitch, -89.0f, 89.0f);
					break;
				}
			}
		}

		if (window->IsKeyDown(SDL_SCANCODE_W)) this->player->Move(fe::Direction::Forwards, camera.get());
		if (window->IsKeyDown(SDL_SCANCODE_A)) this->player->Move(fe::Direction::Left, camera.get());
		if (window->IsKeyDown(SDL_SCANCODE_S)) this->player->Move(fe::Direction::Backwards, camera.get());
		if (window->IsKeyDown(SDL_SCANCODE_D)) this->player->Move(fe::Direction::Right, camera.get());

		if (window->IsKeyDown(SDL_SCANCODE_SPACE)) this->player->Move(fe::Direction::Up, camera.get());
		if (window->IsKeyDown(SDL_SCANCODE_LSHIFT)) this->player->Move(fe::Direction::Down, camera.get());

		if (window->IsKeyDown(SDL_SCANCODE_ESCAPE)) window->StopMouseCapture();
		if (ImGui::GetIO().WantCaptureMouse) window->StopMouseCapture();
	}


  void Run() {
    auto window = this->GetWindow<fe::SDLWindow>();
    window->DisableVSync();
  
    glm::vec3 cameraOffset = glm::vec3(0);

    SDL_Event event;
    while (!window->ShouldClose()) {

      if (player->touchedGround) {
        canJump = true;
      }

      ProcessInput();
      player->state.rotation.y = -yaw + 90.0f;
      glm::vec3 pos = player->state.position + cameraOffset;
      camera->SetPos(pos - camera->front * 6.0f);
      
      camera->setFront(glm::normalize(pos - camera->GetPos()));
      
      if (isConnectedToServer) client->sendPosition(player->state.position, player->state.rotation);
      
      Update();
      Redraw();
    }

    Destroy();
  }

  void InitUI() override {
  }

  void DrawUI() override {
    DrawDebugUI();
  }
};
