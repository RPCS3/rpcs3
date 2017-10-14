// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTimer>

#include "rpcs3_app.h"
#ifdef _WIN32
#include <windows.h>
#endif

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

namespace logs
{
	void set_init();
}

int main(int argc, char** argv)
{
	logs::set_init();

#ifdef _WIN32
	// use this instead of SetProcessDPIAware if Qt ever fully supports this on windows
	// at the moment it can't display QCombobox frames for example
	// I think there was an issue with gsframe if I recall correctly, so look out for that
	//QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	SetProcessDPIAware();

	timeBeginPeriod(1);

	atexit([]
	{
		timeEndPeriod(1);
	});
#else
	qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#endif

	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

	rpcs3_app app(argc, argv);

	// Command line args
	QCommandLineParser parser;
	parser.setApplicationDescription("Welcome to RPCS3 command line.");
	parser.addPositionalArgument("(S)ELF", "Path for directly executing a (S)ELF");
	parser.addHelpOption();
	parser.process(app);

	app.Init();

	if (parser.positionalArguments().length() > 0)
	{
		// Ugly workaround
		QTimer::singleShot(2, [path = sstr(QFileInfo(parser.positionalArguments().at(0)).canonicalFilePath())]
		{
			Emu.BootGame(path, true);
			Emu.Run();
		});
	}

	return app.exec();
}
