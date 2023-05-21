#include "table_item_delegate.h"
#include "gui_settings.h"

table_item_delegate::table_item_delegate(QObject* parent, bool has_icons)
	: QStyledItemDelegate(parent), m_has_icons(has_icons)
{
}

void table_item_delegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	// Remove the focus frame around selected items
	option->state &= ~QStyle::State_HasFocus;

	if (m_has_icons && index.column() == 0)
	{
		// Don't highlight icons
		option->state &= ~QStyle::State_Selected;

		// Center icons
		option->decorationAlignment = Qt::AlignCenter;
		option->decorationPosition = QStyleOptionViewItem::Top;
	}

	QStyledItemDelegate::initStyleOption(option, index);
}

void table_item_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if (m_has_icons && index.column() == static_cast<int>(gui::game_list_columns::icon) && option.state & QStyle::State_Selected)
	{
		// Add background highlight color to icons
		painter->fillRect(option.rect, option.palette.color(QPalette::Highlight));
	}

	QStyledItemDelegate::paint(painter, option, index);
}
