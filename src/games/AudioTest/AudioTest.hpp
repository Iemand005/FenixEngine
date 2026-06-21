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

#include "kiss_fftr.h"

const int FFT_SIZE = 1024;               
const int BINS = (FFT_SIZE / 2) + 1;     

std::vector<float> audioSamples;         
kiss_fftr_cfg      fftConfig;            
float              fftInput[FFT_SIZE];  
kiss_fft_cpx       fftOutput[BINS];      

void SDLCALL MinimalAudioCallback(void* userdata, const SDL_AudioSpec* spec, float* buffer, int buflen) {
    if (!buffer || buflen <= 0) return;

    int num_floats = buflen / (int)sizeof(float);
    int frames = num_floats / spec->channels;

    for (int i = 0; i < frames; ++i) {
        float monoSample = 0.0f;
        for (int c = 0; c < spec->channels; ++c) {
            monoSample += buffer[i * spec->channels + c];
        }
        audioSamples.push_back(monoSample / spec->channels);
    }

    if (audioSamples.size() > FFT_SIZE) {
        audioSamples.erase(audioSamples.begin(), audioSamples.end() - FFT_SIZE);
    }
}



class AudioTest : public fe::EditableGame {
public:

	std::vector<std::string> messages;

	double lastUpdateTime = 0.0f;

	bool canJump = true;

	int mapIndex = 0;

	float magnitudes[BINS];

	AudioTest() : AudioTest(800, 640) {}

	AudioTest(int width, int height, bool vr = false) : fe::EditableGame(width, height, vr) {
		LoadModels();

		SDL_AudioSpec wavSpec;
		Uint8* data = nullptr;
		Uint32 len = 0;

		if (!SDL_LoadWAV("resources/audio/file_example_WAV_5MG.wav", &wavSpec, &data, &len))
		{
			std::cout << "Failed to load WAV: " << SDL_GetError() << "\n";
			return;
		}

		SDL_AudioSpec targetSpec;
		targetSpec.format = SDL_AUDIO_F32;
		targetSpec.channels = 2;
		targetSpec.freq = wavSpec.freq;

		SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &targetSpec, nullptr, nullptr);

		SDL_SetAudioStreamFormat(stream, &wavSpec, &targetSpec);

		SDL_PutAudioStreamData(stream, data, len);

		SDL_AudioDeviceID dev = SDL_GetAudioStreamDevice(stream);
		SDL_SetAudioPostmixCallback(dev, MinimalAudioCallback, nullptr);

		SDL_ResumeAudioDevice(dev);

		fftConfig = kiss_fftr_alloc(FFT_SIZE, 0, nullptr, nullptr);
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

		SDL_Event event;
		while (!window->ShouldClose()) {

			UpdateVisualizerData();

			ProcessInput();
			glm::vec3 pos = player->state.position + cameraOffset;
			camera->SetPos(pos - camera->front * 6.0f);
			camera->UpdateDirection();
			
			if (isConnectedToServer) client->sendPosition(player->state.position, player->state.rotation);
			
			Update();
			Redraw();
		}

		Destroy();
	}

	const int NUM_BARS = 32;
	float bandMagnitudes[NUM_BARS] = {0};       
	float bandMagnitudesSmoothed[NUM_BARS] = {0};

	void ComputeBands(const float* magnitudes, int bins, float* bandsOut, int numBars) {
		float maxBin = (float)(bins - 1);
		for (int b = 0; b < numBars; ++b) {
			float t0 = (float)b / numBars;
			float t1 = (float)(b + 1) / numBars;
			int bin0 = std::max(1, (int)std::pow(maxBin, t0));   
			int bin1 = std::min(bins - 1, std::max(bin0 + 1, (int)std::pow(maxBin, t1)));

			float maxVal = 0.0f;
			for (int i = bin0; i <= bin1; ++i)
				maxVal = std::max(maxVal, magnitudes[i]);
			bandsOut[b] = maxVal;
		}
	}

	void UpdateVisualizerData() {
		if (audioSamples.size() < FFT_SIZE) return;

		for (int i = 0; i < FFT_SIZE; ++i) {
			fftInput[i] = audioSamples[i];
		}

		kiss_fftr(fftConfig, fftInput, fftOutput);

		for (int i = 0; i < BINS; ++i) {
			float real = fftOutput[i].r;
			float imag = fftOutput[i].i;
			
			magnitudes[i] = std::sqrt(real * real + imag * imag) / FFT_SIZE;
		}

		std::cout << "Bass: " << magnitudes[4] << " | Mids: " << magnitudes[20] << "\n";
	}

	void InitUI() override {}

	void DrawUI() override {
		DrawDebugUI();

		ImGui::Begin("Audio");
		{
		ImGui::Text("Hello, World!");
		ImGui::Text("FPS %.1f", fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::Text("Objects: %zu", this->scene->GetObjects().size());
		size_t totalVertices = 0;
		for (auto& obj : this->scene->GetObjects())
			for (auto& mesh : obj->meshes) totalVertices += mesh.GetVertices().size();
		ImGui::Text("Vertices: %zu", totalVertices);

		if (ImGui::Button("Enable VR!", ImVec2(100, 20))) {
			this->EnableXR ();
		}

		if (ImGui::Button("Disable VR :()", ImVec2(100, 20))) {
			this->DestroyXR();
		}

		if (ImGui::Button("Enable AA", ImVec2(70, 20))) {
			std::cout << "Button clicked!" << std::endl;
		}

		static bool wireframe = false;
		if (ImGui::Checkbox("Enable Wireframe", &wireframe)) {
			if (wireframe) this->EnableWireframe();
			else this->DisableWireframe();
		}

		fe::Object* model = this->player.get();
		ImGui::SliderFloat3("Position", &model->state.position.x, -10.0f, 10.0f);
		for (size_t i = 0; i < this->npcs.size(); ++i) {
			ImGui::Text("NPC %zu", i);
			ImGui::SliderFloat3(("Position##npc" + std::to_string(i)).c_str(), &this->npcs[i]->state.position.x, -10.0f, 10.0f);
			ImGui::SliderFloat3(("Rotation##npc" + std::to_string(i)).c_str(), &this->npcs[i]->state.rotation.x, -180.0f, 180.0f);
		}
		}
		ImGui::End();
	}
};
