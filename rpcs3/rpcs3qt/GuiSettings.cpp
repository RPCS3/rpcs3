#include "GuiSettings.h"

#include <QDir>

#ifdef _MSC_VER
const QString m_cGuiSettingsPath = "../bin/GuiConfigs/";
#else
const QString m_cGuiSettingsPath = "GuiConfigs/";
#endif

GuiSettings::GuiSettings(QObject* parent) : QObject(parent), settings(m_cGuiSettingsPath + "CurrentSettings.ini", QSettings::Format::IniFormat, parent),
	settingsDir(QDir::currentPath() + "/" + m_cGuiSettingsPath)
{
}

GuiSettings::~GuiSettings()
{
	settings.sync();
}

void GuiSettings::ChangeToConfig(const QString& name)
{
	if (name != "CurrentSettings")
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

void GuiSettings::Reset(bool removeMeta)
{
	if (removeMeta)
	{
		settings.clear();
	}
	else
	{
		settings.remove("Logger");
		settings.remove("MainWindow");
		settings.remove("GameList");
	}
}

void GuiSettings::WriteGuiGeometry(const QByteArray& res)
{
	settings.beginGroup("MainWindow");
	settings.setValue("geometry", res);
	settings.endGroup();
}

void GuiSettings::WriteGuiState(const QByteArray& res)
{
	settings.beginGroup("MainWindow");
	settings.setValue("windowState", res);
	settings.endGroup();
}

void GuiSettings::WriteGameListState(const QByteArray& res)
{
	settings.beginGroup("GameList");
	settings.setValue("state", res);
	settings.endGroup();
}

void GuiSettings::SetGamelistVisibility(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("gamelistVisible", val);
	settings.endGroup();
}

void GuiSettings::SetLoggerVisibility(bool val)
{
	settings.beginGroup("Logger");
	settings.setValue("loggerVisible", val);
	settings.endGroup();
}

void GuiSettings::SetDebuggerVisibility(bool val)
{
	settings.beginGroup("MainWindow");
	settings.setValue("debuggerVisible", val);
	settings.endGroup();
}

void GuiSettings::SetLogLevel(uint lev)
{
	settings.beginGroup("Logger");
	settings.setValue("level", lev);
	settings.endGroup();
}

void GuiSettings::SetTTYLogging(bool val)
{
	settings.beginGroup("Logger");
	settings.setValue("TTY", val);
	settings.endGroup();
}

void GuiSettings::SetGamelistColVisibility(int col, bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("Col" + QString::number(col) + "visible", val);
	settings.endGroup();
}

void GuiSettings::SetGamelistSortCol(int col)
{
	settings.beginGroup("GameList");
	settings.setValue("sortCol", col);
	settings.endGroup();
}

void GuiSettings::SetGamelistSortAsc(bool val)
{
	settings.beginGroup("GameList");
	settings.setValue("sortAsc", val);
	settings.endGroup();
}

void GuiSettings::SaveCurrentConfig(const QString& friendlyName)
{
	SetCurrentConfig(friendlyName);
	BackupSettingsToTarget(friendlyName);
}

void GuiSettings::SetCurrentConfig(const QString& friendlyName)
{
	settings.beginGroup("Meta");
	settings.setValue("currentConfig", friendlyName);
	settings.endGroup();
}

void GuiSettings::SetStyleSheet(const QString& friendlyName)
{
	settings.beginGroup("Meta");
	settings.setValue("currentStylesheet", friendlyName);
	settings.endGroup();
}

QByteArray GuiSettings::ReadGuiGeometry()
{
	return settings.value("MainWindow/geometry", QByteArray()).toByteArray();
}

QByteArray GuiSettings::ReadGuiState()
{
	return settings.value("MainWindow/windowState", QByteArray()).toByteArray();
}

bool GuiSettings::GetGamelistVisibility()
{
	return settings.value("GameList/gamelistVisible", true).toBool();
}

bool GuiSettings::GetLoggerVisibility()
{
	return settings.value("Logger/loggerVisible", true).toBool();
}

bool GuiSettings::GetDebuggerVisibility()
{
	return settings.value("MainWindow/debuggerVisible", false).toBool();
}

logs::level GuiSettings::GetLogLevel()
{
	return (logs::level) (settings.value("Logger/level", QVariant((uint)(logs::level::warning))).toUInt());
}

bool GuiSettings::GetTTYLogging()
{
	return settings.value("Logger/TTY", true).toBool();
}

bool GuiSettings::GetGamelistColVisibility(int col)
{
	settings.beginGroup("GameList");
	bool ret =settings.value("Col" + QString::number(col) + "visible", true).toBool();
	settings.endGroup();
	return ret;
}

int GuiSettings::GetGamelistSortCol()
{
	return settings.value("GameList/sortCol", 1).toInt();
}

bool GuiSettings::GetGamelistSortAsc()
{
	return settings.value("GameList/sortAsc", true).toBool();
}

QByteArray GuiSettings::GetGameListState()
{
	return settings.value("GameList/state").toByteArray();
}

QStringList GuiSettings::GetConfigEntries()
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

void GuiSettings::BackupSettingsToTarget(const QString& friendlyName)
{	
	QSettings target(m_cGuiSettingsPath + friendlyName + ".ini", QSettings::Format::IniFormat);
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

QString GuiSettings::GetCurrentConfig()
{
	return settings.value("Meta/currentConfig", QString("CurrentSettings")).toString();
}

QString GuiSettings::GetCurrentStylesheet()
{
	return settings.value("Meta/currentStylesheet", QString(tr("default"))).toString();
}

QStringList GuiSettings::GetStylesheetEntries()
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

QString GuiSettings::GetCurrentStylesheetPath()
{
	return settingsDir.absoluteFilePath(GetCurrentStylesheet() + ".qss");
}
