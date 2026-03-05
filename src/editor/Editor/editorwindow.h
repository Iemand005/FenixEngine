#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#include <QMainWindow>
#include <QtOpenGLWidgets/QtOpenGLWidgets>

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

 private:
  Ui::EditorWindow* ui;
  QOpenGLWidget e;
};
#endif  // EDITORWINDOW_H
