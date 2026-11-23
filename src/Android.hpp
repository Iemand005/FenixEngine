#pragma once

#include <string>

namespace fe {

	// Returns the base directory the engine treats as its working directory.
	// - Desktop: the executable's directory (so relative paths like
	//   "resources/..." resolve next to the binary)
	// - Android: the app's internal storage directory (where assets are extracted)
	std::string EngineResourceBaseDir();

	// On Android, copies the given asset subdirectory (relative to the APK's
	// "assets" root) into internal storage and chdir()s there, so that relative
	// reads like "resources/shaders/foo.glsl" work exactly like on desktop.
	// On every other platform this is a no-op. Returns true on success.
	// Safe to call multiple times (only extracts once).
	bool AndroidSetupAssets(const std::string& assetDir = "resources");

} // namespace fe