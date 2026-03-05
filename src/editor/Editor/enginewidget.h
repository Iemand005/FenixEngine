#ifndef ENGINEWIDGET_H
#define ENGINEWIDGET_H

#include <QOpenGLWidget>

class EngineWidget : public QOpenGLWidget {
 public:
  EngineWidget();

  void initializeGL() override;
};

#endif  // ENGINEWIDGET_H
