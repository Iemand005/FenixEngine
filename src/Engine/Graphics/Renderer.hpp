#pragma once
#ifdef FE_EXCLUDE_GLFW
#define GLFW_INCLUDE_NONE
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <glad/glad.h>
#include "../../stdafx.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <type_traits>
#include <array>
#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../engine.h"
#ifndef EXCLUDE_NETWORKING
#include "../networking/networking.hpp"
#endif
// #include "../physics/PhysicsEngine.hpp"
#include "../bases.h"
#include "../Object.hpp"
#include "../Camera.hpp"
#include "../ShaderProgram.hpp"
#include "../ScreenSaverMode.hpp"
#include "../Timer.hpp"
#include "../Scene.hpp"
class Character;

#include "IRenderDevice.hpp"
#include "OpenGLRenderDevice.hpp"
#include "VulkanDevice.hpp"

#include "../window/IWindow.hpp"
#ifndef FE_EXCLUDE_SDL
#include "../window/SDLWindow.hpp"
#endif
#ifndef FE_EXCLUDE_GLFW
#include "../window/GLFW3Window.hpp"
#endif

#ifndef FE_EXCLUDE_SDL
using DefaultWindow = fe::SDLWindow;
#define FE_HAS_WINDOW
#else
#ifndef FE_EXCLUDE_GLFW
using DefaultWindow = fe::GLFW3Window;
#define FE_HAS_WINDOW
#endif
#endif

#define WAYLAND

namespace fe {

	struct RendererOptions : WindowOptions {
		bool useVulkan = true;

		RendererOptions() = default; 

		RendererOptions(int w, int h, bool hidden = false, bool fullscreen = false) : WindowOptions(w, h, hidden, fullscreen) {}
	};

class Renderer {
public:
	std::unique_ptr<IWindow> window = nullptr;
	std::unique_ptr<Scene> scene;
	std::unique_ptr<Camera> camera;
	std::unique_ptr<ShaderProgram> shader;
	fe::Timer fpsCounter;

	std::unique_ptr<IRenderDevice> renderDevice = nullptr;

	float yaw = -90.0f, pitch = 0.0f;

	int lastX, lastY;

	double lastUpdateTime = 0.0f;

	bool canJump = true;

	int mapIndex = 0;

#ifdef USE_VISUALIZER
	AudioVisualiser visualizer;
#endif

#ifndef EXCLUDE_NETWORKING
	std::unique_ptr<Networker> client = nullptr;
#endif

	std::unordered_map<unsigned char, std::shared_ptr<Character>> players = std::unordered_map<unsigned char, std::shared_ptr<Character>>();

	bool isConnectedToServer = false;

	bool useVulkan = false;

	// Shader paths (set via LoadShaders / LoadVulkanShaders before window init)
	std::string vertShaderPath_ = "resources/shaders/VertexShader_vk.spv";
	std::string fragShaderPath_ = "resources/shaders/FragmentShader_vk.spv";
	std::string vertArrayShaderPath_;
	std::string fragArrayShaderPath_;
	std::string vertFoxcraftShaderPath_;
	std::string fragFoxcraftShaderPath_;

	void PushShaderPathsToVulkanDevice() {
		if (!useVulkan || !renderDevice) return;
		auto* vkDev = dynamic_cast<VulkanDevice*>(renderDevice.get());
		if (!vkDev) return;
		vkDev->SetShaderPaths(vertShaderPath_, fragShaderPath_);
		vkDev->SetArrayShaderPaths(vertArrayShaderPath_, fragArrayShaderPath_);
		vkDev->SetFoxcraftShaderPaths(vertFoxcraftShaderPath_, fragFoxcraftShaderPath_);
	}

	Renderer(bool useVulkan = false) {
		CreateRenderDevice(useVulkan);
	}

	template<typename F, typename = std::enable_if_t<std::is_convertible_v<F, GLADloadproc>>>
	Renderer(F loadProc) : Renderer(static_cast<GLADloadproc>(loadProc)) {}

	Renderer(GLADloadproc loadProc);

	Renderer(int width, int height, bool skipInit = false, bool hidden = false, bool fullscreen = false) : Renderer() {
		CreateRenderDevice(false);
#ifdef FE_HAS_WINDOW
		NewWindow(width, height, hidden, fullscreen);// TODO make scrut struct for thes eoptions brudah
#endif
	}

	Renderer(RendererOptions options) {
		CreateRenderDevice(options.useVulkan);
#ifdef FE_HAS_WINDOW
		NewWindow(options.width, options.height, options.hidden, options.fullscreen);
#endif
	}

	void CreateRenderDevice(bool useVulkan = false) {
		if (renderDevice) return; // TODO: throwerror?kaykay

		this->useVulkan = useVulkan;
		if (useVulkan) renderDevice = std::make_unique<VulkanDevice>();
		else renderDevice = std::make_unique<OpenGLRenderDevice>();
	}

#ifdef FE_HAS_WINDOW
	void ActivateScreenSaverMode(ScreenSaverMode mode, void *previewParent = nullptr) {
		auto window = GetWindow<DefaultWindow>();
		switch (mode) {
			case ScreenSaverMode::Preview: {


				window->AttachToNativeParent(previewParent);
				break;
			}

			case ScreenSaverMode::Fullscreen: {
				bool fullscreen = false;
				if (fullscreen)
					window->SetFullscreen();
				else window->GoBorderlessFullscreen();

				// window->Show();

				// SDL_HideCursor();
				// SDL_SetCursor(nullptr);

				window->Show();

				window->ActivateScreenSaverMode();

				window->StartMouseCapture();

				break;
			}

			case ScreenSaverMode::Window: {
				window->Show();
				break;
			}

			case ScreenSaverMode::Config: {
				break;
			}
		}
	}
#endif
	
#ifdef FE_HAS_WINDOW
	void NewWindow(int width, int height, bool hidden = false, bool fullscreen = false) {
		this->window = MakeWindow("Fenix Engine", width, height, hidden, fullscreen, useVulkan);
		PushShaderPathsToVulkanDevice();
		renderDevice->Init(window.get());
	}

	template<typename WindowT = DefaultWindow>
	std::unique_ptr<WindowT> MakeWindow(std::string title, int width, int height, bool hidden = false, bool fullscreen = false, bool useVulkan = false) {
		static_assert(std::is_base_of_v<IWindow, WindowT>, "WindowT must derive from IWindow");
		std::unique_ptr<WindowT> window = std::make_unique<WindowT>(title, width, height, hidden, fullscreen, WindowOptions{}, useVulkan);

		window->resizeEvent = [this](int width, int height) {
			this->Resize(width, height);
		};

		// window->mouseMoveEvent = [this](int x, int y) {
		//   MouseMove(x, y);
		// };
		return std::move(window);
	}
#endif

	void LoadShaders(Shader vertexShader, Shader fragmentShader) {
		this->shader = std::make_unique<fe::ShaderProgram>(vertexShader, fragmentShader);
	}

	void LoadShaders(std::string vertexShaderPath, std::string fragmentShaderPath) {
		this->shader = std::make_unique<fe::ShaderProgram>(vertexShaderPath, fragmentShaderPath);
		// Derive Vulkan SPIR-V paths from the OpenGL paths (VertexShader.glsl -> VertexShader_vk.spv)
		auto vkPath = [](const std::string& glslPath) -> std::string {
			std::string path = glslPath;
			size_t dot = path.rfind('.');
			size_t slash = path.rfind('/');
			if (dot != std::string::npos && slash != std::string::npos && dot > slash) {
				path.insert(dot, "_vk");
				path.replace(dot + 3, std::string::npos, ".spv");
			}
			return path;
		};
		vertShaderPath_ = vkPath(vertexShaderPath);
		fragShaderPath_ = vkPath(fragmentShaderPath);
		PushShaderPathsToVulkanDevice();
	}

	void LoadVulkanShaders(const std::string& vertPath, const std::string& fragPath) {
		vertShaderPath_ = vertPath;
		fragShaderPath_ = fragPath;
		PushShaderPathsToVulkanDevice();
	}

	void LoadArrayShaders(const std::string& vertPath, const std::string& fragPath) {
		vertArrayShaderPath_ = vertPath;
		fragArrayShaderPath_ = fragPath;
		PushShaderPathsToVulkanDevice();
	}

	void LoadFoxcraftShaders(const std::string& vertPath, const std::string& fragPath) {
		vertFoxcraftShaderPath_ = vertPath;
		fragFoxcraftShaderPath_ = fragPath;
		PushShaderPathsToVulkanDevice();
	}

	bool LoadShaderTexts(std::string vertexShaderText, std::string fragmentShaderText) {
		this->shader = std::make_unique<fe::ShaderProgram>();
		return this->shader->LoadShaderTexts(vertexShaderText, fragmentShaderText);
	}

	void SetClearColor(float r, float g, float b, float a = 1) {
		renderDevice->SetClearColor(r, g, b, a);
	}

	void Resize() { Resize(this->window->width, this->window->height); }
	void Resize(int width, int height) {
		// glViewport(0, 0, width, height);
		renderDevice->Resize(width, height);
		this->UpdateAspect(width, height);
	}

	// void Redraw(GLuint fbo) {
	// 	BindFrameBuffer(fbo);
	// 	Redraw();
	// }

	void Clear() { 
		renderDevice->Clear();
	 }

	void RenderMesh(Mesh<>& mesh);
	void RenderObject(Object& object);
	void RenderScene(Scene *scene);
	void RenderScene() { RenderScene(scene.get()); }

	void Redraw() {
#ifdef FE_HAS_WINDOW
		auto window = GetWindow<DefaultWindow>();
#endif
		if (!scene || !camera) return;
		if (!useVulkan && !shader) return;

		Clear();

		if (shader) {
			shader->Use();
#ifdef FE_HAS_WINDOW
			float elapsedTime = (float)window->GetTime();

			shader->SetFloat("time", elapsedTime); // TODO: report time other way (via param?) for embeddded rendering
#endif
			shader->SetMat4("view", camera->GetViewMatrix());
			shader->SetMat4("projection", camera->GetProjectionMatrix());
		}

		renderDevice->SetMat4("view", camera->GetViewMatrix());
		renderDevice->SetMat4("projection", camera->GetProjectionMatrix());

		RenderScene();

		OnDraw();

		DrawUI();

		OnPreSwap();

		renderDevice->SubmitFrame();

		fpsCounter.update();
#ifdef FE_HAS_WINDOW
		if (!useVulkan) window->SwapBuffers();
#endif
	}

	void CheckErrors() {
		CheckErrors("Renderer");
	}

	void CheckErrors(const char* label) {
		if (useVulkan) return;
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR)
			std::cerr << "[GL ERROR] " << label << " -> 0x" << std::hex << err << std::dec << " (" << err << ")" << std::endl;
	}

	void Update() {
		double dt = scene->Update();
	}

	virtual void InitUI() {}
	virtual void DrawUI() {}
	virtual void OnDraw() {}
	virtual void OnPreSwap() {}

	void EnableWireframe();
	void DisableWireframe();
	void ToggleWireframe(bool enabled = false);

	template<typename WindowT = IWindow>
	WindowT* GetWindow() {
		return (WindowT*)this->window.get();
	}

	double GetFPS() {
		return fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0;
	}

	void BindFrameBuffer(int bufferIndex = 0);

	void UpdateAspect(int width, int height) {
		if (this->camera) this->camera->SetAspect(width, height);
	}

	bool ShouldClose() { return this->window->ShouldClose(); }

	void Destroy() {
		if (window) window->Destroy();
	}
};

}
