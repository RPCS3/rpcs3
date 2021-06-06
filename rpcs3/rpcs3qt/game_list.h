#pragma once

#include <QTableWidget>
#include <QMouseEvent>
#include <QPixmap>

#include "game_compatibility.h"
#include "Emu/GameInfo.h"

class movie_item;

/* Having the icons associated with the game info simplifies logic internally */
struct gui_game_info
{
	GameInfo info;
	QString localized_category;
	compat::status compat;
	QPixmap icon;
	QPixmap pxmap;
	bool hasCustomConfig;
	bool hasCustomPadConfig;
	bool has_hover_gif;
	movie_item* item;
};

typedef std::shared_ptr<gui_game_info> game_info;
Q_DECLARE_METATYPE(game_info)

/*
	class used in order to get deselection and hover change
	if you know a simpler way, tell @Megamouse
*/
class game_list : public QTableWidget
{
public:
	void clear_list(); // Use this instead of clearContents

protected:
	movie_item* m_last_hover_item = nullptr;

	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
};
