#include "editorwindow.h"

#include "ui_editorwindow.h"

#include <QPushButton>
#include <QMessageBox>

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

  });
}

EditorWindow::~EditorWindow() { delete ui; }

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
