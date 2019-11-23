#include "MainWindow.hpp"
#include <QApplication>

#include <QtPlugin>
#include <QSettings>

#include "Log.hpp"
#include "Utils.hpp"

#if defined(_WIN32)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif
Q_IMPORT_PLUGIN(QDDSPlugin)
//Q_IMPORT_PLUGIN(QICNSPlugin)
//Q_IMPORT_PLUGIN(QICOPlugin)
Q_IMPORT_PLUGIN(QTgaPlugin)
//Q_IMPORT_PLUGIN(QTiffPlugin)
//Q_IMPORT_PLUGIN(QWbmpPlugin)
//Q_IMPORT_PLUGIN(QWebpPlugin)

int main(int argc, char *argv[])
{
  qRegisterMetaType<std::size_t>("std::size_t");
  qRegisterMetaType<std::function<void()>>("std::function<void()>");
  qRegisterMetaType<QList<QPersistentModelIndex>>("QList<QPersistentModelIndex>");
  qRegisterMetaType<QAbstractItemModel::LayoutChangeHint>("QAbstractItemModel::LayoutChangeHint");
  qRegisterMetaType<Qt::Orientation>("Qt::Orientation");

  QApplication a(argc, argv);
  Utils::AssignIconAxpFile();

  // Assign
//  QString appFilePath = QCoreApplication::applicationFilePath();
//  qDebug() << appFilePath;
//  QFileInfo appFileInfo(appFilePath);
//  QString appName = appFileInfo.fileName();
//  QSettings settings("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
//  settings.beginGroup(".axp");
//  settings.setValue("(Default)", appName);
//  settings.setValue("DefaultIcon/(Default)", QString(QDir::toNativeSeparators(appFilePath) + ",1"));
//  settings.endGroup();
//  settings.beginGroup(appName);
//  settings.setValue("DefaultIcon/(Default)", QString(QDir::toNativeSeparators(appFilePath) + ",1"));
//  settings.endGroup();
//  Utils::AssocNoftify();

  MainWindow w;
  w.show();

  return a.exec();
}
