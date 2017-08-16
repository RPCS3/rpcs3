#pragma once

#include <QPainter>
#include <QAbstractItemDelegate>

class game_list_grid_delegate : public QAbstractItemDelegate
{
public:
	game_list_grid_delegate(const QSize& imageSize, const qreal& margin_factor, const qreal& margin_ratio, const QFont& font, const QColor& font_color, QObject *parent = 0);

	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
	void setItemSize(const QSize& size) { m_size = size; };
	virtual ~game_list_grid_delegate();
private:
	QSize m_size;
	qreal m_margin_factor;
	qreal m_text_factor;
	QFont m_font;
	QColor m_font_color;
};
