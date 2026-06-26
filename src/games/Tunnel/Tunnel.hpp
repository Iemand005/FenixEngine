#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <algorithm>
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

	AudioVisualiser visualizer;
	bool showDebugUI = false;

	std::vector<glm::vec3> path;
	int windowStart = 0;
	float pathIndex = 1.0f;
	std::vector<std::shared_ptr<fe::Object>> chunks;

	static constexpr int NUM_CHUNKS = 12;
	static constexpr int TUNNEL_SEGMENTS = 32;
	static constexpr int SUBDIVISIONS_PER_SEG = 12;
	static constexpr int POINTS_PER_CHUNK = 4;
	static constexpr int OVERLAP = 3;

	float lightSpeed = 0.3f;

	float bgColorFreq = 0.3f;
	float visualizerScale = 8.0f;

	float audioAmplitudeScale = 10.0f;
	float audioSpeedMultiplier = 0.15f;
	float baseSpeedElapsedTimeBumpy = 0.0002f;
	float baseSpeedElapsedTime = 0.0002f;

	Tunnel() : Tunnel(1400, 1200) {}

	Tunnel(int width, int height, bool vr = false) : fe::EditableGame(width, height, vr, false) {

		SetClearColor(1, 1, 0);

		LoadShaders("/home/lasse/Documents/Projects/FenixEngine/src/games/Tunnel/resources/shaders/VertexShader.glsl", "/home/lasse/Documents/Projects/FenixEngine/src/games/Tunnel/resources/shaders/FragmentShader.glsl");

		LoadModels();

		visualizer.Init();
	}

	static bool SDLCALL LiveRedrawWatcher(void* userdata, SDL_Event* event) {
		Tunnel* game = (Tunnel*)userdata;
		if (event->type == SDL_EVENT_WINDOW_EXPOSED)
			game->Redraw();
		return true;
	}

	void LoadModels() {
		GenerateInitialTunnels();

		this->player = std::make_shared<fe::Character>();
		this->scene->AddObject(player);
	}

	void GenerateInitialTunnels() {
		path = {
			{0, 0, 0},
			{3, 1, 0},
			{6, 2, 6},
			{10, 2, 14},
			{14, 1, 22},
			{18, 0, 30}
		};
		GrowPath(NUM_CHUNKS + OVERLAP);

		windowStart = 0;
		pathIndex = 1.0f;

		chunks.resize(NUM_CHUNKS);
		for (int i = 0; i < NUM_CHUNKS; i++) {
			chunks[i] = std::make_shared<fe::Object>();
			BuildTunnelMesh(chunks[i], i);
			chunks[i]->name = "Tunnel" + std::to_string(i);
			scene->AddObject(chunks[i]);
		}
	}

	void GrowPath(int count) {
		float t = window->GetTime();
		int base = path.size();
		for (int i = 0; i < count; i++) {
			int seg = base + i;
			glm::vec3 offset(
				sin(t * 0.5f + seg * 0.5f) * 16.0f,
				cos(t * 0.3f + seg * 0.3f) * 8.0f,
				60.0f);
			path.push_back(path.back() + offset);
		}
	}

	void BuildTunnelMesh(std::shared_ptr<fe::Object> obj, int chunkIndex) {
		std::vector<glm::vec3> pts(POINTS_PER_CHUNK);
		for (int i = 0; i < POINTS_PER_CHUNK; i++)
			pts[i] = path[windowStart + chunkIndex + i];
		obj->meshes.clear();
		obj->meshes.push_back(
			fe::Primitives::GenerateBentTunnel(
				pts, 1.0f, TUNNEL_SEGMENTS, SUBDIVISIONS_PER_SEG, true)
		);
	}

	void SlideChunks() {
		while (windowStart + NUM_CHUNKS + OVERLAP >= (int)path.size())
			GrowPath(1);

		BuildTunnelMesh(chunks[0], NUM_CHUNKS);
		std::rotate(chunks.begin(), chunks.begin() + 1, chunks.end());
		windowStart++;
	}

	glm::vec3 GetGlobalPosition(float idx) const {
		int i = (int)idx;
		float t = idx - i;
		if (i < 1) { i = 1; t = 0.0f; }
		if (i + 2 >= (int)path.size()) { i = (int)path.size() - 3; t = 0.0f; }
		return fe::Primitives::CatmullRom(path[i-1], path[i], path[i+1], path[i+2], t);
	}

	glm::vec3 GetGlobalTangent(float idx) const {
		int i = (int)idx;
		float t = idx - i;
		if (i < 1) { i = 1; t = 0.0f; }
		if (i + 2 >= (int)path.size()) { i = (int)path.size() - 3; t = 0.0f; }
		float t2 = t * t;
		const glm::vec3& p0 = path[i-1];
		const glm::vec3& p1 = path[i];
		const glm::vec3& p2 = path[i+1];
		const glm::vec3& p3 = path[i+2];
		return 0.5f * (
			(-p0 + p2) +
			2.0f * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t +
			3.0f * (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t2
		);
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

		player->state.position.z = 5;
		player->state.position.y = 2;
		float elapsedTimeBumpy = 0.0f;
		float elapsedTime = 0.0f;

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

			float colorR = sin(elapsedTime * bgColorFreq) * 0.5f + 0.5f;
			float colorG = sin(elapsedTime * bgColorFreq + 2.094f) * 0.5f + 0.5f;
			float colorB = sin(elapsedTime * bgColorFreq + 4.189f) * 0.5f + 0.5f;
			SetClearColor(colorR, colorG, colorB);

			float cameraSpeed = 50.0f;
			pathIndex += baseSpeedElapsedTime * cameraSpeed;

			while (pathIndex > windowStart + 2.0f)
				SlideChunks();

			glm::vec3 cameraPos = GetGlobalPosition(pathIndex);
			camera->SetPos(cameraPos);

			glm::vec3 tangent = GetGlobalTangent(pathIndex);
			camera->LookAt(cameraPos + glm::normalize(tangent) * 10.0f);

			float audioR = 0.0f, audioG = 0.0f, audioB = 0.0f;
			for (int i = 0; i < NUM_BARS; i++) {
				float val = visualizer.bandMagnitudesSmoothed[i];
				float frac = (float)i / NUM_BARS;
				audioR += val * (1.0f - frac);
				audioG += val * (0.5f - fabs(frac - 0.5f) * 2.0f);
				audioB += val * frac;
			}
			float total = audioR + audioG + audioB;
			if (total > 0.0f) {
				audioR /= total; audioG /= total; audioB /= total;
			}
			scene->GetLights()[0].position = cameraPos;
			scene->GetLights()[0].color = glm::vec3(1.0f, 0.9f, 0.7f) + glm::vec3(audioR, audioG, audioB) * 0.4f;
			scene->GetLights()[0].intensity = 3.0f;
			scene->GetLights()[0].radius = 80.0f;

			shader->Use();
			shader->SetFloat("wobbleAmount", 0.2f);
			shader->SetFloat("time", elapsedTime);
			shader->SetVec3("objectColor", glm::vec3(0.95f, 0.25f, 0.55f));

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
