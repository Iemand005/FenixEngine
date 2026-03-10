#include "enginewidget.h"

EngineWidget::EngineWidget() {
  //this->game = std::make_unique<fe::Game>();
      }
EngineWidget::EngineWidget(QWidget* parent) : QOpenGLWidget(parent) {

}

// EngineWidget::

void EngineWidget::initializeGL() {
  this->game = std::make_unique<fe::Game>((GLADloadproc)[](const char* name) {
        return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
      });
  // if (!gladLoadGLLoader() {
  //   // handle error
  //   return;
  // }

  glEnable(GL_CULL_FACE);
}
