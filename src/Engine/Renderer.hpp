#pragma once
#ifdef FE_EXCLUDE_GLFW
#define GLFW_INCLUDE_NONE
#endif
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <glad/glad.h>
#include "../stdafx.h"

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

#include "engine.h"
#ifndef EXCLUDE_NETWORKING
#include "networking/networking.hpp"
#endif
#include "physics/PhysicsEngine.hpp"
#include "bases.h"
#include "Object.hpp"
#include "Camera.hpp"
#include "ShaderProgram.hpp"
#include "ScreenSaverMode.hpp"

#include "window/IWindow.hpp"
#ifndef FE_EXCLUDE_SDL
#include "window/SDLWindow.hpp"
#endif
#ifndef FE_EXCLUDE_GLFW
#include "window/GLFW3Window.hpp"
#endif

#ifndef FE_EXCLUDE_SDL
using DefaultWindow = fe::SDLWindow;
#else
using DefaultWindow = fe::GLFW3Window;
#endif

#define WAYLAND

namespace fe {

class Renderer {
 public:
  std::unique_ptr<IWindow> window = nullptr;
  std::unique_ptr<Scene> scene;
  std::unique_ptr<Camera> camera;
  std::unique_ptr<ShaderProgram> shader;
  fe::Timer fpsCounter;

  float yaw = -90.0f, pitch = 0.0f;

  int lastX, lastY;

  double lastUpdateTime = 0.0f;

  bool canJump = true;

  int mapIndex = 0;

#ifndef EXCLUDE_NETWORKING
  std::unique_ptr<Networker> client = nullptr;
#endif

  std::unordered_map<unsigned char, std::shared_ptr<Character>> players =
      std::unordered_map<unsigned char, std::shared_ptr<Character>>();

  bool isConnectedToServer = false;

  Renderer() {}

  template<typename F, typename = std::enable_if_t<std::is_convertible_v<F, GLADloadproc>>>
  Renderer(F loadProc) : Renderer(static_cast<GLADloadproc>(loadProc)) {
    //Init();
  }

  Renderer(GLADloadproc loadProc);

	Renderer(int width, int height, bool skipInit = false, bool hidden = false, bool fullscreen = false) : Renderer() {
		NewWindow(width, height, hidden, fullscreen);// TODO make scrut struct for thes eoptions brudah
	}


  void ActivateScreenSaverMode(ScreenSaverMode mode, void *previewParent = nullptr) {
		auto window = GetWindow<DefaultWindow>();
		switch (mode) {
			case ScreenSaverMode::Preview: {


				window->AttachToNativeParent(previewParent);
				break;
			}

			case ScreenSaverMode::Fullscreen: {
				// window->GoBorderlessFullscreen();
        window->SetFullscreen();

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
	
	void NewWindow(int width, int height, bool hidden = false, bool fullscreen = false) {
		this->window = MakeWindow("Fenix Engine", width, height, hidden, fullscreen);
	}

template<typename WindowT = DefaultWindow>
  std::unique_ptr<WindowT> MakeWindow(std::string title, int width, int height, bool hidden = false, bool fullscreen = false) {
    static_assert(std::is_base_of_v<IWindow, WindowT>, "WindowT must derive from IWindow");
    std::unique_ptr<WindowT> window = std::make_unique<WindowT>(title, width, height, hidden, fullscreen);

    window->resizeEvent = [this](int width, int height) {
      this->Resize(width, height);
      this->Redraw();
    };

    // window->mouseMoveEvent = [this](int x, int y) {
    //   MouseMove(x, y);
    // };
    return std::move(window);
  }


  // void SetShaderProgram(ShaderProgram program) {
  //   this->shader = std::make_unique
  // }

  void LoadShaders(Shader vertexShader, Shader fragmentShader) {
    this->shader = std::make_unique<fe::ShaderProgram>(vertexShader, fragmentShader);
  }

  void LoadShaders(std::string vertexShaderPath, std::string fragmentShaderPath) {
    this->shader = std::make_unique<fe::ShaderProgram>(vertexShaderPath, fragmentShaderPath);
  }

  bool LoadShaderTexts(std::string vertexShaderText, std::string fragmentShaderText) {
    this->shader = std::make_unique<fe::ShaderProgram>();
    return this->shader->LoadShaderTexts(vertexShaderText, fragmentShaderText);
  }

  double getDeltaTime() { return 1; }

  void SetClearColor(float r, float g, float b, float a = 1);

  void Resize() { Resize(this->window->width, this->window->height); }
  void Resize(int width, int height) {
    if (this->scene) this->scene->Resize(width, height);
    this->UpdateAspect(width, height);
  }

  void Redraw(GLuint fbo) {
    BindFrameBuffer(fbo);
    Redraw();
  }

  void Redraw() {
    auto window = GetWindow<DefaultWindow>();
    if (!scene || !camera || !shader) return;

    if (shader) {
      shader->Use();

      float elapsedTime = (float)window->GetTime();
      shader->SetFloat("time", elapsedTime);

      std::cout << "Time: " << elapsedTime << " Wobble: " << 2.0f << std::endl;

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

    scene->Render(*this->shader, *this->camera.get());

    OnDraw();

    CheckErrors();

    glFlush();
    glFinish();

    fpsCounter.update();

    DrawUI();

    if (window) window->SwapBuffers();
  }

  void CheckErrors() {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
      std::cerr << "OpenGL error: " << err << std::endl;
    }
  }

  void Update() {
    double dt = scene->Update();
  }

  virtual void InitUI() {}
  virtual void DrawUI() {}
  virtual void OnDraw() {}

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
