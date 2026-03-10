#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

// #include <glad/glad.h>


#define EXCLUDE_NETWORKING
#define FE_EXCLUDE_SDL
#define FE_EXCLUDE_GLFW
#include <Game.hpp>

#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QMouseEvent>

class EngineWidget : public QOpenGLWidget {

  std::unique_ptr<fe::Game> game;

  bool capturing = false;

 public:
  EngineWidget();
  EngineWidget(QWidget* parent = nullptr);
  ~EngineWidget() {}

  void startMouseCapture();
  void stopMouseCapture();

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void mouseMoveEvent(QMouseEvent* e) override;
  void mousePressEvent(QMouseEvent* e) override;
};

#endif  // ENGINEWIDGET_H
