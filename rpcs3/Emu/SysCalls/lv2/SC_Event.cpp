#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/event.h"

SysCallBase sys_event("sys_event");

//128
int sys_event_queue_create(u32 equeue_id_addr, u32 attr_addr, u64 event_queue_key, int size)
{
	sys_event.Warning("sys_event_queue_create(equeue_id_addr=0x%x, attr_addr=0x%x, event_queue_key=0x%llx, size=%d)",
		equeue_id_addr, attr_addr, event_queue_key, size);

	if(size <= 0 || size > 127)
	{
		return CELL_EINVAL;
	}

	if(!Memory.IsGoodAddr(equeue_id_addr, 4) || !Memory.IsGoodAddr(attr_addr, sizeof(sys_event_queue_attr)))
	{
		return CELL_EFAULT;
	}

	auto& attr = (sys_event_queue_attr&)Memory[attr_addr];
	sys_event.Warning("name = %s", attr.name);
	sys_event.Warning("type = %d", re(attr.type));
	EventQueue* equeue = new EventQueue();
	equeue->size = size;
	equeue->pos = 0;
	equeue->type = re(attr.type);
	strncpy(equeue->name, attr.name, 8);
	Memory.Write32(equeue_id_addr, sys_event.GetNewId(equeue));

	return CELL_OK;
}

int sys_event_queue_receive(u32 equeue_id, u32 event_addr, u32 timeout)
{
	sys_event.Warning("sys_event_queue_receive(equeue_id=0x%x, event_addr=0x%x, timeout=0x%x)",
		equeue_id, event_addr, timeout);

	if(!sys_event.CheckId(equeue_id))
	{
		return CELL_ESRCH;
	}

	PPCThread* thr = GetCurrentPPCThread();

	while(true)
	{
		int status = thr->ThreadStatus();
		if(status == PPCThread_Stopped)
		{
			return CELL_ECANCELED;
		}

		EventQueue* equeue = (EventQueue*)Emu.GetIdManager().GetIDData(equeue_id).m_data;
		for(int i=0; i<equeue->pos; ++i)
		{
			if(!equeue->ports[i]->has_data && equeue->ports[i]->thread)
			{
				SPUThread* thr = (SPUThread*)equeue->ports[i]->thread;
				if(thr->OutIntrMbox.GetCount())
				{
					u32 val;
					thr->OutIntrMbox.Pop(val);
					if(!thr->OutMbox.Pop(val)) val = 0;
					equeue->ports[i]->data1 = val;
					equeue->ports[i]->data2 = 0;
					equeue->ports[i]->data3 = 0;
					equeue->ports[i]->has_data = true;
				}
			}
		}

		bool has_data = false;
		for(int i=0; i<equeue->pos; i++)
		{
			if(equeue->ports[i]->has_data)
			{
				has_data = true;
				auto res = (sys_event_data&)Memory[event_addr];

				re(res.source, equeue->ports[i]->name);
				re(res.data1, equeue->ports[i]->data1);
				re(res.data2, equeue->ports[i]->data2);
				re(res.data3, equeue->ports[i]->data3);

				equeue->ports[i]->has_data = false;
				break;
			}
		}

		if(has_data)
			break;

		Sleep(1);
	}

	return CELL_OK;
}

int sys_event_port_create(u32 eport_id_addr, int port_type, u64 name)
{
	sys_event.Warning("sys_event_port_create(eport_id_addr=0x%x, port_type=0x%x, name=0x%llx)",
		eport_id_addr, port_type, name);

	if(!Memory.IsGoodAddr(eport_id_addr, 4))
	{
		return CELL_EFAULT;
	}

	EventPort* eport = new EventPort();
	u32 id = sys_event.GetNewId(eport);
	eport->has_data = false;
	eport->name = name ? name : id;
	Memory.Write32(eport_id_addr, id);

	return CELL_OK;
}

int sys_event_port_connect_local(u32 event_port_id, u32 event_queue_id)
{
	sys_event.Warning("sys_event_port_connect_local(event_port_id=0x%x, event_queue_id=0x%x)",
		event_port_id, event_queue_id);

	if(!sys_event.CheckId(event_port_id) || !sys_event.CheckId(event_queue_id))
	{
		return CELL_ESRCH;
	}

	EventPort* eport = (EventPort*)Emu.GetIdManager().GetIDData(event_port_id).m_data;
	EventQueue* equeue = (EventQueue*)Emu.GetIdManager().GetIDData(event_queue_id).m_data;
	equeue->ports[equeue->pos++] = eport;

	return CELL_OK;
}