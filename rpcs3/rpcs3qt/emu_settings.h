#pragma once

#include "util/types.hpp"
#include "util/yaml.hpp"

#include "microphone_creator.h"
#include "midi_creator.h"
#include "render_creator.h"
#include "emu_settings_type.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QStringList>
#include <QComboBox>
#include <QSpinBox>
#include <QDateTimeEdit>

#include <string>
#include <vector>

namespace cfg
{
	class _base;
}

class emu_settings : public QObject
{
	/** A settings class for Emulator specific settings.  This class is a refactored version of the wx version.  It is much nicer
	*
	*/
	Q_OBJECT
public:
	std::set<emu_settings_type> m_broken_types; // list of broken settings

	/** Creates a settings object which reads in the config.yml file at rpcs3/bin/%path%/config.yml
	* Settings are only written when SaveSettings is called.
	*/
	emu_settings();

	bool Init();

	/** Connects a combo box with the target settings type*/
	void EnhanceComboBox(QComboBox* combobox, emu_settings_type type, bool is_ranged = false, bool use_max = false, int max = 0, bool sorted = false, bool strict = true);

	/** Connects a check box with the target settings type*/
	void EnhanceCheckBox(QCheckBox* checkbox, emu_settings_type type);

	/** Connects a date time edit box with the target settings type*/
	void EnhanceDateTimeEdit(QDateTimeEdit* date_time_edit, emu_settings_type type, const QString& format, bool use_calendar, bool as_offset_from_now, int offset_update_time=0);

	/** Connects a slider with the target settings type*/
	void EnhanceSlider(QSlider* slider, emu_settings_type type);

	/** Connects an integer spin box with the target settings type*/
	void EnhanceSpinBox(QSpinBox* spinbox, emu_settings_type type, const QString& prefix = "", const QString& suffix = "");

	/** Connects a double spin box with the target settings type*/
	void EnhanceDoubleSpinBox(QDoubleSpinBox* spinbox, emu_settings_type type, const QString& prefix = "", const QString& suffix = "");

	/** Connects a line edit with the target settings type*/
	void EnhanceLineEdit(QLineEdit* edit, emu_settings_type type);

	/** Connects a button group with the target settings type*/
	void EnhanceRadioButton(QButtonGroup* button_group, emu_settings_type type);

	std::vector<std::string> GetLibrariesControl();
	void SaveSelectedLibraries(const std::vector<std::string>& libs);

	/** Returns the valid options for a given setting.*/
	static std::vector<std::string> GetSettingOptions(emu_settings_type type);

	/** Returns the valid options for a given setting.*/
	static QStringList GetQStringSettingOptions(emu_settings_type type);

	/** Returns the default value of the setting type.*/
	std::string GetSettingDefault(emu_settings_type type) const;

	/** Returns the value of the setting type.*/
	std::string GetSetting(emu_settings_type type) const;

	/** Sets the setting type to a given value.*/
	void SetSetting(emu_settings_type type, const std::string& val) const;

	/** Try to find the settings type for a given string.*/
	emu_settings_type FindSettingsType(const cfg::_base* node) const;

	/** Gets all the renderer info for gpu settings.*/
	render_creator* m_render_creator = nullptr;

	/** Gets a list of all the microphones available.*/
	microphone_creator m_microphone_creator;

	/** Gets a list of all the midi devices available.*/
	midi_creator m_midi_creator;

	/** Loads the settings from path.*/
	void LoadSettings(const std::string& title_id = "", bool create_config_from_global = true);

	/** Fixes all registered invalid settings after asking the user for permission.*/
	void OpenCorrectionDialog(QWidget* parent = Q_NULLPTR);

	/** Get a localized and therefore freely adjustable version of the string used in config.yml.*/
	QString GetLocalizedSetting(const QString& original, emu_settings_type type, int index, bool strict) const;

	/** Get a localized and therefore freely adjustable version of the string used in config.yml.*/
	std::string GetLocalizedSetting(const std::string& original, emu_settings_type type, int index, bool strict) const;

	/** Get a localized and therefore freely adjustable version of the string used in config.yml.*/
	std::string GetLocalizedSetting(const cfg::_base* node, u32 index) const;

	/** Validates the settings and logs unused entries or cleans up the yaml*/
	bool ValidateSettings(bool cleanup);

	/** Resets the current settings to the global default. This includes all connected widgets. */
	void RestoreDefaults();

Q_SIGNALS:
	void RestoreDefaultsSignal();

public Q_SLOTS:
	/** Writes the unsaved settings to file.  Used in settings dialog on accept.*/
	void SaveSettings() const;

private:
	YAML::Node m_default_settings; // The default settings as a YAML node.
	YAML::Node m_current_settings; // The current settings as a YAML node.
	std::string m_title_id;
};
