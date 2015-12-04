#pragma once
#include <string>
#include <rpcs3/core/platform.h>

namespace rpcs3::syscalls
{
	class log_base
	{
		bool m_logging;
		bool check_logging() const;

		enum class log_type
		{
			notice,
			success,
			warning,
			error,
			todo,
		};

		void log_output(log_type type, std::string text) const;

		template<typename... Args> never_inline safe_buffers void log_prepare(log_type type, const char* fmt, Args... args) const
		{
			log_output(type, fmt::format(fmt, args...));
		}

		never_inline safe_buffers void log_prepare(log_type type, const char* fmt) const
		{
			log_output(type, fmt);
		}

	public:
		void set_logging(bool value)
		{
			m_logging = value;
		}

		log_base()
		{
			set_logging(false);
		}

		virtual const std::string& name() const = 0;

		template<typename... Args> force_inline void notice(const char* fmt, Args... args) const
		{
			log_prepare(log_type::notice, fmt, fmt::do_unveil(args)...);
		}

		template<typename... Args> force_inline void log(const char* fmt, Args... args) const
		{
			if (check_logging())
			{
				notice(fmt, args...);
			}
		}

		template<typename... Args> force_inline void success(const char* fmt, Args... args) const
		{
			log_prepare(log_type::success, fmt, fmt::do_unveil(args)...);
		}

		template<typename... Args> force_inline void warning(const char* fmt, Args... args) const
		{
			log_prepare(log_type::warning, fmt, fmt::do_unveil(args)...);
		}

		template<typename... Args> force_inline void error(const char* fmt, Args... args) const
		{
			log_prepare(log_type::error, fmt, fmt::do_unveil(args)...);
		}

		template<typename... Args> force_inline void todo(const char* fmt, Args... args) const
		{
			log_prepare(log_type::todo, fmt, fmt::do_unveil(args)...);
		}
	};
}