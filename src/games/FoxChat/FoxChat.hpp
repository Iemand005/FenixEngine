// #include "../../engine/networking/networking.hpp"
#include "../../engine/VRGame.hpp"

class FoxChat : public VRGame {
public:
  FoxChat() : VRGame(800, 600) {

  }
  void start() {
    this->maine();
  }
};