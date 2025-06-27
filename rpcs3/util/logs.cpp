#include "util/logs.hpp"
#include "Utilities/File.h"
#include "Utilities/mutex.h"
#include "Utilities/Thread.h"
#include "Utilities/StrFmt.h"
#include <cstring>
#include <cstdarg>
#include <string>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <regex>

using namespace std::literals::chrono_literals;

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include <zlib.h>

static std::string default_string()
{
	if (thread_ctrl::is_main())
	{
		return {};
	}

	return fmt::format("TID: %u", thread_ctrl::get_tid());
}

// Thread-specific log prefix provider
thread_local std::string(*g_tls_log_prefix)() = &default_string;

// Another thread-specific callback
thread_local void(*g_tls_log_control)(const char* fmt, u64 progress) = [](const char*, u64){};

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
	static_assert(std::is_empty_v<message> && sizeof(message) == 1);
	static_assert(sizeof(channel) == alignof(channel));
	static_assert(uchar(level::always) == 0);
	static_assert(uchar(level::fatal) == 1);
	static_assert(uchar(level::trace) == 7);
	static_assert((offsetof(channel, fatal) & 7) == 1);
	static_assert((offsetof(channel, trace) & 7) == 7);

	// Memory-mapped buffer size
	constexpr u64 s_log_size = 32 * 1024 * 1024;
	static_assert(s_log_size * s_log_size > s_log_size && (s_log_size & (s_log_size - 1)) == 0); // Assert on an overflowing value

	class file_writer
	{
		std::thread m_writer{};
		fs::file m_fout{};
		fs::file m_fout2{};
		u64 m_max_size{};

		std::unique_ptr<uchar[]> m_fptr{};
		z_stream m_zs{};
		shared_mutex m_m{};

		atomic_t<u64, 64> m_buf{0}; // MSB (39 bits): push begin, LSB (25 bis): push size
		atomic_t<u64, 64> m_out{0}; // Amount of bytes written to file

		uchar m_zout[65536]{};

		// Write buffered logs immediately
		bool flush(u64 bufv);

	public:
		file_writer(const std::string& name, u64 max_size);

		virtual ~file_writer();

		// Append raw data
		void log(const char* text, usz size);

		// Ensure written to disk
		void sync();

		// Close file handle after flushing to disk
		void close_prematurely();
	};

	struct file_listener final : file_writer, public listener
	{
		file_listener(const std::string& path, u64 max_size);

		~file_listener() override = default;

		void log(u64 stamp, const message& msg, std::string_view prefix, std::string_view text) override;

		void sync() override
		{
			file_writer::sync();
		}

		void close_prematurely() override
		{
			file_writer::close_prematurely();
		}
	};

	struct root_listener final : public listener
	{
		root_listener() = default;

		~root_listener() override = default;

		// Encode level, current thread name, channel name and write log message
		void log(u64, const message&, std::string_view, std::string_view) override
		{
			// Do nothing
		}

		// Channel registry
		std::unordered_multimap<std::string, channel*> channels{};

		// Messages for delayed listener initialization
		std::vector<stored_message> messages{};
	};

	static root_listener* get_logger()
	{
		// Use magic static
		static root_listener logger{};
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
			steady_clock::time_point start = steady_clock::now();
#endif

			u64 get() const
			{
#ifdef _WIN32
				LARGE_INTEGER now;
				QueryPerformanceCounter(&now);
				const LONGLONG diff = now.QuadPart - start.QuadPart;
				return diff / freq.QuadPart * 1'000'000 + diff % freq.QuadPart * 1'000'000 / freq.QuadPart;
#else
				return (steady_clock::now() - start).count() / 1000;
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
			pair.second->enabled.release(level::notice);
		}
	}

	void silence()
	{
		std::lock_guard lock(g_mutex);

		for (auto&& pair : get_logger()->channels)
		{
			pair.second->enabled.release(level::always);
		}
	}

	void set_level(const std::string& ch_name, level value)
	{
		std::lock_guard lock(g_mutex);

		if (ch_name.find_first_of(".+*?^$()[]{}|\\") != umax)
		{
			const std::regex ex(ch_name);

			// RegEx pattern
			for (auto& channel_pair : get_logger()->channels)
			{
				std::smatch sm;

				if (std::regex_match(channel_pair.first, sm, ex))
				{
					channel_pair.second->enabled.release(value);
				}
			}

			return;
		}

		auto found = get_logger()->channels.equal_range(ch_name);

		while (found.first != found.second)
		{
			found.first->second->enabled.release(value);
			found.first++;
		}
	}

	level get_level(const std::string& ch_name)
	{
		std::lock_guard lock(g_mutex);

		const auto found = get_logger()->channels.equal_range(ch_name);

		if (found.first != found.second)
		{
			return found.first->second->enabled.observe();
		}
		else
		{
			return level::always;
		}
	}

	void set_channel_levels(const std::map<std::string, logs::level, std::less<>>& map)
	{
		for (auto&& pair : map)
		{
			logs::set_level(pair.first, pair.second);
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
	void set_init(std::initializer_list<stored_message> init_msg)
	{
		if (!g_init)
		{
			std::lock_guard lock(g_mutex);

			// Prepend main messages
			for (const auto& msg : init_msg)
			{
				get_logger()->broadcast(msg);
			}

			// Send initial messages
			for (const auto& msg : get_logger()->messages)
			{
				get_logger()->broadcast(msg);
			}

			// Clear it
			get_logger()->messages.clear();
			g_init = true;
		}
	}
}

logs::listener::~listener()
{
	// Shut up all channels on exit
	if (auto logger = get_logger())
	{
		if (logger == this)
		{
			return;
		}

		for (auto&& pair : logger->channels)
		{
			pair.second->enabled.release(level::always);
		}
	}
}

void logs::listener::add(logs::listener* _new)
{
	// Get first (main) listener
	listener* lis = get_logger();

	std::lock_guard lock(g_mutex);

	// Install new listener at the end of linked list
	listener* null = nullptr;

	while (lis->m_next || !lis->m_next.compare_exchange(null, _new))
	{
		lis = lis->m_next;
		null = nullptr;
	}
}

void logs::listener::broadcast(const logs::stored_message& msg) const
{
	for (auto lis = m_next.load(); lis; lis = lis->m_next)
	{
		lis->log(msg.stamp, msg.m, msg.prefix, msg.text);
	}
}

void logs::listener::sync()
{
}

void logs::listener::close_prematurely()
{
}

void logs::listener::sync_all()
{
	for (listener* lis = get_logger(); lis; lis = lis->m_next)
	{
		lis->sync();
	}
}

void logs::listener::close_all_prematurely()
{
	for (listener* lis = get_logger(); lis; lis = lis->m_next)
	{
		lis->close_prematurely();
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

	// Notify start operation
	g_tls_log_control(fmt, 0);

	// Get text, extract va_args
	thread_local std::string text;
	thread_local std::vector<u64> args;

	static constexpr fmt_type_info empty_sup{};

	usz args_count = 0;
	for (auto v = sup; v && v->fmt_string; v++)
		args_count++;

	text.clear();
	args.resize(args_count);

	va_list c_args;
	va_start(c_args, sup);
	for (u64& arg : args)
		arg = va_arg(c_args, u64);
	va_end(c_args);
	fmt::raw_append(text, fmt, sup ? sup : &empty_sup, args.data());
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

	// Notify end operation
	g_tls_log_control(fmt, -1);
}

logs::file_writer::file_writer(const std::string& name, u64 max_size)
	: m_max_size(max_size)
{
	if (name.empty() || !max_size)
	{
		return;
	}

	// Initialize ringbuffer
	m_fptr = std::make_unique<uchar[]>(s_log_size);

	// Actual log file (allowed to fail)
	if (!m_fout.open(name, fs::rewrite))
	{
		fprintf(stderr, "Log file open failed: %s (error %d)\n", name.c_str(), errno);
	}

	// Compressed log, make it inaccessible (foolproof)
	if (m_fout2.open(name + ".gz", fs::rewrite + fs::unread))
	{
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
		if (deflateInit2(&m_zs, 9, Z_DEFLATED, 16 + 15, 9, Z_DEFAULT_STRATEGY) != Z_OK)
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
		{
			m_fout2.close();
		}
	}

	if (!m_fout2)
	{
		fprintf(stderr, "Log file open failed: %s.gz (error %d)\n", name.c_str(), errno);
	}

#ifdef _WIN32
	// Autodelete compressed log file
	FILE_DISPOSITION_INFO disp{};
	disp.DeleteFileW = true;
	SetFileInformationByHandle(m_fout2.get_handle(), FileDispositionInfo, &disp, sizeof(disp));
#endif

	m_writer = std::thread([this]()
	{
		thread_base::set_name("Log Writer");

		thread_ctrl::scoped_priority low_prio(-1);

		while (true)
		{
			const u64 bufv = m_buf;

			if (bufv % s_log_size)
			{
				// Wait if threads are writing logs
				std::this_thread::yield();
				continue;
			}

			if (!flush(bufv))
			{
				if (m_out == umax)
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
	file_writer::sync();

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
	// Cancel compressed log file auto-deletion
	FILE_DISPOSITION_INFO disp;
	disp.DeleteFileW = false;
	SetFileInformationByHandle(m_fout2.get_handle(), FileDispositionInfo, &disp, sizeof(disp));
#else
	// Restore compressed log file permissions
	::fchmod(m_fout2.get_handle(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
}

bool logs::file_writer::flush(u64 bufv)
{
	std::lock_guard lock(m_m);

	const u64 read_pos = m_out;
	const u64 out_index = read_pos % s_log_size;
	const u64 pushed = (bufv / s_log_size) % s_log_size;
	const u64 end = std::min<u64>(out_index <= pushed ? read_pos - out_index + pushed : ((read_pos + s_log_size) & ~(s_log_size - 1)), m_max_size);

	if (end > read_pos)
	{
		// Avoid writing too big fragments
		const u64 size = std::min<u64>(end - read_pos, sizeof(m_zout) / 2);

		// Write uncompressed
		if (m_fout && m_fout.write(m_fptr.get() + out_index, size) != size)
		{
			m_fout.close();
		}

		// Write compressed
		if (m_fout2)
		{
			m_zs.avail_in = static_cast<uInt>(size);
			m_zs.next_in  = m_fptr.get() + out_index;

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

void logs::file_writer::log(const char* text, usz size)
{
	if (!m_fptr)
	{
		return;
	}

	// TODO: write bigger fragment directly in blocking manner
	while (size && size < s_log_size)
	{
		const auto [bufv, pos] = m_buf.fetch_op([&](u64& v) -> uchar*
		{
			const u64 out = m_out % s_log_size;
			const u64 v1 = (v / s_log_size) % s_log_size;
			const u64 v2 = v % s_log_size;

			if (v1 + v2 + size >= (out <= v1 ? out + s_log_size : out)) [[unlikely]]
			{
				return nullptr;
			}

			v += size;
			return m_fptr.get() + (v1 + v2) % s_log_size;
		});

		if (!pos) [[unlikely]]
		{
			if (m_out >= m_max_size || (!m_fout && !m_fout2))
			{
				// Logging is inactive
				return;
			}

			if ((bufv % s_log_size) + size >= s_log_size || bufv % s_log_size)
			{
				// Concurrency limit reached
				std::this_thread::yield();
			}
			else if (!m_m.is_free())
			{
				// Wait for another flush call to complete
				m_m.lock_unlock();
			}
			else
			{
				// Queue is full, need to write out
				flush(bufv);
			}

			continue;
		}

		if (pos - m_fptr.get() + size > s_log_size)
		{
			const auto frag = s_log_size - (pos - m_fptr.get());
			std::memcpy(pos, text, frag);
			std::memcpy(m_fptr.get(), text + frag, size - frag);
		}
		else
		{
			std::memcpy(pos, text, size);
		}

		m_buf += (size * s_log_size) - size;
		break;
	}
}

void logs::file_writer::sync()
{
	if (!m_fptr)
	{
		return;
	}

	// Wait for the writer thread
	while ((m_out % s_log_size) * s_log_size != m_buf % (s_log_size * s_log_size))
	{
		if (m_out >= m_max_size)
		{
			break;
		}

		std::this_thread::yield();
	}

	if (thread_ctrl::get_current())
	{
		return;
	}

	// Ensure written to disk
	if (m_fout)
	{
		m_fout.sync();
	}

	if (m_fout2)
	{
		m_fout2.sync();
	}
}

void logs::file_writer::close_prematurely()
{
	if (!m_fptr)
	{
		return;
	}

	// Ensure written to disk
	sync();

	std::lock_guard lock(m_m);

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

#ifdef _WIN32
		// Cancel compressed log file auto-deletion
		FILE_DISPOSITION_INFO disp;
		disp.DeleteFileW = false;
		SetFileInformationByHandle(m_fout2.get_handle(), FileDispositionInfo, &disp, sizeof(disp));
#else
		// Restore compressed log file permissions
		::fchmod(m_fout2.get_handle(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif

		m_fout2.close();
	}

	if (m_fout)
	{
		m_fout.close();
	}
}

logs::file_listener::file_listener(const std::string& path, u64 max_size)
	: file_writer(path, max_size)
	, listener()
{
	// Write UTF-8 BOM
	file_writer::log("\xEF\xBB\xBF", 3);
}

void logs::file_listener::log(u64 stamp, const logs::message& msg, std::string_view prefix, std::string_view _text)
{
	/*constinit thread_local*/ std::string text;
	text.reserve(50000);

	// Used character: U+00B7 (Middle Dot)
	switch (msg)
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

	// Print microsecond timestamp
	const u64 hours = stamp / 3600'000'000;
	const u64 mins = (stamp % 3600'000'000) / 60'000'000;
	const u64 secs = (stamp % 60'000'000) / 1'000'000;
	const u64 frac = (stamp % 1'000'000);
	fmt::append(text, "%u:%02u:%02u.%06u ", hours, mins, secs, frac);

	if (stamp == 0)
	{
		// Workaround for first special messages to keep backward compatibility
		text.clear();
	}

	if (!prefix.empty())
	{
		text += "{";
		text += prefix;
		text += "} ";
	}

	if (stamp && msg->name && '\0' != *msg->name)
	{
		text += msg->name;
		text += msg == level::todo ? " TODO: " : ": ";
	}
	else if (msg == level::todo)
	{
		text += "TODO: ";
	}

	text += _text;
	text += '\n';

	file_writer::log(text.data(), text.size());
}

std::unique_ptr<logs::listener> logs::make_file_listener(const std::string& path, u64 max_size)
{
	std::unique_ptr<logs::listener> result = std::make_unique<logs::file_listener>(path, max_size);

	// Register file listener
	result->add(result.get());
	return result;
}
