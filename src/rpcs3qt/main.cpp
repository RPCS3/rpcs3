// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "glviewer.h"

int main(int argc, char *argv[])
{
	QGuiApplication app(argc, argv);

	qmlRegisterType<GLViewer>("GLViewer", 1, 0, "GLViewer");
	QQmlApplicationEngine engine(QUrl("qrc:/qml/main.qml"));

	return app.exec();
	Q_UNUSED(engine)
}
