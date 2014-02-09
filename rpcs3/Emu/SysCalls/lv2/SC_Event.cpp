#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/event.h"

SysCallBase sys_event("sys_event");

//128
int sys_event_queue_create(mem32_t equeue_id, mem_ptr_t<sys_event_queue_attr> attr, u64 event_queue_key, int size)
{
	sys_event.Warning("sys_event_queue_create(equeue_id_addr=0x%x, attr_addr=0x%x, event_queue_key=0x%llx, size=%d)",
		equeue_id.GetAddr(), attr.GetAddr(), event_queue_key, size);

	if(size <= 0 || size > 127)
	{
		return CELL_EINVAL;
	}

	if(!equeue_id.IsGood() || !attr.IsGood())
	{
		return CELL_EFAULT;
	}

	equeue_id = sys_event.GetNewId(new EventQueue((u32)attr->protocol, (int)attr->type, attr->name_u64, event_queue_key, size));
	sys_event.Warning("*** event_queue created[%s] (protocol=0x%x, type=0x%x): id = %d",
		attr->name, (u32)attr->protocol, (int)attr->type, equeue_id.GetValue());

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

	int result;
	auto queue_receive = [&](int status) -> bool
	{
		if(status == CPUThread_Stopped)
		{
			result = CELL_ECANCELED;
			return false;
		}

		EventQueue* equeue;
		Emu.GetIdManager().GetIDData(equeue_id, equeue);
		for(int i=0; i<equeue->pos; ++i)
		{
			if(!equeue->ports[i]->has_data && equeue->ports[i]->thread)
			{
				SPUThread* thr = (SPUThread*)equeue->ports[i]->thread;
				if(thr->SPU.OutIntr_Mbox.GetCount())
				{
					u32 val;
					thr->SPU.OutIntr_Mbox.Pop(val);
					if(!thr->SPU.Out_MBox.Pop(val)) val = 0;
					equeue->ports[i]->data1 = val;
					equeue->ports[i]->data2 = 0;
					equeue->ports[i]->data3 = 0;
					equeue->ports[i]->has_data = true;
				}
			}
		}

		for(int i=0; i<equeue->pos; i++)
		{
			if(equeue->ports[i]->has_data)
			{
				auto dst = (sys_event_data&)Memory[event_addr];

				dst.source = equeue->ports[i]->name;
				dst.data1 = equeue->ports[i]->data1;
				dst.data2 = equeue->ports[i]->data2;
				dst.data3 = equeue->ports[i]->data3;

				equeue->ports[i]->has_data = false;

				result = CELL_OK;
				return false;
			}
		}

		return true;
	};

	GetCurrentPPUThread().WaitFor(queue_receive);

	return result;
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
	eport->pos = 0;
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

	EventPort* eport;
	EventQueue* equeue;
	Emu.GetIdManager().GetIDData(event_port_id, eport);
	Emu.GetIdManager().GetIDData(event_queue_id, equeue);
	equeue->ports[equeue->pos++] = eport;
	eport->queue[eport->pos++] = equeue;

	return CELL_OK;
}

int sys_event_port_send(u32 event_port_id, u64 data1, u64 data2, u64 data3)
{
	sys_event.Warning("sys_event_port_send(event_port_id=0x%x, data1=0x%llx, data2=0x%llx, data3=0x%llx)",
		event_port_id, data1, data2, data3);

	if(!sys_event.CheckId(event_port_id))
	{
		return CELL_ESRCH;
	}

	EventPort* eport;
	Emu.GetIdManager().GetIDData(event_port_id, eport);
	
	if(!eport->pos)
	{
		return CELL_ENOTCONN;
	}

	if(eport->has_data)
	{
		return CELL_EBUSY;
	}
	
	eport->has_data = true;
	eport->data1 = data1;
	eport->data2 = data2;
	eport->data3 = data3;

	return CELL_OK;
}

int sys_event_queue_drain(u32 event_queue_id)
{
	sys_event.Error("sys_event_queue_drain(event_queue_id=0x%x)", event_queue_id);

	return CELL_OK;
}