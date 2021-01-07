#pragma once

#include <QTableWidget>
#include <QMouseEvent>

/*
	class used in order to get deselection
	if you know a simpler way, tell @Megamouse
*/
class game_list : public QTableWidget
{
private:
	void mousePressEvent(QMouseEvent *event) override
	{
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
		if (!indexAt(event->pos()).isValid() || itemAt(event->pos())->data(Qt::UserRole) < 0)
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
		{
			clearSelection();
			setCurrentItem(nullptr); // Needed for currentItemChanged
		}
		QTableWidget::mousePressEvent(event);
	}
};
