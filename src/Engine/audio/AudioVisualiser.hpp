
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "kiss_fftr.h"

#ifdef _WIN32
#include <audio/WasapiLoopbackCapture.hpp>
#else
#include <audio/PipeWireLoopbackCapture.hpp>
#endif

const int FFT_SIZE = 1024;               
const int BINS = (FFT_SIZE / 2) + 1;  

const int NUM_BARS = 32;

std::vector<float> audioSamples;         
kiss_fftr_cfg      fftConfig;            
float              fftInput[FFT_SIZE];  
kiss_fft_cpx       fftOutput[BINS]; 

class AudioVisualiser {
public:
	void Init() {
		fftConfig = kiss_fftr_alloc(FFT_SIZE, 0, nullptr, nullptr);
#ifdef _WIN32
		g_loopback.Init();
#else
		g_pwLoopback.Init();
#endif
	}

	void Update() {
		Poll();
		UpdateVisualizerData();
	}

	void Poll() {
		#ifdef _WIN32
    g_loopback.Poll(audioSamples);
#else
    g_pwLoopback.Poll(audioSamples);
#endif
    if (audioSamples.size() > FFT_SIZE) {
        audioSamples.erase(audioSamples.begin(), audioSamples.end() - FFT_SIZE);
    }
	}

	void UpdateVisualizerData() {
		if (audioSamples.size() < FFT_SIZE || !fftConfig) return;

		for (int i = 0; i < FFT_SIZE; ++i) {
			fftInput[i] = audioSamples[i];
		}

		kiss_fftr(fftConfig, fftInput, fftOutput);

		float magnitudes[BINS];
		for (int i = 0; i < BINS; ++i) {
			float real = fftOutput[i].r;
			float imag = fftOutput[i].i;
			magnitudes[i] = std::sqrt(real * real + imag * imag) / FFT_SIZE;
		}

		ComputeBands(magnitudes, BINS, bandMagnitudes, NUM_BARS);

		// Smooth: jump up instantly, decay slowly — the classic "VU meter" feel
		for (int b = 0; b < NUM_BARS; ++b) {
			bandMagnitudesSmoothed[b] = std::max(bandMagnitudes[b], bandMagnitudesSmoothed[b] * 0.85f);
		}
	}

};