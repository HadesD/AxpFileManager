#include "AxpItem.hpp"

#include <QApplication>
#include <QStyle>

#include "Utils.hpp"

AxpItem::AxpItem(const QString &key)
{
  this->setData(key, ItemKeyRole);

  bool m_isDir = (key.at(key.size()-1) == '/');

  this->setData(m_isDir ? 0 : 1, ItemTypeRole);

  const auto& paths = key.split('/');
//  QStandardItem(paths.at(paths.size()- (m_isDir ? 2 : 1)));

  this->setText(paths.at(paths.size()- (m_isDir ? 2 : 1)));

  if (m_isDir) {
    this->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
  } else {
    const auto& itemName = paths.at(paths.size() - 1);
    auto dotArr = itemName.split('.');
    this->setIcon(Utils::iconFromExtensionSmall(dotArr.at(dotArr.size()-1)));
  }
}
