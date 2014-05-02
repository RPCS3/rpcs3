#include "stdafx.h"
#include "Emu/ConLog.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "DbgConsole.h"

LogWriter ConLog;
class LogFrame;
extern LogFrame* ConLogFrame;

_LogBuffer LogBuffer;

std::mutex g_cs_conlog;

const uint max_item_count = 500;
const uint buffer_size = 1024 * 64;

static const std::string g_log_colors[] =
{
	"Black", "Green", "White", "Yellow", "Red",
};

LogWriter::LogWriter()
{
	if (!m_logfile.Open(_PRGNAME_ ".log", rFile::write))
	{
		rMessageBox("Can't create log file! (" _PRGNAME_ ".log)", rMessageBoxCaptionStr, rICON_ERROR);
	}
}

void LogWriter::WriteToLog(const std::string& prefix, const std::string& value, u8 lvl/*, wxColour bgcolour*/)
{
	std::string new_prefix = prefix;
	if (!prefix.empty())
	{
		if (NamedThreadBase* thr = GetCurrentNamedThread())
		{
			new_prefix += " : " + thr->GetThreadName();
		}
	}

	if (m_logfile.IsOpened() && !new_prefix.empty())
		m_logfile.Write("[" + new_prefix + "]: " + value + "\n");

	if (!ConLogFrame || Ini.HLELogLvl.GetValue() == 4 || (lvl != 0 && lvl <= Ini.HLELogLvl.GetValue()))
		return;

	std::lock_guard<std::mutex> lock(g_cs_conlog);

	// TODO: Use ThreadBase instead, track main thread id
	if (rThread::IsMain())
	{
		while (LogBuffer.IsBusy())
		{
			// need extra break condition?
			rYieldIfNeeded();
		}
	}
	else
	{
		while (LogBuffer.IsBusy())
		{
			if (Emu.IsStopped())
			{
				break;
			}
			Sleep(1);
		}
	}

	//if(LogBuffer.put == LogBuffer.get) LogBuffer.Flush();

	LogBuffer.Push(LogPacket(new_prefix, value, g_log_colors[lvl]));
}


void LogWriter::SkipLn()
{
	WriteToLog("", "", 0);
}

void DbgConsole::Close()
{
	i = 1;
}

void DbgConsole::Clear()
{
	i = 2;
}

void DbgConsole::Write(int ch, const std::string &msg)
{
}
