#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

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

error_code cellVoiceConnectIPortToOPort(u32 ips, u32 ops)
{
	cellVoice.todo("cellVoiceConnectIPortToOPort(ips=%d, ops=%d)", ips, ops);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceCreateNotifyEventQueue(vm::ptr<u32> id, vm::ptr<u64> key)
{
	cellVoice.todo("cellVoiceCreateNotifyEventQueue(id=*0x%x, key=*0x%x)", id, key);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceCreatePort(vm::ptr<u32> portId, vm::cptr<CellVoicePortParam> pArg)
{
	cellVoice.todo("cellVoiceCreatePort(portId=*0x%x, pArg=*0x%x)", portId, pArg);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	return CELL_OK;
}

error_code cellVoiceDeletePort(u32 portId)
{
	cellVoice.todo("cellVoiceDeletePort(portId=%d)", portId);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceDisconnectIPortFromOPort(u32 ips, u32 ops)
{
	cellVoice.todo("cellVoiceDisconnectIPortFromOPort(ips=%d, ops=%d)", ips, ops);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceEnd()
{
	cellVoice.todo("cellVoiceEnd()");

	const auto manager = g_fxo->get<voice_manager>();

	if (!manager->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	manager->is_init = false;

	return CELL_OK;
}

error_code cellVoiceGetBitRate(u32 portId, vm::ptr<u32> bitrate)
{
	cellVoice.todo("cellVoiceGetBitRate(portId=%d, bitrate=*0x%x)", portId, bitrate);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceGetMuteFlag(u32 portId, vm::ptr<u16> bMuted)
{
	cellVoice.todo("cellVoiceGetMuteFlag(portId=%d, bMuted=*0x%x)", portId, bMuted);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceGetPortAttr(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceGetPortAttr(portId=%d, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceGetPortInfo(u32 portId, vm::ptr<CellVoiceBasePortInfo> pInfo)
{
	cellVoice.todo("cellVoiceGetPortInfo(portId=%d, pInfo=*0x%x)", portId, pInfo);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceGetSignalState(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceGetSignalState(portId=%d, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceGetVolume(u32 portId, vm::ptr<f32> volume)
{
	cellVoice.todo("cellVoiceGetVolume(portId=%d, volume=*0x%x)", portId, volume);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceInit(vm::ptr<CellVoiceInitParam> pArg)
{
	cellVoice.todo("cellVoiceInit(pArg=*0x%x)", pArg);

	const auto manager = g_fxo->get<voice_manager>();

	if (manager->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_INITIALIZED;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	manager->is_init = true;

	return CELL_OK;
}

error_code cellVoiceInitEx(vm::ptr<CellVoiceInitParam> pArg)
{
	cellVoice.todo("cellVoiceInitEx(pArg=*0x%x)", pArg);

	const auto manager = g_fxo->get<voice_manager>();

	if (manager->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_INITIALIZED;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	manager->is_init = true;

	return CELL_OK;
}

error_code cellVoicePausePort(u32 portId)
{
	cellVoice.todo("cellVoicePausePort(portId=%d)", portId);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoicePausePortAll()
{
	cellVoice.todo("cellVoicePausePortAll()");

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceRemoveNotifyEventQueue(u64 key)
{
	cellVoice.todo("cellVoiceRemoveNotifyEventQueue(key=0x%llx)", key);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceResetPort(u32 portId)
{
	cellVoice.todo("cellVoiceResetPort(portId=%d)", portId);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceResumePort(u32 portId)
{
	cellVoice.todo("cellVoiceResumePort(portId=%d)", portId);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceResumePortAll()
{
	cellVoice.todo("cellVoiceResumePortAll()");

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceSetBitRate(u32 portId, s32 bitrate)
{
	cellVoice.todo("cellVoiceSetBitRate(portId=%d, bitrate=%d)", portId, bitrate);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceSetMuteFlag(u32 portId, u16 bMuted)
{
	cellVoice.todo("cellVoiceSetMuteFlag(portId=%d, bMuted=%d)", portId, bMuted);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceSetMuteFlagAll(u16 bMuted)
{
	cellVoice.todo("cellVoiceSetMuteFlagAll(bMuted=%d)", bMuted);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceSetNotifyEventQueue(u64 key, u64 source)
{
	cellVoice.todo("cellVoiceSetNotifyEventQueue(key=0x%llx, source=%d)", key, source);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceSetPortAttr(u32 portId, u32 attr, vm::ptr<void> attrValue)
{
	cellVoice.todo("cellVoiceSetPortAttr(portId=%d, attr=%d, attrValue=*0x%x)", portId, attr, attrValue);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceSetVolume(u32 portId, f32 volume)
{
	cellVoice.todo("cellVoiceSetVolume(portId=%d, volume=%f)", portId, volume);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceStart()
{
	cellVoice.todo("cellVoiceStart()");

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceStartEx(vm::ptr<CellVoiceStartParam> pArg)
{
	cellVoice.todo("cellVoiceStartEx(pArg=*0x%x)", pArg);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	return CELL_OK;
}

error_code cellVoiceStop()
{
	cellVoice.todo("cellVoiceStop()");

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceUpdatePort(u32 portId, vm::cptr<CellVoicePortParam> pArg)
{
	cellVoice.todo("cellVoiceUpdatePort(portId=%d, pArg=*0x%x)", portId, pArg);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	if (!pArg)
		return CELL_VOICE_ERROR_ARGUMENT_INVALID;

	return CELL_OK;
}

error_code cellVoiceWriteToIPort(u32 ips, vm::cptr<void> data, vm::ptr<u32> size)
{
	cellVoice.todo("cellVoiceWriteToIPort(ips=%d, data=*0x%x, size=*0x%x)", ips, data, size);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceWriteToIPortEx(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, u32 numFrameLost)
{
	cellVoice.todo("cellVoiceWriteToIPortEx(ips=%d, data=*0x%x, size=*0x%x, numFrameLost=%d)", ips, data, size, numFrameLost);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceWriteToIPortEx2(u32 ips, vm::cptr<void> data, vm::ptr<u32> size, s16 frameGaps)
{
	cellVoice.todo("cellVoiceWriteToIPortEx2(ips=%d, data=*0x%x, size=*0x%x, frameGaps=%d)", ips, data, size, frameGaps);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceReadFromOPort(u32 ops, vm::ptr<void> data, vm::ptr<u32> size)
{
	cellVoice.todo("cellVoiceReadFromOPort(ops=%d, data=*0x%x, size=*0x%x)", ops, data, size);

	if (!g_fxo->get<voice_manager>()->is_init)
		return CELL_VOICE_ERROR_LIBVOICE_NOT_INIT;

	return CELL_OK;
}

error_code cellVoiceDebugTopology()
{
	UNIMPLEMENTED_FUNC(cellVoice);

	if (!g_fxo->get<voice_manager>()->is_init)
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
