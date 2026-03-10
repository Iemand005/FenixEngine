#include "enginewidget.h"

EngineWidget::EngineWidget() {}
EngineWidget::EngineWidget(QWidget* parent) : QOpenGLWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);
}

void EngineWidget::initializeGL() {
  this->game = std::make_unique<fe::XRGame>((GLADloadproc)[](const char* name) {
    return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
  });

  // glEnable(GL_CULL_FACE);

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
  capturing = true;
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

void EngineWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    stopMouseCapture();
    capturing = false;
  }
}
