// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge
#ifdef QT_UI

#include <QApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "glviewer.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    /*
    QGuiApplication app(argc, argv);

	qmlRegisterType<GLViewer>("GLViewer", 1, 0, "GLViewer");
	QQmlApplicationEngine engine(QUrl("qrc:/qml/main.qml"));

	return app.exec();
    */

    QApplication app(argc, argv);

    MainWindow mainWin;
    mainWin.show();

    return app.exec();
}

#endif // QT_UI
