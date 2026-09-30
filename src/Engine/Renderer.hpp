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

// Only the GLADloader function-pointer type is needed at this level, so the
// typedef is declared instead of pulling in glad/glad.h (110 KB) and the whole
// GL type set. Everything that actually calls into GL now lives in Renderer.cpp.
typedef void* (*GLADloadproc)(const char* name);

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <type_traits>
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Renderer owns instances of these through pointers/references, so forward
// declarations are enough; the headers themselves now stay in Renderer.cpp.
// Moving them out of here is what stops every TU that includes Renderer.hpp
// from also parsing networking (9.7 KB), ShaderProgram, Scene, Camera,
// Object -> Mesh (15.7 KB), the Vulkan device (14.8 KB) and the window backends.
class Networker;

namespace fe {
	class Camera;
	class Character;
	class GLFW3Window;
	class IWindow;
	class IRenderDevice;
	struct Vertex;
	template <typename VertexType> class Mesh;
	class Object;
	class Scene;
	class SDLWindow;
	class Shader;
	class ShaderProgram;
}

#include "window/IWindow.hpp"
#include "Shader.hpp"
#include "ScreenSaverMode.hpp"
#include "Timer.hpp"

// Selects which window backend gets compiled in, and therefore which one the
// engine instantiates by default. FE_HAS_WINDOW gates the window-facing methods
// below; it stays a header concern because it depends only on the FE_EXCLUDE_*
// compile definitions, not on which headers are included.
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

	std::vector<Object*> transparentScratch_;

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

	// Out of line: the members above are unique_ptr/vector-of-unique_ptr over
	// forward-declared types, so destruction needs the complete types, which
	// only Renderer.cpp has.
	virtual ~Renderer();

	Renderer();

	void PushShaderPathsToVulkanDevice();
	void PushShaderPathsToDevice(IRenderDevice* device);

	explicit Renderer(bool useVulkan);

	template<typename F, typename = std::enable_if_t<std::is_convertible_v<F, GLADloadproc>>>
	Renderer(F loadProc) : Renderer(static_cast<GLADloadproc>(loadProc)) {}

	// Deprecated!!!
	explicit Renderer(GLADloadproc loadProc);

	Renderer(int width, int height, bool skipInit = false, bool hidden = false, bool fullscreen = false);
	Renderer(RendererOptions options);

	virtual void Init() {}
    virtual void Step() {}

	void Run();

	void Init(GLADloadproc loadProc);

	void CreateRenderDevice(bool useVulkan = false);

	IRenderDevice* CreateDevice(bool useVulkan, IWindow *window = nullptr);

	IRenderDevice* GetDeviceForWindow(const IWindow* w);

#ifdef FE_HAS_WINDOW
	void ActivateScreenSaverMode(ScreenSaverMode mode, void *previewParent = nullptr);
	void NewWindow(int width, int height, bool hidden = false, bool fullscreen = false);
	void NewWindow(int width, int height, bool hidden, bool fullscreen, bool useVulkan);
#endif

	void LoadShaders(Shader vertexShader, Shader fragmentShader);
	void LoadShaders(std::string vertexShaderPath, std::string fragmentShaderPath);

	void LoadVulkanShaders(const std::string& vertPath, const std::string& fragPath);
	void LoadArrayShaders(const std::string& vertPath, const std::string& fragPath);
	void LoadFoxcraftShaders(const std::string& vertPath, const std::string& fragPath);

	bool LoadShaderTexts(std::string vertexShaderText, std::string fragmentShaderText);

	void SetClearColor(float r, float g, float b, float a = 1);

	void Resize(int width, int height);

	void Clear();

	void SetTransparentMode(bool enabled);

	void RenderMesh(Mesh<Vertex>& mesh);
	void RenderObject(Object& object, bool transparentPass = false);
	void RenderScene(Scene *scene);
	void RenderScene();
	static void UploadLights(ShaderProgram* target, Scene* targetScene);

	void RenderObjectOnDevice(Object& object, IRenderDevice* dev, bool transparentPass);

	void RenderAdditionalGLWindows();

	void Redraw();

	void CheckErrors();
	void CheckErrors(const char* label);

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

	double GetFPS();

	void BindFrameBuffer(int bufferIndex = 0);

	void UpdateAspect(int width, int height);

	bool ShouldClose();

	void Destroy();
};

}
