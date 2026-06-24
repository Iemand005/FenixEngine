
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