#include "AxpFileListView.hpp"

#include <QMenu>
#include <QDropEvent>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QFileDialog>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <qt_windows.h>
#include <QMimeData>
#include <QDrag>
#include <QWindow>

#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "AxpArchivePort.hpp"
#include "AxpItem.hpp"
#include "Log.hpp"
#include "Utils.hpp"
#include "CustomLabel.hpp"
#include "StyledItemDelegate.hpp"
#include "Global.hpp"

AxpFileListView::AxpFileListView(QWidget *parent) : QTableView(parent),
  mHoverRow(-1), mHoverColumn(-1)
{
  this->setMouseTracking(true);
  this->setSelectionBehavior(SelectRows);
  this->setItemDelegate(new StyledItemDelegate());

  this->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &QTableView::customContextMenuRequested,
        this, &AxpFileListView::showContextMenu);
  connect(this, &QTableView::doubleClicked,
          this, &AxpFileListView::openSelected);
}

void AxpFileListView::showContextMenu(const QPoint &pos)
{
  auto selectedCount = this->selectedIndexes().size();
  if (!selectedCount) {
    return;
  }

  QMenu contextMenu(tr("Context menu"), this);

  auto columnCount = this->model()->columnCount();
  QAction actionPreview("Preview", this);
  if (selectedCount == columnCount) {
    connect(&actionPreview, &QAction::triggered, this, &AxpFileListView::openSelected);
    contextMenu.addAction(&actionPreview);
    contextMenu.addSeparator();
  }

  QAction actionExtractSelected("Extract selected item(s)", this);
  connect(&actionExtractSelected, &QAction::triggered, this, &AxpFileListView::extractSelected);
  contextMenu.addAction(&actionExtractSelected);
  contextMenu.addSeparator();

  QAction actionDelete("Delete", this);
  connect(&actionDelete, &QAction::triggered, this, &AxpFileListView::deleteSelected);
  contextMenu.addAction(&actionDelete);
  contextMenu.addSeparator();

  QAction actionRevert("Revert", this);
  connect(&actionRevert, &QAction::triggered, this, &AxpFileListView::revertSelected);
  contextMenu.addAction(&actionRevert);

  contextMenu.exec(mapToGlobal(pos));
}

void AxpFileListView::extractSelected() const
{
  auto mainWindow = MainWindow::getInstance();
  QString opennedPath = QFileDialog::getExistingDirectory(
        mainWindow
        );
  if (opennedPath.isEmpty()) {
    return;
  }

  auto items = this->selectedIndexes();
  int extractedCount = 0;
  auto axpArc = mainWindow->getAxpArchive();

  QModelIndex dirRootIndex = mainWindow->getUi()->dirList->model()->index(0, 0);
  QString dirKey = mainWindow->getSelectedDirAxpKey();
  LOG_DEBUG(__FUNCTION__ << "DirKey:" << dirKey);

  QDir targetDir(opennedPath + '/' + dirRootIndex.data().toString() + "-Output");

  auto extractFileFn = [axpArc, &targetDir](const std::string& axpItemName)->bool
  {
    QString targetFileName = targetDir.filePath(QString::fromLocal8Bit(axpItemName.c_str()));

    if (!axpArc->extractToDisk(axpItemName, targetFileName.toLocal8Bit().data())) {
      LOG(targetFileName << ": Write error");
      return false;
    }

    return true;
  };

  int itemsSize = items.size();
  for (int i = 0; i < itemsSize; ++i) {
    const auto& item = items[i];
    if (item.column() != 0) {
      continue;
    }
    QString axpKey = item.data(AxpItem::ItemKeyRole).toString();
    auto columnCount = item.model()->columnCount();
    emit mainWindow->invoke([=](){
      mainWindow->setProgress(axpKey, static_cast<std::size_t>(i / columnCount + 1),
                              static_cast<std::size_t>(itemsSize / columnCount));
    });

    std::string itemKey = std::string(axpKey.toLocal8Bit());
    if (!axpArc->exists(itemKey)) {
      for (const auto& fileInfo : axpArc->getFileList()) {
        const auto& _fKey = fileInfo.first;
        if (_fKey.find(itemKey) == 0) {
          if (extractFileFn(_fKey)) {
            ++extractedCount;
          }
        }
      }
    } else {
      if (extractFileFn(itemKey)) {
        ++extractedCount;
      }
    }
  }

  QMessageBox::information(
        mainWindow, "Extracted",
        QString::number(extractedCount) + " item extracted successfully!"
        );

  if (targetDir.exists()) {
    QDesktopServices::openUrl(targetDir.path());
  }
}

void AxpFileListView::openSelected()
{
  auto itemIndex = this->currentIndex();
  itemIndex = itemIndex.model()->index(itemIndex.row(), 0);
  auto itemIndexKeyData = itemIndex.data(AxpItem::ItemKeyRole);
  auto mainWnd = MainWindow::getInstance();

  // Check is file
  if (!itemIndex.data(AxpItem::ItemTypeRole).toBool()) {
    auto dirList = mainWnd->getUi()->dirList;
    if (!dirList->isEnabled()) {
      return;
    }
    auto dirModel = dirList->model();
    auto curDirIndex = dirList->currentIndex();
    auto curItem = reinterpret_cast<const QStandardItemModel*>(dirModel)->itemFromIndex(curDirIndex);
    auto rowCount = curItem->rowCount();
    for (int r = 0; r < rowCount; ++r) {
      auto item = curItem->child(r, 0);
      if (item && (item->data(AxpItem::ItemKeyRole) == itemIndexKeyData)) {
        auto idx = dirModel->index(item->row(), item->column(), curDirIndex);
        dirList->setCurrentIndex(idx);
        break;
      }
    }
    return;
  }

  auto itemKey = itemIndexKeyData.toString();
  auto axpArc = mainWnd->getAxpArchive();
  auto fKey = std::string(itemKey.toLocal8Bit());
  if (!axpArc->exists(fKey)) {
    LOG(itemKey << ": Item does not exist");
    return;
  }

  auto data = axpArc->read(fKey);
  LOG_DEBUG(__FUNCTION__ << "data.size:" << data.size());
  QPixmap pxmap;
  if (pxmap.loadFromData(data)) {
    const auto& pxSize = pxmap.size();
    const auto& desktop = qApp->desktop();
    const auto& desktopSize = desktop->size();
    if ((pxSize.height() > desktopSize.height()) || (pxSize.width() > desktopSize.width())) {
      pxmap = pxmap.scaled(desktopSize/1.2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    QSize minDlgSize(500, 300);
    QLabel* imgDlg = new CustomLabel(this);
    imgDlg->resize(
          std::max(pxmap.width(), minDlgSize.width()),
          std::max(pxSize.height(), minDlgSize.height())
          );
    imgDlg->setWindowFlags(Qt::Window);
    imgDlg->setWindowTitle(QString("%1 (%2 x %3)").arg(QFileInfo(itemKey).fileName()).arg(pxSize.width()).arg(pxSize.height()));
    imgDlg->setAlignment(Qt::AlignCenter);
    imgDlg->setGeometry(
          QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            imgDlg->size(),
            desktop->availableGeometry()
            )
          );

    imgDlg->setPixmap(pxmap);
    imgDlg->show();
  } else {
    LOG_DEBUG(__FUNCTION__ << "trying to open another");
    QTemporaryDir& dir = Global::getTempDir();

    QString fileName(dir.path() + '/' + itemKey);
    if (axpArc->extractToDisk(fKey, fileName.toLocal8Bit().data())) {
      //      QMimeDatabase db;
      //      QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchContent);
      //      LOG_DEBUG(__FUNCTION__ << mime);
      if (reinterpret_cast<int>(ShellExecute(GetDesktopWindow(), "open", fileName.toLocal8Bit().data(), "", "", 1)) < 32)
      {
        LOG_DEBUG(__FUNCTION__ << fileName << ": failed to open native app");
      }
    }
  }
}

void AxpFileListView::deleteSelected()
{
  LOG_DEBUG(__FUNCTION__ << "called");
  const auto& mainWnd = MainWindow::getInstance();
  const auto& axpArc = mainWnd->getAxpArchive();

  auto selectedItems = this->selectedIndexes();

  auto& fileList = axpArc->getFileList();
  for (const auto& item : selectedItems) {
    LOG_DEBUG(__FUNCTION__ << item << item.column() << item.row());
    if (item.column() != 0)
    {
      continue;
    }
    QString q_fileName = item.data(AxpItem::ItemKeyRole).toString();
    AxpArchivePort::FileName fileName = q_fileName.toLocal8Bit().data();
    bool isDir = q_fileName.at(q_fileName.size() - 1) == '/';
    if (!isDir)
    {
      auto& fileListData = fileList[fileName];
      switch (fileListData.status)
      {
        case AxpArchivePort::FileListData::FileStatus::NEW:
          fileList.erase(fileName);
          break;

        case AxpArchivePort::FileListData::FileStatus::UNKNOWN:
          break;

        default:
          fileListData.status = AxpArchivePort::FileListData::FileStatus::DELETED;
      }
    }
    else
    {
      for (auto& fileListItem : fileList)
      {
        const auto& itemKeyName = fileListItem.first;
        if (itemKeyName.find(fileName) != 0)
        {
          continue;
        }

        auto& fileListData = fileList[itemKeyName];

        switch (fileListData.status)
        {
          case AxpArchivePort::FileListData::FileStatus::NEW:
            fileList.erase(fileName);
            break;

          case AxpArchivePort::FileListData::FileStatus::UNKNOWN:
            break;

          default:
            fileListData.status = AxpArchivePort::FileListData::FileStatus::DELETED;
        }
      }
    }
  }

  mainWnd->setCurrentDir(mainWnd->getUi()->dirList->currentIndex());
}

void AxpFileListView::revertSelected()
{
  LOG_DEBUG(__FUNCTION__ << "called");
  const auto& mainWnd = MainWindow::getInstance();
  const auto& axpArc = mainWnd->getAxpArchive();

  auto selectedItems = this->selectedIndexes();

  auto& fileList = axpArc->getFileList();
  for (const auto& item : selectedItems) {
    LOG_DEBUG(__FUNCTION__ << item << item.column() << item.row());
    if (item.column() != 0)
    {
      continue;
    }
    QString q_fileName = item.data(AxpItem::ItemKeyRole).toString();
    AxpArchivePort::FileName fileName = q_fileName.toLocal8Bit().data();
    bool isDir = q_fileName.at(q_fileName.size() - 1) == '/';
    if (!isDir)
    {
      auto& fileListData = fileList[fileName];
      switch (fileListData.status)
      {
        case AxpArchivePort::FileListData::FileStatus::NEW:
          fileList.erase(fileName);
          break;

        case AxpArchivePort::FileListData::FileStatus::UNKNOWN:
          break;

        default:
          fileListData.status = AxpArchivePort::FileListData::FileStatus::ORIGIN;
      }
    }
    else
    {
      for (auto& fileListItem : fileList)
      {
        const auto& itemKeyName = fileListItem.first;
        if (itemKeyName.find(fileName) != 0)
        {
          continue;
        }

        auto& fileListData = fileList[itemKeyName];

        switch (fileListData.status)
        {
          case AxpArchivePort::FileListData::FileStatus::NEW:
            fileList.erase(fileName);
            break;

          case AxpArchivePort::FileListData::FileStatus::UNKNOWN:
            break;

          default:
            fileListData.status = AxpArchivePort::FileListData::FileStatus::ORIGIN;
        }
      }
    }
  }

  mainWnd->setCurrentDir(mainWnd->getUi()->dirList->currentIndex());
}

void AxpFileListView::dropEvent(QDropEvent* event)
{
//  LOG_DEBUG(__FUNCTION__ << event);
//  QTableView::dropEvent(event);
}

void AxpFileListView::dragEnterEvent(QDragEnterEvent* event)
{
//  LOG_DEBUG(__FUNCTION__ << "called");
//  QTableView::dragEnterEvent(event);
}

void AxpFileListView::mouseMoveEvent(QMouseEvent *event)
{
//  LOG_DEBUG(__FUNCTION__ << "called");
  QTableView::mouseMoveEvent(event);

  QModelIndex index = indexAt(event->pos());
  int oldHoverRow = mHoverRow;
  int oldHoverColumn = mHoverColumn;
  mHoverRow = index.row();
  mHoverColumn = index.column();

  if (selectionBehavior() == SelectRows && oldHoverRow != mHoverRow) {
    for (int i = 0; i < model()->columnCount(); ++i)
      update(model()->index(mHoverRow, i));
  }
  if (selectionBehavior() == SelectColumns && oldHoverColumn != mHoverColumn) {
    for (int i = 0; i < model()->rowCount(); ++i) {
      update(model()->index(i, mHoverColumn));
      update(model()->index(i, oldHoverColumn));
    }
  }
}

void AxpFileListView::mousePressEvent(QMouseEvent* event)
{
  QTableView::mousePressEvent(event);
}
