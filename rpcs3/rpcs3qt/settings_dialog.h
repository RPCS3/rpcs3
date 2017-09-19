#pragma once

#include "gui_settings.h"
#include "emu_settings.h"

#include "Emu/GameInfo.h"

#include <QDialog>
#include <QTabWidget>

#include <memory>

namespace Ui
{
	class settings_dialog;
}

class settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit settings_dialog(std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, const int& tabIndex = 0, QWidget *parent = 0, const GameInfo *game = nullptr);
	~settings_dialog();
	int exec();
Q_SIGNALS:
	void GuiSettingsSyncRequest();
	void GuiStylesheetRequest(const QString& path);
	void GuiSettingsSaveRequest();
	void GuiRepaintRequest();
private Q_SLOTS:
	void OnBackupCurrentConfig();
	void OnApplyConfig();
	void OnApplyStylesheet();
private:
	//emulator tab
	void AddConfigs();
	void AddStylesheets();
	QString m_currentStylesheet;
	QString m_currentConfig;
	//gpu tab
	QString m_oldRender = "";

	int m_tab_Index;
	Ui::settings_dialog *ui;
	std::shared_ptr<gui_settings> xgui_settings;
	std::shared_ptr<emu_settings> xemu_settings;
};
