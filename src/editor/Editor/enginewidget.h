#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

#include <glad/glad.h>
#include <QOpenGLWidget>
#include <QOpenGLContext>

#define EXCLUDE_NETWORKING
#define FE_EXCLUDE_SDL
#define FE_EXCLUDE_GLFW
#include <Game.hpp>

class EngineWidget : public QOpenGLWidget {

  std::unique_ptr<fe::Game> game;

 public:
  EngineWidget();
  EngineWidget(QWidget* parent = nullptr);
  ~EngineWidget() {}

  void initializeGL() override;
};

#endif  // ENGINEWIDGET_H
