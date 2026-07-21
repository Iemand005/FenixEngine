#pragma once

#include <string>

#include <SDL3/SDL.h>

#include <glm/glm.hpp>

namespace fe {
	class Joystick {
public:
		Joystick(unsigned int id) : id(id) {
			handle = SDL_OpenJoystick(id);
		}

		~Joystick() {
            if (handle) SDL_CloseJoystick(handle);
        }

		Joystick(const Joystick&) = delete;
        Joystick& operator=(const Joystick&) = delete;
        Joystick(Joystick&& other) noexcept : id(other.id), handle(other.handle) {
            other.handle = nullptr;
        }

		Joystick& operator=(Joystick&& other) noexcept {
            if (this != &other) {
                if (handle) SDL_CloseJoystick(handle);
                id = other.id;
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }

		std:: string GetName() const {
			return SDL_GetJoystickNameForID(id);
		}

		glm::vec2 GetAxis() {
			int raw_x = SDL_GetJoystickAxis(handle, 0);
			int raw_y = SDL_GetJoystickAxis(handle, 1);

			value.x = static_cast<float>(raw_x) / 32768.0f;
			value.y = static_cast<float>(raw_y) / 32768.0f;

			return value;
		}
private:
		SDL_JoystickID id;
		SDL_Joystick* handle = nullptr;

		glm::vec2 value;
	};
}