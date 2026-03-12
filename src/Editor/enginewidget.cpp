#include "enginewidget.h"

EngineWidget::EngineWidget() {}
EngineWidget::EngineWidget(QWidget* parent) : QOpenGLWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
  setMouseTracking(true);

  QSurfaceFormat format;
  format.setSwapInterval(0);
  setFormat(format);

  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, [&]() {
    update();
    // window()
    // ->statusbar->showMessage(QString("FPS: %1 Frames rendered: %2").arg(game->GetFPS()).arg(ui->engineWidget->renderedFrames));
    emit fpsUpdate(game->GetFPS());
  });
  timer->start(0);
}

void EngineWidget::initializeGL() {
  this->game = std::make_unique<fe::XRGame>((GLADloadproc)[](const char* name) {
    return (void*)QOpenGLContext::currentContext()->getProcAddress(name);
  });

  auto map1 = game->LoadStaticOBJ("resources/models/collisiontest.obj");
  game->scene->AddObject(map1);

  auto map2 = game->LoadStaticOBJ("C:/Users/Lasse/3D Objects/Car.obj");
  game->scene->AddObject(map2);
}

void EngineWidget::resizeGL(int w, int h) {
  this->game->Resize(w, h);
}

void EngineWidget::paintGL() {
  GLuint fbo = defaultFramebufferObject();
  game->ToggleWireframe(wireframe);
  game->Redraw(fbo);
  renderedFrames++;

  const qint64 now = frameTimer.nsecsElapsed();
  double dt = (now - lastFrameNs) / 1e9;
  lastFrameNs = now;
  if (dt < 0.0) dt = 0.0;
  if (dt > 0.1) dt = 0.1;
  processInput(dt);
}

void EngineWidget::startMouseCapture() {
  grabMouse();
  grabKeyboard();
  setCursor(Qt::BlankCursor);
  capturing = true;

  QCursor::setPos(mapToGlobal(rect().center()));
}

void EngineWidget::stopMouseCapture() {
  releaseMouse();
  releaseKeyboard();
  unsetCursor();
  capturing = false;
}

void EngineWidget::mouseMoveEvent(QMouseEvent *event) {
  if (!capturing) return;
  QPoint center = rect().center();
  QPoint delta = event->pos() - center;
  int x = delta.x();
  int y = delta.y();
  game->MouseMove(x, y);
  QCursor::setPos(mapToGlobal(center));
  update();
}

void EngineWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton)
    startMouseCapture();
  QOpenGLWidget::mousePressEvent(event);
}

void EngineWidget::keyPressEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) return;
  if (event->key() == Qt::Key_Escape) stopMouseCapture();

  keysDown.insert(event->key());

  update();

  QOpenGLWidget::keyPressEvent(event);
}

void EngineWidget::keyReleaseEvent(QKeyEvent *event) {
  if (event->isAutoRepeat()) return;
  keysDown.remove(event->key());

  QOpenGLWidget::keyReleaseEvent(event);
}

void EngineWidget::processInput(double dt) {
  if (keysDown.contains(Qt::Key_W)) game->MoveCamera(fe::Direction::Forwards, (float)dt);
  if (keysDown.contains(Qt::Key_S)) game->MoveCamera(fe::Direction::Backwards, (float)dt);
  if (keysDown.contains(Qt::Key_A)) game->MoveCamera(fe::Direction::Left, (float)dt);
  if (keysDown.contains(Qt::Key_D)) game->MoveCamera(fe::Direction::Right, (float)dt);
  if (keysDown.contains(Qt::Key_Space)) game->MoveCamera(fe::Direction::Up, (float)dt);
  if (keysDown.contains(Qt::Key_Shift)) game->MoveCamera(fe::Direction::Down, (float)dt);
}

void EngineWidget::toggleTimer(bool enabled) {
  if (!timer) return;
  if (enabled) timer->start(16);
  else timer->stop();
}
