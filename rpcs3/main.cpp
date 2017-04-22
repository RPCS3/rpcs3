// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include "RPCS3App.h"

int main(int argc, char *argv[])
{
	RPCS3App app(argc, argv);
	app.Init();
	return app.exec();
}
