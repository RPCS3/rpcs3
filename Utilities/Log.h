#pragma once

#include "types.h"
#include "Atomic.h"

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

	struct channel
	{
		// Channel prefix (added to every log message)
		const char* const name;

		// The lowest logging level enabled for this channel (used for early filtering)
		atomic_t<level> enabled;

		// Constant initialization: name and initial log level
		constexpr channel(const char* name, level enabled = level::trace)
			: name(name)
			, enabled(enabled)
		{
		}

		// Formatting function
		template<typename... Args>
		void format(level sev, const char* fmt, const Args&... args) const
		{
#ifdef _MSC_VER
			if (sev <= enabled)
#else
			if (__builtin_expect(sev <= enabled, 0))
#endif
				broadcast(*this, sev, fmt, ::unveil<Args>::get(args)...);
		}

#define GEN_LOG_METHOD(_sev)\
		template<typename... Args>\
		void _sev(const char* fmt, const Args&... args) const\
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

	private:
		// Send log message to global logger instance
		static void broadcast(const channel& ch, level sev, const char* fmt...);
	};

	/* Small set of predefined channels */

	extern channel GENERAL;
	extern channel LOADER;
	extern channel MEMORY;
	extern channel RSX;
	extern channel HLE;
	extern channel PPU;
	extern channel SPU;
	extern channel ARMv7;
}

template<>
struct bijective<_log::level, const char*>
{
	static constexpr bijective_pair<_log::level, const char*> map[]
	{
		{ _log::level::always, "Nothing" },
		{ _log::level::fatal, "Fatal" },
		{ _log::level::error, "Error" },
		{ _log::level::todo, "TODO" },
		{ _log::level::success, "Success" },
		{ _log::level::warning, "Warning" },
		{ _log::level::notice, "Notice" },
		{ _log::level::trace, "Trace" },
	};
};

// Legacy:

#define LOG_SUCCESS(ch, fmt, ...) _log::ch.success(fmt, ##__VA_ARGS__)
#define LOG_NOTICE(ch, fmt, ...)  _log::ch.notice (fmt, ##__VA_ARGS__)
#define LOG_WARNING(ch, fmt, ...) _log::ch.warning(fmt, ##__VA_ARGS__)
#define LOG_ERROR(ch, fmt, ...)   _log::ch.error  (fmt, ##__VA_ARGS__)
#define LOG_TODO(ch, fmt, ...)    _log::ch.todo   (fmt, ##__VA_ARGS__)
#define LOG_TRACE(ch, fmt, ...)   _log::ch.trace  (fmt, ##__VA_ARGS__)
#define LOG_FATAL(ch, fmt, ...)   _log::ch.fatal  (fmt, ##__VA_ARGS__)
