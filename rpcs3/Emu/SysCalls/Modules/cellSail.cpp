#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/FS/vfsFile.h"

#include "cellSail.h"
#include "cellPamf.h"

extern Module cellSail;

void playerBoot(vm::ptr<CellSailPlayer> pSelf, u64 userParam)
{
	Emu.GetCallbackManager().Async([=](CPUThread& cpu)
	{
		CellSailEvent event;
		event.major = CELL_SAIL_EVENT_PLAYER_STATE_CHANGED;
		event.minor = 0;
		pSelf->callback(static_cast<PPUThread&>(cpu), pSelf->callbackArg, event, CELL_SAIL_PLAYER_STATE_BOOT_TRANSITION, 0);
	});

	// TODO: Do stuff here
	pSelf->booted = true;

	Emu.GetCallbackManager().Async([=](CPUThread& cpu)
	{
		CellSailEvent event;
		event.major = CELL_SAIL_EVENT_PLAYER_CALL_COMPLETED;
		event.minor = CELL_SAIL_PLAYER_CALL_BOOT;
		pSelf->callback(static_cast<PPUThread&>(cpu), pSelf->callbackArg, event, 0, 0);
	});
}

s32 cellSailMemAllocatorInitialize(vm::ptr<CellSailMemAllocator> pSelf, vm::ptr<CellSailMemAllocatorFuncs> pCallbacks)
{
	cellSail.Warning("cellSailMemAllocatorInitialize(pSelf=*0x%x, pCallbacks=*0x%x)", pSelf, pCallbacks);

	pSelf->callbacks = pCallbacks;

	return CELL_OK;
}

s32 cellSailFutureInitialize(vm::ptr<CellSailFuture> pSelf)
{
	cellSail.Todo("cellSailFutureInitialize(pSelf=*0x%x)", pSelf);
	return CELL_OK;
}

s32 cellSailFutureFinalize(vm::ptr<CellSailFuture> pSelf)
{
	cellSail.Todo("cellSailFutureFinalize(pSelf=*0x%x)", pSelf);
	return CELL_OK;
}

s32 cellSailFutureReset(vm::ptr<CellSailFuture> pSelf, b8 wait)
{
	cellSail.Todo("cellSailFutureReset(pSelf=*0x%x, wait=%d)", pSelf, wait);
	return CELL_OK;
}

s32 cellSailFutureSet(vm::ptr<CellSailFuture> pSelf, s32 result)
{
	cellSail.Todo("cellSailFutureSet(pSelf=*0x%x, result=%d)", pSelf, result);
	return CELL_OK;
}

s32 cellSailFutureGet(vm::ptr<CellSailFuture> pSelf, u64 timeout, vm::ptr<s32> pResult)
{
	cellSail.Todo("cellSailFutureGet(pSelf=*0x%x, timeout=%lld, result=*0x%x)", pSelf, timeout, pResult);
	return CELL_OK;
}

s32 cellSailFutureIsDone(vm::ptr<CellSailFuture> pSelf, vm::ptr<s32> pResult)
{
	cellSail.Todo("cellSailFutureIsDone(pSelf=*0x%x, result=*0x%x)", pSelf, pResult);
	return CELL_OK;
}

s32 cellSailDescriptorGetStreamType()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorGetUri()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorGetMediaInfo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorSetAutoSelection(vm::ptr<CellSailDescriptor> pSelf, b8 autoSelection)
{
	cellSail.Warning("cellSailDescriptorSetAutoSelection(pSelf=*0x%x, autoSelection=%d)", pSelf, autoSelection);

	if (pSelf)
	{
		pSelf->autoSelection = autoSelection;
		return autoSelection;
	}

	return CELL_OK;
}

s32 cellSailDescriptorIsAutoSelection(vm::ptr<CellSailDescriptor> pSelf)
{
	cellSail.Warning("cellSailDescriptorIsAutoSelection(pSelf=*0x%x)", pSelf);
	
	if (pSelf)
	{
		return pSelf->autoSelection;
	}

	return CELL_OK;
}

s32 cellSailDescriptorCreateDatabase(vm::ptr<CellSailDescriptor> pSelf, vm::ptr<void> pDatabase, u32 size, u64 arg)
{
	cellSail.Warning("cellSailDescriptorCreateDatabase(pSelf=*0x%x, pDatabase=*0x%x, size=0x%x, arg=0x%llx)", pSelf, pDatabase, size, arg);

	switch ((s32)pSelf->streamType)
	{
		case CELL_SAIL_STREAM_PAMF:
		{
			u32 addr = pSelf->sp_;
			auto ptr = vm::ptr<CellPamfReader>::make(addr);
			memcpy(pDatabase.get_ptr(), ptr.get_ptr(), sizeof(CellPamfReader));
			break;
		}
		default:
			cellSail.Error("Unhandled stream type: %d", pSelf->streamType);
	}

	return CELL_OK;
}

s32 cellSailDescriptorDestroyDatabase()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorOpen()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorClose()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorSetEs()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorClearEs()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorGetCapabilities()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorInquireCapability()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailDescriptorSetParameter()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSoundAdapterInitialize(vm::ptr<CellSailSoundAdapter> pSelf, vm::cptr<CellSailSoundAdapterFuncs> pCallbacks, vm::ptr<void> pArg)
{
	cellSail.Warning("cellSailSoundAdapterInitialize(pSelf=*0x%x, pCallbacks=*0x%x, pArg=*0x%x)", pSelf, pCallbacks, pArg);

	if (pSelf->initialized)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	if (pSelf->registered)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	pSelf->pMakeup = pCallbacks->pMakeup;
	pSelf->pCleanup = pCallbacks->pCleanup;
	pSelf->pFormatChanged = pCallbacks->pFormatChanged;
	pSelf->arg = pArg;
	pSelf->initialized = true;
	pSelf->registered = false;

	return CELL_OK;
}

s32 cellSailSoundAdapterFinalize(vm::ptr<CellSailSoundAdapter> pSelf)
{
	cellSail.Warning("cellSailSoundAdapterFinalize(pSelf=*0x%x)", pSelf);

	if (!pSelf->initialized)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	if (pSelf->registered)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 cellSailSoundAdapterSetPreferredFormat(vm::ptr<CellSailSoundAdapter> pSelf, vm::cptr<CellSailAudioFormat> pFormat)
{
	cellSail.Warning("cellSailSoundAdapterSetPreferredFormat(pSelf=*0x%x, pFormat=*0x%x)", pSelf, pFormat);

	pSelf->format = *pFormat;

	return CELL_OK;
}

s32 cellSailSoundAdapterGetFrame(vm::ptr<CellSailSoundAdapter> pSelf, u32 samples, vm::ptr<CellSailSoundFrameInfo> pInfo)
{
	cellSail.Todo("cellSailSoundAdapterGetFrame(pSelf=*0x%x, samples=%d, pInfo=*0x%x)", pSelf, samples, pInfo);

	if (!pSelf->initialized)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	if (pSelf->registered)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	if (samples > 2048)
	{
		return CELL_SAIL_ERROR_INVALID_ARG;
	}

	return CELL_OK;
}

s32 cellSailSoundAdapterGetFormat(vm::ptr<CellSailSoundAdapter> pSelf, vm::ptr<CellSailAudioFormat> pFormat)
{
	cellSail.Warning("cellSailSoundAdapterGetFormat(pSelf=*0x%x, pFormat=*0x%x)", pSelf, pFormat);

	*pFormat = pSelf->format;

	return CELL_OK;
}

s32 cellSailSoundAdapterUpdateAvSync()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSoundAdapterPtsToTimePosition()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailGraphicsAdapterInitialize(vm::ptr<CellSailGraphicsAdapter> pSelf, vm::cptr<CellSailGraphicsAdapterFuncs> pCallbacks, vm::ptr<void> pArg)
{
	cellSail.Warning("cellSailGraphicsAdapterInitialize(pSelf=*0x%x, pCallbacks=*0x%x, pArg=*0x%x)", pSelf, pCallbacks, pArg);

	if (pSelf->initialized)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	if (pSelf->registered)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	pSelf->pMakeup = pCallbacks->pMakeup;
	pSelf->pCleanup = pCallbacks->pCleanup;
	pSelf->pFormatChanged = pCallbacks->pFormatChanged;
	pSelf->pAlloc = pCallbacks->pAlloc;
	pSelf->pFree = pCallbacks->pFree;
	pSelf->arg = pArg;
	pSelf->initialized = true;
	pSelf->registered = true;

	return CELL_OK;
}

s32 cellSailGraphicsAdapterFinalize(vm::ptr<CellSailGraphicsAdapter> pSelf)
{
	cellSail.Todo("cellSailGraphicsAdapterFinalize(pSelf=*0x%x)", pSelf);

	if (!pSelf->initialized)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	if (pSelf->registered)
	{
		return CELL_SAIL_ERROR_INVALID_STATE;
	}

	return CELL_OK;
}

s32 cellSailGraphicsAdapterSetPreferredFormat(vm::ptr<CellSailGraphicsAdapter> pSelf, vm::cptr<CellSailVideoFormat> pFormat)
{
	cellSail.Warning("cellSailGraphicsAdapterSetPreferredFormat(pSelf=*0x%x, pFormat=*0x%x)", pSelf, pFormat);

	pSelf->format = *pFormat;

	return CELL_OK;
}

s32 cellSailGraphicsAdapterGetFrame(vm::ptr<CellSailGraphicsAdapter> pSelf, vm::ptr<CellSailGraphicsFrameInfo> pInfo)
{
	cellSail.Todo("cellSailGraphicsAdapterGetFrame(pSelf=*0x%x, pInfo=*0x%x)", pSelf, pInfo);
	return CELL_OK;
}

s32 cellSailGraphicsAdapterGetFrame2(vm::ptr<CellSailGraphicsAdapter> pSelf, vm::ptr<CellSailGraphicsFrameInfo> pInfo, vm::ptr<CellSailGraphicsFrameInfo> pPrevInfo, vm::ptr<u64> pFlipTime, u64 flags)
{
	cellSail.Todo("cellSailGraphicsAdapterGetFrame2(pSelf=*0x%x, pInfo=*0x%x, pPrevInfo=*0x%x, flipTime=*0x%x, flags=0x%llx)", pSelf, pInfo, pPrevInfo, pFlipTime, flags);

	return CELL_OK;
}

s32 cellSailGraphicsAdapterGetFormat(vm::ptr<CellSailGraphicsAdapter> pSelf, vm::ptr<CellSailVideoFormat> pFormat)
{
	cellSail.Warning("cellSailGraphicsAdapterGetFormat(pSelf=*0x%x, pFormat=*0x%x)", pSelf, pFormat);

	*pFormat = pSelf->format;

	return CELL_OK;
}

s32 cellSailGraphicsAdapterUpdateAvSync()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailGraphicsAdapterPtsToTimePosition()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAuReceiverInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAuReceiverFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAuReceiverGet()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererAudioInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererAudioFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererAudioNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererAudioNotifyFrameDone()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererAudioNotifyOutputEos()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererVideoInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererVideoFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererVideoNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererVideoNotifyFrameDone()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailRendererVideoNotifyOutputEos()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyInputEos()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyStreamOut()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifySessionError()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyMediaStateChanged()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyOpenCompleted()
{
	throw EXCEPTION("Unexpected function");
}

s32 cellSailSourceNotifyStartCompleted()
{
	throw EXCEPTION("Unexpected function");
}

s32 cellSailSourceNotifyStopCompleted()
{
	throw EXCEPTION("Unexpected function");
}

s32 cellSailSourceNotifyReadCompleted()
{
	throw EXCEPTION("Unexpected function");
}

s32 cellSailSourceSetDiagHandler()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyCloseCompleted()
{
	throw EXCEPTION("Unexpected function");
}

s32 cellSailMp4MovieGetBrand()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailMp4MovieIsCompatibleBrand()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailMp4MovieGetMovieInfo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailMp4MovieGetTrackByIndex()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailMp4MovieGetTrackById()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailMp4MovieGetTrackByTypeAndIndex()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailMp4TrackGetTrackInfo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailMp4TrackGetTrackReferenceCount()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailMp4TrackGetTrackReference()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAviMovieGetMovieInfo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAviMovieGetStreamByIndex()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAviMovieGetStreamByTypeAndIndex()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAviMovieGetHeader()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAviStreamGetMediaType()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailAviStreamGetHeader()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerInitialize2(
	vm::ptr<CellSailPlayer> pSelf,
	vm::ptr<CellSailMemAllocator> pAllocator,
	vm::ptr<CellSailPlayerFuncNotified> pCallback,
	vm::ptr<void> callbackArg,
	vm::ptr<CellSailPlayerAttribute> pAttribute,
	vm::ptr<CellSailPlayerResource> pResource)
{
	cellSail.Warning("cellSailPlayerInitialize2(pSelf=*0x%x, pAllocator=*0x%x, pCallback=*0x%x, callbackArg=*0x%x, pAttribute=*0x%x, pResource=*0x%x)",
		pSelf, pAllocator, pCallback, callbackArg, pAttribute, pResource);

	pSelf->allocator = *pAllocator;
	pSelf->callback = pCallback;
	pSelf->callbackArg = callbackArg;
	pSelf->attribute = *pAttribute;
	pSelf->resource = *pResource;
	pSelf->booted = false;
	pSelf->paused = true;

	Emu.GetCallbackManager().Async([=](CPUThread& cpu)
	{
		CellSailEvent event;
		event.major = CELL_SAIL_EVENT_PLAYER_STATE_CHANGED;
		event.minor = 0;
		pSelf->callback(static_cast<PPUThread&>(cpu), pSelf->callbackArg, event, CELL_SAIL_PLAYER_STATE_INITIALIZED, 0);
	});

	return CELL_OK;
}

s32 cellSailPlayerFinalize(vm::ptr<CellSailPlayer> pSelf)
{
	cellSail.Todo("cellSailPlayerFinalize(pSelf=*0x%x)", pSelf);

	if (pSelf->sAdapter)
	{
		pSelf->sAdapter->registered = false;
	}

	if (pSelf->gAdapter)
	{
		pSelf->gAdapter->registered = false;
	}

	return CELL_OK;
}

s32 cellSailPlayerRegisterSource()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerGetRegisteredProtocols()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerSetSoundAdapter(vm::ptr<CellSailPlayer> pSelf, u32 index, vm::ptr<CellSailSoundAdapter> pAdapter)
{
	cellSail.Warning("cellSailPlayerSetSoundAdapter(pSelf=*0x%x, index=%d, pAdapter=*0x%x)", pSelf, index, pAdapter);

	if (index > pSelf->attribute.maxAudioStreamNum)
	{
		return CELL_SAIL_ERROR_INVALID_ARG;
	}

	pSelf->sAdapter = pAdapter;
	pAdapter->index = index;
	pAdapter->registered = true;

	return CELL_OK;
}

s32 cellSailPlayerSetGraphicsAdapter(vm::ptr<CellSailPlayer> pSelf, u32 index, vm::ptr<CellSailGraphicsAdapter> pAdapter)
{
	cellSail.Warning("cellSailPlayerSetGraphicsAdapter(pSelf=*0x%x, index=%d, pAdapter=*0x%x)", pSelf, index, pAdapter);

	if (index > pSelf->attribute.maxVideoStreamNum)
	{
		return CELL_SAIL_ERROR_INVALID_ARG;
	}

	pSelf->gAdapter = pAdapter;
	pAdapter->index = index;
	pAdapter->registered = true;

	return CELL_OK;
}

s32 cellSailPlayerSetAuReceiver()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerSetRendererAudio()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerSetRendererVideo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerSetParameter(vm::ptr<CellSailPlayer> pSelf, s32 parameterType, u64 param0, u64 param1)
{
	cellSail.Warning("cellSailPlayerSetParameter(pSelf=*0x%x, parameterType=0x%x, param0=0x%llx, param1=0x%llx)", pSelf, parameterType, param0, param1);

	switch (parameterType)
	{
	case CELL_SAIL_PARAMETER_GRAPHICS_ADAPTER_BUFFER_RELEASE_DELAY: pSelf->graphics_adapter_buffer_release_delay = param1; break; // TODO: Stream index
	case CELL_SAIL_PARAMETER_CONTROL_PPU_THREAD_STACK_SIZE:         pSelf->control_ppu_thread_stack_size = param0; break;
	case CELL_SAIL_PARAMETER_ENABLE_APOST_SRC:                      pSelf->enable_apost_src = param1; break; // TODO: Stream index
	default: cellSail.Todo("cellSailPlayerSetParameter(): unimplemented parameter %s", ParameterCodeToName(parameterType));
	}

	return CELL_OK;
}

s32 cellSailPlayerGetParameter(vm::ptr<CellSailPlayer> pSelf, s32 parameterType, vm::ptr<u64> pParam0, vm::ptr<u64> pParam1)
{
	cellSail.Todo("cellSailPlayerGetParameter(pSelf=*0x%x, parameterType=0x%x, param0=*0x%x, param1=*0x%x)", pSelf, parameterType, pParam0, pParam1);

	switch (parameterType)
	{
	default: cellSail.Error("cellSailPlayerGetParameter(): unimplemented parameter %s", ParameterCodeToName(parameterType));
	}

	return CELL_OK;
}

s32 cellSailPlayerSubscribeEvent()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerUnsubscribeEvent()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerReplaceEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerBoot(PPUThread& ppu, vm::ptr<CellSailPlayer> pSelf, u64 userParam)
{
	cellSail.Warning("cellSailPlayerBoot(pSelf=*0x%x, userParam=%d)", pSelf, userParam);

	playerBoot(pSelf, userParam);

	return CELL_OK;
}

s32 cellSailPlayerAddDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> pDesc)
{
	cellSail.Warning("cellSailPlayerAddDescriptor(pSelf=*0x%x, pDesc=*0x%x)", pSelf, pDesc);

	if (pSelf && pSelf->descriptors < 3 && pDesc)
	{
		pSelf->descriptors++;
		pSelf->registeredDescriptors[pSelf->descriptors] = pDesc;
		pDesc->registered = true;
	}
	else
	{
		cellSail.Error("Descriptor limit reached or the descriptor is unspecified! This should never happen, report this to a developer.");
	}

	return CELL_OK;
}

s32 cellSailPlayerCreateDescriptor(vm::ptr<CellSailPlayer> pSelf, s32 streamType, vm::ptr<void> pMediaInfo, vm::cptr<char> pUri, vm::pptr<CellSailDescriptor> ppDesc)
{
	cellSail.Todo("cellSailPlayerCreateDescriptor(pSelf=*0x%x, streamType=%d, pMediaInfo=*0x%x, pUri=*0x%x, ppDesc=**0x%x)", pSelf, streamType, pMediaInfo, pUri, ppDesc);
	
	u32 descriptorAddress = vm::alloc(sizeof(CellSailDescriptor), vm::main);
	auto descriptor = vm::ptr<CellSailDescriptor>::make(descriptorAddress);
	*ppDesc = descriptor;
	descriptor->streamType = streamType;
	descriptor->registered = false;

	//pSelf->descriptors = 0;
	pSelf->repeatMode = 0;

	switch (streamType)
	{
		case CELL_SAIL_STREAM_PAMF:
		{
			std::string uri = pUri.get_ptr();
			if (uri.substr(0, 12) == "x-cell-fs://")
			{
				std::string path = uri.substr(12);
				vfsFile f;
				if (f.Open(path))
				{
					u64 size = f.GetSize();
					u32 buffer = vm::alloc(size, vm::main);
					auto bufPtr = vm::cptr<PamfHeader>::make(buffer);
					PamfHeader *buf = const_cast<PamfHeader*>(bufPtr.get_ptr());
					assert(f.Read(buf, size) == size);
					u32 sp_ = vm::alloc(sizeof(CellPamfReader), vm::main);
					auto sp = vm::ptr<CellPamfReader>::make(sp_);
					u32 reader = cellPamfReaderInitialize(sp, bufPtr, size, 0);

					descriptor->buffer = buffer;
					descriptor->sp_ = sp_;
				}
				else
				{
					cellSail.Warning("Couldn't open PAMF: %s", uri.c_str());
				}
			}
			else
			{
				cellSail.Warning("Unhandled uri: %s", uri.c_str());
			}
			break;
		}
		default:
			cellSail.Error("Unhandled stream type: %d", streamType);
	}

	return CELL_OK;
}

s32 cellSailPlayerDestroyDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> pDesc)
{
	cellSail.Todo("cellSailPlayerAddDescriptor(pSelf=*0x%x, pDesc=*0x%x)", pSelf, pDesc);

	if (pDesc->registered)
		return CELL_SAIL_ERROR_INVALID_STATE;

	return CELL_OK;
}

s32 cellSailPlayerRemoveDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> ppDesc)
{
	cellSail.Warning("cellSailPlayerAddDescriptor(pSelf=*0x%x, pDesc=*0x%x)", pSelf, ppDesc);

	if (pSelf->descriptors > 0)
	{
		ppDesc = pSelf->registeredDescriptors[pSelf->descriptors];
		delete &pSelf->registeredDescriptors[pSelf->descriptors];
		pSelf->descriptors--;
	}

	return pSelf->descriptors;
}

s32 cellSailPlayerGetDescriptorCount(vm::ptr<CellSailPlayer> pSelf)
{
	cellSail.Warning("cellSailPlayerGetDescriptorCount(pSelf=*0x%x)", pSelf);
	return pSelf->descriptors;
}

s32 cellSailPlayerGetCurrentDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerOpenStream()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerCloseStream()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerOpenEsAudio()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerOpenEsVideo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerOpenEsUser()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerReopenEsAudio()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerReopenEsVideo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerReopenEsUser()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerCloseEsAudio()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerCloseEsVideo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerCloseEsUser()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerStart()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerStop()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerNext()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerCancel()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerSetPaused(vm::ptr<CellSailPlayer> pSelf, b8 paused)
{
	cellSail.Todo("cellSailPlayerSetPaused(pSelf=*0x%x, paused=%d)", pSelf, paused);
	return CELL_OK;
}

s32 cellSailPlayerIsPaused(vm::ptr<CellSailPlayer> pSelf)
{
	cellSail.Warning("cellSailPlayerIsPaused(pSelf=*0x%x)", pSelf);
	return pSelf->paused;
}

s32 cellSailPlayerSetRepeatMode(vm::ptr<CellSailPlayer> pSelf, s32 repeatMode, vm::ptr<CellSailStartCommand> pCommand)
{
	cellSail.Warning("cellSailPlayerSetRepeatMode(pSelf=*0x%x, repeatMode=%d, pCommand=*0x%x)", pSelf, repeatMode, pCommand);

	pSelf->repeatMode = repeatMode;
	pSelf->playbackCommand = pCommand;

	return pSelf->repeatMode;
}

s32 cellSailPlayerGetRepeatMode(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailStartCommand> pCommand)
{
	cellSail.Warning("cellSailPlayerGetRepeatMode(pSelf=*0x%x, pCommand=*0x%x)", pSelf, pCommand);

	pCommand = pSelf->playbackCommand;

	return pSelf->repeatMode;
}

s32 cellSailPlayerSetEsAudioMuted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerSetEsVideoMuted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerIsEsAudioMuted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerIsEsVideoMuted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerDumpImage()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerUnregisterSource()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

Module cellSail("cellSail", []()
{
	REG_FUNC(cellSail, cellSailMemAllocatorInitialize);

	REG_FUNC(cellSail, cellSailFutureInitialize);
	REG_FUNC(cellSail, cellSailFutureFinalize);
	REG_FUNC(cellSail, cellSailFutureReset);
	REG_FUNC(cellSail, cellSailFutureSet);
	REG_FUNC(cellSail, cellSailFutureGet);
	REG_FUNC(cellSail, cellSailFutureIsDone);

	REG_FUNC(cellSail, cellSailDescriptorGetStreamType);
	REG_FUNC(cellSail, cellSailDescriptorGetUri);
	REG_FUNC(cellSail, cellSailDescriptorGetMediaInfo);
	REG_FUNC(cellSail, cellSailDescriptorSetAutoSelection);
	REG_FUNC(cellSail, cellSailDescriptorIsAutoSelection);
	REG_FUNC(cellSail, cellSailDescriptorCreateDatabase);
	REG_FUNC(cellSail, cellSailDescriptorDestroyDatabase);
	REG_FUNC(cellSail, cellSailDescriptorOpen);
	REG_FUNC(cellSail, cellSailDescriptorClose);
	REG_FUNC(cellSail, cellSailDescriptorSetEs);
	REG_FUNC(cellSail, cellSailDescriptorClearEs);
	REG_FUNC(cellSail, cellSailDescriptorGetCapabilities);
	REG_FUNC(cellSail, cellSailDescriptorInquireCapability);
	REG_FUNC(cellSail, cellSailDescriptorSetParameter);

	REG_FUNC(cellSail, cellSailSoundAdapterInitialize);
	REG_FUNC(cellSail, cellSailSoundAdapterFinalize);
	REG_FUNC(cellSail, cellSailSoundAdapterSetPreferredFormat);
	REG_FUNC(cellSail, cellSailSoundAdapterGetFrame);
	REG_FUNC(cellSail, cellSailSoundAdapterGetFormat);
	REG_FUNC(cellSail, cellSailSoundAdapterUpdateAvSync);
	REG_FUNC(cellSail, cellSailSoundAdapterPtsToTimePosition);

	REG_FUNC(cellSail, cellSailGraphicsAdapterInitialize);
	REG_FUNC(cellSail, cellSailGraphicsAdapterFinalize);
	REG_FUNC(cellSail, cellSailGraphicsAdapterSetPreferredFormat);
	REG_FUNC(cellSail, cellSailGraphicsAdapterGetFrame);
	REG_FUNC(cellSail, cellSailGraphicsAdapterGetFrame2);
	REG_FUNC(cellSail, cellSailGraphicsAdapterGetFormat);
	REG_FUNC(cellSail, cellSailGraphicsAdapterUpdateAvSync);
	REG_FUNC(cellSail, cellSailGraphicsAdapterPtsToTimePosition);

	REG_FUNC(cellSail, cellSailAuReceiverInitialize);
	REG_FUNC(cellSail, cellSailAuReceiverFinalize);
	REG_FUNC(cellSail, cellSailAuReceiverGet);

	REG_FUNC(cellSail, cellSailRendererAudioInitialize);
	REG_FUNC(cellSail, cellSailRendererAudioFinalize);
	REG_FUNC(cellSail, cellSailRendererAudioNotifyCallCompleted);
	REG_FUNC(cellSail, cellSailRendererAudioNotifyFrameDone);
	REG_FUNC(cellSail, cellSailRendererAudioNotifyOutputEos);

	REG_FUNC(cellSail, cellSailRendererVideoInitialize);
	REG_FUNC(cellSail, cellSailRendererVideoFinalize);
	REG_FUNC(cellSail, cellSailRendererVideoNotifyCallCompleted);
	REG_FUNC(cellSail, cellSailRendererVideoNotifyFrameDone);
	REG_FUNC(cellSail, cellSailRendererVideoNotifyOutputEos);

	REG_FUNC(cellSail, cellSailSourceInitialize);
	REG_FUNC(cellSail, cellSailSourceFinalize);
	REG_FUNC(cellSail, cellSailSourceNotifyCallCompleted);
	REG_FUNC(cellSail, cellSailSourceNotifyInputEos);
	REG_FUNC(cellSail, cellSailSourceNotifyStreamOut);
	REG_FUNC(cellSail, cellSailSourceNotifySessionError);
	REG_FUNC(cellSail, cellSailSourceNotifyMediaStateChanged);
	REG_FUNC(cellSail, cellSailSourceSetDiagHandler);

	{
		// these functions shouldn't exist
		REG_FUNC(cellSail, cellSailSourceNotifyOpenCompleted);
		REG_FUNC(cellSail, cellSailSourceNotifyStartCompleted);
		REG_FUNC(cellSail, cellSailSourceNotifyStopCompleted);
		REG_FUNC(cellSail, cellSailSourceNotifyReadCompleted);
		REG_FUNC(cellSail, cellSailSourceNotifyCloseCompleted);
	}

	REG_FUNC(cellSail, cellSailMp4MovieGetBrand);
	REG_FUNC(cellSail, cellSailMp4MovieIsCompatibleBrand);
	REG_FUNC(cellSail, cellSailMp4MovieGetMovieInfo);
	REG_FUNC(cellSail, cellSailMp4MovieGetTrackByIndex);
	REG_FUNC(cellSail, cellSailMp4MovieGetTrackById);
	REG_FUNC(cellSail, cellSailMp4MovieGetTrackByTypeAndIndex);
	REG_FUNC(cellSail, cellSailMp4TrackGetTrackInfo);
	REG_FUNC(cellSail, cellSailMp4TrackGetTrackReferenceCount);
	REG_FUNC(cellSail, cellSailMp4TrackGetTrackReference);

	REG_FUNC(cellSail, cellSailAviMovieGetMovieInfo);
	REG_FUNC(cellSail, cellSailAviMovieGetStreamByIndex);
	REG_FUNC(cellSail, cellSailAviMovieGetStreamByTypeAndIndex);
	REG_FUNC(cellSail, cellSailAviMovieGetHeader);
	REG_FUNC(cellSail, cellSailAviStreamGetMediaType);
	REG_FUNC(cellSail, cellSailAviStreamGetHeader);

	REG_FUNC(cellSail, cellSailPlayerInitialize);
	REG_FUNC(cellSail, cellSailPlayerInitialize2);
	REG_FUNC(cellSail, cellSailPlayerFinalize);
	REG_FUNC(cellSail, cellSailPlayerRegisterSource);
	REG_FUNC(cellSail, cellSailPlayerGetRegisteredProtocols);
	REG_FUNC(cellSail, cellSailPlayerSetSoundAdapter);
	REG_FUNC(cellSail, cellSailPlayerSetGraphicsAdapter);
	REG_FUNC(cellSail, cellSailPlayerSetAuReceiver);
	REG_FUNC(cellSail, cellSailPlayerSetRendererAudio);
	REG_FUNC(cellSail, cellSailPlayerSetRendererVideo);
	REG_FUNC(cellSail, cellSailPlayerSetParameter);
	REG_FUNC(cellSail, cellSailPlayerGetParameter);
	REG_FUNC(cellSail, cellSailPlayerSubscribeEvent);
	REG_FUNC(cellSail, cellSailPlayerUnsubscribeEvent);
	REG_FUNC(cellSail, cellSailPlayerReplaceEventHandler);
	REG_FUNC(cellSail, cellSailPlayerBoot);
	REG_FUNC(cellSail, cellSailPlayerCreateDescriptor);
	REG_FUNC(cellSail, cellSailPlayerDestroyDescriptor);
	REG_FUNC(cellSail, cellSailPlayerAddDescriptor);
	REG_FUNC(cellSail, cellSailPlayerRemoveDescriptor);
	REG_FUNC(cellSail, cellSailPlayerGetDescriptorCount);
	REG_FUNC(cellSail, cellSailPlayerGetCurrentDescriptor);
	REG_FUNC(cellSail, cellSailPlayerOpenStream);
	REG_FUNC(cellSail, cellSailPlayerCloseStream);
	REG_FUNC(cellSail, cellSailPlayerOpenEsAudio);
	REG_FUNC(cellSail, cellSailPlayerOpenEsVideo);
	REG_FUNC(cellSail, cellSailPlayerOpenEsUser);
	REG_FUNC(cellSail, cellSailPlayerReopenEsAudio);
	REG_FUNC(cellSail, cellSailPlayerReopenEsVideo);
	REG_FUNC(cellSail, cellSailPlayerReopenEsUser);
	REG_FUNC(cellSail, cellSailPlayerCloseEsAudio);
	REG_FUNC(cellSail, cellSailPlayerCloseEsVideo);
	REG_FUNC(cellSail, cellSailPlayerCloseEsUser);
	REG_FUNC(cellSail, cellSailPlayerStart);
	REG_FUNC(cellSail, cellSailPlayerStop);
	REG_FUNC(cellSail, cellSailPlayerNext);
	REG_FUNC(cellSail, cellSailPlayerCancel);
	REG_FUNC(cellSail, cellSailPlayerSetPaused);
	REG_FUNC(cellSail, cellSailPlayerIsPaused);
	REG_FUNC(cellSail, cellSailPlayerSetRepeatMode);
	REG_FUNC(cellSail, cellSailPlayerGetRepeatMode);
	REG_FUNC(cellSail, cellSailPlayerSetEsAudioMuted);
	REG_FUNC(cellSail, cellSailPlayerSetEsVideoMuted);
	REG_FUNC(cellSail, cellSailPlayerIsEsAudioMuted);
	REG_FUNC(cellSail, cellSailPlayerIsEsVideoMuted);
	REG_FUNC(cellSail, cellSailPlayerDumpImage);
	REG_FUNC(cellSail, cellSailPlayerUnregisterSource);
});
