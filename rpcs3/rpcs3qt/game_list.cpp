#include "stdafx.h"
#include "game_list.h"
#include "movie_item.h"

#include <QApplication>
#include <QHeaderView>
#include <QMenu>

game_list::game_list() : QTableWidget(), game_list_base()
{
	m_icon_ready_callback = [this](const game_info& game, const movie_item_base* item)
	{
		Q_EMIT IconReady(game, item);
	};
}

void game_list::sync_header_actions(std::map<int, QAction*>& actions, std::function<bool(int)> get_visibility)
{
	ensure(get_visibility);

	bool is_dirty = false;

	for (auto& [col, action] : actions)
	{
		const bool is_hidden = !get_visibility(col);
		action->setChecked(!is_hidden);

		if (isColumnHidden(col) != is_hidden)
		{
			setColumnHidden(col, is_hidden);
			is_dirty = true;
		}
	}

	if (is_dirty)
	{
		fix_narrow_columns();
	}
}

void game_list::create_header_actions(std::map<int, QAction*>& actions, std::function<bool(int)> get_visibility, std::function<void(int, bool)> set_visibility)
{
	ensure(get_visibility);
	ensure(set_visibility);

	horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, this, [this, &actions](const QPoint& pos)
	{
		QMenu* configure = new QMenu(this);
		for (auto& [col, action] : actions)
		{
			configure->addAction(action);
		}
		configure->exec(horizontalHeader()->viewport()->mapToGlobal(pos));
	});

	for (auto& [col, action] : actions)
	{
		action->setCheckable(true);

		connect(action, &QAction::triggered, this, [this, &actions, get_visibility, set_visibility, col](bool checked)
		{
			if (!checked) // be sure to have at least one column left so you can call the context menu at all time
			{
				int c = 0;
				for (auto& [col, action] : actions)
				{
					if (get_visibility(col) && ++c > 1)
						break;
				}
				if (c < 2)
				{
					::at32(actions, col)->setChecked(true); // re-enable the checkbox if we don't change the actual state
					return;
				}
			}

			setColumnHidden(col, !checked); // Negate because it's a set col hidden and we have menu say show.
			set_visibility(col, checked);

			if (checked) // handle hidden columns that have zero width after showing them (stuck between others)
			{
				fix_narrow_columns();
			}
		});
	}

	sync_header_actions(actions, get_visibility);
}

void game_list::clear_list()
{
	m_last_hover_item = nullptr;
	clearSelection();
	clearContents();
}

void game_list::fix_narrow_columns()
{
	QApplication::processEvents();

	// handle columns (other than the icon column) that have zero width after showing them (stuck between others)
	for (int col = 1; col < columnCount(); ++col)
	{
		if (isColumnHidden(col))
		{
			continue;
		}

		if (columnWidth(col) <= horizontalHeader()->minimumSectionSize())
		{
			setColumnWidth(col, horizontalHeader()->minimumSectionSize());
		}
	}
}

void game_list::mousePressEvent(QMouseEvent* event)
{
	// Handle deselction when clicking on empty space in the table
	if (!itemAt(event->pos()))
	{
		clearSelection();
		setCurrentItem(nullptr); // Needed for currentItemChanged
	}
	QTableWidget::mousePressEvent(event);
}

void game_list::mouseMoveEvent(QMouseEvent* event)
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

	QTableWidget::mouseMoveEvent(event);
}

void game_list::mouseDoubleClickEvent(QMouseEvent* ev)
{
	if (!ev) return;

	// Qt's itemDoubleClicked signal doesn't distinguish between mouse buttons and there is no simple way to get the pressed button.
	// So we have to ignore this event when another button is pressed.
	if (ev->button() != Qt::LeftButton)
	{
		ev->ignore();
		return;
	}

	QTableWidget::mouseDoubleClickEvent(ev);
}

void game_list::keyPressEvent(QKeyEvent* event)
{
	const auto modifiers = event->modifiers();

	if (modifiers == Qt::ControlModifier && event->key() == Qt::Key_F && !event->isAutoRepeat())
	{
		Q_EMIT FocusToSearchBar();
		return;
	}

	QTableWidget::keyPressEvent(event);
}

void game_list::leaveEvent(QEvent* event)
{
	if (m_last_hover_item)
	{
		m_last_hover_item->set_active(false);
		m_last_hover_item = nullptr;
	}

	QTableWidget::leaveEvent(event);
}

void game_list::FocusAndSelectFirstEntryIfNoneIs()
{
	if (QTableWidgetItem* item = itemAt(0, 0); item && selectedIndexes().isEmpty())
	{
		setCurrentItem(item);
	}

	setFocus();
}
