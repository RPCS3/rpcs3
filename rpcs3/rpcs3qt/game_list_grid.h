#ifndef GAME_LIST_GRID_H
#define GAME_LIST_GRID_H

#include "game_list_grid_delegate.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QString>
#include <QTableWidget>

class game_list_grid : public QTableWidget
{
	Q_OBJECT

	QSize m_icon_size;
	qreal m_margin_factor;
	qreal m_text_factor;
	bool m_text_enabled = true;

public:
	explicit game_list_grid(const QSize& icon_size, const qreal& margin_factor, const qreal& text_factor, const bool& showText);
	~game_list_grid();

	void enableText(const bool& enabled);
	void setIconSize(const QSize& size);
	void addItem(const QPixmap& img, const QString& name, const int& idx, const int& row, const int& col);

	qreal getMarginFactor();

private:
	game_list_grid_delegate* grid_item_delegate; 
};

#endif