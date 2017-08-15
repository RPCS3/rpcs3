#include "Log.h"
#include "File.h"
#include "StrFmt.h"
#include "sema.h"

#include "Utilities/sysinfo.h"
#include "rpcs3_version.h"
#include <string>
#include <unordered_map>

#ifdef _WIN32
#include <Windows.h>
#else
#include <chrono>
#endif

static std::string empty_string()
{
	return {};
}

// Thread-specific log prefix provider
thread_local std::string(*g_tls_log_prefix)() = &empty_string;

template<>
void fmt_class_string<logs::level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto lev)
	{
		switch (lev)
		{
		case logs::level::always: return "Nothing";
		case logs::level::fatal: return "Fatal";
		case logs::level::error: return "Error";
		case logs::level::todo: return "TODO";
		case logs::level::success: return "Success";
		case logs::level::warning: return "Warning";
		case logs::level::notice: return "Notice";
		case logs::level::trace: return "Trace";
		}

		return unknown;
	});
}

namespace logs
{
	class file_writer
	{
		// Could be memory-mapped file
		fs::file m_file;

	public:
		file_writer(const std::string& name);

		virtual ~file_writer() = default;

		// Append raw data
		void log(const char* text, std::size_t size);
	};

	struct file_listener : public file_writer, public listener
	{
		file_listener(const std::string& name)
			: file_writer(name)
			, listener()
		{
			const std::string& start = fmt::format("\xEF\xBB\xBF" "RPCS3 v%s\n%s\n", rpcs3::version.to_string(), utils::get_system_info());
			file_writer::log(start.data(), start.size());
		}

		// Encode level, current thread name, channel name and write log message
		virtual void log(u64 stamp, const message& msg, const std::string& prefix, const std::string& text) override;
	};

	static file_listener* get_logger()
	{
		// Use magic static
		static file_listener logger("RPCS3.log");
		return &logger;
	}

	static u64 get_stamp()
	{
		static struct time_initializer
		{
#ifdef _WIN32
			LARGE_INTEGER freq;
			LARGE_INTEGER start;

			time_initializer()
			{
				QueryPerformanceFrequency(&freq);
				QueryPerformanceCounter(&start);
			}
#else
			std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
#endif

			u64 get() const
			{
#ifdef _WIN32
				LARGE_INTEGER now;
				QueryPerformanceCounter(&now);
				const LONGLONG diff = now.QuadPart - start.QuadPart;
				return diff / freq.QuadPart * 1'000'000 + diff % freq.QuadPart * 1'000'000 / freq.QuadPart;
#else
				return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start).count();
#endif
			}
		} timebase{};

		return timebase.get();
	}

	channel GENERAL("");
	channel LOADER("LDR");
	channel MEMORY("MEM");
	channel RSX("RSX");
	channel HLE("HLE");
	channel PPU("PPU");
	channel SPU("SPU");
	channel ARMv7("ARMv7");

	struct channel_info
	{
		channel* pointer = nullptr;
		level enabled = level::notice;

		void set_level(level value)
		{
			enabled = value;

			if (pointer)
			{
				pointer->enabled = value;
			}
		}
	};

	// Channel registry mutex
	semaphore<> g_mutex;

	// Channel registry
	std::unordered_map<std::string, channel_info> g_channels;

	void reset()
	{
		semaphore_lock lock(g_mutex);

		for (auto&& pair : g_channels)
		{
			pair.second.set_level(level::notice);
		}
	}

	void set_level(const std::string& ch_name, level value)
	{
		semaphore_lock lock(g_mutex);

		g_channels[ch_name].set_level(value);
	}
}

logs::listener::~listener()
{
}

void logs::listener::add(logs::listener* _new)
{
	// Get first (main) listener
	listener* lis = get_logger();

	// Install new listener at the end of linked list
	while (lis->m_next || !lis->m_next.compare_and_swap_test(nullptr, _new))
	{
		lis = lis->m_next;
	}
}

void logs::message::broadcast(const char* fmt, const fmt_type_info* sup, const u64* args)
{
	// Get timestamp
	const u64 stamp = get_stamp();

	// Register channel
	if (ch->enabled == level::_uninit)
	{
		semaphore_lock lock(g_mutex);

		auto& info = g_channels[ch->name];

		if (info.pointer && info.pointer != ch)
		{
			fmt::throw_exception("logs::channel repetition: %s", ch->name);
		}
		else if (!info.pointer)
		{
			info.pointer = ch;
			ch->enabled  = info.enabled;

			// Check level again
			if (info.enabled < sev)
			{
				return;
			}
		}
	}

	// Get text
	std::string text;
	fmt::raw_append(text, fmt, sup, args);
	std::string prefix = g_tls_log_prefix();

	// Get first (main) listener
	listener* lis = get_logger();
	
	// Send message to all listeners
	while (lis)
	{
		lis->log(stamp, *this, prefix, text);
		lis = lis->m_next;
	}
}

[[noreturn]] extern void catch_all_exceptions();

logs::file_writer::file_writer(const std::string& name)
{
	try
	{
		if (!m_file.open(fs::get_config_dir() + name, fs::rewrite + fs::append))
		{
			fmt::throw_exception("Can't create log file %s (error %s)", name, fs::g_tls_error);
		}
	}
	catch (...)
	{
		catch_all_exceptions();
	}
}

void logs::file_writer::log(const char* text, std::size_t size)
{
	m_file.write(text, size);
}

void logs::file_listener::log(u64 stamp, const logs::message& msg, const std::string& prefix, const std::string& _text)
{
	std::string text; text.reserve(prefix.size() + _text.size() + 200);

	// Used character: U+00B7 (Middle Dot)
	switch (msg.sev)
	{
	case level::always:  text = u8"·A "; break;
	case level::fatal:   text = u8"·F "; break;
	case level::error:   text = u8"·E "; break;
	case level::todo:    text = u8"·U "; break;
	case level::success: text = u8"·S "; break;
	case level::warning: text = u8"·W "; break;
	case level::notice:  text = u8"·! "; break;
	case level::trace:   text = u8"·T "; break;
	}

	// Print miscosecond timestamp
	const u64 hours = stamp / 3600'000'000;
	const u64 mins = (stamp % 3600'000'000) / 60'000'000;
	const u64 secs = (stamp % 60'000'000) / 1'000'000;
	const u64 frac = (stamp % 1'000'000);
	fmt::append(text, "%u:%02u:%02u.%06u ", hours, mins, secs, frac);

	if (prefix.size() > 0)
	{
		text += "{";
		text += prefix;
		text += "} ";
	}
	
	if ('\0' != *msg.ch->name)
	{
		text += msg.ch->name;
		text += msg.sev == level::todo ? " TODO: " : ": ";
	}
	else if (msg.sev == level::todo)
	{
		text += "TODO: ";
	}
	
	text += _text;
	text += '\n';

	file_writer::log(text.data(), text.size());
}
