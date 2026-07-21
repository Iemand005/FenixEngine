#pragma once

#include <string>

#include <SDL3/SDL.h>

namespace fe {
	class Joystick {
public:
		Joystick(unsigned int id) {
			this->id = id;
		}

		std:: string GetName() {
			return SDL_GetJoystickNameForID(id);
		}
private:
		SDL_JoystickID id;
	};
}