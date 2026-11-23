#pragma once

#include <memory>
#ifdef FE_ACCELEROMETER
#ifdef _WIN32
#define WINRT_LEAN_AND_MEAN
#define NOMINMAX
#define _SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Sensors.h>
#include <winrt/Windows.Devices.Enumeration.h>
#endif
#endif

#include <glm/glm.hpp>
#include <functional>
#include <mutex>
#include <atomic>
#include <string>
#include <vector>

namespace fe {

	class Accelerometer {
	public:
		struct Impl;

		using ReadingCallback = std::function<void(const glm::vec3& acceleration)>;

		Accelerometer();
		~Accelerometer();

		Accelerometer(const Accelerometer&) = delete;
		Accelerometer& operator=(const Accelerometer&) = delete;

		Accelerometer(Accelerometer&& other) noexcept;
		Accelerometer& operator=(Accelerometer&& other) noexcept;

		bool IsAvailable() const { return available; }
		const std::string& GetName() const { return name; }
		const std::string& GetId() const { return id; }

		glm::vec3 GetAcceleration();

		void Calibrate();
		void Start(ReadingCallback callback);
		void Stop();

		static std::vector<Accelerometer> EnumerateAll();

	private:
		std::atomic<bool> available{false};
		mutable std::mutex mutex;
		glm::vec3 lastReading = glm::vec3(0.0f);
		glm::vec3 calibrationOffset = glm::vec3(0.0f);
		std::string name;
		std::string id;

		std::unique_ptr<Impl> impl;

#ifdef FE_ACCELEROMETER
#ifdef _WIN32
		
#endif
#endif
	};

}
