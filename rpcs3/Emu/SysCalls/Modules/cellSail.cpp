#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellSail_init();
Module cellSail(0x001d, cellSail_init);

// Error Codes
enum
{
	CELL_SAIL_ERROR_INVALID_ARG        = 0x80610701,
	CELL_SAIL_ERROR_INVALID_STATE      = 0x80610702,
	CELL_SAIL_ERROR_UNSUPPORTED_STREAM = 0x80610703,
	CELL_SAIL_ERROR_INDEX_OUT_OF_RANGE = 0x80610704,
	CELL_SAIL_ERROR_EMPTY              = 0x80610705,
	CELL_SAIL_ERROR_FULLED             = 0x80610706,
	CELL_SAIL_ERROR_USING              = 0x80610707,
	CELL_SAIL_ERROR_NOT_AVAILABLE      = 0x80610708,
	CELL_SAIL_ERROR_CANCEL             = 0x80610709,
	CELL_SAIL_ERROR_MEMORY             = 0x806107F0,
	CELL_SAIL_ERROR_INVALID_FD         = 0x806107F1,
	CELL_SAIL_ERROR_FATAL              = 0x806107FF,
};

int cellSailMemAllocatorInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

int cellSailDescriptorSetAutoSelection()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorIsAutoSelection()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailDescriptorCreateDatabase()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

int cellSailSoundAdapterInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSoundAdapterFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSoundAdapterSetPreferredFormat()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSoundAdapterGetFrame()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailSoundAdapterGetFormat()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

int cellSailGraphicsAdapterInitialize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailGraphicsAdapterFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailGraphicsAdapterSetPreferredFormat()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailGraphicsAdapterGetFrame()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailGraphicsAdapterGetFrame2()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailGraphicsAdapterGetFormat()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

int cellSailPlayerInitialize2()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerFinalize()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

int cellSailPlayerSetSoundAdapter()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSetGraphicsAdapter()
{
	UNIMPLEMENTED_FUNC(cellSail);
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

int cellSailPlayerCreateDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerDestroyDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerAddDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerRemoveDescriptor()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerGetDescriptorCount()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
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

int cellSailPlayerSetPaused()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerIsPaused()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerSetRepeatMode()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
}

int cellSailPlayerGetRepeatMode()
{
	UNIMPLEMENTED_FUNC(cellSail);
	return CELL_OK;
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

void cellSail_init()
{
	cellSail.AddFunc(0x346ebba3, cellSailMemAllocatorInitialize);

	cellSail.AddFunc(0x4cc54f8e, cellSailFutureInitialize);
	cellSail.AddFunc(0x9553af65, cellSailFutureFinalize);
	cellSail.AddFunc(0x0c4cb439, cellSailFutureReset);
	cellSail.AddFunc(0xa37fed15, cellSailFutureSet);
	cellSail.AddFunc(0x3a2d806c, cellSailFutureGet);
	cellSail.AddFunc(0x51ecf361, cellSailFutureIsDone);

	cellSail.AddFunc(0xd5f9a15b, cellSailDescriptorGetStreamType);
	cellSail.AddFunc(0x4c191088, cellSailDescriptorGetUri);
	cellSail.AddFunc(0xbd1635f4, cellSailDescriptorGetMediaInfo);
	cellSail.AddFunc(0x76b1a425, cellSailDescriptorSetAutoSelection);
	cellSail.AddFunc(0x277adf21, cellSailDescriptorIsAutoSelection);
	cellSail.AddFunc(0x0abb318b, cellSailDescriptorCreateDatabase);
	cellSail.AddFunc(0x28336e89, cellSailDescriptorDestroyDatabase);
	cellSail.AddFunc(0xc044fab1, cellSailDescriptorOpen);
	cellSail.AddFunc(0x15fd6a2a, cellSailDescriptorClose);
	cellSail.AddFunc(0x0d0c2f0c, cellSailDescriptorSetEs);
	cellSail.AddFunc(0xdf5553ef, cellSailDescriptorClearEs);
	cellSail.AddFunc(0xac9c3b1f, cellSailDescriptorGetCapabilities);
	cellSail.AddFunc(0x92590d52, cellSailDescriptorInquireCapability);
	cellSail.AddFunc(0xee94b99b, cellSailDescriptorSetParameter);

	cellSail.AddFunc(0x3d0d3b72, cellSailSoundAdapterInitialize);
	cellSail.AddFunc(0xd1462438, cellSailSoundAdapterFinalize);
	cellSail.AddFunc(0x1c9d5e5a, cellSailSoundAdapterSetPreferredFormat);
	cellSail.AddFunc(0x7eb8d6b5, cellSailSoundAdapterGetFrame);
	cellSail.AddFunc(0xf25f197d, cellSailSoundAdapterGetFormat);
	cellSail.AddFunc(0xeec22809, cellSailSoundAdapterUpdateAvSync);
	cellSail.AddFunc(0x4ae979df, cellSailSoundAdapterPtsToTimePosition);

	cellSail.AddFunc(0x1c983864, cellSailGraphicsAdapterInitialize);
	cellSail.AddFunc(0x76488bb1, cellSailGraphicsAdapterFinalize);
	cellSail.AddFunc(0x2e3ccb5e, cellSailGraphicsAdapterSetPreferredFormat);
	cellSail.AddFunc(0x0247c69e, cellSailGraphicsAdapterGetFrame);
	cellSail.AddFunc(0x018281a8, cellSailGraphicsAdapterGetFrame2);
	cellSail.AddFunc(0xffd58aa4, cellSailGraphicsAdapterGetFormat);
	cellSail.AddFunc(0x44a20e79, cellSailGraphicsAdapterUpdateAvSync);
	cellSail.AddFunc(0x1872331b, cellSailGraphicsAdapterPtsToTimePosition);

	cellSail.AddFunc(0x3dd9639a, cellSailAuReceiverInitialize);
	cellSail.AddFunc(0xed58e3ec, cellSailAuReceiverFinalize);
	cellSail.AddFunc(0x3a1132ed, cellSailAuReceiverGet);

	cellSail.AddFunc(0x67b4d01f, cellSailRendererAudioInitialize);
	cellSail.AddFunc(0x06dd4174, cellSailRendererAudioFinalize);
	cellSail.AddFunc(0xb7b4ecee, cellSailRendererAudioNotifyCallCompleted);
	cellSail.AddFunc(0xf841a537, cellSailRendererAudioNotifyFrameDone);
	cellSail.AddFunc(0x325039b9, cellSailRendererAudioNotifyOutputEos);

	cellSail.AddFunc(0x8d1ff475, cellSailRendererVideoInitialize);
	cellSail.AddFunc(0x47055fea, cellSailRendererVideoFinalize);
	cellSail.AddFunc(0x954f48f8, cellSailRendererVideoNotifyCallCompleted);
	cellSail.AddFunc(0x5f77e8df, cellSailRendererVideoNotifyFrameDone);
	cellSail.AddFunc(0xdff1cda2, cellSailRendererVideoNotifyOutputEos);

	cellSail.AddFunc(0x9d30bdce, cellSailSourceInitialize);
	cellSail.AddFunc(0xee724c99, cellSailSourceFinalize);
	cellSail.AddFunc(0x764ec2d2, cellSailSourceNotifyCallCompleted);
	cellSail.AddFunc(0x54c53688, cellSailSourceNotifyInputEos);
	cellSail.AddFunc(0x95ee1695, cellSailSourceNotifyStreamOut);
	cellSail.AddFunc(0xf289f0cd, cellSailSourceNotifySessionError);
	cellSail.AddFunc(0xf4009a94, cellSailSourceNotifyMediaStateChanged);
	//cellSail.AddFunc(, cellSailSourceCheck);
	cellSail.AddFunc(0x3df98d41, cellSailSourceNotifyOpenCompleted);
	cellSail.AddFunc(0x640c7278, cellSailSourceNotifyStartCompleted);
	cellSail.AddFunc(0x7473970a, cellSailSourceNotifyStopCompleted);
	cellSail.AddFunc(0x946ecca0, cellSailSourceNotifyReadCompleted);
	cellSail.AddFunc(0xbdb2251a, cellSailSourceSetDiagHandler);
	cellSail.AddFunc(0xc457b203, cellSailSourceNotifyCloseCompleted);

	cellSail.AddFunc(0xb980b76e, cellSailMp4MovieGetBrand);
	cellSail.AddFunc(0xd4049de0, cellSailMp4MovieIsCompatibleBrand);
	cellSail.AddFunc(0x5783a454, cellSailMp4MovieGetMovieInfo);
	cellSail.AddFunc(0x5faf802b, cellSailMp4MovieGetTrackByIndex);
	cellSail.AddFunc(0x85b07126, cellSailMp4MovieGetTrackById);
	cellSail.AddFunc(0xc2d90ec9, cellSailMp4MovieGetTrackByTypeAndIndex);
	cellSail.AddFunc(0xa48be428, cellSailMp4TrackGetTrackInfo);
	cellSail.AddFunc(0x72236ec1, cellSailMp4TrackGetTrackReferenceCount);
	cellSail.AddFunc(0x5f44f64f, cellSailMp4TrackGetTrackReference);
	//cellSail.AddFunc(, cellSailMp4ConvertTimeScale);

	cellSail.AddFunc(0x6e83f5c0, cellSailAviMovieGetMovieInfo);
	cellSail.AddFunc(0x3e908c56, cellSailAviMovieGetStreamByIndex);
	cellSail.AddFunc(0xddebd2a5, cellSailAviMovieGetStreamByTypeAndIndex);
	cellSail.AddFunc(0x10298371, cellSailAviMovieGetHeader);
	cellSail.AddFunc(0xc09e2f23, cellSailAviStreamGetMediaType);
	cellSail.AddFunc(0xcc3cca60, cellSailAviStreamGetHeader);

	cellSail.AddFunc(0x17932b26, cellSailPlayerInitialize);
	cellSail.AddFunc(0x23654375, cellSailPlayerInitialize2);
	cellSail.AddFunc(0x18b4629d, cellSailPlayerFinalize);
	cellSail.AddFunc(0xbedccc74, cellSailPlayerRegisterSource);
	cellSail.AddFunc(0x186b98d3, cellSailPlayerGetRegisteredProtocols);
	cellSail.AddFunc(0x1139a206, cellSailPlayerSetSoundAdapter);
	cellSail.AddFunc(0x18bcd21b, cellSailPlayerSetGraphicsAdapter);
	cellSail.AddFunc(0xf5747e1f, cellSailPlayerSetAuReceiver);
	cellSail.AddFunc(0x92eaf6ca, cellSailPlayerSetRendererAudio);
	cellSail.AddFunc(0xecf56150, cellSailPlayerSetRendererVideo);
	cellSail.AddFunc(0x5f7c7a6f, cellSailPlayerSetParameter);
	cellSail.AddFunc(0x952269c9, cellSailPlayerGetParameter);
	cellSail.AddFunc(0x6f0b1002, cellSailPlayerSubscribeEvent);
	cellSail.AddFunc(0x69793952, cellSailPlayerUnsubscribeEvent);
	cellSail.AddFunc(0x47632810, cellSailPlayerReplaceEventHandler);
	cellSail.AddFunc(0xbdf21b0f, cellSailPlayerBoot);
	cellSail.AddFunc(0xd7938b8d, cellSailPlayerCreateDescriptor);
	cellSail.AddFunc(0xfc839bd4, cellSailPlayerDestroyDescriptor);
	cellSail.AddFunc(0x7c8dff3b, cellSailPlayerAddDescriptor);
	cellSail.AddFunc(0x9897fbd1, cellSailPlayerRemoveDescriptor);
	cellSail.AddFunc(0x752f8585, cellSailPlayerGetDescriptorCount);
	cellSail.AddFunc(0x75fca288, cellSailPlayerGetCurrentDescriptor);
	cellSail.AddFunc(0x34ecc1b9, cellSailPlayerOpenStream);
	cellSail.AddFunc(0x85beffcc, cellSailPlayerCloseStream);
	cellSail.AddFunc(0x145f9b11, cellSailPlayerOpenEsAudio);
	cellSail.AddFunc(0x477501f6, cellSailPlayerOpenEsVideo);
	cellSail.AddFunc(0xa849d0a7, cellSailPlayerOpenEsUser);
	cellSail.AddFunc(0x4fa5ad09, cellSailPlayerReopenEsAudio);
	cellSail.AddFunc(0xf60a8a69, cellSailPlayerReopenEsVideo);
	cellSail.AddFunc(0x7b6fa92e, cellSailPlayerReopenEsUser);
	cellSail.AddFunc(0xbf9b8d72, cellSailPlayerCloseEsAudio);
	cellSail.AddFunc(0x07924359, cellSailPlayerCloseEsVideo);
	cellSail.AddFunc(0xaed9d6cd, cellSailPlayerCloseEsUser);
	cellSail.AddFunc(0xe535b0d3, cellSailPlayerStart);
	cellSail.AddFunc(0xeba8d4ec, cellSailPlayerStop);
	cellSail.AddFunc(0x26563ddc, cellSailPlayerNext);
	cellSail.AddFunc(0x950d53c1, cellSailPlayerCancel);
	cellSail.AddFunc(0xd1d55a90, cellSailPlayerSetPaused);
	cellSail.AddFunc(0xaafa17b8, cellSailPlayerIsPaused);
	cellSail.AddFunc(0xfc5baf8a, cellSailPlayerSetRepeatMode);
	cellSail.AddFunc(0x38144ecf, cellSailPlayerGetRepeatMode);
	cellSail.AddFunc(0x91d287f6, cellSailPlayerSetEsAudioMuted);
	cellSail.AddFunc(0xf1446a40, cellSailPlayerSetEsVideoMuted);
	cellSail.AddFunc(0x09de25fd, cellSailPlayerIsEsAudioMuted);
	cellSail.AddFunc(0xdbe32ed4, cellSailPlayerIsEsVideoMuted);
	cellSail.AddFunc(0xcc987ba6, cellSailPlayerDumpImage);
	cellSail.AddFunc(0x025b4974, cellSailPlayerUnregisterSource);
}