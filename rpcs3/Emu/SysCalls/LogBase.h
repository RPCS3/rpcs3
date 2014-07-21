#pragma once

class LogBase
{
	bool m_logging;

public:
	LogBase()
		: m_logging(false)
	{
	}

	void SetLogging(bool value)
	{
		m_logging = value;
	}

	virtual const std::string& GetName() const = 0;

	template<typename... Targs> void Notice(const u32 id, const char* fmt, Targs... args)
	{
		LOG_NOTICE(HLE, GetName() + fmt::Format("[%d]: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Notice(const char* fmt, Targs... args)
	{
		LOG_NOTICE(HLE, GetName() + ": " + fmt::Format(fmt, args...));
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
		LOG_WARNING(HLE, GetName() + fmt::Format("[%d] warning: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Warning(const char* fmt, Targs... args)
	{
		LOG_WARNING(HLE, GetName() + " warning: " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const u32 id, const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + fmt::Format("[%d] error: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Error(const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + " error: " + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const u32 id, const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + fmt::Format("[%d] TODO: ", id) + fmt::Format(fmt, args...));
	}

	template<typename... Targs> void Todo(const char* fmt, Targs... args)
	{
		LOG_ERROR(HLE, GetName() + " TODO: " + fmt::Format(fmt, args...));
	}
};