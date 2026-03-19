#pragma once

#include "XRGame.hpp"

namespace fe
{
  class EditableGameBase : public XRGame
  {

    fe::Object *selectedObject = nullptr;
    int lighselecIndex = -1;

    void DrawGizmo(const glm::vec3& position) {
      if (!scene) return;
      scene->DrawCircle(position, 10, 32, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.95f, 0.80f, 0.15f));
      scene->DrawArrow(position, {1.0f, 0.0f, 0.0f}, 2.0f, {1.0f, 0.0f, 0.0f});
      scene->DrawArrow(position, {0.0f, 1.0f, 0.0f}, 2.0f, {0.0f, 1.0f, 0.0f});
      scene->DrawArrow(position, {0.0f, 0.0f, 1.0f}, 2.0f, {0.0f, 0.0f, 1.0f});
    }

  public:
    EditableGameBase() {
    }

    EditableGameBase(GLADloadproc loadProc) : XRGame(loadProc) {
    }

    EditableGameBase(int width, int height, bool vr = false) : XRGame(width, height, vr) {
    }

    void OnDraw() override {
      // for (auto &light : scene->GetLightArray())
      // auto lights = scene->GetLights();
      // int lightCount = scene->GetLightCount();
      // for (int i = 0; i < lightCount; ++i)
      if (lighselecIndex >=0 && lighselecIndex < kMaxPointLights)DrawGizmo(lights[lighselecIndex].position);

      // auto objects = scene->GetObjects();
      // for (auto &object : objects)
      //   DrawGizmo(object->state.position);

      if (selectedObject) DrawGizmo(selectedObject->state.position);
    }

    void SelectObjectByIndex(int index) {
      // this->geto
      auto objs = scene->GetObjects();
      auto objec = objs[index];
      selectedObject = objec.get();
    }

    void UnselectObject() {
      selectedObject = nullptr;
    }

    void SelectLightByIndex(int index) {
      lighselecIndex = index;
    }

    void UnselectLight() {
      lighselecIndex = -1;
    }
  };
  
} // namespace fe
