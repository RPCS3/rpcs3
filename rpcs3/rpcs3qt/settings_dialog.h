#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "gui_settings.h"
#include "emu_settings.h"

#include "Emu/GameInfo.h"

#include <QDialog>
#include <QTabWidget>

#include <memory>

class settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit settings_dialog(std::shared_ptr<gui_settings> xSettings, Render_Creator r_Creator, QWidget *parent = 0, GameInfo *game = nullptr);
signals:
	void GuiSettingsSyncRequest();
	void GuiStylesheetRequest(const QString& path);
	void GuiSettingsSaveRequest();
private:
	QTabWidget *tabWidget;
};

#endif // SETTINGSDIALOG_H
