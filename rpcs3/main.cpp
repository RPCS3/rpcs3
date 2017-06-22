// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge

#include <QApplication>
#include <QCommandLineParser>

#include "rpcs3_app.h"
#ifdef _WIN32
#include <windows.h>
#endif

inline std::string sstr(const QString& _in) { return _in.toUtf8().toStdString(); }

int main(int argc, char** argv)
{
#ifdef _WIN32
	SetProcessDPIAware();
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
#else
	qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#endif

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
		Emu.SetPath(sstr(parser.positionalArguments().at(0)));
		Emu.Load();
		Emu.Run();
	}

	return app.exec();
}
