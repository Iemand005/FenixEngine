#pragma once

#include <string>

#include <SDL3/SDL.h>

#include <glm/glm.hpp>

namespace fe {
	class Joystick {
public:
		Joystick(unsigned int id) {
			this->id = id;
		}

		~Joystick() {
            if (handle)
                SDL_CloseJoystick(handle);
        }

		std:: string GetName() {
			return SDL_GetJoystickNameForID(id);
		}

		glm::vec2 GetAxis() {
			Sint16 left_x = SDL_GetGamepadAxis(id, SDL_GAMEPAD_AXIS_LEFTX);
			Sint16 left_y = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY);

		}
private:
		SDL_JoystickID id;
		SDL_Joystick* handle;
	};
}