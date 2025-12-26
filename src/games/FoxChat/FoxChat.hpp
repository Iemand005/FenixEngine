// #include "../../engine/networking/networking.hpp"
#include "../../engine/VRGame.hpp"

class FoxChat : public VRGame {
public:
  FoxChat() : VRGame(800, 600) {

  }
  void start() {
    this->maine();
        auto scene = std::make_unique<fe::Scene>();
    auto shader = std::make_unique<fe::ShaderProgram>("VertexShader.glsl", "FragmentShader.glsl");

    std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>("resources/models/collisiontest.obj");
    model->isStatic = true;
    model->needsUpdate = false;
    scene->addModel(model);

    auto playerObject = std::make_shared<fe::Object>("resources/models/citizen.obj", 0.1f);
    scene->addModel(playerObject);
    player = std::static_pointer_cast<fe::Character>(playerObject);
  }
};