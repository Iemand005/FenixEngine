#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#define EXCLUDE_NETWORKING
#define FE_EXCLUDE_SDL
#define FE_EXCLUDE_GLFW

#include <EditableGameBase.hpp>

#include <QMainWindow>
#include <QtOpenGLWidgets/QtOpenGLWidgets>

#include "mjpegserver.h"

// namespace fe{ class XRGame; }

QT_BEGIN_NAMESPACE
namespace Ui {
class EditorWindow;
}
QT_END_NAMESPACE

class EditorWindow : public QMainWindow {
  Q_OBJECT

 public:
  EditorWindow(QWidget* parent = nullptr);
  ~EditorWindow();

  void reloadModelList();


  fe::EditableGameBase *game();

 protected:
  bool event(QEvent* event) override;

 public slots:
  void compileShaders();
  void updateSelectedObjectState();

 private:
  Ui::EditorWindow* ui;
  QOpenGLWidget e;
  QTimer* timer = nullptr;
  fe::Object *selectedObject;
  MjpegServer *mjpegServer;
};
#endif  // EDITORWINDOW_H
