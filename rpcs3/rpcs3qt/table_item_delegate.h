#pragma once

#include <QStyledItemDelegate>
#include <QPainter>

/** This class is used to get rid of somewhat ugly item focus rectangles. You could change the rectangle instead of omiting it if you wanted */
class table_item_delegate : public QStyledItemDelegate
{
private:
	bool m_has_icons;

public:
	explicit table_item_delegate(QObject *parent = nullptr, bool has_icons = false) : QStyledItemDelegate(parent), m_has_icons(has_icons) {}

	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
	{
		// Remove the focus frame around selected items
		option->state &= ~QStyle::State_HasFocus;

		if (m_has_icons && index.column() == 0)
		{
			// Don't highlight icons
			option->state &= ~QStyle::State_Selected;

			// Center icons
			option->decorationAlignment = Qt::AlignCenter;
			option->decorationPosition = QStyleOptionViewItem::Top;
		}

		QStyledItemDelegate::initStyleOption(option, index);
	}

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
	{
		if (index.column() == 0 && option.state & QStyle::State_Selected)
		{
			// Add background highlight color to icons
			painter->fillRect(option.rect, option.palette.color(QPalette::Highlight));
		}

		QStyledItemDelegate::paint(painter, option, index);
	}
};
