#pragma once
#ifdef WIN32
#define WINRT_LEAN_AND_MEAN
#define _AMD64_
#include <winrt/impl/Windows.Devices.Sensors.2.h>
#include <winrt/Windows.Devices.Sensors.h>

using namespace winrt;
#endif

namespace fe {
	class Accelerometer {
		Windows::Devices::Sensors::Accelerometer accelerometer;

	};
};