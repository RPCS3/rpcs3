// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge
#ifdef QT_UI

#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	MainWindow mainWin;
	QSize defaultSize = QSize(800, 600);
	mainWin.resize(defaultSize);

	mainWin.show();
	
	return app.exec();
}

#endif // QT_UI
