#ifndef AXPDIRLISTVIEW_HPP
#define AXPDIRLISTVIEW_HPP

#include <QTreeView>

class AxpDirListView : public QTreeView
{
  public:
    explicit AxpDirListView(QWidget* parent = nullptr);

  protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

};

#endif // AXPDIRLISTVIEW_HPP
