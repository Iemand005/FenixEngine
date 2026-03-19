
#include "XRGame.hpp"

namespace fe
{
  class EditableGameBase : public XRGame
  {
    void DrawGizmo() {
      if (!scene) return;
      scene->DrawCircle(10, 32);
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
