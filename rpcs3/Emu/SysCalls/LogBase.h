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

	void LogOutput(LogType type, const char* info, const std::string& text) const;
	void LogOutput(LogType type, const u32 id, const char* info, const std::string& text) const;

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

	template<typename... Targs> __noinline void Notice(const u32 id, const char* fmt, Targs... args) const
	{
		LogOutput(LogNotice, id, " : ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __noinline void Notice(const char* fmt, Targs... args) const
	{
		LogOutput(LogNotice, ": ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __forceinline void Log(const char* fmt, Targs... args) const
	{
		if (CheckLogging())
		{
			Notice(fmt, args...);
		}
	}

	template<typename... Targs> __forceinline void Log(const u32 id, const char* fmt, Targs... args) const
	{
		if (CheckLogging())
		{
			Notice(id, fmt, args...);
		}
	}

	template<typename... Targs> __noinline void Success(const u32 id, const char* fmt, Targs... args) const
	{
		LogOutput(LogSuccess, id, " : ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __noinline void Success(const char* fmt, Targs... args) const
	{
		LogOutput(LogSuccess, ": ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __noinline void Warning(const u32 id, const char* fmt, Targs... args) const
	{
		LogOutput(LogWarning, id, " warning: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __noinline void Warning(const char* fmt, Targs... args) const
	{
		LogOutput(LogWarning, " warning: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __noinline void Error(const u32 id, const char* fmt, Targs... args) const
	{
		LogOutput(LogError, id, " error: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __noinline void Error(const char* fmt, Targs... args) const
	{
		LogOutput(LogError, " error: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __noinline void Todo(const u32 id, const char* fmt, Targs... args) const
	{
		LogOutput(LogError, id, " TODO: ", fmt::Format(fmt, args...));
	}

	template<typename... Targs> __noinline void Todo(const char* fmt, Targs... args) const
	{
		LogOutput(LogError, " TODO: ", fmt::Format(fmt, args...));
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
