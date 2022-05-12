// Qt5.10+ frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge, Megamouse and flash-fire

#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTimer>
#include <QObject>
#include <QStyleFactory>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QMetaEnum>
#include <QStandardPaths>

#include "rpcs3qt/gui_application.h"
#include "rpcs3qt/fatal_error_dialog.h"
#include "rpcs3qt/curl_handle.h"
#include "rpcs3qt/main_window.h"

#include "headless_application.h"
#include "Utilities/sema.h"
#include "Crypto/decrypt_binaries.h"
#ifdef _WIN32
#include <windows.h>
#include "util/dyn_lib.hpp"

// TODO(cjj19970505@live.cn)
// When compiling with WIN32_LEAN_AND_MEAN definition
// NTSTATUS is defined in CMake build but not in VS build
// May be caused by some different header pre-inclusion between CMake and VS configurations.
#if !defined(NTSTATUS)
// Copied from ntdef.h
typedef _Return_type_success_(return >= 0) LONG NTSTATUS;
#endif
DYNAMIC_IMPORT("ntdll.dll", NtQueryTimerResolution, NTSTATUS(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution));
DYNAMIC_IMPORT("ntdll.dll", NtSetTimerResolution, NTSTATUS(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution));
#else
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#endif

#ifdef __linux__
#include <sys/time.h>
#include <sys/resource.h>
#endif

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include "Utilities/Config.h"
#include "Utilities/Thread.h"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "rpcs3_version.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include <thread>
#include <charconv>

#include "util/sysinfo.hpp"

inline std::string sstr(const QString& _in) { return _in.toStdString(); }

static semaphore<> s_qt_init;

static atomic_t<bool> s_headless = false;
static atomic_t<bool> s_no_gui = false;
static atomic_t<char*> s_argv0;

extern thread_local std::string(*g_tls_log_prefix)();

#ifndef _WIN32
extern char **environ;
#endif

LOG_CHANNEL(sys_log, "SYS");
LOG_CHANNEL(q_debug, "QDEBUG");

[[noreturn]] extern void report_fatal_error(std::string_view _text)
{
	std::string buf;

	// Check if thread id is in string
	if (_text.find("\nThread id = "sv) == umax)
	{
		// Copy only when needed
		buf = std::string(_text);

		// Always print thread id
		fmt::append(buf, "\n\nThread id = %s.", std::this_thread::get_id());
	}

	std::string_view text = buf.empty() ? _text : buf;

	if (s_headless)
	{
#ifdef _WIN32
		if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
			[[maybe_unused]] const auto con_out = freopen("conout$", "w", stderr);
#endif
		std::cerr << fmt::format("RPCS3: %s\n", text);
		std::abort();
	}

	const bool local = s_qt_init.try_lock();

	// Possibly created and assigned here
	static QScopedPointer<QCoreApplication> app;

	if (local)
	{
		static int argc = 1;
		static char* argv[] = {+s_argv0};
		app.reset(new QApplication{argc, argv});
	}
	else
	{
		std::cerr << fmt::format("RPCS3: %s\n", text);
	}

	static auto show_report = [](std::string_view text)
	{
		fatal_error_dialog dlg(text);
		dlg.exec();
	};

#if defined(__APPLE__)
	if (!pthread_main_np())
	{
		dispatch_sync_f(dispatch_get_main_queue(), &text, [](void* text){ show_report(*static_cast<std::string_view*>(text)); });
	}
	else
#endif
	{
		// If Qt is already initialized, spawn a new RPCS3 process with an --error argument
		if (local)
		{
			show_report(text);
#ifdef _WIN32
			ExitProcess(0);
#else
			kill(getpid(), SIGKILL);
#endif
		}

#ifdef _WIN32
		wchar_t buffer[32767];
		GetModuleFileNameW(nullptr, buffer, sizeof(buffer) / 2);
		const std::wstring arg(text.cbegin(), text.cend()); // ignore unicode for now
		_wspawnl(_P_WAIT, buffer, buffer, L"--error", arg.c_str(), nullptr);
#else
		pid_t pid;
		std::vector<char> data(text.data(), text.data() + text.size() + 1);
		std::string run_arg = +s_argv0;
		std::string err_arg = "--error";

		if (run_arg.find_first_of('/') == umax)
		{
			// AppImage has "rpcs3" in argv[0], can't just execute it
#ifdef __linux__
			char buffer[PATH_MAX]{};
			if (::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1) > 0)
			{
				printf("Found exec link: %s\n", buffer);
				run_arg = buffer;
			}
#endif
		}

		char* argv[] = {run_arg.data(), err_arg.data(), data.data(), nullptr};
		int ret = posix_spawn(&pid, run_arg.c_str(), nullptr, nullptr, argv, environ);

		if (ret == 0)
		{
			int status;
			waitpid(pid, &status, 0);
		}
		else
		{
			std::fprintf(stderr, "posix_spawn() failed: %d\n", ret);
		}
#endif
	}

	std::abort();
}

struct fatal_error_listener final : logs::listener
{
	~fatal_error_listener() override = default;

	void log(u64 /*stamp*/, const logs::message& msg, const std::string& prefix, const std::string& text) override
	{
		if (msg == logs::level::fatal)
		{
			std::string _msg = "RPCS3: ";

			if (!prefix.empty())
			{
				_msg += prefix;
				_msg += ": ";
			}

			if (msg->name && '\0' != *msg->name)
			{
				_msg += msg->name;
				_msg += ": ";
			}

			_msg += text;
			_msg += '\n';

#ifdef _WIN32
			// If launched from CMD
			if (AttachConsole(ATTACH_PARENT_PROCESS))
				[[maybe_unused]] const auto con_out = freopen("CONOUT$", "w", stderr);
#endif
			// Output to error stream as is
			std::cerr << _msg;

#ifdef _WIN32
			if (IsDebuggerPresent())
			{
				// Output string to attached debugger
				OutputDebugStringA(_msg.c_str());
			}
#endif
			// Pause emulation if fatal error encountered
			Emu.Pause(true);
		}
	}
};

// Arguments that force a headless application (need to be checked in create_application)
constexpr auto arg_headless     = "headless";
constexpr auto arg_decrypt      = "decrypt";
constexpr auto arg_commit_db    = "get-commit-db";

// Arguments that can be used with a gui application
constexpr auto arg_no_gui       = "no-gui";
constexpr auto arg_high_dpi     = "hidpi";
constexpr auto arg_rounding     = "dpi-rounding";
constexpr auto arg_styles       = "styles";
constexpr auto arg_style        = "style";
constexpr auto arg_stylesheet   = "stylesheet";
constexpr auto arg_config       = "config";
constexpr auto arg_q_debug      = "qDebug";
constexpr auto arg_error        = "error";
constexpr auto arg_updating     = "updating";
constexpr auto arg_user_id      = "user-id";
constexpr auto arg_installfw    = "installfw";
constexpr auto arg_installpkg   = "installpkg";
constexpr auto arg_timer        = "high-res-timer";
constexpr auto arg_verbose_curl = "verbose-curl";
constexpr auto arg_any_location = "allow-any-location";

int find_arg(std::string arg, int& argc, char* argv[])
{
	arg = "--" + arg;
	for (int i = 0; i < argc; ++i) // It's not guaranteed that argv 0 is the executable.
		if (!strcmp(arg.c_str(), argv[i]))
			return i;
	return -1;
}

QCoreApplication* create_application(int& argc, char* argv[])
{
	if (find_arg(arg_headless, argc, argv) != -1 ||
		find_arg(arg_decrypt, argc, argv) != -1 ||
		find_arg(arg_commit_db, argc, argv) != -1)
	{
		return new headless_application(argc, argv);
	}

#ifdef __linux__
	// set the DISPLAY variable in order to open web browsers
	if (qEnvironmentVariable("DISPLAY", "").isEmpty())
	{
		qputenv("DISPLAY", ":0");
	}

	// Set QT_AUTO_SCREEN_SCALE_FACTOR to 0. This value is deprecated but still seems to make problems on some distros
	if (!qEnvironmentVariable("QT_AUTO_SCREEN_SCALE_FACTOR", "").isEmpty())
	{
		qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0");
	}
#endif

	bool use_high_dpi = true;

	const int i_hdpi = find_arg(arg_high_dpi, argc, argv);
	if (i_hdpi != -1)
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
		// Set QT_SCALE_FACTOR_ROUNDING_POLICY from environment. Defaults to cli argument, which defaults to PassThrough.
		Qt::HighDpiScaleFactorRoundingPolicy rounding_val = Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;
		const QMetaEnum meta_enum = QMetaEnum::fromType<Qt::HighDpiScaleFactorRoundingPolicy>();
		QString rounding_str_cli = meta_enum.valueToKey(static_cast<int>(rounding_val));

		const auto check_dpi_rounding_arg = [&rounding_str_cli, &rounding_val, &meta_enum](const char* val) -> bool
		{
			// Try to find out if the argument is a valid string representation of Qt::HighDpiScaleFactorRoundingPolicy
			bool ok{false};
			if (const int enum_index = meta_enum.keyToValue(val, &ok); ok)
			{
				rounding_str_cli = meta_enum.valueToKey(enum_index);
				rounding_val = static_cast<Qt::HighDpiScaleFactorRoundingPolicy>(enum_index);
			}
			return ok;
		};

		if (const int i_rounding = find_arg(arg_rounding, argc, argv); i_rounding != -1)
		{
			if (const int i_rounding_2 = i_rounding + 1; argc > i_rounding_2)
			{
				if (const auto arg_val = argv[i_rounding_2]; !check_dpi_rounding_arg(arg_val))
				{
					const std::string msg = fmt::format("The command line value %s for %s is not allowed. Please use a valid value for Qt::HighDpiScaleFactorRoundingPolicy.", arg_val, arg_rounding);
					sys_log.error("%s", msg); // Don't exit with fatal error. The resulting dialog might be unreadable with dpi problems.
					std::cerr << msg << std::endl;
				}
			}
		}

		// Get the environment variable. Fallback to cli argument.
		rounding_str_cli = qEnvironmentVariable("QT_SCALE_FACTOR_ROUNDING_POLICY", rounding_str_cli);

		if (!check_dpi_rounding_arg(rounding_str_cli.toStdString().c_str()))
		{
			const std::string msg = fmt::format("The value %s for the environment variable QT_SCALE_FACTOR_ROUNDING_POLICY is not allowed. Please use a valid value for Qt::HighDpiScaleFactorRoundingPolicy.", rounding_str_cli.toStdString());
			sys_log.error("%s", msg); // Don't exit with fatal error. The resulting dialog might be unreadable with dpi problems.
			std::cerr << msg << std::endl;
		}

		QApplication::setHighDpiScaleFactorRoundingPolicy(rounding_val);
	}

	return new gui_application(argc, argv);
}

void log_q_debug(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	Q_UNUSED(context)

	switch (type)
	{
	case QtDebugMsg: q_debug.trace("%s", msg.toStdString()); break;
	case QtInfoMsg: q_debug.notice("%s", msg.toStdString()); break;
	case QtWarningMsg: q_debug.warning("%s", msg.toStdString()); break;
	case QtCriticalMsg: q_debug.error("%s", msg.toStdString()); break;
	case QtFatalMsg: q_debug.fatal("%s", msg.toStdString()); break;
	}
}

template <>
void fmt_class_string<std::chrono::sys_time<typename std::chrono::system_clock::duration>>::format(std::string& out, u64 arg)
{
	std::ostringstream ss;
	const std::time_t dateTime = std::chrono::system_clock::to_time_t(get_object(arg));
 	const std::tm tm = *std::localtime(&dateTime);
	ss << std::put_time(&tm, "%Y-%m-%eT%H:%M:%S");
 	out += ss.str();
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
	if (int err_pos = find_arg(arg_error, argc, argv); err_pos != -1)
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

	static fs::file instance_lock;

	// True if an argument --updating found
	const bool is_updating = find_arg(arg_updating, argc, argv) != -1;

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

			report_fatal_error("Cannot create RPCS3.log (access denied)."
#ifdef _WIN32
				"\nNote that RPCS3 cannot be installed in Program Files or similar directories with limited permissions."
#else
				"\nPlease, check RPCS3 permissions in '~/.config/rpcs3'."
#endif
			);
		}

		report_fatal_error(fmt::format("Cannot create RPCS3.log (error %s)", fs::g_tls_error));
	}

#ifdef _WIN32
	if (!SetProcessWorkingSetSize(GetCurrentProcess(), 0x80000000, 0xC0000000)) // 2-3 GiB
	{
		report_fatal_error("Not enough memory for RPCS3 process.");
	}

	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

	ensure(thread_ctrl::is_main(), "Not main thread");

	// Initialize TSC freq (in case it isn't)
	static_cast<void>(utils::get_tsc_freq());

	// Initialize thread pool finalizer (on first use)
	static_cast<void>(named_thread("", [](int) {}));

	static std::unique_ptr<logs::listener> log_file;
	{
		// Check free space
		fs::device_stat stats{};
		if (!fs::statfs(fs::get_cache_dir(), stats) || stats.avail_free < 128 * 1024 * 1024)
		{
			report_fatal_error(fmt::format("Not enough free space (%f KB)", stats.avail_free / 1000000.));
		}

		// Limit log size to ~25% of free space
		log_file = logs::make_file_listener(fs::get_cache_dir() + "RPCS3.log", stats.avail_free / 4);
	}

	static std::unique_ptr<logs::listener> fatal_listener = std::make_unique<fatal_error_listener>();
	logs::listener::add(fatal_listener.get());

	{
		// Write RPCS3 version
		logs::stored_message ver{sys_log.always()};
		ver.text = fmt::format("RPCS3 v%s", rpcs3::get_verbose_version());

		// Write System information
		logs::stored_message sys{sys_log.always()};
		sys.text = utils::get_system_info();

		// Write OS version
		logs::stored_message os{sys_log.always()};
		os.text = utils::get_OS_version();

		// Write Qt version
		logs::stored_message qt{(strcmp(QT_VERSION_STR, qVersion()) != 0) ? sys_log.error : sys_log.notice};
		qt.text = fmt::format("Qt version: Compiled against Qt %s | Run-time uses Qt %s", QT_VERSION_STR, qVersion());

		logs::stored_message time{sys_log.always()};
		time.text = fmt::format("Current Time: %s", std::chrono::system_clock::now());

		logs::set_init({std::move(ver), std::move(sys), std::move(os), std::move(qt), std::move(time)});
	}

#ifdef _WIN32
	sys_log.notice("Initialization times before main(): %fGc", intro_cycles / 1000000000.);
#elif defined(RUSAGE_THREAD)
	sys_log.notice("Initialization times before main(): %fs", intro_time / 1000000000.);
#endif

	std::string argument_str;
	for (int i = 0; i < argc; i++)
	{
		argument_str += "'" + std::string(argv[i]) + "'";
		if (i != argc - 1) argument_str += " ";
	}
	sys_log.notice("argc: %d, argv: %s", argc, argument_str);

#ifdef __linux__
	struct ::rlimit rlim;
	rlim.rlim_cur = 4096;
	rlim.rlim_max = 4096;
#ifdef RLIMIT_NOFILE
	if (::setrlimit(RLIMIT_NOFILE, &rlim) != 0)
	{
		std::cerr << "Failed to set max open file limit (4096).\n";
	}
#endif

	rlim.rlim_cur = 0x80000000;
	rlim.rlim_max = 0x80000000;
#ifdef RLIMIT_MEMLOCK
	if (::setrlimit(RLIMIT_MEMLOCK, &rlim) != 0)
	{
		std::cerr << "Failed to set RLIMIT_MEMLOCK size to 2 GiB. Try to update your system configuration.\n";
	}
#endif
	// Work around crash on startup on KDE: https://bugs.kde.org/show_bug.cgi?id=401637
	setenv( "KDE_DEBUG", "1", 0 );
#endif

#ifdef __APPLE__
	struct ::rlimit rlim;
	::getrlimit(RLIMIT_NOFILE, &rlim);
	rlim.rlim_cur = OPEN_MAX;
	if (::setrlimit(RLIMIT_NOFILE, &rlim) != 0)
	{
		std::cerr << "Failed to set max open file limit (" << OPEN_MAX << ").\n";
	}
#endif

#ifndef _WIN32
	// Write file limits
	sys_log.notice("Maximum open file descriptors: %i", utils::get_maxfiles());
#endif

	if (utils::get_low_power_mode())
	{
		sys_log.error("Low Power Mode is enabled, performance may be reduced.");
	}

	std::lock_guard qt_init(s_qt_init);

	// The constructor of QApplication eats the --style and --stylesheet arguments.
	// By checking for stylesheet().isEmpty() we could implicitly know if a stylesheet was passed,
	// but I haven't found an implicit way to check for style yet, so we naively check them both here for now.
	const bool use_cli_style = find_arg(arg_style, argc, argv) != -1 || find_arg(arg_stylesheet, argc, argv) != -1;

	QScopedPointer<QCoreApplication> app(create_application(argc, argv));
	app->setApplicationVersion(QString::fromStdString(rpcs3::get_version().to_string()));
	app->setApplicationName("RPCS3");
	app->setOrganizationName("RPCS3");

	// Command line args
	static QCommandLineParser parser;
	parser.setApplicationDescription("Welcome to RPCS3 command line.");
	parser.addPositionalArgument("(S)ELF", "Path for directly executing a (S)ELF");
	parser.addPositionalArgument("[Args...]", "Optional args for the executable");

	const QCommandLineOption help_option    = parser.addHelpOption();
	const QCommandLineOption version_option = parser.addVersionOption();
	parser.addOption(QCommandLineOption(arg_headless, "Run RPCS3 in headless mode."));
	parser.addOption(QCommandLineOption(arg_no_gui, "Run RPCS3 without its GUI."));
	parser.addOption(QCommandLineOption(arg_high_dpi, "Enables Qt High Dpi Scaling.", "enabled", "1"));
	parser.addOption(QCommandLineOption(arg_rounding, "Sets the Qt::HighDpiScaleFactorRoundingPolicy for values like 150% zoom.", "rounding", "4"));
	parser.addOption(QCommandLineOption(arg_styles, "Lists the available styles."));
	parser.addOption(QCommandLineOption(arg_style, "Loads a custom style.", "style", ""));
	parser.addOption(QCommandLineOption(arg_stylesheet, "Loads a custom stylesheet.", "path", ""));
	const QCommandLineOption config_option(arg_config, "Forces the emulator to use this configuration file for CLI-booted game.", "path", "");
	parser.addOption(config_option);
	const QCommandLineOption installfw_option(arg_installfw, "Forces the emulator to install this firmware file.", "path", "");
	parser.addOption(installfw_option);
	const QCommandLineOption installpkg_option(arg_installpkg, "Forces the emulator to install this pkg file.", "path", "");
	parser.addOption(installpkg_option);
	const QCommandLineOption decrypt_option(arg_decrypt, "Decrypt PS3 binaries.", "path(s)", "");
	parser.addOption(decrypt_option);
	const QCommandLineOption user_id_option(arg_user_id, "Start RPCS3 as this user.", "user id", "");
	parser.addOption(user_id_option);
	parser.addOption(QCommandLineOption(arg_q_debug, "Log qDebug to RPCS3.log."));
	parser.addOption(QCommandLineOption(arg_error, "For internal usage."));
	parser.addOption(QCommandLineOption(arg_updating, "For internal usage."));
	parser.addOption(QCommandLineOption(arg_commit_db, "Update commits.lst cache. Optional arguments: <path> <sha>"));
	parser.addOption(QCommandLineOption(arg_timer, "Enable high resolution timer for better performance (windows)", "enabled", "1"));
	parser.addOption(QCommandLineOption(arg_verbose_curl, "Enable verbose curl logging."));
	parser.addOption(QCommandLineOption(arg_any_location, "Allow RPCS3 to be run from any location. Dangerous"));
	parser.process(app->arguments());

	// Don't start up the full rpcs3 gui if we just want the version or help.
	if (parser.isSet(version_option) || parser.isSet(help_option))
		return 0;

	// Set curl to verbose if needed
	rpcs3::curl::g_curl_verbose = parser.isSet(arg_verbose_curl);

	if (rpcs3::curl::g_curl_verbose)
	{
#ifdef _WIN32
		if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
		{
			[[maybe_unused]] const auto con_out = freopen("CONOUT$", "w", stdout);
			[[maybe_unused]] const auto con_err = freopen("CONOUT$", "w", stderr);
		}
#endif
		fprintf(stdout, "Enabled Curl verbose logging.\n");
		sys_log.always()("Enabled Curl verbose logging. Please look at your console output.");
	}

	// Handle update of commit database
	if (parser.isSet(arg_commit_db))
	{
#ifdef _WIN32
		if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
		{
			[[maybe_unused]] const auto con_out = freopen("CONOUT$", "w", stdout);
			[[maybe_unused]] const auto con_err = freopen("CONOUT$", "w", stderr);
		}

		std::string path;
#else
		std::string path = "bin/git/commits.lst";
#endif
		std::string from_sha;

		if (const int i_arg_commit_db = find_arg(arg_commit_db, argc, argv); i_arg_commit_db != -1)
		{
			if (int i = i_arg_commit_db + 1; argc > i)
			{
				path = argv[i++];

				if (argc > i)
				{
					from_sha = argv[i];
				}
			}
#ifdef _WIN32
			else
			{
				fprintf(stderr, "Missing path argument.\n");
				return 1;
			}
#endif
		}
		else
		{
			fprintf(stderr, "Can not find argument --%s\n", arg_commit_db);
			return 1;
		}

		fs::file file(path, fs::read + fs::write + fs::append + fs::create);

		if (!file)
		{
			fprintf(stderr, "Failed to open file: '%s' (errno=%d)\n", path.c_str(), errno);
			return 1;
		}

		fprintf(stdout, "\nAppending commits to '%s' ...\n", path.c_str());

		// Get existing list
		std::string data = file.to_string();
		std::vector<std::string> list = fmt::split(data, {"\n"});

		const bool was_empty = data.empty();

		// SHA to start
		std::string last;

		if (!list.empty())
		{
			// Decode last entry to check last written commit
			QByteArray buf(list.back().c_str(), list.back().size());
			QJsonDocument doc = QJsonDocument::fromJson(buf);

			if (doc.isObject() && doc["sha"].isString())
			{
				last = doc["sha"].toString().toStdString();
			}
		}

		list.clear();

		// JSON buffer
		QByteArray buf;

		// CURL handle to work with GitHub API
		rpcs3::curl::curl_handle curl;

		struct curl_slist* hhdr{};
		hhdr = curl_slist_append(hhdr, "Accept: application/vnd.github.v3+json");
		hhdr = curl_slist_append(hhdr, "User-Agent: curl/7.37.0");

		CURLcode err = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hhdr);
		if (err != CURLE_OK) fprintf(stderr, "curl_easy_setopt(CURLOPT_HTTPHEADER) error: %s", curl_easy_strerror(err));

		err = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](const char* ptr, usz, usz size, void* json) -> usz
		{
			static_cast<QByteArray*>(json)->append(ptr, size);
			return size;
		});
		if (err != CURLE_OK) fprintf(stderr, "curl_easy_setopt(CURLOPT_WRITEFUNCTION) error: %s", curl_easy_strerror(err));

		err = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
		if (err != CURLE_OK) fprintf(stderr, "curl_easy_setopt(CURLOPT_WRITEDATA) error: %s", curl_easy_strerror(err));

		u32 page = 1;
		constexpr u32 per_page = 100;

		while (page <= 55)
		{
			fprintf(stdout, "Fetching page %d ...\n", page);

			std::string url = "https://api.github.com/repos/RPCS3/rpcs3/commits?per_page=";
			fmt::append(url, "%u&page=%u", per_page, page++);
			if (!from_sha.empty())
				fmt::append(url, "&sha=%s", from_sha);

			err = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			if (err != CURLE_OK)
			{
				fprintf(stderr, "curl_easy_setopt(CURLOPT_URL, %s) error: %s", url.c_str(), curl_easy_strerror(err));
				break;
			}

			// Reset error buffer before we call curl_easy_perform
			curl.reset_error_buffer();

			err = curl_easy_perform(curl);
			if (err != CURLE_OK)
			{
				const std::string error_string = curl.get_verbose_error(err);
				fprintf(stderr, "curl_easy_perform(): %s", error_string.c_str());
				break;
			}

			QJsonDocument info = QJsonDocument::fromJson(buf);

			if (!info.isArray()) [[unlikely]]
			{
				fprintf(stderr, "Bad response:\n%s", buf.data());
				break;
			}

			u32 count = 0;

			for (auto&& ref : info.array())
			{
				if (!ref.isObject())
				{
					page = -1;
					break;
				}

				count++;

				QJsonObject result, author, committer;
				QJsonObject commit = ref.toObject();

				auto commit_ = commit["commit"].toObject();
				auto author_ = commit_["author"].toObject();
				auto committer_ = commit_["committer"].toObject();
				auto _author = commit["author"].toObject();
				auto _committer = commit["committer"].toObject();

				result["sha"] = commit["sha"];
				result["msg"] = commit_["message"];

				author["name"] = author_["name"];
				author["date"] = author_["date"];
				author["email"] = author_["email"];
				author["login"] = _author["login"];
				author["avatar"] = _author["avatar_url"];

				committer["name"] = committer_["name"];
				committer["date"] = committer_["date"];
				committer["email"] = committer_["email"];
				committer["login"] = _committer["login"];
				committer["avatar"] = _committer["avatar_url"];

				result["author"] = author;
				result["committer"] = committer;

				QJsonDocument out(result);
				buf = out.toJson(QJsonDocument::JsonFormat::Compact);
				buf += "\n";

				if (was_empty || !from_sha.empty())
				{
					data = buf.toStdString() + std::move(data);
				}
				else if (commit["sha"].toString().toStdString() == last)
				{
					page = -1;
					break;
				}
				else
				{
					// Append to the list
					list.emplace_back(buf.data(), buf.size());
				}
			}

			buf.clear();

			if (count < per_page)
			{
				break;
			}
		}

		if (was_empty || !from_sha.empty())
		{
			file.trunc(0);
			file.write(data);
		}
		else
		{
			// Append list in reverse order
			for (usz i = list.size() - 1; ~i; --i)
			{
				file.write(list[i]);
			}
		}

		curl_slist_free_all(hhdr);

		fprintf(stdout, "Finished fetching commits: %s\n", path.c_str());
		return 0;
	}

	if (parser.isSet(arg_q_debug))
	{
		qInstallMessageHandler(log_q_debug);
	}

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

	std::string active_user;

	if (parser.isSet(arg_user_id))
	{
		active_user = parser.value(arg_user_id).toStdString();

		if (rpcs3::utils::check_user(active_user) == 0)
		{
			report_fatal_error(fmt::format("Failed to set user ID '%s'.\nThe user ID must consist of 8 digits and cannot be 00000000.", active_user));
		}
	}

	s_no_gui = parser.isSet(arg_no_gui);

	if (auto gui_app = qobject_cast<gui_application*>(app.data()))
	{
		gui_app->setAttribute(Qt::AA_UseHighDpiPixmaps);
		gui_app->setAttribute(Qt::AA_DisableWindowContextHelpButton);
		gui_app->setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

		gui_app->SetShowGui(!s_no_gui);
		gui_app->SetUseCliStyle(use_cli_style);
		gui_app->SetWithCliBoot(parser.isSet(arg_installfw) || parser.isSet(arg_installpkg) || !parser.positionalArguments().isEmpty());
		gui_app->SetActiveUser(active_user);

		if (!gui_app->Init())
		{
			Emu.Quit(true);
			return 0;
		}
	}
	else if (auto headless_app = qobject_cast<headless_application*>(app.data()))
	{
		s_headless = true;

		headless_app->SetActiveUser(active_user);

		if (!headless_app->Init())
		{
			Emu.Quit(true);
			return 0;
		}
	}
	else
	{
		// Should be unreachable
		report_fatal_error("RPCS3 initialization failed!");
	}

	// Check if the current location is actually useable
	if (!parser.isSet(arg_any_location))
	{
		const std::string emu_dir = rpcs3::utils::get_emu_dir();

		// Check temporary directories
		for (const QString& path : QStandardPaths::standardLocations(QStandardPaths::StandardLocation::TempLocation))
		{
			if (Emu.IsPathInsideDir(emu_dir, path.toStdString()))
			{
				report_fatal_error(fmt::format(
					"RPCS3 should never be run from a temporary location!\n"
					"Please install RPCS3 in a persistent location.\n"
					"Current location:\n%s", emu_dir));
				return 1;
			}
		}
	}

#ifdef _WIN32
	// Create dummy permanent low resolution timer to workaround messing with system timer resolution
	QTimer* dummy_timer = new QTimer(app.data());
	dummy_timer->start(13);

	ULONG min_res, max_res, orig_res;
	bool got_timer_resolution = NtQueryTimerResolution(&min_res, &max_res, &orig_res) == 0;

	// Set 0.5 msec timer resolution for best performance
	// - As QT5 timers (QTimer) sets the timer resolution to 1 msec, override it here.
	if (parser.value(arg_timer).toStdString() == "1")
	{
		ULONG new_res;
		if (got_timer_resolution && NtSetTimerResolution(max_res, TRUE, &new_res) == 0)
		{
			NtSetTimerResolution(max_res, TRUE, &new_res);
			sys_log.notice("New timer resolution: %d us (old=%d us, min=%d us, max=%d us)", new_res / 10, orig_res / 10, min_res / 10, max_res / 10);
			got_timer_resolution = false; // Invalidate for log message later
		}
		else
		{
			sys_log.error("Failed to set timer resolution!");
		}
	}
	else
	{
		sys_log.warning("High resolution timer disabled!");
	}

	if (got_timer_resolution)
	{
		sys_log.notice("Timer resolution: %d us (min=%d us, max=%d us)", orig_res / 10, min_res / 10, max_res / 10);
	}
#endif

	if (parser.isSet(arg_decrypt))
	{
#ifdef _WIN32
		if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())
		{
			[[maybe_unused]] const auto con_out = freopen("CONOUT$", "w", stdout);
			[[maybe_unused]] const auto con_in = freopen("CONIN$", "r", stdin);
		}
#endif

		std::vector<std::string> vec_modules;
		for (const QString& mod : parser.values(decrypt_option))
		{
			const QFileInfo fi(mod);
			if (!fi.exists())
			{
				std::cout << "File not found: " << mod.toStdString() << std::endl;
				return 1;
			}
			if (!fi.isFile())
			{
				std::cout << "Not a file: " << mod.toStdString() << std::endl;
				return 1;
			}
			vec_modules.push_back(fi.absoluteFilePath().toStdString());
		}

		const auto input_cb = [](std::string old_path, std::string path, bool tried) -> std::string
		{
			const std::string hint = fmt::format("Hint: KLIC (KLicense key) is a 16-byte long string. (32 hexadecimal characters)"
				"\nAnd is logged with some sceNpDrm* functions when the game/application which owns \"%0\" is running.", path);

			if (tried)
			{
				std::cout << "Failed to decrypt " << old_path << " with specfied KLIC, retrying.\n" << hint << std::endl;
			}

			std::cout << "Enter KLIC of " << path << "\nHexadecimal only, 32 characters:" << std::endl;

			std::string input;
			std::cin >> input;

			return input;
		};

		Emu.Init();
		decrypt_sprx_libraries(vec_modules, input_cb);
		Emu.Quit(true);
		return 0;
	}

	// Force install firmware or pkg first if specified through command-line
	if (parser.isSet(arg_installfw) || parser.isSet(arg_installpkg))
	{
		if (auto gui_app = qobject_cast<gui_application*>(app.data()))
		{
			if (s_no_gui)
			{
				report_fatal_error("Cannot perform installation in no-gui mode!");
			}

			if (gui_app->m_main_window)
			{
				if (parser.isSet(arg_installfw) && parser.isSet(arg_installpkg))
				{
					QMessageBox::warning(gui_app->m_main_window, QObject::tr("Invalid command-line arguments!"), QObject::tr("Cannot perform multiple installations at the same time!"));
				}
				else if (parser.isSet(arg_installfw))
				{
					gui_app->m_main_window->InstallPup(parser.value(installfw_option));
				}
				else
				{
					gui_app->m_main_window->InstallPackages({parser.value(installpkg_option)});
				}
			}
			else
			{
				report_fatal_error("Cannot perform installation. No main window found!");
			}
		}
		else
		{
			report_fatal_error("Cannot perform installation in headless mode!");
		}
	}

	for (const auto& opt : parser.optionNames())
	{
		sys_log.notice("Option passed via command line: %s %s", opt.toStdString(), parser.value(opt).toStdString());
	}

	if (const QStringList args = parser.positionalArguments(); !args.isEmpty() && !is_updating && !parser.isSet(arg_installfw) && !parser.isSet(arg_installpkg))
	{
		sys_log.notice("Booting application from command line: %s", args.at(0).toStdString());

		// Propagate command line arguments
		std::vector<std::string> rpcs3_argv;

		if (args.length() > 1)
		{
			rpcs3_argv.emplace_back();

			for (int i = 1; i < args.length(); i++)
			{
				const std::string arg = args[i].toStdString();
				rpcs3_argv.emplace_back(arg);
				sys_log.notice("Optional command line argument %d: %s", i, arg);
			}
		}

		std::string config_path;

		if (parser.isSet(arg_config))
		{
			config_path = parser.value(config_option).toStdString();

			if (!fs::is_file(config_path))
			{
				report_fatal_error(fmt::format("No config file found: %s", config_path));
			}
		}

		// Postpone startup to main event loop
		Emu.CallFromMainThread([path = sstr(QFileInfo(args.at(0)).absoluteFilePath()), rpcs3_argv = std::move(rpcs3_argv), config_path = std::move(config_path)]() mutable
		{
			Emu.argv = std::move(rpcs3_argv);
			Emu.SetForceBoot(true);

			const cfg_mode config_mode = config_path.empty() ? cfg_mode::custom : cfg_mode::config_override;

			if (const game_boot_result error = Emu.BootGame(path, "", false, false, config_mode, config_path); error != game_boot_result::no_errors)
			{
				sys_log.error("Booting '%s' with cli argument failed: reason: %s", path, error);

				if (s_headless || s_no_gui)
				{
					report_fatal_error(fmt::format("Booting '%s' failed!\n\nReason: %s", path, error));
				}
			}
		});
	}
	else if (s_headless || s_no_gui)
	{
#ifdef _WIN32
		// If launched from CMD
		if (AttachConsole(ATTACH_PARENT_PROCESS))
			[[maybe_unused]] const auto con_out = freopen("CONOUT$", "w", stderr);
#endif
		sys_log.error("Cannot run %s mode without boot target. Terminating...", s_headless ? "headless" : "no-gui");
		fprintf(stderr, "Cannot run %s mode without boot target. Terminating...\n", s_headless ? "headless" : "no-gui");

		if (s_no_gui)
		{
			QMessageBox::warning(nullptr, QObject::tr("Missing command-line arguments!"), QObject::tr("Cannot run no-gui mode without boot target.\nTerminating..."));
		}

		Emu.Quit(true);
		return 0;
	}

	// run event loop (maybe only needed for the gui application)
	return app->exec();
}

// Temporarily, this is code from std for prebuilt LLVM. I don't understand why this is necessary.
// From the same MSVC 19.27.29112.0, LLVM libs depend on these, but RPCS3 gets linker errors.
#ifdef _WIN32
extern "C"
{
	int __stdcall __std_init_once_begin_initialize(void** ppinit, ulong f, int* fp, void** lpc) noexcept
	{
		return InitOnceBeginInitialize(reinterpret_cast<LPINIT_ONCE>(ppinit), f, fp, lpc);
	}

	int __stdcall __std_init_once_complete(void** ppinit, ulong f, void* lpc) noexcept
	{
		return InitOnceComplete(reinterpret_cast<LPINIT_ONCE>(ppinit), f, lpc);
	}

	usz __stdcall __std_get_string_size_without_trailing_whitespace(const char* str, usz size) noexcept
	{
		while (size)
		{
			switch (str[size - 1])
			{
			case 0:
			case ' ':
			case '\n':
			case '\r':
			case '\t':
			{
				size--;
				continue;
			}
			default: break;
			}

			break;
		}

		return size;
	}

	usz __stdcall __std_system_error_allocate_message(const unsigned long msg_id, char** ptr_str) noexcept
	{
		return __std_get_string_size_without_trailing_whitespace(*ptr_str, FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, msg_id, 0, reinterpret_cast<char*>(ptr_str), 0, nullptr));
	}

	void __stdcall __std_system_error_deallocate_message(char* s) noexcept
	{
		LocalFree(s);
	}
}
#endif
