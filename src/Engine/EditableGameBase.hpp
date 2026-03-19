
#include "XRGame.hpp"

namespace fe
{
  class EditableGameBase : public XRGame
  {
  // private:
    // std::unique_ptr<XRGame> game;

    void DrawGizmo() {
      scene->DrawCircle(10, 32);
    }

  public:
    EditableGameBase() {
      Init();
    }

    EditableGameBase(GLADloadproc loadProc) : XRGame(loadProc) {
      Init();
    }

    EditableGameBase(int width, int height, bool vr = false) : XRGame(width, height, vr) {
      Init();
    }

    void Init() {
      this->onDraw = [&]() {
       this->DrawGizmo();
     };
    }

    ~EditableGameBase() {

    };
  };
  
} // namespace fe
