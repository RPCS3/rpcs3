#include "gui_settings.h"

#include "game_list_frame.h"
#include "qt_utils.h"

#include <QCoreApplication>
#include <QMessageBox>

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

gui_settings::gui_settings(QObject* parent) : QObject(parent), m_settings(ComputeSettingsDir() + tr("CurrentSettings") + ".ini", QSettings::Format::IniFormat, parent),
	m_settingsDir(ComputeSettingsDir())
{
}

gui_settings::~gui_settings()
{
	m_settings.sync();
}

QString gui_settings::GetSettingsDir()
{
	return m_settingsDir.absolutePath();
}

QString gui_settings::ComputeSettingsDir()
{
	return QString::fromStdString(fs::get_config_dir()) + "/GuiConfigs/";
}

void gui_settings::ChangeToConfig(const QString& name)
{
	if (name != tr("CurrentSettings"))
	{ // don't try to change to yourself.
		Reset(false);

		QSettings other(m_settingsDir.absoluteFilePath(name + ".ini"), QSettings::IniFormat);
		for (const QString& key : other.allKeys())
		{
			m_settings.setValue(key, other.value(key));
		}
		m_settings.sync();
	}
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

QVariant gui_settings::GetValue(const gui_save& entry)
{
	return m_settings.value(entry.key + "/" + entry.name, entry.def);
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

QStringList gui_settings::GetGameListCategoryFilters()
{
	QStringList filterList;
	if (GetCategoryVisibility(Category::Non_Disc_Game)) filterList.append(category::non_disc_games);
	if (GetCategoryVisibility(Category::Disc_Game)) filterList.append(category::disc_Game);
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
	case Category::Non_Disc_Game:
		value = gui::cat_hdd_game; break;
	case Category::Disc_Game:
		value = gui::cat_disc_game; break;
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
	case Category::Non_Disc_Game:
		value = gui::cat_hdd_game; break;
	case Category::Disc_Game:
		value = gui::cat_disc_game; break;
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
		LOG_WARNING(GENERAL, "SetCategoryVisibility: wrong cat <%d>", cat);
		break;
	}

	SetValue(value, val);
}

void gui_settings::ShowInfoBox(const gui_save& entry, const QString& title, const QString& text, QWidget* parent)
{
	if (GetValue(entry).toBool())
	{
		QMessageBox* mb = new QMessageBox(QMessageBox::Information, title, text, QMessageBox::Ok, parent);
		mb->setCheckBox(new QCheckBox(tr("Don't show again")));
		mb->deleteLater();
		mb->exec();
		if (mb->checkBox()->isChecked())
		{
			SetValue(entry, false);
			LOG_NOTICE(GENERAL, "Info Box for Entry %s is now disabled", sstr(entry.name));
		}
	}
	else LOG_NOTICE(GENERAL, "Info Box for Entry %s was ignored", sstr(entry.name));
}

void gui_settings::SetGamelistColVisibility(int col, bool val)
{
	// hide sound format and parental level
	bool show = col != 8 && col != 9;
	SetValue(gui_save(gui::game_list, "Col" + QString::number(col) + "visible", show), val);
}

void gui_settings::SetCustomColor(int col, const QColor& val)
{
	SetValue(gui_save(gui::meta, "CustomColor" + QString::number(col), gui::mw_tool_bar_color), val);
}

void gui_settings::SaveCurrentConfig(const QString& friendlyName)
{
	SetValue(gui::m_currentConfig, friendlyName);
	BackupSettingsToTarget(friendlyName);
}

logs::level gui_settings::GetLogLevel()
{
	return (logs::level) GetValue(gui::l_level).toUInt();
}

bool gui_settings::GetGamelistColVisibility(int col)
{
	// hide sound format, parental level, firmware version and path by default
	bool show = col != gui::column_sound && col != gui::column_parental && col != gui::column_firmware && col != gui::column_path;
	return GetValue(gui_save(gui::game_list, "Col" + QString::number(col) + "visible", show)).toBool();
}

QColor gui_settings::GetCustomColor(int col)
{
	return GetValue(gui_save(gui::meta, "CustomColor" + QString::number(col), gui::mw_tool_bar_color)).value<QColor>();
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

void gui_settings::BackupSettingsToTarget(const QString& friendlyName)
{	
	QSettings target(ComputeSettingsDir() + friendlyName + ".ini", QSettings::Format::IniFormat);
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
#if !defined(_WIN32) && !defined(__APPLE__)
	// Makes stylesheets load if using AppImage or installed to /usr/bin
	QDir linuxStylesheetDir = QCoreApplication::applicationDirPath() + "/../share/rpcs3/GuiConfigs/";
	res.append(gui::utils::get_dir_entries(linuxStylesheetDir, nameFilter));
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

	return m_settingsDir.absoluteFilePath(stylesheet + ".qss");
}
