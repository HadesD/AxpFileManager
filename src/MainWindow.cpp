#include "MainWindow.hpp"
#include "ui_MainWindow.h"

#include <QDirModel>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QProcess>
#include <QLocale>
#include <QMutex>
#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include <QDirIterator>
#include <QList>
#include <QSysInfo>

#include "AxpArchive.hpp"
#include "AxpArchivePort.hpp"

#include "AboutDialog.hpp"
#include "AxpItem.hpp"
#include "AxpItemModel.hpp"
#include "Log.hpp"
#include "Utils.hpp"
#include "LaunchExtendsUtils.hpp"

MainWindow* MainWindow::s_instance = nullptr;

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  s_instance = this;

  LOG_DEBUG(__FUNCTION__ << QThread::currentThread());
  ui->setupUi(this);
  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);

  m_axpArchive = new AxpArchivePort;
  m_dirModel = new QStandardItemModel(ui->dirList);
  m_fileModel = new QStandardItemModel(ui->fileList);
  m_fileModel->setSortRole(AxpItem::ItemTypeRole);
  m_fileModel->setHorizontalHeaderLabels(QStringList() << "Name" << "Size" << "Status");

  ui->dirList->setModel(m_dirModel);
  ui->fileList->setModel(m_fileModel);

  ui->fileList->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);

  connect(this, &MainWindow::invoke, this, &MainWindow::invokeCallback);

  const auto& args = QCoreApplication::arguments();

  if (args.size() > 1) {
    const QString& opennedPath = args.at(1);

    QTimer::singleShot(0, [this, opennedPath](){
      this->openAxpArchive(opennedPath);
    });
  }
}

MainWindow::~MainWindow()
{
  LOG_DEBUG(__FUNCTION__ << "called");
  delete m_axpArchive;
  delete ui;
  s_instance = nullptr;
}

MainWindow *MainWindow::getInstance()
{
  return s_instance;
}

Ui::MainWindow* MainWindow::getUi() const
{
  return ui;
}

QString MainWindow::getSelectedDirAxpKey() const
{
  auto curIndex = ui->dirList->currentIndex();
  if (!curIndex.isValid()) return QString();

  auto data = curIndex.data(AxpItem::ItemKeyRole);

  return !data.isNull() && data.isValid() ? data.toString() : QString();
}

void MainWindow::openAxpArchive(const QString &fileName)
{
  if (fileName.isEmpty())
  {
    LOG(__FUNCTION__ << "You must input axp file name");
    return;
  }

  if (!this->closeOpenningAxp())
  {
    return;
  }

  m_axpArchive->setAxpArchiveFileName(fileName);
  m_axpArchive->setAxpArchiveFileEditable(false);

  m_axpArchive->setProgressCallback([this](auto fileName, auto cur, auto total) {
    QString qStringFileName = QString::fromLocal8Bit(fileName.data());
    emit this->invoke([=](){
      this->setProgress(qStringFileName, cur, total);
    });
    this->onAxpReadListProgress(qStringFileName, cur, total);
  });

  m_axpArchive->startOpenAxpArchive([=]() {
    emit this->invoke([=](){
      const auto& nativePath = QDir::toNativeSeparators(fileName);
      ui->workingPathLabel->setText("File: <a href=\""+nativePath+"\">"+nativePath+"</a>");
      ui->workingPathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    });

    QStandardItem* item = new AxpItem("/");
    item->setText(QFileInfo(fileName).fileName());
    item->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    m_dirModel->appendRow(item);
    auto rootIndex = m_dirModel->index(0, 0);
    ui->dirList->expand(rootIndex);
  },
  [=](){
    emit this->invoke([=](){
      if (auto errCode = m_axpArchive->getLastError() != AXP::AXP_ERRORS::AXP_ERR_SUCCESS)
      {
        LOG_DEBUG("Error:" << static_cast<int>(errCode) << m_axpArchive->getLastErrorMessage());
        QMessageBox::warning(this, "Error - Can not open file",
                             QString("Error Code: %1\nError Message: %2\nFile name: %3")
                             .arg(static_cast<int>(errCode))
                             .arg(m_axpArchive->getLastErrorMessage())
                             .arg(fileName));
        m_axpArchive->close();
        return;
      }

      ui->actionSave->setDisabled(false);
      ui->actionSave_As->setDisabled(false);
      ui->actionExtract_All_Data->setDisabled(false);
      ui->actionAdd_File->setDisabled(false);
      ui->actionAdd_Folder->setDisabled(false);
      ui->actionClose_openning_file->setDisabled(false);

      ui->saveBtn->setDisabled(false);
      ui->saveAsBtn->setDisabled(false);
      ui->addFileBtn->setDisabled(false);
      ui->addFolderBtn->setDisabled(false);
      ui->extractAllBtn->setDisabled(false);
//      ui->extractSelectedBtn->setDisabled(true);
    });

//    LaunchExtendsUtils::downloadLaunchExtends();
  });
}

void MainWindow::onAxpReadListProgress(const QString &fileName, const size_t current, const size_t total)
{
  Q_UNUSED(current);
  Q_UNUSED(total);

  auto dirs = fileName.split('/');

  QString fileBaseName = dirs.back();

  auto dirParent = m_dirModel->item(0, 0);
  if (!dirParent)
  {
    LOG(__FUNCTION__ << "Error: not found diParent." << fileName << current << total);
    return;
  }

  int dirSize = dirs.size() - 1;
  for (int i = 0; i < dirSize; ++i) {
    auto& pathPart = dirs.at(i);
    bool foundParent = false;
    int rowCount = dirParent->rowCount();
    for (int r = 0; r < rowCount; ++r) {
      auto child = dirParent->child(r);
      if (child->text() == pathPart) {
        dirParent = child;
        foundParent = true;
        break;
      }
    }
    if (!foundParent) {
      QString key;
      for (int j = 0; j <= i; ++j) {
        key += dirs[j] + '/';
      }
      auto newChild = new AxpItem(key);
      dirParent->appendRow(newChild);
      dirParent = newChild;
    }
  }
}

void MainWindow::setProgress(const QString &name, const std::size_t current, const std::size_t total)
{
  ui->progressBar->setMaximum(static_cast<int>(total));
  ui->progressBar->setValue(static_cast<int>(current));
  ui->messageLabel->setText(
        "[" + QString::number(current) + "/" + QString::number(total) + "]: " + name
        );
}

bool MainWindow::closeOpenningAxp()
{
  if (m_axpArchive->isModified())
  {
    auto retMsg = QMessageBox::question(this, "Confirm discard modified", "You have unsave changes!\n\nDo you want to discard?");
    if (retMsg != QMessageBox::StandardButton::Yes)
    {
      return false;
    }
  }

  ui->actionSave->setDisabled(true);
  ui->actionSave_As->setDisabled(true);
  ui->actionExtract_All_Data->setDisabled(true);
  ui->actionAdd_File->setDisabled(true);
  ui->actionAdd_Folder->setDisabled(true);
  ui->actionClose_openning_file->setDisabled(true);
  ui->saveBtn->setDisabled(true);
  ui->saveAsBtn->setDisabled(true);
  ui->addFileBtn->setDisabled(true);
  ui->addFolderBtn->setDisabled(true);
  ui->extractAllBtn->setDisabled(true);
  ui->extractSelectedBtn->setDisabled(true);

  // CLose first
  m_fileModel->removeRows(0, m_fileModel->rowCount());
  m_dirModel->clear();
  m_axpArchive->close();

  return true;
}

bool MainWindow::event(QEvent* event)
{
  if (event->type() ==  QEvent::Close)
  {
    if (!this->closeOpenningAxp())
    {
      event->ignore();
      return false;
    }
  }

  return QMainWindow::event(event);
}

void MainWindow::on_actionOpen_triggered()
{
  static QString opennedPath;
  opennedPath = QFileDialog::getOpenFileName(
        this, "Open File", opennedPath, tr("Axp archive (*.axp);;All Files (*)")
        );
  this->openAxpArchive(opennedPath);
}

void MainWindow::on_actionAbout_triggered()
{
  QMessageBox::information(this, "About us", "Axp File Manager Application\n\nCopyright (c) Dark.Hades");
}

void MainWindow::on_actionNew_triggered()
{
  static QString opennedPath;
  opennedPath = QFileDialog::getSaveFileName(
        this, tr("Create new Axp archive file"), opennedPath,
        tr("Axp archive (*.axp);;All Files (*)"));
  if (opennedPath.isEmpty())
  {
    return;
  }
  m_axpArchive->close();

  if (!m_axpArchive->saveToDiskFile(opennedPath.toLocal8Bit().data()))
  {
    return;
  }

  this->openAxpArchive(opennedPath);
}

void MainWindow::setCurrentDir(const QModelIndex &index)
{
  LOG_DEBUG(__FUNCTION__ << "called");

  ui->dirList->setDisabled(true);
  QThread* setCurDirThread = new QThread();
  connect(setCurDirThread, &QThread::started, [=](auto) {
    LOG_DEBUG("setCurrentDir::Thread" << "called");
    m_fileModel->removeRows(0, m_fileModel->rowCount());

    auto rootIndex = ui->dirList->model()->index(0, 0);
    bool isRoot = (index == rootIndex);

    QString parentKey = this->getSelectedDirAxpKey();

    auto& fileList = m_axpArchive->getFileList();

    bool startedFound = false;
    QMap<QString, bool> addedItems;
    for (const auto& fileInfo : fileList) {
      auto& fKey = fileInfo.first;
      QString local8BitFileName = QString::fromLocal8Bit(fKey.c_str());
      if (isRoot || local8BitFileName.indexOf(parentKey) == 0) {
        auto keySize = isRoot ? 0 : parentKey.size();
        QString itemPathName = local8BitFileName.mid(keySize);
        int indexOfSlash = itemPathName.indexOf('/');
        QString itemName = itemPathName.mid(0, indexOfSlash);
        if (addedItems.count(itemName)) {
          continue;
        }
        addedItems[itemName] = true;
        bool isDir = (indexOfSlash != -1);
        QList<QStandardItem*> items;
        items.reserve(m_fileModel->columnCount());

        auto item = new AxpItem(isDir ? local8BitFileName.mid(0, keySize + itemName.size() + 1) : local8BitFileName);
        items.append(item);

        auto sizeItem = new QStandardItem();
#if !defined(QT_DEBUG)
#endif
        if (!isDir)
        {
          sizeItem->setText(
                QLocale(QLocale::English).toString(static_cast<double>(fileInfo.second.size), 'f', 0) + " B"
                );
        }
        sizeItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        items.append(sizeItem);

        auto statusItem = new QStandardItem();
        QString statusTxt;
        switch (fileInfo.second.status)
        {
          case AxpArchivePort::FileListData::FileStatus::NEW:
            statusItem->setForeground(Qt::green);
            statusTxt = "New";
            break;

          case AxpArchivePort::FileListData::FileStatus::DELETED:
            statusItem->setForeground(Qt::red);
            statusTxt = "Deleted";
            break;

          case AxpArchivePort::FileListData::FileStatus::MODIFIED:
            statusItem->setForeground(Qt::yellow);
            statusTxt = "Modified";
            break;

          case AxpArchivePort::FileListData::FileStatus::ORIGIN:
            statusItem->setForeground(Qt::black);
            statusTxt = "Origin";
            break;

          default:
            statusTxt = "Unknown";
        }
        statusItem->setText(statusTxt);
        statusItem->setTextAlignment(Qt::AlignCenter);
        items.append(statusItem);

        m_fileModel->appendRow(items);
        startedFound = true;
      } else {
        if (startedFound) {
          break;
        }
      }
    }

    m_fileModel->sort(0);
    setCurDirThread->quit();
  });
  connect(setCurDirThread, &QThread::finished, [=]() {
    setCurDirThread->deleteLater();
  });
  connect(setCurDirThread, &QThread::destroyed, [=]() {
    ui->dirList->setDisabled(false);
    LOG_DEBUG("setCurrentDir::Thread" << "completed");
  });

  setCurDirThread->start();
  LOG_DEBUG(__FUNCTION__ << "completed!");
}

void MainWindow::on_actionExtract_All_Data_triggered()
{
  static QString opennedPath;
  opennedPath = QFileDialog::getExistingDirectory(this, "Choose folder to save extract file(s)", opennedPath);
  if (opennedPath.isEmpty()) {
    return;
  }

  auto m_extractItemListThread = new QThread(this);
  connect(m_extractItemListThread, &QThread::started, [=](){
    LOG_DEBUG("Extract ThreadID:" << QThread::currentThread());
    QFileInfo fileInfo(m_axpArchive->getArchiveFileName());
    QDir targetDir(opennedPath + '/' + fileInfo.fileName() + "-Output");
    if (!targetDir.exists())
    {
      targetDir.mkpath(".");
    }

    auto& m_fileList = m_axpArchive->getFileList();

    uint32_t i = 0;
    for (const auto& fileInfo : m_fileList)
    {
      ++i;
      const auto fKeyName = fileInfo.first;
      const auto& fname = QString::fromLocal8Bit(fKeyName.c_str());
      emit this->invoke([=]() {
        this->setProgress(fname, i, static_cast<uint>(m_fileList.size()));
      });

      auto toFileName(targetDir.filePath(fname));

      if (!m_axpArchive->extractToDisk(fKeyName, toFileName.toLocal8Bit().data())) {
        LOG_DEBUG("Write error");
      }
    }
    LOG("Extracted resources!");

    if (targetDir.exists()) {
      QDesktopServices::openUrl(targetDir.path());
    }
    m_extractItemListThread->quit();
  });
  m_extractItemListThread->start();
}

void MainWindow::on_actionExit_triggered()
{
  if (this->closeOpenningAxp())
  {
    this->close();
  }
}

void MainWindow::on_workingPathLabel_linkActivated(const QString &link)
{
  QProcess::startDetached("explorer.exe", (QStringList() << "/select," << link));
}

void MainWindow::on_actionAdd_File_triggered()
{
  LOG_DEBUG(__FUNCTION__ << "called");
  auto opennedPaths = QFileDialog::getOpenFileNames(
        this, "Choose file(s) to add", nullptr, "All files (*)"
        );
  LOG_DEBUG(__FUNCTION__ << opennedPaths);

  auto& fileList = m_axpArchive->getFileList();
  for (const auto& q_diskFileName : opennedPaths)
  {
    auto selectedPath = this->getSelectedDirAxpKey();
    QFileInfo fileInfo(q_diskFileName);
    if (!fileInfo.exists())
    {
      continue;
    }
    AxpArchivePort::FileName diskFileName = q_diskFileName.toLocal8Bit().data();
    AxpArchivePort::FileName fileName = ((selectedPath == "") || (selectedPath == "/"))
        ? fileInfo.fileName().toLocal8Bit().data()
        : QDir(selectedPath).filePath(fileInfo.fileName()).toLocal8Bit().data();
    auto& fileListItem = fileList[fileName];
    switch (fileListItem.status)
    {
      case AxpArchivePort::FileListData::FileStatus::UNKNOWN:
      case AxpArchivePort::FileListData::FileStatus::NEW:
        fileListItem.status = AxpArchivePort::FileListData::FileStatus::NEW;
        break;

      default:
        fileListItem.status = AxpArchivePort::FileListData::FileStatus::MODIFIED;
    }

    fileListItem.nameFromDisk = diskFileName;
    fileListItem.size = AXP::getDiskFileSize(diskFileName.data());
  }

  // Update list view
  this->setCurrentDir(ui->dirList->currentIndex());
  LOG_DEBUG(__FUNCTION__ << "completed");
}

void MainWindow::on_actionAdd_Folder_triggered()
{
  auto opennedPaths = QFileDialog::getExistingDirectory(this, "Choose folder to add");
  if (opennedPaths.isEmpty())
  {
    return;
  }

  LOG_DEBUG(__FUNCTION__ << opennedPaths);

  QThread* loadThread = new QThread;
  connect(loadThread, &QThread::started, [=](){
    QStringList pathArr = opennedPaths.split('/');
    const auto& dirBaseName = pathArr.back();
    const auto opennedPathSize = opennedPaths.size();

    auto& fileList = m_axpArchive->getFileList();
    QDirIterator itDir(opennedPaths, QDir::Files, QDirIterator::Subdirectories);
    while (itDir.hasNext())
    {
      const auto& diskFileName = itDir.next();
      const AxpArchivePort::FileName fileName = (dirBaseName + diskFileName.mid(opennedPathSize)).toLocal8Bit().data();
      LOG_DEBUG(__FUNCTION__ << fileName.data());

      auto& fileListData = fileList[fileName];
      switch (fileListData.status)
      {
        case AxpArchivePort::FileListData::FileStatus::UNKNOWN:
        case AxpArchivePort::FileListData::FileStatus::NEW:
          fileListData.status = AxpArchivePort::FileListData::FileStatus::NEW;
          break;

        default:
          fileListData.status = AxpArchivePort::FileListData::FileStatus::MODIFIED;
      }
      fileListData.nameFromDisk = diskFileName.toLocal8Bit().data();
      fileListData.size = AXP::getDiskFileSize(diskFileName.toLocal8Bit().data());
      this->onAxpReadListProgress(QString::fromLocal8Bit(fileName.data()), 0, 0);
    }
    loadThread->quit();
  });

  connect(loadThread, &QThread::finished, [=](){
    emit this->invoke([=](){
      this->setCurrentDir(ui->dirList->currentIndex());
    });
    loadThread->deleteLater();
  });

  loadThread->start();
}

void MainWindow::on_actionSave_As_triggered()
{
  auto opennedPaths = QFileDialog::getSaveFileName(this, "Save File...", QFileInfo(m_axpArchive->getArchiveFileName()).fileName());
  QThread* saveThread = new QThread;
  connect(saveThread, &QThread::started, [=](){
    if (m_axpArchive->saveToDiskFile(opennedPaths.toLocal8Bit().data()))
    {
      QDesktopServices::openUrl(QFileInfo(opennedPaths).path());
    }
    else
    {
      LOG(__FUNCTION__ << m_axpArchive->getLastErrorMessage());
    }
    saveThread->deleteLater();
    saveThread->quit();
  });
  saveThread->start();
}

void MainWindow::on_actionNew_From_directory_triggered()
{
  static QString opennedPath;
  opennedPath = QFileDialog::getExistingDirectory(this, "Choose folder to create new Axp", QDir(opennedPath).filePath(".."));
  if (opennedPath.isEmpty())
  {
    return;
  }

  auto fileBaseName = Utils::basename(opennedPath);
  static constexpr auto fileExt = ".axp";
  static QString opennedPathSave;
  if (opennedPathSave.isEmpty())
  {
    QDir::setCurrent(opennedPath);
  }
  opennedPathSave = QFileDialog::getSaveFileName(
        this, "Save File...",
        fileBaseName.indexOf(fileExt) != -1 ? fileBaseName : (fileBaseName + fileExt)
          );
  if (opennedPathSave.isEmpty())
  {
    return;
  }
  QDir::setCurrent(QDir(opennedPathSave).filePath(".."));

  m_axpArchive->close();

  QThread* saveThread = new QThread;
  connect(saveThread, &QThread::started, [=](){
    QStringList pathArr = opennedPath.split('/');
    const auto opennedPathSize = opennedPath.size();

    auto& fileList = m_axpArchive->getFileList();
    QDirIterator itDir(opennedPath, QDir::Files, QDirIterator::Subdirectories);
    while (itDir.hasNext())
    {
      const auto& diskFileName = itDir.next();
      const AxpArchivePort::FileName fileName = (diskFileName.mid(opennedPathSize + 1)).toLocal8Bit().data();
      LOG_DEBUG(__FUNCTION__ << fileName.data());

      auto& fileListItem = fileList[fileName];
      switch (fileListItem.status)
      {
        case AxpArchivePort::FileListData::FileStatus::UNKNOWN:
        case AxpArchivePort::FileListData::FileStatus::NEW:
          fileListItem.status = AxpArchivePort::FileListData::FileStatus::NEW;
          break;

        default:
          fileListItem.status = AxpArchivePort::FileListData::FileStatus::MODIFIED;
      }
      fileListItem.nameFromDisk = diskFileName.toLocal8Bit().data();
    }

    m_axpArchive->setProgressCallback([this](auto fileName, auto cur, auto total) {
      QString qStringFileName = QString::fromLocal8Bit(fileName.data());
      emit this->invoke([=](){
        this->setProgress(qStringFileName, cur, total);
      });
    });

    if (!m_axpArchive->saveToDiskFile(opennedPathSave.toLocal8Bit().data()))
    {
      LOG(__FUNCTION__ << "error" << m_axpArchive->getLastErrorMessage());
      saveThread->quit();
      return;
    }

    // Prevent ask
    m_axpArchive->close();

    emit this->invoke([=](){
      this->openAxpArchive(opennedPathSave);
    });
    saveThread->quit();
  });
  connect(saveThread, &QThread::finished, [=](){
    saveThread->deleteLater();
  });
  saveThread->start();
}

void MainWindow::on_actionSave_triggered()
{
  auto opennedPaths = m_axpArchive->getArchiveFileName();
  QThread* saveThread = new QThread;
  connect(saveThread, &QThread::started, [=](){
    if (m_axpArchive->isModified())
    {
      if (m_axpArchive->saveToDiskFile(opennedPaths.toLocal8Bit().data()))
      {
        emit this->invoke([=](){
          this->openAxpArchive(opennedPaths);
        });
      }
      else
      {
        LOG(__FUNCTION__ << m_axpArchive->getLastErrorMessage());
      }
    }
    saveThread->deleteLater();
    saveThread->quit();
  });
  saveThread->start();
}

void MainWindow::on_actionClose_openning_file_triggered()
{
  this->closeOpenningAxp();
}

void MainWindow::on_addFileBtn_clicked()
{
  this->on_actionAdd_File_triggered();
}

void MainWindow::on_addFolderBtn_clicked()
{
  this->on_actionAdd_Folder_triggered();
}

void MainWindow::on_extractAllBtn_clicked()
{
  this->on_actionExtract_All_Data_triggered();
}

void MainWindow::on_extractSelectedBtn_clicked()
{
}

void MainWindow::on_saveBtn_clicked()
{
  this->on_actionSave_triggered();
}

void MainWindow::on_saveAsBtn_clicked()
{
  this->on_actionSave_As_triggered();
}
