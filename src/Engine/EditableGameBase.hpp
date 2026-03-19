
#include "XRGame.hpp"

namespace fe
{
  class EditableGameBase : public XRGame
  {
    void DrawGizmo() {
      if (!scene) return;
      scene->DrawCircle(10, 32);
      scene->DrawArrow({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 5.0f, {1.0f, 0.2f, 0.2f});
    }

  public:
    EditableGameBase() {
    }

    EditableGameBase(GLADloadproc loadProc) : XRGame(loadProc) {
    }

    EditableGameBase(int width, int height, bool vr = false) : XRGame(width, height, vr) {
    }

    void OnDraw() override {
      DrawGizmo();
    }
  };
  
} // namespace fe
