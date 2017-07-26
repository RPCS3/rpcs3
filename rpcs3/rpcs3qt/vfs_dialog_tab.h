#pragma once

#include "stdafx.h"
#include "Emu/System.h"

#include "gui_settings.h"
#include "emu_settings.h"

#include <QWidget>
#include <QListWidget>
#include <QLabel>

struct vfs_settings_info
{
	QString name; // name of tab
	emu_settings::SettingsType settingLoc; // Where the setting is saved in emu_settings
	GUI_SAVE listLocation; // Where the list of dir options are saved
	cfg::string* cfg_node; // Needed since emu_settings overrides settings file and doesn't touch g_cfg currently.
};

class vfs_dialog_tab : public QWidget
{
	Q_OBJECT

public:
	explicit vfs_dialog_tab(const vfs_settings_info& info, gui_settings* guiSettings, emu_settings* emuSettings, QWidget* parent = nullptr);

	void SaveSettings();
	void AddNewDirectory();
	void Reset();
private:
	QString EmuConfigDir();

	const QString EmptyPath = "Empty Path";

	vfs_settings_info m_info;
	gui_settings* m_gui_settings;
	emu_settings* m_emu_settings;

	// UI variables needed in higher scope
	QListWidget* dirList;
	QLabel*		 selectedConfigLabel;
};
