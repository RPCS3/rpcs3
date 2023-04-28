#pragma once

#include <QTableWidget>
#include <QMouseEvent>
#include <QKeyEvent>

#include "game_list_base.h"
#include "util/atomic.hpp"

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

	void clear_list() override; // Use this instead of clearContents

public Q_SLOTS:
	void FocusAndSelectFirstEntryIfNoneIs();

Q_SIGNALS:
	void FocusToSearchBar();
	void IconReady(const game_info& game);

protected:
	movie_item* m_last_hover_item = nullptr;

	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void leaveEvent(QEvent *event) override;
};
