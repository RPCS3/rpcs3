#include "gui_settings.h"

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
	if (GetCategoryHDDGameVisibility()) filterList.append("HDD Game");
	if (GetCategoryDiscGameVisibility()) filterList.append("Disc Game");
	if (GetCategoryHomeVisibility()) filterList.append("Home");
	if (GetCategoryAudioVideoVisibility()) filterList.append("Audio/Video");
	if (GetCategoryGameDataVisibility()) filterList.append("Game Data");
	return filterList;
}

bool gui_settings::GetCategoryHDDGameVisibility()
{
	return settings.value("GameList/categoryHDDGameVisible", true).toBool();
}

bool gui_settings::GetCategoryDiscGameVisibility()
{
	return settings.value("GameList/categoryDiscGameVisible", true).toBool();
}

bool gui_settings::GetCategoryHomeVisibility()
{
	return settings.value("GameList/categoryHomeVisible", true).toBool();
}

bool gui_settings::GetCategoryAudioVideoVisibility()
{
	return settings.value("GameList/categoryAudioVideoVisible", true).toBool();
}

bool gui_settings::GetCategoryGameDataVisibility()
{
	return settings.value("GameList/categoryGameDataVisible", true).toBool();
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

void gui_settings::SetCategoryHDDGameVisibility(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("categoryHDDGameVisible", val);
	settings.endGroup();
}

void gui_settings::SetCategoryDiscGameVisibility(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("categoryDiscGameVisible", val);
	settings.endGroup();
}

void gui_settings::SetCategoryHomeVisibility(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("categoryHomeVisible", val);
	settings.endGroup();
}

void gui_settings::SetCategoryAudioVideoVisibility(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("categoryAudioVideoVisible", val);
	settings.endGroup();
}

void gui_settings::SetCategoryGameDataVisibility(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("categoryGameDataVisible", val);
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
