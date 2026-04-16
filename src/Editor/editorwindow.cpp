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

  mjpegServer = new MjpegServer(this);
  mjpegServer->startServer(8080);

  connect(ui->shaderButton, SIGNAL(clicked()), SLOT(compileShaders()));

  ui->lightDirDial->setRange(0, 360);
  ui->lightDirDial->setWrapping(true);
  connect(ui->lightDirDial, &QDial::valueChanged, [&](int value) {
    auto game = this->game();
    if (!game) return;
    float rad = glm::radians(static_cast<float>(value));
    auto lightCount = game->scene->GetLightCount();
    auto pointLights = game->scene->GetLights();

    float r = 5.0f;
    pointLights[0].position = glm::vec3(std::cos(rad) * r, 2.0f, std::sin(rad) * r);
    ui->engineWidget->update();
  });

  connect(ui->xrButton, &QPushButton::clicked, [&]() {
#ifdef _WIN32
    HDC hdc = wglGetCurrentDC();
    HGLRC hglrc = wglGetCurrentContext();
    game()->initOpenXR(hdc, hglrc);
#endif
    game()->EnableXR();
  });

  connect(ui->modelButton, &QPushButton::clicked, [&]() {
    QString file = QFileDialog::getOpenFileName(this, "Open Model", "", "Models (*.obj);;All Files (*)");
    if (!file.isEmpty()) {
      ui->engineWidget->makeCurrent();
      auto obj = game()->LoadStaticOBJ(file.toStdString());
      ui->engineWidget->doneCurrent();
      game()->scene->AddObject(obj);
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

  connect(ui->addLightButton, &QPushButton::clicked, [&]() {
    ui->engineWidget->getGame()->scene->AddLight();
  });

  connect(ui->lightListWidget, &QListWidget::currentRowChanged, [&](int row) {
    qDebug() << "Selected item:" << row;
    game()->SelectLightByIndex(row);
  });

  connect(ui->engineWidget, &EngineWidget::fpsUpdate, [&](float fps) {
    ui->statusbar->showMessage(QString("FPS: %1 Frames rendered: %2").arg(fps).arg(ui->engineWidget->renderedFrames));
  });

  connect(ui->loadLevelButton, &QPushButton::clicked, [&]() {
    game()->LoadLevel();
  });

  connect(ui->saveButton, &QPushButton::clicked, [&]() {
    game()->SaveLevel();
  });

  connect(ui->sendFrameButton, &QPushButton::clicked, [&]() {
    QImage frame = ui->engineWidget->grabFramebuffer();

    mjpegServer->sendFrame(frame);
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

      this->game()->SelectObjectByIndex(row);
    } else this->game()->UnselectObject();
  });


  connect(ui->xPosDial, &QDial::valueChanged, [&]() { selectedObject->state.position.x = ui->xPosDial->value() / 10.0f; });
  connect(ui->yPosDial, &QDial::valueChanged, [&]() { selectedObject->state.position.y = ui->yPosDial->value() / 10.0f; });
  connect(ui->zPosDial, &QDial::valueChanged, [&]() { selectedObject->state.position.z = ui->zPosDial->value() / 10.0f; });
  connect(ui->xRotDial, &QDial::valueChanged, [&]() { selectedObject->state.rotation.x = ui->xRotDial->value() / 10.0f; });
  connect(ui->yRotDial, &QDial::valueChanged, [&]() { selectedObject->state.rotation.y = ui->yRotDial->value() / 10.0f; });
  connect(ui->zRotDial, &QDial::valueChanged, [&]() { selectedObject->state.rotation.z = ui->zRotDial->value() / 10.0f; });

  connect(ui->xPosDial, SIGNAL(valueChanged()), this, SLOT(updateSelectedObjectState()));


  for (auto &a: {ui->xPosDial, ui->yPosDial, ui->zPosDial, ui->xRotDial, ui->yRotDial, ui->zRotDial})
    connect(a, SIGNAL(valueChanged()), this, SLOT(updateSelectedObjectState()));


  // Load lights

  ui->lightListWidget->clear();

  auto lightCount = game->scene->GetLightCount();
  // auto lights = game->scene->GetLights();
  for (int i = 0; i < lightCount; ++i)
    new QListWidgetItem(tr("Light"), ui->lightListWidget);
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

fe::EditableGameBase *EditorWindow::game() {
  return ui->engineWidget->getGame();
}
