#include "Log.h"
#include "File.h"
#include "StrFmt.h"
#include "sema.h"

#include "Utilities/sysinfo.h"
#include "rpcs3_version.h"
#include <string>
#include <unordered_map>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <chrono>
#include <sys/mman.h>
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
	constexpr std::size_t s_log_size = 128 * 1024 * 1024;
	constexpr std::size_t s_log_size_s = s_log_size / 2;
	constexpr std::size_t s_log_size_t = s_log_size / 4 + s_log_size_s;
	constexpr std::size_t s_log_size_e = s_log_size / 8 + s_log_size_t;
	constexpr std::size_t s_log_size_f = s_log_size / 16 + s_log_size_e;

	class file_writer
	{
		fs::file m_file;
		std::string m_name;

#ifdef _WIN32
		::HANDLE m_fmap;
#endif
		atomic_t<std::size_t> m_pos{0};
		std::size_t m_size{0};
		uchar* m_fptr{};

	public:
		file_writer(const std::string& name);

		virtual ~file_writer();

		// Append raw data
		void log(logs::level sev, const char* text, std::size_t size);
	};

	struct file_listener : public file_writer, public listener
	{
		file_listener(const std::string& name);

		virtual ~file_listener() = default;

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

	struct stored_message
	{
		message m;
		u64 stamp;
		std::string prefix;
		std::string text;
	};

	// Channel registry mutex
	semaphore<> g_mutex;

	// Channel registry
	std::unordered_map<std::string, channel_info> g_channels;

	// Messages for delayed listener initialization
	std::vector<stored_message> g_messages;

	// Must be set to true in main()
	atomic_t<bool> g_init{false};

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

	// Must be called in main() to stop accumulating messages in g_messages
	void set_init()
	{
		if (!g_init)
		{
			semaphore_lock lock(g_mutex);
			g_messages.clear();
			g_init = true;
		}
	}
}

logs::listener::~listener()
{
}

void logs::listener::add(logs::listener* _new)
{
	// Get first (main) listener
	listener* lis = get_logger();

	semaphore_lock lock(g_mutex);

	// Install new listener at the end of linked list
	while (lis->m_next || !lis->m_next.compare_and_swap_test(nullptr, _new))
	{
		lis = lis->m_next;
	}

	// Send initial messages
	for (const auto& msg : g_messages)
	{
		_new->log(msg.stamp, msg.m, msg.prefix, msg.text);
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
	thread_local std::string text; text.clear();
	fmt::raw_append(text, fmt, sup, args);
	std::string prefix = g_tls_log_prefix();

	// Get first (main) listener
	listener* lis = get_logger();

	if (!g_init)
	{
		semaphore_lock lock(g_mutex);

		if (!g_init)
		{
			while (lis)
			{
				lis->log(stamp, *this, prefix, text);
				lis = lis->m_next;
			}

			// Store message additionally
			g_messages.emplace_back(stored_message{*this, stamp, std::move(prefix), text});
		}
	}
	
	// Send message to all listeners
	while (lis)
	{
		lis->log(stamp, *this, prefix, text);
		lis = lis->m_next;
	}
}

[[noreturn]] extern void catch_all_exceptions();

logs::file_writer::file_writer(const std::string& name)
	: m_name(fs::get_config_dir() + name)
{
	try
	{
		if (!m_file.open(m_name, fs::read + fs::write + fs::create + fs::trunc + fs::unshare))
		{
			fmt::throw_exception("Can't create file %s (error %s)", name, fs::g_tls_error);
		}

#ifdef _WIN32
		m_fmap = CreateFileMappingW(m_file.get_handle(), 0, PAGE_READWRITE, s_log_size >> 32, s_log_size & 0xffffffff, 0);
		m_fptr = (uchar*)MapViewOfFile(m_fmap, FILE_MAP_WRITE, 0, 0, 0);
#else
		m_file.trunc(s_log_size);
		m_fptr = (uchar*)::mmap(0, s_log_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_file.get_handle(), 0);
#endif

		std::memset(m_fptr, '\n', s_log_size);
	}
	catch (...)
	{
		catch_all_exceptions();
	}
}

logs::file_writer::~file_writer()
{
	if (m_size == 0)
	{
		m_size = std::min<std::size_t>(+m_pos, s_log_size);
	}

#ifdef _WIN32
	UnmapViewOfFile(m_fptr);
	CloseHandle(m_fmap);
	m_file.seek(m_size);
	SetEndOfFile(m_file.get_handle());
#else
	::munmap(m_fptr, s_log_size);
	m_file.trunc(m_size);
#endif
}

void logs::file_writer::log(logs::level sev, const char* text, std::size_t size)
{
	// Adaptive log limit
	const auto lim =
		sev >= logs::level::success ? s_log_size_s :
		sev == logs::level::todo ? s_log_size_t :
		sev == logs::level::error ? s_log_size_e : s_log_size_f;

	if (m_pos >= lim)
	{
		return;
	}

	// Acquire memory
	const auto pos = m_pos.fetch_add(size);

	// Write if possible
	if (pos + size <= s_log_size)
	{
		std::memcpy(m_fptr + pos, text, size);
	}
	else if (pos <= s_log_size)
	{
		m_size = pos;
	}
}

logs::file_listener::file_listener(const std::string& name)
	: file_writer(name)
	, listener()
{
	// Write UTF-8 BOM
	file_writer::log(logs::level::always, "\xEF\xBB\xBF", 3);

	// Write initial message
	stored_message ver;
	ver.m.ch  = nullptr;
	ver.m.sev = level::always;
	ver.stamp = 0;
	ver.text  = fmt::format("RPCS3 v%s\n%s", rpcs3::version.to_string(), utils::get_system_info());
	file_writer::log(logs::level::always, ver.text.data(), ver.text.size());
	file_writer::log(logs::level::always, "\n", 1);
	g_messages.emplace_back(std::move(ver));
}

void logs::file_listener::log(u64 stamp, const logs::message& msg, const std::string& prefix, const std::string& _text)
{
	thread_local std::string text;

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
	
	if (msg.ch && '\0' != *msg.ch->name)
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

	file_writer::log(msg.sev, text.data(), text.size());
}
