#pragma once

#include <QPainter>
#include <QStyledItemDelegate>

class game_list_grid_delegate : public QStyledItemDelegate
{
public:
	game_list_grid_delegate(const QSize& imageSize, const qreal& margin_factor, const qreal& margin_ratio, QObject *parent = 0);

	virtual void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const override;
	void setItemSize(const QSize& size);
	virtual ~game_list_grid_delegate();
private:
	QSize m_size;
	qreal m_margin_factor;
	qreal m_text_factor;
};
