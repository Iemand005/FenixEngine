#include "Renderer.hpp"
#include "Object.hpp"
#include "Scene.hpp"

#include <algorithm>

using namespace fe;

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

Renderer::Renderer(GLADloadproc loadProc) {
	if (!gladLoadGLLoader(loadProc)) {
		std::cerr << "Failed to load OpenGL functions (GLAD)";
	}
	CreateRenderDevice(false);
	// renderDevice = std::make_unique<OpenGLRenderDevice>();

	// renderDevice->Init();
	// this->SetClearColor(0.1f, 0.4f, 1.0f);
}

void Renderer::BindFrameBuffer(int bufferIndex) {
	renderDevice->BindFramebuffer(bufferIndex);
}

void Renderer::RenderMesh(Mesh<>& mesh) {
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
	if (shader) shader->SetMat4("model", model);
	renderDevice->SetMat4("model", model);
	renderDevice->SetVec3("objectColor", object.color);
	if (object.reverseWinding) renderDevice->SetFrontFace(false);
	for (auto& mesh : object.meshes) {
		if (mesh->GetHasTransparency() != transparentPass) continue;
		mesh->SetDevice(renderDevice.get());
		renderDevice->DrawMesh(mesh->GetGPUBuffers(), mesh->GetGPUTexture());
	}
	if (auto* obj = dynamic_cast<Object*>(&object)) {
		for (auto& child : obj->GetChildren())
			RenderObject(*child, transparentPass);
	}
	if (object.reverseWinding) renderDevice->SetFrontFace(true);
}

void Renderer::RenderScene(Scene *scene) {
	if (shader) {
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

	renderDevice->BeginFrame();

	// Opaque pass (depth write ON)
	renderDevice->SetTransparentMode(false);
	for (auto& object : scene->GetObjects()) {
		RenderObject(*object, false);
	}

	// Transparent pass (depth write OFF)
	renderDevice->SetTransparentMode(true);
	for (auto& object : scene->GetObjects()) {
		RenderObject(*object, true);
	}
	renderDevice->SetTransparentMode(false);
}
