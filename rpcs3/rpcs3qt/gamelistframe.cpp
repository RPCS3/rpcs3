#ifdef QT_UI

#include "gamelistframe.h"

GameListFrame::GameListFrame(QWidget *parent) : QDockWidget(tr("Game List"), parent)
{
	gameList = new QTableWidget(this);
	gameList->setColumnCount(7);
	gameList->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Icon")));
	gameList->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Name")));
	gameList->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Serial")));
	gameList->setHorizontalHeaderItem(3, new QTableWidgetItem(tr("FW")));
	gameList->setHorizontalHeaderItem(4, new QTableWidgetItem(tr("Version")));
	gameList->setHorizontalHeaderItem(5, new QTableWidgetItem(tr("Category")));
	gameList->setHorizontalHeaderItem(6, new QTableWidgetItem(tr("Path")));

	setWidget(gameList);
}

#endif // QT_UI
