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

#include <EditableGame.hpp>
#include <Primitives.hpp>

#include <audio/AudioVisualiser.hpp>

class Cake : public fe::EditableGame {
public:

	double lastUpdateTime = 0.0f;

	bool canJump = true;

	int mapIndex = 0;

	std::vector<std::shared_ptr<fe::Object>> rectangles;

	AudioVisualiser visualizer;

	bool showCandle = true;

	std::shared_ptr<fe::Object> wick;
	std::shared_ptr<fe::Object> flameParticle;

	Cake() : Cake(1400, 1200) {}

	Cake(int width, int height, bool vr = false) : fe::EditableGame(width, height, vr) {

		SetClearColor(1, 1, 0);

		LoadShaders("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");

		LoadModels();

		visualizer.Init();
	}

	static bool SDLCALL LiveRedrawWatcher(void* userdata, SDL_Event* event) {
		Cake* game = (Cake*)userdata;
		if (event->type == SDL_EVENT_WINDOW_EXPOSED)
			game->Redraw();
		return true;
	}

	void LoadModels() {

		auto barShader = std::make_shared<fe::ShaderProgram>("resources/shaders/debug.vert", "resources/shaders/debug.frag");;

		for (int i = 0; i < NUM_BARS; ++i) {
			auto cube = std::make_shared<fe::Object>(fe::Primitives::GenerateCube(1.0f));
			
			cube->name = "Bar_" + i;
			cube->state.position = glm::vec3(-15.0f + i * 1.0f, 0.0f, -25.0f);
			if (false)
				cube->shader = barShader;

			// cube->meshes[0].sha
	
			this->scene->AddObject(cube);

			rectangles.push_back(cube);
		}

		this->player = std::make_shared<fe::Character>();
		this->scene->AddObject(player);

		// Cake mesh

		fe::UVRect cakeTopBtmUV;
		cakeTopBtmUV.u0 = 1.0f / 16.0f;
		cakeTopBtmUV.u1 = 15.0f / 16.0f;
		cakeTopBtmUV.v0 = 1.0f / 16.0f;
		cakeTopBtmUV.v1 = 15.0f / 16.0f;

		fe::UVRect cakeSideUV;
		cakeSideUV.u0 = 1.0f / 16.0f;
		cakeSideUV.u1 = 15.0f / 16.0f;
		cakeSideUV.v0 = 0.0f / 16.0f;
		cakeSideUV.v1 = 8.0f / 16.0f;

		fe::CubeUVs cakeUVs;


		cakeUVs.top = cakeUVs.bottom = cakeTopBtmUV;
		cakeUVs.front = cakeUVs.back = cakeUVs.left = cakeUVs.right = cakeSideUV;

		auto planeMesh = fe::Primitives::GenerateCube({fe::PlaneDirection::Top}, cakeUVs);
		planeMesh.loadTexture("resources/textures/cake_top.png", fe::TextureScaling::Nearest);

		auto sideMesh = fe::Primitives::GenerateCube({fe::PlaneDirection::Front, fe::PlaneDirection::Left, fe::PlaneDirection::Right, fe::PlaneDirection::Back}, cakeUVs);
		sideMesh.loadTexture("resources/textures/cake_side.png", fe::TextureScaling::Nearest);

		auto bottomMesh = fe::Primitives::GenerateCube({fe::PlaneDirection::Bottom}, cakeUVs);
		bottomMesh.loadTexture("resources/textures/cake_bottom.png", fe::TextureScaling::Nearest);
		bottomMesh.hasTransparency = true;

		auto CAKEObject = std::make_shared<fe::Object>(planeMesh);
		CAKEObject->meshes.push_back(sideMesh);
		CAKEObject->meshes.push_back(bottomMesh);

		CAKEObject->name = "Cake";
		CAKEObject->state.position.y = 0.25f;
		CAKEObject->state.scale.x = CAKEObject->state.scale.z = 14.0f / 16.0f;
		CAKEObject->state.scale.y = 0.5f;
		this->scene->AddObject(CAKEObject);

		// Candle mesh

		fe::CubeUVs candleUVs;

		fe::UVRect topUV;
		topUV.u0 = 0;
		topUV.u1 = 2.0f / 16.0f;
		topUV.v0 = 6.0f / 16.0f;
		topUV.v1 = 8.0f / 16.0f;
		fe::UVRect sideUV;

		sideUV.u0 = 0;
		sideUV.u1 = 2.0f / 16.0f;
		sideUV.v0 = 0;
		sideUV.v1 = 8.0f / 16.0f;

		candleUVs.top = candleUVs.bottom = topUV;
		candleUVs.front = candleUVs.back = candleUVs.left = candleUVs.right = sideUV;

		auto candleMesh = fe::Primitives::GenerateCube(candleUVs);
		candleMesh.loadTexture("resources/textures/candle.png", fe::TextureScaling::Nearest);

		auto candle = std::make_shared<fe::Object>(candleMesh);

		candle->name = "Candle";
		candle->state.position.y = 0.75f;
		candle->state.scale.x = candle->state.scale.z = 2.0f / 16.0f;
		candle->state.scale.y = 8.0f / 16.0f;
		scene->AddObject(candle);

		// Candle wick

		fe::UVRect wickUV;
		wickUV.u0 = 0.0f / 16.0f;
		wickUV.u1 = 1.0f / 16.0f;
		wickUV.v0 = 10.0f / 16.0f;
		wickUV.v1 = 11.0f / 16.0f;

		auto wickMesh = fe::Primitives::GeneratePlane(fe::PlaneDirection::Front, 1.0f / 16.0f, 1.0f / 16.0f, wickUV);
		wickMesh.loadTexture("resources/textures/candle.png", fe::TextureScaling::Nearest);

		auto wickObject = std::make_shared<fe::Object>(wickMesh);
		candle->name = "Wick";
		wickObject->state.position.y = 1.03f;
		wick = wickObject;
		scene->AddObject(wickObject);

		// Flame particle

		fe::UVRect flameUV;
		flameUV.u0 = 0.0f / 128.0f;
		flameUV.u1 = 8.0f / 128.0f;
		flameUV.v0 = 96.0f / 128.0f;
		flameUV.v1 = 104.0f / 128.0f;

		auto flameMesh = fe::Primitives::GeneratePlane(fe::PlaneDirection::Front, 0.15f, 0.15f, flameUV);
		flameMesh.loadTexture("resources/textures/particles.png", fe::TextureScaling::Nearest);

		auto particle = std::make_shared<fe::Object>(flameMesh);
		candle->name = "Flame";
		particle->meshes[0].hasTransparency = true;
		particle->state.position.y = 1.085f;
		flameParticle = particle;
		scene->AddObject(particle);
	}

	void ProcessInput() {
		SDL_Event event;
		fe::SDLWindow *window = (fe::SDLWindow*)this->window.get();
		while (window->PollSDLEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			auto io = ImGui::GetIO();
			switch (event.type) {
				case SDL_EVENT_QUIT:
				window->PrepareClose();
				break;
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (event.button.button == SDL_BUTTON_LEFT && !io.WantCaptureMouse) {
					window->StartMouseCapture();
				}
				break;
				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				// Get actual pixel dimensions
					Redraw();
					break;
				case SDL_EVENT_MOUSE_MOTION:
				{
					if (!window->capturingMouse) break;
					float sensitivity = 0.1f;

					camera->yaw   += event.motion.xrel * sensitivity;
					camera->pitch -= event.motion.yrel * sensitivity;

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

		float elapsedTime = 0.0f;
		float scale = 10.0f;

		SDL_Event event;
		while (!window->ShouldClose()) {
			
			ProcessInput();

			visualizer.Update();

			float totalMagnitude = 0.0f;
      for (int i = 0; i < NUM_BARS; ++i) {
          totalMagnitude += visualizer.bandMagnitudes[i];
      }
      float avgMagnitude = totalMagnitude / NUM_BARS;
      
      float baseSpeed = 0.0002f;
      float speed = baseSpeed + (avgMagnitude * scale * 0.15f);
      elapsedTime += speed;
      glm::vec3 lightCenter = glm::vec3(5, 5, 5);
      float radius = 3.0f;
      
      scene->GetLights()[0].position = lightCenter + glm::vec3(
          sin(elapsedTime * 0.5f) * radius,
          cos(elapsedTime * 0.3f) * radius * 0.5f,
          sin(elapsedTime * 0.7f) * radius
      );

			glm::vec3 pos = player->state.position + cameraOffset;

			camera->SetPos(pos);
			camera->UpdateDirection();

			flameParticle->LookAt(camera->GetPos());
			wick->LookAt(camera->GetPos());

			scene->GetLights()[0].position += 0.1f;

			
			Update();
			Redraw();
		}

		Destroy();
	}


	void InitUI() override {}
	
	void DrawAudioVisualizer() {	
		ImGui::SetNextWindowSize(ImVec2(440, 240), ImGuiCond_FirstUseEver);
		ImGui::Begin("Audio Spectrum");

		ImVec2 canvasPos  = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize = ImGui::GetContentRegionAvail();
		canvasSize.y = std::max(canvasSize.y, 80.0f);

		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(18, 18, 18, 255));

		float barGap   = 2.0f;
		float barWidth = (canvasSize.x - barGap * (NUM_BARS - 1)) / NUM_BARS;
		float scale    = 8.0f;

		for (int i = 0; i < NUM_BARS; ++i) {
			float ah = visualizer.bandMagnitudes[i] * scale;
			float normalized = std::clamp(ah, 0.0f, 1.0f);
			float barHeight  = normalized * canvasSize.y;

			float x0 = canvasPos.x + i * (barWidth + barGap);
			ImVec2 barMin(x0, canvasPos.y + canvasSize.y - barHeight);
			ImVec2 barMax(x0 + barWidth, canvasPos.y + canvasSize.y);

			ImU32 color = IM_COL32(60 + (int)(195 * normalized), 140, 255 - (int)(140 * normalized), 255);
			draw->AddRectFilled(barMin, barMax, color);

			rectangles[i]->state.scale = glm::vec3(1.0f, barHeight / 10.0f, 1.0f);
		}

		ImGui::Dummy(canvasSize);
		ImGui::End();
	}

	void DrawUI() override {
		BeginFrame();
		DrawDebugUI();

		DrawAudioVisualizer();
		EndFrame();
	}
};
