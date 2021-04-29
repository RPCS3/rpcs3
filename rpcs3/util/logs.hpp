#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <initializer_list>
#include "util/atomic.hpp"
#include "Utilities/StrFmt.h"

namespace logs
{
	enum class level : unsigned
	{
		always, // Highest log severity (cannot be disabled)
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

	struct stored_message
	{
		message m;
		u64 stamp;
		std::string prefix;
		std::string text;
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

		// Special purpose
		void broadcast(const stored_message&) const;
	};

	struct channel
	{
		// Channel prefix (added to every log message)
		const char* const name;

		// The lowest logging level enabled for this channel (used for early filtering)
		atomic_t<level> enabled;

		// Initialize channel
		constexpr channel(const char* name) noexcept
			: name(name)
			, enabled(level::notice)
		{
		}

#define GEN_LOG_METHOD(_sev)\
		const message msg_##_sev{this, level::_sev};\
		template <typename CharT, usz N, typename... Args>\
		void _sev(const CharT(&fmt)[N], const Args&... args)\
		{\
			if (level::_sev <= enabled.observe()) [[unlikely]]\
			{\
				if constexpr (sizeof...(Args) > 0)\
				{\
					static constexpr fmt_type_info type_list[sizeof...(Args) + 1]{fmt_type_info::make<fmt_unveil_t<Args>>()...};\
					msg_##_sev.broadcast(reinterpret_cast<const char*>(fmt), type_list, u64{fmt_unveil<Args>::get(args)}...);\
				}\
				else\
				{\
					msg_##_sev.broadcast(reinterpret_cast<const char*>(fmt), nullptr);\
				}\
			}\
		}\

		GEN_LOG_METHOD(always)
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

	// Log level control: set specific channels to level::fatal
	void set_channel_levels(const std::map<std::string, logs::level>& map);

	// Get all registered log channels
	std::vector<std::string> get_channels();

	// Helper: no additional name specified
	consteval const char* make_channel_name(const char* name, const char* alt = nullptr)
	{
		return alt ? alt : name;
	}

	// Called in main()
	std::unique_ptr<logs::listener> make_file_listener(const std::string& path, u64 max_size);

	// Called in main()
	void set_init(std::initializer_list<stored_message>);
}

#if __cpp_constinit >= 201907
#define LOG_CONSTINIT constinit
#else
#define LOG_CONSTINIT
#endif

#define LOG_CHANNEL(ch, ...) LOG_CONSTINIT inline ::logs::channel ch(::logs::make_channel_name(#ch, ##__VA_ARGS__)); \
	namespace logs { inline ::logs::registerer reg_##ch{ch}; }

LOG_CHANNEL(rsx_log, "RSX");
