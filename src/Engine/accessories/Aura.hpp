#pragma once

#include <memory>

class Aura {

public:
	Aura();
	~Aura();

	bool IsOpen() const;
	bool SetColor(char r, char g, char b);

	BYTE lastR = 0, lastG = 0, lastB = 0;
	bool auraInitialized = false;

	void SetColorRGB(float r, float g, float b)
	{

		BYTE ir = (BYTE)(std::clamp(r, 0.0f, 1.0f) * 255.0f + 0.5f);
		BYTE ig = (BYTE)(std::clamp(g, 0.0f, 1.0f) * 255.0f + 0.5f);
		BYTE ib = (BYTE)(std::clamp(b, 0.0f, 1.0f) * 255.0f + 0.5f);

		if (auraInitialized && ir == lastR && ig == lastG && ib == lastB)
			return;

		aura.SetColor(ir, ig, ib);
		lastR = ir; lastG = ig; lastB = ib;
		auraInitialized = true;
	}

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};