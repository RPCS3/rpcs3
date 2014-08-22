#pragma once

class LogBase
{
	bool m_logging;

	void LogNotice(const std::string& text);
	void LogWarning(const std::string& text);
	void LogError(const std::string& text);

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
		LogNotice(GetName() + fmt::Format("[%d]: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Notice(const char* fmt, Targs... args)
	{
		LogNotice(GetName() + ": " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> __forceinline void Log(const char* fmt, Targs... args)
	{
		if (m_logging)
		{
			Notice(fmt, args...);
		}
	}

	template<typename... Targs> __forceinline void Log(const u32 id, const char* fmt, Targs... args)
	{
		if (m_logging)
		{
			Notice(id, fmt, args...);
		}
	}

	template<typename... Targs> void Warning(const u32 id, const char* fmt, Targs... args)
	{
		LogWarning(GetName() + fmt::Format("[%d] warning: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Warning(const char* fmt, Targs... args)
	{
		LogWarning(GetName() + " warning: " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const u32 id, const char* fmt, Targs... args)
	{
		LogError(GetName() + fmt::Format("[%d] error: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const char* fmt, Targs... args)
	{
		LogError(GetName() + " error: " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const u32 id, const char* fmt, Targs... args)
	{
		LogError(GetName() + fmt::Format("[%d] TODO: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const char* fmt, Targs... args)
	{
		LogError(GetName() + " TODO: " + fmt::Format(fmt, args...));
	}
};