#include "stdafx.h"
#include "DbgCommand.h"

SendDbgCommandCb SendDbgCommandFunc = nullptr;

void SendDbgCommand(DbgCommand id, CPUThread* t)
{
	SendDbgCommandFunc(id, t);
}

void SetSendDbgCommandCallback(SendDbgCommandCb cb)
{
	SendDbgCommandFunc = cb;
}