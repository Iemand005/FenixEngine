#include "Renderer.hpp"
#include "Object.hpp"
#include "Scene.hpp"

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

void Renderer::Init(GLADloadproc loadProc) {
	if (!gladLoadGLLoader(loadProc))
		std::cerr << "Failed to load GLAS dhsit" << std::endl;
		// throw std::runtime_error("Failed to load OpenGL functions (GLAD)");
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

	// Transparent pass (depth write OFF, sorted back-to-front)
	renderDevice->SetTransparentMode(true);
	{
		std::vector<Object*> transparentObjs;
		for (auto& object : scene->GetObjects()) {
			std::vector<Object*> children;
			CollectObjects(*object, children);
			for (auto* o : children) {
				if (HasTransparentMesh(*o))
					transparentObjs.push_back(o);
			}
		}
		glm::vec3 camPos = camera ? camera->GetPos() : glm::vec3(0.0f);
		std::sort(transparentObjs.begin(), transparentObjs.end(),
			[&camPos](const Object* a, const Object* b) {
				float da = glm::length2(a->state.position - camPos);
				float db = glm::length2(b->state.position - camPos);
				return da > db;
			});
		for (auto* obj : transparentObjs) {
			glm::mat4 model = obj->GetModelMatrix();
			if (shader) shader->SetMat4("model", model);
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
