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
		if (!indexAt(event->pos()).isValid() || itemAt(event->pos())->data(Qt::UserRole) < 0)
		{
			clearSelection();
			setCurrentItem(nullptr); // Needed for currentItemChanged
		}
		QTableWidget::mousePressEvent(event);
	}
};
