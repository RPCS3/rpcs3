#pragma once
#include <common/safe_ring_buffer.h>
#include <common/basic_types.h>
#include <common/fmt.h>

#include <array>
#include <set>
#include <atomic>


#define BUFFERED_LOGGING 1

//first parameter is of type Log::LogType and text is of type std::string

#define LOG_SUCCESS(logType, text, ...)     ::rpcs3::core::log_message(::rpcs3::core::logType, ::rpcs3::core::log::severity::success, text, ##__VA_ARGS__)
#define LOG_NOTICE(logType, text, ...)      ::rpcs3::core::log_message(::rpcs3::core::logType, ::rpcs3::core::log::severity::notice,  text, ##__VA_ARGS__) 
#define LOG_WARNING(logType, text, ...)     ::rpcs3::core::log_message(::rpcs3::core::logType, ::rpcs3::core::log::severity::warning, text, ##__VA_ARGS__) 
#define LOG_ERROR(logType, text, ...)       ::rpcs3::core::log_message(::rpcs3::core::logType, ::rpcs3::core::log::severity::error,   text, ##__VA_ARGS__)

namespace rpcs3
{
	using namespace common;

	inline namespace core
	{

		namespace log
		{
			static constexpr unsigned int g_max_buffer_size = 1024 * 1024;
			static constexpr unsigned int g_buffer_size = 1000;

			enum type : u32
			{
				general = 0,
				loader,
				memory,
				rsx,
				hle,
				ppu,
				spu,
				armv7,
				tty,
			};

			struct type_name
			{
				type type;
				std::string name;
			};

			//well I'd love make_array() but alas manually counting is not the end of the world
			static const std::array<type_name, 9> g_type_name_table = { {
					{ type::general, "G: " },
					{ type::loader, "LDR: " },
					{ type::memory, "MEM: " },
					{ type::rsx, "RSX: " },
					{ type::hle, "HLE: " },
					{ type::ppu, "PPU: " },
					{ type::spu, "SPU: " },
					{ type::armv7, "ARM: " },
					{ type::tty, "TTY: " }
					} };

			enum class severity : u32
			{
				notice = 0,
				warning,
				success,
				error,
			};

			struct message
			{
				using size_type = u32;
				type type;
				severity serverity;
				std::string text;

				u32 size() const;
				void serialize(char *output) const;
				static message deserialize(char *input, u32* size_out = nullptr);
			};

			struct listener
			{
				virtual ~listener() {};
				virtual void log(const message &msg) = 0;
			};

			struct channel
			{
				channel();
				channel(const std::string& name);
				channel(channel& other) = delete;
				void log(const message &msg);
				void add_listener(std::shared_ptr<listener> listener);
				void remove_listener(std::shared_ptr<listener> listener);
				std::string name;

			private:
				bool m_enabled;
				severity m_log_level;
				std::mutex m_mtx_listener;
				std::set<std::shared_ptr<listener>> m_listeners;
			};

			struct manager
			{
				manager();
				~manager();
				static manager& instance();
				log::channel& channel(type type);

				void log(message msg);
				void add_listener(std::shared_ptr<listener> listener);
				void remove_listener(std::shared_ptr<listener> listener);
#ifdef BUFFERED_LOGGING
				void consume_log();
#endif
			private:
#ifdef BUFFERED_LOGGING
				safe_ring_buffer<char, g_max_buffer_size> m_buffer;
				std::condition_variable m_buffer_ready;
				std::mutex m_mtx_status;
				std::atomic<bool> m_exiting;
				std::thread m_log_consumer;
#endif
				std::array<log::channel, std::tuple_size<decltype(g_type_name_table)>::value> m_channels;
				//std::array<log::channel,g_type_name_table.size()> m_channels; //TODO: use this once Microsoft sorts their shit out
			};
		}

		static struct { inline operator log::type() { return log::type::general; } } general;
		static struct { inline operator log::type() { return log::type::loader; } } loader;
		static struct { inline operator log::type() { return log::type::memory; } } memory;
		static struct { inline operator log::type() { return log::type::rsx; } } rsx;
		static struct { inline operator log::type() { return log::type::hle; } } hle;
		static struct { inline operator log::type() { return log::type::ppu; } } ppu;
		static struct { inline operator log::type() { return log::type::spu; } } spu;
		static struct { inline operator log::type() { return log::type::armv7; } } armv7;
		static struct { inline operator log::type() { return log::type::tty; } } tty;

		void log_message(log::type type, log::severity sev, const char* text);
		void log_message(log::type type, log::severity sev, std::string text);

		template<typename... Args> never_inline void log_message(log::type type, log::severity sev, const char* fmt, Args... args)
		{
			log_message(type, sev, fmt::format(fmt, fmt::do_unveil(args)...));
		}
	}
}