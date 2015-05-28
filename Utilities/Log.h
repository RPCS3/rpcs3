#pragma once
#include "Utilities/MTRingbuffer.h"

//#define BUFFERED_LOGGING 1

//first parameter is of type Log::LogType and text is of type std::string

#define LOG_SUCCESS(logType, text, ...)           log_message(logType, Log::LogSeveritySuccess, text, ##__VA_ARGS__)
#define LOG_NOTICE(logType, text, ...)            log_message(logType, Log::LogSeverityNotice,  text, ##__VA_ARGS__) 
#define LOG_WARNING(logType, text, ...)           log_message(logType, Log::LogSeverityWarning, text, ##__VA_ARGS__) 
#define LOG_ERROR(logType, text, ...)             log_message(logType, Log::LogSeverityError,   text, ##__VA_ARGS__)

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
		ARMv7,
		TTY,
	};


	struct LogTypeName
	{
		LogType mType;
		std::string mName;
	};

	//well I'd love make_array() but alas manually counting is not the end of the world
	static const std::array<LogTypeName, 9> gTypeNameTable = { {
			{ GENERAL, "G: " },
			{ LOADER, "LDR: " },
			{ MEMORY, "MEM: " },
			{ RSX, "RSX: " },
			{ HLE, "HLE: " },
			{ PPU, "PPU: " },
			{ SPU, "SPU: " },
			{ ARMv7, "ARM: " },
			{ TTY, "TTY: " }
			} };

	enum LogSeverity : u32
	{
		LogSeverityNotice = 0,
		LogSeverityWarning,
		LogSeveritySuccess,
		LogSeverityError,
	};

	struct LogMessage
	{
		using size_type = u32;
		LogType mType;
		LogSeverity mServerity;
		std::string mText;

		u32 size() const;
		void serialize(char *output) const;
		static LogMessage deserialize(char *input, u32* size_out=nullptr);
	};

	struct LogListener
	{
		virtual ~LogListener() {};
		virtual void log(const LogMessage &msg) = 0;
	};

	struct LogChannel
	{
		LogChannel();
		LogChannel(const std::string& name);
		LogChannel(LogChannel& other) = delete;
		LogChannel& operator = (LogChannel& other) = delete;
		void log(const LogMessage &msg);
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
static struct { inline operator Log::LogType() { return Log::LogType::ARMv7; } } ARMv7;
static struct { inline operator Log::LogType() { return Log::LogType::TTY; } } TTY;

void log_message(Log::LogType type, Log::LogSeverity sev, const char* text);
void log_message(Log::LogType type, Log::LogSeverity sev, std::string text);

template<typename... Args> never_inline void log_message(Log::LogType type, Log::LogSeverity sev, const char* fmt, Args... args) printf_alike(3, 4)
{
	log_message(type, sev, fmt::Format(fmt, fmt::do_unveil(args)...));
}
