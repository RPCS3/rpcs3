#include "Log.h"
#include "File.h"
#include "StrFmt.h"
#include "sema.h"

#include "Utilities/sysinfo.h"
#include "Utilities/Thread.h"
#include "rpcs3_version.h"
#include <cstring>
#include <cstdarg>
#include <string>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>

using namespace std::literals::chrono_literals;

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include <zlib.h>

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
	// Memory-mapped buffer size
	constexpr u64 s_log_size = 32 * 1024 * 1024;

	class file_writer
	{
		fs::file m_file;
		std::string m_name;
		std::thread m_writer;
		fs::file m_fout;
		fs::file m_fout2;
		u64 m_max_size;

#ifdef _WIN32
		::HANDLE m_fmap;
#endif
		uchar* m_fptr{};
		z_stream m_zs{};
		shared_mutex m_m;

		alignas(128) atomic_t<u64> m_buf{0}; // MSB (40 bit): push begin, LSB (24 bis): push size
		alignas(128) atomic_t<u64> m_out{0}; // Amount of bytes written to file

		uchar m_zout[65536];

		// Write buffered logs immediately
		bool flush(u64 bufv);

	public:
		file_writer(const std::string& name);

		virtual ~file_writer();

		// Append raw data
		void log(logs::level sev, const char* text, std::size_t size);
	};

	struct stored_message
	{
		message m;
		u64 stamp;
		std::string prefix;
		std::string text;
	};

	struct file_listener : public file_writer, public listener
	{
		file_listener(const std::string& name);

		virtual ~file_listener() = default;

		// Encode level, current thread name, channel name and write log message
		virtual void log(u64 stamp, const message& msg, const std::string& prefix, const std::string& text) override;

		// Channel registry
		std::unordered_multimap<std::string, channel*> channels;

		// Messages for delayed listener initialization
		std::vector<stored_message> messages;
	};

	static file_listener* get_logger()
	{
		// Use magic static
		static file_listener logger("RPCS3");
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

	// Channel registry mutex
	static shared_mutex g_mutex;

	// Must be set to true in main()
	static atomic_t<bool> g_init{false};

	void reset()
	{
		std::lock_guard lock(g_mutex);

		for (auto&& pair : get_logger()->channels)
		{
			pair.second->enabled.store(level::notice, std::memory_order_relaxed);
		}
	}

	void silence()
	{
		std::lock_guard lock(g_mutex);

		for (auto&& pair : get_logger()->channels)
		{
			pair.second->enabled.store(level::always, std::memory_order_relaxed);
		}
	}

	void set_level(const std::string& ch_name, level value)
	{
		std::lock_guard lock(g_mutex);

		auto found = get_logger()->channels.equal_range(ch_name);

		while (found.first != found.second)
		{
			found.first->second->enabled.store(value, std::memory_order_relaxed);
			found.first++;
		}
	}

	level get_level(const std::string& ch_name)
	{
		std::lock_guard lock(g_mutex);

		auto found = get_logger()->channels.equal_range(ch_name);

		if (found.first != found.second)
		{
			return found.first->second->enabled.load(std::memory_order_relaxed);
		}
		else
		{
			return level::always;
		}
	}

	std::vector<std::string> get_channels()
	{
		std::vector<std::string> result;

		std::lock_guard lock(g_mutex);

		for (auto&& p : get_logger()->channels)
		{
			// Copy names removing duplicates
			if (result.empty() || result.back() != p.first)
			{
				result.push_back(p.first);
			}
		}

		return result;
	}

	// Must be called in main() to stop accumulating messages in g_messages
	void set_init()
	{
		if (!g_init)
		{
			std::lock_guard lock(g_mutex);
			printf("DEBUG: %zu log messages accumulated. %zu channels registered.\n", get_logger()->messages.size(), get_logger()->channels.size());
			get_logger()->messages.clear();
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

	std::lock_guard lock(g_mutex);

	// Install new listener at the end of linked list
	while (lis->m_next || !lis->m_next.compare_and_swap_test(nullptr, _new))
	{
		lis = lis->m_next;
	}

	// Send initial messages
	for (const auto& msg : get_logger()->messages)
	{
		_new->log(msg.stamp, msg.m, msg.prefix, msg.text);
	}
}

logs::registerer::registerer(channel& _ch)
{
	std::lock_guard lock(g_mutex);

	get_logger()->channels.emplace(_ch.name, &_ch);
}

void logs::message::broadcast(const char* fmt, const fmt_type_info* sup, ...) const
{
	// Get timestamp
	const u64 stamp = get_stamp();

	// Get text, extract va_args
	thread_local std::string text;
	thread_local std::vector<u64> args;

	std::size_t args_count = 0;
	for (auto v = sup; v->fmt_string; v++)
		args_count++;

	text.clear();
	args.resize(args_count);

	va_list c_args;
	va_start(c_args, sup);
	for (u64& arg : args)
		arg = va_arg(c_args, u64);
	va_end(c_args);
	fmt::raw_append(text, fmt, sup, args.data());
	std::string prefix = g_tls_log_prefix();

	// Get first (main) listener
	listener* lis = get_logger();

	if (!g_init)
	{
		std::lock_guard lock(g_mutex);

		if (!g_init)
		{
			while (lis)
			{
				lis->log(stamp, *this, prefix, text);
				lis = lis->m_next;
			}

			// Store message additionally
			get_logger()->messages.emplace_back(stored_message{*this, stamp, std::move(prefix), text});
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
	: m_name(name)
{
	const std::string log_name = fs::get_cache_dir() + name + ".log";
	const std::string buf_name = fs::get_cache_dir() + name + ".buf";

	const std::string s_filelock = fs::get_cache_dir() + ".restart_lock";

	try
	{
		if (!m_file.open(buf_name, fs::read + fs::rewrite + fs::lock))
		{
#ifdef _WIN32
			auto prev_error = fs::g_tls_error;

			if (fs::exists(s_filelock))
			{
				// A restart is happening, wait for the file to be accessible
				u32 tries = 0;
				while (!m_file.open(buf_name, fs::read + fs::rewrite + fs::lock) && tries < 100)
				{
					std::this_thread::sleep_for(100ms);
					tries++;
				}
			}
			else
			{
				fs::g_tls_error = prev_error;
			}

			if (!m_file)
#endif
			{
				if (fs::g_tls_error == fs::error::acces)
				{
					if (fs::exists(buf_name))
					{
						fmt::throw_exception("Another instance of %s is running. Close it or kill its process, if necessary.", name);
					}
					else
					{
						fmt::throw_exception("Cannot create %s.log (access denied)."
#ifdef _WIN32
						                     "\nNote that %s cannot be installed in Program Files or similar directory with limited permissions."
#else
						                     "\nPlease, check %s permissions in '~/.config/'."
#endif
						                     , name, name);
					}
				}

				fmt::throw_exception("Cannot create %s.log (error %s)", name, fs::g_tls_error);
			}
		}

#ifdef _WIN32
		fs::remove_file(s_filelock); // remove restart token if it exists
#endif

		// Check free space
		fs::device_stat stats{};
		if (!fs::statfs(fs::get_cache_dir(), stats) || stats.avail_free < s_log_size * 8)
		{
			fmt::throw_exception("Not enough free space (%f KB)", stats.avail_free / 1000000.);
		}

		// Limit log size to ~25% of free space
		m_max_size = stats.avail_free / 4;

		// Initialize memory mapped file
#ifdef _WIN32
		m_fmap = CreateFileMappingW(m_file.get_handle(), 0, PAGE_READWRITE, s_log_size >> 32, s_log_size & 0xffffffff, 0);
		m_fptr = m_fmap ? static_cast<uchar*>(MapViewOfFile(m_fmap, FILE_MAP_WRITE, 0, 0, 0)) : nullptr;
#else
		m_file.trunc(s_log_size);
		m_fptr = static_cast<uchar*>(::mmap(0, s_log_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_file.get_handle(), 0));
#endif

		verify(name.c_str()), m_fptr;

		// Rotate backups (TODO)
		if (std::string gz_file_name = fs::get_cache_dir() + m_name + ".log.gz"; fs::is_file(gz_file_name))
		{
			fs::remove_file(fs::get_cache_dir() + name + "1.log.gz");
			fs::create_dir(fs::get_cache_dir() + "old_logs");
			fs::rename(gz_file_name, fs::get_cache_dir() + "old_logs/" + m_name + ".log.gz", true);
		}

		// Actual log file (allowed to fail)
		m_fout.open(log_name, fs::rewrite);

		// Compressed log, make it inaccessible (foolproof)
		if (m_fout2.open(log_name + ".gz", fs::rewrite + fs::unread))
		{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
			if (deflateInit2(&m_zs, 9, Z_DEFLATED, 16 + 15, 9, Z_DEFAULT_STRATEGY) != Z_OK)
#pragma GCC diagnostic pop
				m_fout2.close();
		}

#ifdef _WIN32
		// Autodelete compressed log file
		FILE_DISPOSITION_INFO disp;
		disp.DeleteFileW = true;
		SetFileInformationByHandle(m_fout2.get_handle(), FileDispositionInfo, &disp, sizeof(disp));
#endif
	}
	catch (const std::exception& e)
	{
		std::thread([text = std::string{e.what()}]{ report_fatal_error(text); }).detach();
		return;
	}
	catch (...)
	{
		std::thread([]{ report_fatal_error("Unknown error" HERE); }).detach();
		return;
	}

	m_writer = std::thread([this]()
	{
		thread_ctrl::set_native_priority(-1);

		while (true)
		{
			const u64 bufv = m_buf;

			if (bufv & 0xffffff)
			{
				// Wait if threads are writing logs
				std::this_thread::yield();
				continue;
			}

			if (!flush(bufv))
			{
				if (m_out == -1)
				{
					break;
				}

				std::this_thread::sleep_for(10ms);
			}
		}
	});
}

logs::file_writer::~file_writer()
{
	if (!m_fptr)
	{
		return;
	}

	// Stop writer thread
	while (m_out << 24 < m_buf)
	{
		std::this_thread::yield();
	}

	m_out = -1;
	m_writer.join();

	if (m_fout2)
	{
		m_zs.avail_in = 0;
		m_zs.next_in  = nullptr;

		do
		{
			m_zs.avail_out = sizeof(m_zout);
			m_zs.next_out  = m_zout;

			if (deflate(&m_zs, Z_FINISH) == Z_STREAM_ERROR || m_fout2.write(m_zout, sizeof(m_zout) - m_zs.avail_out) != sizeof(m_zout) - m_zs.avail_out)
			{
				break;
			}
		}
		while (m_zs.avail_out == 0);

		deflateEnd(&m_zs);
	}

#ifdef _WIN32
	// Cancel compressed log file autodeletion
	FILE_DISPOSITION_INFO disp;
	disp.DeleteFileW = false;
	SetFileInformationByHandle(m_fout2.get_handle(), FileDispositionInfo, &disp, sizeof(disp));

	UnmapViewOfFile(m_fptr);
	CloseHandle(m_fmap);
#else
	// Restore compressed log file permissions
	::fchmod(m_fout2.get_handle(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	::munmap(m_fptr, s_log_size);
#endif
}

bool logs::file_writer::flush(u64 bufv)
{
	std::lock_guard lock(m_m);

	const u64 st  = +m_out;
	const u64 end = std::min<u64>((st + s_log_size) & ~(s_log_size - 1), bufv >> 24);

	if (end > st)
	{
		// Avoid writing too big fragments
		const u64 size = std::min<u64>(end - st, sizeof(m_zout) / 2);

		// Write uncompressed
		if (m_fout && st < m_max_size && m_fout.write(m_fptr + st % s_log_size, size) != size)
		{
			m_fout.close();
		}

		// Write compressed
		if (m_fout2 && st < m_max_size)
		{
			m_zs.avail_in = static_cast<uInt>(size);
			m_zs.next_in  = m_fptr + st % s_log_size;

			do
			{
				m_zs.avail_out = sizeof(m_zout);
				m_zs.next_out  = m_zout;

				if (deflate(&m_zs, Z_NO_FLUSH) == Z_STREAM_ERROR || m_fout2.write(m_zout, sizeof(m_zout) - m_zs.avail_out) != sizeof(m_zout) - m_zs.avail_out)
				{
					deflateEnd(&m_zs);
					m_fout2.close();
					break;
				}
			}
			while (m_zs.avail_out == 0);
		}

		m_out += size;
		return true;
	}

	return false;
}

void logs::file_writer::log(logs::level sev, const char* text, std::size_t size)
{
	if (!m_fptr)
	{
		return;
	}

	// TODO: write bigger fragment directly in blocking manner
	while (size && size <= 0xffffff)
	{
		u64 bufv;

		const auto pos = m_buf.atomic_op([&](u64& v) -> uchar*
		{
			const u64 v1 = v >> 24;
			const u64 v2 = v & 0xffffff;

			if (v2 + size > 0xffffff || v1 + v2 + size >= m_out + s_log_size) [[unlikely]]
			{
				bufv = v;
				return nullptr;
			}

			v += size;
			return m_fptr + (v1 + v2) % s_log_size;
		});

		if (!pos) [[unlikely]]
		{
			if ((bufv & 0xffffff) + size > 0xffffff || bufv & 0xffffff)
			{
				// Concurrency limit reached
				std::this_thread::yield();
			}
			else
			{
				// Queue is full, need to write out
				flush(bufv);
			}
			continue;
		}

		if (pos + size > m_fptr + s_log_size)
		{
			const auto frag = m_fptr + s_log_size - pos;
			std::memcpy(pos, text, frag);
			std::memcpy(m_fptr, text + frag, size - frag);
		}
		else
		{
			std::memcpy(pos, text, size);
		}

		m_buf += (u64{size} << 24) - size;
		break;
	}
}

logs::file_listener::file_listener(const std::string& name)
	: file_writer(name)
	, listener()
{
	// Write UTF-8 BOM
	file_writer::log(logs::level::always, "\xEF\xBB\xBF", 3);

	const std::string firmware_version = utils::get_firmware_version();
	const std::string firmware_string  = firmware_version.empty() ? "" : (" | Firmware version: " + firmware_version);

	// Write initial message
	stored_message ver;
	ver.m.ch  = nullptr;
	ver.m.sev = level::always;
	ver.stamp = 0;
	ver.text  = fmt::format("RPCS3 v%s | %s%s\n%s", rpcs3::get_version().to_string(), rpcs3::get_branch(), firmware_string, utils::get_system_info());

	file_writer::log(logs::level::always, ver.text.data(), ver.text.size());
	file_writer::log(logs::level::always, "\n", 1);
	messages.emplace_back(std::move(ver));

	// Write OS version
	stored_message os;
	os.m.ch  = nullptr;
	os.m.sev = level::notice;
	os.stamp = 0;
	os.text = utils::get_OS_version();

	file_writer::log(logs::level::notice, os.text.data(), os.text.size());
	file_writer::log(logs::level::notice, "\n", 1);
	messages.emplace_back(std::move(os));
}

void logs::file_listener::log(u64 stamp, const logs::message& msg, const std::string& prefix, const std::string& _text)
{
	thread_local std::string text;

	// Used character: U+00B7 (Middle Dot)
	switch (msg.sev)
	{
	case level::always:  text = reinterpret_cast<const char*>(u8"·A "); break;
	case level::fatal:   text = reinterpret_cast<const char*>(u8"·F "); break;
	case level::error:   text = reinterpret_cast<const char*>(u8"·E "); break;
	case level::todo:    text = reinterpret_cast<const char*>(u8"·U "); break;
	case level::success: text = reinterpret_cast<const char*>(u8"·S "); break;
	case level::warning: text = reinterpret_cast<const char*>(u8"·W "); break;
	case level::notice:  text = reinterpret_cast<const char*>(u8"·! "); break;
	case level::trace:   text = reinterpret_cast<const char*>(u8"·T "); break;
	}

	// Print µs timestamp
	const u64 hours = stamp / 3600'000'000;
	const u64 mins = (stamp % 3600'000'000) / 60'000'000;
	const u64 secs = (stamp % 60'000'000) / 1'000'000;
	const u64 frac = (stamp % 1'000'000);
	fmt::append(text, "%u:%02u:%02u.%06u ", hours, mins, secs, frac);

	if (!prefix.empty())
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
