#pragma once

#include "stdafx.h"
#include "Utilities/Config.h"

#include "gui_settings.h"
#include "emu_settings.h"

#include <QListWidget>
#include <QLabel>

struct vfs_settings_info
{
	QString name; // name of tab
	emu_settings::SettingsType settingLoc; // Where the setting is saved in emu_settings
	gui_save listLocation; // Where the list of dir options are saved
	cfg::string* cfg_node; // Needed since emu_settings overrides settings file and doesn't touch g_cfg currently.
};

class vfs_dialog_tab : public QWidget
{
	Q_OBJECT

public:
	explicit vfs_dialog_tab(vfs_settings_info info, std::shared_ptr<gui_settings> guiSettings, std::shared_ptr<emu_settings> emuSettings, QWidget* parent = nullptr);

	void SetSettings();

	// Reset this tab without saving the settings yet
	void Reset();

private:
	void AddNewDirectory();
	void RemoveDirectory();
	void SetCurrentRow(int row) { m_currentRow = row; }

	const QString EmptyPath = tr("Empty Path");

	vfs_settings_info m_info;
	std::shared_ptr<gui_settings> m_gui_settings;
	std::shared_ptr<emu_settings> m_emu_settings;
	int m_currentRow = -1;

	// UI variables needed in higher scope
	QListWidget* m_dirList;
	QLabel* m_selectedConfigLabel;
};
