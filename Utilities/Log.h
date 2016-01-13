#pragma once

#include "SharedMutex.h"

namespace _log
{
	enum class level : uint
	{
		always, // highest level (unused, cannot be disabled)
		fatal,
		error,
		todo,
		success,
		warning,
		notice,
		trace, // lowest level (usually disabled)
	};

	struct channel;
	struct listener;

	// Log manager
	class logger final
	{
		mutable shared_mutex m_mutex;

		std::set<listener*> m_listeners;

	public:
		// Register listener
		void add_listener(listener* listener);

		// Unregister listener
		void remove_listener(listener* listener);

		// Send log message to all listeners
		void broadcast(const channel& ch, level sev, const std::string& text) const;
	};

	// Send log message to global logger instance
	void broadcast(const channel& ch, level sev, const std::string& text);

	// Log channel (source)
	struct channel
	{
		// Channel prefix (also used for identification)
		const std::string name;

		// The lowest logging level enabled for this channel (used for early filtering)
		std::atomic<level> enabled;

		// Initialization (max level enabled by default)
		channel(const std::string& name, level = level::trace);

		virtual ~channel() = default;

		// Log without formatting
		force_inline void log(level sev, const std::string& text) const
		{
			if (sev <= enabled)
				broadcast(*this, sev, text);
		}

		// Log with formatting
		template<typename... Args>
		force_inline safe_buffers void format(level sev, const char* fmt, const Args&... args) const
		{
			if (sev <= enabled)
				broadcast(*this, sev, fmt::format(fmt, fmt::do_unveil(args)...));
		}

#define GEN_LOG_METHOD(_sev)\
		template<typename... Args>\
		force_inline void _sev(const char* fmt, const Args&... args)\
		{\
			return format<Args...>(level::_sev, fmt, args...);\
		}

		GEN_LOG_METHOD(fatal)
		GEN_LOG_METHOD(error)
		GEN_LOG_METHOD(todo)
		GEN_LOG_METHOD(success)
		GEN_LOG_METHOD(warning)
		GEN_LOG_METHOD(notice)
		GEN_LOG_METHOD(trace)

#undef GEN_LOG_METHOD
	};

	// Log listener (destination)
	struct listener
	{
		listener();
		
		virtual ~listener();

		virtual void log(const channel& ch, level sev, const std::string& text) = 0;
	};

	class file_writer
	{
		// Could be memory-mapped file
		fs::file m_file;

	public:
		file_writer(const std::string& name);

		virtual ~file_writer() = default;

		// Append raw data
		void log(const std::string& text);

		// Get current file size (may be used by secondary readers)
		std::size_t size() const;
	};

	struct file_listener : public file_writer, public listener
	{
		file_listener(const std::string& name)
			: file_writer(name)
			, listener()
		{
		}

		// Encode level, current thread name, channel name and write log message
		virtual void log(const channel& ch, level sev, const std::string& text) override;
	};

	// Global variable for RPCS3.log
	extern file_listener g_log_file;

	// Global variable for TTY.log
	extern file_writer g_tty_file;

	// Small set of predefined channels:

	extern channel GENERAL;
	extern channel LOADER;
	extern channel MEMORY;
	extern channel RSX;
	extern channel HLE;
	extern channel PPU;
	extern channel SPU;
	extern channel ARMv7;
}

// Legacy:

#define LOG_SUCCESS(ch, fmt, ...) _log::ch.success(fmt, ##__VA_ARGS__)
#define LOG_NOTICE(ch, fmt, ...)  _log::ch.notice (fmt, ##__VA_ARGS__)
#define LOG_WARNING(ch, fmt, ...) _log::ch.warning(fmt, ##__VA_ARGS__)
#define LOG_ERROR(ch, fmt, ...)   _log::ch.error  (fmt, ##__VA_ARGS__)
#define LOG_TODO(ch, fmt, ...)    _log::ch.todo   (fmt, ##__VA_ARGS__)
#define LOG_TRACE(ch, fmt, ...)   _log::ch.trace  (fmt, ##__VA_ARGS__)
#define LOG_FATAL(ch, fmt, ...)   _log::ch.fatal  (fmt, ##__VA_ARGS__)
