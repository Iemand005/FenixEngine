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

class EngineWidget : public QOpenGLWidget {

  std::unique_ptr<fe::XRGame> game;

  bool capturing = false;

 public:
  EngineWidget();
  EngineWidget(QWidget* parent = nullptr);
  ~EngineWidget() {}

  XRGame *game() { return game; }

  void startMouseCapture();
  void stopMouseCapture();

  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  void mouseMoveEvent(QMouseEvent* e) override;
  void mousePressEvent(QMouseEvent* e) override;
  void keyPressEvent(QKeyEvent *event) override;

 // public slots:
 //  void compileShaders();

};

#endif  // ENGINEWIDGET_H
