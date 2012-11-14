#include "stdafx.h"
#include "RSXThread.h"

enum MethodFlag
{
	CELL_GCM_METHOD_FLAG_NON_INCREMENT	= 0x40000000,
	CELL_GCM_METHOD_FLAG_JUMP			= 0x20000000,
	CELL_GCM_METHOD_FLAG_CALL			= 0x00000002,
	CELL_GCM_METHOD_FLAG_RETURN			= 0x00020000,
};

RSXThread::RSXThread(
		CellGcmControl* ctrl,
		u32 ioAddress,
		void (&cmd)(const u32 fcmd, const u32 cmd, mem32_t& args, const u32 count),
		bool (&_flip)(),
		bool (&_TestExit)()
		)
	: m_ctrl(*ctrl)
	, m_ioAddress(ioAddress)
	, DoCmd(cmd)
	, flip(_flip)
	, TestExit(_TestExit)
{
}

void RSXThread::Task()
{
	while(!TestDestroy() && !TestExit())
	{
		if(m_ctrl.get == m_ctrl.put)
		{
			flip();

			Sleep(1);
			continue;
		}

		const u32 get = re(m_ctrl.get);
		const u32 cmd = Memory.Read32(m_ioAddress + get);
		const u32 count = (cmd >> 18) & 0x7ff;
		mem32_t data(m_ioAddress + get + 4);

		if(cmd & CELL_GCM_METHOD_FLAG_JUMP)
		{
			m_ctrl.get = re32(cmd & ~(CELL_GCM_METHOD_FLAG_JUMP | CELL_GCM_METHOD_FLAG_NON_INCREMENT));
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_CALL)
		{
			call_stack.AddCpy(get + 4);
			m_ctrl.get = re32(cmd & ~CELL_GCM_METHOD_FLAG_CALL);
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_RETURN)
		{
			const u32 pos = call_stack.GetCount() - 1;
			m_ctrl.get = re32(call_stack[pos]);
			call_stack.RemoveAt(pos);
			continue;
		}
		if(cmd & CELL_GCM_METHOD_FLAG_NON_INCREMENT)
		{
			//ConLog.Warning("non increment cmd! 0x%x", cmd);
		}

#if	0
		wxString debug = getMethodName(cmd & 0x3ffff);
		debug += "(";
		for(u32 i=0; i<count; ++i) debug += (i ? ", " : "") + wxString::Format("0x%x", data[i]);
		debug += ")";
		ConLog.Write(debug);
#endif
		DoCmd(cmd, cmd & 0x3ffff, data, count);

		m_ctrl.get = re32(get + (count + 1) * 4);
		memset(Memory.GetMemFromAddr(m_ioAddress + get), 0, (count + 1) * 4);
	}
}

void RSXThread::OnExit()
{
	call_stack.Clear();
}