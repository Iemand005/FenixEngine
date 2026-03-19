
#include "XRGame.hpp"

namespace fe
{
  class EditableGameBase : public XRGame
  {
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
      DrawGizmo({0, 1, 0});
    }
  };
  
} // namespace fe
