#pragma once
#include "window/Joystick.hpp"
#define GLFW_INCLUDE_NONE
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>


#ifndef EXCLUDE_NETWORKING
#include "networking/networking.hpp"
#endif
#include "Character.hpp"
#include "Object.hpp"
#include "ShaderProgram.hpp"
#include "bases.h"
#include "saver/Level.hpp"

#define WAYLAND

#include "Renderer.hpp"

#include "physics/PhysicsFactory.hpp"

namespace fe {

class Game : public Renderer {
   public:
	int lastX, lastY;

	std::shared_ptr<Character> player;

	std::vector<std::shared_ptr<Character>> npcs = std::vector<std::shared_ptr<Character>>();

	std::vector<std::shared_ptr<Object>> maps = std::vector<std::shared_ptr<Object>>();

	// TODO: deprecate!
	std::vector<std::string> messages;

	std::vector<Joystick> joysticks;

	void RefreshJoysticks() {
		joysticks.clear();
		auto* window = GetWindow<SDLWindow>();
		if (window) {
			auto newJoysticks = window->GetJoysticks();
			joysticks = std::move(newJoysticks);
		}
	}

	double lastUpdateTime = 0.0f;

	bool canJump = true;

	int mapIndex = 0;

#ifndef EXCLUDE_NETWORKING
	std::unique_ptr<Networker> client = nullptr;
#endif

	std::unordered_map<unsigned char, std::shared_ptr<Character>> players = std::unordered_map<unsigned char, std::shared_ptr<Character>>();

	bool isConnectedToServer = false;

	struct Impl;
	std::unique_ptr<Impl> impl;

	std::unique_ptr<fe::Level> level = std::make_unique<fe::Level>();

	~Game();

	Game();

	typedef void* (*GLADloadproc)(const char* name);

	template <typename F, typename = std::enable_if_t<std::is_convertible_v<F, GLADloadproc>>>
	Game(F loadProc) : Game(reinterpret_cast<GLADloadproc>(loadProc)) {}

	Game(GLADloadproc loadProc);

	Game(int width, int height, bool skipInit = false, bool showWindow = true);

	Game(RendererOptions options);

	void Init();

	void Log(const std::string& message) { std::cout << message << std::endl; }

	PhysicsFactory* GetPhysicsFactory();

	void LoadShaders(std::string vertexShaderPath, std::string fragmentShaderPath) { Renderer::LoadShaders(vertexShaderPath, fragmentShaderPath); }

	bool LoadShaderTexts(std::string vertexShaderText, std::string fragmentShaderText) {
		this->shader = std::make_unique<fe::ShaderProgram>();
		return this->shader->LoadShaderTexts(vertexShaderText, fragmentShaderText);
	}

	void MovePlayer(Direction direction) { this->player->Move(direction, camera.get()); }

	void MoveCamera(Direction direction, float dt = 1.0f) { this->camera->Move(direction, dt); }

	void MouseMove(int x, int y) {
		const float sensitivity = 0.1f;

		this->yaw += sensitivity * x;
		this->pitch += sensitivity * -y;

		if (this->pitch > 89.0f) this->pitch = 89.0f;
		if (this->pitch < -89.0f) this->pitch = -89.0f;

		glm::vec3 direction;
		direction.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
		direction.y = sin(glm::radians(this->pitch));
		direction.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
		this->camera->setFront(glm::normalize(direction));
	}

	void SaveLevel(std::string fileName = "level.fes") { this->level->Save(this->scene->GetFilteredObjects(player), fileName); }

	void LoadLevel(std::string fileName = "level.fes") {
		auto objects = this->level->Load(fileName);
		this->scene->ClearObjects();
		this->scene->AddObject(player);
		for (auto& object : objects) this->scene->AddObject(object);
	}

#ifndef EXCLUDE_NETWORKING
	void connectToServer(std::string address, unsigned short port, std::string username) { this->client->Connect(address, port, username); }
#endif

	void loadMap(int index) {
		auto map = maps.at(index);
		scene->GetObjects()[0] = map;

		for (auto& mesh : map->meshes) {
			std::vector<glm::vec3> positions;
			mesh->GetPositions(positions);
			mesh->SetPhysicsObject(nullptr);
		}
	}

	void nextMap() {
		loadMap(mapIndex);
		mapIndex++;
		if (mapIndex >= maps.size()) mapIndex = 0;
	}

	void SpawnPlayer(unsigned char playerId) {
		auto newPlayer = std::static_pointer_cast<fe::Character>(this->player->Clone());

		this->players.insert_or_assign(playerId, newPlayer);

		this->scene->AddObject(newPlayer);
	}

	std::shared_ptr<fe::Object> LoadObj(std::string path, float scale = 1.0f) {
		std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>(path, scale);
		this->scene->AddObject(model);
		return model;
	}

	void LoadModel(const std::string& path);

	std::shared_ptr<fe::Object> loadOBJButDontAdd(std::string path, float scale = 1.0f) { return std::make_shared<fe::Object>(path, scale); }

	std::shared_ptr<fe::Object> LoadStaticOBJ(std::string path, float scale = 1.0f) {
		std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>(path, scale);
		model->isStatic = true;
		return model;
	}

	void Redraw(GLuint fbo) {
		BindFrameBuffer(fbo);
		Renderer::Redraw();
	}

	void Update() {
		double dt = scene->Update();
		UpdatePhysics(dt);
	}

	void UpdatePhysics(double deltaTime);

	virtual void InitUI() {}
	virtual void DrawUI() {}

	double GetFPS() { return fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0; }

	void UpdateAspect(int width, int height) {
		if (this->camera) this->camera->SetAspect(width, height);
	}
};

}  // namespace fe
