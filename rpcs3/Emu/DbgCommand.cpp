#include "stdafx.h"

void SendDbgCommand(DbgCommand id, CPUThread* thr )
{
	wxGetApp().SendDbgCommand(id, thr);
}