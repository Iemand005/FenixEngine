#pragma once
#include <exception>
#include <stdexcept>
#ifdef FE_EXCLUDE_GLFW
#define GLFW_INCLUDE_NONE
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <glad/glad.h>
// #include "../stdafx.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <type_traits>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#ifndef EXCLUDE_NETWORKING
#include "networking/networking.hpp"
#endif

#include "Object.hpp"
#include "Camera.hpp"
#include "ShaderProgram.hpp"
#include "ScreenSaverMode.hpp"
#include "Timer.hpp"
#include "Scene.hpp"
class Character;

#include "Graphics/IRenderDevice.hpp"
#include "Graphics/OpenGLRenderDevice.hpp"
#ifdef FE_HAS_VULKAN
#include "Graphics/VulkanDevice.hpp"
#endif

#include "window/IWindow.hpp"
#ifndef FE_EXCLUDE_SDL
#include "window/SDLWindow.hpp"
#endif
#ifndef FE_EXCLUDE_GLFW
#include "window/GLFW3Window.hpp"
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
#ifndef __EMSCRIPTEN__
		bool useVulkan = true;
		#else
		bool useVulkan = false;
#endif

		GLADloadproc loadProc = nullptr;

		RendererOptions() = default; 

		RendererOptions(int w, int h, bool hidden = false, bool fullscreen = false) : WindowOptions(w, h, hidden, fullscreen) {}
		RendererOptions(int w, int h, bool useVulkan, bool hidden = false, bool fullscreen = false) : WindowOptions(w, h, hidden, fullscreen), useVulkan(useVulkan) {}
	};

class Renderer {
public:
	std::vector<std::unique_ptr<IWindow>> windows;
	std::unique_ptr<Scene> scene;
	std::unique_ptr<Camera> camera;
	std::unique_ptr<ShaderProgram> shader;
	fe::Timer fpsCounter;

	std::unique_ptr<IRenderDevice> renderDevice = nullptr;
	std::vector<std::unique_ptr<IRenderDevice>> renderDevices;
	std::unordered_map<const IWindow*, IRenderDevice*> windowDeviceMap;

	float yaw = -90.0f, pitch = 0.0f;

	float clearColorR_ = 0.0f, clearColorG_ = 0.0f, clearColorB_ = 0.0f, clearColorA_ = 1.0f;

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
	bool frustumCullingEnabled = false;
	bool vsyncEnabled = true;

	// Shader paths (set via LoadShaders / LoadVulkanShaders before window init)
	std::string vertShaderPath_ = "resources/shaders/VertexShader_vk.spv";
	std::string fragShaderPath_ = "resources/shaders/FragmentShader_vk.spv";
	std::string vertArrayShaderPath_;
	std::string fragArrayShaderPath_;
	std::string vertFoxcraftShaderPath_;
	std::string fragFoxcraftShaderPath_;

	void PushShaderPathsToVulkanDevice() {
		if (!renderDevice) return;
#ifdef FE_HAS_VULKAN
		PushShaderPathsToDevice(renderDevice.get());
#endif
	}

	void PushShaderPathsToDevice(IRenderDevice* device) {
#ifdef FE_HAS_VULKAN
		auto* vkDev = dynamic_cast<VulkanDevice*>(device);
		if (!vkDev) return;
		vkDev->SetShaderPaths(vertShaderPath_, fragShaderPath_);
		vkDev->SetArrayShaderPaths(vertArrayShaderPath_, fragArrayShaderPath_);
		vkDev->SetFoxcraftShaderPaths(vertFoxcraftShaderPath_, fragFoxcraftShaderPath_);
#endif
	}

	Renderer(bool useVulkan = false) {
		CreateRenderDevice(useVulkan);
	}

	template<typename F, typename = std::enable_if_t<std::is_convertible_v<F, GLADloadproc>>>
	Renderer(F loadProc) : Renderer(static_cast<GLADloadproc>(loadProc)) {}

	// Deprecated!!!
	Renderer(GLADloadproc loadProc) {
		Init(loadProc);
		CreateRenderDevice(false);
	}

	Renderer(int width, int height, bool skipInit = false, bool hidden = false, bool fullscreen = false) : Renderer() {
		CreateRenderDevice(false);
#ifdef FE_HAS_WINDOW
		NewWindow(width, height, hidden, fullscreen);// TODO make scrut struct for thes eoptions brudah
#endif
	}

	Renderer(RendererOptions options) {
		CreateRenderDevice(options.useVulkan);
#ifdef FE_HAS_WINDOW
		NewWindow(options.width, options.height, options.hidden, options.fullscreen, options.useVulkan);
#endif
		
	}

	void Init(GLADloadproc loadProc);

	void CreateRenderDevice(bool useVulkan = false) {
		if (renderDevice) return; // TODO: throwerror?kaykay

		this->useVulkan = useVulkan;
#ifdef FE_HAS_VULKAN
		if (useVulkan) renderDevice = std::make_unique<VulkanDevice>();
		else renderDevice = std::make_unique<OpenGLRenderDevice>();
#else
		renderDevice = std::make_unique<OpenGLRenderDevice>();
#endif
	}

	IRenderDevice* CreateDevice(bool useVulkan, IWindow *window = nullptr) {
		if (renderDevice && renderDevice->IsVulkan() == useVulkan) return renderDevice.get();
		for (auto& dev : renderDevices) {
			if (dev->IsVulkan() == useVulkan) return dev.get();
		}
		std::unique_ptr<IRenderDevice> dev;
		if (useVulkan) {
#ifdef FE_HAS_VULKAN
			dev = std::make_unique<VulkanDevice>();
#else
			dev = std::make_unique<OpenGLRenderDevice>();
#endif
		} else {
			dev = std::make_unique<OpenGLRenderDevice>();
		}
		if (useVulkan) PushShaderPathsToDevice(dev.get());
		if (window) dev->Init(window);
		IRenderDevice* ptr = dev.get();
		renderDevices.push_back(std::move(dev));
		return ptr;
	}

	IRenderDevice* GetDeviceForWindow(const IWindow* w) {
		auto it = windowDeviceMap.find(w);
		if (it != windowDeviceMap.end()) return it->second;
		return renderDevice.get();
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
		// throw new std::exception("deprecatfucker");
		NewWindow(width, height, hidden, fullscreen, useVulkan);
	}

	void NewWindow(int width, int height, bool hidden, bool fullscreen, bool useVulkan) {
#ifdef __EMSCRIPTEN__
		useVulkan = false;
#endif
#ifndef FE_EXCLUDE_SDL

		SDL_GLContext sharedContext = nullptr;
		if (!useVulkan && !windows.empty()) {
			auto* firstWin = dynamic_cast<SDLWindow*>(windows.front().get());
			if (firstWin) sharedContext = firstWin->GetSDLGLContext();
		}

		auto window = MakeWindow("Fenix Engine", width, height, hidden, fullscreen, useVulkan, sharedContext);
		window->UnbindGLContext();
#else
		auto window = std::make_unique<DefaultWindow>("Fenix Engine", width, height, hidden, fullscreen, WindowOptions{}, useVulkan);
#endif

		IRenderDevice* device = nullptr;
		if (windowDeviceMap.empty()) {
			if (!renderDevice) {
				CreateRenderDevice(useVulkan);
			}
			device = renderDevice.get();
			PushShaderPathsToDevice(device);
			device->Init(window.get());
		} else {
			device = CreateDevice(useVulkan, window.get());
			if (useVulkan) {
				PushShaderPathsToDevice(device);
				// device->RegisterWindow(window.get());
			} else {
				device->RegisterWindow(window.get());
			}
		}
		windowDeviceMap[window.get()] = device;
		windows.push_back(std::move(window));
	}
#ifndef FE_EXCLUDE_SDL

	template<typename WindowT = DefaultWindow>
	std::unique_ptr<WindowT> MakeWindow(std::string title, int width, int height, bool hidden = false, bool fullscreen = false, bool useVulkan = false, SDL_GLContext sharedContext = nullptr) {
		static_assert(std::is_base_of_v<IWindow, WindowT>, "WindowT must derive from IWindow");
		std::unique_ptr<WindowT> window;
		if constexpr (std::is_same_v<WindowT, SDLWindow>) {
			if (!useVulkan) {
				window = std::make_unique<WindowT>(title, width, height, hidden, fullscreen, WindowOptions{}, useVulkan, sharedContext);
			} else {
				window = std::make_unique<WindowT>(title, width, height, hidden, fullscreen, WindowOptions{}, useVulkan);
			}
		} else {
			window = std::make_unique<WindowT>(title, width, height, hidden, fullscreen, WindowOptions{}, useVulkan);
		}

		window->resizeEvent = [this](int width, int height) {
			this->Resize(width, height);
		};

		// window->mouseMoveEvent = [this](int x, int y) {
		//   MouseMove(x, y);
		// };
		return std::move(window);
	}
#endif

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
#ifdef FE_HAS_VULKAN
		vertShaderPath_ = vertPath;
		fragShaderPath_ = fragPath;
		PushShaderPathsToVulkanDevice();
#endif
	}

	void LoadArrayShaders(const std::string& vertPath, const std::string& fragPath) {
#ifdef FE_HAS_VULKAN
		vertArrayShaderPath_ = vertPath;
		fragArrayShaderPath_ = fragPath;
		PushShaderPathsToVulkanDevice();
#endif
	}

	void LoadFoxcraftShaders(const std::string& vertPath, const std::string& fragPath) {
#ifdef FE_HAS_VULKAN
		vertFoxcraftShaderPath_ = vertPath;
		fragFoxcraftShaderPath_ = fragPath;
		PushShaderPathsToVulkanDevice();
#endif
	}

	bool LoadShaderTexts(std::string vertexShaderText, std::string fragmentShaderText) {
		this->shader = std::make_unique<fe::ShaderProgram>();
		return this->shader->LoadShaderTexts(vertexShaderText, fragmentShaderText);
	}

	void SetClearColor(float r, float g, float b, float a = 1) {
		clearColorR_ = r; clearColorG_ = g; clearColorB_ = b; clearColorA_ = a;
		renderDevice->SetClearColor(r, g, b, a);
		for (auto& dev : renderDevices) dev->SetClearColor(r, g, b, a);
	}

	void Resize() {
		throw std::runtime_error("Please implement");
		// Resize(this->window->width, this->window->height);
	}
	void Resize(int width, int height) {
		// glViewport(0, 0, width, height);
		renderDevice->Resize(width, height);
		for (auto& dev : renderDevices) dev->Resize(width, height);
		this->UpdateAspect(width, height);
	}

	// void Redraw(GLuint fbo) {
	// 	BindFrameBuffer(fbo);
	// 	Redraw();
	// }

	void Clear() { 
		renderDevice->Clear();
		for (auto& dev : renderDevices) dev->Clear();
	 }

	void SetTransparentMode(bool enabled) {
		renderDevice->SetTransparentMode(enabled);
	}

	void RenderMesh(Mesh<>& mesh);
	void RenderObject(Object& object, bool transparentPass = false);
	void RenderScene(Scene *scene);
	void RenderScene() { RenderScene(scene.get()); }

	void RenderObjectOnDevice(Object& object, IRenderDevice* dev, bool transparentPass) {
		if (frustumCullingEnabled) {
			glm::vec3 modelPos = glm::vec3(object.GetModelMatrix()[3]);
			glm::vec3 center = modelPos + object.boundingCenterOffset;
			glm::vec3 toCenter = center - camera->GetPos();
			if (glm::dot(toCenter, camera->front) < -object.boundingRadius)
				return;
		}
		glm::mat4 model = object.GetModelMatrix();
		dev->SetMat4("model", model);
		dev->SetVec3("objectColor", object.color);
		if (object.reverseWinding) dev->SetFrontFace(false);
		for (auto& mesh : object.meshes) {
			if (mesh->GetHasTransparency() != transparentPass) continue;
			dev->DrawMesh(mesh->GetGPUBuffersFor(dev), mesh->GetGPUTextureFor(dev));
		}
		if (object.reverseWinding) dev->SetFrontFace(true);

		for (auto& child : object.GetChildren())
			RenderObjectOnDevice(*child, dev, transparentPass);
	}

	void RenderAdditionalGLWindows() {
#ifdef FE_HAS_WINDOW
		IWindow* primaryWindow = windows.front().get();
		for (auto& [w, d] : windowDeviceMap) {
			if (d->IsVulkan()) continue;
			if (w == primaryWindow) continue;

			w->MakeCurrentGLContext();

			// d->SetActiveWindow(w);

			int vw = 0, vh = 0;
			// w->GetSize(&vw, &vh);
			if (vw > 0 && vh > 0)
				d->Resize(vw, vh);

			d->EnableDepthTest();
			d->EnableFaceCulling();

			d->SetClearColor(clearColorR_, clearColorG_, clearColorB_, clearColorA_);
			d->Clear();

			glm::mat4 perWindowProj = camera->GetProjectionMatrix();
			if (vw > 0 && vh > 0)
				perWindowProj = glm::perspective(glm::radians(camera->fov), (float)vw / (float)vh, camera->nearDist, camera->farDist);

			if (shader) {
				shader->Use();
				float elapsedTime = (float)GetWindow()->GetTime();
				shader->SetFloat("time", elapsedTime);

				if (scene) {
					int count = scene->GetLightCount();
					auto pointLights = scene->GetLights();
					shader->SetInt("lightCount", count);
					for (int i = 0; i < count; ++i) {
						const auto& l = pointLights[i];
						shader->SetVec3("pointLights[" + std::to_string(i) + "].position", l.position);
						shader->SetVec3("pointLights[" + std::to_string(i) + "].color", l.color);
						shader->SetFloat("pointLights[" + std::to_string(i) + "].intensity", l.intensity);
						shader->SetFloat("pointLights[" + std::to_string(i) + "].radius", std::max(0.001f, l.radius));
					}
				}

				shader->SetMat4("view", camera->GetViewMatrix());
				shader->SetMat4("projection", perWindowProj);
			}

			d->SetMat4("view", camera->GetViewMatrix());
			d->SetMat4("projection", perWindowProj);

			d->BeginFrame();

			d->SetTransparentMode(false);
			for (auto& object : scene->GetObjects())
				RenderObjectOnDevice(*object, d, false);

			d->SetTransparentMode(true);
			for (auto& object : scene->GetObjects())
				RenderObjectOnDevice(*object, d, true);

			d->SetTransparentMode(false);
			d->SubmitFrame();
			w->SwapBuffers();
		}
		auto* primaryDev = GetDeviceForWindow(primaryWindow);
		if (primaryDev) primaryDev->SetActiveWindow(primaryWindow);
#endif
	}

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

		scene->SetRenderDevice(renderDevice.get());
		scene->SetCameraMatrices(camera->GetViewMatrix(), camera->GetProjectionMatrix());

		RenderScene();

		OnDraw();

		DrawUI();

		OnPreSwap();

		renderDevice->SubmitFrame();
		for (auto& dev : renderDevices) {
			// if (dev->IsVulkan()) dev->SubmitFrame();
			// camera.
			auto windows = dev->GetWindows();
			for (auto &window : windows) {
				camera->SetAspect(window->width, window->height);
				renderDevice->SetMat4("view", camera->GetViewMatrix());
				renderDevice->SetMat4("projection", camera->GetProjectionMatrix());
				dev->SubmitFrame(window);
			}
		}

		fpsCounter.update();
#ifdef FE_HAS_WINDOW
		// if (!useVulkan)
		window->SwapBuffers();
#endif

		RenderAdditionalGLWindows();
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

	void SetVSync(bool enabled);

	template<typename WindowT = IWindow>
	WindowT* GetWindow() {
		return (WindowT*)this->windows.front().get();
	}

	double GetFPS() {
		return fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0;
	}

	void BindFrameBuffer(int bufferIndex = 0);

	void UpdateAspect(int width, int height) {
		if (this->camera) this->camera->SetAspect(width, height);
	}

	bool ShouldClose() { return this->GetWindow()->ShouldClose(); }

	void Destroy() {
		for (auto &window : windows)
			window->Destroy();
	}
};

}
