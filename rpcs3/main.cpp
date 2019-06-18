// Qt5.10+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge, Megamouse and flash-fire

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTimer>
#include <QObject>
#include <QMessageBox>
#include <QTextDocument>

#include "rpcs3qt/gui_application.h"

#include "rpcs3_app.h"
#include "Utilities/sema.h"
#ifdef _WIN32
#include <windows.h>
#include "Utilities/dynamic_library.h"
DYNAMIC_IMPORT("ntdll.dll", NtQueryTimerResolution, NTSTATUS(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution));
DYNAMIC_IMPORT("ntdll.dll", NtSetTimerResolution, NTSTATUS(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution));
#endif

#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#endif

#include "rpcs3_version.h"

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
	s_qt_mutex.lock();

	if (!s_qt_init.try_lock())
	{
		s_init.lock();
		static int argc = 1;
		static char arg1[] = {"ERROR"};
		static char* argv[] = {arg1};
		static QApplication app0{argc, argv};
	}

	auto show_report = [](const std::string& text)
	{
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
	};

#ifdef __APPLE__
	// Cocoa access is not allowed outside of the main thread
	if (!pthread_main_np())
	{
		dispatch_sync(dispatch_get_main_queue(), ^ { show_report(text); });
	}
	else
#endif
	{
		show_report(text);
	}

	std::abort();
}

const char* ARG_NO_GUI = "no-gui";

QCoreApplication* createApplication(int& argc, char* argv[])
{
	for (int i = 1; i < argc; ++i)
		if (!qstrcmp(argv[i], ARG_NO_GUI))
			return new rpcs3_app(argc, argv);
	return new gui_application(argc, argv);
}

int main(int argc, char** argv)
{
	logs::set_init();

#ifdef __linux__
	struct ::rlimit rlim;
	rlim.rlim_cur = 4096;
	rlim.rlim_max = 4096;
	if (::setrlimit(RLIMIT_NOFILE, &rlim) != 0)
		std::fprintf(stderr, "Failed to set max open file limit (4096).");
#endif

	s_init.unlock();
	s_qt_mutex.lock();

	QScopedPointer<QCoreApplication> app(createApplication(argc, argv));

	// Command line args
	QCommandLineParser parser;
	parser.setApplicationDescription("Welcome to RPCS3 command line.");
	parser.addPositionalArgument("(S)ELF", "Path for directly executing a (S)ELF");
	parser.addPositionalArgument("[Args...]", "Optional args for the executable");

	const QCommandLineOption helpOption    = parser.addHelpOption();
	const QCommandLineOption versionOption = parser.addVersionOption();
	parser.addOption(QCommandLineOption("no-gui"));
	parser.process(app->arguments());

	// Don't start up the full rpcs3 gui if we just want the version or help.
	if (parser.isSet(versionOption) || parser.isSet(helpOption))
		return 0;

	app->setApplicationVersion(qstr(rpcs3::version.to_string()));
	app->setApplicationName("RPCS3");

	if (auto gui_app = qobject_cast<gui_application*>(app.get()))
	{
#if defined(_WIN32) || defined(__APPLE__)
		app->setAttribute(Qt::AA_EnableHighDpiScaling);
#else
		setenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1", 0);
#endif
		app->setAttribute(Qt::AA_UseHighDpiPixmaps);
		app->setAttribute(Qt::AA_DisableWindowContextHelpButton);
		app->setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

		gui_app->Init();
	}
	else if (auto non_gui_app = qobject_cast<rpcs3_app*>(app.get()))
	{
		non_gui_app->Init();
	}

#ifdef _WIN32
	// Set 0.5 msec timer resolution for best performance
	// - As QT5 timers (QTimer) sets the timer resolution to 1 msec, override it here.
	// - Don't bother "unsetting" the timer resolution after the emulator stops as QT5 will still require the timer resolution to be set to 1 msec.
	ULONG min_res, max_res, orig_res, new_res;
	if (NtQueryTimerResolution(&min_res, &max_res, &orig_res) == 0)
	{
		NtSetTimerResolution(max_res, TRUE, &new_res);
	}
#endif

	QStringList args = parser.positionalArguments();

	if (args.length() > 0)
	{
		// Propagate command line arguments
		std::vector<std::string> argv;

		if (args.length() > 1)
		{
			args.removeAll(ARG_NO_GUI);

			argv.emplace_back();

			for (u32 i = 1; i < args.length(); i++)
			{
				argv.emplace_back(args[i].toStdString());
			}
		}

		// Ugly workaround
		QTimer::singleShot(2, [path = sstr(QFileInfo(args.at(0)).canonicalFilePath()), argv = std::move(argv)]() mutable
		{
			Emu.argv = std::move(argv);
			Emu.SetForceBoot(true);
			Emu.BootGame(path, "", true);
		});
	}

	s_qt_init.unlock();
	s_qt_mutex.unlock();

	// run event loop (maybe only needed for the gui application)
	return app->exec();
}
