#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/FS/vfsFile.h"

#include "cellSail.h"
#include "cellPamf.h"

extern Module cellSail;

// TODO: Create an internal cellSail thread

s32 cellSailMemAllocatorInitialize(vm::ptr<CellSailMemAllocator> pSelf, vm::ptr<CellSailMemAllocatorFuncs> pCallbacks)
{
	cellSail.Warning("cellSailMemAllocatorInitialize(pSelf_addr=0x%x, pCallbacks_addr=0x%x)", pSelf.addr(), pCallbacks.addr());

	pSelf->callbacks = pCallbacks;

	return CELL_OK;
}

s32 cellSailFutureInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailFutureFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailFutureReset()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailFutureSet()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailFutureGet()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailFutureIsDone()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

s32 cellSailDescriptorSetAutoSelection(vm::ptr<CellSailDescriptor> pSelf, bool autoSelection)
{
	cellSail.Warning("cellSailDescriptorSetAutoSelection(pSelf_addr=0x%x, autoSelection=%s)", pSelf.addr(), autoSelection ? "true" : "false");

	if (pSelf) {
		pSelf->autoSelection = autoSelection;
		return autoSelection;
	}

	return CELL_OK;
}

s32 cellSailDescriptorIsAutoSelection(vm::ptr<CellSailDescriptor> pSelf)
{
	cellSail.Warning("cellSailDescriptorIsAutoSelection(pSelf_addr=0x%x)", pSelf.addr());
	
	if (pSelf)
		return pSelf->autoSelection;

	return CELL_OK;
}

s32 cellSailDescriptorCreateDatabase(vm::ptr<CellSailDescriptor> pSelf, vm::ptr<void> pDatabase, u32 size, u64 arg)
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

s32 cellSailSoundAdapterInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSoundAdapterFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSoundAdapterSetPreferredFormat()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSoundAdapterGetFrame()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSoundAdapterGetFormat()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

s32 cellSailGraphicsAdapterInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailGraphicsAdapterFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailGraphicsAdapterSetPreferredFormat()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailGraphicsAdapterGetFrame()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailGraphicsAdapterGetFrame2()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailGraphicsAdapterGetFormat()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

s32 cellSailSourceCheck()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyOpenCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyStartCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyStopCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyReadCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceSetDiagHandler()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailSourceNotifyCloseCompleted()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
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

s32 cellSailMp4ConvertTimeScale()
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

s32 cellSailPlayerInitialize2(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailMemAllocator> pAllocator, vm::ptr<CellSailPlayerFuncNotified> pCallback, u64 callbackArg,
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

s32 cellSailPlayerFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

s32 cellSailPlayerSetSoundAdapter()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerSetGraphicsAdapter()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

s32 cellSailPlayerSetParameter()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerGetParameter()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

s32 cellSailPlayerBoot()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

s32 cellSailPlayerAddDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> pDesc)
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

s32 cellSailPlayerCreateDescriptor(vm::ptr<CellSailPlayer> pSelf, s32 streamType, vm::ptr<u32> pMediaInfo, vm::cptr<char> pUri, vm::ptr<u32> ppDesc)
{
	cellSail.Warning("cellSailPlayerCreateDescriptor(pSelf_addr=0x%x, streamType=%d, pMediaInfo_addr=0x%x, pUri_addr=0x%x, ppDesc_addr=0x%x)", pSelf.addr(), streamType,
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
					u32 buf_ = Memory.Alloc(size, 1);
					auto bufPtr = vm::cptr<PamfHeader>::make(buf_);
					PamfHeader *buf = const_cast<PamfHeader*>(bufPtr.get_ptr());
					assert(f.Read(buf, size) == size);
					u32 sp_ = Memory.Alloc(sizeof(CellPamfReader), 1);
					auto sp = vm::ptr<CellPamfReader>::make(sp_);
					u32 r = cellPamfReaderInitialize(sp, bufPtr, size, 0);

					descriptor->internalData[0] = buf_;
					descriptor->internalData[1] = sp_;
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

	//cellSail.Todo("pSelf_addr=0x%x, pDesc_addr=0x%x", pSelf.addr(), descriptor.addr());
	//cellSailPlayerAddDescriptor(pSelf, ppDesc);

	return CELL_OK;
}

s32 cellSailPlayerDestroyDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> pDesc)
{
	cellSail.Todo("cellSailPlayerAddDescriptor(pSelf_addr=0x%x, pDesc_addr=0x%x)", pSelf.addr(), pDesc.addr());

	if (pDesc->registered)
		return CELL_SAIL_ERROR_INVALID_STATE;

	return CELL_OK;
}

s32 cellSailPlayerRemoveDescriptor(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailDescriptor> ppDesc)
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

s32 cellSailPlayerGetDescriptorCount(vm::ptr<CellSailPlayer> pSelf)
{
	cellSail.Warning("cellSailPlayerGetDescriptorCount(pSelf_addr=0x%x)", pSelf.addr());
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

s32 cellSailPlayerSetPaused(vm::ptr<CellSailPlayer> pSelf, bool paused)
{
	cellSail.Todo("cellSailPlayerSetPaused(pSelf_addr=0x%x, paused=%d)", pSelf.addr(), paused);
	return CELL_OK;
}

s32 cellSailPlayerIsPaused(vm::ptr<CellSailPlayer> pSelf)
{
	cellSail.Warning("cellSailPlayerIsPaused(pSelf_addr=0x%x)", pSelf.addr());
	return pSelf->paused;
}

s32 cellSailPlayerSetRepeatMode(vm::ptr<CellSailPlayer> pSelf, s32 repeatMode, vm::ptr<CellSailStartCommand> pCommand)
{
	cellSail.Warning("cellSailPlayerSetRepeatMode(pSelf_addr=0x%x, repeatMode=%d, pCommand_addr=0x%x)", pSelf.addr(), repeatMode, pCommand.addr());

	pSelf->repeatMode = repeatMode;
	pSelf->playbackCommand = pCommand;

	return pSelf->repeatMode;
}

s32 cellSailPlayerGetRepeatMode(vm::ptr<CellSailPlayer> pSelf, vm::ptr<CellSailStartCommand> pCommand)
{
	cellSail.Warning("cellSailPlayerGetRepeatMode(pSelf_addr=0x%x, pCommand_addr=0x%x)", pSelf.addr(), pCommand.addr());

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
