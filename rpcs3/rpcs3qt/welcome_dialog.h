#ifndef WELCOME_DIALOG_H
#define WELCOME_DIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QCloseEvent>

class welcome_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit welcome_dialog(QWidget* parent = nullptr);
private:
	void keyPressEvent(QKeyEvent* event)
	{
		// this won't work with Alt+F4, the window still closes
		if (event->key() == Qt::Key_Escape)
		{
			event->ignore();
		}
	}
	void closeEvent(QCloseEvent* event)
	{
		// spontaneous: don't close on external system level events like Alt+F4
		if (event->spontaneous())
		{
			event->ignore();
		}
	}
};

#endif
