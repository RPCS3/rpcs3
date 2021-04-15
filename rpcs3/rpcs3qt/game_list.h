#pragma once

#include <QTableWidget>
#include <QMouseEvent>
#include <QPixmap>

#include "game_compatibility.h"
#include "Emu/GameInfo.h"

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
};

typedef std::shared_ptr<gui_game_info> game_info;
Q_DECLARE_METATYPE(game_info)

/*
	class used in order to get deselection
	if you know a simpler way, tell @Megamouse
*/
class game_list : public QTableWidget
{
public:
	int m_last_entered_row = -1;
	int m_last_entered_col = -1;

private:
	void mousePressEvent(QMouseEvent *event) override
	{
		if (!indexAt(event->pos()).isValid() || !itemAt(event->pos())->data(Qt::UserRole).isValid())
		{
			clearSelection();
			setCurrentItem(nullptr); // Needed for currentItemChanged
		}
		QTableWidget::mousePressEvent(event);
	}
};
