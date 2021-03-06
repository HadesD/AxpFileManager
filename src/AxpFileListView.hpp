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
    void revertSelected();

  protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

  private:
    int mHoverRow, mHoverColumn;
};

#endif // AXPFILELIST_HPP
