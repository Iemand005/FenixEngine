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

#include "kiss_fftr.h"

#include <EditableGame.hpp>
#include <Primitives.hpp>
#ifdef _WIN32
#include <audio/WasapiLoopbackCapture.hpp>
#else
#include <audio/PipeWireLoopbackCapture.hpp>
#endif
#include <audio/AudioVisualiser.hpp>


const int FFT_SIZE = 1024;               
const int BINS = (FFT_SIZE / 2) + 1;  

const int NUM_BARS = 32;


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



class Cake : public fe::EditableGame {
public:

	double lastUpdateTime = 0.0f;

	bool canJump = true;

	int mapIndex = 0;

	std::vector<std::shared_ptr<fe::Object>> rectangles;

	AudioVisualiser visualizer;

	Cake() : Cake(800, 640) {}

	Cake(int width, int height, bool vr = false) : fe::EditableGame(width, height, vr) {

		

		LoadShaders("resources/shaders/debug.vert", "resources/shaders/debug.frag");

		LoadModels();

		for (int i = 0; i < NUM_BARS; ++i) {
			auto cube = std::make_shared<fe::Object>(fe::Primitives::GenerateCube(1.0f));

			cube->state.position = glm::vec3(-15.0f + i * 1.0f, 0.0f, -5.0f);
	
			this->scene->AddObject(cube);

			rectangles.push_back(cube);
		}


// #ifdef _WIN32
// 		g_loopback.Init();
// #else
//     	g_pwLoopback.Init();
// #endif
// 		fftConfig = kiss_fftr_alloc(FFT_SIZE, 0, nullptr, nullptr);

		visualizer.Init();


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
		
		


		SDL_AddEventWatch(LiveRedrawWatcher, this);
	}

	static bool SDLCALL LiveRedrawWatcher(void* userdata, SDL_Event* event) {
		Cake* game = (Cake*)userdata;
		if (event->type == SDL_EVENT_WINDOW_EXPOSED) {
			game->Redraw();
		}
		return true;
	}

	void LoadModels() {

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

			visualizer.Update();

			ProcessInput();

			glm::vec3 pos = player->state.position + cameraOffset;

			camera->SetPos(pos);
			camera->UpdateDirection();
			
			Update();
			Redraw();
		}

		Destroy();
	}

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
			float ah = bandMagnitudesSmoothed[i] * scale;
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
