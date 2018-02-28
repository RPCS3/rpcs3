#pragma once

#include "types.h"
#include "Atomic.h"
#include "StrFmt.h"
#include <climits>

namespace logs
{
	enum class level : uint
	{
		always, // Highest log severity (unused, cannot be disabled)
		fatal,
		error,
		todo,
		success,
		warning,
		notice,
		trace, // Lowest severity (usually disabled)

		_uninit = UINT_MAX, // Special value for delayed initialization
	};

	struct channel;

	// Message information (temporary data)
	struct message
	{
		channel* ch;
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

		virtual ~listener();

		// Process log message
		virtual void log(u64 stamp, const message& msg, const std::string& prefix, const std::string& text) = 0;

		// Add new listener
		static void add(listener*);
	};

	struct channel
	{
		// Channel prefix (added to every log message)
		const char* const name;

		// The lowest logging level enabled for this channel (used for early filtering)
		atomic_t<level> enabled;

		// Constant initialization: channel name
		constexpr channel(const char* name)
			: name(name)
			, enabled(level::_uninit)
		{
		}

		// Formatting function
		template<typename... Args>
		SAFE_BUFFERS FORCE_INLINE void format(level sev, const char* fmt, const Args&... args)
		{
			if (UNLIKELY(sev <= enabled))
			{
				message{this, sev}.broadcast(fmt, fmt::get_type_info<fmt_unveil_t<Args>...>(), fmt_args_t<Args...>{fmt_unveil<Args>::get(args)...});
			}
		}

#define GEN_LOG_METHOD(_sev)\
		template<typename... Args>\
		SAFE_BUFFERS void _sev(const char* fmt, const Args&... args)\
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

	// Log level control: set all channels to level::notice
	void reset();

	// Log level control: register channel if necessary, set channel level
	void set_level(const std::string&, level);
}

// Legacy:

#define LOG_SUCCESS(ch, fmt, ...) logs::ch.success("" fmt, ##__VA_ARGS__)
#define LOG_NOTICE(ch, fmt, ...)  logs::ch.notice ("" fmt, ##__VA_ARGS__)
#define LOG_WARNING(ch, fmt, ...) logs::ch.warning("" fmt, ##__VA_ARGS__)
#define LOG_ERROR(ch, fmt, ...)   logs::ch.error  ("" fmt, ##__VA_ARGS__)
#define LOG_TODO(ch, fmt, ...)    logs::ch.todo   ("" fmt, ##__VA_ARGS__)
#define LOG_TRACE(ch, fmt, ...)   logs::ch.trace  ("" fmt, ##__VA_ARGS__)
#define LOG_FATAL(ch, fmt, ...)   logs::ch.fatal  ("" fmt, ##__VA_ARGS__)
