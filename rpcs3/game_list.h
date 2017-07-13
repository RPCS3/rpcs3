#pragma once

#include <QTableWidget>
#include <QMouseEvent>

/*
	class used in order to get deselection
	if you know a simpler way, tell @Megamouse
*/
class game_list : public QTableWidget {
private:
	void mousePressEvent(QMouseEvent *event)
	{
		if (!indexAt(event->pos()).isValid())
		{
			clearSelection();
		}
		QTableWidget::mousePressEvent(event);
	}
};
