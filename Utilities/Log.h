#pragma once
#include "Utilities/MTRingbuffer.h"

//#define BUFFERED_LOGGING 1

//first parameter is of type Log::LogType and text is of type std::string

#define LOG_SUCCESS(logType, text, ...)           log_message(logType, Log::Success, text, ##__VA_ARGS__)
#define LOG_NOTICE(logType, text, ...)            log_message(logType, Log::Notice,  text, ##__VA_ARGS__) 
#define LOG_WARNING(logType, text, ...)           log_message(logType, Log::Warning, text, ##__VA_ARGS__) 
#define LOG_ERROR(logType, text, ...)             log_message(logType, Log::Error,   text, ##__VA_ARGS__)

namespace Log
{
	const unsigned int MAX_LOG_BUFFER_LENGTH = 1024*1024;
	const unsigned int gBuffSize = 1000;

	enum LogType : u32
	{
		GENERAL = 0,
		LOADER,
		MEMORY,
		RSX,
		HLE,
		PPU,
		SPU,
		TTY,
	};


	struct LogTypeName
	{
		LogType mType;
		std::string mName;
	};

	//well I'd love make_array() but alas manually counting is not the end of the world
	static const std::array<LogTypeName, 8> gTypeNameTable = { {
			{ GENERAL, "G: " },
			{ LOADER, "LDR: " },
			{ MEMORY, "MEM: " },
			{ RSX, "RSX: " },
			{ HLE, "HLE: " },
			{ PPU, "PPU: " },
			{ SPU, "SPU: " },
			{ TTY, "TTY: " }
			} };

	enum LogSeverity : u32
	{
		Success = 0,
		Notice,
		Warning,
		Error,
	};

	struct LogMessage
	{
		using size_type = u32;
		LogType mType;
		LogSeverity mServerity;
		std::string mText;

		u32 size();
		void serialize(char *output);
		static LogMessage deserialize(char *input, u32* size_out=nullptr);
	};

	struct LogListener
	{
		virtual ~LogListener() {};
		virtual void log(LogMessage msg) = 0;
	};

	struct LogChannel
	{
		LogChannel();
		LogChannel(const std::string& name);
		LogChannel(LogChannel& other) = delete;
		LogChannel& operator = (LogChannel& other) = delete;
		void log(LogMessage msg);
		void addListener(std::shared_ptr<LogListener> listener);
		void removeListener(std::shared_ptr<LogListener> listener);
		std::string name;
	private:
		bool mEnabled;
		LogSeverity mLogLevel;
		std::mutex mListenerLock;
		std::set<std::shared_ptr<LogListener>> mListeners;
	};

	struct LogManager
	{
		LogManager();
		~LogManager();
		static LogManager& getInstance();
		LogChannel& getChannel(LogType type);
		void log(LogMessage msg);
		void addListener(std::shared_ptr<LogListener> listener);
		void removeListener(std::shared_ptr<LogListener> listener);
#ifdef BUFFERED_LOGGING
		void consumeLog();
#endif
	private:
#ifdef BUFFERED_LOGGING
		MTRingbuffer<char, MAX_LOG_BUFFER_LENGTH> mBuffer;
		std::condition_variable mBufferReady;
		std::mutex mStatusMut;
		std::atomic<bool> mExiting;
		std::thread mLogConsumer;
#endif
		std::array<LogChannel, std::tuple_size<decltype(gTypeNameTable)>::value> mChannels;
		//std::array<LogChannel,gTypeNameTable.size()> mChannels; //TODO: use this once Microsoft sorts their shit out
	};
}

static struct { inline operator Log::LogType() { return Log::LogType::GENERAL; } } GENERAL;
static struct { inline operator Log::LogType() { return Log::LogType::LOADER; } } LOADER;
static struct { inline operator Log::LogType() { return Log::LogType::MEMORY; } } MEMORY;
static struct { inline operator Log::LogType() { return Log::LogType::RSX; } } RSX;
static struct { inline operator Log::LogType() { return Log::LogType::HLE; } } HLE;
static struct { inline operator Log::LogType() { return Log::LogType::PPU; } } PPU;
static struct { inline operator Log::LogType() { return Log::LogType::SPU; } } SPU;
static struct { inline operator Log::LogType() { return Log::LogType::TTY; } } TTY;

inline void log_message(Log::LogType type, Log::LogSeverity sev, const char* text)
{
	//another msvc bug makes this not work, uncomment this and delete everything else in this function when it's fixed
	//Log::LogManager::getInstance().log({logType, severity, text})

	Log::LogMessage msg{type, sev, text};
	Log::LogManager::getInstance().log(msg);
}

template<typename T, typename ...Ts> 
inline void log_message(Log::LogType type, Log::LogSeverity sev, const char* text, T arg, Ts... args)
{
	Log::LogMessage msg{type, sev, fmt::Format(text, arg, args...)};
	Log::LogManager::getInstance().log(msg);
}