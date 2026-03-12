#include <QApplication>
#include <QLocale>
#include <QTranslator>

#include "editorwindow.h"

int main(int argc, char* argv[]) {

  QSurfaceFormat format;
  format.setDepthBufferSize(24);
  format.setStencilBufferSize(8);
  format.setVersion(3, 3);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSwapInterval(0);
  QSurfaceFormat::setDefaultFormat(format);

  QApplication a(argc, argv);

  QTranslator translator;
  const QStringList uiLanguages = QLocale::system().uiLanguages();
  for (const QString& locale : uiLanguages) {
    const QString baseName = "Editor_" + QLocale(locale).name();
    if (translator.load(":/i18n/" + baseName)) {
      a.installTranslator(&translator);
      break;
    }
  }
  EditorWindow w;
  w.show();
  return a.exec();
}
