#include "Object.hpp"
#include "Mesh.hpp"
#include "Scene.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace fe;

Scene::Scene() {
	objects = std::vector<std::shared_ptr<Object>>();
}

Scene::~Scene() {
}

void Scene::AddObject(std::shared_ptr<Object> object) { objects.push_back(object); }

std::shared_ptr<Object> Scene::AddObject(Mesh<Vertex> mesh) {
	auto obj = std::make_shared<Object>();
	obj->PushMesh(std::move(mesh));
	objects.push_back(obj);
	return obj;
}

std::vector<std::shared_ptr<Object>> Scene::GetFilteredObjects(std::shared_ptr<Object> exclude) const {
	std::vector<std::shared_ptr<Object>> filtered;
	std::copy_if(objects.begin(), objects.end(), std::back_inserter(filtered), [exclude](const std::shared_ptr<Object>& obj) {
		return obj != exclude;
	});
	return filtered;
}

static bool RemoveChildRecursive(Object* parent, Object* target) {
	for (auto& child : parent->children) {
		if (child.get() == target) {
			parent->RemoveChild(child.get());
			return true;
		}
		if (RemoveChildRecursive(child.get(), target))
			return true;
	}
	return false;
}

bool Scene::RemoveObject(std::shared_ptr<Object> object) {
	auto it = std::find(objects.begin(), objects.end(), object);
	if (it != objects.end()) {
		objects.erase(it);
		return true;
	}
	for (auto& obj : objects) {
		if (RemoveChildRecursive(obj.get(), object.get()))
			return true;
	}
	return false;
}

bool Scene::RemoveObject(Object* object) {
	auto it = std::find_if(objects.begin(), objects.end(),
		[object](const std::shared_ptr<Object>& obj) {
			return obj.get() == object;
		});
	if (it != objects.end()) {
		objects.erase(it);
		return true;
	}
	for (auto& obj : objects) {
		if (RemoveChildRecursive(obj.get(), object))
			return true;
	}
	return false;
}

void Scene::SetLight(int index) {
	pointLights[index].position = glm::vec3(3.0f, 3.0f, 3.0f);
	pointLights[index].color = glm::vec3(1.0f);
	pointLights[index].intensity = 1.0f;
	pointLights[index].radius = 10.0f;
}

static void UpdateObjectRecursive(Object& obj, double dt) {
	obj.Update(dt);
	if (auto* o = dynamic_cast<Object*>(&obj)) {
		for (auto& child : o->GetChildren())
			UpdateObjectRecursive(*child, dt);
	}
}

double Scene::Update() {
	auto deltaTime = timer.update();
	for (auto& object : objects) {
		UpdateObjectRecursive(*object, deltaTime);
	}
	ResolveCollisions();
	return deltaTime;
}

void Scene::ResolveCollisions() {
	for (auto& object : objects) {
		if (object->state.position.y < -10.0f) {
			auto pos = object->state.position;
			pos.y = 10;
			if (!object->physicsObject) continue;
			object->physicsObject->SetPosition(pos);
			object->physicsObject->SetLinearVelocity(glm::vec3(0.0f));
		}
	}
}

// TODO: these draw functions should go into renderer, not in scene!!
void Scene::DrawCircle(const glm::vec3& position, float radius, int segments, const glm::vec3& rotationDegrees, const glm::vec3& color) {
	if (radius <= 0.0f) return;
	segments = std::max(segments, 3);
	if (!renderDevice_) return;

	glm::mat4 rotation(1.0f);
	rotation = glm::rotate(rotation, glm::radians(rotationDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
	rotation = glm::rotate(rotation, glm::radians(rotationDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
	rotation = glm::rotate(rotation, glm::radians(rotationDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), position) * rotation;

	std::vector<glm::vec3> circleVertices;
	circleVertices.reserve(static_cast<size_t>(segments));

	constexpr float kTwoPi = 6.28318530717958647692f;
	for (int i = 0; i < segments; ++i) {
		float t = kTwoPi * (static_cast<float>(i) / static_cast<float>(segments));
		glm::vec3 localPoint(std::cos(t) * radius, 0.0f, std::sin(t) * radius);
		circleVertices.emplace_back(glm::vec3(transform * glm::vec4(localPoint, 1.0f)));
	}

	renderDevice_->DrawGizmoLines(
		reinterpret_cast<const float*>(circleVertices.data()),
		static_cast<int>(circleVertices.size()),
		GizmoDrawMode::LineLoop, color, 2.0f,
		viewMatrix_, projectionMatrix_);
}

void Scene::DrawArrow(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color, float headLengthScale, float headRadiusScale) {
	if (!renderDevice_) return;

	glm::vec3 direction = to - from;
	float length = glm::length(direction);
	if (length <= 0.0001f) return;

	glm::vec3 forward = direction / length;
	float headLength = std::clamp(length * headLengthScale, 0.0f, length * 0.95f);
	float headRadius = std::clamp(length * headRadiusScale, 0.0f, headLength * 0.75f);
	if (headRadius <= 0.0001f) {
		headRadius = length * 0.05f;
	}

	glm::vec3 shaftEnd = to - forward * headLength;

	glm::vec3 reference = std::abs(forward.y) < 0.999f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 right = glm::cross(forward, reference);
	if (glm::length(right) <= 0.0001f) {
		reference = glm::vec3(0.0f, 0.0f, 1.0f);
		right = glm::cross(forward, reference);
	}
	right = glm::normalize(right);
	glm::vec3 up = glm::normalize(glm::cross(right, forward));

	std::array<glm::vec3, 4> headRing = {
			shaftEnd + right * headRadius + up * headRadius,
			shaftEnd - right * headRadius + up * headRadius,
			shaftEnd - right * headRadius - up * headRadius,
			shaftEnd + right * headRadius - up * headRadius};

	std::vector<glm::vec3> arrowVertices;
	arrowVertices.reserve(18);
	arrowVertices.push_back(from);
	arrowVertices.push_back(shaftEnd);

	for (const auto& point : headRing) {
		arrowVertices.push_back(to);
		arrowVertices.push_back(point);
	}

	for (size_t i = 0; i < headRing.size(); ++i) {
		arrowVertices.push_back(headRing[i]);
		arrowVertices.push_back(headRing[(i + 1) % headRing.size()]);
	}

	renderDevice_->DrawGizmoLines(
		reinterpret_cast<const float*>(arrowVertices.data()),
		static_cast<int>(arrowVertices.size()),
		GizmoDrawMode::Lines, color, 2.0f,
		viewMatrix_, projectionMatrix_);
}

void Scene::DrawArrow(const glm::vec3& origin, const glm::vec3& direction, float length, const glm::vec3& color, float headLengthScale, float headRadiusScale) {
	if (length <= 0.0f) return;

	float directionLength = glm::length(direction);
	if (directionLength <= 0.0001f) return;

	DrawArrow(origin, origin + (direction / directionLength) * length, color, headLengthScale, headRadiusScale);
}

void Scene::DrawCircle(float radius, int segments) {
	DrawCircle(glm::vec3(0.0f), radius, segments, glm::vec3(0.0f), glm::vec3(0.95f, 0.80f, 0.15f));
}

void Scene::DrawCircle(const glm::vec3& position, float radius, int segments) {
	DrawCircle(position, radius, segments, glm::vec3(0.0f), glm::vec3(0.95f, 0.80f, 0.15f));
}
