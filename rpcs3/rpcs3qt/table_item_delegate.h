#pragma once

#include <QItemDelegate>

/** This class is used to get rid of somewhat ugly item focus rectangles. You could change the rectangle instead of omiting it if you wanted */
class table_item_delegate : public QItemDelegate
{
public:
	explicit table_item_delegate(QObject *parent = 0) {}
	virtual void drawFocus(QPainter * /*painter*/, const QStyleOptionViewItem & /*option*/, const QRect & /*rect*/) const {}
};
