#pragma once

#include "flow_widget_item.h"
#include "movie_item_base.h"
#include "game_list_base.h"

#include <QLabel>

class game_list_grid_item : public flow_widget_item, public movie_item_base
{
	Q_OBJECT

public:
	game_list_grid_item(QWidget* parent, game_info game, const QString& title);

	void set_icon_size(const QSize& size);
	void set_icon(const QPixmap& pixmap);
	void adjust_size();

	const game_info& game() const
	{
		return m_game;
	}

	void show_title(bool visible);

	void polish_style() override;

	bool event(QEvent* event) override;

private:
	QSize m_icon_size{};
	QLabel* m_icon_label{};
	QLabel* m_title_label{};
	game_info m_game{};
};
