#pragma once

#include <QTreeWidget>
#include <QMouseEvent>

// Allows to double click the items in order to de/select them
class custom_tree_widget : public QTreeWidget
{
	using QTreeWidget::QTreeWidget;

public:
	void set_checkable_by_double_click_callback(std::function<bool(QTreeWidgetItem*, int)> cb)
	{
		m_checkable_by_dc_cb = std::move(cb);
	}

	void mouseDoubleClickEvent(QMouseEvent* event)
	{
		QTreeWidget::mouseDoubleClickEvent(event);

		if (!event || !m_checkable_by_dc_cb) return;

		const QPoint pos = event->pos();
		QTreeWidgetItem* item = itemAt(pos);

		if (!item || !(item->flags() & Qt::ItemIsUserCheckable)) return;

		const int column = columnAt(pos.x());
		if (!m_checkable_by_dc_cb(item, column)) return;

		const QModelIndex index = indexFromItem(item, column);
		if (!index.isValid()) return;

		QStyleOptionViewItem option;
		option.initFrom(this);
		option.rect = visualRect(index);
		option.features = QStyleOptionViewItem::HasCheckIndicator;

		const QRect checkbox_region = style()->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &option, this);
		if (checkbox_region.contains(pos)) return;

		const Qt::CheckState current_state = item->checkState(column);
		item->setCheckState(column, (current_state == Qt::CheckState::Checked) ? Qt::CheckState::Unchecked : Qt::CheckState::Checked);
	}

private:
	std::function<bool(QTreeWidgetItem*, int)> m_checkable_by_dc_cb;
};
