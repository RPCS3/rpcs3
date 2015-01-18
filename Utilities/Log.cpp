#include "stdafx.h"
#include "rPlatform.h"
#include "Log.h"
#include "rMsgBox.h"
#include <iostream>
#include <cinttypes>
#include "Thread.h"
#include "rFile.h"

using namespace Log;

LogManager *gLogManager = nullptr;

u32 LogMessage::size() const
{
	//1 byte for NULL terminator
	return (u32)(sizeof(LogMessage::size_type) + sizeof(LogType) + sizeof(LogSeverity) + sizeof(std::string::value_type) * mText.size() + 1);
}

void LogMessage::serialize(char *output) const
{
	LogMessage::size_type size = this->size();
	memcpy(output, &size, sizeof(LogMessage::size_type));
	output += sizeof(LogMessage::size_type);
	memcpy(output, &mType, sizeof(LogType));
	output += sizeof(LogType);
	memcpy(output, &mServerity, sizeof(LogSeverity));
	output += sizeof(LogSeverity);
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
	msg.mServerity = *(reinterpret_cast<LogSeverity*>(input));
	input += sizeof(LogSeverity);
	if (msgSize > 9000)
	{
		int wtf = 6;
	}
	msg.mText.append(input, msgSize - 1 - sizeof(LogSeverity) - sizeof(LogType));
	if (size_out){(*size_out) = msgSize;}
	return msg;
}



LogChannel::LogChannel() : LogChannel("unknown")
{}

LogChannel::LogChannel(const std::string& name) :
	  name(name)
	, mEnabled(true)
	, mLogLevel(Warning)
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
	void log(const LogMessage &msg)
	{
		std::cerr << msg.mText << std::endl;
	}
};

struct FileListener : LogListener
{
	rFile mFile;
	bool mPrependChannelName;

	FileListener(const std::string& name = _PRGNAME_, bool prependChannel = true)
		: mFile(std::string(rPlatform::getConfigDir() + name + ".log").c_str(), rFile::write),
		mPrependChannelName(prependChannel)
	{
		if (!mFile.IsOpened())
		{
			rMessageBox("Can't create log file! (" + name + ".log)", "Error", rICON_ERROR);
		}
	}

	void log(const LogMessage &msg)
	{
		std::string text = msg.mText;
		if (mPrependChannelName)
		{
			text.insert(0, gTypeNameTable[static_cast<u32>(msg.mType)].mName);

		}
		mFile.Write(text);
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
	std::shared_ptr<LogListener> TTYListener(new FileListener("TTY",false));
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
		case Success:
			prefix = "S ";
			break;
		case Notice:
			prefix = "! ";
			break;
		case Warning:
			prefix = "W ";
			break;
		case Error:
			prefix = "E ";
			break;
		}
		if (NamedThreadBase* thr = GetCurrentNamedThread())
		{
			prefix += "{" + thr->GetThreadName() + "} ";
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
	if (!gLogManager)
	{
		gLogManager = new LogManager();
	}
	return *gLogManager;
}
LogChannel &LogManager::getChannel(LogType type)
{
	return mChannels[static_cast<u32>(type)];
}

void log_message(Log::LogType type, Log::LogSeverity sev, const char* text)
{
	//another msvc bug makes this not work, uncomment this and delete everything else in this function when it's fixed
	//Log::LogManager::getInstance().log({logType, severity, text})

	Log::LogMessage msg{ type, sev, text };
	Log::LogManager::getInstance().log(msg);
}

void log_message(Log::LogType type, Log::LogSeverity sev, const std::string& text)
{
	Log::LogMessage msg{ type, sev, text };
	Log::LogManager::getInstance().log(msg);
}
