#include "stdafx.h"
#include <iostream>
#include <cinttypes>
#include "Thread.h"
#include "File.h"
#include "Log.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace Log;

std::unique_ptr<LogManager> g_log_manager;

u32 LogMessage::size() const
{
	//1 byte for NULL terminator
	return (u32)(sizeof(LogMessage::size_type) + sizeof(LogType) + sizeof(Severity) + sizeof(std::string::value_type) * mText.size() + 1);
}

void LogMessage::serialize(char *output) const
{
	LogMessage::size_type size = this->size();
	memcpy(output, &size, sizeof(LogMessage::size_type));
	output += sizeof(LogMessage::size_type);
	memcpy(output, &mType, sizeof(LogType));
	output += sizeof(LogType);
	memcpy(output, &mServerity, sizeof(Severity));
	output += sizeof(Severity);
	memcpy(output, mText.c_str(), mText.size() );
	output += sizeof(std::string::value_type)*mText.size();
	*output = '\0';

}
LogMessage LogMessage::deserialize(char *input, u32* size_out)
{
	LogMessage msg;
	LogMessage::size_type msgSize = *(reinterpret_cast<LogMessage::size_type*>(input));
	input += sizeof(LogMessage::size_type);
	msg.mType = *(reinterpret_cast<LogType*>(input));
	input += sizeof(LogType);
	msg.mServerity = *(reinterpret_cast<Severity*>(input));
	input += sizeof(Severity);
	if (msgSize > 9000)
	{
		int wtf = 6;
	}
	msg.mText.append(input, msgSize - 1 - sizeof(Severity) - sizeof(LogType));
	if (size_out){(*size_out) = msgSize;}
	return msg;
}



LogChannel::LogChannel() : LogChannel("unknown")
{}

LogChannel::LogChannel(const std::string& name) :
	  name(name)
	, mEnabled(true)
	, mLogLevel(Severity::Warning)
{}

void LogChannel::log(const LogMessage &msg)
{
	std::lock_guard<std::mutex> lock(mListenerLock);
	for (auto &listener : mListeners)
	{
		listener->log(msg);
	}
}

void LogChannel::addListener(std::shared_ptr<LogListener> listener)
{
	std::lock_guard<std::mutex> lock(mListenerLock);
	mListeners.insert(listener);
}
void LogChannel::removeListener(std::shared_ptr<LogListener> listener)
{
	std::lock_guard<std::mutex> lock(mListenerLock);
	mListeners.erase(listener);
}

struct CoutListener : LogListener
{
	void log(const LogMessage &msg) override
	{
		std::cerr << msg.mText << std::endl;
	}
};

struct FileListener : LogListener
{
	fs::file mFile;
	bool mPrependChannelName;

	FileListener(const std::string& name = _PRGNAME_ ".log", bool prependChannel = true)
		: mFile(fs::get_config_dir() + name, fom::rewrite)
		, mPrependChannelName(prependChannel)
	{
		if (!mFile)
		{
#ifdef _WIN32
			MessageBoxA(0, ("Can't create log file: " + name).c_str(), "Error", MB_ICONERROR);
#else
			std::printf("Can't create log file: %s\n", name.c_str());
#endif
		}
	}

	void log(const LogMessage &msg) override
	{
		std::string text = msg.mText;
		if (mPrependChannelName)
		{
			text.insert(0, gTypeNameTable[static_cast<u32>(msg.mType)].mName);
			
			if (msg.mType == Log::TTY)
			{
				text = fmt::escape(text);
				if (text[text.length() - 1] != '\n')
				{
					text += '\n';
				}
			}
		}

		mFile << text;
	}
};

LogManager::LogManager() 
#ifdef BUFFERED_LOGGING
	: mExiting(false), mLogConsumer()
#endif
{
	auto it = mChannels.begin();
	std::shared_ptr<LogListener> listener(new FileListener());
	for (const LogTypeName& name : gTypeNameTable)
	{
		it->name = name.mName;
		it->addListener(listener);
		it++;
	}
	std::shared_ptr<LogListener> TTYListener(new FileListener("TTY.log", false));
	getChannel(TTY).addListener(TTYListener);
#ifdef BUFFERED_LOGGING
	mLogConsumer = std::thread(&LogManager::consumeLog, this);
#endif
}

LogManager::~LogManager()
{
#ifdef BUFFERED_LOGGING
	mExiting = true;
	mBufferReady.notify_all();
	mLogConsumer.join();
}

void LogManager::consumeLog()
{
	std::unique_lock<std::mutex> lock(mStatusMut);
	while (!mExiting)
	{
		mBufferReady.wait(lock);
		mBuffer.lockGet();
		size_t size = mBuffer.size();
		std::vector<char> local_messages(size);
		mBuffer.popN(&local_messages.front(), size);
		mBuffer.unlockGet();

		u32 cursor = 0;
		u32 removed = 0;
		while (cursor < size)
		{
			Log::LogMessage msg = Log::LogMessage::deserialize(local_messages.data() + cursor, &removed);
			cursor += removed;
			getChannel(msg.mType).log(msg);
		}
	}
#endif
}

void LogManager::log(LogMessage msg)
{
	//don't do any formatting changes or filtering to the TTY output since we
	//use the raw output to do diffs with the output of a real PS3 and some
	//programs write text in single bytes to the console
	if (msg.mType != TTY)
	{
		std::string prefix;
		switch (msg.mServerity)
		{
		case Severity::Success:
			prefix = "S ";
			break;
		case Severity::Notice:
			prefix = "! ";
			break;
		case Severity::Warning:
			prefix = "W ";
			break;
		case Severity::Error:
			prefix = "E ";
			break;
		}
		if (auto thr = thread_ctrl::get_current())
		{
			prefix += "{" + thr->get_name() + "} ";
		}
		msg.mText.insert(0, prefix);
		msg.mText.append(1,'\n');
	}
#ifdef BUFFERED_LOGGING
	size_t size = msg.size();
	std::vector<char> temp_buffer(size);
	msg.serialize(temp_buffer.data());
	mBuffer.pushRange(temp_buffer.begin(), temp_buffer.end());
	mBufferReady.notify_one();
#else
	mChannels[static_cast<u32>(msg.mType)].log(msg);
#endif
}

void LogManager::addListener(std::shared_ptr<LogListener> listener)
{
	for (auto& channel : mChannels)
	{
		channel.addListener(listener);
	}
}

void LogManager::removeListener(std::shared_ptr<LogListener> listener)
{
	for (auto& channel : mChannels)
	{
		channel.removeListener(listener);
	}
}

LogManager& LogManager::getInstance()
{
	if (!g_log_manager)
	{
		g_log_manager.reset(new LogManager());
	}

	return *g_log_manager;
}

LogChannel &LogManager::getChannel(LogType type)
{
	return mChannels[static_cast<u32>(type)];
}

void log_message(Log::LogType type, Log::Severity sev, const char* text)
{
	log_message(type, sev, std::string(text));
}

void log_message(Log::LogType type, Log::Severity sev, std::string text)
{
	if (g_log_manager)
	{
		g_log_manager->log({ type, sev, std::move(text) });
	}
	else
	{
		const auto severity =
			sev == Severity::Notice ? "Notice" :
			sev == Severity::Warning ? "Warning" :
			sev == Severity::Success ? "Success" :
			sev == Severity::Error ? "Error" : "Unknown";

#ifdef _WIN32
		MessageBoxA(0, text.c_str(), severity,
			sev == Severity::Notice ? MB_ICONINFORMATION :
			sev == Severity::Warning ? MB_ICONEXCLAMATION :
			sev == Severity::Error ? MB_ICONERROR : MB_ICONINFORMATION);
#else
		std::printf("[Log:%s] %s\n", severity, text.c_str());
#endif
	}
}
