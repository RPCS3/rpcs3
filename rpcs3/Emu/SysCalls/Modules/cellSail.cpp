#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/FS/vfsFile.h"

#include "cellSail.h"
#include "cellPamf.h"

extern Module cellSail;

int cellSailMemAllocatorInitialize(vm::ptr<CellSailMemAllocator> pSelf, vm::ptr<CellSailMemAllocatorFuncs> pCallbacks)
{
	cellSail.Warning("cellSailMemAllocatorInitialize(pSelf_addr=0x%x, pCallbacks_addr=0x%x)", pSelf.addr(), pCallbacks.addr());

	pSelf->callbacks = pCallbacks;
	// TODO: Create a cellSail thread

	return CELL_OK;
}

int cellSailFutureInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailFutureFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailFutureReset()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailFutureSet()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailFutureGet()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailFutureIsDone()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorGetStreamType()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorGetUri()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorGetMediaInfo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorSetAutoSelection(vm::ptr<CellSailDescriptor> pSelf, bool autoSelection)
{
	cellSail.Warning("cellSailDescriptorSetAutoSelection(pSelf_addr=0x%x, autoSelection=%s)", pSelf.addr(), autoSelection ? "true" : "false");

	if (pSelf) {
		pSelf->autoSelection = autoSelection;
		return autoSelection;
	}

	return CELL_OK;
}

int cellSailDescriptorIsAutoSelection(vm::ptr<CellSailDescriptor> pSelf)
{
	cellSail.Warning("cellSailDescriptorIsAutoSelection(pSelf_addr=0x%x)", pSelf.addr());
	
	if (pSelf)
		return pSelf->autoSelection;

	return CELL_OK;
}

int cellSailDescriptorCreateDatabase(vm::ptr<CellSailDescriptor> pSelf, vm::ptr<void> pDatabase, u32 size, u64 arg)
{
	cellSail.Warning("cellSailDescriptorCreateDatabase(pSelf=0x%x, pDatabase=0x%x, size=0x%x, arg=0x%x", pSelf.addr(), pDatabase.addr(), size, arg);

	switch ((s32)pSelf->streamType) {
		case CELL_SAIL_STREAM_PAMF:
		{
			u32 addr = pSelf->internalData[1];
			auto ptr = vm::ptr<CellPamfReader>::make(addr);
			memcpy(pDatabase.get_ptr(), ptr.get_ptr(), sizeof(CellPamfReader));
			break;
		}
		default:
			cellSail.Error("Unhandled stream type: %d", pSelf->streamType);
	}

	return CELL_OK;
}

int cellSailDescriptorDestroyDatabase()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorOpen()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorClose()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorSetEs()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorClearEs()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorGetCapabilities()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorInquireCapability()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorSetParameter()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSoundAdapterInitialize(vm::ptr<CellSailSoundAdapter> pSelf, const vm::ptr<CellSailSoundAdapterFuncs> pCallbacks, vm::ptr<u32> pArg)
{
	cellSail.Warning("cellSailSoundAdapterInitialize(pSelf_addr=0x%x, pCallbacks_addr=0x%x, pArg=0x%x)", pSelf.addr(), pCallbacks.addr(), pArg.addr());

	pSelf->callbacks = pCallbacks;
	pSelf->arg = be_t<u32>::make(pArg.addr());
	pSelf->initialized = true;

	return CELL_OK;
}

int cellSailSoundAdapterFinalize(vm::ptr<CellSailSoundAdapter> pSelf)
{
	cellSail.Warning("cellSailSoundAdapterFinalize(pSelf_addr=0x%x)", pSelf.addr());

	if (!pSelf->initialized)
		return CELL_SAIL_ERROR_INVALID_STATE;

	if (pSelf->registered)
		return CELL_SAIL_ERROR_INVALID_STATE;

	Memory.Free(pSelf.addr()); // Is it right to free the sound adapter?

	return CELL_OK;
}

int cellSailSoundAdapterSetPreferredFormat(vm::ptr<CellSailSoundAdapter> pSelf, const vm::ptr<CellSailAudioFormat> pFormat)
{
	cellSail.Warning("cellSailSoundAdapterSetPreferredFormat(pSelf_addr=0x%x, pFormat_addr=0x%x)", pSelf.addr(), pFormat.addr());

	pSelf->format = *pFormat.get_ptr();

	return CELL_OK;
}

int cellSailSoundAdapterGetFrame(vm::ptr<CellSailSoundAdapter> pSelf, u32 samples, vm::ptr<CellSailSoundFrameInfo> pInfo)
{
	cellSail.Todo("cellSailSoundAdapterGetFrame(pSelf_addr=0x%x, samples=%d, pInfo_addr=0x%x)", pSelf.addr(), samples, pInfo.addr());

	if (!pSelf->initialized)
		return CELL_SAIL_ERROR_INVALID_STATE;

	if (samples > 2048)
		return CELL_SAIL_ERROR_INVALID_ARG;

	pInfo->tag = 0xDEFECA7E;

	return CELL_OK;
}

int cellSailSoundAdapterGetFormat(vm::ptr<CellSailSoundAdapter> pSelf, vm::ptr<CellSailAudioFormat> pFormat)
{
	cellSail.Todo("cellSailSoundAdapterGetFormat(pSelf_addr=0x%x, pFormat_addr=0x%x)", pSelf.addr(), pFormat.addr());

	*pFormat = pSelf->format;

	return CELL_OK;
}

int cellSailSoundAdapterUpdateAvSync()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSoundAdapterPtsToTimePosition()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailGraphicsAdapterInitialize(vm::ptr<CellSailGraphicsAdapter> pSelf, const vm::ptr<CellSailGraphicsAdapterFuncs> pCallbacks, vm::ptr<u32> pArg)
{
	cellSail.Warning("cellSailGraphicsAdapterInitialize(pSelf_addr=0x%x, pCallbacks_addr=0x%x, pArg_addr=0x%x)", pSelf.addr(), pCallbacks.addr(), pArg.addr());

	pSelf->initialized = true;
	pSelf->callbacks = pCallbacks;
	pSelf->arg = be_t<u32>::make(pArg.addr());

	return CELL_OK;
}

int cellSailGraphicsAdapterFinalize(vm::ptr<CellSailGraphicsAdapter> pSelf)
{
	cellSail.Todo("cellSailGraphicsAdapterFinalize(pSelf_addr=0x%x)", pSelf.addr());

	if (!pSelf->initialized)
		return CELL_SAIL_ERROR_INVALID_STATE;

	if (pSelf->registered)
		return CELL_SAIL_ERROR_INVALID_STATE;

	Memory.Free(pSelf.addr()); // Is it right to free the graphics adapter?

	return CELL_OK;
}

int cellSailGraphicsAdapterSetPreferredFormat(vm::ptr<CellSailGraphicsAdapter> pSelf, const vm::ptr<CellSailVideoFormat> pFormat)
{
	cellSail.Warning("cellSailGraphicsAdapterSetPreferredFormat(pSelf_addr=0x%x, pFormat_addr=0x%x)");

	pSelf->format = *pFormat.get_ptr();

	return CELL_OK;
}

int cellSailGraphicsAdapterGetFrame(vm::ptr<CellSailGraphicsAdapter> pSelf, vm::ptr<CellSailGraphicsFrameInfo> pInfo)
{
	cellSail.Todo("cellSailGraphicsAdapterGetFrame(pSelf_addr=0x%x, pInfo_addr=0x%x)", pSelf.addr(), pInfo.addr());
	return CELL_OK;
}

int cellSailGraphicsAdapterGetFrame2(vm::ptr<CellSailGraphicsAdapter> pSelf, vm::ptr<CellSailGraphicsFrameInfo> pInfo, vm::ptr<CellSailGraphicsFrameInfo> pPrevInfo,
									 vm::ptr<u64> pFlipTime, u64 flags)
{
	cellSail.Todo("cellSailGraphicsAdapterGetFrame2(pSelf_addr=0x%x, pInfo_addr=0x%x, pPrevInfo_addr=0x%x, flipTime_addr=0x%x, flags=0x%d)", pSelf.addr(), pInfo.addr(),
				  pPrevInfo.addr(), pFlipTime.addr(), flags);
	return CELL_OK;
}

int cellSailGraphicsAdapterGetFormat(vm::ptr<CellSailGraphicsAdapter> pSelf, vm::ptr<CellSailVideoFormat> pFormat)
{
	cellSail.Warning("cellSailGraphicsAdapterGetFormat(pSelf_addr=0x%x, format_addr=0x%x)", pSelf.addr(), pFormat.addr());

	*pFormat = pSelf->format;

	return CELL_OK;
}

int cellSailGraphicsAdapterUpdateAvSync()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailGraphicsAdapterPtsToTimePosition()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAuReceiverInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAuReceiverFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAuReceiverGet()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererAudioInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererAudioFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererAudioNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererAudioNotifyFrameDone()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererAudioNotifyOutputEos()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererVideoInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererVideoFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererVideoNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererVideoNotifyFrameDone()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailRendererVideoNotifyOutputEos()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyCallCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyInputEos()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyStreamOut()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifySessionError()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyMediaStateChanged()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceCheck()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyOpenCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyStartCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyStopCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyReadCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceSetDiagHandler()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSourceNotifyCloseCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4MovieGetBrand()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4MovieIsCompatibleBrand()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4MovieGetMovieInfo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4MovieGetTrackByIndex()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4MovieGetTrackById()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4MovieGetTrackByTypeAndIndex()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4TrackGetTrackInfo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4TrackGetTrackReferenceCount()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4TrackGetTrackReference()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailMp4ConvertTimeScale()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAviMovieGetMovieInfo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAviMovieGetStreamByIndex()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAviMovieGetStreamByTypeAndIndex()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAviMovieGetHeader()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAviStreamGetMediaType()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailAviStreamGetHeader()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerInitialize2(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailMemAllocator> pAllocator, vm::ptr<CellSailPlayerFuncNotified> pCallback, u64 callbackArg,
	                          vm::ptr<CellSailPlayerAttribute> pAttribute, vm::ptr<CellSailPlayerResource> pResource)
{
	cellSail.Warning("cellSailPlayerInitialize2(pSelf_addr=0x%x, pAllocator_addr=0x%x, pCallback=0x%x, callbackArg=%d, pAttribute_addr=0x%x, pResource=0x%x)", pSelf.addr(),
		pAllocator.addr(), pCallback.addr(), callbackArg, pAttribute.addr(), pResource.addr());

	pSelf->allocator = pAllocator;
	pSelf->callback = pCallback;
	pSelf->callbackArgument = callbackArg;
	pSelf->attribute = pAttribute;
	pSelf->resource = pResource;

	return CELL_OK;
}

int cellSailPlayerFinalize(vm::ptr<CellSailPlayer> pSelf)
{
	cellSail.Todo("cellSailPlayerFinalize(pSelf_addr=0x%x)", pSelf.addr());

	pSelf->sAdapter->registered = false;
	pSelf->gAdapter->registered = false;
	//pSelf->sAdapter->callbacks->pCleanup(pSelf->sAdapter->arg); Use callback manager or something...
	//pSelf->gAdapter->callbacks->pCleanup(pSelf->gAdapter->arg);

	return CELL_OK;
}

int cellSailPlayerRegisterSource()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerGetRegisteredProtocols()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSetSoundAdapter(vm::ptr<CellSailPlayer> pSelf, s32 index, vm::ptr<CellSailSoundAdapter> pAdapter)
{
	cellSail.Warning("cellSailPlayerSetSoundAdapter(pSelf_addr=0x%x, index=%d, pAdapter_addr=0x%x)", pSelf.addr(), index, pAdapter.addr());

	if (index < 0 || index > pSelf->attribute->maxAudioStreamNum)
		return CELL_SAIL_ERROR_INVALID_ARG;

	pSelf->sAdapter = pAdapter;
	pAdapter->index = index;
	pAdapter->registered = true;

	return CELL_OK;
}

int cellSailPlayerSetGraphicsAdapter(vm::ptr<CellSailPlayer> pSelf, s32 index, vm::ptr<CellSailGraphicsAdapter> pAdapter)
{
	cellSail.Warning("cellSailPlayerSetGraphicsAdapter(pSelf_addr=0x%x, index=%d, pAdapter_addr=0x%x)", pSelf.addr(), index, pAdapter.addr());

	if (index < 0 || index > pSelf->attribute->maxVideoStreamNum)
		return CELL_SAIL_ERROR_INVALID_ARG;

	pSelf->gAdapter = pAdapter;
	pAdapter->index = index;
	pAdapter->registered = true;

	return CELL_OK;
}

int cellSailPlayerSetAuReceiver()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSetRendererAudio()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSetRendererVideo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSetParameter()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerGetParameter()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSubscribeEvent()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerUnsubscribeEvent()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerReplaceEventHandler()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerBoot()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerAddDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> pDesc)
{
	cellSail.Warning("cellSailPlayerAddDescriptor(pSelf_addr=0x%x, pDesc_addr=0x%x)", pSelf.addr(), pDesc.addr());

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

int cellSailPlayerCreateDescriptor(vm::ptr<CellSailPlayer> pSelf, s32 streamType, vm::ptr<u32> pMediaInfo, vm::ptr<const char> pUri, vm::ptr<u32> ppDesc)
{
	cellSail.Todo("cellSailPlayerCreateDescriptor(pSelf_addr=0x%x, streamType=%d, pMediaInfo_addr=0x%x, pUri_addr=0x%x, ppDesc_addr=0x%x)", pSelf.addr(), streamType,
					pMediaInfo.addr(), pUri.addr(), ppDesc.addr());
	
	u32 descriptorAddress = Memory.Alloc(sizeof(CellSailDescriptor), 1);
	auto descriptor = vm::ptr<CellSailDescriptor>::make(descriptorAddress);
	*ppDesc = descriptorAddress;
	descriptor->streamType = streamType;
	descriptor->registered = false;

	//pSelf->descriptors = 0;
	pSelf->repeatMode = 0;

	switch (streamType)
	{
		case CELL_SAIL_STREAM_PAMF:
		{
			std::string uri = pUri.get_ptr();
			if (uri.substr(0, 12) == "x-cell-fs://") {
				std::string path = uri.substr(12);
				vfsFile f;
				if (f.Open(path)) {
					u64 size = f.GetSize();
					u32 buffer = Memory.Alloc(size, 1);
					auto bufPtr = vm::ptr<const PamfHeader>::make(buffer);
					PamfHeader *buf = const_cast<PamfHeader*>(bufPtr.get_ptr());
					assert(f.Read(buf, size) == size);
					u32 sp_ = Memory.Alloc(sizeof(CellPamfReader), 1);
					auto sp = vm::ptr<CellPamfReader>::make(sp_);
					u32 reader = cellPamfReaderInitialize(sp, bufPtr, size, 0);

					descriptor->buffer = buffer;
					descriptor->sp_ = sp_;
				}
				else
					cellSail.Warning("Couldn't open PAMF: %s", uri.c_str());

			}
			else
				cellSail.Warning("Unhandled uri: %s", uri.c_str());
			break;
		}
		default:
			cellSail.Error("Unhandled stream type: %d", streamType);
	}

	// Make sure to add the descriptor to the pSelf.
	//cellSailPlayerAddDescriptor(pSelf, ppDesc);

	return CELL_OK;
}

int cellSailPlayerDestroyDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> pDesc)
{
	cellSail.Todo("cellSailPlayerAddDescriptor(pSelf_addr=0x%x, pDesc_addr=0x%x)", pSelf.addr(), pDesc.addr());

	if (pDesc->registered)
		return CELL_SAIL_ERROR_INVALID_STATE;

	return CELL_OK;
}

int cellSailPlayerRemoveDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> ppDesc)
{
	cellSail.Warning("cellSailPlayerAddDescriptor(pSelf_addr=0x%x, pDesc_addr=0x%x)", pSelf.addr(), ppDesc.addr());

	if (pSelf->descriptors > 0)
	{
		ppDesc = pSelf->registeredDescriptors[pSelf->descriptors];
		delete &pSelf->registeredDescriptors[pSelf->descriptors];
		pSelf->descriptors--;
	}

	return pSelf->descriptors;
}

int cellSailPlayerGetDescriptorCount(vm::ptr<CellSailPlayer> pSelf)
{
	cellSail.Warning("cellSailPlayerGetDescriptorCount(pSelf_addr=0x%x)", pSelf.addr());
	return pSelf->descriptors;
}

int cellSailPlayerGetCurrentDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerOpenStream()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerCloseStream()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerOpenEsAudio()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerOpenEsVideo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerOpenEsUser()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerReopenEsAudio()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerReopenEsVideo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerReopenEsUser()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerCloseEsAudio()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerCloseEsVideo()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerCloseEsUser()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerStart()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerStop()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerNext()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerCancel()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSetPaused(vm::ptr<CellSailPlayer> pSelf, bool paused)
{
	cellSail.Todo("cellSailPlayerSetPaused(pSelf_addr=0x%x, paused=%d)", pSelf.addr(), paused);
	return CELL_OK;
}

int cellSailPlayerIsPaused(vm::ptr<CellSailPlayer> pSelf)
{
	cellSail.Warning("cellSailPlayerIsPaused(pSelf_addr=0x%x)", pSelf.addr());
	return pSelf->paused;
}

int cellSailPlayerSetRepeatMode(vm::ptr<CellSailPlayer> pSelf, s32 repeatMode, vm::ptr<CellSailStartCommand> pCommand)
{
	cellSail.Warning("cellSailPlayerSetRepeatMode(pSelf_addr=0x%x, repeatMode=%d, pCommand_addr=0x%x)", pSelf.addr(), repeatMode, pCommand.addr());

	pSelf->repeatMode = repeatMode;
	pSelf->playbackCommand = pCommand;

	return pSelf->repeatMode;
}

int cellSailPlayerGetRepeatMode(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailStartCommand> pCommand)
{
	cellSail.Warning("cellSailPlayerGetRepeatMode(pSelf_addr=0x%x, pCommand_addr=0x%x)", pSelf.addr(), pCommand.addr());

	pCommand = pSelf->playbackCommand;

	return pSelf->repeatMode;
}

int cellSailPlayerSetEsAudioMuted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSetEsVideoMuted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerIsEsAudioMuted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerIsEsVideoMuted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerDumpImage()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerUnregisterSource()
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
	REG_FUNC(cellSail, cellSailSourceCheck);
	REG_FUNC(cellSail, cellSailSourceNotifyOpenCompleted);
	REG_FUNC(cellSail, cellSailSourceNotifyStartCompleted);
	REG_FUNC(cellSail, cellSailSourceNotifyStopCompleted);
	REG_FUNC(cellSail, cellSailSourceNotifyReadCompleted);
	REG_FUNC(cellSail, cellSailSourceSetDiagHandler);
	REG_FUNC(cellSail, cellSailSourceNotifyCloseCompleted);

	REG_FUNC(cellSail, cellSailMp4MovieGetBrand);
	REG_FUNC(cellSail, cellSailMp4MovieIsCompatibleBrand);
	REG_FUNC(cellSail, cellSailMp4MovieGetMovieInfo);
	REG_FUNC(cellSail, cellSailMp4MovieGetTrackByIndex);
	REG_FUNC(cellSail, cellSailMp4MovieGetTrackById);
	REG_FUNC(cellSail, cellSailMp4MovieGetTrackByTypeAndIndex);
	REG_FUNC(cellSail, cellSailMp4TrackGetTrackInfo);
	REG_FUNC(cellSail, cellSailMp4TrackGetTrackReferenceCount);
	REG_FUNC(cellSail, cellSailMp4TrackGetTrackReference);
	REG_FUNC(cellSail, cellSailMp4ConvertTimeScale);

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
