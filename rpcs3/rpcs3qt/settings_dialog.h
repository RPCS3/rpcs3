#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "gui_settings.h"

#include <QDialog>
#include <QTabWidget>

#include <memory>

class settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit settings_dialog(std::shared_ptr<gui_settings> xSettings, QWidget *parent = 0, const std::string &path = "");
signals:
	void GuiSettingsSyncRequest();
	void GuiStylesheetRequest(const QString& path);
	void GuiSettingsSaveRequest();
private:
	QTabWidget *tabWidget;
};

#endif // SETTINGSDIALOG_H
