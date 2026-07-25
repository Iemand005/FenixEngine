#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <glm/glm.hpp>

namespace fe {
	class Joystick {
public:
		Joystick(unsigned int id) : id(id) {
			handle = SDL_OpenJoystick(id);
			if (handle && SDL_IsJoystickHaptic(handle)) {
				haptic = SDL_OpenHapticFromJoystick(handle);
				if (haptic) hapticFeatures = SDL_GetHapticFeatures(haptic);
			}
		}

		~Joystick() {
			for (auto eid : effectIds) SDL_DestroyHapticEffect(haptic, eid);
			if (haptic) SDL_CloseHaptic(haptic);
			if (handle) SDL_CloseJoystick(handle);
		}

		Joystick(const Joystick&) = delete;
		Joystick& operator=(const Joystick&) = delete;

		Joystick(Joystick&& other) noexcept
			: id(other.id), handle(other.handle), haptic(other.haptic),
			  hapticFeatures(other.hapticFeatures), effectIds(std::move(other.effectIds)),
			  constEffectId(other.constEffectId), periodicEffectId(other.periodicEffectId),
			  springEffectId(other.springEffectId) {
			other.handle = nullptr;
			other.haptic = nullptr;
			other.constEffectId = -1;
			other.periodicEffectId = -1;
			other.springEffectId = -1;
		}

		Joystick& operator=(Joystick&& other) noexcept {
			if (this != &other) {
				for (auto eid : effectIds) SDL_DestroyHapticEffect(haptic, eid);
				if (haptic) SDL_CloseHaptic(haptic);
				if (handle) SDL_CloseJoystick(handle);
				id = other.id;
				handle = other.handle;
				haptic = other.haptic;
				hapticFeatures = other.hapticFeatures;
				effectIds = std::move(other.effectIds);
				constEffectId = other.constEffectId;
				periodicEffectId = other.periodicEffectId;
				springEffectId = other.springEffectId;
				other.handle = nullptr;
				other.haptic = nullptr;
				other.constEffectId = -1;
				other.periodicEffectId = -1;
				other.springEffectId = -1;
			}
			return *this;
		}

		std::string GetName() const {
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

		void StopRumble() { Rumble(0.0f, 0); }

		bool IsHaptic() const { return haptic != nullptr; }
		Uint32 GetHapticFeatures() const { return hapticFeatures; }

		SDL_HapticEffectID CreateEffect(const SDL_HapticEffect& effect) {
			if (!haptic) return -1;
			SDL_HapticEffectID eid = SDL_CreateHapticEffect(haptic, &effect);
			if (eid >= 0) effectIds.push_back(eid);
			return eid;
		}

		bool RunEffect(SDL_HapticEffectID eid, Uint32 iterations = SDL_HAPTIC_INFINITY) {
			if (!haptic) return false;
			return SDL_RunHapticEffect(haptic, eid, iterations);
		}

		bool UpdateEffect(SDL_HapticEffectID eid, const SDL_HapticEffect& effect) {
			if (!haptic) return false;
			return SDL_UpdateHapticEffect(haptic, eid, &effect);
		}

		bool StopEffect(SDL_HapticEffectID eid) {
			if (!haptic) return false;
			return SDL_StopHapticEffect(haptic, eid);
		}

		void DestroyEffect(SDL_HapticEffectID eid) {
			if (!haptic) return;
			SDL_DestroyHapticEffect(haptic, eid);
			auto it = std::find(effectIds.begin(), effectIds.end(), eid);
			if (it != effectIds.end()) effectIds.erase(it);
		}

		SDL_HapticEffectID CreateConstantEffect(Sint16 level, const SDL_HapticDirection& dir, Uint32 length_ms = SDL_HAPTIC_INFINITY) {
			SDL_HapticEffect effect{};
			effect.type = SDL_HAPTIC_CONSTANT;
			effect.constant.direction = dir;
			effect.constant.length = length_ms;
			effect.constant.level = level;
			return CreateEffect(effect);
		}

		SDL_HapticEffectID CreatePeriodicEffect(Uint16 type, Sint16 magnitude, Uint16 period, Uint32 length_ms = SDL_HAPTIC_INFINITY) {
			SDL_HapticEffect effect{};
			effect.type = type;
			effect.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
			effect.periodic.direction.dir[0] = 1;
			effect.periodic.length = length_ms;
			effect.periodic.period = period;
			effect.periodic.magnitude = magnitude;
			return CreateEffect(effect);
		}

		SDL_HapticEffectID CreateSpringEffect(Sint16 right_coeff, Sint16 left_coeff, Uint32 length_ms = SDL_HAPTIC_INFINITY) {
			SDL_HapticEffect effect{};
			effect.type = SDL_HAPTIC_SPRING;
			effect.condition.length = length_ms;
			effect.condition.right_coeff[0] = right_coeff;
			effect.condition.left_coeff[0] = left_coeff;
			effect.condition.right_sat[0] = 0xFFFF;
			effect.condition.left_sat[0] = 0xFFFF;
			return CreateEffect(effect);
		}
private:
		SDL_JoystickID id;
		SDL_Joystick* handle = nullptr;
		SDL_Haptic* haptic = nullptr;
		Uint32 hapticFeatures = 0;
		std::vector<SDL_HapticEffectID> effectIds;
	public:
		SDL_HapticEffectID constEffectId = -1;
		SDL_HapticEffectID periodicEffectId = -1;
		SDL_HapticEffectID springEffectId = -1;
	};
}