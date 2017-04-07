#ifndef GAMELISTFRAME_H
#define GAMELISTFRAME_H

#include <QDockWidget>
#include <QTableWidget>

class GameListFrame : public QDockWidget
{
	Q_OBJECT

public:
	explicit GameListFrame(QWidget *parent = 0);

private:
	QTableWidget *gameList;
};

#endif // GAMELISTFRAME_H
