#pragma once

#include <imgui.h>

#include "EditableGame.hpp"
#include "physics/BasicDebugRenderer.hpp"


void fe::EditableGame::OnDraw() {
	EditableGameBase::OnDraw();
#ifndef EXCLUDE_JOLT
	if (GetPhysicsEngine()) {
		GetPhysicsEngine()->RenderDebug(camera->GetViewMatrix(), camera->GetProjectionMatrix());
	}
#endif
}


void fe::EditableGame::DrawDebugUI() {

	ImGui::Begin("Debug");
	{
	ImGui::Text("Hello, World!");
	
	ImGui::Text("Graphics API: %s", ImGui::GetIO().BackendRendererName ? ImGui::GetIO().BackendRendererName : "Unknown");

	ImGui::Text("FPS %.1f", fpsCounter.deltaTime > 0.0 ? 1.0 / fpsCounter.deltaTime : 0.0);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::Text("Objects: %zu", this->scene->GetObjects().size());
	size_t totalVertices = 0;
	for (auto& obj : this->scene->GetObjects()) totalVertices += obj->GetTotalVertexCount();
	ImGui::Text("Vertices: %zu", totalVertices);

	if (ImGui::Button("Enable VR!", ImVec2(100, 20))) {
		this->EnableXR ();
	}

	if (ImGui::Button("Disable VR :()", ImVec2(100, 20))) {
		this->DestroyXR();
	}

	if (ImGui::Button("Enable AA", ImVec2(70, 20))) {
		std::cout << "Button clicked!" << std::endl;
	}

	static bool wireframe = false;
	if (ImGui::Checkbox("Enable Wireframe", &wireframe)) {
		if (wireframe) this->EnableWireframe();
		else this->DisableWireframe();
	}

	// BasicDebugRenderer::DrawImGuiToggle("Show physics debug");
#ifndef EXCLUDE_JOLT
	ImGui::Checkbox("Show physics debug", &BasicDebugRenderer::DebugRenderingEnabled());

	if (ImGui::Button(physicsGravityEnabled ? "Disable Gravity" : "Enable Gravity")) {
		physicsGravityEnabled = !physicsGravityEnabled;
		if (this->player) {
			this->player->gravityEnabled = physicsGravityEnabled;
		}
		if (physicsGravityEnabled) {
			if (GetPhysicsEngine()) GetPhysicsEngine()->EnableGravity();
		} else {
			if (GetPhysicsEngine()) GetPhysicsEngine()->DisableGravity();
		}
	}

#else
	ImGui::Text("Jolt disabled.");
#endif


	if (camera) {
		float fov = camera->GetFOV();
		if (ImGui::SliderFloat("FOV", &fov, -10.0f, 179.0f, "%.1f deg")) {
			camera->SetFOV(fov);
		}
	}

	fe::Object<>* model = this->player.get();
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

	ImGui::InputText("Model file (.obj)", filenameBuffer, IM_ARRAYSIZE(filenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::DragFloat3("Scale##newObj", &newObjectScale, 0.001f);
	if (ImGui::Button("Load model")) {
		LoadObj(filenameBuffer, newObjectScale);
	}


	static char mapNameBuffer[512] = "level.fes\0";
	ImGui::InputText("Map file", mapNameBuffer, IM_ARRAYSIZE(mapNameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

	if (ImGui::Button("Save map!"))
		this->SaveLevel();

	if (ImGui::Button("Load map!"))
		this->LoadLevel();

	if (ImGui::Button("Clear objects"))
		this->scene->ClearObjects();

	static bool snapToGrid = true;
	ImGui::Checkbox("Snap to grid", &snapToGrid);
	float step = snapToGrid ? 0.1f : 0.0001f;

	size_t i = 0;
	for (auto &object : scene->GetObjects()) {
		// ImGui::Text("Object %zu", i);
		ImGui::Text(object->name.c_str());
		ImGui::DragFloat3(("Position##npc" + std::to_string(i)).c_str(), &object->state.position.x, step);
		if (object->physicsObject) {
			std::string physicsPosLabel = "Physics Pos##" + std::to_string(i);
			glm::vec3 physicsPos = object->physicsObject->GetPosition();
			if (ImGui::DragFloat3(physicsPosLabel.c_str(), &physicsPos.x, step)) {
				object->physicsObject->SetPosition(physicsPos);
			}
			std::string physicsVelLabel = "Physics Vel##" + std::to_string(i);
			glm::vec3 physicsVel = object->physicsObject->GetLinearVelocity();
			if (ImGui::DragFloat3(physicsVelLabel.c_str(), &physicsVel.x, step)) {
				object->physicsObject->SetLinearVelocity(physicsVel);
			}
		}
		ImGui::DragFloat3(("Rotation##npc" + std::to_string(i)).c_str(), &object->state.rotation.x, step);
		ImGui::DragFloat3(("Scale##npc" + std::to_string(i)).c_str(), &object->state.scale.x, step);
		if(ImGui::Button(("Focus##" + std::to_string(i)).c_str())) {
			glm::vec3 offset = glm::vec3(3.0f, 2.0f, 3.0f);
			camera->SetPos(object->state.position + offset);
			camera->LookAt(object->state.position);
		}

		ImGui::Separator();

		++i;
	}

	if (ImGui::Button("Add light"))
		this->scene->AddLight();

	auto lights = scene->GetLights();
	for (int i = 0; i < scene->GetLightCount(); ++i) {
		ImGui::Text("Light %zu", i);
		ImGui::DragFloat3(("Position##light" + std::to_string(i)).c_str(), &lights[i].position.x, step);
		ImGui::DragFloat3(("Colour##light" + std::to_string(i)).c_str(), &lights[i].color.x, step);
		ImGui::DragFloat(("Radius##light" + std::to_string(i)).c_str(), &lights[i].radius, step);
		ImGui::DragFloat(("Intensity##light" + std::to_string(i)).c_str(), &lights[i].intensity, step);
	}
	}
	ImGui::End();


	if (this->client) DrawNetworkDebugUI();
}