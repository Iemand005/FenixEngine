// #include "../../engine/networking/networking.hpp"
#include "../../engine/XRGame.hpp"

class FoxChat : public XRGame {
public:
  FoxChat() : XRGame(800, 600) {

  }
  void start() {
    auto scene = std::make_unique<fe::Scene>();
    auto shader = std::make_unique<fe::ShaderProgram>("VertexShader.glsl", "FragmentShader.glsl");

    std::shared_ptr<fe::Object> model = std::make_shared<fe::Object>("resources/models/collisiontest.obj");
    model->isStatic = true;
    scene->AddObject(model);

    auto playerObject = std::make_shared<fe::Object>("resources/models/Ryan.obj");
    scene->AddObject(playerObject);
    player = std::static_pointer_cast<fe::Character>(playerObject);

    this->run();
  }
};