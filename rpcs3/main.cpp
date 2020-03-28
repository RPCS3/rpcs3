// Qt5.10+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge, Megamouse and flash-fire

#include <iostream>

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QLayout>
#include <QTimer>
#include <QObject>
#include <QMessageBox>
#include <QTextDocument>
#include <QStyleFactory>

#include "rpcs3qt/gui_application.h"

#include "headless_application.h"
#include "Utilities/sema.h"
#ifdef _WIN32
#include <windows.h>
#include "Utilities/dynamic_library.h"
DYNAMIC_IMPORT("ntdll.dll", NtQueryTimerResolution, NTSTATUS(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution));
DYNAMIC_IMPORT("ntdll.dll", NtSetTimerResolution, NTSTATUS(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution));
#else
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#endif

#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef __APPLE__
#include <dispatch/dispatch.h>
#endif

#include "Utilities/sysinfo.h"
#include "Utilities/Config.h"
#include "rpcs3_version.h"
#include "Emu/System.h"
#include <thread>
#include <charconv>

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

template <typename... Args>
inline auto tr(Args&&... args)
{
	return QObject::tr(std::forward<Args>(args)...);
}

static semaphore<> s_qt_init;

static atomic_t<char*> s_argv0;

#ifndef _WIN32
extern char **environ;
#endif

LOG_CHANNEL(sys_log, "SYS");

[[noreturn]] extern void report_fatal_error(const std::string& text)
{
	const bool local = s_qt_init.try_lock();

	// Possibly created and assigned here
	QScopedPointer<QCoreApplication> app;

	if (local)
	{
		static int argc = 1;
		static char* argv[] = {+s_argv0};
		app.reset(new QApplication{argc, argv});
	}

	if (!local)
	{
		fprintf(stderr, "RPCS3: %s\n", text.c_str());
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
		// If Qt is already initialized, spawn a new RPCS3 process with an --error argument
		if (local)
		{
			// Since we only show an error, we can hope for a graceful exit
			show_report(text);
			app.reset();
			std::exit(0);
		}
		else
		{
#ifdef _WIN32
			wchar_t buffer[32767];
			GetModuleFileNameW(nullptr, buffer, sizeof(buffer) / 2);
			std::wstring arg(text.cbegin(), text.cend()); // ignore unicode for now
			_wspawnl(_P_WAIT, buffer, buffer, L"--error", arg.c_str(), nullptr);
#else
			pid_t pid;
			std::vector<char> data(text.data(), text.data() + text.size() + 1);
			std::string err_arg = "--error";
			char* argv[] = {+s_argv0, err_arg.data(), data.data(), nullptr};
			int ret = posix_spawn(&pid, +s_argv0, nullptr, nullptr, argv, environ);

			if (ret == 0)
			{
				int status;
				waitpid(pid, &status, 0);
			}
			else
			{
				fprintf(stderr, "posix_spawn() failed: %d\n", ret);
			}
#endif
			std::abort();
		}
	}

	std::abort();
}

struct pause_on_fatal final : logs::listener
{
	~pause_on_fatal() override = default;

	void log(u64 /*stamp*/, const logs::message& msg, const std::string& /*prefix*/, const std::string& /*text*/) override
	{
		if (msg.sev == logs::level::fatal)
		{
			// Pause emulation if fatal error encountered
			Emu.Pause();
		}
	}
};

const char* arg_headless   = "headless";
const char* arg_no_gui     = "no-gui";
const char* arg_high_dpi   = "hidpi";
const char* arg_rounding   = "dpi-rounding";
const char* arg_styles     = "styles";
const char* arg_style      = "style";
const char* arg_stylesheet = "stylesheet";
const char* arg_error      = "error";
const char* arg_updating   = "updating";

int find_arg(std::string arg, int& argc, char* argv[])
{
	arg = "--" + arg;
	for (int i = 1; i < argc; ++i)
		if (!strcmp(arg.c_str(), argv[i]))
			return i;
	return 0;
}

QCoreApplication* createApplication(int& argc, char* argv[])
{
	if (find_arg(arg_headless, argc, argv))
		return new headless_application(argc, argv);

#ifdef __linux__
	// set the DISPLAY variable in order to open web browsers
	if (qEnvironmentVariable("DISPLAY", "").isEmpty())
	{
		qputenv("DISPLAY", ":0");
	}
#endif

	bool use_high_dpi = true;

	const auto i_hdpi = find_arg(arg_high_dpi, argc, argv);
	if (i_hdpi)
	{
		const std::string cmp_str = "0";
		const auto i_hdpi_2 = (argc > (i_hdpi + 1)) ? (i_hdpi + 1) : 0;
		const auto high_dpi_setting = (i_hdpi_2 && !strcmp(cmp_str.c_str(), argv[i_hdpi_2])) ? "0" : "1";

		// Set QT_ENABLE_HIGHDPI_SCALING from environment. Defaults to cli argument, which defaults to 1.
		use_high_dpi = "1" == qEnvironmentVariable("QT_ENABLE_HIGHDPI_SCALING", high_dpi_setting);
	}

	// AA_EnableHighDpiScaling has to be set before creating a QApplication
	QApplication::setAttribute(use_high_dpi ? Qt::AA_EnableHighDpiScaling : Qt::AA_DisableHighDpiScaling);

	if (use_high_dpi)
	{
		// Set QT_SCALE_FACTOR_ROUNDING_POLICY from environment. Defaults to cli argument, which defaults to RoundPreferFloor.
		auto rounding_val = Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;
		auto rounding_str = std::to_string(static_cast<int>(rounding_val));
		const auto i_rounding = find_arg(arg_rounding, argc, argv);

		if (i_rounding)
		{
			const auto i_rounding_2 = (argc > (i_rounding + 1)) ? (i_rounding + 1) : 0;

			if (i_rounding_2)
			{
				const auto arg_val = argv[i_rounding_2];
				const auto arg_len = std::strlen(arg_val);
				s64 rounding_val_cli = 0;

				if (!cfg::try_to_int64(&rounding_val_cli, arg_val, static_cast<int>(Qt::HighDpiScaleFactorRoundingPolicy::Unset), static_cast<int>(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough)))
				{
					std::cout << "The value " << arg_val << " for " << arg_rounding << " is not allowed. Please use a valid value for Qt::HighDpiScaleFactorRoundingPolicy.\n";
				}
				else
				{
					rounding_val = static_cast<Qt::HighDpiScaleFactorRoundingPolicy>(static_cast<int>(rounding_val_cli));
					rounding_str = std::to_string(static_cast<int>(rounding_val));
				}
			}
		}

		{
			rounding_str = qEnvironmentVariable("QT_SCALE_FACTOR_ROUNDING_POLICY", rounding_str.c_str()).toStdString();

			s64 rounding_val_final = 0;

			if (cfg::try_to_int64(&rounding_val_final, rounding_str, static_cast<int>(Qt::HighDpiScaleFactorRoundingPolicy::Unset), static_cast<int>(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough)))
			{
				rounding_val = static_cast<Qt::HighDpiScaleFactorRoundingPolicy>(static_cast<int>(rounding_val_final));
				rounding_str = std::to_string(static_cast<int>(rounding_val));
			}
			else
			{
				std::cout << "The value " << rounding_str << " for " << arg_rounding << " is not allowed. Please use a valid value for Qt::HighDpiScaleFactorRoundingPolicy.\n";
			}
		}
		QApplication::setHighDpiScaleFactorRoundingPolicy(rounding_val);
	}

	return new gui_application(argc, argv);
}



int main(int argc, char** argv)
{
#ifdef _WIN32
	ULONG64 intro_cycles{};
	QueryThreadCycleTime(GetCurrentThread(), &intro_cycles);
#elif defined(RUSAGE_THREAD)
	struct ::rusage intro_stats{};
	::getrusage(RUSAGE_THREAD, &intro_stats);
	const u64 intro_time = (intro_stats.ru_utime.tv_sec + intro_stats.ru_stime.tv_sec) * 1000000000ull + (intro_stats.ru_utime.tv_usec + intro_stats.ru_stime.tv_usec) * 1000ull;
#endif

	s_argv0 = argv[0]; // Save for report_fatal_error

	// Only run RPCS3 to display an error
	if (int err_pos = find_arg(arg_error, argc, argv))
	{
		// Reconstruction of the error from multiple args
		std::string error;
		for (int i = err_pos + 1; i < argc; i++)
		{
			if (i > err_pos + 1)
				error += ' ';
			error += argv[i];
		}

		report_fatal_error(error);
	}

	const std::string lock_name = fs::get_cache_dir() + "RPCS3.buf";

	fs::file instance_lock;

	// True if an argument --updating found
	const bool is_updating = find_arg(arg_updating, argc, argv) != 0;

	// Keep trying to lock the file for ~2s normally, and for ~10s in the case of --updating
	for (u32 num = 0; num < (is_updating ? 500u : 100u) && !instance_lock.open(lock_name, fs::rewrite + fs::lock); num++)
	{
		std::this_thread::sleep_for(20ms);
	}

	if (!instance_lock)
	{
		if (fs::g_tls_error == fs::error::acces)
		{
			if (fs::exists(lock_name))
			{
				report_fatal_error("Another instance of RPCS3 is running. Close it or kill its process, if necessary.");
			}
			else
			{
				report_fatal_error("Cannot create RPCS3.log (access denied)."
#ifdef _WIN32
				"\nNote that RPCS3 cannot be installed in Program Files or similar directory with limited permissions."
#else
				"\nPlease, check RPCS3 permissions in '~/.config/rpcs3'."
#endif
				);
			}
		}
		else
		{
			report_fatal_error(fmt::format("Cannot create RPCS3.log (error %s)", fs::g_tls_error));
		}

		return 1;
	}

	std::unique_ptr<logs::listener> log_file;
	{
		// Check free space
		fs::device_stat stats{};
		if (!fs::statfs(fs::get_cache_dir(), stats) || stats.avail_free < 128 * 1024 * 1024)
		{
			report_fatal_error(fmt::format("Not enough free space (%f KB)", stats.avail_free / 1000000.));
			return 1;
		}

		// Limit log size to ~25% of free space
		log_file = logs::make_file_listener(fs::get_cache_dir() + "RPCS3.log", stats.avail_free / 4);
	}

	std::unique_ptr<logs::listener> log_pauser = std::make_unique<pause_on_fatal>();
	logs::listener::add(log_pauser.get());

	{
		const std::string firmware_version = utils::get_firmware_version();
		const std::string firmware_string  = firmware_version.empty() ? "" : (" | Firmware version: " + firmware_version);

		// Write initial message
		logs::stored_message ver;
		ver.m.ch  = nullptr;
		ver.m.sev = logs::level::always;
		ver.stamp = 0;
		ver.text  = fmt::format("RPCS3 v%s | %s%s\n%s", rpcs3::get_version().to_string(), rpcs3::get_branch(), firmware_string, utils::get_system_info());

		// Write OS version
		logs::stored_message os;
		os.m.ch  = nullptr;
		os.m.sev = logs::level::notice;
		os.stamp = 0;
		os.text = utils::get_OS_version();

		logs::set_init({std::move(ver), std::move(os)});
	}

#ifdef _WIN32
	sys_log.notice("Initialization times before main(): %fGc", intro_cycles / 1000000000.);
#elif defined(RUSAGE_THREAD)
	sys_log.notice("Initialization times before main(): %fs", intro_time / 1000000000.);
#endif

#ifdef __linux__
	struct ::rlimit rlim;
	rlim.rlim_cur = 4096;
	rlim.rlim_max = 4096;
	if (::setrlimit(RLIMIT_NOFILE, &rlim) != 0)
		std::fprintf(stderr, "Failed to set max open file limit (4096).");
	// Work around crash on startup on KDE: https://bugs.kde.org/show_bug.cgi?id=401637
	setenv( "KDE_DEBUG", "1", 0 );
#endif

	std::lock_guard qt_init(s_qt_init);

	// The constructor of QApplication eats the --style and --stylesheet arguments.
	// By checking for stylesheet().isEmpty() we could implicitly know if a stylesheet was passed,
	// but I haven't found an implicit way to check for style yet, so we naively check them both here for now.
	const bool use_cli_style = find_arg(arg_style, argc, argv) || find_arg(arg_stylesheet, argc, argv);

	QScopedPointer<QCoreApplication> app(createApplication(argc, argv));
	app->setApplicationVersion(QString::fromStdString(rpcs3::get_version().to_string()));
	app->setApplicationName("RPCS3");

	// Command line args
	QCommandLineParser parser;
	parser.setApplicationDescription("Welcome to RPCS3 command line.");
	parser.addPositionalArgument("(S)ELF", "Path for directly executing a (S)ELF");
	parser.addPositionalArgument("[Args...]", "Optional args for the executable");

	const QCommandLineOption helpOption    = parser.addHelpOption();
	const QCommandLineOption versionOption = parser.addVersionOption();
	parser.addOption(QCommandLineOption(arg_headless, "Run RPCS3 in headless mode."));
	parser.addOption(QCommandLineOption(arg_no_gui, "Run RPCS3 without its GUI."));
	parser.addOption(QCommandLineOption(arg_high_dpi, "Enables Qt High Dpi Scaling.", "enabled", "1"));
	parser.addOption(QCommandLineOption(arg_rounding, "Sets the Qt::HighDpiScaleFactorRoundingPolicy for values like 150% zoom.", "rounding", "4"));
	parser.addOption(QCommandLineOption(arg_styles, "Lists the available styles."));
	parser.addOption(QCommandLineOption(arg_style, "Loads a custom style.", "style", ""));
	parser.addOption(QCommandLineOption(arg_stylesheet, "Loads a custom stylesheet.", "path", ""));
	parser.addOption(QCommandLineOption(arg_updating, "For internal usage."));
	parser.process(app->arguments());

	// Don't start up the full rpcs3 gui if we just want the version or help.
	if (parser.isSet(versionOption) || parser.isSet(helpOption))
		return 0;

	if (parser.isSet(arg_styles))
	{
#ifdef _WIN32
		if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
			[[maybe_unused]] const auto con_out = freopen("CONOUT$", "w", stdout);
#endif
		for (const auto& style : QStyleFactory::keys())
			std::cout << "\n" << style.toStdString();

		return 0;
	}

	if (auto gui_app = qobject_cast<gui_application*>(app.data()))
	{
		gui_app->setAttribute(Qt::AA_UseHighDpiPixmaps);
		gui_app->setAttribute(Qt::AA_DisableWindowContextHelpButton);
		gui_app->setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

		gui_app->SetShowGui(!parser.isSet(arg_no_gui));
		gui_app->SetUseCliStyle(use_cli_style);
		gui_app->Init();
	}
	else if (auto headless_app = qobject_cast<headless_application*>(app.data()))
	{
		headless_app->Init();
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
			argv.emplace_back();

			for (int i = 1; i < args.length(); i++)
			{
				argv.emplace_back(args[i].toStdString());
			}
		}

		// Ugly workaround
		QTimer::singleShot(2, [path = sstr(QFileInfo(args.at(0)).absoluteFilePath()), argv = std::move(argv)]() mutable
		{
			Emu.argv = std::move(argv);
			Emu.SetForceBoot(true);
			Emu.BootGame(path, "", true);
		});
	}

	// run event loop (maybe only needed for the gui application)
	return app->exec();
}
