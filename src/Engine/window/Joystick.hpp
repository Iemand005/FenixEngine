#pragma once

#include <SDL3/SDL.h>
#include <stringx>

namespace fe {
	class Joystick {
public:
		Joystick(unsigned int id) {
			joystick = id;
		}

		std:: string GetName() {
			return SDL_GetJoystickNameForID(joystickIds[i]);
		}
private:
	SDL_JoystickID joystick;
	};
}