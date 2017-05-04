#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "GuiSettings.h"

#include <QDialog>
#include <QTabWidget>

#include <memory>

class SettingsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SettingsDialog(std::shared_ptr<GuiSettings> xSettings, QWidget *parent = 0, const std::string &path = "");

signals:
	void GuiSettingsSyncRequest();
	void GuiStylesheetRequest(const QString& path);
	void GuiSettingsSaveRequest();
private:
	QTabWidget *tabWidget;
};

#endif // SETTINGSDIALOG_H
