
#include "XRGame.hpp"

namespace fe
{
  class EditableGameBase : public XRGame
  {
  // private:
    // std::unique_ptr<XRGame> game;
  public:
    EditableGameBase() {

    }

    EditableGameBase(int width, int height, bool vr = false) : XRGame(width, height, vr) {
    }
    ~EditableGameBase() {

    };
  };
  
} // namespace fe
