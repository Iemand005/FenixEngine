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

	AudioVisualiser visualizer;
	bool showDebugUI = false;

	std::vector<glm::vec3> currentPath;
	std::vector<glm::vec3> nextPath;
	std::shared_ptr<fe::Object> currentTunnel;
	std::shared_ptr<fe::Object> nextTunnel;
	float currentPathLength = 0.0f;
	float nextPathLength = 0.0f;
	float pathProgress = 0.0f;

	static constexpr int TUNNEL_SEGMENTS = 32;
	static constexpr int SUBDIVISIONS_PER_SEG = 12;

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
		currentPath = {
			{0, 0, 0},
			{2, 1, 0},
			{4, 2, 2},
			{5, 1, 4},
			{6, 0, 6},
			{7, -1, 8},
			{8, 0, 10},
			{9, 1, 12}
		};

		currentTunnel = std::make_shared<fe::Object>(
			fe::Primitives::GenerateBentTunnel(currentPath, 1.0f, TUNNEL_SEGMENTS, SUBDIVISIONS_PER_SEG, true)
		);
		currentTunnel->name = "CurrentTunnel";
		scene->AddObject(currentTunnel);
		currentPathLength = CalcPathLength(currentPath);

		GenerateNextTunnelPath();

		nextTunnel = std::make_shared<fe::Object>(
			fe::Primitives::GenerateBentTunnel(nextPath, 1.0f, TUNNEL_SEGMENTS, SUBDIVISIONS_PER_SEG, true)
		);
		nextTunnel->name = "NextTunnel";
		scene->AddObject(nextTunnel);
		nextPathLength = CalcPathLength(nextPath);

		CopySeamRing(currentTunnel->meshes[0], nextTunnel->meshes[0]);
	}

	void GenerateNextTunnelPath() {
		glm::vec3 lastPoint = currentPath.back();
		glm::vec3 prevPoint = currentPath[currentPath.size() - 2];
		glm::vec3 thirdLast = currentPath[currentPath.size() - 3];
		glm::vec3 mirrorPoint = lastPoint + (lastPoint - prevPoint);

		nextPath.clear();
		nextPath.reserve(8);
		nextPath.push_back(thirdLast);
		nextPath.push_back(prevPoint);
		nextPath.push_back(lastPoint);
		nextPath.push_back(mirrorPoint);

		float elapsedTime = window->GetTime();

		for (int i = 0; i < 4; i++) {
			glm::vec3 offset(
				sin(elapsedTime * 0.5f + i * 0.5f) * 3.0f,
				cos(elapsedTime * 0.3f + i * 0.3f) * 1.5f,
				2.0f * (i + 1));
			nextPath.push_back(mirrorPoint + offset);
		}
	}

	static void CopySeamRing(fe::Mesh& currentMesh, const fe::Mesh& nextMesh) {
		size_t ringSize = TUNNEL_SEGMENTS;
		size_t nextRingStart = 2 * SUBDIVISIONS_PER_SEG * ringSize;
		size_t currLastStart = currentMesh.vertices.size() - ringSize;
		std::copy(
			nextMesh.vertices.begin() + nextRingStart,
			nextMesh.vertices.begin() + nextRingStart + ringSize,
			currentMesh.vertices.begin() + currLastStart
		);
	}

	void SwapTunnels() {
		// Save the forward-difference ring at lastPoint from nextTunnel
		const auto& nextVerts = nextTunnel->meshes[0].vertices;
		size_t ringSize = TUNNEL_SEGMENTS;
		size_t lastPointRingStart = 2 * SUBDIVISIONS_PER_SEG * ringSize;
		std::vector<fe::Vertex> seamVerts(
			nextVerts.begin() + lastPointRingStart,
			nextVerts.begin() + lastPointRingStart + ringSize
		);

		currentPath = nextPath;
		currentPathLength = nextPathLength;

		std::swap(currentTunnel, nextTunnel);
		currentTunnel->name = "CurrentTunnel";
		nextTunnel->name = "NextTunnel";

		GenerateNextTunnelPath();

		nextTunnel->meshes.clear();
		nextTunnel->meshes.push_back(
			fe::Primitives::GenerateBentTunnel(nextPath, 1.0f, TUNNEL_SEGMENTS, SUBDIVISIONS_PER_SEG, true)
		);

		// Stamp the saved seam vertices onto the new mesh's ring at lastPoint
		auto& newVerts = nextTunnel->meshes[0].vertices;
		std::copy(seamVerts.begin(), seamVerts.end(),
				  newVerts.begin() + lastPointRingStart);

		nextPathLength = CalcPathLength(nextPath);
	}

	static float CalcPathLength(const std::vector<glm::vec3>& path) {
		float length = 0.0f;
		for (size_t i = 0; i < path.size() - 1; i++)
			length += glm::distance(path[i], path[i + 1]);
		return length;
	}

	struct PathSample {
		int seg;
		float t;
		glm::vec3 p0, p1, p2, p3;
	};

	PathSample SamplePath(const std::vector<glm::vec3>& path, float progress) const {
		progress = glm::clamp(progress, 0.0f, 1.0f);
		float scaled = progress * (float)(path.size() - 1);
		int seg = (int)scaled;
		if (seg >= (int)path.size() - 1) {
			seg = (int)path.size() - 2;
			scaled = (float)seg + 1.0f;
		}
		float t = scaled - seg;
		return {
			seg, t,
			(seg > 0) ? path[seg - 1] : path[seg] - (path[seg + 1] - path[seg]),
			path[seg],
			path[seg + 1],
			(seg + 2 < (int)path.size()) ? path[seg + 2] : path[seg + 1] + (path[seg + 1] - path[seg])
		};
	}

	glm::vec3 GetSmoothPathPosition(const std::vector<glm::vec3>& path, float progress) const {
		auto s = SamplePath(path, progress);
		return fe::Primitives::CatmullRom(s.p0, s.p1, s.p2, s.p3, s.t);
	}

	glm::vec3 GetSmoothPathTangent(const std::vector<glm::vec3>& path, float progress) const {
		auto s = SamplePath(path, progress);
		float t2 = s.t * s.t;
		return 0.5f * (
			(-s.p0 + s.p2) +
			2.0f * (2.0f * s.p0 - 5.0f * s.p1 + 4.0f * s.p2 - s.p3) * s.t +
			3.0f * (-s.p0 + 3.0f * s.p1 - 3.0f * s.p2 + s.p3) * t2
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

			float cameraSpeed = 15.0f;
			pathProgress += baseSpeedElapsedTime * cameraSpeed / currentPathLength;

			if (pathProgress >= 1.0f) {
				SwapTunnels();
				pathProgress = 2.0f / (currentPath.size() - 1);
			}

			glm::vec3 cameraPos = GetSmoothPathPosition(currentPath, pathProgress);
			camera->SetPos(cameraPos);

			glm::vec3 tangent = GetSmoothPathTangent(currentPath, pathProgress);
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
			scene->GetLights()[0].color = {audioR, audioG, audioB};
			scene->GetLights()[0].intensity = 3.0f;
			scene->GetLights()[0].radius = 8.0f;

			shader->Use();
			shader->SetFloat("wobbleAmount", 0.2f);
			shader->SetVec3("objectColor", glm::vec3(0.2f, 0.8f, 1.0f));

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
