// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include "rpcs3_app.h"

int main(int argc, char** argv)
{
#ifdef Q_OS_WIN
	SetProcessDPIAware();
#endif

	rpcs3_app app(argc, argv);
	app.Init();
	return app.exec();
}
