// Qt5.2+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTimer>
#include <QObject>

#include "rpcs3_app.h"
#include "Utilities/sema.h"
#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#endif

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

template <typename... Args>
inline auto tr(Args&&... args)
{
	return QObject::tr(std::forward<Args>(args)...);
}

namespace logs
{
	void set_init();
}

static semaphore<> s_init{0};
static semaphore<> s_qt_init{0};
static semaphore<> s_qt_mutex{};

[[noreturn]] extern void report_fatal_error(const std::string& text)
{
	s_qt_mutex.wait();

	if (!s_qt_init.try_wait())
	{
		s_init.wait();
		static int argc = 1;
		static char arg1[] = {"ERROR"};
		static char* argv[] = {arg1};
		static QApplication app0{argc, argv};
	}

	QMessageBox msg;
	msg.setWindowTitle(tr("RPCS3: Fatal Error"));
	msg.setIcon(QMessageBox::Critical);
	msg.setTextFormat(Qt::RichText);
	msg.setText(QString(R"(
		<p style="white-space: nowrap;">
			%1<br>
			%2<br>
			<a href='https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support'>https://github.com/RPCS3/rpcs3/wiki/How-to-ask-for-Support</a><br>
			%3<br>
		</p>
		)")
		.arg(Qt::convertFromPlainText(QString::fromStdString(text)))
		.arg(tr("HOW TO REPORT ERRORS:"))
		.arg(tr("Please, don't send incorrect reports. Thanks for understanding.")));
	msg.layout()->setSizeConstraint(QLayout::SetFixedSize);
	msg.exec();

	std::abort();
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
#else
	qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
#endif

#ifdef __linux__
	struct ::rlimit rlim;
	rlim.rlim_cur = 4096;
	rlim.rlim_max = 4096;
	if (::setrlimit(RLIMIT_NOFILE, &rlim) != 0)
		std::fprintf(stderr, "Failed to set max open file limit (4096).");
#endif

	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
	QCoreApplication::setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

	s_init.post();
	s_qt_mutex.wait();
	rpcs3_app app(argc, argv);

	// Command line args
	QCommandLineParser parser;
	parser.setApplicationDescription("Welcome to RPCS3 command line.");
	parser.addPositionalArgument("(S)ELF", "Path for directly executing a (S)ELF");
	parser.addPositionalArgument("[Args...]", "Optional args for the executable");
	parser.addHelpOption();
	parser.parse(QCoreApplication::arguments());

	app.Init();

	QStringList args = parser.positionalArguments();

	if (args.length() > 0)
	{
		// Propagate command line arguments
		std::vector<std::string> argv;

		if (args.length() > 1)
		{
			argv.emplace_back();

			for (int i = 1; i < args.length(); i++)
			{
				argv.emplace_back(args[i].toStdString());
			}
		}

		// Ugly workaround
		QTimer::singleShot(2, [path = sstr(QFileInfo(args.at(0)).canonicalFilePath()), argv = std::move(argv)]() mutable
		{
			Emu.argv = std::move(argv);
			Emu.SetForceBoot(true);
			Emu.BootGame(path, true);
		});
	}

	s_qt_init.post();
	s_qt_mutex.post();
	return app.exec();
}
