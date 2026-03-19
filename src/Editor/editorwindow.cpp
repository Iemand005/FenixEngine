#include "editorwindow.h"

#include "ui_editorwindow.h"

#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLContext>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef _WIN32
  #include <windows.h>
  #include <wingdi.h>
#endif

EditorWindow::EditorWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::EditorWindow) {
  ui->setupUi(this);

  connect(ui->shaderButton, SIGNAL(clicked()), SLOT(compileShaders()));

  ui->lightDirDial->setRange(0, 360);
  ui->lightDirDial->setWrapping(true);
  connect(ui->lightDirDial, &QDial::valueChanged, [&](int value) {
    auto game = ui->engineWidget->getGame();
    if (!game) return;
    float rad = glm::radians(static_cast<float>(value));
    auto lightCount = game->scene->GetLightCount();
    auto pointLights = game->scene->GetLights();

    float r = 5.0f;
    pointLights[0].position = glm::vec3(std::cos(rad) * r, 2.0f, std::sin(rad) * r);
    ui->engineWidget->update();
  });

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
      ui->engineWidget->makeCurrent();
      auto obj = game->LoadStaticOBJ(file.toStdString());
      ui->engineWidget->doneCurrent();
      game->scene->AddObject(obj);
      ui->engineWidget->update();
    }
  });

  connect(ui->autoRefreshBox, &QCheckBox::clicked, [&](bool checked) {
    ui->engineWidget->toggleTimer(checked);
  });

  connect(ui->wireframeBox, &QCheckBox::clicked, [&](bool checked) {
    ui->engineWidget->wireframe = checked;
    ui->engineWidget->update();
  });

  connect(ui->objectsButton, &QPushButton::clicked, [&]() {
    reloadModelList();
  });

  connect(ui->objectListView->selectionModel(), &QItemSelectionModel::selectionChanged, [&](const QItemSelection &selected, const QItemSelection &deselected) {
    QModelIndexList indexes = ui->objectListView->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
      QString text = indexes.first().data().toString();
      qDebug() << "Selected item:" << text;
    }
  });

  connect(ui->lightListWidget->selectionModel(), &QItemSelectionModel::selectionChanged, [&](const QItemSelection &selected, const QItemSelection &deselected) {
    QModelIndexList indexes = ui->objectListView->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
      QString text = indexes.first().data().toString();
      qDebug() << "Selected item:" << text;
    }
  });

  connect(ui->engineWidget, &EngineWidget::fpsUpdate, [&](float fps) {
    ui->statusbar->showMessage(QString("FPS: %1 Frames rendered: %2").arg(fps).arg(ui->engineWidget->renderedFrames));
  });
}

EditorWindow::~EditorWindow() { delete ui; }

bool EditorWindow::event(QEvent* event) {
  if (event->type() == QEvent::WindowDeactivate) {
    ui->engineWidget->stopMouseCapture();
  }

  return QMainWindow::event(event);
}

void EditorWindow::reloadModelList() {
  auto game = ui->engineWidget->getGame();
  auto objects = game->scene->GetObjects();
  auto model = new QStringListModel;
  for (auto &object : objects) {
    auto name = object->GetName();
    int row = model->rowCount();

    model->insertRow(row);
    QModelIndex index = model->index(row);
    model->setData(index, name.c_str());

  }

  ui->objectListView->setModel(model);
  connect(ui->objectListView->selectionModel(), &QItemSelectionModel::selectionChanged, [&](const QItemSelection &selected, const QItemSelection &deselected) {
    QModelIndexList indexes = ui->objectListView->selectionModel()->selectedIndexes();
    if (!indexes.isEmpty()) {
      QString text = indexes.first().data().toString();
      qDebug() << "Selected item:" << text;
      auto i =indexes.first();
      auto game = ui->engineWidget->getGame();
      int row = i.row();
      auto objects = game->scene->GetObjects();
      auto objec = objects[row];
      selectedObject = objec.get();
      auto pos = selectedObject->state.position;
      auto rot = selectedObject->state.position;

      ui->xPosDial->setValue(pos.x);
      ui->yPosDial->setValue(pos.y);
      ui->zPosDial->setValue(pos.z);

      ui->xRotDial->setValue(rot.x);
      ui->yRotDial->setValue(rot.y);
      ui->zRotDial->setValue(rot.z);
    }
  });


  connect(ui->xPosDial, &QDial::valueChanged, [&]() { selectedObject->state.position.x = ui->xPosDial->value(); });
  connect(ui->yPosDial, &QDial::valueChanged, [&]() { selectedObject->state.position.y = ui->yPosDial->value(); });
  connect(ui->zPosDial, &QDial::valueChanged, [&]() { selectedObject->state.position.z = ui->zPosDial->value(); });
  connect(ui->xRotDial, &QDial::valueChanged, [&]() { selectedObject->state.rotation.x = ui->xRotDial->value(); });
  connect(ui->yRotDial, &QDial::valueChanged, [&]() { selectedObject->state.rotation.y = ui->yRotDial->value(); });
  connect(ui->zRotDial, &QDial::valueChanged, [&]() { selectedObject->state.rotation.z = ui->zRotDial->value(); });///..//file:///C:/Users/Lasse/AppData/Local/Packages/Microsoft.ScreenSketch_8wekyb3d8bbwe/TempState/Recordings/20260312-1750-48.4422010.mp4

  connect(ui->xPosDial, SIGNAL(valueChanged()), this, SLOT(updateSelectedObjectState()));


  for (auto &a: {ui->xPosDial, ui->yPosDial, ui->zPosDial, ui->xRotDial, ui->yRotDial, ui->zRotDial})
    connect(a, SIGNAL(valueChanged()), this, SLOT(updateSelectedObjectState()));

}

void EditorWindow::updateSelectedObjectState() {
  selectedObject->state.position.x = ui->xPosDial->value();
  selectedObject->state.position.y = ui->yPosDial->value();
  selectedObject->state.position.z = ui->zPosDial->value();

  selectedObject->state.rotation.x = ui->xRotDial->value();
  selectedObject->state.rotation.y = ui->yRotDial->value();
  selectedObject->state.rotation.z = ui->zRotDial->value();
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
