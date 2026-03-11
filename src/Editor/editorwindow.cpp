#include "editorwindow.h"

#include "ui_editorwindow.h"

#include <QPushButton>

EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::EditorWindow) {
  ui->setupUi(this);

  connect(ui->shaderButton, SIGNAL(clicked()), SLOT(compileShaders()));
}

EditorWindow::~EditorWindow() { delete ui; }

void EditorWindow::compileShaders() {

  ui->engineWidget->getGame()->shader->LoadShaderTexts(ui->vertexShaderTextEdit->toPlainText().toStdString(), ui->fragmentShaderTextEdit->toPlainText().toStdString());
}
