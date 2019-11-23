#include "AxpDirListView.hpp"

#include "MainWindow.hpp"
#include "Log.hpp"

AxpDirListView::AxpDirListView(QWidget *parent) : QTreeView (parent)
{
}

void AxpDirListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
  LOG_DEBUG(__FUNCTION__ << "called");
  QTreeView::currentChanged(current, previous);
  if (!current.isValid()) {
    return;
  }

  LOG_DEBUG(__FUNCTION__ << current << previous);

  const auto& mWnd = MainWindow::getInstance();
  mWnd->setCurrentDir(current);
  LOG_DEBUG(__FUNCTION__ << "completed!");
}
