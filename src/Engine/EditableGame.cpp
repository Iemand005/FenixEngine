#pragma once

#include "EditableGame.hpp"

#include <imgui.h>

#include <algorithm>
#include <limits>

#include <SDL3/SDL.h>

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.hpp"
#include "Object.hpp"
#include "Scene.hpp"
#include "physics/BasicDebugRenderer.hpp"

using namespace fe;

void fe::EditableGame::DetectAndHandleClick() {
	if (!scene || !camera) return;

	for (auto &window : windows) {
		float mx, my;
		Uint32 buttons = SDL_GetMouseState(&mx, &my);
		bool leftDown = (buttons & SDL_BUTTON_LMASK) != 0;

		if (leftDown && !leftWasDown_ && !ImGui::GetIO().WantCaptureMouse) {
			RaycastSelect(static_cast<int>(mx), static_cast<int>(my), window->width, window->height);
		}

		leftWasDown_ = leftDown;
	}
}

void fe::EditableGame::OnDraw() {
	if (IsSelectModeEnabled())
		DetectAndHandleClick();
	EditableGameBase::OnDraw();
}

static void RaycastNode(fe::Object* obj, const glm::vec3& rayOrigin, const glm::vec3& rayDir, fe::Object*& hitObject, float& closestT) {
	glm::mat4 modelMatrix = obj->GetModelMatrix();
	glm::mat4 invModel = glm::inverse(modelMatrix);

	glm::vec3 localOrigin = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
	glm::vec3 localDir = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

	for (auto& mesh : obj->meshes) {
		std::vector<glm::vec3> positions;
		mesh->GetPositions(positions);
		const auto& indices = mesh->GetIndices();

		if (indices.empty() || positions.empty()) continue;

		for (size_t i = 0; i + 2 < indices.size(); i += 3) {
			const glm::vec3& v0 = positions[indices[i]];
			const glm::vec3& v1 = positions[indices[i + 1]];
			const glm::vec3& v2 = positions[indices[i + 2]];

			glm::vec3 e1 = v1 - v0;
			glm::vec3 e2 = v2 - v0;
			glm::vec3 p = glm::cross(localDir, e2);
			float det = glm::dot(e1, p);

			if (std::abs(det) < 1e-8f) continue;
			float invDet = 1.0f / det;

			glm::vec3 tVec = localOrigin - v0;
			float u = glm::dot(tVec, p) * invDet;
			if (u < 0.0f || u > 1.0f) continue;

			glm::vec3 q = glm::cross(tVec, e1);
			float v = glm::dot(localDir, q) * invDet;
			if (v < 0.0f || u + v > 1.0f) continue;

			float t = glm::dot(e2, q) * invDet;
			if (t > 0.0f && t < closestT) {
				closestT = t;
				hitObject = obj;
			}
		}
	}

	for (auto& child : obj->children)
		RaycastNode(child.get(), rayOrigin, rayDir, hitObject, closestT);
}

void fe::EditableGameBase::RaycastSelect(int mx, int my, int w, int h) {
	if (!scene || !camera) return;

	glm::vec3 nearPos = glm::unProject(
		glm::vec3(static_cast<float>(mx), static_cast<float>(h - my), 0.0f),
		camera->GetViewMatrix(),
		camera->GetProjectionMatrix(),
		glm::vec4(0, 0, w, h)
	);
	glm::vec3 farPos = glm::unProject(
		glm::vec3(static_cast<float>(mx), static_cast<float>(h - my), 1.0f),
		camera->GetViewMatrix(),
		camera->GetProjectionMatrix(),
		glm::vec4(0, 0, w, h)
	);

	glm::vec3 rayOrigin = nearPos;
	glm::vec3 rayDir = glm::normalize(farPos - nearPos);

	Object* hitObject = nullptr;
	float closestT = std::numeric_limits<float>::max();

	for (auto& obj : scene->GetObjects())
		RaycastNode(obj.get(), rayOrigin, rayDir, hitObject, closestT);

	if (hitObject)
		SelectObject(hitObject);
}

void DrawObjectNode(Object* object, Camera* camera, Scene* scene, EditableGame* game, float step) {
	ImGui::PushID(object);

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (object->children.empty()) {
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	if (game->GetSelectedObject() == object)
		flags |= ImGuiTreeNodeFlags_Selected;

	bool open = ImGui::TreeNodeEx(object->name.c_str(), flags);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		game->SelectObject(object);

	ImGui::DragFloat3("Position", &object->state.position.x, step);

	if (object->physicsObject) {
		if (ImGui::TreeNode("Physics")) {
			glm::vec3 physicsPos = object->physicsObject->GetPosition();
			if (ImGui::DragFloat3("Pos", &physicsPos.x, step)) {
				object->physicsObject->SetPosition(physicsPos);
			}
			glm::vec3 physicsVel = object->physicsObject->GetLinearVelocity();
			if (ImGui::DragFloat3("Vel", &physicsVel.x, step)) {
				object->physicsObject->SetLinearVelocity(physicsVel);
			}
			float friction = object->physicsObject->GetFriction();
			if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f)) {
				object->physicsObject->SetFriction(friction);
			}
			float linDamp = object->physicsObject->GetLinearDamping();
			if (ImGui::SliderFloat("Lin Damping", &linDamp, 0.0f, 1.0f)) {
				float angDamp = object->physicsObject->GetAngularDamping();
				object->physicsObject->SetDamping(linDamp, angDamp);
			}
			float angDamp = object->physicsObject->GetAngularDamping();
			if (ImGui::SliderFloat("Ang Damping", &angDamp, 0.0f, 1.0f)) {
				float linDamp = object->physicsObject->GetLinearDamping();
				object->physicsObject->SetDamping(linDamp, angDamp);
			}
			ImGui::TreePop();
		}
	}

	ImGui::DragFloat3("Rotation", &object->state.rotation.x, step);
	ImGui::DragFloat3("Scale", &object->state.scale.x, step);

	if (ImGui::Button("Focus")) {
		glm::vec3 offset = glm::vec3(3.0f, 2.0f, 3.0f);
		camera->SetPos(object->state.position + offset);
		camera->LookAt(object->state.position);
	}

	if (ImGui::Button("Delete")) {
		scene->RemoveObject(object);
	}

	ImGui::Separator();

	if (open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
		for (auto& child : object->children) {
			DrawObjectNode(child.get(), camera, scene, game, step);
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
}

void fe::EditableGame::DrawDebugUI() {
#ifdef FE_INCLUDE_OPENVR
	if (openVR && openVR->mode == OpenVR::Mode::Scene) {
		ImGui::Begin("XR");
		ImGui::Text("OpenVR HMD active");
		if (ImGui::Button("Stop OpenVR")) {
			DestroyXR();
		}
		ImGui::End();
		return;
	}
#endif

	ImGui::Begin("Debug");
	{
		ImGui::Text("Hello, World!");

		ImGui::Text("Graphics API: %s", ImGui::GetIO().BackendRendererName ? ImGui::GetIO().BackendRendererName : "Unknown");
		if (renderDevice) ImGui::Text("Device: %s", renderDevice->GetDeviceName());

		ImGui::Text("FPS %.1f", fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

		ImGui::Text("Objects: %zu", this->scene->GetObjects().size());
		size_t totalVertices = 0;
		for (auto& obj : this->scene->GetObjects()) totalVertices += obj->GetTotalVertexCount();
		ImGui::Text("Vertices: %zu", totalVertices);

		ImGui::Checkbox("Frustum Culling", &frustumCullingEnabled);

		bool selectMode = IsSelectModeEnabled();
		if (ImGui::Checkbox("Select Mode (click 3D objects)", &selectMode))
			SetSelectModeEnabled(selectMode);

		if (ImGui::Button("Enable VR!", ImVec2(100, 20))) {
			this->EnableXR();
		}

		if (ImGui::Button("Disable VR :()", ImVec2(100, 20))) {
			this->DestroyXR();
		}

#ifdef FE_INCLUDE_OPENVR
		ImGui::Begin("XR");
		if (ImGui::Button("Start OpenVR HMD")) {
			StartOpenVR();
		}
		ImGui::End();
#endif

		if (ImGui::Button("Enable AA", ImVec2(70, 20))) {
			std::cout << "Button clicked!" << std::endl;
		}

		if (ImGui::Checkbox("VSync", &vsyncEnabled)) {
			SetVSync(vsyncEnabled);
		}

		static bool wireframe = false;
		if (ImGui::Checkbox("Enable Wireframe", &wireframe)) {
			if (wireframe)
				this->EnableWireframe();
			else
				this->DisableWireframe();
		}

		static bool showDepthBuffer = false;
		ImGui::Checkbox("Show Depth Buffer", &showDepthBuffer);
		if (showDepthBuffer) {
			std::vector<float> depths;
			int readW = 0, readH = 0;
			if (renderDevice->ReadDepthBuffer(depths, readW, readH) && readW > 0 && readH > 0) {
				float mn = 1.0f, mx = 0.0f;
				for (float d : depths) {
					if (d < mn) mn = d;
					if (d > mx) mx = d;
				}
				float rng = mx - mn;
				std::vector<unsigned char> img(static_cast<size_t>(readW) * readH * 4);
				for (int i = 0; i < readW * readH; i++) {
					unsigned char c = static_cast<unsigned char>(((rng > 0.001f ? (depths[i] - mn) / rng : 0.5f)) * 255.0f);
					img[i * 4] = c;
					img[i * 4 + 1] = c;
					img[i * 4 + 2] = c;
					img[i * 4 + 3] = 255;
				}
				void* tex = renderDevice->UploadToImGui(img.data(), readW, readH);
				if (tex) {
					ImGui::Begin("Depth Buffer");
					ImVec2 avail = ImGui::GetContentRegionAvail();
					float scale = std::min(avail.x / readW, avail.y / readH);
					if (scale > 1.0f) scale = 1.0f;
					ImGui::Image(tex, ImVec2(readW * scale, readH * scale));
					ImGui::End();
				}
			}
		}

// BasicDebugRenderer::DrawImGuiToggle("Show physics debug");
#ifndef EXCLUDE_JOLT
		ImGui::Checkbox("Show physics debug", &BasicDebugRenderer::DebugRenderingEnabled());

		if (ImGui::Button(physicsGravityEnabled ? "Disable Gravity" : "Enable Gravity")) {
			physicsGravityEnabled = !physicsGravityEnabled;
			if (this->player) {
				this->player->gravityEnabled = physicsGravityEnabled;
			}
			auto physics = GetPhysicsFactory();
			if (physics) {
				if (physicsGravityEnabled)
					physics->EnableGravity();
				else physics->DisableGravity();
			}
		}

#else
		ImGui::Text("Jolt disabled.");
#endif

		glm::vec3 cp = camera->GetPos();
		float p[3] = {cp.x, cp.y, cp.z};
		if (ImGui::DragFloat3("Camera Pos", p)) camera->SetPos(glm::vec3(p[0], p[1], p[2]));

		if (camera) {
			float fov = camera->GetFOV();
			if (ImGui::SliderFloat("FOV", &fov, -10.0f, 179.0f, "%.1f deg")) {
				camera->SetFOV(fov);
			}
			if (ImGui::SliderFloat("Far Plane", &camera->farDist, 10.0f, 3000.0f, "%.1f")) {
				camera->SetAspect(camera->aspect);
			}
		}

		fe::Object* model = this->player.get();
		if (model) {
			ImGui::SliderFloat3("Position", &model->state.position.x, -10.0f, 10.0f);
			if (model->physicsObject) {
				ImGui::Separator();
				ImGui::Text("Physics body");
				glm::vec3 physicsPos = model->physicsObject->GetPosition();
				if (ImGui::DragFloat3("Physics Position", &physicsPos.x, 0.01f)) {
					model->physicsObject->SetPosition(physicsPos);
				}
				glm::vec3 physicsVel = model->physicsObject->GetLinearVelocity();
				if (ImGui::DragFloat3("Physics Velocity", &physicsVel.x, 0.01f)) {
					model->physicsObject->SetLinearVelocity(physicsVel);
				}
			}
			for (size_t i = 0; i < this->npcs.size(); ++i) {
				ImGui::Text("NPC %zu", i);
				ImGui::SliderFloat3(("Position##npc" + std::to_string(i)).c_str(), &this->npcs[i]->state.position.x, -10.0f, 10.0f);
				ImGui::SliderFloat3(("Rotation##npc" + std::to_string(i)).c_str(), &this->npcs[i]->state.rotation.x, -180.0f, 180.0f);
			}
		}
		ImGui::End();
	}

	ImGui::Begin("Objects");
	{
		static char filenameBuffer[512] = "\0";
		static float newObjectScale = 1.0f;

		if (ImGui::Button("Open Model")) {
			auto w = this->GetWindow<SDLWindow>();
			w->OpenFileDialog([this](const std::string& path) { this->LoadModel(path); });
		}
		ImGui::SameLine();
		if (ImGui::Button("Load .obj")) {
			LoadObj(filenameBuffer, newObjectScale);
		}
		ImGui::InputText(".obj path", filenameBuffer, IM_ARRAYSIZE(filenameBuffer));
		ImGui::DragFloat3("Scale##newObj", &newObjectScale, 0.001f);

		static char mapNameBuffer[512] = "level.fes\0";
		ImGui::InputText("Map file", mapNameBuffer, IM_ARRAYSIZE(mapNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

		if (ImGui::Button("Save map!")) this->SaveLevel();

		if (ImGui::Button("Load map!")) this->LoadLevel();

		if (ImGui::Button("Clear objects")) this->scene->ClearObjects();

		if (ImGui::Button("New Window!")) NewWindow(500, 500);

		if (ImGui::Button("New Vulkan Window", ImVec2(150, 0))) NewWindow(800, 600, false, false, true);
		if (ImGui::Button("New OpenGL Window", ImVec2(150, 0))) NewWindow(800, 600, false, false, false);

		static bool snapToGrid = true;
		ImGui::Checkbox("Snap to grid", &snapToGrid);
		float step = snapToGrid ? 0.1f : 0.0001f;

		for (auto& object : scene->GetObjects()) DrawObjectNode(object.get(), camera.get(), scene.get(), this, step);

		if (ImGui::Button("Add light")) this->scene->AddLight();

		auto lights = scene->GetLights();
		for (int i = 0; i < scene->GetLightCount(); ++i) {
			ImGui::Text("Light %i", i);
			ImGui::DragFloat3(("Position##light" + std::to_string(i)).c_str(), &lights[i].position.x, step);
			ImGui::DragFloat3(("Colour##light" + std::to_string(i)).c_str(), &lights[i].color.x, step);
			ImGui::DragFloat(("Radius##light" + std::to_string(i)).c_str(), &lights[i].radius, step);
			ImGui::DragFloat(("Intensity##light" + std::to_string(i)).c_str(), &lights[i].intensity, step);
		}
	}
	ImGui::End();

	ImGui::Begin("Joysticks");
	{
		if (ImGui::Button("Refresh")) RefreshJoysticks();
		ImGui::SameLine();
		ImGui::Text("%zu joystick(s)", joysticks.size());

		static std::vector<float> constForceLevels, shakeMagnitudes, springCoeffs;
		constForceLevels.resize(joysticks.size());
		shakeMagnitudes.resize(joysticks.size());
		springCoeffs.resize(joysticks.size());
		for (size_t i = 0; i < joysticks.size(); ++i) {
			ImGui::Text("%zu: %s", i, joysticks[i].GetName().c_str());
			ImGui::SameLine();
			ImGui::TextColored(joysticks[i].IsHaptic() ? ImVec4(0,1,0,1) : ImVec4(1,0,0,1), joysticks[i].IsHaptic() ? "[Haptic]" : "[No Haptic]");
			auto axis = joysticks[i].GetAxis();
			ImGui::ProgressBar((axis.x + 1.0f) * 0.5f, ImVec2(0.0f, 0.0f), "");
			ImGui::SameLine();
			ImGui::Text("X: %.2f", axis.x);
			ImGui::ProgressBar((axis.y + 1.0f) * 0.5f, ImVec2(0.0f, 0.0f), "");
			ImGui::SameLine();
			ImGui::Text("Y: %.2f", axis.y);

			ImGui::Indent();
			if (ImGui::SliderFloat(("Const Force##c" + std::to_string(i)).c_str(), &constForceLevels[i], -1.0f, 1.0f)) {
				Sint16 level = static_cast<Sint16>(constForceLevels[i] * 32767.0f);
				SDL_HapticDirection dir{};
				dir.type = SDL_HAPTIC_CARTESIAN;
				dir.dir[0] = 1;
				SetJoystickConstantForce(i, level, dir);
			}
			if (ImGui::SliderFloat(("Shake##p" + std::to_string(i)).c_str(), &shakeMagnitudes[i], 0.0f, 1.0f)) {
				Sint16 mag = static_cast<Sint16>(shakeMagnitudes[i] * 32767.0f);
				SetJoystickPeriodicEffect(i, SDL_HAPTIC_SINE, mag, 5000);
			}
			if (ImGui::SliderFloat(("Spring##s" + std::to_string(i)).c_str(), &springCoeffs[i], -1.0f, 1.0f)) {
				Sint16 coeff = static_cast<Sint16>(springCoeffs[i] * 32767.0f);
				SetJoystickSpringForce(i, coeff, coeff);
			}
			if (ImGui::SmallButton(("Stop##h" + std::to_string(i)).c_str())) {
				StopJoystickHaptic(i);
				constForceLevels[i] = 0.0f;
				shakeMagnitudes[i] = 0.0f;
				springCoeffs[i] = 0.0f;
			}
			ImGui::Unindent();
		}
	}
	ImGui::End();

	if (this->client) DrawNetworkDebugUI();
}