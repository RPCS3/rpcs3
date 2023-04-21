#include "table_item_delegate.h"

#include <QTableWidget>
#include "movie_item.h"
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
	if (index.column() == gui::game_list_columns::column_icon && option.state & QStyle::State_Selected)
	{
		// Add background highlight color to icons
		painter->fillRect(option.rect, option.palette.color(QPalette::Highlight));
	}

	QStyledItemDelegate::paint(painter, option, index);

	// Find out if the icon or size items are visible
	if (index.column() == gui::game_list_columns::column_dir_size || (m_has_icons && index.column() == gui::game_list_columns::column_icon))
	{
		if (const QTableWidget* table = static_cast<const QTableWidget*>(parent()))
		{
			if (const QTableWidgetItem* current_item = table->item(index.row(), index.column());
				current_item && table->visibleRegion().intersects(table->visualItemRect(current_item)))
			{
				if (movie_item* item = static_cast<movie_item*>(table->item(index.row(), gui::game_list_columns::column_icon)))
				{
					if (index.column() == gui::game_list_columns::column_dir_size)
					{
						if (!item->size_on_disk_loading())
						{
							item->call_size_calc_func();
						}
					}
					else if (m_has_icons && index.column() == gui::game_list_columns::column_icon)
					{
						if (!item->icon_loading())
						{
							item->call_icon_load_func();
						}
					}
				}
			}
		}
	}
}
