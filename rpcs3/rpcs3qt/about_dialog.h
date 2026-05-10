#pragma once

#include <QDialog>

namespace Ui
{
	class about_dialog;
}

class about_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit about_dialog(QWidget* parent = nullptr);
	~about_dialog();

private:
	std::unique_ptr<Ui::about_dialog> ui;
};
