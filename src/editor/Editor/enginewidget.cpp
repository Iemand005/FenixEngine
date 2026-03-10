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

  glEnable(GL_CULL_FACE);
}

void EngineWidget::resizeGL(int w, int h) {
  this->game->Resize(w, h);
}

void EngineWidget::paintGL() {
  this->game->Redraw();
}
