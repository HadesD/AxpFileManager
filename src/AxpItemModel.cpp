#include "AxpItemModel.hpp"

#include "AxpFile.hpp"

AxpItemModel::AxpItemModel(AxpFile* axpFile) :
  QAbstractItemModel(),
  m_pAxpFile(axpFile)
{
}

QModelIndex AxpItemModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

//    TreeItem *parentItem;

//    if (!parent.isValid())
//        parentItem = rootItem;
//    else
//        parentItem = static_cast<TreeItem*>(parent.internalPointer());

//    TreeItem *childItem = parentItem->child(row);
//    if (childItem)
//        return createIndex(row, column, childItem);
//    else
//        return QModelIndex();
    return QModelIndex();
}

QModelIndex AxpItemModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

//    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
//    TreeItem *parentItem = childItem->parentItem();

//    if (parentItem == rootItem)
//        return QModelIndex();

//    return createIndex(parentItem->row(), 0, parentItem);
    return QModelIndex();
}

int AxpItemModel::rowCount(const QModelIndex &parent) const
{
   return static_cast<int>(m_pAxpFile->m_FileList.size());
}

int AxpItemModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant AxpItemModel::data(const QModelIndex &index, int role) const
{
  if (role != Qt::DisplayRole)
  {
    return QVariant();
  }

  auto& fileList = m_pAxpFile->m_FileList;
  if (fileList.size() < static_cast<std::size_t>(index.row()))
  {
    return QVariant();
  }

  return QString("Row%1, Column%2 : %3")
      .arg(index.row() + 1)
      .arg(index.column() +1)
      .arg(m_pAxpFile->m_FileList.at(index.row()))
      ;
}

Qt::ItemFlags AxpItemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QVariant AxpItemModel::headerData(
    int section, Qt::Orientation orientation,
    int role
    ) const
{
//    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
//        return rootItem->data(section);

    return QVariant();
}
