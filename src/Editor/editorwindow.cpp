#include "editorwindow.h"

#include "ui_editorwindow.h"

#include <QPushButton>

EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::EditorWindow) {
  ui->setupUi(this);

  connect(ui->shaderButton, SIGNAL(clicked()), SLOT(compileShaders()));

  connect(ui->xrButton, &QPushButton::clicked, [&]() {
    fe::XRGame *game = ui->engineWidget->getGame();
    game->EnableXR();
  });
}

EditorWindow::~EditorWindow() { delete ui; }

void EditorWindow::compileShaders() {

  // ui->engineWidget->getGame()->shader->LoadShaderTexts(ui->vertexShaderTextEdit->toPlainText().toStdString(), ui->fragmentShaderTextEdit->toPlainText().toStdString());

  try {
    bool success = ui->engineWidget->getGame()->LoadShaderTexts(ui->vertexShaderTextEdit->toPlainText().toStdString(), ui->fragmentShaderTextEdit->toPlainText().toStdString());
  } catch (std::exception ex) {
    std::cout << ex.what();
  }

  // ui->engineWidget->getGame()->Redraw();
  update();
  ui->engineWidget->update();
}
