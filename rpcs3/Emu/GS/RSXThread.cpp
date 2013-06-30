#include "stdafx.h"
#include "RSXThread.h"

RSXThread::RSXThread(CellGcmControl* ctrl, u32 ioAddress)
	: m_ctrl(*ctrl)
	, m_ioAddress(ioAddress)
{
}

void RSXThread::Task()
{
}

void RSXThread::OnExit()
{
	call_stack.Clear();
}