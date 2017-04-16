#ifndef GAMELISTFRAME_H
#define GAMELISTFRAME_H

#include <QDockWidget>
#include <QTableWidget>

class GameListFrame : public QDockWidget
{
	Q_OBJECT

public:
	explicit GameListFrame(QWidget *parent = 0);

signals:
	void GameListFrameClosed();
protected:
	/** Override inherited method from Qt to allow signalling when close happened.*/
	void closeEvent(QCloseEvent* event);
private:
	QTableWidget *gameList;
};

#endif // GAMELISTFRAME_H
