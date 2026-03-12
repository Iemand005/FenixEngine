#include "editorwindow.h"

#include "ui_editorwindow.h"

#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>

EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::EditorWindow) {
  ui->setupUi(this);

  connect(ui->shaderButton, SIGNAL(clicked()), SLOT(compileShaders()));

  connect(ui->xrButton, &QPushButton::clicked, [&]() {
    fe::XRGame *game = ui->engineWidget->getGame();
    game->EnableXR();
  });

  connect(ui->modelButton, &QPushButton::clicked, [&]() {
    fe::XRGame *game = ui->engineWidget->getGame();
    // game->LoadObj();
    QFileDialog dialog;
    if (dialog.exec() == QFileDialog::Accepted) {
      QString file;
      dialog.fileSelected(file);
      game->LoadObj(file.toStdString());
    }
  });

  connect(ui->autoRefreshBox, &QCheckBox::clicked, [&](bool checked) {
    ui->engineWidget->toggleTimer(checked);
  });

  connect(ui->wireframeBox, &QCheckBox::clicked, [&](bool checked) {
    auto game = ui->engineWidget->getGame();
    auto fbo = ui->engineWidget->defaultFramebufferObject();
    game ->BindFrameBuffer(fbo);
    game->ToggleWireframe(checked);
    ui->engineWidget->wireframe = checked;
  });
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
