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
	glm::vec3 lastUp = glm::vec3(0, 1, 0);
	glm::vec3 lastRight = glm::vec3(1, 0, 0);
	glm::vec3 prevEndForward{0};
	bool hasPrevEnd = false;

	static constexpr int POINTS_PER_CHUNK = 4;
	static constexpr int SHIFT = 3;
	static constexpr int MAX_CHUNKS = 32;
	static constexpr int TUNNEL_SEGMENTS = 64;
	static constexpr int SUBDIVISIONS_PER_SEG = 48;

	int NUM_CHUNKS = 4;

	float lightSpeed = 0.3f;

	float bgColorFreq = 0.3f;
	float visualizerScale = 8.0f;

	float audioAmplitudeScale = 10.0f;
	float audioSpeedMultiplier = 0.15f;
	float baseSpeedElapsedTimeBumpy = 0.0002f;
	float baseSpeedElapsedTime = 0.0002f;

	float cameraSpeed = 1.0f;
	float motionAmount = 1.2f;
	float tunnelRoundness = 0.0f;
	float haustraStrength = 0.6f;
	float animationSpeed = 1.0f;
	float turnStrength = 1.0f;
	float twistStrength = 1.0f;
	float swirlStrength = 1.0f;
	float farPlane = 500.0f;
	bool freeCamera = false;
	float freeCamSpeed = 15.0f;
	float segmentLength = 12.0f;

	Tunnel() : Tunnel(1400, 1200) {}

	Tunnel(int width, int height, bool vr = false) : fe::EditableGame(width, height, vr, false) {

		SetClearColor(1, 1, 0);

		LoadShaders("resources/shaders/VertexShader.glsl", "resources/shaders/FragmentShader.glsl");

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
			{3, 1, 5},
			{8, 3, 12},
			{10, 5, 22}
		};
		GrowPath(NUM_CHUNKS * SHIFT);

		windowStart = 0;
		pathIndex = 0.5f;

		chunks.resize(NUM_CHUNKS);
		for (int i = 0; i < NUM_CHUNKS; i++) {
			chunks[i] = std::make_shared<fe::Object>();
			BuildTunnelMesh(chunks[i], windowStart + i * SHIFT);
			chunks[i]->name = "Tunnel" + std::to_string(i);
			scene->AddObject(chunks[i]);
		}
	}

	void GrowPath(int count) {
		float t = window->GetTime();
		int freeSeg = (path.size() - 4) / 3;
		float sl = segmentLength;
		for (int i = 0; i < count; i++) {
			int n = path.size();
			float seg = freeSeg;
			float ampMod = 0.05 + 0.95 * pow(abs(sin(seg * 1.5 + t * 0.1)), 0.5);
			if (n % 3 == 1) {
				path.push_back(2.0f * path[n-1] - path[n-2]);
			} else if (n % 3 == 2) {
				glm::vec3 dir = path[n-1] - path[n-2];
				float dirLen = glm::length(dir);
				glm::vec3 step = (dirLen > 1e-6f)
					? dir * (sl / dirLen)
					: glm::vec3(0, 0, sl);
				float sa = seg * 2.0f + t * 0.3f;
				glm::vec3 wobble(
					(sin(t * 1.5f + seg * 4.0f) * 6.0f * turnStrength
					 + sin(sa) * 8.0f * swirlStrength) * ampMod,
					(cos(t * 1.0f + seg * 3.5f) * 4.0f * twistStrength) * ampMod,
					(sin(t * 0.8f + seg * 3.0f) * 4.0f
					 + cos(sa) * 8.0f * swirlStrength) * ampMod);
				path.push_back(path[n-1] + step + wobble);
			} else {
				glm::vec3 dir = path[n-1] - path[n-2];
				float dirLen = glm::length(dir);
				glm::vec3 step = (dirLen > 1e-6f)
					? dir * (sl / dirLen)
					: glm::vec3(0, 0, sl);
				float sa = seg * 2.0f + t * 0.3f;
				glm::vec3 wobble(
					(sin(t * 1.2f + seg * 3.5f) * 7.0f * turnStrength
					 + sin(sa + 1.0f) * 9.0f * swirlStrength) * ampMod,
					(cos(t * 0.8f + seg * 3.0f) * 5.0f * twistStrength) * ampMod,
					(sin(t * 0.6f + seg * 2.5f) * 4.5f
					 + cos(sa + 1.0f) * 9.0f * swirlStrength) * ampMod);
				path.push_back(path[n-1] + step + wobble);
				freeSeg++;
			}
		}
	}

	void BuildTunnelMesh(std::shared_ptr<fe::Object> obj, int pointIdx) {
		std::vector<glm::vec3> pts(POINTS_PER_CHUNK);
		for (int i = 0; i < POINTS_PER_CHUNK; i++)
			pts[i] = path[pointIdx + i];

		const glm::vec3* endFwdPtr = nullptr;
		glm::vec3 endFwd;
		{
			glm::vec3 diff = pts[3] - pts[2];
			float len = glm::length(diff);
			if (len > 1e-6f) {
				endFwd = diff / len;
				endFwdPtr = &endFwd;
			}
		}

		obj->meshes.clear();
		obj->PushMesh(
			fe::Primitives::GenerateBentTunnel(
				pts, 1.0f, TUNNEL_SEGMENTS, SUBDIVISIONS_PER_SEG, true,
				&lastUp, &lastRight, &lastUp, &lastRight,
				hasPrevEnd ? &prevEndForward : nullptr, endFwdPtr,
				(float)(pointIdx / SHIFT))
		);

		if (endFwdPtr)
			prevEndForward = endFwd;
		hasPrevEnd = true;
	}

	void SlideChunks() {
		int needed = windowStart + NUM_CHUNKS * SHIFT + POINTS_PER_CHUNK;
		while (needed >= (int)path.size())
			GrowPath(SHIFT);

		BuildTunnelMesh(chunks[0], windowStart + NUM_CHUNKS * SHIFT);
		std::rotate(chunks.begin(), chunks.begin() + 1, chunks.end());
		windowStart += SHIFT;
	}

	void SetActiveChunks(int newCount) {
		newCount = std::clamp(newCount, 2, MAX_CHUNKS);
		if (newCount > NUM_CHUNKS) {
			for (int i = NUM_CHUNKS; i < newCount; i++) {
				chunks.push_back(std::make_shared<fe::Object>());
				BuildTunnelMesh(chunks[i], windowStart + i * SHIFT);
				chunks[i]->name = "Tunnel" + std::to_string(i);
				scene->AddObject(chunks[i]);
			}
		} else if (newCount < NUM_CHUNKS) {
			for (int i = newCount; i < NUM_CHUNKS; i++)
				scene->RemoveObject(chunks[i]);
			chunks.resize(newCount);
		}
		NUM_CHUNKS = newCount;
	}

	glm::vec3 GetGlobalPosition(float idx) const {
		int seg = (int)idx;
		float t = idx - seg;
		if (seg < 0) { seg = 0; t = 0.0f; }
		int base = seg * SHIFT;
		if (base + 3 >= (int)path.size()) {
			base = (int)path.size() - 4;
			t = 0.0f;
		}
		return fe::Primitives::CubicBezier(path[base], path[base+1], path[base+2], path[base+3], t);
	}

	glm::vec3 GetGlobalTangent(float idx) const {
		int seg = (int)idx;
		float t = idx - seg;
		if (seg < 0) { seg = 0; t = 0.0f; }
		int base = seg * SHIFT;
		if (base + 3 >= (int)path.size()) {
			base = (int)path.size() - 4;
			t = 0.0f;
		}
		return fe::Primitives::CubicBezierTangent(path[base], path[base+1], path[base+2], path[base+3], t);
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
					camera->pitch = std::clamp(camera->pitch, -89.0f, 89.0f);
					break;
				}
				case SDL_EVENT_KEY_DOWN:
					if (event.key.key == SDLK_F11) {
						window->ToggleFullscreen();
					}
					else if (event.key.key == SDLK_F3) {
						showDebugUI = !showDebugUI;
					}
					else if (event.key.key == SDLK_F2) {
						freeCamera = !freeCamera;
						window->StartMouseCapture();
					}
					break;
			}
		}

		if (!freeCamera) {
			if (window->IsKeyDown(SDL_SCANCODE_W)) this->player->Move(fe::Direction::Forwards, camera.get());
			if (window->IsKeyDown(SDL_SCANCODE_A)) this->player->Move(fe::Direction::Left, camera.get());
			if (window->IsKeyDown(SDL_SCANCODE_S)) this->player->Move(fe::Direction::Backwards, camera.get());
			if (window->IsKeyDown(SDL_SCANCODE_D)) this->player->Move(fe::Direction::Right, camera.get());

			if (window->IsKeyDown(SDL_SCANCODE_SPACE)) this->player->Move(fe::Direction::Up, camera.get());
			if (window->IsKeyDown(SDL_SCANCODE_LSHIFT)) this->player->Move(fe::Direction::Down, camera.get());
		}

		if (window->IsKeyDown(SDL_SCANCODE_ESCAPE)) window->StopMouseCapture();
		if (ImGui::GetIO().WantCaptureMouse) window->StopMouseCapture();
	}

	void Run() {
		auto window = this->GetWindow<fe::SDLWindow>();
		window->Show();
		window->DisableVSync();

		player->state.position.z = 5;
		player->state.position.y = 2;
		camera->farDist = farPlane;
		camera->SetAspect(camera->aspect);
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

			pathIndex += baseSpeedElapsedTime * cameraSpeed;

			while (pathIndex > (float)(windowStart / SHIFT) + 1.0f)
				SlideChunks();

			if (freeCamera) {
				float dt = fpsCounter.deltaTime;
				float spd = freeCamSpeed * dt;
				glm::vec3 cp = camera->GetPos();
				glm::vec3 right = glm::normalize(glm::cross(camera->front, camera->up));
				if (window->IsKeyDown(SDL_SCANCODE_W)) cp += camera->front * spd;
				if (window->IsKeyDown(SDL_SCANCODE_S)) cp -= camera->front * spd;
				if (window->IsKeyDown(SDL_SCANCODE_A)) cp -= right * spd;
				if (window->IsKeyDown(SDL_SCANCODE_D)) cp += right * spd;
				if (window->IsKeyDown(SDL_SCANCODE_SPACE)) cp += camera->up * spd;
				if (window->IsKeyDown(SDL_SCANCODE_LSHIFT)) cp -= camera->up * spd;
				camera->SetPos(cp);
				glm::vec3 riderPos = GetGlobalPosition(pathIndex);
				player->state.position = riderPos;
				scene->GetLights()[0].position = riderPos;
			} else {
				glm::vec3 cameraPos = GetGlobalPosition(pathIndex);
				camera->SetPos(cameraPos);
				glm::vec3 tangent = GetGlobalTangent(pathIndex);
				camera->LookAt(cameraPos + glm::normalize(tangent) * 10.0f);
				scene->GetLights()[0].position = cameraPos;
			}

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
			scene->GetLights()[0].color = glm::vec3(1.0f, 0.9f, 0.7f) + glm::vec3(audioR, audioG, audioB) * 0.4f;
			scene->GetLights()[0].intensity = 3.0f;
			scene->GetLights()[0].radius = 80.0f;

			shader->Use();
			shader->SetFloat("wobbleAmount", motionAmount);
			shader->SetFloat("roundness", tunnelRoundness);
			shader->SetFloat("haustraStrength", haustraStrength);
			shader->SetFloat("animSpeed", animationSpeed);
			shader->SetFloat("time", elapsedTime);
			shader->SetVec3("objectColor", glm::vec3(0.55f, 0.08f, 0.12f));

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

		ImGui::Begin("Tunnel Controls");
		ImGui::Text(freeCamera ? "Camera: FREE (F2 to toggle)" : "Camera: FOLLOW (F2 to toggle)");
		ImGui::SliderFloat("Motion Amount", &motionAmount, 0.0f, 3.0f);
		ImGui::SliderFloat("Roundness", &tunnelRoundness, 0.0f, 0.6f);
		ImGui::SliderFloat("Haustra Strength", &haustraStrength, 0.0f, 2.0f);
		ImGui::SliderFloat("Animation Speed", &animationSpeed, 0.0f, 5.0f);
		ImGui::SliderFloat("Camera Speed", &cameraSpeed, 0.0f, 5.0f);
		ImGui::SliderFloat("Free Cam Speed", &freeCamSpeed, 1.0f, 100.0f);
		ImGui::SliderFloat("Segment Length", &segmentLength, 1.0f, 30.0f);
		int nc = NUM_CHUNKS;
		if (ImGui::SliderInt("Active Segments", &nc, 2, MAX_CHUNKS) && nc != NUM_CHUNKS)
			SetActiveChunks(nc);
		ImGui::Separator();
		ImGui::Text("Path Curves");
		ImGui::SliderFloat("Turn Strength", &turnStrength, 0.0f, 10.0f);
		ImGui::SliderFloat("Twist Strength", &twistStrength, 0.0f, 10.0f);
		ImGui::SliderFloat("Swirl Strength", &swirlStrength, 0.0f, 10.0f);
		ImGui::Separator();
		if (ImGui::SliderFloat("Far Plane", &farPlane, 50.0f, 5000.0f)) {
			camera->farDist = farPlane;
			camera->SetAspect(camera->aspect);
		}
		ImGui::End();

		DrawDebugUI();

		EndFrame();
	}
};
