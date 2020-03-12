#include "util/logs.hpp"
#include "Utilities/File.h"
#include "Utilities/mutex.h"
#include "Utilities/Thread.h"
#include <cstring>
#include <cstdarg>
#include <string>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstring>
#include <cerrno>

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
		std::thread m_writer;
		fs::file m_fout;
		fs::file m_fout2;
		u64 m_max_size;

		std::unique_ptr<uchar[]> m_fptr;
		z_stream m_zs{};
		shared_mutex m_m;

		alignas(128) atomic_t<u64> m_buf{0}; // MSB (40 bit): push begin, LSB (24 bis): push size
		alignas(128) atomic_t<u64> m_out{0}; // Amount of bytes written to file

		uchar m_zout[65536];

		// Write buffered logs immediately
		bool flush(u64 bufv);

	public:
		file_writer(const std::string& name, u64 max_size);

		virtual ~file_writer();

		// Append raw data
		void log(const char* text, std::size_t size);
	};

	struct file_listener final : file_writer, public listener
	{
		file_listener(const std::string& path, u64 max_size);

		~file_listener() override = default;

		void log(u64 stamp, const message& msg, const std::string& prefix, const std::string& text) override;
	};

	struct root_listener final : public listener
	{
		root_listener() = default;

		~root_listener() override = default;

		// Encode level, current thread name, channel name and write log message
		void log(u64 stamp, const message& msg, const std::string& prefix, const std::string& text) override
		{
			// Do nothing
		}

		// Channel registry
		std::unordered_multimap<std::string, channel*> channels;

		// Messages for delayed listener initialization
		std::vector<stored_message> messages;
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
			pair.second->enabled.store(level::always, std::memory_order_relaxed);
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

	while (lis->m_next || !lis->m_next.compare_exchange_strong(null, _new))
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
	/*constinit thread_local*/ std::string text;
	/*constinit thread_local*/ std::basic_string<u64> args;

	static constexpr fmt_type_info empty_sup{};

	std::size_t args_count = 0;
	for (auto v = sup; v && v->fmt_string; v++)
		args_count++;

	text.reserve(50000);
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
}

logs::file_writer::file_writer(const std::string& name, u64 max_size)
	: m_max_size(max_size)
{
	if (!name.empty() && max_size)
	{
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
				m_fout2.close();
		}

		if (!m_fout2)
		{
			fprintf(stderr, "Log file open failed: %s.gz (error %d)\n", name.c_str(), errno);
		}

#ifdef _WIN32
		// Autodelete compressed log file
		FILE_DISPOSITION_INFO disp;
		disp.DeleteFileW = true;
		SetFileInformationByHandle(m_fout2.get_handle(), FileDispositionInfo, &disp, sizeof(disp));
#endif
	}
	else
	{
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
#else
	// Restore compressed log file permissions
	::fchmod(m_fout2.get_handle(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
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
		if (m_fout && st < m_max_size && m_fout.write(m_fptr.get() + st % s_log_size, size) != size)
		{
			m_fout.close();
		}

		// Write compressed
		if (m_fout2 && st < m_max_size)
		{
			m_zs.avail_in = static_cast<uInt>(size);
			m_zs.next_in  = m_fptr.get() + st % s_log_size;

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

void logs::file_writer::log(const char* text, std::size_t size)
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
			return m_fptr.get() + (v1 + v2) % s_log_size;
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

		if (pos + size > m_fptr.get() + s_log_size)
		{
			const auto frag = m_fptr.get() + s_log_size - pos;
			std::memcpy(pos, text, frag);
			std::memcpy(m_fptr.get(), text + frag, size - frag);
		}
		else
		{
			std::memcpy(pos, text, size);
		}

		m_buf += (u64{size} << 24) - size;
		break;
	}
}

logs::file_listener::file_listener(const std::string& path, u64 max_size)
	: file_writer(path, max_size)
	, listener()
{
	// Write UTF-8 BOM
	file_writer::log("\xEF\xBB\xBF", 3);
}

void logs::file_listener::log(u64 stamp, const logs::message& msg, const std::string& prefix, const std::string& _text)
{
	/*constinit thread_local*/ std::string text;
	text.reserve(50000);

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

	if (msg.ch == nullptr && stamp == 0)
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

	file_writer::log(text.data(), text.size());
}

std::unique_ptr<logs::listener> logs::make_file_listener(const std::string& path, u64 max_size)
{
	std::unique_ptr<logs::listener> result = std::make_unique<logs::file_listener>(path, max_size);

	// Register file listener
	result->add(result.get());
	return result;
}
