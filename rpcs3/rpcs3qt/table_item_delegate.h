#pragma once

#include <QStyledItemDelegate>
#include <QPainter>

/** This class is used to get rid of somewhat ugly item focus rectangles. You could change the rectangle instead of omiting it if you wanted */
class table_item_delegate : public QStyledItemDelegate
{
public:
	explicit table_item_delegate(QObject *parent = 0) : QStyledItemDelegate(parent) {}

	virtual void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
	{
		// Remove the focus frame around selected items
		option->state &= ~QStyle::State_HasFocus;

		if (index.column() == 0)
		{
			// Don't highlight icons
			option->state &= ~QStyle::State_Selected;
		}

		QStyledItemDelegate::initStyleOption(option, index);
	}

	virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override
	{
		if (index.column() == 0 && option.state & QStyle::State_Selected)
		{
			// Add background highlight color to icons
			painter->fillRect(option.rect, option.palette.color(QPalette::Highlight));
		}

		QStyledItemDelegate::paint(painter, option, index);
	}
};
