#include "enginewidget.h"

EngineWidget::EngineWidget() {}
EngineWidget::EngineWidget(QWidget* parent) : QOpenGLWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);

  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, [&]() {
    update();
  });
}

void EngineWidget::initializeGL() {
  this->game = std::make_unique<fe::XRGame>((GLADloadproc)[](const char* name) {
    return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
  });

  auto map1 = game->LoadStaticOBJ("resources/models/collisiontest.obj");
  game->scene->AddObject(map1);
}

void EngineWidget::resizeGL(int w, int h) {
  this->game->Resize(w, h);
}

void EngineWidget::paintGL() {
  // this->game->Update();
  GLuint fbo = defaultFramebufferObject();
  this->game->Redraw(fbo);
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
  int x = delta.x();
  int y = delta.y();
  game->MouseMove(x, y);
  QCursor::setPos(mapToGlobal(center));
  update();
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

  switch (event->key()) {
    case Qt::Key_W: game->MoveCamera(fe::Direction::Forwards); break;
    case Qt::Key_A: game->MoveCamera(fe::Direction::Left); break;
    case Qt::Key_S: game->MoveCamera(fe::Direction::Backwards); break;
    case Qt::Key_D: game->MoveCamera(fe::Direction::Right); break;
  }

  update();
}

void EngineWidget::toggleTimer(bool enabled) {
   if (enabled) timer->start(0);
   else timer->stop();
}
