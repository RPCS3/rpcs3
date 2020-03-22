#pragma once

#include <QDialog>

class QListWidget;

class screenshot_manager_dialog : public QDialog
{
	Q_OBJECT

public:
	screenshot_manager_dialog(QWidget* parent = nullptr);

private:
	QListWidget* m_grid = nullptr;
};
