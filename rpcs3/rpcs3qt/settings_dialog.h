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

struct Render_Info
{
	QString name;
	QString old_adapter;
	QStringList adapters;
	emu_settings::SettingsType type;
	bool supported = true;
	bool has_adapters = true;

	Render_Info(){};
	Render_Info(const QString& name) : name(name), has_adapters(false){};
	Render_Info(const QString& name, const QStringList& adapters, bool supported, const emu_settings::SettingsType& type)
		: name(name), adapters(adapters), supported(supported), type(type) {};
};

class settings_dialog : public QDialog
{
	Q_OBJECT

public:
	explicit settings_dialog(std::shared_ptr<gui_settings> xSettings, const Render_Creator& r_Creator, const int& tabIndex = 0, QWidget *parent = 0, const GameInfo *game = nullptr);
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

	Render_Info m_D3D12;
	Render_Info m_Vulkan;
	Render_Info m_OpenGL;
	Render_Info m_NullRender;

	int m_tab_Index;
	Ui::settings_dialog *ui;
	std::shared_ptr<gui_settings> xgui_settings;
};
