#ifndef AXPITEM_HPP
#define AXPITEM_HPP

#include <QStandardItem>

class AxpItem : public QStandardItem
{
  public:
    static constexpr auto ItemKeyRole = Qt::UserRole + 1;
    static constexpr auto ItemTypeRole = Qt::UserRole + 2;

  public:
    explicit AxpItem(const QString& key);
};

#endif // AXPARCHIVEITEM_HPP
