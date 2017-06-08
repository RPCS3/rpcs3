// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include "rpcs3_app.h"
#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
	SetProcessDPIAware();
#else
	qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#endif

	rpcs3_app app(argc, argv);
	app.Init();
	return app.exec();
}
