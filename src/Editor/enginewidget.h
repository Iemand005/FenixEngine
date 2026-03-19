#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

// #include <glad/glad.h>


#define EXCLUDE_NETWORKING
#define FE_EXCLUDE_SDL
#define FE_EXCLUDE_GLFW

#include <EditableGameBase.hpp>

#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QSet>
#include <QElapsedTimer>
#include <QTimer>

// template<typename GameT = fe::Game>
class EngineWidget : public QOpenGLWidget {
  Q_OBJECT

  std::unique_ptr<fe::EditableGameBase> game;

  bool capturing = false;

 public:
  EngineWidget();
  EngineWidget(QWidget* parent = nullptr);
  ~EngineWidget() {}

  fe::EditableGameBase *getGame() { return game.get(); }

  void startMouseCapture();
  void stopMouseCapture();
  void processInput(double dt);

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void mouseMoveEvent(QMouseEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;

  void toggleTimer(bool enabled = true);

  bool wireframe = false;
  unsigned long long renderedFrames = 0;
 signals:
  void fpsUpdate(float fps);
 public slots:
 private:
  QTimer* timer = nullptr;
  QElapsedTimer frameTimer;
  qint64 lastFrameNs = 0;
  QSet<int> keysDown;

};

#endif  // ENGINEWIDGET_H
