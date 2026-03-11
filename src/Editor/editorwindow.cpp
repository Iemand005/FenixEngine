#include "editorwindow.h"

#include "ui_editorwindow.h"

#include <QPushButton>

EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::EditorWindow) {
  ui->setupUi(this);

  connect(ui->shaderButton, &QPushButton::clicked, []() {

  });
}

EditorWindow::~EditorWindow() { delete ui; }

void EditorWindow::compileShaders() {
  // engineWidget->getGame()->shader->LoadShaders("", "");
  // this->engineWidget()

  auto game = this->ui->engineWidget->getGame();
  // this->ui->shaderButton()

  ui->engineWidget->getGame()->shader->LoadShaderTexts("", "");
}
