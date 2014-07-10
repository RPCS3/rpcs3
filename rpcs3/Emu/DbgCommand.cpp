#include "stdafx.h"
#include "rpcs3.h"

void SendDbgCommand(DbgCommand id, CPUThread* thr )
{
	wxGetApp().SendDbgCommand(id, thr);
}
