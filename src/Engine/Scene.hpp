
#pragma once
#define WIN32_LEAN_AND_MEAN
#include <glad/glad.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Timer.hpp"
#include "IMesh.hpp"
#include "Vertex.hpp"

namespace fe {

static constexpr int kMaxPointLights = 8;

struct PointLight {
	glm::vec3 position{0.0f};
	glm::vec3 color{1.0f};
	float intensity{1.0f};
	float radius{10.0f};
};

class Object;
class ObjectBase;
template<typename VertexType> class Mesh;

class Scene {
	private:
	std::vector<std::shared_ptr<ObjectBase>> objects;
	glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
	Timer timer;

	std::array<PointLight, kMaxPointLights> pointLights{};
	int lightCount = 1;

	glm::mat4 lastViewMatrix = glm::mat4(1.0f);
	glm::mat4 lastProjectionMatrix = glm::mat4(1.0f);
	bool hasCameraMatrices = false;

	GLuint gizmoProgram = 0;
	GLuint gizmoVAO = 0;
	GLuint gizmoVBO = 0;
	GLint gizmoModelLoc = -1;
	GLint gizmoViewLoc = -1;
	GLint gizmoProjectionLoc = -1;
	GLint gizmoColorLoc = -1;
	bool gizmoRendererReady = false;

	static GLuint CompileShader(GLenum type, const char* source);
	static bool CheckProgramLink(GLuint program, std::string& logOut);

	void DrawGizmoLines(const std::vector<glm::vec3>& vertices, GLenum mode, const glm::vec3& color, float lineWidth = 2.0f);
	void EnsureGizmoRenderer();

public:
	Scene();
	~Scene();

	std::vector<std::shared_ptr<ObjectBase>>& GetObjects() { return objects; }
	std::vector<std::shared_ptr<ObjectBase>> GetFilteredObjects(std::shared_ptr<ObjectBase> exclude) const;
	void ClearObjects() { objects.clear(); }

	void AddObject(std::shared_ptr<ObjectBase> object);
	std::shared_ptr<Object> AddObject(Mesh<Vertex> mesh);
	bool RemoveObject(std::shared_ptr<ObjectBase> object);

	void AddLight() { ++lightCount; }
	void RemoveLight() { --lightCount; }
	void SetLight(int index = 0);

	int GetLightCount() { return lightCount; }
	PointLight* GetLights() { return pointLights.data(); }
	std::array<PointLight, kMaxPointLights> GetLightArray() { return pointLights; }

	void EndRender() { glBindVertexArray(0); }

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
