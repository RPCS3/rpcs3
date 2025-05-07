// Qt6 frontend implementation for rpcs3. Known to work on Windows, Linux, Mac
// by Sacha Refshauge, Megamouse and flash-fire

#include <iostream>
#include <chrono>
#include <clocale>
#include <span>

#include <QApplication>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QTimer>
#include <QObject>
#include <QStyleFactory>
#include <QMessageBox>
#include <QMetaEnum>
#include <QStandardPaths>

#include "rpcs3qt/gui_application.h"
#include "rpcs3qt/fatal_error_dialog.h"
#include "rpcs3qt/curl_handle.h"
#include "rpcs3qt/main_window.h"
#include "rpcs3qt/uuid.h"

#include "headless_application.h"
#include "Utilities/sema.h"
#include "Utilities/date_time.h"
#include "util/console.h"
#include "Crypto/decrypt_binaries.h"
#ifdef _WIN32
#include "module_verifier.hpp"
#include "util/dyn_lib.hpp"
#include <shellapi.h>

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
#include <sys/prctl.h>
#endif

#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#endif

#include "Utilities/Config.h"
#include "Utilities/Thread.h"
#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "util/media_utils.h"
#include "rpcs3_version.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include <thread>
#include <charconv>

#include "util/sysinfo.hpp"

// Let's initialize the locale first
static const bool s_init_locale = []()
{
	std::setlocale(LC_ALL, "C");
	return true;
}();

static semaphore<> s_qt_init;

static atomic_t<bool> s_headless = false;
static atomic_t<bool> s_no_gui = false;
static atomic_t<char*> s_argv0 = nullptr;
static bool s_is_error_launch = false;

std::string g_input_config_override;

extern thread_local std::string(*g_tls_log_prefix)();
extern thread_local std::string_view g_tls_serialize_name;

#ifndef _WIN32
extern char **environ;
#endif

LOG_CHANNEL(sys_log, "SYS");
LOG_CHANNEL(q_debug, "QDEBUG");

#ifdef _WIN32
std::set<std::string> get_one_drive_paths()
{
	std::set<std::string> paths;

	// NOTE: Disabled. The environment variables can lead to false positives.
	//for (const char* key : { "OneDrive", "OneDriveConsumer", "OneDriveCommercial" })
	//{
	//	if (const char* env_path = std::getenv(key))
	//	{
	//		sys_log.notice("get_one_drive_paths: Found OneDrive env path: '%s' (key='%s')", env_path, key);
	//		paths.insert(env_path);
	//	}
	//}

	for (const wchar_t* key : { L"Software\\Microsoft\\OneDrive\\Accounts\\Personal" })
	{
		HKEY hkey = NULL;
		LSTATUS status = RegOpenKeyW(HKEY_CURRENT_USER, key, &hkey);
		if (status != ERROR_SUCCESS)
		{
			sys_log.trace("get_one_drive_paths: RegOpenKeyW failed: %s (key='%s')", fmt::win_error{static_cast<unsigned long>(status), nullptr}, wchar_to_utf8(key));
			continue;
		}

		std::wstring path_buffer;
		DWORD type = 0U;

		do
		{
			path_buffer.resize(path_buffer.size() + MAX_PATH);
			DWORD buffer_size = static_cast<DWORD>(path_buffer.size() - 1);
			status = RegQueryValueExW(hkey, L"UserFolder", NULL, &type, reinterpret_cast<LPBYTE>(path_buffer.data()), &buffer_size);
		}
		while (status == ERROR_MORE_DATA);

		const LSTATUS close_status = RegCloseKey(hkey);
		if (close_status != ERROR_SUCCESS)
		{
			sys_log.error("get_one_drive_paths: RegCloseKey failed: %s", fmt::win_error{static_cast<unsigned long>(close_status), nullptr});
		}

		if (status != ERROR_SUCCESS)
		{
			sys_log.trace("get_one_drive_paths: RegQueryValueExW failed: %s", fmt::win_error{static_cast<unsigned long>(status), nullptr});
			continue;
		}

		if ((type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ))
		{
			const std::string path = wchar_to_utf8(path_buffer.data());
			sys_log.notice("get_one_drive_paths: Found OneDrive registry path: '%s' (key='%s')", path, wchar_to_utf8(key));
			paths.insert(path);
		}
	}

	return paths;
}
#endif

[[noreturn]] extern void report_fatal_error(std::string_view _text, bool is_html = false, bool include_help_text = true)
{
#ifdef __linux__
	extern void jit_announce(uptr, usz, std::string_view);
#endif
	std::string buf;

	if (!s_is_error_launch)
	{
		buf = std::string(_text);

		// Check if thread id is in string
		if (_text.find("\nThread id = "sv) == umax && !thread_ctrl::is_main())
		{
			// Append thread id if it isn't already, except on main thread
			fmt::append(buf, "\n\nThread id = %u.", thread_ctrl::get_tid());
		}

		if (!g_tls_serialize_name.empty())
		{
			fmt::append(buf, "\nSerialized Object: %s", g_tls_serialize_name);
		}

		const system_state state = Emu.GetStatus(false);

		if (state == system_state::stopped)
		{
			fmt::append(buf, "\nEmulation is stopped");
		}
		else
		{
			const std::string& name = Emu.GetTitleAndTitleID();
			fmt::append(buf, "\nTitle: \"%s\" (emulation is %s)", name.empty() ? "N/A" : name.data(), state == system_state::stopping ? "stopping" : "running");
		}

		fmt::append(buf, "\nBuild: \"%s\"", rpcs3::get_verbose_version());
		fmt::append(buf, "\nDate: \"%s\"", std::chrono::system_clock::now());
	}

	std::string_view text = s_is_error_launch ? _text : buf;

	if (s_headless)
	{
		utils::attach_console(utils::console_stream::std_err, true);

		utils::output_stderr(fmt::format("RPCS3: %s\n", text));
#ifdef __linux__
		jit_announce(0, 0, "");
#endif
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
		utils::output_stderr(fmt::format("RPCS3: %s\n", text));
	}

	static auto show_report = [is_html, include_help_text](std::string_view text)
	{
		fatal_error_dialog dlg(text, is_html, include_help_text);
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
		constexpr DWORD size = 32767;
		std::vector<wchar_t> buffer(size);
		GetModuleFileNameW(nullptr, buffer.data(), size);
		const std::wstring arg(text.cbegin(), text.cend()); // ignore unicode for now
		_wspawnl(_P_WAIT, buffer.data(), buffer.data(), L"--error", arg.c_str(), nullptr);
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

#ifdef __linux__
	jit_announce(0, 0, "");
#endif
	std::abort();
}

struct fatal_error_listener final : logs::listener
{
public:
	~fatal_error_listener() override = default;

	void log(u64 /*stamp*/, const logs::message& msg, std::string_view prefix, std::string_view text) override
	{
		if (msg == logs::level::fatal || (msg == logs::level::always && m_log_always))
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

			// If launched from CMD
			utils::attach_console(msg == logs::level::fatal ? utils::console_stream::std_err : utils::console_stream::std_out, false);

			// Output to error stream as is
			if (msg == logs::level::fatal)
			{
				utils::output_stderr(_msg);
			}
			else
			{
				std::cout << _msg;
			}

#ifdef _WIN32
			if (IsDebuggerPresent())
			{
				// Output string to attached debugger
				OutputDebugStringA(_msg.c_str());
			}
#endif
			if (msg == logs::level::fatal)
			{
				// Pause emulation if fatal error encountered
				Emu.Pause(true);
			}
		}
	}

	void log_always(bool enabled)
	{
		m_log_always = enabled;
	}

private:
	bool m_log_always = false;
};

// Arguments that force a headless application (need to be checked in create_application)
constexpr auto arg_headless     = "headless";
constexpr auto arg_decrypt      = "decrypt";

// Arguments that can be used with a gui application
constexpr auto arg_no_gui       = "no-gui";
constexpr auto arg_fullscreen   = "fullscreen"; // only useful with no-gui
constexpr auto arg_gs_screen    = "game-screen";
constexpr auto arg_high_dpi     = "hidpi";
constexpr auto arg_rounding     = "dpi-rounding";
constexpr auto arg_styles       = "styles";
constexpr auto arg_style        = "style";
constexpr auto arg_stylesheet   = "stylesheet";
constexpr auto arg_config       = "config";
constexpr auto arg_input_config = "input-config"; // only useful with no-gui
constexpr auto arg_q_debug      = "qDebug";
constexpr auto arg_error        = "error";
constexpr auto arg_updating     = "updating";
constexpr auto arg_user_id      = "user-id";
constexpr auto arg_installfw    = "installfw";
constexpr auto arg_installpkg   = "installpkg";
constexpr auto arg_savestate    = "savestate";
constexpr auto arg_rsx_capture  = "rsx-capture";
constexpr auto arg_timer        = "high-res-timer";
constexpr auto arg_verbose_curl = "verbose-curl";
constexpr auto arg_any_location = "allow-any-location";
constexpr auto arg_codecs       = "codecs";

#ifdef _WIN32
constexpr auto arg_stdout       = "stdout";
constexpr auto arg_stderr       = "stderr";
#endif

constexpr auto arg_emulation_barrier = "";

int find_arg(const char* to_search, std::span<char* const> argv)
{
	// It's not guaranteed that argv 0 is the executable.
	for (usz i = 0; i != argv.size(); i++)
	{
		const auto argp = argv[i];

		if (argp[0] == '-' && argp[1] == '-' && !std::strcmp(to_search, argp + 2))
		{
			return ::narrow<int>(i);
		}
	}

	return -1;
}

QCoreApplication* create_application(std::span<char* const> qt_argv)
{
	static int s_argc = static_cast<int>(qt_argv.size());
	static char** const s_argv = const_cast<char**>(qt_argv.data());

	if (find_arg(arg_headless, qt_argv) != -1 ||
		find_arg(arg_decrypt, qt_argv) != -1)
	{
		return new headless_application(s_argc, s_argv);
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

	const int i_hdpi = find_arg(arg_high_dpi, qt_argv);
	if (i_hdpi != -1)
	{
		const std::string cmp_str = "0";
		const auto i_hdpi_2 = (s_argc > (i_hdpi + 1)) ? (i_hdpi + 1) : 0;
		const auto high_dpi_setting = (i_hdpi_2 && !std::strcmp(cmp_str.c_str(), qt_argv[i_hdpi_2])) ? "0" : "1";

		// Set QT_ENABLE_HIGHDPI_SCALING from environment. Defaults to cli argument, which defaults to 1.
		use_high_dpi = "1" == qEnvironmentVariable("QT_ENABLE_HIGHDPI_SCALING", high_dpi_setting);
	}

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

		if (const int i_rounding = find_arg(arg_rounding, qt_argv); i_rounding != -1)
		{
			if (const int i_rounding_2 = i_rounding + 1; s_argc > i_rounding_2)
			{
				if (const auto arg_val = qt_argv[i_rounding_2]; !check_dpi_rounding_arg(arg_val))
				{
					const std::string msg = fmt::format("The command line value %s for %s is not allowed. Please use a valid value for Qt::HighDpiScaleFactorRoundingPolicy.", arg_val, arg_rounding);
					sys_log.error("%s", msg); // Don't exit with fatal error. The resulting dialog might be unreadable with dpi problems.
					utils::output_stderr(msg, true);
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

	return new gui_application(s_argc, s_argv);
}

template <>
void fmt_class_string<QString>::format(std::string& out, u64 arg)
{
 	out += get_object(arg).toStdString();
}

void log_q_debug(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
	Q_UNUSED(context)

	switch (type)
	{
	case QtDebugMsg: q_debug.trace("%s", msg); break;
	case QtInfoMsg: q_debug.notice("%s", msg); break;
	case QtWarningMsg: q_debug.warning("%s", msg); break;
	case QtCriticalMsg: q_debug.error("%s", msg); break;
	case QtFatalMsg: q_debug.fatal("%s", msg); break;
	}
}

void run_platform_sanity_checks()
{
#ifdef _WIN32
	// Check if we loaded modules correctly
	WIN32_module_verifier::run();
#endif
}

int run_rpcs3(int argc, char** argv)
{
#ifdef _WIN32
	ULONG64 intro_cycles{};
	QueryThreadCycleTime(GetCurrentThread(), &intro_cycles);
#elif defined(RUSAGE_THREAD)
	struct ::rusage intro_stats{};
	::getrusage(RUSAGE_THREAD, &intro_stats);
	const u64 intro_time = (intro_stats.ru_utime.tv_sec + intro_stats.ru_stime.tv_sec) * 1000000000ull + (intro_stats.ru_utime.tv_usec + intro_stats.ru_stime.tv_usec) * 1000ull;
#endif

#ifdef __linux__
	// Set timerslack value for Linux. The default value is 50,000ns. Change this to just 1 since we value precise timers.
	prctl(PR_SET_TIMERSLACK, 1, 0, 0, 0);
#endif

	s_argv0 = argv[0]; // Save for report_fatal_error

	std::span<char* const> argv_span{ argv, argc + 0u };
	std::span<char* const> emu_argv;
	std::span<char* const> qt_argv;

	if (int emu_pos = find_arg(arg_emulation_barrier, argv_span); emu_pos != -1)
	{
		emu_argv = argv_span.subspan(emu_pos + 1);
		qt_argv = argv_span.subspan(0, emu_pos);
	}
	else
	{
		qt_argv = argv_span;
	}

	// Only run RPCS3 to display an error
	if (int err_pos = find_arg(arg_error, qt_argv); err_pos != -1)
	{
		// Reconstruction of the error from multiple args
		std::string error;
		for (int i = err_pos + 1; i < argc; i++)
		{
			if (i > err_pos + 1)
				error += ' ';
			error += argv[i];
		}

		s_is_error_launch = true;
		report_fatal_error(error);
	}

	const std::string lock_name = fs::get_cache_dir() + "RPCS3.buf";
	const std::string log_name = fs::get_log_dir() + "RPCS3.log";

	static fs::file instance_lock;

	// True if an argument --updating found
	const bool is_updating = find_arg(arg_updating, qt_argv) != -1;

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
				report_fatal_error(fmt::format("Another instance of RPCS3 is running.\nClose it or kill its process, if necessary.\n'%s' still exists.", lock_name));
			}

			report_fatal_error(fmt::format("Cannot create '%s' or '%s' (access denied).\n"
#ifdef _WIN32
				"Note that RPCS3 cannot be installed in Program Files or similar directories with limited permissions."
#else
				"Please, check RPCS3 permissions."
#endif
				, log_name, lock_name));
		}

		report_fatal_error(fmt::format("Cannot create'%s' or '%s' (error=%s)", log_name, lock_name, fs::g_tls_error));
	}

#ifdef _WIN32
	if (!SetProcessWorkingSetSize(GetCurrentProcess(), 0x80000000, 0xC0000000)) // 2-3 GiB
	{
		report_fatal_error("Not enough memory for RPCS3 process.");
	}

	WSADATA wsa_data{};
	if (const int res = WSAStartup(MAKEWORD(2, 2), &wsa_data); res != 0)
	{
		report_fatal_error(fmt::format("WSAStartup failed (error=%s)", fmt::win_error{static_cast<unsigned long>(res), nullptr}));
	}
#endif

#if defined(__APPLE__) && defined(__x86_64__)
	if (const utils::OS_version os = utils::get_OS_version();
		os.version_major == 14 && os.version_minor < 3 && (utils::get_cpu_brand().rfind("VirtualApple", 0) == 0))
	{
		report_fatal_error(fmt::format("RPCS3 requires macOS 14.3.0 or later.\nYou're currently using macOS %i.%i.%i.\nPlease update macOS from System Settings.\n\n", os.version_major, os.version_minor, os.version_patch));
	}
#endif

	ensure(thread_ctrl::is_main(), "Not main thread");

	// Initialize thread pool finalizer (on first use)
	static_cast<void>(named_thread("", [](int) {}));

	static std::unique_ptr<logs::listener> log_file;
	{
		// Check free space
		fs::device_stat stats{};
		if (!fs::statfs(fs::get_cache_dir(), stats) || stats.avail_free < 128 * 1024 * 1024)
		{
			std::fprintf(stderr, "Not enough free space for logs (%f KB)", stats.avail_free / 1000000.);
		}

		// Limit log size to ~25% of free space
		log_file = logs::make_file_listener(log_name, stats.avail_free / 4);
	}

	static std::unique_ptr<fatal_error_listener> fatal_listener = std::make_unique<fatal_error_listener>();
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
		os.text = utils::get_OS_version_string();

		// Write Qt version
		logs::stored_message qt{(strcmp(QT_VERSION_STR, qVersion()) != 0) ? sys_log.error : sys_log.notice};
		qt.text = fmt::format("Qt version: Compiled against Qt %s | Run-time uses Qt %s", QT_VERSION_STR, qVersion());

		// Write current time
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
		if (i > 0) argument_str += " ";
		argument_str += '\'' + std::string(argv[i]) + '\'';
	}

	sys_log.notice("argc: %d, argv: %s", argc, argument_str);

#ifdef _WIN32
	int n_args = 0;
	if (LPWSTR* arg_list = CommandLineToArgvW(GetCommandLineW(), &n_args))
	{
		std::string utf8_args;
		for (int i = 0; i < n_args; i++)
		{
			if (i > 0) utf8_args += " ";
			utf8_args += '\'' + wchar_to_utf8(arg_list[i]) + '\'';
		}
		LocalFree(arg_list);
		sys_log.notice("argv_utf8: %s", utf8_args);
	}
#endif

	// Before we proceed, run some sanity checks
	run_platform_sanity_checks();

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
	const bool use_cli_style = find_arg(arg_style, qt_argv) != -1 || find_arg(arg_stylesheet, qt_argv) != -1;

	QScopedPointer<QCoreApplication> app(create_application(qt_argv));
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
	parser.addOption(QCommandLineOption(arg_fullscreen, "Run games in fullscreen mode. Only used when no-gui is set."));
	const QCommandLineOption screen_option(arg_gs_screen, "Forces the emulator to use the specified screen for the game window.", "index", "");
	parser.addOption(screen_option);
	parser.addOption(QCommandLineOption(arg_high_dpi, "Enables Qt High Dpi Scaling.", "enabled", "1"));
	parser.addOption(QCommandLineOption(arg_rounding, "Sets the Qt::HighDpiScaleFactorRoundingPolicy for values like 150% zoom.", "rounding", "4"));
	parser.addOption(QCommandLineOption(arg_styles, "Lists the available styles."));
	parser.addOption(QCommandLineOption(arg_style, "Loads a custom style.", "style", ""));
	parser.addOption(QCommandLineOption(arg_stylesheet, "Loads a custom stylesheet.", "path", ""));
	const QCommandLineOption config_option(arg_config, "Forces the emulator to use this configuration file for CLI-booted game.", "path", "");
	parser.addOption(config_option);
	const QCommandLineOption input_config_option(arg_input_config, "Forces the emulator to use this input config file for CLI-booted game.", "name", "");
	parser.addOption(input_config_option);
	const QCommandLineOption installfw_option(arg_installfw, "Forces the emulator to install this firmware file.", "path", "");
	parser.addOption(installfw_option);
	const QCommandLineOption installpkg_option(arg_installpkg, "Forces the emulator to install this pkg file.", "path", "");
	parser.addOption(installpkg_option);
	const QCommandLineOption decrypt_option(arg_decrypt, "Decrypt PS3 binaries.", "path(s)", "");
	parser.addOption(decrypt_option);
	const QCommandLineOption user_id_option(arg_user_id, "Start RPCS3 as this user.", "user id", "");
	parser.addOption(user_id_option);
	const QCommandLineOption savestate_option(arg_savestate, "Path for directly loading a savestate.", "path", "");
	parser.addOption(savestate_option);
	const QCommandLineOption rsx_capture_option(arg_rsx_capture, "Path for directly loading an rsx capture.", "path", "");
	parser.addOption(rsx_capture_option);
	parser.addOption(QCommandLineOption(arg_q_debug, "Log qDebug to RPCS3.log."));
	parser.addOption(QCommandLineOption(arg_error, "For internal usage."));
	parser.addOption(QCommandLineOption(arg_updating, "For internal usage."));
	parser.addOption(QCommandLineOption(arg_timer, "Enable high resolution timer for better performance (windows)", "enabled", "1"));
	parser.addOption(QCommandLineOption(arg_verbose_curl, "Enable verbose curl logging."));
	parser.addOption(QCommandLineOption(arg_any_location, "Allow RPCS3 to be run from any location. Dangerous"));
	const QCommandLineOption codec_option(arg_codecs, "List ffmpeg codecs");
	parser.addOption(codec_option);

#ifdef _WIN32
	parser.addOption(QCommandLineOption(arg_stdout, "Attach the console window and listen to standard output stream. (STDOUT)"));
	parser.addOption(QCommandLineOption(arg_stderr, "Attach the console window and listen to error output stream. (STDERR)"));
#endif

	parser.addOption(QCommandLineOption("N/A", "Arguments after \"--\" are considered emulation arguments."));

	parser.process(app->arguments());

	// Don't start up the full rpcs3 gui if we just want the version or help.
	if (parser.isSet(version_option) || parser.isSet(help_option))
		return 0;

	if (parser.isSet(codec_option))
	{
		utils::attach_console(utils::console_stream::std_out | utils::console_stream::std_err, true);

		for (const utils::ffmpeg_codec& codec : utils::list_ffmpeg_decoders())
		{
			fprintf(stdout, "Found ffmpeg decoder: %s (%d, %s)\n", codec.name.c_str(), codec.codec_id, codec.long_name.c_str());
			sys_log.success("Found ffmpeg decoder: %s (%d, %s)", codec.name, codec.codec_id, codec.long_name);
		}
		for (const utils::ffmpeg_codec& codec : utils::list_ffmpeg_encoders())
		{
			fprintf(stdout, "Found ffmpeg encoder: %s (%d, %s)\n", codec.name.c_str(), codec.codec_id, codec.long_name.c_str());
			sys_log.success("Found ffmpeg encoder: %s (%d, %s)", codec.name, codec.codec_id, codec.long_name);
		}
		return 0;
	}

#ifdef _WIN32
	if (parser.isSet(arg_stdout) || parser.isSet(arg_stderr))
	{
		int stream = 0;
		if (parser.isSet(arg_stdout))
		{
			stream |= utils::console_stream::std_out;
		}
		if (parser.isSet(arg_stderr))
		{
			stream |= utils::console_stream::std_err;
		}
		utils::attach_console(stream, true);
	}
#endif

	// Set curl to verbose if needed
	rpcs3::curl::g_curl_verbose = parser.isSet(arg_verbose_curl);

	if (rpcs3::curl::g_curl_verbose)
	{
		utils::attach_console(utils::console_stream::std_out | utils::console_stream::std_err, true);
		fprintf(stdout, "Enabled Curl verbose logging.\n");
		sys_log.always()("Enabled Curl verbose logging. Please look at your console output.");
	}

	if (parser.isSet(arg_q_debug))
	{
		qInstallMessageHandler(log_q_debug);
	}

	if (parser.isSet(arg_styles))
	{
		utils::attach_console(utils::console_stream::std_out, true);

		for (const auto& style : QStyleFactory::keys())
			std::cout << "\n" << style.toStdString();

		return 0;
	}

	// Enable console output of "always" log messages.
	// Do this after parsing any Qt cli args that might open a window.
	fatal_listener->log_always(true);

	// Log unique ID
	gui::utils::log_uuid();

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

	if (gui_application* gui_app = qobject_cast<gui_application*>(app.data()))
	{
		gui_app->setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);

		gui_app->SetShowGui(!s_no_gui);
		gui_app->SetUseCliStyle(use_cli_style);
		gui_app->SetWithCliBoot(parser.isSet(arg_installfw) || parser.isSet(arg_installpkg) || !parser.positionalArguments().isEmpty());
		gui_app->SetActiveUser(active_user);

		if (parser.isSet(arg_fullscreen))
		{
			if (!s_no_gui)
			{
				report_fatal_error(fmt::format("The option '%s' can only be used in combination with '%s'.", arg_fullscreen, arg_no_gui));
			}

			gui_app->SetStartGamesFullscreen(true);
		}

		if (parser.isSet(arg_gs_screen))
		{
			const int game_screen_index = parser.value(arg_gs_screen).toInt();

			if (game_screen_index < 0)
			{
				report_fatal_error(fmt::format("The option '%s' can only be used with numbers >= 0 (you used %d)", arg_gs_screen, game_screen_index));
			}

			gui_app->SetGameScreenIndex(game_screen_index);
		}

		if (!gui_app->Init())
		{
			Emu.Quit(true);
			return 0;
		}
	}
	else if (headless_application* headless_app = qobject_cast<headless_application*>(app.data()))
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
				report_fatal_error(QObject::tr(
					"RPCS3 should never be run from a temporary location!\n"
					"Please install RPCS3 in a persistent location.\n"
					"Current location:\n%0").arg(QString::fromStdString(emu_dir)).toStdString());
				return 1;
			}
		}

		// Check nonsensical archive locations
		for (const std::string_view& expr : { "/Rar$"sv })
		{
			if (emu_dir.find(expr) != umax)
			{
				report_fatal_error(QObject::tr(
					"RPCS3 should never be run from an archive!\n"
					"Please install RPCS3 in a persistent location.\n"
					"Current location:\n%0").arg(QString::fromStdString(emu_dir)).toStdString());
				return 1;
			}
		}

#ifdef _WIN32
		// Check OneDrive locations
		for (const std::string& one_drive_path : get_one_drive_paths())
		{
			if (Emu.IsPathInsideDir(emu_dir, one_drive_path))
			{
				report_fatal_error(QObject::tr(
					"RPCS3 should never be run from a OneDrive path!\n"
					"Please move RPCS3 to a location not synced by OneDrive.\n"
					"Current location:\n%0").arg(QString::fromStdString(emu_dir)).toStdString());
				return 1;
			}
		}
#endif
	}

// Set timerslack value for Linux. The default value is 50,000ns. Change this to just 1 since we value precise timers.
#ifdef __linux__
	prctl(PR_SET_TIMERSLACK, 1, 0, 0, 0);
#endif

#ifdef _WIN32
	// Create dummy permanent low resolution timer to workaround messing with system timer resolution
	QTimer* dummy_timer = new QTimer(app.data());
	dummy_timer->start(13);

	ULONG min_res, max_res, orig_res;
	bool got_timer_resolution = NtQueryTimerResolution(&min_res, &max_res, &orig_res) == 0;

	// Set 0.5 msec timer resolution for best performance
	// - As QT timers (QTimer) sets the timer resolution to 1 msec, override it here.
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
		utils::attach_console(utils::console_stream::std_out | utils::console_stream::std_in, true);

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

		Emu.Init();

		std::shared_ptr<decrypt_binaries_t> decrypter = std::make_shared<decrypt_binaries_t>(std::move(vec_modules));

		usz mod_index = decrypter->decrypt();
		usz repeat_count = mod_index == 0 ? 1 : 0;

		while (!decrypter->done())
		{
			const std::string& path = (*decrypter)[mod_index];
			const std::string filename = path.substr(path.find_last_of(fs::delim) + 1);

			const std::string hint = fmt::format("Hint: KLIC (KLicense key) is a 16-byte long string. (32 hexadecimal characters)"
				"\nAnd is logged with some sceNpDrm* functions when the game/application which owns \"%0\" is running.", filename);

			if (repeat_count >= 2)
			{
				std::cout << "Failed to decrypt " << path << " with specfied KLIC, retrying.\n" << hint << std::endl;
			}

			std::cout << "Enter KLIC of " << filename << "\nHexadecimal only, 32 characters:" << std::endl;

			std::string input;
			std::cin >> input;

			if (input.empty())
			{
				break;
			}

			const usz new_index = decrypter->decrypt(input);
			repeat_count = new_index == mod_index ? repeat_count + 1 : 0;
			mod_index = new_index;
		}

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
		sys_log.notice("Option passed via command line: %s %s", opt, parser.value(opt));
	}

	if (parser.isSet(arg_savestate))
	{
		const std::string savestate_path = parser.value(savestate_option).toStdString();
		sys_log.notice("Booting savestate from command line: %s", savestate_path);

		if (!fs::is_file(savestate_path))
		{
			report_fatal_error(fmt::format("No savestate file found: %s", savestate_path));
		}

		Emu.CallFromMainThread([path = savestate_path]()
		{
			Emu.SetForceBoot(true);

			if (const game_boot_result error = Emu.BootGame(path); error != game_boot_result::no_errors)
			{
				sys_log.error("Booting savestate '%s' failed: reason: %s", path, error);

				if (s_headless || s_no_gui)
				{
					report_fatal_error(fmt::format("Booting savestate '%s' failed!\n\nReason: %s", path, error));
				}
			}
		});
	}
	else if (parser.isSet(arg_rsx_capture))
	{
		const std::string rsx_capture_path = parser.value(rsx_capture_option).toStdString();
		sys_log.notice("Booting rsx capture from command line: %s", rsx_capture_path);

		if (!fs::is_file(rsx_capture_path))
		{
			report_fatal_error(fmt::format("No rsx capture file found: %s", rsx_capture_path));
		}

		Emu.CallFromMainThread([path = rsx_capture_path]()
		{
			if (!Emu.BootRsxCapture(path))
			{
				sys_log.error("Booting rsx capture '%s' failed", path);

				if (s_headless || s_no_gui)
				{
					report_fatal_error(fmt::format("Booting rsx capture '%s' failed!", path));
				}
			}
		});
	}
	else if (const QStringList args = parser.positionalArguments(); (!args.isEmpty() || !emu_argv.empty()) && !is_updating && !parser.isSet(arg_installfw) && !parser.isSet(arg_installpkg))
	{
		std::string spath = (args.isEmpty() ? emu_argv[0] : ::at32(args, 0).toStdString());

		if (spath.starts_with(Emulator::vfs_boot_prefix))
		{
			sys_log.notice("Booting application from command line using VFS path: %s", spath.substr(Emulator::vfs_boot_prefix.size()));
		}
		else if (spath.starts_with(Emulator::game_id_boot_prefix))
		{
			sys_log.notice("Booting application from command line using GAMEID: %s", spath.substr(Emulator::game_id_boot_prefix.size()));
		}
		else
		{
			sys_log.notice("Booting application from command line: %s", spath);
		}

		// Propagate command line arguments
		std::vector<std::string> rpcs3_argv;

		if (args.length() > 1)
		{
			// Reserve empty string for executable path
			rpcs3_argv.emplace_back();

			rpcs3_argv.emplace_back();

			for (int i = 1; i != args.length(); i++)
			{
				const std::string arg = args[i].toStdString();
				rpcs3_argv.emplace_back(arg);

				sys_log.error("Optional command line argument %d: %s"
					"\nPlease pass emulation arguments after an empty \"--\" paramater."
					"\nIn the future, the emulator would not support optional arguments without it.", i, arg);
			}
		}

		// Additional arguments passed after "--"
		if (emu_argv.size() > (args.isEmpty() ? 1 : 0))
		{
			// Reserve empty string for executable path
			if (rpcs3_argv.empty())
			{
				rpcs3_argv.emplace_back();
			}

			rpcs3_argv.emplace_back();

			for (usz i = args.isEmpty() ? 1 : 0; i != emu_argv.size(); i++)
			{
				const std::string arg = args[i].toStdString();
				rpcs3_argv.emplace_back(arg);

				sys_log.success("Optional command line argument %d: %s", i, arg);
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

		if (parser.isSet(arg_input_config))
		{
			if (!s_no_gui)
			{
				report_fatal_error(fmt::format("The option '%s' can only be used in combination with '%s'.", arg_input_config, arg_no_gui));
			}

			g_input_config_override = parser.value(input_config_option).toStdString();

			if (g_input_config_override.empty())
			{
				report_fatal_error(fmt::format("Input config file name is empty"));
			}
		}

		if (!spath.starts_with("%RPCS3_"))
		{
			spath = QFileInfo(::at32(args, 0)).absoluteFilePath().toStdString();
		}

		// Postpone startup to main event loop
		Emu.CallFromMainThread([path = std::move(spath), rpcs3_argv = std::move(rpcs3_argv), config_path = std::move(config_path)]() mutable
		{
			Emu.argv = std::move(rpcs3_argv);
			Emu.SetForceBoot(true);

			const cfg_mode config_mode = config_path.empty() ? cfg_mode::custom : cfg_mode::config_override;

			if (const game_boot_result error = Emu.BootGame(path, "", false, config_mode, config_path); error != game_boot_result::no_errors)
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
		// If launched from CMD
		utils::attach_console(utils::console_stream::std_out | utils::console_stream::std_err, false);

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
