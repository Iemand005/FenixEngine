#pragma once

#include <memory>

class Aura {

public:
	Aura();
	~Aura();

	bool IsOpen() const;
	bool SetColor(char r, char g, char b);

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};