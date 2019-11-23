#ifndef AXPFILELISTVIEW_HPP
#define AXPFILELISTVIEW_HPP

#include <QTableView>

class QDropEvent;

/**
 * Hover used:
 * https://github.com/lowbees/Hover-entire-row-of-QTableView
 */
class AxpFileListView : public QTableView
{
    Q_OBJECT

  public:
    explicit AxpFileListView(QWidget *parent = nullptr);
    QModelIndex hoverIndex() const { return model()->index(mHoverRow, mHoverColumn); }

  protected:
    void showContextMenu(const QPoint& pos);
    void extractSelected() const;
    void openSelected();
    void deleteSelected();
    void dropEvent(QDropEvent* event) override;

  protected:
    virtual void mouseMoveEvent(QMouseEvent *event) override;

  private:
    int mHoverRow, mHoverColumn;
};

#endif // AXPFILELIST_HPP
