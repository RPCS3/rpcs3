#pragma once

class LogBase
{
	bool m_logging;
	bool CheckLogging() const;

	enum LogType
	{
		LogNotice,
		LogSuccess,
		LogWarning,
		LogError,
		LogFatal,
		LogTodo,
	};

	void LogOutput(LogType type, const std::string& text) const;

	template<typename... Args> never_inline void LogPrepare(LogType type, const char* fmt, Args... args) const
	{
		LogOutput(type, fmt::Format(fmt, args...));
	}

public:
	void SetLogging(bool value)
	{
		m_logging = value;
	}

	LogBase()
	{
		SetLogging(false);
	}

	virtual const std::string& GetName() const = 0;

	template<typename... Args> force_inline printf_alike(2, 3) void Notice(const char* fmt, Args... args) const
	{
		LogPrepare(LogNotice, fmt, fmt::do_unveil(args)...);
	}

	template<typename... Args> force_inline printf_alike(2, 3) void Log(const char* fmt, Args... args) const
	{
		if (CheckLogging())
		{
			Notice(fmt, args...);
		}
	}

	template<typename... Args> force_inline printf_alike(2, 3) void Success(const char* fmt, Args... args) const
	{
		LogPrepare(LogSuccess, fmt, fmt::do_unveil(args)...);
	}

	template<typename... Args> force_inline printf_alike(2, 3) void Warning(const char* fmt, Args... args) const
	{
		LogPrepare(LogWarning, fmt, fmt::do_unveil(args)...);
	}

	template<typename... Args> force_inline printf_alike(2, 3) void Error(const char* fmt, Args... args) const
	{
		LogPrepare(LogError, fmt, fmt::do_unveil(args)...);
	}

	template<typename... Args> force_inline printf_alike(2, 3) void Fatal(const char* fmt, Args... args) const
	{
		LogPrepare(LogFatal, fmt, fmt::do_unveil(args)...);
	}

	template<typename... Args> force_inline printf_alike(2, 3) void Todo(const char* fmt, Args... args) const
	{
		LogPrepare(LogTodo, fmt, fmt::do_unveil(args)...);
	}
};

namespace hle
{
	struct error
	{
		const s32 code;
		const LogBase* const base;
		const std::string text;

		error(s32 errorCode, const char* errorText = nullptr);
		error(s32 errorCode, const LogBase& base, const char* errorText = nullptr);
		error(s32 errorCode, const LogBase* base, const char* errorText = nullptr);

		void print(const char* func = nullptr);
	};
}
