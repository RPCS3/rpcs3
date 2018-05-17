#pragma once

#include "game_list.h"
#include "game_list_grid_delegate.h"

#include <QHBoxLayout>
#include <QScrollArea>

class game_list_grid : public game_list
{
	Q_OBJECT

	QSize m_icon_size;
	QColor m_icon_color;
	qreal m_margin_factor;
	qreal m_text_factor;
	bool m_text_enabled = true;

public:
	explicit game_list_grid(const QSize& icon_size, const QColor& icon_color, const qreal& margin_factor, const qreal& text_factor, const bool& showText);
	~game_list_grid();

	void enableText(const bool& enabled);
	void setIconSize(const QSize& size);
	void addItem(const QPixmap& img, const QString& name, const int& row, const int& col);

	qreal getMarginFactor();

private:
	game_list_grid_delegate* grid_item_delegate; 
};
