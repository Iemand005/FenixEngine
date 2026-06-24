
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

};