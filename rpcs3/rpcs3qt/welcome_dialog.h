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
};

#endif
