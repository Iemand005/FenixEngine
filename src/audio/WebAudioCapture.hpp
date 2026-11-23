#pragma once

// Browser (Emscripten) audio source for the visualiser.
//
// The Web Audio API already performs the FFT in the browser, so we never
// compile kissfft into the wasm. JavaScript hands us the analyser's
// frequency bins and time-domain samples through the exported C functions
// below, and the visualiser reads them back each frame. This mirrors the
// Init/Poll/Shutdown interface of WasapiLoopbackCapture and
// PipeWireLoopbackCapture so AudioVisualiser can treat all three alike.

#include <algorithm>
#include <cstring>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

class WebAudioCapture {
public:
	static constexpr int MAX_BINS = 1024;
	static constexpr int MAX_TIME = 2048;

	bool Init(int = 0, int = 0) {
		std::fill(m_magnitudes, m_magnitudes + MAX_BINS, 0.0f);
		std::fill(m_timeDomain, m_timeDomain + MAX_TIME, 0.0f);
		m_binCount = 0;
		m_timeCount = 0;
		return true;
	}

	void Shutdown() {}

	bool IsCapturing() const { return m_binCount > 0; }

	// Matches the loopback capture interface, but on web the raw samples are
	// produced in the browser; only bins + time domain are meaningful here.
	void Poll(std::vector<float>& /*outSamples*/) {}

	int GetBinCount() const { return m_binCount; }
	int GetTimeCount() const { return m_timeCount; }
	const float* GetMagnitudes() const { return m_magnitudes; }
	const float* GetTimeDomain() const { return m_timeDomain; }

	void SetBins(const float* data, int count) {
		if (!data || count <= 0) {
			m_binCount = 0;
			return;
		}
		count = std::min(count, MAX_BINS);
		std::memcpy(m_magnitudes, data, sizeof(float) * count);
		m_binCount = count;
	}

	void SetTimeDomain(const float* data, int count) {
		if (!data || count <= 0) {
			m_timeCount = 0;
			return;
		}
		count = std::min(count, MAX_TIME);
		std::memcpy(m_timeDomain, data, sizeof(float) * count);
		m_timeCount = count;
	}

private:
	float m_magnitudes[MAX_BINS] = {0.0f};
	float m_timeDomain[MAX_TIME] = {0.0f};
	int m_binCount = 0;
	int m_timeCount = 0;
};

#ifdef __EMSCRIPTEN__
inline WebAudioCapture g_webAudio;

extern "C" {

EMSCRIPTEN_KEEPALIVE void FE_AudioSetFrequencyBins(const float* data, int count) {
	g_webAudio.SetBins(data, count);
}

EMSCRIPTEN_KEEPALIVE void FE_AudioSetTimeDomain(const float* data, int count) {
	g_webAudio.SetTimeDomain(data, count);
}

EMSCRIPTEN_KEEPALIVE int FE_AudioIsCapturing() {
	return g_webAudio.IsCapturing() ? 1 : 0;
}

}
#endif
