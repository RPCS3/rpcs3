// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge
#ifdef QT_UI

#include <QApplication>
#include <QDebug>
#include "GUI/mainwindow.h"

#include "stdafx.h"
#include "Emu/System.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	MainWindow mainWin;
	QSize defaultSize = QSize(800, 600);
	mainWin.resize(defaultSize);

	mainWin.show();

	// Simple code to test and make sure that everything is linking correctly with the emulator core. 
	qDebug() << Emu.IsPaused();
	return app.exec();
}

#endif // QT_UI
