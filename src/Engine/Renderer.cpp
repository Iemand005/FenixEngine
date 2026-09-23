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
		std::cerr << "Failed to load OpenGL functions (GLAD)" << std::endl;
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
	renderDevice->SetMat4("model", model);
	renderDevice->SetVec3("objectColor", object.color);
	if (object.reverseWinding) renderDevice->SetFrontFace(false);
	for (auto& mesh : object.meshes) {
		if (mesh->GetHasTransparency() != transparentPass) continue;
		mesh->SetDevice(renderDevice.get());
		renderDevice->DrawMesh(mesh->GetGPUBuffers(), mesh->GetGPUTexture());
	}
	for (auto& child : object.GetChildren())
		RenderObject(*child, transparentPass);
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

void Renderer::UploadLights(ShaderProgram* target, Scene* targetScene) {
	static std::vector<std::string> namePos, nameCol, nameInt, nameRad;
	if (namePos.empty()) {
		namePos.reserve(kMaxPointLights);
		nameCol.reserve(kMaxPointLights);
		nameInt.reserve(kMaxPointLights);
		nameRad.reserve(kMaxPointLights);
		for (int i = 0; i < kMaxPointLights; ++i) {
			std::string prefix = "pointLights[" + std::to_string(i) + "].";
			namePos.push_back(prefix + "position");
			nameCol.push_back(prefix + "color");
			nameInt.push_back(prefix + "intensity");
			nameRad.push_back(prefix + "radius");
		}
	}
	if (!target || !targetScene) return;
	int count = std::min(targetScene->GetLightCount(), kMaxPointLights);
	target->SetInt("lightCount", count);
	if (count <= 0) return;
	const auto* lights = targetScene->GetLights();
	for (int i = 0; i < count; ++i) {
		const auto& l = lights[i];
		target->SetVec3(namePos[i], l.position);
		target->SetVec3(nameCol[i], l.color);
		target->SetFloat(nameInt[i], l.intensity);
		target->SetFloat(nameRad[i], std::max(0.001f, l.radius));
	}
}

void Renderer::RenderScene(Scene *scene) {
	if (shader) UploadLights(shader.get(), scene);

	renderDevice->BeginFrame();

	// Opaque pass (depth write ON)
	renderDevice->SetTransparentMode(false);
	for (auto& object : scene->GetObjects()) {
		RenderObject(*object, false);
	}

	// Transparent pass (depth write OFF, sorted back-to-front)
	renderDevice->SetTransparentMode(true);
	{
		transparentScratch_.clear();
		transparentScratch_.reserve(scene->GetObjects().size() * 4);
		for (auto& object : scene->GetObjects())
			CollectObjects(*object, transparentScratch_);
		auto end = std::remove_if(transparentScratch_.begin(), transparentScratch_.end(),
			[](const Object* o) { return !HasTransparentMesh(*o); });
		size_t count = static_cast<size_t>(end - transparentScratch_.begin());
		if (count > 1) {
			glm::vec3 camPos = camera ? camera->GetPos() : glm::vec3(0.0f);
			std::sort(transparentScratch_.begin(), end,
				[&camPos](const Object* a, const Object* b) {
					float da = glm::length2(a->state.position - camPos);
					float db = glm::length2(b->state.position - camPos);
					return da > db;
				});
		}
		for (size_t i = 0; i < count; ++i) {
			Object* obj = transparentScratch_[i];
			glm::mat4 model = obj->GetModelMatrix();
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
