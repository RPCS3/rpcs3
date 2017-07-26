#pragma once

#include "gui_settings.h"
#include "emu_settings.h"

#include "Emu/GameInfo.h"

#include <QDialog>
#include <QTabWidget>

#include <memory>

namespace Ui {
	class settings_dialog;
}

class settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit settings_dialog(std::shared_ptr<gui_settings> xSettings, const Render_Creator& r_Creator, const int& tabIndex = 0, QWidget *parent = 0, const GameInfo *game = nullptr);
	int exec();
Q_SIGNALS:
	void GuiSettingsSyncRequest();
	void GuiStylesheetRequest(const QString& path);
	void GuiSettingsSaveRequest();
	void ToolBarRepaintRequest();
private Q_SLOTS:
	void OnBackupCurrentConfig();
	void OnApplyConfig();
	void OnApplyStylesheet();
private:
	//emulator tab
	void AddConfigs();
	void AddStylesheets();
	QString m_startingStylesheet;
	QString m_startingConfig;
	//gpu tab
	QString m_oldRender = "";
	bool m_isD3D12 = false;
	bool m_isVulkan = false;

	int m_tab_Index;
	Ui::settings_dialog *ui;
	std::shared_ptr<gui_settings> xgui_settings;
};
