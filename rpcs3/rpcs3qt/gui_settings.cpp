#include "gui_settings.h"

#include "qt_utils.h"
#include "localized.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QMessageBox>

LOG_CHANNEL(cfg_log, "CFG");

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

gui_settings::gui_settings(QObject* parent) : settings(parent)
{
	m_current_name = gui::Settings;
	m_settings.reset(new QSettings(ComputeSettingsDir() + gui::Settings + ".ini", QSettings::Format::IniFormat, parent));

	const QString settings_name = GetValue(gui::m_currentConfig).toString();

	if (settings_name != m_current_name)
	{
		ChangeToConfig(settings_name);
	}
}

QString gui_settings::GetCurrentUser()
{
	// load user
	bool is_valid_user;
	const QString user = GetValue(gui::um_active_user).toString();
	const u32 user_id = user.toInt(&is_valid_user);

	// set user if valid
	if (is_valid_user && user_id > 0)
	{
		return user;
	}

	cfg_log.fatal("Could not parse user setting: '%s' = '%d'.", user.toStdString(), user_id);
	return QString();
}

bool gui_settings::ChangeToConfig(const QString& config_name)
{
	if (m_current_name == config_name)
	{
		return false;
	}

	// Backup current config
	SaveCurrentConfig(m_current_name);

	// Save new config name to the default config
	SaveConfigNameToDefault(config_name);

	// Sync file just in case
	m_settings->sync();

	// Load new config
	m_settings.reset(new QSettings(m_settings_dir.absoluteFilePath(config_name + ".ini"), QSettings::IniFormat));

	// Save own name to new config
	SetValue(gui::m_currentConfig, config_name);
	m_settings->sync();

	m_current_name = config_name;

	return true;
}

void gui_settings::Reset(bool remove_meta)
{
	if (remove_meta)
	{
		m_settings->clear();
	}
	else
	{
		m_settings->remove(gui::logger);
		m_settings->remove(gui::main_window);
		m_settings->remove(gui::game_list);
	}
}

QStringList gui_settings::GetGameListCategoryFilters()
{
	QStringList filterList;

	if (GetCategoryVisibility(Category::HDD_Game)) filterList.append(category::cat_hdd_game);
	if (GetCategoryVisibility(Category::Disc_Game)) filterList.append(category::cat_disc_game);
	if (GetCategoryVisibility(Category::PS1_Game)) filterList.append(category::cat_ps1_game);
	if (GetCategoryVisibility(Category::PS2_Game)) filterList.append(category::ps2_games);
	if (GetCategoryVisibility(Category::PSP_Game)) filterList.append(category::psp_games);
	if (GetCategoryVisibility(Category::Home)) filterList.append(category::cat_home);
	if (GetCategoryVisibility(Category::Media)) filterList.append(category::media);
	if (GetCategoryVisibility(Category::Data)) filterList.append(category::data);
	if (GetCategoryVisibility(Category::Unknown_Cat)) filterList.append(category::cat_unknown);
	if (GetCategoryVisibility(Category::Others)) filterList.append(category::others);

	return filterList;
}

bool gui_settings::GetCategoryVisibility(int cat)
{
	gui_save value;

	switch (cat)
	{
	case Category::HDD_Game:
		value = gui::cat_hdd_game; break;
	case Category::Disc_Game:
		value = gui::cat_disc_game; break;
	case Category::PS1_Game:
		value = gui::cat_ps1_game; break;
	case Category::PS2_Game:
		value = gui::cat_ps2_game; break;
	case Category::PSP_Game:
		value = gui::cat_psp_game; break;
	case Category::Home:
		value = gui::cat_home; break;
	case Category::Media:
		value = gui::cat_audio_video; break;
	case Category::Data:
		value = gui::cat_game_data; break;
	case Category::Unknown_Cat:
		value = gui::cat_unknown; break;
	case Category::Others:
		value = gui::cat_other; break;
	default:
		cfg_log.warning("GetCategoryVisibility: wrong cat <%d>", cat);
		break;
	}

	return GetValue(value).toBool();
}

void gui_settings::SetCategoryVisibility(int cat, const bool& val)
{
	gui_save value;

	switch (cat)
	{
	case Category::HDD_Game:
		value = gui::cat_hdd_game; break;
	case Category::Disc_Game:
		value = gui::cat_disc_game; break;
	case Category::Home:
		value = gui::cat_home; break;
	case Category::PS1_Game:
		value = gui::cat_ps1_game; break;
	case Category::PS2_Game:
		value = gui::cat_ps2_game; break;
	case Category::PSP_Game:
		value = gui::cat_psp_game; break;
	case Category::Media:
		value = gui::cat_audio_video; break;
	case Category::Data:
		value = gui::cat_game_data; break;
	case Category::Unknown_Cat:
		value = gui::cat_unknown; break;
	case Category::Others:
		value = gui::cat_other; break;
	default:
		cfg_log.warning("SetCategoryVisibility: wrong cat <%d>", cat);
		break;
	}

	SetValue(value, val);
}

void gui_settings::ShowBox(bool confirm, const QString& title, const QString& text, const gui_save& entry, int* result = nullptr, QWidget* parent = nullptr, bool always_on_top = false)
{
	const std::string dialog_type = confirm ? "Confirmation" : "Info";

	if (entry.name.isEmpty() || GetValue(entry).toBool())
	{
		const QFlags<QMessageBox::StandardButton> buttons = confirm ? QMessageBox::Yes | QMessageBox::No : QMessageBox::Ok;
		const QMessageBox::Icon icon = confirm ? QMessageBox::Question : QMessageBox::Information;

		QMessageBox* mb = new QMessageBox(icon, title, text, buttons, parent, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | (always_on_top ? Qt::WindowStaysOnTopHint : Qt::Widget));
		mb->deleteLater();

		if (!entry.name.isEmpty())
		{
			mb->setCheckBox(new QCheckBox(tr("Don't show again")));
		}

		connect(mb, &QMessageBox::finished, [&](int res)
		{
			if (result)
			{
				*result = res;
			}
			if (!entry.name.isEmpty() && mb->checkBox()->isChecked())
			{
				SetValue(entry, false);
				cfg_log.notice("%s Dialog for Entry %s is now disabled", dialog_type, sstr(entry.name));
			}
		});

		mb->exec();
	}
	else cfg_log.notice("%s Dialog for Entry %s was ignored", dialog_type, sstr(entry.name));
}

void gui_settings::ShowConfirmationBox(const QString& title, const QString& text, const gui_save& entry, int* result = nullptr, QWidget* parent = nullptr)
{
	ShowBox(true, title, text, entry, result, parent, true);
}

void gui_settings::ShowInfoBox(const QString& title, const QString& text, const gui_save& entry, QWidget* parent = nullptr)
{
	ShowBox(false, title, text, entry, nullptr, parent, false);
}

void gui_settings::SetGamelistColVisibility(int col, bool val)
{
	SetValue(GetGuiSaveForColumn(col), val);
}

void gui_settings::SetCustomColor(int col, const QColor& val)
{
	SetValue(gui_save(gui::meta, "CustomColor" + QString::number(col), gui::gl_icon_color), val);
}

void gui_settings::SaveCurrentConfig(const QString& config_name)
{
	SaveConfigNameToDefault(config_name);
	BackupSettingsToTarget(config_name);
	ChangeToConfig(config_name);
}

logs::level gui_settings::GetLogLevel()
{
	return logs::level{GetValue(gui::l_level).toUInt()};
}

bool gui_settings::GetGamelistColVisibility(int col)
{
	return GetValue(GetGuiSaveForColumn(col)).toBool();
}

QColor gui_settings::GetCustomColor(int col)
{
	return GetValue(gui_save(gui::meta, "CustomColor" + QString::number(col), gui::gl_icon_color)).value<QColor>();
}

QStringList gui_settings::GetConfigEntries()
{
	const QStringList name_filter = QStringList("*.ini");
	const QFileInfoList entries = m_settings_dir.entryInfoList(name_filter, QDir::Files);

	QStringList res;

	for (const QFileInfo &entry : entries)
	{
		res.append(entry.baseName());
	}

	return res;
}

// Save the name of the used config to the default settings file
void gui_settings::SaveConfigNameToDefault(const QString& config_name)
{
	if (m_current_name == gui::Settings)
	{
		SetValue(gui::m_currentConfig, config_name);
		m_settings->sync();
	}
	else
	{
		QSettings tmp(m_settings_dir.absoluteFilePath(gui::Settings + ".ini"), QSettings::Format::IniFormat, parent());
		tmp.beginGroup(gui::m_currentConfig.key);
		tmp.setValue(gui::m_currentConfig.name, config_name);
		tmp.endGroup();
	}
}

void gui_settings::BackupSettingsToTarget(const QString& config_name)
{
	QSettings target(ComputeSettingsDir() + config_name + ".ini", QSettings::Format::IniFormat);

	for (const QString& key : m_settings->allKeys())
	{
		if (!key.startsWith(gui::meta))
		{
			target.setValue(key, m_settings->value(key));
		}
	}

	target.sync();
}

QStringList gui_settings::GetStylesheetEntries()
{
	const QStringList name_filter = QStringList("*.qss");
	QStringList res = gui::utils::get_dir_entries(m_settings_dir, name_filter);
#if !defined(_WIN32)
	// Makes stylesheets load if using AppImage (App Bundle) or installed to /usr/bin
#ifdef __APPLE__
	QDir platformStylesheetDir = QCoreApplication::applicationDirPath() + "/../Resources/GuiConfigs/";
#else
	QDir platformStylesheetDir = QCoreApplication::applicationDirPath() + "/../share/rpcs3/GuiConfigs/";
#endif
	res.append(gui::utils::get_dir_entries(platformStylesheetDir, name_filter));
	res.removeDuplicates();
#endif
	res.sort(Qt::CaseInsensitive);
	return res;
}

QString gui_settings::GetCurrentStylesheetPath()
{
	const Localized localized;

	const QString stylesheet = GetValue(gui::m_currentStylesheet).toString();

	if (stylesheet == gui::Default)
	{
		return "";
	}
	else if (stylesheet == gui::None)
	{
		return "-";
	}

	return m_settings_dir.absoluteFilePath(stylesheet + ".qss");
}

QSize gui_settings::SizeFromSlider(int pos)
{
	return gui::gl_icon_size_min + (gui::gl_icon_size_max - gui::gl_icon_size_min) * (1.f * pos / gui::gl_max_slider_pos);
}

gui_save gui_settings::GetGuiSaveForColumn(int col)
{
	// hide sound format, parental level, firmware version and path by default
	const bool show = col != gui::column_sound && col != gui::column_parental && col != gui::column_firmware && col != gui::column_path;
	return gui_save{ gui::game_list, "visibility_" + gui::get_game_list_column_name(static_cast<gui::game_list_columns>(col)), show };
}
