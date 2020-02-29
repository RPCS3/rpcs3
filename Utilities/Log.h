#pragma once

#include "types.h"
#include "StrFmt.h"
#include "util/atomic.hpp"
#include <atomic>

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
	};

	struct channel;

	// Message information
	struct message
	{
		channel* ch;
		level sev;

	private:
		// Send log message to global logger instance
		void broadcast(const char*, const fmt_type_info*, ...) const;

		friend struct channel;
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
		std::atomic<level> enabled;

		// Initialize channel
		constexpr channel(const char* name) noexcept
			: name(name)
			, enabled(level::notice)
		{
		}

	private:
#if __cpp_char8_t >= 201811
		using char2 = char8_t;
#else
		using char2 = uchar;
#endif

#define GEN_LOG_METHOD(_sev)\
		const message msg_##_sev{this, level::_sev};\
		template <std::size_t N, typename... Args>\
		void _sev(const char(&fmt)[N], const Args&... args)\
		{\
			if (level::_sev <= enabled.load(std::memory_order_relaxed)) [[unlikely]]\
			{\
				static constexpr fmt_type_info type_list[sizeof...(Args) + 1]{fmt_type_info::make<fmt_unveil_t<Args>>()...};\
				msg_##_sev.broadcast(fmt, type_list, u64{fmt_unveil<Args>::get(args)}...);\
			}\
		}\
		template <std::size_t N, typename... Args>\
		void _sev(const char2(&fmt)[N], const Args&... args)\
		{\
			if (level::_sev <= enabled.load(std::memory_order_relaxed)) [[unlikely]]\
			{\
				static constexpr fmt_type_info type_list[sizeof...(Args) + 1]{fmt_type_info::make<fmt_unveil_t<Args>>()...};\
				msg_##_sev.broadcast(reinterpret_cast<const char*>(+fmt), type_list, u64{fmt_unveil<Args>::get(args)}...);\
			}\
		}\

	public:
		GEN_LOG_METHOD(fatal)
		GEN_LOG_METHOD(error)
		GEN_LOG_METHOD(todo)
		GEN_LOG_METHOD(success)
		GEN_LOG_METHOD(warning)
		GEN_LOG_METHOD(notice)
		GEN_LOG_METHOD(trace)

#undef GEN_LOG_METHOD
	};

	struct registerer
	{
		registerer(channel& _ch);
	};

	// Log level control: set all channels to level::notice
	void reset();

	// Log level control: set all channels to level::always
	void silence();

	// Log level control: register channel if necessary, set channel level
	void set_level(const std::string&, level);

	// Log level control: get channel level
	level get_level(const std::string&);

	// Get all registered log channels
	std::vector<std::string> get_channels();

	// Helper: no additional name specified
	constexpr const char* make_channel_name(const char* name)
	{
		return name;
	}

	// Helper: special channel name specified
	constexpr const char* make_channel_name(const char*, const char* name, ...)
	{
		return name;
	}
}

#if __cpp_constinit >= 201907
#define LOG_CONSTINIT constinit
#else
#define LOG_CONSTINIT
#endif

#define LOG_CHANNEL(ch, ...) LOG_CONSTINIT inline ::logs::channel ch(::logs::make_channel_name(#ch, ##__VA_ARGS__)); \
	namespace logs { inline ::logs::registerer reg_##ch{ch}; }

LOG_CHANNEL(rsx_log, "RSX");
