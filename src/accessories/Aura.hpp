#pragma once

#include <memory>
#include <algorithm>

class Aura {

public:
	Aura();
	~Aura();

	bool IsOpen() const;
	bool SetColor(char r, char g, char b, bool force = false);

	bool auraInitialized = false;

	void SetColorFloat(float r, float g, float b, bool force = false) {
		char ir = (char)(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
		char ig = (char)(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
		char ib = (char)(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);

		SetColor(ir, ig, ib, force);
		
		auraInitialized = true;
	}

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};