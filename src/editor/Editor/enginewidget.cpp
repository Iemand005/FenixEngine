#include "enginewidget.h"

EngineWidget::EngineWidget() {
  //this->game = std::make_unique<fe::Game>();
      }
EngineWidget::EngineWidget(QWidget* parent) : QOpenGLWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
}

// EngineWidget::

void EngineWidget::initializeGL() {
  this->game = std::make_unique<fe::Game>((GLADloadproc)[](const char* name) {
    return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
  });

  glEnable(GL_CULL_FACE);

  auto map1 = game->loadStaticOBJ("resources/models/collisiontest.obj");
  game->scene->AddObject(map1);
}

void EngineWidget::resizeGL(int w, int h) {
  this->game->Resize(w, h);
}

void EngineWidget::paintGL() {
  this->game->Redraw();
}

void EngineWidget::startMouseCapture() {
  grabMouse();
  grabKeyboard();
  setCursor(Qt::BlankCursor);
}

void EngineWidget::stopMouseCapture() {
  releaseMouse();
  releaseKeyboard();
  unsetCursor();
}

void EngineWidget::mouseMoveEvent(QMouseEvent* e) {
  if (!capturing) return;
  QPoint center = rect().center();
  QPoint delta = e->pos() - center;
  // delta.x(), delta.y()
  game->MouseMove(delta.x(), delta.y());
  QCursor::setPos(mapToGlobal(center));
}

void EngineWidget::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton)
    startMouseCapture();
  QOpenGLWidget::mousePressEvent(e);
}
