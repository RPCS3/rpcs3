#pragma once

#include <QAction>
#include <QList>
#include <QTableWidget>
#include <QMouseEvent>
#include <QKeyEvent>

#include "game_list_base.h"

#include <functional>

class movie_item;

/*
	class used in order to get deselection and hover change
	if you know a simpler way, tell @Megamouse
*/
class game_list : public QTableWidget, public game_list_base
{
	Q_OBJECT

public:
	game_list();

	void sync_header_actions(QList<QAction*>& actions, std::function<bool(int)> get_visibility);
	void create_header_actions(QList<QAction*>& actions, std::function<bool(int)> get_visibility, std::function<void(int, bool)> set_visibility);

	void clear_list() override; // Use this instead of clearContents

	/** Fix columns with width smaller than the minimal section size */
	void fix_narrow_columns();

public Q_SLOTS:
	void FocusAndSelectFirstEntryIfNoneIs();

Q_SIGNALS:
	void FocusToSearchBar();
	void IconReady(const movie_item_base* item);

protected:
	movie_item* m_last_hover_item = nullptr;

	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void keyPressEvent(QKeyEvent* event) override;
	void leaveEvent(QEvent* event) override;
};
