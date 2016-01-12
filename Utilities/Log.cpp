#include "stdafx.h"
#include "Thread.h"
#include "File.h"
#include "Log.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace _log
{
	logger& get_logger()
	{
		// Use magic static for global logger instance
		static logger instance;
		return instance;
	}

	file_listener g_log_file(_PRGNAME_ ".log");

	file_writer g_tty_file("TTY.log");

	channel GENERAL("", level::notice);
	channel LOADER("LDR", level::notice);
	channel MEMORY("MEM", level::notice);
	channel RSX("RSX", level::notice);
	channel HLE("HLE", level::notice);
	channel PPU("PPU", level::notice);
	channel SPU("SPU", level::notice);
	channel ARMv7("ARMv7");
}

_log::listener::listener()
{
	// Register self
	get_logger().add_listener(this);
}

_log::listener::~listener()
{
	// Unregister self
	get_logger().remove_listener(this);
}

_log::channel::channel(const std::string& name, _log::level init_level)
	: name{ name }
	, enabled{ init_level }
{
	// TODO: register config property "name" associated with "enabled" member
}

void _log::logger::add_listener(_log::listener* listener)
{
	std::lock_guard<shared_mutex> lock(m_mutex);

	m_listeners.emplace(listener);
}

void _log::logger::remove_listener(_log::listener* listener)
{
	std::lock_guard<shared_mutex> lock(m_mutex);

	m_listeners.erase(listener);
}

void _log::logger::broadcast(const _log::channel& ch, _log::level sev, const std::string& text) const
{
	reader_lock lock(m_mutex);

	for (auto listener : m_listeners)
	{
		listener->log(ch, sev, text);
	}
}

void _log::broadcast(const _log::channel& ch, _log::level sev, const std::string& text)
{
	get_logger().broadcast(ch, sev, text);
}

_log::file_writer::file_writer(const std::string& name)
{
	try
	{
		if (!m_file.open(fs::get_config_dir() + name, fom::rewrite | fom::append))
		{
			throw EXCEPTION("Can't create log file %s (error %d)", name, errno);
		}
	}
	catch (const fmt::exception& e)
	{
#ifdef _WIN32
		MessageBoxA(0, e.what(), "_log::file_writer() failed", MB_ICONERROR);
#else
		std::printf("_log::file_writer() failed: %s\n", e.what());
#endif
	}
}

void _log::file_writer::log(const std::string& text)
{
	m_file.write(text);
}

std::size_t _log::file_writer::size() const
{
	return m_file.seek(0, fs::seek_cur);
}

void _log::file_listener::log(const _log::channel& ch, _log::level sev, const std::string& text)
{
	std::string msg; msg.reserve(text.size() + 200);

	// Used character: U+00B7 (Middle Dot)
	switch (sev)
	{
	case level::always:  msg = u8"·A "; break;
	case level::fatal:   msg = u8"·F "; break;
	case level::error:   msg = u8"·E "; break;
	case level::todo:    msg = u8"·U "; break;
	case level::success: msg = u8"·S "; break;
	case level::warning: msg = u8"·W "; break;
	case level::notice:  msg = u8"·! "; break;
	case level::trace:   msg = u8"·T "; break;
	}

	// TODO: print time?

	if (auto t = thread_ctrl::get_current())
	{
		msg += '{';
		msg += t->get_name();
		msg += "} ";
	}

	if (ch.name.size())
	{
		msg += ch.name;
		msg += sev == level::todo ? " TODO: " : ": ";
	}
	else if (sev == level::todo)
	{
		msg += "TODO: ";
	}
	
	msg += text;
	msg += '\n';

	file_writer::log(msg);
}
