#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

// #include <glad/glad.h>


#define EXCLUDE_NETWORKING
#define FE_EXCLUDE_SDL
#define FE_EXCLUDE_GLFW
#include <Game.hpp>

#include <QOpenGLWidget>
#include <QOpenGLContext>

class EngineWidget : public QOpenGLWidget {

  std::unique_ptr<fe::Game> game;

 public:
  EngineWidget();
  EngineWidget(QWidget* parent = nullptr);
  ~EngineWidget() {}

  void initializeGL() override;
};

#endif  // ENGINEWIDGET_H
