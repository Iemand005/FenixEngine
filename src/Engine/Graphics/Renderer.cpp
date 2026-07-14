#include <glad/glad.h>

#include "Renderer.hpp"
#include "../Object.hpp"
#include "../Scene.hpp"

#include <algorithm>

using namespace fe;

void Renderer::EnableWireframe() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}
void Renderer::DisableWireframe() {
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::ToggleWireframe(bool enabled) {
  if (enabled) EnableWireframe();
  else DisableWireframe();
}

Renderer::Renderer(GLADloadproc loadProc) {
  renderDevice = std::make_unique<OpenGLRenderDevice>();
  if (!gladLoadGLLoader(loadProc)) {
    std::cerr << "Failed to load OpenGL functions (GLAD)";
  }
  renderDevice->Init(nullptr);
  this->SetClearColor(0.1f, 0.4f, 1.0f);
}

void Renderer::BindFrameBuffer(int bufferIndex) {
  glBindFramebuffer(GL_FRAMEBUFFER, bufferIndex);
}

void Renderer::RenderMesh(Mesh<>& mesh) {
	mesh.SetDevice(renderDevice.get());
	renderDevice->DrawMesh(mesh.gpuBuffers.get(), mesh.gpuTexture.get());
}

void Renderer::RenderObject(ObjectBase& object) {
	if (!shader) return;
	shader->SetMat4("model", object.GetModelMatrix());
	renderDevice->SetMat4("model", object.GetModelMatrix());
	object.Render(renderDevice.get());
}

void Renderer::RenderScene(Scene *scene) {
	if (!shader) {
		std::cerr << "[DEBUG Renderer] RenderScene: shader is null!" << std::endl;
		return;
	}

	static bool loggedSceneInfo = false;
	if (!loggedSceneInfo) {
		std::cerr << "[DEBUG Renderer] RenderScene: scene=" << scene
				  << " objects=" << scene->GetObjects().size()
				  << " lights=" << scene->GetLightCount() << std::endl;
		for (auto& obj : scene->GetObjects()) {
			std::cerr << "[DEBUG Renderer]   object: '" << obj->name
					  << "' pos=(" << obj->state.position.x << "," << obj->state.position.y << "," << obj->state.position.z << ")"
					  << " meshCount=" << obj->GetMeshCount()
					  << " vertexCount=" << obj->GetTotalVertexCount() << std::endl;
		}
		loggedSceneInfo = true;
	}

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

	for (auto& object : scene->GetObjects()) {
		RenderObject(*object);
	}
}
