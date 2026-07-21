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
			if (!handle) return glm::vec2(0.0f);

			return glm::vec2(SDL_GetJoystickAxis(handle, 0), SDL_GetJoystickAxis(handle, 1)) / 32768.0f;
		}
private:
		SDL_JoystickID id;
		SDL_Joystick* handle = nullptr;
	};
}