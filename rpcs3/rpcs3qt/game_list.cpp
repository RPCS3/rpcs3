#include "game_list.h"
#include "movie_item.h"

void game_list::clear_list()
{
	m_last_hover_item = nullptr;
	clearSelection();
	clearContents();
}

void game_list::mousePressEvent(QMouseEvent *event)
{
	if (!indexAt(event->pos()).isValid() || !itemAt(event->pos())->data(Qt::UserRole).isValid())
	{
		clearSelection();
		setCurrentItem(nullptr); // Needed for currentItemChanged
	}
	QTableWidget::mousePressEvent(event);
}

void game_list::mouseMoveEvent(QMouseEvent *event)
{
	movie_item* new_item = static_cast<movie_item*>(itemAt(event->pos()));

	if (new_item != m_last_hover_item)
	{
		if (m_last_hover_item)
		{
			m_last_hover_item->set_active(false);
		}
		if (new_item)
		{
			new_item->set_active(true);
		}
	}

	m_last_hover_item = new_item;
}

void game_list::leaveEvent(QEvent *event)
{
	if (m_last_hover_item)
	{
		m_last_hover_item->set_active(false);
		m_last_hover_item = nullptr;
	}
}
