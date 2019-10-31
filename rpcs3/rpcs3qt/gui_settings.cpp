#include "gui_settings.h"

#include "game_list_frame.h"
#include "qt_utils.h"

#include <QCoreApplication>
#include <QMessageBox>

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

gui_settings::gui_settings(QObject* parent) : QObject(parent)
	, m_current_name(gui::Settings)
	, m_settings(ComputeSettingsDir() + gui::Settings + ".ini", QSettings::Format::IniFormat, parent)
	, m_settingsDir(ComputeSettingsDir())
{
	const QString settings_name = GetValue(gui::m_currentConfig).toString();

	if (settings_name != m_current_name)
	{
		ChangeToConfig(settings_name);
	}
}

gui_settings::~gui_settings()
{
	m_settings.sync();
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

	LOG_FATAL(GENERAL, "Could not parse user setting: '%s' = '%d'.", user.toStdString(), user_id);
	return QString();
}

QString gui_settings::GetSettingsDir()
{
	return m_settingsDir.absolutePath();
}

QString gui_settings::ComputeSettingsDir()
{
	return QString::fromStdString(fs::get_config_dir()) + "/GuiConfigs/";
}

bool gui_settings::ChangeToConfig(const QString& friendly_name)
{
	if (m_current_name == friendly_name)
	{
		return false;
	}

	if (friendly_name != gui::Settings)
	{
		if (m_current_name == gui::Settings)
		{
			SetValue(gui::m_currentConfig, friendly_name);
		}
		else
		{
			QSettings tmp(m_settingsDir.absoluteFilePath(gui::Settings + ".ini"), QSettings::Format::IniFormat, parent());
			tmp.beginGroup(gui::m_currentConfig.key);
			tmp.setValue(gui::m_currentConfig.name, friendly_name);
			tmp.endGroup();
		}
	}

	m_settings.sync();

	Reset(true);

	QSettings other(m_settingsDir.absoluteFilePath(friendly_name + ".ini"), QSettings::IniFormat);

	for (const QString& key : other.allKeys())
	{
		m_settings.setValue(key, other.value(key));
	}

	SetValue(gui::m_currentConfig, friendly_name);

	m_settings.sync();

	m_current_name = friendly_name;

	return true;
}

void gui_settings::Reset(bool removeMeta)
{
	if (removeMeta)
	{
		m_settings.clear();
	}
	else
	{
		m_settings.remove(gui::logger);
		m_settings.remove(gui::main_window);
		m_settings.remove(gui::game_list);
	}
}

void gui_settings::RemoveValue(const QString& key, const QString& name)
{
	m_settings.beginGroup(key);
	m_settings.remove(name);
	m_settings.endGroup();
}

QVariant gui_settings::GetValue(const gui_save& entry)
{
	return m_settings.value(entry.key + "/" + entry.name, entry.def);
}

QVariant gui_settings::GetValue(const QString& key, const QString& name, const QString& def)
{
	return m_settings.value(key + "/" + name, def);
}

QVariant gui_settings::List2Var(const q_pair_list& list)
{
	QByteArray ba;
	QDataStream stream(&ba, QIODevice::WriteOnly);
	stream << list;
	return QVariant(ba);
}

q_pair_list gui_settings::Var2List(const QVariant& var)
{
	q_pair_list list;
	QByteArray ba = var.toByteArray();
	QDataStream stream(&ba, QIODevice::ReadOnly);
	stream >> list;
	return list;
}

void gui_settings::SetValue(const gui_save& entry, const QVariant& value)
{
	m_settings.beginGroup(entry.key);
	m_settings.setValue(entry.name, value);
	m_settings.endGroup();
}

void gui_settings::SetValue(const QString& key, const QString& name, const QVariant& value)
{
	m_settings.beginGroup(key);
	m_settings.setValue(name, value);
	m_settings.endGroup();
}

void gui_settings::SetPlaytime(const QString& serial, const qint64& elapsed)
{
	m_playtime[serial] = elapsed;
	SetValue(gui::playtime, serial, elapsed);
}

qint64 gui_settings::GetPlaytime(const QString& serial)
{
	return m_playtime[serial];
}

void gui_settings::SetLastPlayed(const QString& serial, const QString& date)
{
	m_last_played[serial] = date;
	SetValue(gui::last_played, serial, date);
}

QString gui_settings::GetLastPlayed(const QString& serial)
{
	return m_last_played[serial];
}

QStringList gui_settings::GetGameListCategoryFilters()
{
	QStringList filterList;
	if (GetCategoryVisibility(Category::HDD_Game)) filterList.append(category::hdd_game);
	if (GetCategoryVisibility(Category::Disc_Game)) filterList.append(category::disc_game);
	if (GetCategoryVisibility(Category::PS1_Game)) filterList.append(category::ps1_game);
	if (GetCategoryVisibility(Category::PS2_Game)) filterList.append(category::ps2_games);
	if (GetCategoryVisibility(Category::PSP_Game)) filterList.append(category::psp_games);
	if (GetCategoryVisibility(Category::Home)) filterList.append(category::home);
	if (GetCategoryVisibility(Category::Media)) filterList.append(category::media);
	if (GetCategoryVisibility(Category::Data)) filterList.append(category::data);
	if (GetCategoryVisibility(Category::Unknown_Cat)) filterList.append(category::unknown);
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
		LOG_WARNING(GENERAL, "GetCategoryVisibility: wrong cat <%d>", cat);
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
		LOG_WARNING(GENERAL, "SetCategoryVisibility: wrong cat <%d>", cat);
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
				LOG_NOTICE(GENERAL, "%s Dialog for Entry %s is now disabled", dialog_type, sstr(entry.name));
			}
		});

		mb->exec();
	}
	else LOG_NOTICE(GENERAL, "%s Dialog for Entry %s was ignored", dialog_type, sstr(entry.name));
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

void gui_settings::SaveCurrentConfig(const QString& friendly_name)
{
	if (friendly_name != gui::Settings)
	{
		if (m_current_name == gui::Settings)
		{
			SetValue(gui::m_currentConfig, friendly_name);
			m_settings.sync();
		}
		else
		{
			QSettings tmp(m_settingsDir.absoluteFilePath(gui::Settings + ".ini"), QSettings::Format::IniFormat, parent());
			tmp.beginGroup(gui::m_currentConfig.key);
			tmp.setValue(gui::m_currentConfig.name, friendly_name);
			tmp.endGroup();
		}
	}

	BackupSettingsToTarget(friendly_name);
	ChangeToConfig(friendly_name);
}

logs::level gui_settings::GetLogLevel()
{
	return (logs::level) GetValue(gui::l_level).toUInt();
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
	QStringList nameFilter;
	nameFilter << "*.ini";
	QFileInfoList entries = m_settingsDir.entryInfoList(nameFilter, QDir::Files);
	QStringList res;
	for (const QFileInfo &entry : entries)
	{
		res.append(entry.baseName());
	}

	return res;
}

void gui_settings::BackupSettingsToTarget(const QString& friendly_name)
{	
	QSettings target(ComputeSettingsDir() + friendly_name + ".ini", QSettings::Format::IniFormat);

	for (const QString& key : m_settings.allKeys())
	{
		if (!key.startsWith(gui::meta))
		{
			target.setValue(key, m_settings.value(key));
		}
	}

	target.sync();
}

QStringList gui_settings::GetStylesheetEntries()
{
	QStringList nameFilter = QStringList("*.qss");
	QStringList res = gui::utils::get_dir_entries(m_settingsDir, nameFilter);
#if !defined(_WIN32)
	// Makes stylesheets load if using AppImage (App Bundle) or installed to /usr/bin
#ifdef __APPLE__
	QDir platformStylesheetDir = QCoreApplication::applicationDirPath() + "/../Resources/GuiConfigs/";
#else
	QDir platformStylesheetDir = QCoreApplication::applicationDirPath() + "/../share/rpcs3/GuiConfigs/";
#endif
	res.append(gui::utils::get_dir_entries(platformStylesheetDir, nameFilter));
	res.removeDuplicates();
#endif
	res.sort(Qt::CaseInsensitive);
	return res;
}

QString gui_settings::GetCurrentStylesheetPath()
{
	QString stylesheet = GetValue(gui::m_currentStylesheet).toString();

	if (stylesheet == gui::Default)
	{
		return "";
	}
	else if (stylesheet == gui::None)
	{
		return "-";
	}

	return m_settingsDir.absoluteFilePath(stylesheet + ".qss");
}

QSize gui_settings::SizeFromSlider(int pos)
{
	return gui::gl_icon_size_min + (gui::gl_icon_size_max - gui::gl_icon_size_min) * (pos / (float)gui::gl_max_slider_pos);
}

gui_save gui_settings::GetGuiSaveForColumn(int col)
{
	// hide sound format, parental level, firmware version and path by default
	bool show = col != gui::column_sound && col != gui::column_parental && col != gui::column_firmware && col != gui::column_path;
	return gui_save{ gui::game_list, "visibility_" + gui::get_game_list_column_name((gui::game_list_columns)col), show };
}
