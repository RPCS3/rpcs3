#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "cellVoice.h"

LOG_CHANNEL(cellVoice);

template<>
void fmt_class_string<CellVoiceError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
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

void voice_manager::reset()
{
	id_ctr = 0;
	port_source = 0;
	ports.clear();
	queue_keys.clear();
}

void voice_manager::save(utils::serial& ar)
{
	GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellVoice);
	ar(id_ctr, port_source, ports, queue_keys, voice_service_started);
}

error_code cellVoiceConnectIPortToOPort(u32 ips, u32 ops)
{
	cellVoice.todo("cellVoiceConnectIPortToOPort(ips=%d, ops=%d)", ips, ops);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return CELL_VOICE_ERROR_TOPOLOGY;

	auto oport = manager.access_port(ops);

	if (!oport || oport->info.portType <= CELLVOICE_PORTTYPE_IN_VOICE)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoiceCreateNotifyEventQueue(ppu_thread& ppu, vm::ptr<u32> id, vm::ptr<u64> key)
{
	cellVoice.warning("cellVoiceCreateNotifyEventQueue(id=*0x%x, key=*0x%x)", id, key);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	vm::var<sys_event_queue_attribute_t> attr;
	attr->protocol = SYS_SYNC_FIFO;
	attr->type = SYS_PPU_QUEUE;
	attr->name_u64 = 0;

	for (u64 i = 0; i < 10; i++)
	{
		// Create an event queue "bruteforcing" an available key
		const u64 key_value = 0x80004d494f323285ull + i;
		if (CellError res{sys_event_queue_create(ppu, id, attr, key_value, 0x40) + 0u})
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
	cellVoice.warning("cellVoiceCreatePort(portId=*0x%x, pArg=*0x%x)", portId, pArg);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	switch (pArg->portType)
	{
	case CELLVOICE_PORTTYPE_IN_PCMAUDIO:
	case CELLVOICE_PORTTYPE_OUT_PCMAUDIO:
	{
		if (pArg->pcmaudio.format.dataType > CELLVOICE_PCM_INTEGER_LITTLE_ENDIAN)
			return CELL_VOICE_ERROR_ARGUMENT_INVALID;

		break;
	}
	case CELLVOICE_PORTTYPE_IN_VOICE:
	case CELLVOICE_PORTTYPE_OUT_VOICE:
	{
		// Must be an exact value
		switch (pArg->voice.bitrate)
		{
		case CELLVOICE_BITRATE_3850:
		case CELLVOICE_BITRATE_4650:
		case CELLVOICE_BITRATE_5700:
		case CELLVOICE_BITRATE_7300:
		case CELLVOICE_BITRATE_14400:
		case CELLVOICE_BITRATE_16000:
		case CELLVOICE_BITRATE_22533:
			break;
		default:
		{
			return CELL_VOICE_ERROR_ARGUMENT_INVALID;
		}
		}
		break;
	}
	case CELLVOICE_PORTTYPE_IN_MIC:
	case CELLVOICE_PORTTYPE_OUT_SECONDARY:
	{
		break;
	}
	default:
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}

	if (manager.ports.size() > CELLVOICE_MAX_PORT)
		return CELL_VOICE_ERROR_RESOURCE_INSUFFICIENT;

	// Id: bits [8,15] seem to contain a "random" value
	// bits [0,7] are based on creation counter modulo 0xa0
	// The rest are set to zero and ignored.
	manager.id_ctr++; manager.id_ctr %= 0xa0;

	// It isn't known whether bits[8,15] are guaranteed to be non-zero
	constexpr u32 min_value = 1;

	for (u32 ctr2 = min_value; ctr2 < CELLVOICE_MAX_PORT + min_value; ctr2++)
	{
		const auto [port, success] = manager.ports.try_emplace(static_cast<u16>((ctr2 << 8) | manager.id_ctr));

		if (success)
		{
			port->second.info = *pArg;
			*portId = port->first;
			return CELL_OK;
		}
	}

	fmt::throw_exception("Unreachable");
}

error_code cellVoiceDeletePort(u32 portId)
{
	cellVoice.warning("cellVoiceDeletePort(portId=%d)", portId);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (manager.ports.erase(static_cast<u16>(portId)) == 0)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoiceDisconnectIPortFromOPort(u32 ips, u32 ops)
{
	cellVoice.todo("cellVoiceDisconnectIPortFromOPort(ips=%d, ops=%d)", ips, ops);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return CELL_VOICE_ERROR_TOPOLOGY;

	auto oport = manager.access_port(ops);

	if (!oport || oport->info.portType <= CELLVOICE_PORTTYPE_IN_VOICE)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoiceEnd()
{
	cellVoice.warning("cellVoiceEnd()");

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (std::exchange(manager.voice_service_started, false))
	{
		for (auto& key_pair : manager.queue_keys)
		{
			if (auto queue = lv2_event_queue::find(key_pair.first))
			{
				for (const auto& source : key_pair.second)
				{
					queue->send(source, CELLVOICE_EVENT_SERVICE_DETACHED, 0, 0);
				}
			}
		}
	}

	manager.reset();
	manager.is_init = false;

	return CELL_OK;
}

error_code cellVoiceGetBitRate(u32 portId, vm::ptr<u32> bitrate)
{
	cellVoice.warning("cellVoiceGetBitRate(portId=%d, bitrate=*0x%x)", portId, bitrate);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	// No nullptr check!

	// Constant value for errors (meaning unknown)
	*bitrate = 0x4f323285;

	auto port = manager.access_port(portId);

	if (!port || (port->info.portType != CELLVOICE_PORTTYPE_IN_VOICE && port->info.portType != CELLVOICE_PORTTYPE_OUT_VOICE))
		return CELL_VOICE_ERROR_TOPOLOGY;

	*bitrate = port->info.voice.bitrate;
	return CELL_OK;
}

error_code cellVoiceGetMuteFlag(u32 portId, vm::ptr<u16> bMuted)
{
	cellVoice.warning("cellVoiceGetMuteFlag(portId=%d, bMuted=*0x%x)", portId, bMuted);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	*bMuted = port->info.bMute;
	return CELL_OK;
}

error_code cellVoiceGetPortAttr(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceGetPortAttr(portId=%d, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	// Report detached microphone
	return not_an_error(CELL_VOICE_ERROR_DEVICE_NOT_PRESENT);
}

error_code cellVoiceGetPortInfo(u32 portId, vm::ptr<CellVoiceBasePortInfo> pInfo)
{
	cellVoice.todo("cellVoiceGetPortInfo(portId=%d, pInfo=*0x%x)", portId, pInfo);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	if (!manager.voice_service_started)
		return CELL_VOICE_ERROR_SERVICE_DETACHED;

	// No nullptr check!
	pInfo->portType = port->info.portType;

	// TODO

	return CELL_OK;
}

error_code cellVoiceGetSignalState(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceGetSignalState(portId=%d, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	// Report detached microphone
	return not_an_error(CELL_VOICE_ERROR_DEVICE_NOT_PRESENT);
}

error_code cellVoiceGetVolume(u32 portId, vm::ptr<f32> volume)
{
	cellVoice.warning("cellVoiceGetVolume(portId=%d, volume=*0x%x)", portId, volume);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	// No nullptr check!

	// Constant value for errors (meaning unknown)
	*volume = std::bit_cast<f32, s32>(0x4f323285);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	*volume = port->info.volume;
	return CELL_OK;
}

error_code cellVoiceInit(vm::ptr<CellVoiceInitParam> pArg)
{
	cellVoice.todo("cellVoiceInit(pArg=*0x%x)", pArg);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_INITIALIZED;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	manager.is_init = true;

	return CELL_OK;
}

error_code cellVoiceInitEx(vm::ptr<CellVoiceInitParam> pArg)
{
	cellVoice.todo("cellVoiceInitEx(pArg=*0x%x)", pArg);

	auto& manager = g_fxo->get<voice_manager>();

	if (manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_INITIALIZED;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	manager.is_init = true;

	return CELL_OK;
}

error_code cellVoicePausePort(u32 portId)
{
	cellVoice.todo("cellVoicePausePort(portId=%d)", portId);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoicePausePortAll()
{
	cellVoice.todo("cellVoicePausePortAll()");

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceRemoveNotifyEventQueue(u64 key)
{
	cellVoice.warning("cellVoiceRemoveNotifyEventQueue(key=0x%llx)", key);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (manager.queue_keys.erase(key) == 0)
		return CELL_VOICE_ERROR_EVENT_QUEUE;

	return CELL_OK;
}

error_code cellVoiceResetPort(u32 portId)
{
	cellVoice.todo("cellVoiceResetPort(portId=%d)", portId);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoiceResumePort(u32 portId)
{
	cellVoice.todo("cellVoiceResumePort(portId=%d)", portId);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoiceResumePortAll()
{
	cellVoice.todo("cellVoiceResumePortAll()");

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceSetBitRate(u32 portId, CellVoiceBitRate bitrate)
{
	cellVoice.warning("cellVoiceSetBitRate(portId=%d, bitrate=%d)", portId, +bitrate);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port || (port->info.portType != CELLVOICE_PORTTYPE_IN_VOICE && port->info.portType != CELLVOICE_PORTTYPE_OUT_VOICE))
		return CELL_VOICE_ERROR_TOPOLOGY;

	// TODO: Check ordering of checks.
	switch (bitrate)
	{
	case CELLVOICE_BITRATE_3850:
	case CELLVOICE_BITRATE_4650:
	case CELLVOICE_BITRATE_5700:
	case CELLVOICE_BITRATE_7300:
	case CELLVOICE_BITRATE_14400:
	case CELLVOICE_BITRATE_16000:
	case CELLVOICE_BITRATE_22533:
		break;
	default:
	{
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;
	}
	}

	port->info.voice.bitrate = bitrate;
	return CELL_OK;
}

error_code cellVoiceSetMuteFlag(u32 portId, u16 bMuted)
{
	cellVoice.warning("cellVoiceSetMuteFlag(portId=%d, bMuted=%d)", portId, bMuted);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	port->info.bMute = bMuted;
	return CELL_OK;
}

error_code cellVoiceSetMuteFlagAll(u16 bMuted)
{
	cellVoice.warning("cellVoiceSetMuteFlagAll(bMuted=%d)", bMuted);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	// Doesn't change port->bMute value
	return CELL_OK;
}

error_code cellVoiceSetNotifyEventQueue(u64 key, u64 source)
{
	cellVoice.warning("cellVoiceSetNotifyEventQueue(key=0x%llx, source=0x%llx)", key, source);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	// Note: it is allowed to enqueue the key twice (another source is enqueued with FIFO ordering)
	// It is not allowed to enqueue an invalid key

	if (!lv2_event_queue::find(key))
		return CELL_VOICE_ERROR_EVENT_QUEUE;

	if (!source)
	{
		// same thing as sys_event_port_send with port.name == 0
		// Try to give different port id everytime
		source = ((process_getpid() + 1ull) << 32) | (lv2_event_port::id_base + manager.port_source * lv2_event_port::id_step);
		manager.port_source = (manager.port_source + 1) % lv2_event_port::id_count;
	}

	manager.queue_keys[key].push_back(source);
	return CELL_OK;
}

error_code cellVoiceSetPortAttr(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceSetPortAttr(portId=%d, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	// Report detached microphone
	return not_an_error(CELL_VOICE_ERROR_DEVICE_NOT_PRESENT);
}

error_code cellVoiceSetVolume(u32 portId, f32 volume)
{
	cellVoice.warning("cellVoiceSetVolume(portId=%d, volume=%f)", portId, volume);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	port->info.volume = volume;
	return CELL_OK;
}

error_code VoiceStart(voice_manager& manager)
{
	if (std::exchange(manager.voice_service_started, true))
		return CELL_OK;

	for (auto& key_pair : manager.queue_keys)
	{
		if (auto queue = lv2_event_queue::find(key_pair.first))
		{
			for (const auto& source : key_pair.second)
			{
				queue->send(source, CELLVOICE_EVENT_SERVICE_ATTACHED, 0, 0);
			}
		}
	}

	return CELL_OK;
}

error_code cellVoiceStart()
{
	cellVoice.warning("cellVoiceStart()");

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return VoiceStart(manager);
}

error_code cellVoiceStartEx(vm::ptr<CellVoiceStartParam> pArg)
{
	cellVoice.todo("cellVoiceStartEx(pArg=*0x%x)", pArg);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	// TODO: Check provided memory container

	return VoiceStart(manager);
}

error_code cellVoiceStop()
{
	cellVoice.warning("cellVoiceStop()");

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!std::exchange(manager.voice_service_started, false))
		return CELL_OK;

	for (auto& key_pair : manager.queue_keys)
	{
		if (auto queue = lv2_event_queue::find(key_pair.first))
		{
			for (const auto& source : key_pair.second)
			{
				queue->send(source, CELLVOICE_EVENT_SERVICE_DETACHED, 0, 0);
			}
		}
	}

	return CELL_OK;
}

error_code cellVoiceUpdatePort(u32 portId, vm::cptr<CellVoicePortParam> pArg)
{
	cellVoice.warning("cellVoiceUpdatePort(portId=%d, pArg=*0x%x)", portId, pArg);

	auto& manager = g_fxo->get<voice_manager>();

	std::scoped_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	auto port = manager.access_port(portId);

	if (!port)
		return CELL_VOICE_ERROR_TOPOLOGY;

	// Not all info is updated
	port->info.bMute = pArg->bMute;
	port->info.volume = pArg->volume;
	port->info.threshold = pArg->threshold;

	if (port->info.portType == CELLVOICE_PORTTYPE_IN_VOICE || port->info.portType == CELLVOICE_PORTTYPE_OUT_VOICE)
	{
		port->info.voice.bitrate = pArg->voice.bitrate;
	}

	return CELL_OK;
}

error_code cellVoiceWriteToIPort(u32 ips, vm::cptr<void> data, vm::ptr<u32> size)
{
	cellVoice.todo("cellVoiceWriteToIPort(ips=%d, data=*0x%x, size=*0x%x)", ips, data, size);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoiceWriteToIPortEx(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, u32 numFrameLost)
{
	cellVoice.todo("cellVoiceWriteToIPortEx(ips=%d, data=*0x%x, size=*0x%x, numFrameLost=%d)", ips, data, size, numFrameLost);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoiceWriteToIPortEx2(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, s16 frameGaps)
{
	cellVoice.todo("cellVoiceWriteToIPortEx2(ips=%d, data=*0x%x, size=*0x%x, frameGaps=%d)", ips, data, size, frameGaps);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto iport = manager.access_port(ips);

	if (!iport || iport->info.portType >= CELLVOICE_PORTTYPE_OUT_PCMAUDIO)
		return CELL_VOICE_ERROR_TOPOLOGY;

	return CELL_OK;
}

error_code cellVoiceReadFromOPort(u32 ops, vm::ptr<void> data, vm::ptr<u32> size)
{
	// Spammy TODO
	cellVoice.trace("cellVoiceReadFromOPort(ops=%d, data=*0x%x, size=*0x%x)", ops, data, size);

	auto& manager = g_fxo->get<voice_manager>();

	reader_lock lock(manager.mtx);

	if (!manager.is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	auto oport = manager.access_port(ops);

	if (!oport || oport->info.portType <= CELLVOICE_PORTTYPE_IN_VOICE)
		return CELL_VOICE_ERROR_TOPOLOGY;

	if (size)
		*size = 0;

	return CELL_OK;
}

error_code cellVoiceDebugTopology()
{
	UNIMPLEMENTED_FUNC(cellVoice);

	if (!g_fxo->get<voice_manager>().is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_VOICE_ERROR_NOT_IMPLEMENTED;
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
