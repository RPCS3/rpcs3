#include "game_list_delegate.h"
#include "movie_item.h"
#include "gui_settings.h"

#include <QHeaderView>

game_list_delegate::game_list_delegate(QObject* parent)
	: table_item_delegate(parent, true)
{}

void game_list_delegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	table_item_delegate::paint(painter, option, index);

	// Find out if the icon or size items are visible
	if (index.column() == static_cast<int>(gui::game_list_columns::dir_size) || (m_has_icons && index.column() == static_cast<int>(gui::game_list_columns::icon)))
	{
		if (const QTableWidget* table = static_cast<const QTableWidget*>(parent()))
		{
			// We need to remove the headers from our calculation. The visualItemRect starts at 0,0 while the visibleRegion doesn't.
			QRegion visible_region = table->visibleRegion();
			visible_region.translate(-table->verticalHeader()->width(), -table->horizontalHeader()->height());

			if (const QTableWidgetItem* current_item = table->item(index.row(), index.column());
				current_item && visible_region.boundingRect().intersects(table->visualItemRect(current_item)))
			{
				if (movie_item* item = static_cast<movie_item*>(table->item(index.row(), static_cast<int>(gui::game_list_columns::icon))))
				{
					if (index.column() == static_cast<int>(gui::game_list_columns::dir_size))
					{
						if (!item->size_on_disk_loading())
						{
							item->call_size_calc_func();
						}
					}
					else if (m_has_icons && index.column() == static_cast<int>(gui::game_list_columns::icon))
					{
						if (!item->icon_loading())
						{
							item->call_icon_load_func(index.row());
						}
					}
				}
			}
		}
	}
}
