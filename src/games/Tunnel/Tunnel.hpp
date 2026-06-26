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
#include <ScreenSaverMode.hpp>

class Tunnel : public fe::EditableGame {
public:

	double lastUpdateTime = 0.0f;

	bool canJump = true;

	int mapIndex = 0;

	std::vector<std::shared_ptr<fe::Object>> rectangles;

	AudioVisualiser visualizer;

	bool showCandle = true;

	bool showDebugUI = false;

	std::shared_ptr<fe::Object> wick;
	std::shared_ptr<fe::Object> flameParticle;

	// Camera tweaks
	float cameraPanSpeed = 2.1f;
	float cameraPanFreqX = 0.3f;
	float cameraPanFreqY = 0.2f;
	float cameraPanFreqZ = 0.15f;
	glm::vec3 cameraOffsetScales = glm::vec3(2.0f, 0.5f, 1.0f);
	float cameraPanVariationFreq = 0.001f;
	
	// Light tweaks
	float lightSpeed = 0.3f;
	
	// Light 0
	glm::vec3 lightCenter0 = glm::vec3(0, 5, 5);
	float lightRadius0 = 3.0f;
	float light0FreqX = 0.5f;
	float light0FreqY = 0.3f;
	float light0FreqZ = 0.7f;
	
	// Light 1
	glm::vec3 lightCenter1 = glm::vec3(-5, 4, 3);
	float lightRadius1 = 2.5f;
	float light1FreqX = 0.4f;
	float light1FreqY = 0.25f;
	float light1FreqZ = 0.6f;
	float light1ColorFreq = 0.2f;
	
	// Light 2
	glm::vec3 lightCenter2 = glm::vec3(3, 6, 4);
	float lightRadius2 = 2.0f;
	float light2FreqX = 0.35f;
	float light2FreqY = 0.28f;
	float light2FreqZ = 0.55f;
	float light2ColorFreq = 0.25f;
	
	// Colors
	float bgColorFreq = 0.3f;
	float light1RadialColorFreq = 0.2f;
	float light2RadialColorFreq = 0.25f;
	
	// Flame tweaks
	float flameCycleDuration = 3.0f;
	float flamePhaseMultiplier = 1.10f;
	float flameFlickerSpeed1 = 30.0f;
	float flameFlickerIntensity1 = 0.15f;
	float flameFlickerSpeed2 = 12.0f;
	float flameFlickerIntensity2 = 0.08f;
	float flameShrinkAmount = 0.8f;
	float flameBaseScale = 0.9f;
	
	// Visualizer tweaks
	float visualizerScale = 8.0f;
	float visualizerBarHeightMult = 10.0f;
	
	// Simulation tweaks
	float audioAmplitudeScale = 10.0f;
	float audioSpeedMultiplier = 0.15f;
	float baseSpeedElapsedTimeBumpy = 0.0002f;
	float baseSpeedElapsedTime = 0.0002f;

	bool isCyanide = false;

	Tunnel() : Tunnel(1400, 1200) {}

	Tunnel(int width, int height, bool vr = false) : fe::EditableGame(width, height, vr, false) {

		window->SetTitle(isCyanide ? "HAPPY BIRTHDAY CYANIDE!!" : "HAPPY BIRTHDAY FLEXAN!!");

		SetClearColor(1, 1, 0);

		LoadShaders("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");

		LoadModels();

		scene->AddLight();
		scene->AddLight();
		scene->GetLights()[1].color = {0.9f, 0.4f, 0.3f};
		scene->GetLights()[1].radius = 15.0f;

		scene->GetLights()[2].color = {0.2f, 0.6f, 0.8f};
		scene->GetLights()[2].intensity = 2.0f;

		visualizer.Init();
	}

	static bool SDLCALL LiveRedrawWatcher(void* userdata, SDL_Event* event) {
		Tunnel* game = (Tunnel*)userdata;
		if (event->type == SDL_EVENT_WINDOW_EXPOSED)
			game->Redraw();
		return true;
	}

	void LoadModels() {

		fe::Mesh tunnel = fe::Primitives::GenerateTunnel();
		fe::Mesh bigTunnel = fe::Primitives::GenerateTunnel(0.5f, 20.0f, 64, 20);

		auto tunnelObject = std::make_shared<fe::Object>(bigTunnel);
		tunnelObject->name = "Tunnel";


		this->scene->AddObject(tunnelObject);

		std::vector<glm::vec3> path = {
			{0, 0, 0},
			{2, 1, 0},
			{4, 2, 2},
			{5, 1, 4}
		};
		fe::Mesh tunnel2 = fe::Primitives::GenerateBentTunnel(path, 1.0f, 32, 12, true);
		auto tunne2obj = this->scene->AddObject(tunnel2);
		tunne2obj->name = "Taratatatar";



		auto barShader = std::make_shared<fe::ShaderProgram>("resources/shaders/debug.vert", "resources/shaders/debug.frag");;

		for (int i = 0; i < NUM_BARS; ++i) {
			auto cube = std::make_shared<fe::Object>(fe::Primitives::GenerateCube(1.0f));

			cube->name = "Bar_";
			cube->state.position = glm::vec3(-15.0f + i * 1.0f, 0.0f, -25.0f);
			if (false)
				cube->shader = barShader;

			// cube->meshes[0].sha
	
			this->scene->AddObject(cube);

			rectangles.push_back(cube);
		}

		this->player = std::make_shared<fe::Character>();
		this->scene->AddObject(player);

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
					camera->UpdateDirection();

					pitch = std::clamp(pitch, -89.0f, 89.0f);
					break;
				}
				case SDL_EVENT_KEY_DOWN:
					if (event.key.key == SDLK_F11) {
						window->ToggleFullscreen();
					}
					else if (event.key.key == SDLK_F3) {
						showDebugUI = !showDebugUI;
					}
					break;
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
		window->Show();
		window->DisableVSync();
	
		glm::vec3 cameraOffset = glm::vec3(0);
		player->state.position.z = 5;
		player->state.position.y = 2;
		float elapsedTimeBumpy = 0.0f;
		float elapsedTime = 0.0f;
		SDL_Event event;
		
		while (!window->ShouldClose()) {
			
			ProcessInput();
			visualizer.Update();

			float totalMagnitude = 0.0f;
			for (int i = 0; i < NUM_BARS; ++i) {
					totalMagnitude += visualizer.bandMagnitudes[i];
			}
			float avgMagnitude = totalMagnitude / NUM_BARS;
			
			float speed = baseSpeedElapsedTimeBumpy + (avgMagnitude * audioAmplitudeScale * audioSpeedMultiplier);
			elapsedTimeBumpy += speed;
			elapsedTime += baseSpeedElapsedTime;

			float cameraPanSpeedVariation = abs(sin(elapsedTime * cameraPanVariationFreq)) * 0.5f;
			float cameraPanSpeeda = cameraPanSpeed + cameraPanSpeedVariation;

			cameraOffset.x = sin(elapsedTime * cameraPanSpeeda * cameraPanFreqX) * cameraOffsetScales.x;
			cameraOffset.y = cos(elapsedTime * cameraPanSpeeda * cameraPanFreqY) * cameraOffsetScales.y;
			cameraOffset.z = sin(elapsedTime * cameraPanSpeeda * cameraPanFreqZ) * cameraOffsetScales.z;
			
			float colorR = sin(elapsedTime * bgColorFreq) * 0.5f + 0.5f;
			float colorG = sin(elapsedTime * bgColorFreq + 2.094f) * 0.5f + 0.5f;
			float colorB = sin(elapsedTime * bgColorFreq + 4.189f) * 0.5f + 0.5f;

			SetClearColor(colorR, colorG, colorB);

			float light1R = sin(elapsedTime * light1RadialColorFreq) * 0.5f + 0.5f;
			float light1G = sin(elapsedTime * light1RadialColorFreq + 2.094f) * 0.5f + 0.5f;
			float light1B = sin(elapsedTime * light1RadialColorFreq + 4.189f) * 0.5f + 0.5f;
			scene->GetLights()[1].color = {light1R, light1G, light1B};

			float light2R = sin(elapsedTime * light2RadialColorFreq + 1.047f) * 0.5f + 0.5f;
			float light2G = sin(elapsedTime * light2RadialColorFreq + 3.14f) * 0.5f + 0.5f;
			float light2B = sin(elapsedTime * light2RadialColorFreq + 5.236f) * 0.5f + 0.5f;
			scene->GetLights()[2].color = {light2R, light2G, light2B};

			// Light 0
			scene->GetLights()[0].position = lightCenter0 + glm::vec3(
				sin(elapsedTimeBumpy * light0FreqX * lightSpeed) * lightRadius0,
				cos(elapsedTimeBumpy * light0FreqY * lightSpeed) * lightRadius0 * 0.5f,
				sin(elapsedTimeBumpy * light0FreqZ * lightSpeed) * lightRadius0
			);

			// Light 1
			scene->GetLights()[1].position = lightCenter1 + glm::vec3(
				sin(elapsedTimeBumpy * light1FreqX * lightSpeed) * lightRadius1,
				cos(elapsedTimeBumpy * light1FreqY * lightSpeed) * lightRadius1 * 0.6f,
				sin(elapsedTimeBumpy * light1FreqZ * lightSpeed) * lightRadius1
			);

			// Light 2
			scene->GetLights()[2].position = lightCenter2 + glm::vec3(
				sin(elapsedTimeBumpy * light2FreqX * lightSpeed) * lightRadius2,
				cos(elapsedTimeBumpy * light2FreqY * lightSpeed) * lightRadius2 * 0.7f,
				sin(elapsedTimeBumpy * light2FreqZ * lightSpeed) * lightRadius2
			);
			
			// glm::vec3 pos = player->state.position + cameraOffset;
			glm::vec3 pos = player->state.position;
			camera->SetPos(pos);

			UpdateVisualizerBars();
			
			Update();
			Redraw();
		}
		Destroy();
	}


	void InitUI() override {}

	void UpdateVisualizerBars() {
    for (int i = 0; i < NUM_BARS; ++i) {
      float ah = visualizer.bandMagnitudesSmoothed[i] * visualizerScale;
      float normalized = std::clamp(ah, 0.0f, 1.0f);
      float barHeight = normalized * visualizerBarHeightMult;
      rectangles[i]->state.scale = glm::vec3(1.0f, barHeight, 1.0f);
    }
	}
	
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

		for (int i = 0; i < NUM_BARS; ++i) {
			float ah = visualizer.bandMagnitudesSmoothed[i] * visualizerScale;
			float normalized = std::clamp(ah, 0.0f, 1.0f);
			float barHeight  = normalized * canvasSize.y;

			float x0 = canvasPos.x + i * (barWidth + barGap);
			ImVec2 barMin(x0, canvasPos.y + canvasSize.y - barHeight);
			ImVec2 barMax(x0 + barWidth, canvasPos.y + canvasSize.y);

			ImU32 color = IM_COL32(60 + (int)(195 * normalized), 140, 255 - (int)(140 * normalized), 255);
			draw->AddRectFilled(barMin, barMax, color);
		}

		ImGui::Dummy(canvasSize);
		ImGui::End();
	}

	void DrawUI() override {
		if (!showDebugUI) return;
		BeginFrame();

		DrawAudioVisualizer();
		DrawDebugUI();

		EndFrame();
	}
};
