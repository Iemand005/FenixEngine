#include "editorwindow.h"

#include "ui_editorwindow.h"

#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLContext>

#ifdef _WIN32
  #include <windows.h>
  #include <wingdi.h>
#endif

EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::EditorWindow) {
  ui->setupUi(this);

  connect(ui->shaderButton, SIGNAL(clicked()), SLOT(compileShaders()));

  connect(ui->xrButton, &QPushButton::clicked, [&]() {
    fe::XRGame *game = ui->engineWidget->getGame();
#ifdef _WIN32
    // auto native = QOpenGLContext::currentContext()->nativeHandle();
    // QWGLNativeContext wgl = native.value<QWGLNativeContext>();
    // HDC hdc = wgl.hdc();
    // HGLRC hglrc = wgl.context();

    HDC hdc = wglGetCurrentDC();
    HGLRC hglrc = wglGetCurrentContext();
    game->initOpenXR(hdc, hglrc);
#endif
    game->EnableXR();
  });

  connect(ui->modelButton, &QPushButton::clicked, [&]() {
    fe::XRGame *game = ui->engineWidget->getGame();
    QString file = QFileDialog::getOpenFileName(this, "Open Model", "", "Models (*.obj);;All Files (*)");
    if (!file.isEmpty()) {
      game->LoadObj(file.toStdString());
    }
  });

  connect(ui->autoRefreshBox, &QCheckBox::clicked, [&](bool checked) {
    ui->engineWidget->toggleTimer(checked);
  });

  connect(ui->wireframeBox, &QCheckBox::clicked, [&](bool checked) {
    ui->engineWidget->wireframe = checked;
    ui->engineWidget->update();
  });

  // timer = new QTimer(this);
  // QObject::connect(timer, &QTimer::timeout, [&]() {
  //   auto game = ui->engineWidget->getGame();
  //   ui->statusbar->showMessage(QString("FPS: %1 Frames rendered: %2").arg(game->GetFPS()).arg(ui->engineWidget->renderedFrames));
  //   QMetaObject::invokeMethod(qApp, [&]() {
  //     update();
  //     ui->engineWidget->repaint();
  //   });
  // });

  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, [&]() {
    QMetaObject::invokeMethod(ui->engineWidget, "update");
    auto game = ui->engineWidget->getGame();
    ui->statusbar->showMessage(QString("FPS: %1 Frames rendered: %2").arg(game->GetFPS()).arg(ui->engineWidget->renderedFrames));
  });
  timer->start(0);

  // timer->start(0);
}

EditorWindow::~EditorWindow() { delete ui; }

bool EditorWindow::event(QEvent* event) {
  if (event->type() == QEvent::WindowDeactivate) {
    ui->engineWidget->stopMouseCapture();
  }

  return QMainWindow::event(event);
}

void EditorWindow::compileShaders() {

  try {
    bool success = ui->engineWidget->getGame()->LoadShaderTexts(ui->vertexShaderTextEdit->toPlainText().toStdString(), ui->fragmentShaderTextEdit->toPlainText().toStdString());
  } catch (std::exception ex) {
    std::cout << ex.what();
    QMessageBox messageBox;
    messageBox.setText("Failed to compile shaders.");
    messageBox.setInformativeText(ex.what());
    messageBox.setIcon(QMessageBox::Critical);
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.setDefaultButton(QMessageBox::Ok);
    messageBox.exec();
  }

  update();
  ui->engineWidget->update();
}
