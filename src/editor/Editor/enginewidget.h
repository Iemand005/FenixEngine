#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

#include <QOpenGLWidget>

#define EXCLUDE_NETWORKING
#include <Game.hpp>

class EngineWidget : public QOpenGLWidget {



 public:
  EngineWidget();
  EngineWidget(QWidget* parent = nullptr);
  ~EngineWidget() {}

  void initializeGL() override;
};

#endif  // ENGINEWIDGET_H
