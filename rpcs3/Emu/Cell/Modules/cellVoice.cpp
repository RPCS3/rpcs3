#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/lv2/sys_event.h"

#include "cellVoice.h"

LOG_CHANNEL(cellVoice);

template <>
void fmt_class_string<CellVoiceError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellVoiceError value)
	{
		switch (value)
		{
			STR_CASE(CELL_VOICE_ERROR_ADDRESS_INVALID);
			STR_CASE(CELL_VOICE_ERROR_ARGUMENT_INVALID);
			STR_CASE(CELL_VOICE_ERROR_CONTAINER_INVALID);
			STR_CASE(CELL_VOICE_ERROR_DEVICE_NOT_PRESENT);
			STR_CASE(CELL_VOICE_ERROR_EVENT_DISPATCH);
			STR_CASE(CELL_VOICE_ERROR_EVENT_QUEUE);
			STR_CASE(CELL_VOICE_ERROR_GENERAL);
			STR_CASE(CELL_VOICE_ERROR_LIBVOICE_INITIALIZED);
			STR_CASE(CELL_VOICE_ERROR_LIBVOICE_NOT_INIT);
			STR_CASE(CELL_VOICE_ERROR_NOT_IMPLEMENTED);
			STR_CASE(CELL_VOICE_ERROR_PORT_INVALID);
			STR_CASE(CELL_VOICE_ERROR_RESOURCE_INSUFFICIENT);
			STR_CASE(CELL_VOICE_ERROR_SERVICE_ATTACHED);
			STR_CASE(CELL_VOICE_ERROR_SERVICE_DETACHED);
			STR_CASE(CELL_VOICE_ERROR_SERVICE_HANDLE);
			STR_CASE(CELL_VOICE_ERROR_SERVICE_NOT_FOUND);
			STR_CASE(CELL_VOICE_ERROR_SHAREDMEMORY);
			STR_CASE(CELL_VOICE_ERROR_TOPOLOGY);
		}
		return unknown;
	});
}

error_code cellVoiceConnectIPortToOPort(u32 ips, u32 ops)
{
	cellVoice.todo("cellVoiceConnectIPortToOPort(iPort=0x%x, oPort=0x%x)", ips, ops);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	return CELL_OK;
}

error_code cellVoiceCreateNotifyEventQueue(vm::ptr<u32> id, vm::ptr<u64> key)
{
	cellVoice.todo("cellVoiceCreateNotifyEventQueue(id=*0x%x, key=*0x%x)", id, key);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	// axed from cellAudio
	vm::var<sys_event_queue_attribute_t> attr;
	attr->protocol = SYS_SYNC_FIFO;
	attr->type = SYS_PPU_QUEUE;
	attr->name_u64 = 0;

	for (u64 i = 0; i < 10; i++)
	{
		// Create an event queue "bruteforcing" an available key
		const u64 key_value = 0x80004d494f323285ull + i;
		if (const s32 res = sys_event_queue_create(id, attr, key_value, 32))
		{
			if (res != CELL_EEXIST)
			{
				return res;
			}
		}
		else
		{
			*key = key_value;
			return CELL_OK;
		}
	}

	return CELL_VOICE_ERROR_EVENT_QUEUE;
}

error_code cellVoiceCreatePort(vm::ptr<u32> portId, vm::cptr<CellVoicePortParam> pArg)
{
	cellVoice.todo("cellVoiceCreatePort(portId=*0x%x, pArg=*0x%x)", portId, pArg);

	if (!pArg)
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.size() > CELLVOICE_MAX_PORT)
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	port_info new_port;
	new_port.bitrate = pArg->voice.bitrate;
	new_port.state = CELLVOICE_PORTSTATE_IDLE;
	new_port.param.portType = pArg->portType;
	new_port.param.threshold = pArg->threshold;
	new_port.param.bMute = pArg->bMute;
	new_port.param.volume = pArg->volume;
	// TODO: more

	handler->ports.emplace(++handler->port_index, new_port);


	return CELL_OK;
}

s32 cellVoiceDeletePort(u32 portId)
{
	cellVoice.todo("cellVoiceDeletePort(portId=%d)", portId);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.erase(portId) == 0)
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	return CELL_OK;
}

s32 cellVoiceDisconnectIPortFromOPort()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

error_code cellVoiceEnd()
{
	cellVoice.todo("cellVoiceEnd()");

	const auto handler = fxm::withdraw<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->voice_ready)
	{
		for (auto key_source_pair : handler->event_queues)
		{
			if (auto queue = lv2_event_queue::find(key_source_pair.first))
			{
				queue->send(key_source_pair.second, CELLVOICE_EVENT_SERVICE_DETACHED, 0, 0);
			}
		}
	}

	return CELL_OK;
}

s32 cellVoiceGetBitRate(u32 portId, vm::ptr<u32> bitrate)
{
	cellVoice.todo("cellVoiceGetBitRate(portId=%d, bitrate=*0x%x)", portId, bitrate);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	if (!bitrate)
	{
		return CELL_EFAULT;
	}

	*bitrate = handler->ports[portId].bitrate; // handler->ports[portId].voice.bitrate;

	return CELL_OK;
}

s32 cellVoiceGetMuteFlag(u32 portId, vm::ptr<u16> bMuted)
{
	cellVoice.todo("cellVoiceGetMuteFlag(portId=%d, bMuted=*0x%x)", portId, bMuted);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	if (!bMuted)
	{
		return CELL_EFAULT;
	}

	*bMuted = handler->ports[portId].param.bMute;

	return CELL_OK;
}

s32 cellVoiceGetPortAttr(u32 portId, s32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceGetPortAttr(portId=%d, attr=*0x%x, attrValue=*0x%x)", portId, attr, attrValue);

	if (!attrValue)
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	if (true)
	{
		return CELL_VOICE_ERROR_DEVICE_NOT_PRESENT;
	}

	return CELL_OK;
}

s32 cellVoiceGetPortInfo(u32 portId, vm::ptr<s32> attr)
{
	cellVoice.todo("cellVoiceGetPortInfo(portId=%d, attr=*0x%x)", portId, attr);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	return CELL_OK;
}

s32 cellVoiceGetSignalState(u32 portId, s32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceGetSignalState(portId=%d, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	if (!attrValue)
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	if (true)
	{
		return CELL_VOICE_ERROR_DEVICE_NOT_PRESENT;
	}

	return CELL_OK;
}

s32 cellVoiceGetVolume(u32 portId, vm::ptr<f32> volume)
{
	cellVoice.todo("cellVoiceGetVolume(portId=%d, volume=*0x%x)", portId, volume);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	if (!volume)
	{
		return CELL_EFAULT;
	}

	*volume = handler->ports[portId].param.volume;

	return CELL_OK;
}

error_code cellVoiceInit(vm::ptr<CellVoiceInitParam> pArg)
{
	cellVoice.todo("cellVoiceInit(pArg=*0x%x)", pArg);

	if (!pArg)
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	const auto handler = fxm::make<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_INITIALIZED;
	}

	if (!pArg)
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	vm::var<u32> id;

	vm::var<sys_event_queue_attribute_t> attr;
	attr->protocol = SYS_SYNC_FIFO;
	attr->type = SYS_PPU_QUEUE;
	attr->name_u64 = 0;

	for (u64 i = 0; i < 10; i++)
	{
		// Create an event queue "bruteforcing" an available key
		const u64 key_value = 0x80004d494f323285ull + i;
		if (const s32 res = sys_event_queue_create(id, attr, key_value, 32))
		{
			if (res != CELL_EEXIST)
			{
				return res;
			}
		}
		else
		{
			handler->event_queues[key_value] = 0x54EB5CFC;
			return CELL_OK;
		}
	}

	return CELL_OK;
}

s32 cellVoiceInitEx(vm::ptr<CellVoiceInitParam> pArg)
{
	cellVoice.todo("cellVoiceInitEx(pArg=*0x%x)", pArg);

	// TODO

	return CELL_OK;
}

s32 cellVoicePausePort(u32 portId)
{
	cellVoice.todo("cellVoicePausePort(portId=%d)", portId);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	handler->ports[portId].state = CELLVOICE_PORTSTATE_READY;

	return CELL_OK;
}

s32 cellVoicePausePortAll()
{
	cellVoice.todo("cellVoicePausePortAll()");

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	for (auto& port : handler->ports)
	{
		port.second.state = CELLVOICE_PORTSTATE_READY;
	}

	return CELL_OK;
}

s32 cellVoiceRemoveNotifyEventQueue(u64 key)
{
	cellVoice.todo("cellVoiceRemoveNotifyEventQueue(key=%d)", key);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	for (auto key_source_pair : handler->event_queues)
	{
		if (key_source_pair.first == key)
		{
			handler->event_queues.erase(key);

			return CELL_OK;
		}
	}

	return CELL_VOICE_ERROR_EVENT_QUEUE;
}

s32 cellVoiceResetPort(u32 portId)
{
	cellVoice.todo("cellVoiceResetPort(portId=%d)", portId);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	handler->ports[portId].state = CELLVOICE_PORTSTATE_IDLE;

	return CELL_OK;
}

s32 cellVoiceResumePort(u32 portId)
{
	cellVoice.todo("cellVoiceResumePort(portId=%d)", portId);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	handler->ports[portId].state = CELLVOICE_PORTSTATE_RUNNING;

	return CELL_OK;
}

s32 cellVoiceResumePortAll()
{
	cellVoice.todo("cellVoiceResumePortAll()");

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	for (auto& port : handler->ports)
	{
		port.second.state = CELLVOICE_PORTSTATE_RUNNING;
	}

	return CELL_OK;
}

error_code cellVoiceSetBitRate(u32 portId, s32 bitrate)
{
	cellVoice.todo("cellVoiceSetBitRate(portId=0x%x, bitrate=%d)", portId, bitrate);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	handler->ports[portId].bitrate = bitrate; // handler->ports[portId].voice.bitrate = bitrate;

	return CELL_OK;
}

s32 cellVoiceSetMuteFlag(u32 portId, u16 bMuted)
{
	cellVoice.todo("cellVoiceSetMuteFlag(portId=%d, bMuted=%d)", portId, bMuted);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	handler->ports[portId].param.bMute = bMuted;

	return CELL_OK;
}

s32 cellVoiceSetMuteFlagAll(u16 bMuted)
{
	cellVoice.todo("cellVoiceSetMuteFlagAll(bMuted=%d)", bMuted);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	for (auto& port : handler->ports)
	{
		port.second.param.bMute = bMuted;
	}

	return CELL_OK;
}

error_code cellVoiceSetNotifyEventQueue(u64 key, u64 source)
{
	cellVoice.todo("cellVoiceSetNotifyEventQueue(key=0x%llx, source=0x%x)", key, source);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	for (auto key_source_pair : handler->event_queues) // check for duplicates
	{
		if (key_source_pair.first == key)
		{
			return CELL_VOICE_ERROR_EVENT_QUEUE;
		}
	}

	handler->event_queues[key] = source;

	return CELL_OK;
}

s32 cellVoiceSetPortAttr(u32 portId, s32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceSetPortAttr(portId=%d, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	if (!attrValue)
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellVoiceSetVolume(u32 portId, f32 volume)
{
	cellVoice.todo("cellVoiceSetVolume(portId=%d, volume=%f)", portId, volume);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	handler->ports[portId].param.volume = volume;

	return CELL_OK;
}

error_code cellVoiceStart()
{
	cellVoice.todo("cellVoiceStart()");

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	for (auto key_source_pair : handler->event_queues)
	{
		if (auto voiceQueue = lv2_event_queue::find(key_source_pair.first))
		{
			voiceQueue->send(key_source_pair.second, CELLVOICE_EVENT_SERVICE_ATTACHED, 0, 0);
		}
	}

	handler->voice_ready = true;
	handler->portstate = CELLVOICE_PORTSTATE_READY;

	return CELL_OK;
}

s32 cellVoiceStartEx(vm::ptr<CellVoiceStartParam> pArg)
{
	cellVoice.todo("cellVoiceStartEx(pArg=*0x%x)", pArg);

	if (!pArg)
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	// TODO

	return CELL_OK;
}

error_code cellVoiceStop()
{
	cellVoice.todo("cellVoiceStop()");

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->voice_ready)
	{
		for (auto key_source_pair : handler->event_queues)
		{
			if (auto voiceQueue = lv2_event_queue::find(key_source_pair.first))
			{
				voiceQueue->send(key_source_pair.second, CELLVOICE_EVENT_SERVICE_DETACHED, 0, 0);
			}
		}

		handler->voice_ready = false;
		handler->portstate = CELLVOICE_PORTSTATE_IDLE;
	}

	return CELL_OK;
}

s32 cellVoiceUpdatePort(u32 portId, vm::cptr<CellVoicePortParam> pArg)
{
	cellVoice.todo("cellVoiceUpdatePort(portId=%d, pArg=*0x%x)", portId, pArg);

	if (!pArg)
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	if (handler->ports.find(portId) == handler->ports.end())
	{
		return CELL_VOICE_ERROR_TOPOLOGY;
	}

	handler->ports[portId].param.threshold = pArg->threshold;
	handler->ports[portId].param.bMute = pArg->bMute;
	handler->ports[portId].param.volume = pArg->volume;
	handler->ports[portId].param.voice     = pArg->voice;

	return CELL_OK;
}

s32 cellVoiceWriteToIPort(u32 ips, vm::cptr<void> data, vm::ptr<u32> size)
{
	cellVoice.todo("cellVoiceWriteToIPort(ips=%d, data=*0x%x, size=*0x%x)", ips, data, size);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellVoiceWriteToIPortEx(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, u32 numFrameLost)
{
	cellVoice.todo("cellVoiceWriteToIPortEx(ips=%d, data=*0x%x, size=*0x%x, numFrameLost=%d)", ips, data, size, numFrameLost);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellVoiceWriteToIPortEx2(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, s16 frameGaps)
{
	cellVoice.todo("cellVoiceWriteToIPortEx2(ips=%d, data=*0x%x, size=*0x%x, frameGaps=%d)", ips, data, size, frameGaps);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellVoiceReadFromOPort(u32 ops, vm::ptr<void> data, vm::ptr<u32> size)
{
	cellVoice.todo("cellVoiceReadFromOPort(ops=%d, data=*0x%x, size=*0x%x)", ops, data, size);

	const auto handler = fxm::get<voice_t>();
	if (!handler)
	{
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;
	}

	return CELL_OK;
}

s32 cellVoiceDebugTopology()
{
	UNIMPLEMENTED_FUNC(cellVoice);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellVoice)("cellVoice", []()
{
	REG_FUNC(cellVoice, cellVoiceConnectIPortToOPort);
	REG_FUNC(cellVoice, cellVoiceCreateNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceCreatePort);
	REG_FUNC(cellVoice, cellVoiceDeletePort);
	REG_FUNC(cellVoice, cellVoiceDisconnectIPortFromOPort);
	REG_FUNC(cellVoice, cellVoiceEnd);
	REG_FUNC(cellVoice, cellVoiceGetBitRate);
	REG_FUNC(cellVoice, cellVoiceGetMuteFlag);
	REG_FUNC(cellVoice, cellVoiceGetPortAttr);
	REG_FUNC(cellVoice, cellVoiceGetPortInfo);
	REG_FUNC(cellVoice, cellVoiceGetSignalState);
	REG_FUNC(cellVoice, cellVoiceGetVolume);
	REG_FUNC(cellVoice, cellVoiceInit);
	REG_FUNC(cellVoice, cellVoiceInitEx);
	REG_FUNC(cellVoice, cellVoicePausePort);
	REG_FUNC(cellVoice, cellVoicePausePortAll);
	REG_FUNC(cellVoice, cellVoiceRemoveNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceResetPort);
	REG_FUNC(cellVoice, cellVoiceResumePort);
	REG_FUNC(cellVoice, cellVoiceResumePortAll);
	REG_FUNC(cellVoice, cellVoiceSetBitRate);
	REG_FUNC(cellVoice, cellVoiceSetMuteFlag);
	REG_FUNC(cellVoice, cellVoiceSetMuteFlagAll);
	REG_FUNC(cellVoice, cellVoiceSetNotifyEventQueue);
	REG_FUNC(cellVoice, cellVoiceSetPortAttr);
	REG_FUNC(cellVoice, cellVoiceSetVolume);
	REG_FUNC(cellVoice, cellVoiceStart);
	REG_FUNC(cellVoice, cellVoiceStartEx);
	REG_FUNC(cellVoice, cellVoiceStop);
	REG_FUNC(cellVoice, cellVoiceUpdatePort);
	REG_FUNC(cellVoice, cellVoiceWriteToIPort);
	REG_FUNC(cellVoice, cellVoiceWriteToIPortEx);
	REG_FUNC(cellVoice, cellVoiceWriteToIPortEx2);
	REG_FUNC(cellVoice, cellVoiceReadFromOPort);
	REG_FUNC(cellVoice, cellVoiceDebugTopology);
});
