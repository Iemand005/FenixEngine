#ifndef EDITORWINDOW_H
#define EDITORWINDOW_H

#define EXCLUDE_NETWORKING
#define FE_EXCLUDE_SDL
#define FE_EXCLUDE_GLFW
#include <Game.hpp>

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

  void reloadModelList() {
    auto game = ui->engineWidget->getGame();
  }

 protected:
  bool event(QEvent* event) override;

 public slots:
  void compileShaders();

 private:
  Ui::EditorWindow* ui;
  QOpenGLWidget e;
  QTimer* timer = nullptr;
};
#endif  // EDITORWINDOW_H
