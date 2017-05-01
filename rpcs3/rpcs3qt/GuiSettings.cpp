#include "GuiSettings.h"

GuiSettings::GuiSettings(QObject* parent) : QSettings("GuiSettings.ini", QSettings::Format::IniFormat, parent)
{
}

GuiSettings::~GuiSettings()
{
	sync();
}

void GuiSettings::writeGuiGeometry(const QByteArray& res)
{
	beginGroup("MainWindow");
	setValue("geometry", res);
	endGroup();
}

void GuiSettings::writeGuiState(const QByteArray& res)
{
	beginGroup("MainWindow");
	setValue("windowState", res);
	endGroup();
}

void GuiSettings::setGamelistVisibility(bool val)
{
	beginGroup("MainWindow");
	setValue("gamelistVisible", val);
	endGroup();
}

void GuiSettings::setLoggerVisibility(bool val)
{
	beginGroup("MainWindow");
	setValue("loggerVisible", val);
	endGroup();
}

void GuiSettings::setDebuggerVisibility(bool val)
{
	beginGroup("MainWindow");
	setValue("debuggerVisible", val);
	endGroup();
}

QByteArray GuiSettings::readGuiGeometry()
{
	return value("MainWindow/geometry", QByteArray()).toByteArray();
}

QByteArray GuiSettings::readGuiState()
{
	return value("MainWindow/windowState", QByteArray()).toByteArray();
}

bool GuiSettings::getGameListVisibility()
{
	return value("MainWindow/gamelistVisible", true).toBool();
}

bool GuiSettings::getLoggerVisibility()
{
	return value("MainWindow/loggerVisible", true).toBool();
}

bool GuiSettings::getDebuggerVisibility()
{
	return value("MainWindow/debuggerVisible", false).toBool();
}
