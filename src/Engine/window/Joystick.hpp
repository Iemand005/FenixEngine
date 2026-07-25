#pragma once

#include <algorithm>
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

		void Rumble(float strength, Uint32 duration_ms = UINT32_MAX) {
			if (!handle) return;
			Uint16 val = static_cast<Uint16>(std::clamp(strength, 0.0f, 1.0f) * 65535.0f);
			SDL_RumbleJoystick(handle, val, val, duration_ms);
		}

		void StopRumble() {
			Rumble(0.0f, 0);
		}
private:
		SDL_JoystickID id;
		SDL_Joystick* handle = nullptr;
	};
}