
#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Timer.hpp"
#include "IMesh.hpp"
#include "Vertex.hpp"
#include "Graphics/IRenderDevice.hpp"

namespace fe {

static constexpr int kMaxPointLights = 8;

struct PointLight {
	glm::vec3 position{0.0f};
	glm::vec3 color{1.0f};
	float intensity{1.0f};
	float radius{10.0f};
};

class Object;
template<typename VertexType> class Mesh;

class Scene {
	private:
	std::vector<std::shared_ptr<Object>> objects;
	glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
	Timer timer;

	std::array<PointLight, kMaxPointLights> pointLights{};
	int lightCount = 1;

	IRenderDevice* renderDevice_ = nullptr;
	glm::mat4 viewMatrix_ = glm::mat4(1.0f);
	glm::mat4 projectionMatrix_ = glm::mat4(1.0f);

public:
	Scene();
	~Scene();

	void SetRenderDevice(IRenderDevice* device) { renderDevice_ = device; }
	void SetCameraMatrices(const glm::mat4& view, const glm::mat4& proj) { viewMatrix_ = view; projectionMatrix_ = proj; }

	std::vector<std::shared_ptr<Object>>& GetObjects() { return objects; }
	std::vector<std::shared_ptr<Object>> GetFilteredObjects(std::shared_ptr<Object> exclude) const;
	void ClearObjects() { objects.clear(); }

	void AddObject(std::shared_ptr<Object> object);
	std::shared_ptr<Object> AddObject(Mesh<Vertex> mesh);
	bool RemoveObject(std::shared_ptr<Object> object);
	bool RemoveObject(Object* object);

	void AddLight() { ++lightCount; }
	void RemoveLight() { --lightCount; }
	void SetLight(int index = 0);

	int GetLightCount() { return lightCount; }
	PointLight* GetLights() { return pointLights.data(); }
	std::array<PointLight, kMaxPointLights> GetLightArray() { return pointLights; }

	double Update();
	void ResolveCollisions();
	double GetDeltaTime() { return timer.deltaTime; }

	void DrawCircle(float radius, int segments);
	void DrawCircle(const glm::vec3& position, float radius, int segments);
	void DrawCircle(const glm::vec3& position, float radius, int segments, const glm::vec3& rotationDegrees, const glm::vec3& color);

	void DrawArrow(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color = glm::vec3(0.95f, 0.80f, 0.15f), float headLengthScale = 0.20f, float headRadiusScale = 0.12f);
	void DrawArrow(const glm::vec3& origin, const glm::vec3& direction, float length, const glm::vec3& color = glm::vec3(0.95f, 0.80f, 0.15f), float headLengthScale = 0.10f, float headRadiusScale = 0.5f);
};

}
