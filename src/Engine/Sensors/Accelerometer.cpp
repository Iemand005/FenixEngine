#include "Accelerometer.hpp"

#include <iostream>

using namespace fe;

#ifdef _WIN32

void Accelerometer::EnsureApartment() {
	if (!apartmentInitialized) {
		winrt::init_apartment(winrt::apartment_type::multi_threaded);
		apartmentInitialized = true;
	}
}

Accelerometer::Accelerometer() {
	try {
		EnsureApartment();
		sensor = winrt::Windows::Devices::Sensors::Accelerometer::GetDefault();
		if (sensor) {
			available = true;
			std::cout << "[Accelerometer] Device found" << std::endl;
		} else {
			std::cout << "[Accelerometer] No device available" << std::endl;
		}
	} catch (winrt::hresult_error const& ex) {
		std::cerr << "[Accelerometer] Init failed: "
		          << winrt::to_string(ex.message()) << std::endl;
	}
}

Accelerometer::~Accelerometer() {
	Stop();
}

glm::vec3 Accelerometer::GetAcceleration() const {
	if (!available || !sensor) return glm::vec3(0.0f);

	auto reading = sensor.GetCurrentReading();
	if (reading) {
		std::lock_guard lock(mutex);
		lastReading = glm::vec3(
			static_cast<float>(reading.AccelerationX()),
			static_cast<float>(reading.AccelerationY()),
			static_cast<float>(reading.AccelerationZ())
		);
		return lastReading;
	}
	return glm::vec3(0.0f);
}

void Accelerometer::Start(ReadingCallback callback) {
	if (!available || !sensor || running) return;

	running = true;
	userCallback = std::move(callback);

	token = sensor.ReadingChanged(
		[this](winrt::Windows::Foundation::IInspectable const& sender,
		       winrt::Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs const& args)
	{
		HandleReading(sender, args);
	});

	std::cout << "[Accelerometer] Started" << std::endl;
}

void Accelerometer::Stop() {
	if (!running || !sensor) return;

	sensor.ReadingChanged(token);
	running = false;
	userCallback = nullptr;
	std::cout << "[Accelerometer] Stopped" << std::endl;
}

void Accelerometer::HandleReading(
	winrt::Windows::Foundation::IInspectable const& sender,
	winrt::Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs const& args)
{
	auto reading = args.Reading();
	if (!reading) return;

	{
		std::lock_guard lock(mutex);
		lastReading = glm::vec3(
			static_cast<float>(reading.AccelerationX()),
			static_cast<float>(reading.AccelerationY()),
			static_cast<float>(reading.AccelerationZ())
		);
	}

	if (userCallback) {
		userCallback(lastReading);
	}
}

#else

Accelerometer::Accelerometer() {
	std::cout << "[Accelerometer] Not available on this platform" << std::endl;
}

Accelerometer::~Accelerometer() = default;

glm::vec3 Accelerometer::GetAcceleration() const {
	return glm::vec3(0.0f);
}

void Accelerometer::Start(ReadingCallback callback) {
	(void)callback;
	std::cout << "[Accelerometer] Not available on this platform" << std::endl;
}

void Accelerometer::Stop() {}

#endif
