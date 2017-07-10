#pragma once

#include <QDialog>
#include <QKeyEvent>
#include <QCloseEvent>

namespace Ui {
	class welcome_dialog;
}

class welcome_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit welcome_dialog(QWidget* parent = nullptr);

private:
	Ui::welcome_dialog *ui;
};
