#pragma once

#include <QStyledItemDelegate>
#include <QPainter>

/** This class is used to get rid of somewhat ugly item focus rectangles. You could change the rectangle instead of omiting it if you wanted */
class table_item_delegate : public QStyledItemDelegate
{
public:
	explicit table_item_delegate(QObject *parent = nullptr, bool has_icons = false);

	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

protected:
	bool m_has_icons{};
};
