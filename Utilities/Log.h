#pragma once

#include "types.h"
#include "Atomic.h"

namespace logs
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
struct bijective<logs::level, const char*>
{
	static constexpr bijective_pair<logs::level, const char*> map[]
	{
		{ logs::level::always, "Nothing" },
		{ logs::level::fatal, "Fatal" },
		{ logs::level::error, "Error" },
		{ logs::level::todo, "TODO" },
		{ logs::level::success, "Success" },
		{ logs::level::warning, "Warning" },
		{ logs::level::notice, "Notice" },
		{ logs::level::trace, "Trace" },
	};
};

// Legacy:

#define LOG_SUCCESS(ch, fmt, ...) logs::ch.success(fmt, ##__VA_ARGS__)
#define LOG_NOTICE(ch, fmt, ...)  logs::ch.notice (fmt, ##__VA_ARGS__)
#define LOG_WARNING(ch, fmt, ...) logs::ch.warning(fmt, ##__VA_ARGS__)
#define LOG_ERROR(ch, fmt, ...)   logs::ch.error  (fmt, ##__VA_ARGS__)
#define LOG_TODO(ch, fmt, ...)    logs::ch.todo   (fmt, ##__VA_ARGS__)
#define LOG_TRACE(ch, fmt, ...)   logs::ch.trace  (fmt, ##__VA_ARGS__)
#define LOG_FATAL(ch, fmt, ...)   logs::ch.fatal  (fmt, ##__VA_ARGS__)
