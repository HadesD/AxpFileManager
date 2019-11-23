#include "StyledItemDelegate.hpp"

#include "AxpFileListView.hpp"
StyledItemDelegate::StyledItemDelegate()
    : QStyledItemDelegate ()
{

}


void StyledItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    opt.state &= ~QStyle::State_HasFocus;

    AxpFileListView *view = qobject_cast<AxpFileListView*>(opt.styleObject);
    QTableView::SelectionBehavior behavior = view->selectionBehavior();
    QModelIndex hoverIndex = view->hoverIndex();

    if (!(option.state & QStyle::State_Selected) && behavior != QTableView::SelectItems) {
        if (behavior == QTableView::SelectRows && hoverIndex.row() == index.row())
            opt.state |= QStyle::State_MouseOver;
        if (behavior == QTableView::SelectColumns && hoverIndex.column() == index.column())
            opt.state |= QStyle::State_MouseOver;
    }

    QStyledItemDelegate::paint(painter, opt, index);
}
