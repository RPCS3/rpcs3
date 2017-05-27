#include "gui_settings.h"

#include "game_list_frame.h"

#include <QCoreApplication>
#include <QDir>


gui_settings::gui_settings(QObject* parent) : QObject(parent), settings(ComputeSettingsDir() + tr("CurrentSettings") + ".ini", QSettings::Format::IniFormat, parent),
	settingsDir(ComputeSettingsDir())
{
}

gui_settings::~gui_settings()
{
	settings.sync();
}

QString gui_settings::ComputeSettingsDir()
{
	QString path = QDir(QDir::currentPath()).relativeFilePath(QCoreApplication::applicationDirPath());
	path += "/GuiConfigs/";
	return path;
}

void gui_settings::ChangeToConfig(const QString& name)
{
	if (name != tr("CurrentSettings"))
	{ // don't try to change to yourself.
		Reset(false);

		QSettings other(settingsDir.absoluteFilePath(name + ".ini"), QSettings::IniFormat);

		QStringList keys = other.allKeys();
		for (QStringList::iterator i = keys.begin(); i != keys.end(); i++)
		{
			settings.setValue(*i, other.value(*i));
		}
		settings.sync();
	}
}

void gui_settings::Reset(bool removeMeta)
{
	if (removeMeta)
	{
		settings.clear();
	}
	else
	{
		settings.remove("Logger");
		settings.remove("main_window");
		settings.remove("GameList");
	}
}

QStringList gui_settings::GetGameListCategoryFilters()
{
	QStringList filterList;
	if (GetCategoryVisibility(category::hdd_Game)) filterList.append(category::hdd_Game);
	if (GetCategoryVisibility(category::disc_Game)) filterList.append(category::disc_Game);
	if (GetCategoryVisibility(category::home)) filterList.append(category::home);
	if (GetCategoryVisibility(category::audio_Video)) filterList.append(category::audio_Video);
	if (GetCategoryVisibility(category::game_Data)) filterList.append(category::game_Data);
	if (GetCategoryVisibility(category::unknown)) filterList.append(category::unknown);
	return filterList;
}

bool gui_settings::GetCategoryVisibility(QString cat)
{
	QString value = "";

	if (cat == category::hdd_Game) value = "GameList/categoryHDDGameVisible";
	else if (cat == category::disc_Game) value = "GameList/categoryDiscGameVisible";
	else if (cat == category::home) value = "GameList/categoryHomeVisible";
	else if (cat == category::audio_Video) value = "GameList/categoryAudioVideoVisible";
	else if (cat == category::game_Data) value = "GameList/categoryGameDataVisible";
	else if (cat == category::unknown) value = "GameList/categoryUnknownVisible";

	return settings.value(value, true).toBool();
}

void gui_settings::SetCategoryVisibility(QString cat, bool val)
{
	QString value = "";

	if (cat == category::hdd_Game) value = "categoryHDDGameVisible";
	else if (cat == category::disc_Game) value = "categoryDiscGameVisible";
	else if (cat == category::home) value = "categoryHomeVisible";
	else if (cat == category::audio_Video) value = "categoryAudioVideoVisible";
	else if (cat == category::game_Data) value = "categoryGameDataVisible";
	else if (cat == category::unknown) value = "categoryUnknownVisible";

	settings.beginGroup("GameList");
	settings.setValue(value, val);
	settings.endGroup();
}

void gui_settings::WriteGuiGeometry(const QByteArray& res)
{
	settings.beginGroup("main_window");
	settings.setValue("geometry", res);
	settings.endGroup();
}

void gui_settings::WriteGuiState(const QByteArray& res)
{
	settings.beginGroup("main_window");
	settings.setValue("windowState", res);
	settings.endGroup();
}

void gui_settings::WriteGameListState(const QByteArray& res)
{
	settings.beginGroup("GameList");
	settings.setValue("state", res);
	settings.endGroup();
}

void gui_settings::SetGamelistVisibility(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("gamelistVisible", val);
	settings.endGroup();
}

void gui_settings::SetLoggerVisibility(bool val)
{
	settings.beginGroup("Logger");
	settings.setValue("loggerVisible", val);
	settings.endGroup();
}

void gui_settings::SetDebuggerVisibility(bool val)
{
	settings.beginGroup("main_window");
	settings.setValue("debuggerVisible", val);
	settings.endGroup();
}

void gui_settings::SetControlsVisibility(bool val)
{
	settings.beginGroup("main_window");
	settings.setValue("controlsVisible", val);
	settings.endGroup();
}

void gui_settings::SetLogLevel(uint lev)
{
	settings.beginGroup("Logger");
	settings.setValue("level", lev);
	settings.endGroup();
}

void gui_settings::SetTTYLogging(bool val)
{
	settings.beginGroup("Logger");
	settings.setValue("TTY", val);
	settings.endGroup();
}

void gui_settings::SetGamelistColVisibility(int col, bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("Col" + QString::number(col) + "visible", val);
	settings.endGroup();
}

void gui_settings::SetGamelistSortCol(int col)
{
	settings.beginGroup("GameList");
	settings.setValue("sortCol", col);
	settings.endGroup();
}

void gui_settings::SetGamelistSortAsc(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("sortAsc", val);
	settings.endGroup();
}

void gui_settings::SaveCurrentConfig(const QString& friendlyName)
{
	SetCurrentConfig(friendlyName);
	BackupSettingsToTarget(friendlyName);
}

void gui_settings::SetCurrentConfig(const QString& friendlyName)
{
	settings.beginGroup("Meta");
	settings.setValue("currentConfig", friendlyName);
	settings.endGroup();
}

void gui_settings::SetStyleSheet(const QString& friendlyName)
{
	settings.beginGroup("Meta");
	settings.setValue("currentStylesheet", friendlyName);
	settings.endGroup();
}

QByteArray gui_settings::ReadGuiGeometry()
{
	return settings.value("main_window/geometry", QByteArray()).toByteArray();
}

QByteArray gui_settings::ReadGuiState()
{
	return settings.value("main_window/windowState", QByteArray()).toByteArray();
}

bool gui_settings::GetGamelistVisibility()
{
	return settings.value("GameList/gamelistVisible", true).toBool();
}

bool gui_settings::GetLoggerVisibility()
{
	return settings.value("Logger/loggerVisible", true).toBool();
}

bool gui_settings::GetDebuggerVisibility()
{
	return settings.value("main_window/debuggerVisible", false).toBool();
}

bool gui_settings::GetControlsVisibility()
{
	return settings.value("main_window/controlsVisible", true).toBool();
}

logs::level gui_settings::GetLogLevel()
{
	return (logs::level) (settings.value("Logger/level", QVariant((uint)(logs::level::warning))).toUInt());
}

bool gui_settings::GetTTYLogging()
{
	return settings.value("Logger/TTY", true).toBool();
}

bool gui_settings::GetGamelistColVisibility(int col)
{
	settings.beginGroup("GameList");
	bool ret =settings.value("Col" + QString::number(col) + "visible", true).toBool();
	settings.endGroup();
	return ret;
}

int gui_settings::GetGamelistSortCol()
{
	return settings.value("GameList/sortCol", 1).toInt();
}

bool gui_settings::GetGamelistSortAsc()
{
	return settings.value("GameList/sortAsc", true).toBool();
}

QByteArray gui_settings::GetGameListState()
{
	return settings.value("GameList/state").toByteArray();
}

QStringList gui_settings::GetConfigEntries()
{
	QStringList nameFilter;
	nameFilter << "*.ini";
	QFileInfoList entries = settingsDir.entryInfoList(nameFilter, QDir::Files);
	QStringList res;
	for (QFileInfo entry : entries)
	{
		res.append(entry.baseName());
	}

	return res;
}

void gui_settings::BackupSettingsToTarget(const QString& friendlyName)
{	
	QSettings target(ComputeSettingsDir() + friendlyName + ".ini", QSettings::Format::IniFormat);
	QStringList keys = settings.allKeys();
	for (QStringList::iterator i = keys.begin(); i != keys.end(); i++)
	{
		if (!i->startsWith("Meta"))
		{
			target.setValue(*i, settings.value(*i));
		}
	}
	target.sync();
}

QString gui_settings::GetCurrentConfig()
{
	return settings.value("Meta/currentConfig", tr("CurrentSettings")).toString();
}

QString gui_settings::GetCurrentStylesheet()
{
	return settings.value("Meta/currentStylesheet", tr("default")).toString();
}

QStringList gui_settings::GetStylesheetEntries()
{
	QStringList nameFilter;
	nameFilter << "*.qss";
	QString path = settingsDir.absolutePath();
	QFileInfoList entries = settingsDir.entryInfoList(nameFilter, QDir::Files);
	QStringList res;
	for (QFileInfo entry : entries)
	{
		res.append(entry.baseName());
	}

	return res;
}

QString gui_settings::GetCurrentStylesheetPath()
{
	return settingsDir.absoluteFilePath(GetCurrentStylesheet() + ".qss");
}
