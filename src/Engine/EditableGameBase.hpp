#pragma once

#include "XRGame.hpp"

namespace fe {
class EditableGameBase : public XRGame {
	fe::Object* selectedObject = nullptr;
	int lighselecIndex = -1;
	bool selectModeEnabled_ = false;

	void DrawGizmo(const glm::vec3& position) {
		if (!scene) return;
		scene->DrawCircle(position, 10, 32, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.95f, 0.80f, 0.15f));
		scene->DrawArrow(position, {1.0f, 0.0f, 0.0f}, 2.0f, {1.0f, 0.0f, 0.0f});
		scene->DrawArrow(position, {0.0f, 1.0f, 0.0f}, 2.0f, {0.0f, 1.0f, 0.0f});
		scene->DrawArrow(position, {0.0f, 0.0f, 1.0f}, 2.0f, {0.0f, 0.0f, 1.0f});
	}

   public:
	EditableGameBase() {}

	EditableGameBase(GLADloadproc loadProc) : XRGame(loadProc) {}

	EditableGameBase(int width, int height, bool vr = false, bool showWindow = true) : XRGame(width, height, vr, true, showWindow) {}

	EditableGameBase(XRGameOptions options) : XRGame(options) {}

	void OnDraw() override {
		if (selectedObject) DrawGizmo(selectedObject->state.position);

		auto physics = GetPhysicsFactory();
		if (physics) physics->RenderDebug(camera->GetViewMatrix(), camera->GetProjectionMatrix());
	}

	bool IsSelectModeEnabled() const { return selectModeEnabled_; }
	void SetSelectModeEnabled(bool enabled) { selectModeEnabled_ = enabled; }

	void RaycastSelect(int mx, int my, int w, int h);

	void SelectObjectByIndex(int index) {
		auto objs = scene->GetObjects();
		auto objec = objs[index];
		selectedObject = objec.get();
	}

	void SelectObject(fe::Object* obj) { selectedObject = obj; }
	fe::Object* GetSelectedObject() const { return selectedObject; }
	void UnselectObject() { selectedObject = nullptr; }

	void SelectLightByIndex(int index) { lighselecIndex = index; }

	void UnselectLight() { lighselecIndex = -1; }
};

}  // namespace fe
