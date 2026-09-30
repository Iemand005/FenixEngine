#pragma once
#include "Renderer.hpp"

#include "Object.hpp"
#include "Camera.hpp"
#include "Scene.hpp"
#include "ShaderProgram.hpp"
#include "Shader.hpp"
#include "Mesh.hpp"

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

#ifndef EXCLUDE_NETWORKING
#include "networking/networking.hpp"
#endif

#include <algorithm>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL 
#include <glm/gtx/norm.hpp>

using namespace fe;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

static void EmscriptenLoopWrapper(void* arg) {
    auto* engine = static_cast<Renderer*>(arg);
    
    if (engine->ShouldClose()) {
        emscripten_cancel_main_loop();
        engine->Destroy();
        return;
    }
    
    engine->Step();
}
#endif

Renderer::Renderer() = default;

Renderer::~Renderer() = default;

void Renderer::Init(GLADloadproc loadProc) {
	if (!gladLoadGLLoader(loadProc))
		std::cerr << "Failed to load OpenGL functions (GLAD)" << std::endl;
}

void Renderer::PushShaderPathsToVulkanDevice() {
	if (!renderDevice) return;
#ifdef FE_HAS_VULKAN
	PushShaderPathsToDevice(renderDevice.get());
#endif
}

void Renderer::PushShaderPathsToDevice(IRenderDevice* device) {
#ifdef FE_HAS_VULKAN
	auto* vkDev = dynamic_cast<VulkanDevice*>(device);
	if (!vkDev) return;
	vkDev->SetShaderPaths(VertexFormat::Standard, vertShaderPath_, fragShaderPath_);
	vkDev->SetShaderPaths(VertexFormat::Array, vertArrayShaderPath_, fragArrayShaderPath_);
	vkDev->SetShaderPaths(VertexFormat::Packed, vertFoxcraftShaderPath_, fragFoxcraftShaderPath_);
#endif
}

Renderer::Renderer(bool useVulkan) {
	CreateRenderDevice(useVulkan);
}

// Deprecated!!!
Renderer::Renderer(GLADloadproc loadProc) {
	Init(loadProc);
	CreateRenderDevice(false);
}

Renderer::Renderer(int width, int height, bool skipInit, bool hidden, bool fullscreen) : Renderer() {
	CreateRenderDevice(false);
#ifdef FE_HAS_WINDOW
	NewWindow(width, height, hidden, fullscreen);// TODO make scrut struct for thes eoptions brudah
#endif
}

Renderer::Renderer(RendererOptions options) {
	CreateRenderDevice(options.useVulkan);
#ifdef FE_HAS_WINDOW
	NewWindow(options.width, options.height, options.hidden, options.fullscreen, options.useVulkan);
#endif
}

void Renderer::Run() {
    Init();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(EmscriptenLoopWrapper, this, 0, 1);
#else
    while (!ShouldClose())
        Step();
    Destroy();
#endif
}

void Renderer::CreateRenderDevice(bool useVulkan) {
	if (renderDevice) return; // TODO: throwerror?kaykay

	this->useVulkan = useVulkan;
#ifdef FE_HAS_VULKAN
	if (useVulkan) renderDevice = std::make_unique<VulkanDevice>();
	else
#endif
	renderDevice = std::make_unique<OpenGLRenderDevice>();
}

IRenderDevice* Renderer::CreateDevice(bool useVulkan, IWindow *window) {
	if (renderDevice && renderDevice->IsVulkan() == useVulkan) return renderDevice.get();
	for (auto& dev : renderDevices)
		if (dev->IsVulkan() == useVulkan) return dev.get();
	std::unique_ptr<IRenderDevice> dev;
#ifdef FE_HAS_VULKAN
	if (useVulkan) dev = std::make_unique<VulkanDevice>();
	else
#endif
	dev = std::make_unique<OpenGLRenderDevice>();

	if (useVulkan) PushShaderPathsToDevice(dev.get());
	if (window) dev->Init(window);
	IRenderDevice* ptr = dev.get();
	renderDevices.push_back(std::move(dev));
	return ptr;
}

IRenderDevice* Renderer::GetDeviceForWindow(const IWindow* w) {
	auto it = windowDeviceMap.find(w);
	if (it != windowDeviceMap.end()) return it->second;
	return renderDevice.get();
}

#ifdef FE_HAS_WINDOW
void Renderer::ActivateScreenSaverMode(ScreenSaverMode mode, void *previewParent) {
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
#ifndef FE_EXCLUDE_SDL
template<typename WindowT = DefaultWindow>
static std::unique_ptr<WindowT> MakeWindow(Renderer* self, std::string title, int width, int height, bool hidden, bool fullscreen, bool useVulkan, SDL_GLContext sharedContext) {
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

	window->resizeEvent = [self](int width, int height) {
		self->Resize(width, height);
	};

	window->onLiveMoveResize = [self]() {
		self->Redraw();
	};
#ifdef _WIN32
	window->EnableLiveResizePump();
#endif

	// window->mouseMoveEvent = [self](int x, int y) {
	//   self->MouseMove(x, y);
	// };
	return std::move(window);
}
#endif

void Renderer::NewWindow(int width, int height, bool hidden, bool fullscreen) {
	// throw new std::exception("deprecatfucker");
	NewWindow(width, height, hidden, fullscreen, useVulkan);
}

void Renderer::NewWindow(int width, int height, bool hidden, bool fullscreen, bool useVulkan) {
#ifdef __EMSCRIPTEN__
	useVulkan = false;
#endif
#ifndef FE_EXCLUDE_SDL

	SDL_GLContext sharedContext = nullptr;
	if (!useVulkan && !windows.empty()) {
		auto* firstWin = dynamic_cast<SDLWindow*>(windows.front().get());
		if (firstWin) sharedContext = firstWin->GetSDLGLContext();
	}

	auto window = MakeWindow(this, "Fenix Engine", width, height, hidden, fullscreen, useVulkan, sharedContext);
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

#endif


void Renderer::LoadShaders(Shader vertexShader, Shader fragmentShader) {
	this->shader = std::make_unique<fe::ShaderProgram>(vertexShader, fragmentShader);
}

void Renderer::LoadShaders(std::string vertexShaderPath, std::string fragmentShaderPath) {
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

void Renderer::LoadVulkanShaders(const std::string& vertPath, const std::string& fragPath) {
#ifdef FE_HAS_VULKAN
	vertShaderPath_ = vertPath;
	fragShaderPath_ = fragPath;
	PushShaderPathsToVulkanDevice();
#endif
}

void Renderer::LoadArrayShaders(const std::string& vertPath, const std::string& fragPath) {
#ifdef FE_HAS_VULKAN
	vertArrayShaderPath_ = vertPath;
	fragArrayShaderPath_ = fragPath;
	PushShaderPathsToVulkanDevice();
#endif
}

void Renderer::LoadFoxcraftShaders(const std::string& vertPath, const std::string& fragPath) {
#ifdef FE_HAS_VULKAN
	vertFoxcraftShaderPath_ = vertPath;
	fragFoxcraftShaderPath_ = fragPath;
	PushShaderPathsToVulkanDevice();
#endif
}

bool Renderer::LoadShaderTexts(std::string vertexShaderText, std::string fragmentShaderText) {
	this->shader = std::make_unique<fe::ShaderProgram>();
	return this->shader->LoadShaderTexts(vertexShaderText, fragmentShaderText);
}

void Renderer::SetClearColor(float r, float g, float b, float a) {
	clearColorR_ = r; clearColorG_ = g; clearColorB_ = b; clearColorA_ = a;
	renderDevice->SetClearColor(r, g, b, a);
	for (auto& dev : renderDevices) dev->SetClearColor(r, g, b, a);
}

void Renderer::Resize(int width, int height) {
	// glViewport(0, 0, width, height);
	renderDevice->Resize(width, height);
	for (auto& dev : renderDevices) dev->Resize(width, height);
	this->UpdateAspect(width, height);
}

void Renderer::Clear() { 
	renderDevice->Clear();
	for (auto& dev : renderDevices) dev->Clear();
}

void Renderer::SetTransparentMode(bool enabled) {
	renderDevice->SetTransparentMode(enabled);
}

void Renderer::RenderMesh(Mesh<Vertex>& mesh) {
	mesh.SetDevice(renderDevice.get());
	renderDevice->DrawMesh(mesh.gpuBuffers.get(), mesh.gpuTexture.get());
}

void Renderer::RenderObject(Object& object, bool transparentPass) {
	glm::mat4 model = object.GetModelMatrix();
	glm::vec3 modelPos = glm::vec3(model[3]);
	glm::vec3 center = modelPos + object.boundingCenterOffset;
	glm::vec3 toCenter = center - camera->GetPos();
	if (frustumCullingEnabled && glm::dot(toCenter, camera->front) < -object.boundingRadius)
		return;
	renderDevice->SetMat4("model", model);
	renderDevice->SetVec3("objectColor", object.color);
	if (object.reverseWinding) renderDevice->SetFrontFace(false);
	for (auto& mesh : object.meshes) {
		if (mesh->GetHasTransparency() != transparentPass) continue;
		mesh->SetDevice(renderDevice.get());
		renderDevice->DrawMesh(mesh->GetGPUBuffers(), mesh->GetGPUTexture());
	}
	for (auto& child : object.GetChildren())
		RenderObject(*child, transparentPass);
	if (object.reverseWinding) renderDevice->SetFrontFace(true);
}

static bool HasTransparentMesh(const Object& obj) {
	for (auto& mesh : obj.meshes) {
		if (mesh->GetHasTransparency()) return true;
	}
	return false;
}

static void CollectObjects(Object& obj, std::vector<Object*>& out) {
	out.push_back(&obj);
	for (auto& child : obj.GetChildren())
		CollectObjects(*child, out);
}

void Renderer::UploadLights(ShaderProgram* target, Scene* targetScene) {
	static std::vector<std::string> namePos, nameCol, nameInt, nameRad;
	if (namePos.empty()) {
		namePos.reserve(kMaxPointLights);
		nameCol.reserve(kMaxPointLights);
		nameInt.reserve(kMaxPointLights);
		nameRad.reserve(kMaxPointLights);
		for (int i = 0; i < kMaxPointLights; ++i) {
			std::string prefix = "pointLights[" + std::to_string(i) + "].";
			namePos.push_back(prefix + "position");
			nameCol.push_back(prefix + "color");
			nameInt.push_back(prefix + "intensity");
			nameRad.push_back(prefix + "radius");
		}
	}
	if (!target || !targetScene) return;
	int count = std::min(targetScene->GetLightCount(), kMaxPointLights);
	target->SetInt("lightCount", count);
	if (count <= 0) return;
	const auto* lights = targetScene->GetLights();
	for (int i = 0; i < count; ++i) {
		const auto& l = lights[i];
		target->SetVec3(namePos[i], l.position);
		target->SetVec3(nameCol[i], l.color);
		target->SetFloat(nameInt[i], l.intensity);
		target->SetFloat(nameRad[i], std::max(0.001f, l.radius));
	}
}

void Renderer::RenderScene(Scene *scene) {
	if (shader) UploadLights(shader.get(), scene);

	renderDevice->BeginFrame();

	// Opaque pass (depth write ON)
	renderDevice->SetTransparentMode(false);
	for (auto& object : scene->GetObjects()) {
		RenderObject(*object, false);
	}

	// Transparent pass (depth write OFF, sorted back-to-front)
	renderDevice->SetTransparentMode(true);
	{
		transparentScratch_.clear();
		transparentScratch_.reserve(scene->GetObjects().size() * 4);
		for (auto& object : scene->GetObjects())
			CollectObjects(*object, transparentScratch_);
		auto end = std::remove_if(transparentScratch_.begin(), transparentScratch_.end(),
			[](const Object* o) { return !HasTransparentMesh(*o); });
		size_t count = static_cast<size_t>(end - transparentScratch_.begin());
		if (count > 1) {
			glm::vec3 camPos = camera ? camera->GetPos() : glm::vec3(0.0f);
			std::sort(transparentScratch_.begin(), end,
				[&camPos](const Object* a, const Object* b) {
					float da = glm::length2(a->state.position - camPos);
					float db = glm::length2(b->state.position - camPos);
					return da > db;
				});
		}
		for (size_t i = 0; i < count; ++i) {
			Object* obj = transparentScratch_[i];
			glm::mat4 model = obj->GetModelMatrix();
			renderDevice->SetMat4("model", model);
			renderDevice->SetVec3("objectColor", obj->color);
			if (obj->reverseWinding) renderDevice->SetFrontFace(false);
			for (auto& mesh : obj->meshes) {
				if (!mesh->GetHasTransparency()) continue;
				mesh->SetDevice(renderDevice.get());
				renderDevice->DrawMesh(mesh->GetGPUBuffers(), mesh->GetGPUTexture());
			}
			if (obj->reverseWinding) renderDevice->SetFrontFace(true);
		}
	}
	renderDevice->SetTransparentMode(false);
}

void Renderer::RenderScene() {
	RenderScene(scene.get());
}

void Renderer::RenderObjectOnDevice(Object& object, IRenderDevice* dev, bool transparentPass) {
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

void Renderer::RenderAdditionalGLWindows() {
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
			shader->SetFloat("time", elapsedTime); // TODO: report time other way (via param?) for embeddded rendering

			if (scene) UploadLights(shader.get(), scene.get());
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

void Renderer::Redraw() {
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
		UploadLights(shader.get(), scene.get());
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
		auto windows = dev->GetWindows();
		for (auto &window : windows) {
			camera->SetAspect(window->width, window->height);
			dev->SetMat4("view", camera->GetViewMatrix());
			dev->SetMat4("projection", camera->GetProjectionMatrix());
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

void Renderer::CheckErrors() {
	CheckErrors("Renderer");
}

void Renderer::CheckErrors(const char* label) {
	if (useVulkan) return;
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
		std::cerr << "[GL ERROR] " << label << " -> 0x" << std::hex << err << std::dec << " (" << err << ")" << std::endl;
}

void Renderer::EnableWireframe() {
	renderDevice->EnableWireframe();
}

void Renderer::DisableWireframe() {
	renderDevice->DisableWireframe();
}

void Renderer::ToggleWireframe(bool enabled) {
	if (enabled) renderDevice->EnableWireframe();
	else renderDevice->DisableWireframe();
}

void Renderer::SetVSync(bool enabled) {
	vsyncEnabled = enabled;
	if (renderDevice) renderDevice->SetVSync(enabled);
	for (auto& dev : renderDevices) dev->SetVSync(enabled);
}

double Renderer::GetFPS() {
	return fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0;
}

void Renderer::BindFrameBuffer(int bufferIndex) {
	renderDevice->BindFramebuffer(bufferIndex);
}

void Renderer::UpdateAspect(int width, int height) {
	if (this->camera) this->camera->SetAspect(width, height);
}

bool Renderer::ShouldClose() { return this->GetWindow()->ShouldClose(); }

void Renderer::Destroy() {
	for (auto &window : windows)
		window->Destroy();
}
