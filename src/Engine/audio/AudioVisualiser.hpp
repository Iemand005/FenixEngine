
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <cmath>

#ifndef FE_EXCLUDE_KISSFFT
#include "kiss_fftr.h"
#endif

#ifdef __EMSCRIPTEN__
#include <audio/WebAudioCapture.hpp>
#elif defined(_WIN32)
#include <audio/WasapiLoopbackCapture.hpp>
#else
#include <audio/PipeWireLoopbackCapture.hpp>
#endif

const int FFT_SIZE = 1024;               
const int BINS = (FFT_SIZE / 2) + 1;  

const int NUM_BARS = 32;

std::vector<float> audioSamples;         
#ifndef FE_EXCLUDE_KISSFFT
kiss_fftr_cfg      fftConfig;            
#endif
float              fftInput[FFT_SIZE];  
#ifndef FE_EXCLUDE_KISSFFT
kiss_fft_cpx       fftOutput[BINS]; 
#endif

class AudioVisualiser {
public:
	void Init() {
#ifndef FE_EXCLUDE_KISSFFT
		fftConfig = kiss_fftr_alloc(FFT_SIZE, 0, nullptr, nullptr);
#endif
#ifdef __EMSCRIPTEN__
		g_webAudio.Init();
#elif defined(_WIN32)
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
#ifdef __EMSCRIPTEN__
		g_webAudio.Poll(audioSamples);
#elif defined(_WIN32)
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

	bool smoothed = true;

	void UpdateVisualizerData() {
#ifdef __EMSCRIPTEN__
		// The browser (AnalyserNode) already computed the FFT; reuse its bins
		// directly instead of compiling kissfft into the wasm. The browser
		// reports near-full-scale (0..1) magnitudes, so calibrate them down to
		// roughly match the kissfft path's sqrt(r*r+i*i)/FFT_SIZE scale.
		const float WEB_BIN_SCALE = 0.35f;
		int binCount = g_webAudio.GetBinCount();
		if (binCount < 2) return;

		float magnitudes[WebAudioCapture::MAX_BINS];
		const float* src = g_webAudio.GetMagnitudes();
		for (int i = 0; i < binCount; ++i)
			magnitudes[i] = src[i] * WEB_BIN_SCALE;

		ComputeBands(magnitudes, binCount, bandMagnitudes, NUM_BARS);

		// Smooth: jump up instantly, decay slowly — the classic "VU meter" feel
		for (int b = 0; b < NUM_BARS; ++b)
			bandMagnitudesSmoothed[b] = std::max(bandMagnitudes[b], bandMagnitudesSmoothed[b] * 0.85f);
#else
#ifndef FE_EXCLUDE_KISSFFT
		if (audioSamples.size() < FFT_SIZE || !fftConfig) return;

		for (int i = 0; i < FFT_SIZE; ++i)
			fftInput[i] = audioSamples[i];

		kiss_fftr(fftConfig, fftInput, fftOutput);

		float magnitudes[BINS];
		for (int i = 0; i < BINS; ++i) {
			float real = fftOutput[i].r;
			float imag = fftOutput[i].i;
			magnitudes[i] = std::sqrt(real * real + imag * imag) / FFT_SIZE;
		}

		ComputeBands(magnitudes, BINS, bandMagnitudes, NUM_BARS);

		// Smooth: jump up instantly, decay slowly — the classic "VU meter" feel
		for (int b = 0; b < NUM_BARS; ++b)
			bandMagnitudesSmoothed[b] = std::max(bandMagnitudes[b], bandMagnitudesSmoothed[b] * 0.85f);
#endif
#endif
	}

	// Cross-platform accessors so games can read bins + waveform regardless of
	// the backend (WASAPI/PipeWire FFT on desktop, browser AnalyserNode on web).

	bool IsCapturing() const {
#ifdef __EMSCRIPTEN__
		return g_webAudio.IsCapturing();
#elif defined(_WIN32)
		return g_loopback.IsCapturing();
#else
		return g_pwLoopback.IsCapturing();
#endif
	}

	int GetFrequencyBinCount() const {
#ifdef __EMSCRIPTEN__
		return g_webAudio.GetBinCount();
#else
		return BINS;
#endif
	}

	const float* GetFrequencyMagnitudes() const {
#ifdef __EMSCRIPTEN__
		return g_webAudio.GetMagnitudes();
#else
		return nullptr;
#endif
	}

	int GetTimeDomainCount() const {
#ifdef __EMSCRIPTEN__
		return g_webAudio.GetTimeCount();
#else
		return 0;
#endif
	}

	const float* GetTimeDomainSamples() const {
#ifdef __EMSCRIPTEN__
		return g_webAudio.GetTimeDomain();
#else
		return nullptr;
#endif
	}

};