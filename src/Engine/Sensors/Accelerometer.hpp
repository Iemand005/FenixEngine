#pragma once

#ifdef _WIN32
#define WINRT_LEAN_AND_MEAN
#define NOMINMAX
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Sensors.h>
#endif

#include <glm/glm.hpp>
#include <functional>
#include <mutex>
#include <atomic>

namespace fe {

	class Accelerometer {
	public:
		using ReadingCallback = std::function<void(const glm::vec3& acceleration)>;

		Accelerometer();
		~Accelerometer();

		Accelerometer(const Accelerometer&) = delete;
		Accelerometer& operator=(const Accelerometer&) = delete;

		bool IsAvailable() const { return available; }

		glm::vec3 GetAcceleration() const;

		void Start(ReadingCallback callback);
		void Stop();

	private:
		std::atomic<bool> available{false};
		mutable std::mutex mutex;
		glm::vec3 lastReading = glm::vec3(0.0f);

#ifdef _WIN32
		void EnsureApartment();
		bool apartmentInitialized = false;

		winrt::Windows::Devices::Sensors::Accelerometer sensor{nullptr};
		winrt::event_token token{};
		std::atomic<bool> running{false};
		ReadingCallback userCallback;

		void HandleReading(
			winrt::Windows::Foundation::IInspectable const& sender,
			winrt::Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs const& args);
#endif
	};

}
