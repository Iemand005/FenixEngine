#include "Accelerometer.hpp"

#include <iostream>
#include <thread>
#include <condition_variable>

using namespace fe;

#ifdef _WIN32

#ifdef FE_ACCELEROMETER
void HandleReading(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs const& args)
{
	auto reading = args.Reading();
	if (!reading) return;

	{
		std::lock_guard lock(mutex);
		lastReading = glm::vec3(
			static_cast<float>(reading.AccelerationX()),
			static_cast<float>(reading.AccelerationY()),
			static_cast<float>(reading.AccelerationZ())
		) - calibrationOffset;
	}

	if (userCallback) {
		userCallback(lastReading);
	}
}
#endif

Accelerometer::Impl {
winrt::Windows::Devices::Sensors::Accelerometer sensor{nullptr};
		winrt::event_token token{};
		std::atomic<bool> running{false};
		ReadingCallback userCallback;

		// void HandleReading(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs const& args);
};

Accelerometer::Accelerometer() {
#ifdef FE_ACCELEROMETER

	try {
		sensor = winrt::Windows::Devices::Sensors::Accelerometer::GetDefault();
		if (sensor) {
			available = true;
			name = "Default Accelerometer";
			std::cout << "[Accelerometer] Device found: " << name << std::endl;
		} else {
			std::cout << "[Accelerometer] No device available" << std::endl;
		}
	} catch (winrt::hresult_error const& ex) {
		std::cerr << "[Accelerometer] Init failed: "
		          << winrt::to_string(ex.message()) << std::endl;
	}
#endif
}

Accelerometer::~Accelerometer() {
	Stop();
}

Accelerometer::Accelerometer(Accelerometer&& other) noexcept
	: available(other.available.load())
	, lastReading(other.lastReading)
	, calibrationOffset(other.calibrationOffset)
	, name(std::move(other.name))
	, id(std::move(other.id))
	// , sensor(std::move(other.sensor))
	// , token(other.token)
	// , running(other.running.load())
	// , userCallback(std::move(other.userCallback))
{
	other.available = false;
#ifdef FE_ACCELEROMETER

	other.running = false;
	other.sensor = nullptr;
#endif
}

Accelerometer& Accelerometer::operator=(Accelerometer&& other) noexcept {
	
	if (this != &other) {
		Stop();
		available = other.available.load();
		lastReading = other.lastReading;
		calibrationOffset = other.calibrationOffset;
		name = std::move(other.name);
		id = std::move(other.id);
		#ifdef FE_ACCELEROMETER
		sensor = std::move(other.sensor);
		running = other.running.load();
		userCallback = std::move(other.userCallback);
		other.available = false;
		other.running = false;
		token = other.token;
		other.sensor = nullptr;
#endif
	}
	return *this;
}

glm::vec3 Accelerometer::GetAcceleration() {
#ifdef FE_ACCELEROMETER

	if (!available || !sensor) return glm::vec3(0.0f);

	auto reading = sensor.GetCurrentReading();
	if (reading) {
		std::lock_guard lock(mutex);
		lastReading.x = static_cast<float>(reading.AccelerationX());
		lastReading.y = static_cast<float>(reading.AccelerationY());
		lastReading.z = static_cast<float>(reading.AccelerationZ());
		return lastReading - calibrationOffset;
	}
#endif
	return glm::vec3(0.0f);
}

void Accelerometer::Calibrate() {
	std::lock_guard lock(mutex);
	calibrationOffset = lastReading;
	std::cout << "[Accelerometer] Calibrated: ("
	          << calibrationOffset.x << ", "
	          << calibrationOffset.y << ", "
	          << calibrationOffset.z << ")" << std::endl;
}

void Accelerometer::Start(ReadingCallback callback) {
	if (!available || !sensor || running) return;

	running = true;
	userCallback = std::move(callback);

#ifdef FE_ACCELEROMETER

	token = sensor.ReadingChanged(
		[this](winrt::Windows::Foundation::IInspectable const& sender,
		       winrt::Windows::Devices::Sensors::AccelerometerReadingChangedEventArgs const& args)
	{
		HandleReading(sender, args);
	});

#endif

	std::cout << "[Accelerometer] Started: " << name << std::endl;
}

void Accelerometer::Stop() {
	if (!running || !sensor) return;
#ifdef FE_ACCELEROMETER

	sensor.ReadingChanged(token);
#endif
	running = false;
	userCallback = nullptr;
	std::cout << "[Accelerometer] Stopped: " << name << std::endl;
}

std::vector<Accelerometer> Accelerometer::EnumerateAll() {
	std::vector<Accelerometer> result;

#ifdef FE_ACCELEROMETER

	// WinRT async .get() requires MTA - spawn a thread with MTA apartment
	std::mutex mtx;
	std::condition_variable cv;
	bool done = false;

	std::thread enumerationThread([&]() {
		winrt::init_apartment(winrt::apartment_type::multi_threaded);

		try {
			winrt::hstring selector = winrt::Windows::Devices::Sensors::Accelerometer::GetDeviceSelector(winrt::Windows::Devices::Sensors::AccelerometerReadingType::Standard);
			winrt::Windows::Devices::Enumeration::DeviceInformationCollection devices =
				winrt::Windows::Devices::Enumeration::DeviceInformation::FindAllAsync(selector).get();

			uint32_t count = devices.Size();
			for (uint32_t i = 0; i < count; i++) {
				winrt::Windows::Devices::Enumeration::DeviceInformation device = devices.GetAt(i);
				winrt::hstring devId = device.Id();
				winrt::hstring devName = device.Name();
				std::string devIdStr = winrt::to_string(devId);
				std::string devNameStr = winrt::to_string(devName);

				try {
					winrt::Windows::Devices::Sensors::Accelerometer sensor =
						winrt::Windows::Devices::Sensors::Accelerometer::FromIdAsync(devId).get();

					if (sensor) {
						Accelerometer acc;
						acc.sensor = sensor;
						acc.available = true;
						acc.name = devNameStr;
						acc.id = devIdStr;
						std::cout << "[Accelerometer] Enumerated: " << devNameStr << " (" << devIdStr << ")" << std::endl;

						std::lock_guard lock(mtx);
						result.push_back(std::move(acc));
					}
				} catch (winrt::hresult_error const& ex) {
					std::cerr << "[Accelerometer] Failed to open " << devNameStr << ": "
					          << winrt::to_string(ex.message()) << std::endl;
				}
			}
		} catch (winrt::hresult_error const& ex) {
			std::cerr << "[Accelerometer] Enumeration failed: "
			          << winrt::to_string(ex.message()) << std::endl;
		}

		std::lock_guard lock(mtx);
		done = true;
		cv.notify_one();
	});

	{
		std::unique_lock lock(mtx);
		cv.wait(lock, [&] { return done; });
	}
	enumerationThread.join();

	if (result.empty()) {
		std::cout << "[Accelerometer] No devices found via enumeration" << std::endl;

		// Fallback to default
		Accelerometer def;
		if (def.IsAvailable()) {
			result.push_back(std::move(def));
		}
	}
#endif

	return result;
}

#else

Accelerometer::Accelerometer() {
	std::cout << "[Accelerometer] Not available on this platform" << std::endl;
}

Accelerometer::~Accelerometer() = default;

Accelerometer::Accelerometer(Accelerometer&&) noexcept = default;
Accelerometer& Accelerometer::operator=(Accelerometer&&) noexcept = default;

glm::vec3 Accelerometer::GetAcceleration() {
	return glm::vec3(0.0f);
}

void Accelerometer::Calibrate() {}

void Accelerometer::Start(ReadingCallback callback) {
	(void)callback;
	std::cout << "[Accelerometer] Not available on this platform" << std::endl;
}

void Accelerometer::Stop() {}

std::vector<Accelerometer> Accelerometer::EnumerateAll() {
	return {};
}

#endif
