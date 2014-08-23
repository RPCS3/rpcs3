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
	};

	void LogOutput(LogType type, const char* info, const std::string& text);
	void LogOutput(LogType type, const u32 id, const char* info, const std::string& text);

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

	template<typename... Targs> void Notice(const u32 id, const char* fmt, Targs... args)
	{
		LogOutput(LogNotice, id, ": ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Notice(const char* fmt, Targs... args)
	{
		LogOutput(LogNotice, ": ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __forceinline void Log(const char* fmt, Targs... args)
	{
		if (CheckLogging())
		{
			Notice(fmt, args...);
		}
	}

	template<typename... Targs> __forceinline void Log(const u32 id, const char* fmt, Targs... args)
	{
		if (CheckLogging())
		{
			Notice(id, fmt, args...);
		}
	}

	template<typename... Targs> void Success(const u32 id, const char* fmt, Targs... args)
	{
		LogOutput(LogSuccess, id, ": ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Success(const char* fmt, Targs... args)
	{
		LogOutput(LogSuccess, ": ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Warning(const u32 id, const char* fmt, Targs... args)
	{
		LogOutput(LogWarning, id, " warning: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Warning(const char* fmt, Targs... args)
	{
		LogOutput(LogWarning, " warning: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const u32 id, const char* fmt, Targs... args)
	{
		LogOutput(LogError, id, " error: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const char* fmt, Targs... args)
	{
		LogOutput(LogError, " error: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const u32 id, const char* fmt, Targs... args)
	{
		LogOutput(LogError, id, " TODO: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const char* fmt, Targs... args)
	{
		LogOutput(LogError, " TODO: ", fmt::Format(fmt, args...));
	}
};