
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
     this->onDraw = [&]() {
       this->DrawGizmo();
     };
    }

    EditableGameBase(GLADloadproc loadProc) : XRGame(loadProc) {

    }

    EditableGameBase(int width, int height, bool vr = false) : XRGame(width, height, vr) {
      this->onDraw = [&]() {
        this->DrawGizmo();
      };
    }

    void Init() {
      
    }

    ~EditableGameBase() {

    };
  };
  
} // namespace fe
