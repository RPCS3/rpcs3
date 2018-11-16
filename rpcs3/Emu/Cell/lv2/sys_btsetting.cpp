#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/vm.h"
#include "Emu/IdManager.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/lv2/sys_event.h"

#include "sys_btsetting.h"


LOG_CHANNEL(sys_btsetting);

error_code sys_btsetting_if(u64 cmd, vm::ptr<void> msg)
{
	sys_btsetting.todo("sys_btsetting_if(cmd=0x%llx, msg=*0%x)", cmd, msg);

	if (cmd == 0)
	{
		// init

		auto packet = vm::static_ptr_cast<BtInitPacket>(msg);

		sys_btsetting.todo("init page_proc_addr =0x%x", packet->page_proc_addr);
		// second arg controls message
		// lets try just disabling for now
		if (auto q = idm::get<lv2_obj, lv2_event_queue>(packet->equeue_id))
		{
			q->send(0, 0, 0xDEAD, 0);
		}
	}
	//else
		//fmt::throw_exception("unhandled btpckt");

	return CELL_OK;
}
