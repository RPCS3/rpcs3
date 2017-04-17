#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QTabWidget>

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(QWidget *parent = 0, const std::string &path = "");

private:
	QTabWidget *tabWidget;
};

#endif // SETTINGSDIALOG_H
