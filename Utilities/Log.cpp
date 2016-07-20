#include "Log.h"
#include "File.h"
#include "StrFmt.h"

#include "rpcs3_version.h"
#include <cstdarg>
#include <string>

// Thread-specific log prefix provider
thread_local std::string(*g_tls_log_prefix)() = nullptr;

#ifndef _MSC_VER
constexpr DECLARE(bijective<logs::level, const char*>::map);
#endif

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
			const std::string& start = fmt::format("\xEF\xBB\xBF" "RPCS3 v%s\n", rpcs3::version.to_string());
			file_writer::log(start.data(), start.size());
		}

		// Encode level, current thread name, channel name and write log message
		virtual void log(const message& msg) override;
	};

	static file_listener* get_logger()
	{
		// Use magic static
		static file_listener logger("RPCS3.log");
		return &logger;
	}

	channel GENERAL(nullptr, level::notice);
	channel LOADER("LDR", level::notice);
	channel MEMORY("MEM", level::notice);
	channel RSX("RSX", level::notice);
	channel HLE("HLE", level::notice);
	channel PPU("PPU", level::notice);
	channel SPU("SPU", level::notice);
	channel ARMv7("ARMv7");
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

void logs::channel::broadcast(const logs::channel& ch, logs::level sev, const char* fmt...)
{
	va_list args;
	va_start(args, fmt);
	std::string&& text = fmt::unsafe_vformat(fmt, args);
	std::string&& prefix = g_tls_log_prefix ? g_tls_log_prefix() : "";
	va_end(args);

	// Prepare message information
	message msg;
	msg.ch = &ch;
	msg.sev = sev;
	msg.text = text.data();
	msg.text_size = text.size();
	msg.prefix = prefix.data();
	msg.prefix_size = prefix.size();

	// Get first (main) listener
	listener* lis = get_logger();
	
	// Send message to all listeners
	while (lis)
	{
		lis->log(msg);
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
			throw fmt::exception("Can't create log file %s (error %s)", name, fs::g_tls_error);
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

void logs::file_listener::log(const logs::message& msg)
{
	std::string text; text.reserve(msg.text_size + msg.prefix_size + 200);

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

	// TODO: print time?

	if (msg.prefix_size > 0)
	{
		text += fmt::format("{%s} ", msg.prefix);
	}
	
	if (msg.ch->name)
	{
		text += msg.ch->name;
		text += msg.sev == level::todo ? " TODO: " : ": ";
	}
	else if (msg.sev == level::todo)
	{
		text += "TODO: ";
	}
	
	text += msg.text;
	text += '\n';

	file_writer::log(text.data(), text.size());
}
