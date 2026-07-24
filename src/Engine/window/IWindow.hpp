#pragma once

#include <functional>
#include <string>

namespace fe {

	struct VulkanExtensions {
		const char*const*extensions;
		unsigned int extensionCount;
	};

	struct WindowSize {
		int width, height;

		WindowSize() = default; 
    	WindowSize(int w, int h) : width(w), height(h) {}
	};

	// struct WindowOptio

	inline bool IsWayland() {
#ifndef _WIN32
		const char* session = std::getenv("XDG_SESSION_TYPE");
		if (session && strcmp(session, "wayland") == 0)
			return true;

		const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
		if (wayland_display != NULL)
			return true;
#endif
		return false;
	}

	struct WindowOptions : WindowSize {
		bool hidden = false, fullscreen = false, tenBit = true;
		long long int x11WindowId;

		WindowOptions() = default; 

		WindowOptions(int w, int h, bool hidden = false, bool fullscreen = false) : WindowSize(w, h), hidden(hidden), fullscreen(fullscreen) {}
	};


	using ResizeDelegate = std::function<void(int, int)>;
	using MouseMoveDelegate = std::function<void(int, int)>;

	class IWindow {

		bool shouldClose = false;

public:

		int width, height;

		bool _isScreensaving = false;
		double startX, startY;

		bool isFullscreen = false;

		ResizeDelegate resizeEvent;
		MouseMoveDelegate mouseMoveEvent;

		IWindow(int width, int height) : width(width), height(height) {}

		virtual bool ShouldClose() { return shouldClose; }

		virtual void PrepareClose() { shouldClose = true; }

		virtual void SetSwapInterval(int interval) = 0;

		void EnableVSync() {
			SetSwapInterval(1);
		}

		void DisableVSync() {
			SetSwapInterval(0);
		}

		virtual void StartMouseCapture() {}

		virtual void StopMouseCapture() {}

		virtual void SetTitle(const char *newTitle) {};

		bool CapturingMouse() {return false;};

		virtual void GetMousePosition(double *x, double *y) = 0;

		virtual VulkanExtensions GetVulkanExtensions() = 0;
		
		virtual void *CreateVulkanSurface(void *instance) = 0;

		virtual WindowSize GetFramebufferSize() = 0;

		virtual void MakeCurrentGLContext() = 0;

		void ActivateScreenSaverMode() {
			GetMousePosition(&startX, &startY);
			_isScreensaving = true;
		};

		virtual void SetFullscreen(bool enabled) = 0;
		void SetFullscreen() { SetFullscreen(true); }
		void ToggleFullscreen() {
			SetFullscreen(!isFullscreen);
		}

		virtual void GoBorderlessFullscreen() = 0;
		
		virtual void SwapBuffers() = 0;

		virtual double GetTime() = 0;

		using FileDialogCallback = std::function<void(const std::string&)>;
		virtual void OpenFileDialog(FileDialogCallback callback, const char* filterName = "Model Files", const char* filterPattern = "*.glb;*.gltf;*.obj") {}

		virtual void Destroy() {};

	};
}
