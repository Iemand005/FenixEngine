#pragma once

#include <SDL3/SDL.h>

namespace fe {
	class Joystick {
public:
		Joystick(unsigned int id) {
			joystick = id;
		}
private:
	SDL_JoystickID joystick;
	};
}