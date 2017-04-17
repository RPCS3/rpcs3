// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include "GUI/mainwindow.h"

#include "stdafx.h"
#include "Emu/System.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	MainWindow mainWin;
	QSize defaultSize = QDesktopWidget().availableGeometry().size() * 0.7;
	mainWin.resize(defaultSize);

	mainWin.show();

	// Simple code to test and make sure that everything is linking correctly with the emulator core. 
	qDebug() << Emu.IsPaused();
	return app.exec();
}