#pragma once

#include "types.h"
#include "Platform.h"
#include "Atomic.h"
#include "StrFmt.h"

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

	struct channel;

	// Message information (temporary data)
	struct message
	{
		const channel* ch;
		level sev;

		// Send log message to global logger instance
		void broadcast(const char*, const fmt_type_info*, const u64*);
	};

	class listener
	{
		// Next listener (linked list)
		atomic_t<listener*> m_next{};

		friend struct message;

	public:
		constexpr listener() = default;

		virtual ~listener() = default;

		// Process log message
		virtual void log(const message& msg, const std::string& prefix, const std::string& text) = 0;

		// Add new listener
		static void add(listener*);
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
		SAFE_BUFFERS FORCE_INLINE void format(level sev, const char* fmt, const Args&... args) const
		{
			if (UNLIKELY(sev <= enabled))
			{
				message{this, sev}.broadcast(fmt, fmt_type_info::get<fmt_unveil_t<Args>...>(), fmt_args_t<Args...>{fmt_unveil<Args>::get(args)...});
			}
		}

#define GEN_LOG_METHOD(_sev)\
		template<typename... Args>\
		SAFE_BUFFERS void _sev(const char* fmt, const Args&... args) const\
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

// Legacy:

#define LOG_SUCCESS(ch, fmt, ...) logs::ch.success(fmt, ##__VA_ARGS__)
#define LOG_NOTICE(ch, fmt, ...)  logs::ch.notice (fmt, ##__VA_ARGS__)
#define LOG_WARNING(ch, fmt, ...) logs::ch.warning(fmt, ##__VA_ARGS__)
#define LOG_ERROR(ch, fmt, ...)   logs::ch.error  (fmt, ##__VA_ARGS__)
#define LOG_TODO(ch, fmt, ...)    logs::ch.todo   (fmt, ##__VA_ARGS__)
#define LOG_TRACE(ch, fmt, ...)   logs::ch.trace  (fmt, ##__VA_ARGS__)
#define LOG_FATAL(ch, fmt, ...)   logs::ch.fatal  (fmt, ##__VA_ARGS__)
