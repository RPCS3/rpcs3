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
	enum class level : unsigned char
	{
		always = 0, // Highest log severity (cannot be disabled)
		fatal = 1,
		error = 2,
		todo = 3,
		success = 4,
		warning = 5,
		notice = 6,
		trace = 7, // Lowest severity (usually disabled)
	};

	struct channel;

	// Message information
	struct message
	{
		// Default constructor
		consteval message() = default;

		// Cannot be moved because it relies on its location
		message(const message&) = delete;

		message& operator =(const message&) = delete;

		// Send log message to the given channel with severity
		template <typename... Args>
		void operator()(const const_str& fmt, const Args&... args) const;

		operator level() const
		{
			return level(uchar(reinterpret_cast<uptr>(this) & 7));
		}

		const channel* operator->() const
		{
			return reinterpret_cast<const channel*>(reinterpret_cast<uptr>(this) & -16);
		}

		inline explicit operator bool() const;

	private:
		// Send log message to global logger instance
		void broadcast(const char*, const fmt_type_info*, ...) const;

		friend struct channel;
	};

	struct stored_message
	{
		const message& m;
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
		virtual void log(u64 stamp, const message& msg, std::string_view prefix, std::string_view text) = 0;

		// Flush contents (file writer)
		virtual void sync();

		// Close file handle after flushing to disk (hazardous)
		virtual void close_prematurely();

		// Add new listener
		static void add(listener*);

		// Special purpose
		void broadcast(const stored_message&) const;

		// Flush log to disk
		static void sync_all();

		// Close file handle after flushing to disk (hazardous)
		static void close_all_prematurely();
	};

	struct alignas(16) channel : private message
	{
		// Channel prefix (added to every log message)
		const char* const name;

		// The lowest logging level enabled for this channel (used for early filtering)
		atomic_t<level> enabled;

		// Initialize channel
		consteval channel(const char* name) noexcept
			: message{}
			, name(name)
			, enabled(level::notice)
		{
		}

		// Special access to "always visible" channel which shouldn't be used
		const message& always() const
		{
			return *this;
		}

#define GEN_LOG_METHOD(_sev)\
		const message _sev{};\

		GEN_LOG_METHOD(fatal)
		GEN_LOG_METHOD(error)
		GEN_LOG_METHOD(todo)
		GEN_LOG_METHOD(success)
		GEN_LOG_METHOD(warning)
		GEN_LOG_METHOD(notice)
		GEN_LOG_METHOD(trace)

#undef GEN_LOG_METHOD
	};

	inline logs::message::operator bool() const
	{
		// Test if enabled
		return *this <= (*this)->enabled.observe();
	}

	template <typename... Args>
	FORCE_INLINE SAFE_BUFFERS(void) message::operator()(const const_str& fmt, const Args&... args) const
	{
		if (operator bool()) [[unlikely]]
		{
			if constexpr (sizeof...(Args) > 0)
			{
				broadcast(fmt, fmt::type_info_v<Args...>, u64{fmt_unveil<Args>::get(args)}...);
			}
			else
			{
				broadcast(fmt, nullptr);
			}
		}
	}

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
	void set_channel_levels(const std::map<std::string, logs::level, std::less<>>& map);

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

#define LOG_CHANNEL(ch, ...) inline constinit ::logs::channel ch(::logs::make_channel_name(#ch, ##__VA_ARGS__)); \
	namespace logs { inline ::logs::registerer reg_##ch{ch}; }

LOG_CHANNEL(rsx_log, "RSX");
