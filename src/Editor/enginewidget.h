#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

// #include <glad/glad.h>


#define EXCLUDE_NETWORKING
#define FE_EXCLUDE_SDL
#define FE_EXCLUDE_GLFW
#include <XRGame.hpp>

#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QTimer>

// template<typename GameT = fe::Game>
class EngineWidget : public QOpenGLWidget {

  std::unique_ptr<fe::XRGame> game;

  bool capturing = false;

 public:
  EngineWidget();
  EngineWidget(QWidget* parent = nullptr);
  ~EngineWidget() {}

  fe::XRGame *getGame() { return game.get(); }

  void startMouseCapture();
  void stopMouseCapture();

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void mouseMoveEvent(QMouseEvent* e) override;
  void mousePressEvent(QMouseEvent* e) override;
  void keyPressEvent(QKeyEvent *event) override;

  void toggleTimer(bool enabled = true);

  bool wireframe = false;
  unsigned long long renderedFrames = 0;
 public slots:
 private:
  QTimer* timer = nullptr;

};

#endif  // ENGINEWIDGET_H
