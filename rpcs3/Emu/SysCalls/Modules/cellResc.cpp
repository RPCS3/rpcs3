#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSysutil.h"
#include "Emu/RSX/sysutil_video.h"
#include "Emu/RSX/GSManager.h"
#include "cellResc.h"

extern Module cellResc;

extern s32 cellVideoOutConfigure(u32 videoOut, vm::ptr<CellVideoOutConfiguration> config, vm::ptr<CellVideoOutOption> option, u32 waitForEvent);
extern s32 cellGcmSetFlipMode(u32 mode);
extern void cellGcmSetFlipHandler(vm::ptr<void(u32)> handler);
extern void cellGcmSetVBlankHandler(vm::ptr<void(u32)> handler);
extern s32 cellGcmAddressToOffset(u32 address, vm::ptr<u32> offset);
extern s32 cellGcmSetDisplayBuffer(u32 id, u32 offset, u32 pitch, u32 width, u32 height);
extern s32 cellGcmSetPrepareFlip(vm::ptr<CellGcmContextData> ctx, u32 id);
extern s32 cellGcmSetSecondVFrequency(u32 freq);
extern u32 cellGcmGetLabelAddress(u8 index);
extern u32 cellGcmGetTiledPitchSize(u32 size);

CCellRescInternal* s_rescInternalInstance = nullptr;

// Local Functions
int cellRescGetNumColorBuffers(u32 dstMode, u32 palTemporalMode, u32 reserved);

// Help Functions
inline bool IsPal()            { return s_rescInternalInstance->m_dstMode == CELL_RESC_720x576; }
inline bool IsPal60Hsync()     { return (IsPal() && s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_FOR_HSYNC); }
inline bool IsPalDrop()        { return (IsPal() && s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_DROP); }
inline bool IsPalInterpolate() {
	return (IsPal() && ((s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_INTERPOLATE)
		|| (s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_INTERPOLATE_30_DROP)
		|| (s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE)));
}
inline bool IsNotPalInterpolate() { return !IsPalInterpolate(); }
inline bool IsPalTemporal() { return (IsPal() && s_rescInternalInstance->m_initConfig.palTemporalMode != CELL_RESC_PAL_50); }
inline bool IsNotPalTemporal() { return !IsPalTemporal(); }
inline bool IsNotPal() { return !IsPal(); }
inline bool IsGcmFlip()  {
	return (IsNotPal() || (IsPal() && (s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_50
		|| s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_FOR_HSYNC)));
}
inline int GetNumColorBuffers(){ return IsPalInterpolate() ? 6 : (IsPalDrop() ? 3 : 2); }
inline bool IsInterlace()      { return s_rescInternalInstance->m_initConfig.interlaceMode == CELL_RESC_INTERLACE_FILTER; }
inline bool IsTextureNR()      { return !IsInterlace(); }

static const float
PICTURE_SIZE = (1.0f),
UV_DELTA_PS = (1.f / 8.f),
UV_DELTA_LB = (1.f / 6.f),
XY_DELTA_LB = (1.f / 8.f),
PI = 3.141592741f;

void BuildupVertexBufferNR()
{
	const float PX_FS = PICTURE_SIZE;
	const float PY_FS = PICTURE_SIZE;

	const float UV_HALF   = 0.5f;
	const float UV_CENTER = 0.5f;
	float U_FS  = UV_HALF / s_rescInternalInstance->m_ratioAdjX;
	float V_FS  = UV_HALF / s_rescInternalInstance->m_ratioAdjY;
	float U_FS0 = UV_CENTER - U_FS;
	float V_FS0 = UV_CENTER - V_FS;
	float U_FS1 = UV_CENTER + U_FS;
	float V_FS1 = UV_CENTER + V_FS;
	float V_LB  = (UV_HALF + UV_DELTA_LB) / s_rescInternalInstance->m_ratioAdjY;
	float V_LB0 = UV_CENTER - V_LB;
	float V_LB1 = UV_CENTER + V_LB;
	float U_PS  = (UV_HALF - UV_DELTA_PS) / s_rescInternalInstance->m_ratioAdjX;
	float U_PS0 = UV_CENTER - U_PS;
	float U_PS1 = UV_CENTER + U_PS;

	auto vv = vm::ptr<RescVertex_t>::make(s_rescInternalInstance->m_vertexArrayEA);

	if (s_rescInternalInstance->m_dstMode == CELL_RESC_720x480 || s_rescInternalInstance->m_dstMode == CELL_RESC_720x576)
	{
		switch((u32)s_rescInternalInstance->m_initConfig.ratioMode)
		{
		case CELL_RESC_LETTERBOX:  
			goto NR_LETTERBOX;
		case CELL_RESC_PANSCAN:    
			goto NR_PANSCAN;
		default:                   
			goto NR_FULLSCREEN;
		}
	} 
	else 
	{
		goto NR_FULLSCREEN;
	}

NR_FULLSCREEN:
	vv->Px = -PX_FS; vv->Py =  PY_FS; vv->u = U_FS0; vv->v = V_FS0; vv->u2 = 0.0f; vv->v2 = 0.0f; ++vv;
	vv->Px =  PX_FS; vv->Py =  PY_FS; vv->u = U_FS1; vv->v = V_FS0; vv->u2 = 1.0f; vv->v2 = 0.0f; ++vv;
	vv->Px =  PX_FS; vv->Py = -PY_FS; vv->u = U_FS1; vv->v = V_FS1; vv->u2 = 1.0f; vv->v2 = 1.0f; ++vv;
	vv->Px = -PX_FS; vv->Py = -PY_FS; vv->u = U_FS0; vv->v = V_FS1; vv->u2 = 0.0f; vv->v2 = 1.0f; ++vv;
	s_rescInternalInstance->m_nVertex = VERTEX_NUMBER_NORMAL;
	return;

NR_LETTERBOX:
	vv->Px = -PX_FS; vv->Py =  PY_FS; vv->u = U_FS0; vv->v = V_LB0; vv->u2 = 0.0f; vv->v2 = 0.0f; ++vv;
	vv->Px =  PX_FS; vv->Py =  PY_FS; vv->u = U_FS1; vv->v = V_LB0; vv->u2 = 1.0f; vv->v2 = 0.0f; ++vv;
	vv->Px =  PX_FS; vv->Py = -PY_FS; vv->u = U_FS1; vv->v = V_LB1; vv->u2 = 1.0f; vv->v2 = 1.0f; ++vv;
	vv->Px = -PX_FS; vv->Py = -PY_FS; vv->u = U_FS0; vv->v = V_LB1; vv->u2 = 0.0f; vv->v2 = 1.0f; ++vv;
	s_rescInternalInstance->m_nVertex = VERTEX_NUMBER_NORMAL;
	return;

NR_PANSCAN:
	vv->Px = -PX_FS; vv->Py =  PY_FS; vv->u = U_PS0; vv->v = V_FS0; vv->u2 = 0.0f; vv->v2 = 0.0f; ++vv;
	vv->Px =  PX_FS; vv->Py =  PY_FS; vv->u = U_PS1; vv->v = V_FS0; vv->u2 = 1.0f; vv->v2 = 0.0f; ++vv;
	vv->Px =  PX_FS; vv->Py = -PY_FS; vv->u = U_PS1; vv->v = V_FS1; vv->u2 = 1.0f; vv->v2 = 1.0f; ++vv;
	vv->Px = -PX_FS; vv->Py = -PY_FS; vv->u = U_PS0; vv->v = V_FS1; vv->u2 = 0.0f; vv->v2 = 1.0f; ++vv;
	s_rescInternalInstance->m_nVertex = VERTEX_NUMBER_NORMAL;
	return;
}

void BuildupVertexBufferUN(s32 srcIdx)
{
	if (s_rescInternalInstance->m_bNewlyAdjustRatio)
	{
		s_rescInternalInstance->m_srcWidthInterlace  = s_rescInternalInstance->m_rescSrc[srcIdx].width;
		s_rescInternalInstance->m_srcHeightInterlace = s_rescInternalInstance->m_rescSrc[srcIdx].height;
		s_rescInternalInstance->m_bNewlyAdjustRatio = false;
	} 
	else 
	{
		if (s_rescInternalInstance->m_srcWidthInterlace == s_rescInternalInstance->m_rescSrc[srcIdx].width 
		   && s_rescInternalInstance->m_srcHeightInterlace == s_rescInternalInstance->m_rescSrc[srcIdx].height)
		{
			return;
		} 
		else 
		{
			s_rescInternalInstance->m_srcWidthInterlace  = s_rescInternalInstance->m_rescSrc[srcIdx].width;
			s_rescInternalInstance->m_srcHeightInterlace = s_rescInternalInstance->m_rescSrc[srcIdx].height;
		}
	}

	const float PX_FS = PICTURE_SIZE;
	const float PY_FS = PICTURE_SIZE;
	
	float U_HALF = s_rescInternalInstance->m_rescSrc[srcIdx].width  * 0.5f;
	float V_HALF = s_rescInternalInstance->m_rescSrc[srcIdx].height * 0.5f;
	float U_CENTER = U_HALF;
	float V_CENTER = V_HALF;
	float U_FS  = U_HALF / s_rescInternalInstance->m_ratioAdjX;
	float V_FS  = V_HALF / s_rescInternalInstance->m_ratioAdjY;
	float U_FS0 = U_CENTER - U_FS;
	float V_FS0 = V_CENTER - V_FS;
	float U_FS1 = U_CENTER + U_FS;
	float V_FS1 = V_CENTER + V_FS;
	float V_LB  = V_HALF * (1.f + 2.f*UV_DELTA_LB) / s_rescInternalInstance->m_ratioAdjY;
	float V_LB0 = V_CENTER - V_LB;
	float V_LB1 = V_CENTER + V_LB;
	float U_PS  = U_HALF * (1.f - 2.f*UV_DELTA_PS) / s_rescInternalInstance->m_ratioAdjX;
	float U_PS0 = U_CENTER - U_PS;
	float U_PS1 = U_CENTER + U_PS;
	float U2_FS0 = 0.0f;
	float V2_FS0 = 0.0f;
	float U2_FS1 = (float)s_rescInternalInstance->m_dstWidth;
	float V2_FS1 = (float)s_rescInternalInstance->m_dstHeight;

	auto vv = vm::ptr<RescVertex_t>::make(s_rescInternalInstance->m_vertexArrayEA);

	if (s_rescInternalInstance->m_dstMode == CELL_RESC_720x480 || s_rescInternalInstance->m_dstMode == CELL_RESC_720x576)
	{
		switch((u32)s_rescInternalInstance->m_initConfig.ratioMode)
		{
		case CELL_RESC_LETTERBOX:  
			goto UN_LETTERBOX;
		case CELL_RESC_PANSCAN:   
			goto UN_PANSCAN;
		default:                   
			goto UN_FULLSCREEN;
		}
	} 
	else 
	{
		goto UN_FULLSCREEN;
	}

UN_FULLSCREEN:
	vv->Px = -PX_FS; vv->Py =  PY_FS; vv->u = U_FS0; vv->v = V_FS0; vv->u2 = U2_FS0; vv->v2 = V2_FS0; ++vv;
	vv->Px =  PX_FS; vv->Py =  PY_FS; vv->u = U_FS1; vv->v = V_FS0; vv->u2 = U2_FS1; vv->v2 = V2_FS0; ++vv;
	vv->Px =  PX_FS; vv->Py = -PY_FS; vv->u = U_FS1; vv->v = V_FS1; vv->u2 = U2_FS1; vv->v2 = V2_FS1; ++vv;
	vv->Px = -PX_FS; vv->Py = -PY_FS; vv->u = U_FS0; vv->v = V_FS1; vv->u2 = U2_FS0; vv->v2 = V2_FS1; ++vv;
	s_rescInternalInstance->m_nVertex = VERTEX_NUMBER_NORMAL;
	return;

UN_LETTERBOX:
	vv->Px = -PX_FS; vv->Py =  PY_FS; vv->u = U_FS0; vv->v = V_LB0; vv->u2 = U2_FS0; vv->v2 = V2_FS0; ++vv;
	vv->Px =  PX_FS; vv->Py =  PY_FS; vv->u = U_FS1; vv->v = V_LB0; vv->u2 = U2_FS1; vv->v2 = V2_FS0; ++vv;
	vv->Px =  PX_FS; vv->Py = -PY_FS; vv->u = U_FS1; vv->v = V_LB1; vv->u2 = U2_FS1; vv->v2 = V2_FS1; ++vv;
	vv->Px = -PX_FS; vv->Py = -PY_FS; vv->u = U_FS0; vv->v = V_LB1; vv->u2 = U2_FS0; vv->v2 = V2_FS1; ++vv;
	s_rescInternalInstance->m_nVertex = VERTEX_NUMBER_NORMAL;
	return;

UN_PANSCAN:
	vv->Px = -PX_FS; vv->Py =  PY_FS; vv->u = U_PS0; vv->v = V_FS0; vv->u2 = U2_FS0; vv->v2 = V2_FS0; ++vv;
	vv->Px =  PX_FS; vv->Py =  PY_FS; vv->u = U_PS1; vv->v = V_FS0; vv->u2 = U2_FS1; vv->v2 = V2_FS0; ++vv;
	vv->Px =  PX_FS; vv->Py = -PY_FS; vv->u = U_PS1; vv->v = V_FS1; vv->u2 = U2_FS1; vv->v2 = V2_FS1; ++vv;
	vv->Px = -PX_FS; vv->Py = -PY_FS; vv->u = U_PS0; vv->v = V_FS1; vv->u2 = U2_FS0; vv->v2 = V2_FS1; ++vv;
	s_rescInternalInstance->m_nVertex = VERTEX_NUMBER_NORMAL;
	return;
}

inline int InternalVersion(vm::ptr<CellRescInitConfig> conf)
{
	switch ((u32)conf->size)
	{
		case 20: 
			return 1;
		case 24: 
			return 2;
		case 28: 
			return 3;
		default: return -1;
	}
}

inline int InternalVersion() {
	switch ((u32)s_rescInternalInstance->m_initConfig.size)
	{
		case 20: 
			return 1;
		case 24: 
			return 2;
		case 28: 
			return 3;
		default: 
			return -1;
	}
}

u8 RescBufferMode2SysutilResolutionId(u32 bufferMode)
{
	switch (bufferMode)
	{
	case CELL_RESC_720x576:	  
		return CELL_VIDEO_OUT_RESOLUTION_576;
	case CELL_RESC_1280x720:  
		return CELL_VIDEO_OUT_RESOLUTION_720;
	case CELL_RESC_1920x1080: 
		return CELL_VIDEO_OUT_RESOLUTION_1080;
	default:                  
		return CELL_VIDEO_OUT_RESOLUTION_480;
	}
}

u8 RescDstFormat2SysutilFormat(u32 dstFormat)
{
	switch (dstFormat) 
	{
	case CELL_RESC_SURFACE_F_W16Z16Y16X16: 
		return CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT;
	default:                               
		return CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	}
}

u8 GcmSurfaceFormat2GcmTextureFormat(u8 surfaceFormat, u8 surfaceType)
{
	u8 result = 0;

	switch (surfaceFormat)
	{
	case CELL_GCM_SURFACE_A8R8G8B8:		
		result = CELL_GCM_TEXTURE_A8R8G8B8;			
		break;
	case CELL_GCM_SURFACE_F_W16Z16Y16X16:	
		result = CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;	
		break;
	default: 
		return 0xFF; //Error
	}

	switch (surfaceType)
	{
	case CELL_GCM_SURFACE_PITCH:   
		result |= CELL_GCM_TEXTURE_LN; 
		break;
	case CELL_GCM_SURFACE_SWIZZLE: 
		result |= CELL_GCM_TEXTURE_SZ; 
		break;
	default: 
		return 0xFF; //Error
	}

	result |= CELL_GCM_TEXTURE_NR;

	return result;
}

int GetRescDestsIndex(u32 dstMode)
{
	switch(dstMode)
	{
	case CELL_RESC_720x480:   
		return 0;
	case CELL_RESC_720x576:   
		return 1;
	case CELL_RESC_1280x720:  
		return 2;
	case CELL_RESC_1920x1080: 
		return 3;
	default: 
		return -1;
	}
}

void GetScreenSize(u32 mode, s32 *width, s32 *height)
{	
	switch (mode)
	{
	case CELL_RESC_720x480:   
		*width = 720;  *height = 480;  
		break;
	case CELL_RESC_720x576:   
		*width = 720;  *height = 576; 
		break;
	case CELL_RESC_1280x720:  
		*width = 1280; *height = 720;	 
		break;
	case CELL_RESC_1920x1080: 
		*width = 1920; *height = 1080; 
		break;
	default: 
		*width = *height = 0; 
		break;
	}
}

int CalculateSurfaceByteSize(u32 mode, CellRescDsts *dsts)
{
	s32 width, height;
	GetScreenSize(mode, &width, &height);
	return dsts->pitch * roundup(height, dsts->heightAlign);
}

int CalculateMaxColorBuffersSize()
{
	s32 oneBufSize, bufNum, totalBufSize, maxBufSize;
	maxBufSize = 0;

	for (u32 bufMode = CELL_RESC_720x480; bufMode <= CELL_RESC_1920x1080; bufMode <<= 1)
	{
		if (s_rescInternalInstance->m_initConfig.supportModes & bufMode) 
		{
			oneBufSize   = CalculateSurfaceByteSize(bufMode, &(s_rescInternalInstance->m_rescDsts[GetRescDestsIndex(bufMode)]));
			bufNum       = cellRescGetNumColorBuffers(bufMode, s_rescInternalInstance->m_initConfig.palTemporalMode, 0);
			totalBufSize = oneBufSize * bufNum;
			maxBufSize   = (maxBufSize > totalBufSize) ? maxBufSize : totalBufSize;
		}
	}

	return maxBufSize;
}

bool CheckInitConfig(vm::ptr<CellRescInitConfig> initConfig)
{
	if ((initConfig->resourcePolicy & ~((u32)0x3)) || (initConfig->supportModes & 0xF) == 0 || (initConfig->ratioMode > 2) || (initConfig->palTemporalMode > 5))
	{
		return false;
	}

	if ( InternalVersion() >= 2 )
	{
		if (InternalVersion() == 2 && initConfig->interlaceMode > 1)
		{
			return false;
		}
	}

	if ( InternalVersion() >= 3 )
	{
		if ((initConfig->interlaceMode > 4) || (initConfig->flipMode > 1))
		{
			return false;
		}
	}

	return true;
}

void InitMembers()
{
	s_rescInternalInstance->m_dstMode = (CellRescBufferMode)0;
	s_rescInternalInstance->m_interlaceElement = CELL_RESC_ELEMENT_FLOAT;
	s_rescInternalInstance->m_colorBuffersEA = 0;
	s_rescInternalInstance->m_vertexArrayEA = 0;
	s_rescInternalInstance->m_fragmentUcodeEA = 0;
	s_rescInternalInstance->m_interlaceTableEA = 0;
	s_rescInternalInstance->m_bufIdFront = 0;
	s_rescInternalInstance->m_dstWidth = 0;
	s_rescInternalInstance->m_dstHeight = 0;
	s_rescInternalInstance->m_dstPitch = 0;
	s_rescInternalInstance->m_srcWidthInterlace = 0;
	s_rescInternalInstance->m_srcHeightInterlace = 0;
	s_rescInternalInstance->m_dstBufInterval = 0;
	s_rescInternalInstance->m_nVertex = 0;
	s_rescInternalInstance->m_ratioAdjX = 1.f;
	s_rescInternalInstance->m_ratioAdjY = 1.f;
	s_rescInternalInstance->m_interlaceTableLength = 32;
	s_rescInternalInstance->m_bInitialized = false;
	s_rescInternalInstance->m_bNewlyAdjustRatio = false;

	s_rescInternalInstance->s_applicationVBlankHandler.set(0);
	s_rescInternalInstance->s_applicationFlipHandler.set(0);

	//E PAL related variables
	//s_rescInternalInstance->m_intrThread50 = 0;
	//s_rescInternalInstance->m_lastDummyFlip = 0;
	//s_rescInternalInstance->m_lastVsync60 = 0;
	//s_rescInternalInstance->m_lastVsync50 = 0;
	s_rescInternalInstance->m_bufIdFrontPrevDrop = 2;
	s_rescInternalInstance->m_bufIdPalMidPrev = 4;
	s_rescInternalInstance->m_bufIdPalMidNow = 5;
	//s_rescInternalInstance->m_cgpTvalue = 0;
	s_rescInternalInstance->m_isDummyFlipped = true;
	s_rescInternalInstance->m_flexRatio = 0.f;   // interpolate
	s_rescInternalInstance->m_commandIdxCaF = 1;
	s_rescInternalInstance->m_rcvdCmdIdx = 0;

	//s_rescInternalInstance->m_lastV60.idx = 0;
	//s_rescInternalInstance->m_lastV60.time = Util::GetSystemTime();
	//s_rescInternalInstance->m_lastV50.idx = 0;
	//s_rescInternalInstance->m_lastV50.time = Util::GetSystemTime();

	//s_rescInternalInstance->m_feedback.interval60 = 1;

	for (int i = 0; i<SRC_BUFFER_NUM; i++) {
		s_rescInternalInstance->m_rescSrc[i].format = 0;
		s_rescInternalInstance->m_rescSrc[i].pitch = 0;
		s_rescInternalInstance->m_rescSrc[i].width = 0;
		s_rescInternalInstance->m_rescSrc[i].height = 0;
		s_rescInternalInstance->m_rescSrc[i].offset = 0;
	}

	for (int i = 0; i<MAX_DST_BUFFER_NUM; i++) {
		s_rescInternalInstance->m_dstOffsets[i] = 0;
	}

	for (int i = 0; i<RESC_PARAM_NUM; i++) {
		s_rescInternalInstance->m_cgParamIndex[i] = 0xFF;
	}
	{
		s_rescInternalInstance->m_rescDsts[0].format = CELL_RESC_SURFACE_A8R8G8B8;
		s_rescInternalInstance->m_rescDsts[0].pitch = cellGcmGetTiledPitchSize(720 * 4);
		s_rescInternalInstance->m_rescDsts[0].heightAlign = 8;
		s_rescInternalInstance->m_rescDsts[1].format = CELL_RESC_SURFACE_A8R8G8B8;
		s_rescInternalInstance->m_rescDsts[1].pitch = cellGcmGetTiledPitchSize(720 * 4);
		s_rescInternalInstance->m_rescDsts[1].heightAlign = 8;
		s_rescInternalInstance->m_rescDsts[2].format = CELL_RESC_SURFACE_A8R8G8B8;
		s_rescInternalInstance->m_rescDsts[2].pitch = cellGcmGetTiledPitchSize(1280 * 4);
		s_rescInternalInstance->m_rescDsts[2].heightAlign = 8;
		s_rescInternalInstance->m_rescDsts[3].format = CELL_RESC_SURFACE_A8R8G8B8;
		s_rescInternalInstance->m_rescDsts[3].pitch = cellGcmGetTiledPitchSize(1920 * 4);
		s_rescInternalInstance->m_rescDsts[3].heightAlign = 8;
	}
}

void SetupRsxRenderingStates(vm::ptr<CellGcmContextData>& cntxt)
{
	//TODO: use cntxt
	GSLockCurrent lock(GS_LOCK_WAIT_FLUSH);
	GSRender& r = Emu.GetGSManager().GetRender();
	r.m_set_color_mask = true; r.m_color_mask_a = r.m_color_mask_r = r.m_color_mask_g = r.m_color_mask_b = true;
	r.m_set_depth_mask = true; r.m_depth_mask = 0;
	r.m_set_alpha_test = false;
	r.m_set_blend = false;
	r.m_set_blend_mrt1 = r.m_set_blend_mrt2 = r.m_set_blend_mrt3 = false;
	r.m_set_logic_op = false;
	r.m_set_cull_face = false;
	r.m_set_depth_bounds_test = false;
	r.m_set_depth_test = false;
	r.m_set_poly_offset_fill = false;
	r.m_set_stencil_test = false;
	r.m_set_two_sided_stencil_test_enable = false;
	r.m_set_two_side_light_enable = false;
	r.m_set_point_sprite_control = false;
	r.m_set_dither = true;
	r.m_set_shade_mode = true; r.m_shade_mode = CELL_GCM_SMOOTH;
	r.m_set_frequency_divider_operation = CELL_GCM_FREQUENCY_DIVIDE;

	r.m_set_viewport_horizontal = r.m_set_viewport_vertical = true;
	r.m_viewport_x = 0;
	r.m_viewport_y = 0;
	r.m_viewport_w = s_rescInternalInstance->m_dstWidth;
	r.m_viewport_h = s_rescInternalInstance->m_dstHeight;

	r.m_set_scissor_horizontal = r.m_set_scissor_vertical = true;
	r.m_scissor_x = 0;
	r.m_scissor_y = 0;
	r.m_scissor_w = s_rescInternalInstance->m_dstWidth;
	r.m_scissor_h = s_rescInternalInstance->m_dstHeight;

	r.m_width = s_rescInternalInstance->m_dstWidth;
	r.m_height = s_rescInternalInstance->m_dstHeight;

	r.m_surface_depth_format = 2;
	r.m_surface_color_target = 1;

	if (IsPalInterpolate()) 
	{
		//MRT
		//GcmCmdTypePrefix::cellGcmSetColorMaskMrt(con, CELL_GCM_COLOR_MASK_MRT1_A | CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B);
	}
}

void SetupVertexArrays(vm::ptr<CellGcmContextData>& cntxt)
{
	GSLockCurrent lock(GS_LOCK_WAIT_FLUSH);
	GSRender& r = Emu.GetGSManager().GetRender();
	
	//TODO
}

void SetupSurfaces(vm::ptr<CellGcmContextData>& cntxt)
{
	bool isMrt;
	u32 dstOffset0, dstOffset1;

	if (IsNotPalInterpolate()) 
	{
		isMrt = false;
		dstOffset0 = s_rescInternalInstance->m_dstOffsets[s_rescInternalInstance->m_bufIdFront];
		dstOffset1 = 0;
	}
	else 
	{
		isMrt = true;
		dstOffset0 = s_rescInternalInstance->m_dstOffsets[s_rescInternalInstance->m_bufIdFront];
		dstOffset1 = s_rescInternalInstance->m_dstOffsets[s_rescInternalInstance->m_bufIdPalMidNow];
	}

	GSLockCurrent lock(GS_LOCK_WAIT_FLUSH);
	GSRender& r = Emu.GetGSManager().GetRender();

	r.m_surface_type = CELL_GCM_SURFACE_PITCH;
	r.m_surface_antialias = CELL_GCM_SURFACE_CENTER_1;
	r.m_surface_color_format = (u8)s_rescInternalInstance->m_pRescDsts->format;
	r.m_surface_color_target = (!isMrt) ? CELL_GCM_SURFACE_TARGET_0 : CELL_GCM_SURFACE_TARGET_MRT1;
	//surface.colorLocation[0] = CELL_GCM_LOCATION_LOCAL;
	r.m_surface_offset_a = dstOffset0;
	r.m_surface_pitch_a = s_rescInternalInstance->m_dstPitch;
	//surface.colorLocation[1] = CELL_GCM_LOCATION_LOCAL;
	r.m_surface_offset_b = (!isMrt) ? 0 : dstOffset1;
	r.m_surface_pitch_b = (!isMrt) ? 64 : s_rescInternalInstance->m_dstPitch;
	//surface.colorLocation[2] = CELL_GCM_LOCATION_LOCAL;
	r.m_surface_offset_c = 0;
	r.m_surface_pitch_c = 64;
	//surface.colorLocation[3] = CELL_GCM_LOCATION_LOCAL;
	r.m_surface_offset_d = 0;
	r.m_surface_pitch_d = 64;
	r.m_surface_depth_format = CELL_GCM_SURFACE_Z24S8;
	//surface.depthLocation = CELL_GCM_LOCATION_LOCAL;
	r.m_surface_offset_z = 0;
	r.m_surface_pitch_z = 64;
	r.m_surface_width = s_rescInternalInstance->m_dstWidth;
	r.m_surface_height = s_rescInternalInstance->m_dstHeight;
	r.m_surface_clip_x = 0;
	r.m_surface_clip_y = 0;
}

// Module Functions
int cellRescInit(vm::ptr<CellRescInitConfig> initConfig)
{
	cellResc.Warning("cellRescInit(initConfig_addr=0x%x)", initConfig.addr());

	if (s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescInit : CELL_RESC_ERROR_REINITIALIZED");
		return CELL_RESC_ERROR_REINITIALIZED;
	}

	if (InternalVersion(initConfig) == -1 || !CheckInitConfig(initConfig))
	{
		cellResc.Error("cellRescInit : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	InitMembers();
	s_rescInternalInstance->m_initConfig = *initConfig; // TODO: This may be incompatible with older binaries
	s_rescInternalInstance->m_bInitialized = true;

	return CELL_OK;
}

void cellRescExit()
{
	cellResc.Warning("cellRescExit()");

	if (!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescExit(): not initialized");
		return;
	}

	if (IsPalTemporal())
	{
		cellGcmSetSecondVFrequency(CELL_GCM_DISPLAY_FREQUENCY_DISABLE);
		cellGcmSetVBlankHandler(vm::null);
		//GcmSysTypePrefix::cellGcmSetSecondVHandler(NULL);

		if (IsPalInterpolate())
		{
			// TODO: ExitSystemResource()
			//int ret = ExitSystemResource();
			//if (ret != CELL_OK)
			//{
			//	cellResc.Error("failed to clean up system resources.. continue. 0x%x\n", ret);
			//}
		}
	}

	s_rescInternalInstance->m_bInitialized = false;
}

int cellRescVideoOutResolutionId2RescBufferMode(u32 resolutionId, vm::ptr<u32> bufferMode)
{
	cellResc.Log("cellRescVideoOutResolutionId2RescBufferMode(resolutionId=%d, bufferMode_addr=0x%x)", resolutionId, bufferMode.addr());

	switch (resolutionId)
	{
	case CELL_VIDEO_OUT_RESOLUTION_1080: 
		*bufferMode = CELL_RESC_1920x1080; 
		break;
	case CELL_VIDEO_OUT_RESOLUTION_720:  
		*bufferMode = CELL_RESC_1280x720;  
		break;
	case CELL_VIDEO_OUT_RESOLUTION_480:  
		*bufferMode = CELL_RESC_720x480;   
		break;
	case CELL_VIDEO_OUT_RESOLUTION_576:  
		*bufferMode = CELL_RESC_720x576;   
		break;
	default: 
		cellResc.Error("cellRescVideoOutResolutionId2RescBufferMod : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

int cellRescSetDsts(u32 dstsMode, vm::ptr<CellRescDsts> dsts)
{
	cellResc.Log("cellRescSetDsts(dstsMode=%d, CellRescDsts_addr=0x%x)", dstsMode, dsts.addr());

	if (!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescSetDst : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if ((dstsMode != CELL_RESC_720x480) && (dstsMode != CELL_RESC_720x576) && (dstsMode != CELL_RESC_1280x720) && (dstsMode != CELL_RESC_1920x1080))
	{
		cellResc.Error("cellRescSetDsts : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	s_rescInternalInstance->m_rescDsts[GetRescDestsIndex(dstsMode)] = *dsts;

	return CELL_OK;
}

void SetVBlankHandler(vm::ptr<void(u32)> handler)
{
	if (!s_rescInternalInstance->m_bInitialized || s_rescInternalInstance->m_dstMode == 0) 
	{
		// If this function is called before SetDisplayMode, handler should be stored and set it properly later in SetDisplayMode.
		s_rescInternalInstance->s_applicationVBlankHandler = handler;
		return;
	}

	if (IsNotPalTemporal()) 
	{
		cellGcmSetVBlankHandler(handler);
		s_rescInternalInstance->s_applicationVBlankHandler.set(0);
	}
	else if (IsPal60Hsync()) 
	{
		//cellGcmSetSecondVHandler(handler);
		s_rescInternalInstance->s_applicationVBlankHandler.set(0);
	}
	else 
	{
		s_rescInternalInstance->s_applicationVBlankHandler = handler;
	}
}


void SetFlipHandler(vm::ptr<void(u32)> handler)
{
	if (!s_rescInternalInstance->m_bInitialized || s_rescInternalInstance->m_dstMode == 0) 
	{
		// If this function is called before SetDisplayMode, handler should be stored and set it properly later in SetDisplayMode.
		s_rescInternalInstance->s_applicationFlipHandler = handler;
		return;
	}

	if (IsGcmFlip()) 
	{
		cellGcmSetFlipHandler(handler);
		s_rescInternalInstance->s_applicationFlipHandler.set(0);
	}
	else 
	{
		s_rescInternalInstance->s_applicationFlipHandler = handler;
	}
}

int cellRescSetDisplayMode(u32 displayMode)
{
	cellResc.Warning("cellRescSetDisplayMode(displayMode=%d)", displayMode);

	if (!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescSetDisplayMode : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (!(s_rescInternalInstance->m_initConfig.supportModes & displayMode))
	{
		cellResc.Error("cellRescSetDisplayMode : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	if ((displayMode != CELL_RESC_720x480) && (displayMode != CELL_RESC_720x576) &&
		(displayMode != CELL_RESC_1280x720) && (displayMode != CELL_RESC_1920x1080))
	{
		cellResc.Error("cellRescSetDisplayMode : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	s_rescInternalInstance->m_dstMode = displayMode;

	if ((IsPalInterpolate() || IsPalDrop()) && s_rescInternalInstance->m_initConfig.flipMode == CELL_RESC_DISPLAY_HSYNC)
	{
		cellResc.Error("cellRescSetDisplayMode : CELL_RESC_ERROR_BAD_COMBINATIONT");
		return CELL_RESC_ERROR_BAD_COMBINATION;
	}

	if (IsPal60Hsync() && s_rescInternalInstance->m_initConfig.flipMode==CELL_RESC_DISPLAY_VSYNC)
	{
		cellResc.Error("cellRescSetDisplayMode : CELL_RESC_ERROR_BAD_COMBINATIONT");
		return CELL_RESC_ERROR_BAD_COMBINATION;
	}

	s_rescInternalInstance->m_pRescDsts = &s_rescInternalInstance->m_rescDsts[GetRescDestsIndex(displayMode)];
	GetScreenSize(s_rescInternalInstance->m_dstMode, &(s_rescInternalInstance->m_dstWidth),	&(s_rescInternalInstance->m_dstHeight));
	
	s_rescInternalInstance->m_dstPitch = s_rescInternalInstance->m_pRescDsts->pitch;
	s_rescInternalInstance->m_dstBufInterval = s_rescInternalInstance->m_dstPitch * roundup(s_rescInternalInstance->m_dstHeight, s_rescInternalInstance->m_pRescDsts->heightAlign);

	/*if (IsPalInterpolate()) {
		if(IsInterlace()) m_pCFragmentShader = m_pCFragmentShaderArray[RESC_SHADER_DEFAULT_INTERLACE_PAL];
		else			  m_pCFragmentShader = m_pCFragmentShaderArray[RESC_SHADER_DEFAULT_BILINEAR_PAL];
	} else {
		if(IsInterlace()) m_pCFragmentShader = m_pCFragmentShaderArray[RESC_SHADER_DEFAULT_INTERLACE];
		else			  m_pCFragmentShader = m_pCFragmentShaderArray[RESC_SHADER_DEFAULT_BILINEAR];
	}*/

	vm::var<CellVideoOutConfiguration> videocfg;
	videocfg->resolutionId = RescBufferMode2SysutilResolutionId(s_rescInternalInstance->m_dstMode);
	videocfg->format       = RescDstFormat2SysutilFormat(s_rescInternalInstance->m_pRescDsts->format );
	videocfg->aspect       = CELL_VIDEO_OUT_ASPECT_AUTO;
	videocfg->pitch        = s_rescInternalInstance->m_dstPitch;

	cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, videocfg, vm::null, 0);

	if (IsPalInterpolate())
	{
		//int ret = InitSystemResource();
		//if (ret) return ret;
		//InitLabels();
		cellGcmSetSecondVFrequency(CELL_GCM_DISPLAY_FREQUENCY_59_94HZ);
		//cellGcmSetVBlankHandler(IntrHandler50);
		//cellGcmSetSecondVHandler(IntrHandler60);
		cellGcmSetFlipHandler(vm::null);
	}
	else if (IsPalDrop())
	{
		//InitLabels();
		cellGcmSetSecondVFrequency(CELL_GCM_DISPLAY_FREQUENCY_59_94HZ);
		cellGcmSetVBlankHandler(vm::null);
		//cellGcmSetSecondVHandler(IntrHandler60Drop);
		cellGcmSetFlipHandler(vm::null);
	} 
	else if (IsPal60Hsync())
	{
		cellGcmSetSecondVFrequency(CELL_GCM_DISPLAY_FREQUENCY_59_94HZ);
		cellGcmSetVBlankHandler(vm::null);
	}

	if (s_rescInternalInstance->s_applicationVBlankHandler) SetVBlankHandler(s_rescInternalInstance->s_applicationVBlankHandler);
	if (s_rescInternalInstance->s_applicationFlipHandler)   SetFlipHandler(s_rescInternalInstance->s_applicationFlipHandler);
	cellGcmSetFlipMode((s_rescInternalInstance->m_initConfig.flipMode == CELL_RESC_DISPLAY_VSYNC) ? CELL_GCM_DISPLAY_VSYNC : CELL_GCM_DISPLAY_HSYNC);

	return CELL_OK;
}

int cellRescAdjustAspectRatio(float horizontal, float vertical)
{
	cellResc.Warning("cellRescAdjustAspectRatio(horizontal=%f, vertical=%f)", horizontal, vertical);

	if (!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescAdjustAspectRatio : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if ((horizontal < 0.5f || 2.f < horizontal) || (vertical < 0.5f || 2.f < vertical))
	{
		cellResc.Error("cellRescAdjustAspectRatio : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	s_rescInternalInstance->m_ratioAdjX = horizontal;
	s_rescInternalInstance->m_ratioAdjY = vertical;

	if (s_rescInternalInstance->m_vertexArrayEA)
	{
		if (IsTextureNR())
		{
			BuildupVertexBufferNR();
		}
		else
		{
			s_rescInternalInstance->m_bNewlyAdjustRatio = true;
		}
	}

	return CELL_OK;
}

int cellRescSetPalInterpolateDropFlexRatio(float ratio)
{
	cellResc.Warning("cellRescSetPalInterpolateDropFlexRatio(ratio=%f)", ratio);

	if (!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescSetPalInterpolateDropFlexRatio : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (ratio < 0.f || 1.f < ratio)
	{
		cellResc.Error("cellRescSetPalInterpolateDropFlexRatio : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	s_rescInternalInstance->m_flexRatio = ratio;

	return CELL_OK;
}

int cellRescGetBufferSize(vm::ptr<u32> colorBuffers, vm::ptr<u32> vertexArray, vm::ptr<u32> fragmentShader)
{
	cellResc.Warning("cellRescGetBufferSize(colorBuffers_addr=0x%x, vertexArray_addr=0x%x, fragmentShader_addr=0x%x)",
		colorBuffers.addr(), vertexArray.addr(), fragmentShader.addr());

	if (!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescGetBufferSize : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	s32 colorBuffersSize, vertexArraySize, fragmentUcodeSize;
	if (s_rescInternalInstance->m_initConfig.resourcePolicy & CELL_RESC_MINIMUM_VRAM)
	{
		colorBuffersSize  = s_rescInternalInstance->m_dstBufInterval * GetNumColorBuffers();
		vertexArraySize   = 0x180; //sizeof(RescVertex_t) * VERTEX_NUMBER_RESERVED;
		//fragmentUcodeSize = m_pCFragmentShader->GetUcodeSize();
		fragmentUcodeSize = 0x300;
	}
	else //CELL_RESC_CONSTANT_VRAM
	{ 
		colorBuffersSize  = CalculateMaxColorBuffersSize();
		vertexArraySize   = 0x180; //sizeof(RescVertex_t) * VERTEX_NUMBER_RESERVED;
		fragmentUcodeSize = 0x300;
	}

	if (colorBuffers)
	{
		*colorBuffers = colorBuffersSize;
	}

	if (vertexArray)
	{
		*vertexArray = vertexArraySize;
	}

	if (fragmentShader)
	{
		*fragmentShader = fragmentUcodeSize;
	}

	return CELL_OK;
}

int cellRescGetNumColorBuffers(u32 dstMode, u32 palTemporalMode, u32 reserved)
{
	cellResc.Log("cellRescGetNumColorBuffers(dstMode=%d, palTemporalMode=%d, reserved=%d)", dstMode, palTemporalMode, reserved);

	if (reserved != 0)
	{
		cellResc.Error("cellRescGetNumColorBuffers : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return dstMode==CELL_RESC_720x576
		? ((palTemporalMode==CELL_RESC_PAL_60_INTERPOLATE || 
		    palTemporalMode==CELL_RESC_PAL_60_INTERPOLATE_30_DROP || 
		    palTemporalMode==CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE)
			? 6
			: (palTemporalMode==CELL_RESC_PAL_60_DROP
				? 3
				: 2))
		: 2;
}

int cellRescGcmSurface2RescSrc(vm::ptr<CellGcmSurface> gcmSurface, vm::ptr<CellRescSrc> rescSrc)
{
	cellResc.Log("cellRescGcmSurface2RescSrc(gcmSurface_addr=0x%x, rescSrc_addr=0x%x)", gcmSurface.addr(), rescSrc.addr());

	u8 textureFormat = GcmSurfaceFormat2GcmTextureFormat(gcmSurface->colorFormat, gcmSurface->type);
	s32 xW = 1, xH = 1;

	switch(gcmSurface->antialias)
	{
	case CELL_GCM_SURFACE_SQUARE_ROTATED_4: 
		xW=xH=2; 
		break;
	case CELL_GCM_SURFACE_SQUARE_CENTERED_4: 
		xW=xH=2; 
		break; 
	case CELL_GCM_SURFACE_DIAGONAL_CENTERED_2: 
		xW=2; 
		break;
	default:         
		break;
	}

	rescSrc->format  = textureFormat;
	rescSrc->pitch   = gcmSurface->colorPitch[0];
	rescSrc->width   = gcmSurface->width  * xW;
	rescSrc->height  = gcmSurface->height * xH;
	rescSrc->offset  = gcmSurface->colorOffset[0];

	return CELL_OK;
}

int cellRescSetSrc(s32 idx, vm::ptr<CellRescSrc> src)
{
	cellResc.Log("cellRescSetSrc(idx=0x%x, src_addr=0x%x)", idx, src.addr());

	if(!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescSetSrc : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if (idx < 0 || idx >= SRC_BUFFER_NUM || src->width < 1 || src->width > 4096 || src->height < 1 || src->height > 4096)
	{
		cellResc.Error("cellRescSetSrc : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	cellResc.Log(" *** format=0x%x", src->format);
	cellResc.Log(" *** pitch=%d", src->pitch);
	cellResc.Log(" *** width=%d", src->width);
	cellResc.Log(" *** height=%d", src->height);
	cellResc.Log(" *** offset=0x%x", src->offset);

	s_rescInternalInstance->m_rescSrc[idx] = *src;

	cellGcmSetDisplayBuffer(idx, src->offset, src->pitch, src->width, src->height);

	return 0;
}

int cellRescSetConvertAndFlip(vm::ptr<CellGcmContextData> cntxt, s32 idx)
{
	cellResc.Log("cellRescSetConvertAndFlip(cntxt_addr=0x%x, indx=0x%x)", cntxt.addr(), idx);

	if(!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescSetConvertAndFlip : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if(idx < 0 || SRC_BUFFER_NUM <= idx)
	{
		cellResc.Error("cellRescSetConvertAndFlip : CELL_RESC_ERROR_BAD_ARGUMENT");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}


	if (!IsTextureNR())
	{
		BuildupVertexBufferUN(idx);
	}

	// Setup GPU internal status
	SetupRsxRenderingStates(cntxt);

	// Setup vertex array pointers
	SetupVertexArrays(cntxt);

	// Setup surface
	SetupSurfaces(cntxt);

	//TODO: ?

	cellGcmSetPrepareFlip(cntxt, idx);

	return CELL_OK;
}

int cellRescSetWaitFlip()
{
	cellResc.Log("cellRescSetWaitFlip()");
	GSLockCurrent lock(GS_LOCK_WAIT_FLIP);

	return CELL_OK;
}

int cellRescSetBufferAddress(vm::ptr<u32> colorBuffers, vm::ptr<u32> vertexArray, vm::ptr<u32> fragmentShader)
{
	cellResc.Warning("cellRescSetBufferAddress(colorBuffers_addr=0x%x, vertexArray_addr=0x%x, fragmentShader_addr=0x%x)", colorBuffers.addr(), vertexArray.addr(), fragmentShader.addr());

	if(!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescSetBufferAddress : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if(colorBuffers.addr() % COLOR_BUFFER_ALIGNMENT || vertexArray.addr() % VERTEX_BUFFER_ALIGNMENT || fragmentShader.addr() % FRAGMENT_SHADER_ALIGNMENT)
	{
		cellResc.Error("cellRescSetBufferAddress : CELL_RESC_ERROR_BAD_ALIGNMENT");
		return CELL_RESC_ERROR_BAD_ALIGNMENT;
	}

	s_rescInternalInstance->m_colorBuffersEA  = colorBuffers.addr();
	s_rescInternalInstance->m_vertexArrayEA   = vertexArray.addr();
	s_rescInternalInstance->m_fragmentUcodeEA = fragmentShader.addr();

	vm::var<be_t<u32>> dstOffset;
	cellGcmAddressToOffset(s_rescInternalInstance->m_colorBuffersEA, dstOffset);

	for (int i=0; i<GetNumColorBuffers(); i++)
	{
		s_rescInternalInstance->m_dstOffsets[i] = dstOffset.value() + i * s_rescInternalInstance->m_dstBufInterval;
	}

	for (int i=0; i<GetNumColorBuffers(); i++)
	{
		int ret = cellGcmSetDisplayBuffer(i, s_rescInternalInstance->m_dstOffsets[i], s_rescInternalInstance->m_dstPitch, s_rescInternalInstance->m_dstWidth, s_rescInternalInstance->m_dstHeight);
		if (ret) return ret;
	}

	if (IsTextureNR())
	{
		BuildupVertexBufferNR();
	}

	//TODO: ?

	return CELL_OK;
}

void cellRescSetFlipHandler(vm::ptr<void(u32)> handler)
{
	cellResc.Warning("cellRescSetFlipHandler(handler_addr=0x%x)", handler.addr());

	Emu.GetGSManager().GetRender().m_flip_handler = handler;
}

void cellRescResetFlipStatus()
{
	cellResc.Log("cellRescResetFlipStatus()");

	Emu.GetGSManager().GetRender().m_flip_status = 1;
}

int cellRescGetFlipStatus()
{
	cellResc.Log("cellRescGetFlipStatus()");

	return Emu.GetGSManager().GetRender().m_flip_status;
}

int cellRescGetRegisterCount()
{
	UNIMPLEMENTED_FUNC(cellResc);
	return CELL_OK;
}

u64 cellRescGetLastFlipTime()
{
	cellResc.Log("cellRescGetLastFlipTime()");

	return Emu.GetGSManager().GetRender().m_last_flip_time;
}

int cellRescSetRegisterCount()
{
	UNIMPLEMENTED_FUNC(cellResc);
	return CELL_OK;
}

void cellRescSetVBlankHandler(vm::ptr<void(u32)> handler)
{
	cellResc.Warning("cellRescSetVBlankHandler(handler_addr=0x%x)", handler.addr());

	Emu.GetGSManager().GetRender().m_vblank_handler = handler;
}

u16 FloatToHalf(float val)
{
	u8 *tmp = (u8*)&val;
	u32 bits = ((u32)tmp[0] << 24) | ((u32)tmp[1] << 16) | ((u32)tmp[2] << 8) | (u32)tmp[3];

	if (bits == 0) 
	{
		return 0;
	}
	s32 e = ((bits & 0x7f800000) >> 23) - 127 + 15;
	if (e < 0) 
	{
		return 0;
	}
	else if (e > 31) 
	{
		e = 31;
	}
	u32 s = bits & 0x80000000;
	u32 m = bits & 0x007fffff;

	return ((s >> 16) & 0x8000) | ((e << 10) & 0x7c00) | ((m >> 13) & 0x03ff);
}

static void blackman(float window[])
{
	const float x0 = ((1.f * 2.f*PI) / 5.f) - PI;
	const float x1 = ((2.f * 2.f*PI) / 5.f) - PI;
	const float x2 = ((3.f * 2.f*PI) / 5.f) - PI;
	const float x3 = ((4.f * 2.f*PI) / 5.f) - PI;

	const float a0 = 0.42f + (0.50f * cosf(x0)) + (0.08f * cosf(2.f*x0));
	const float a1 = 0.42f + (0.50f * cosf(x1)) + (0.08f * cosf(2.f*x1));
	const float a2 = 0.42f + (0.50f * cosf(x2)) + (0.08f * cosf(2.f*x2));
	const float a3 = 0.42f + (0.50f * cosf(x3)) + (0.08f * cosf(2.f*x3));

	window[0] = ((100.f - SEVIRITY) / 100.f + SEVIRITY / 100.f*a0);
	window[1] = ((100.f - SEVIRITY) / 100.f + SEVIRITY / 100.f*a1);
	window[2] = ((100.f - SEVIRITY) / 100.f + SEVIRITY / 100.f*a2);
	window[3] = ((100.f - SEVIRITY) / 100.f + SEVIRITY / 100.f*a3);
}

int CreateInterlaceTable(u32 ea_addr, float srcH, float dstH, CellRescTableElement depth, int length)
{
	float phi[4], transient[4];
	float y_fraction;
	float bandwidth = 0.5f / (srcH / dstH);
	float phi_b = 2.f * PI * bandwidth;
	float window[4];
	auto buf16 = vm::ptr<u16>::make(ea_addr);
	auto buf32 = vm::ptr<float>::make(ea_addr);

	blackman(window);

	for (int i = 0; i<length; i++) 
	{
		y_fraction = (float)i / (float)length;

		phi[0] = phi_b * (-1.5f - y_fraction);
		phi[1] = phi_b * (-0.5f - y_fraction);
		phi[2] = phi_b * (0.5f - y_fraction);
		phi[3] = phi_b * (1.5f - y_fraction);

		transient[0] = (fabsf(phi[0]) > 1E-10) ? (sinf(phi[0]) / phi[0] * window[0]) : window[0];
		transient[1] = (fabsf(phi[1]) > 1E-10) ? (sinf(phi[1]) / phi[1] * window[1]) : window[1];
		transient[2] = (fabsf(phi[2]) > 1E-10) ? (sinf(phi[2]) / phi[2] * window[2]) : window[2];
		transient[3] = (fabsf(phi[3]) > 1E-10) ? (sinf(phi[3]) / phi[3] * window[3]) : window[3];

		float total4 = transient[0] + transient[1] + transient[2] + transient[3];

		if (depth == CELL_RESC_ELEMENT_HALF)
		{
			buf16[0] = FloatToHalf(transient[0] / total4);
			buf16[1] = FloatToHalf(transient[1] / total4);
			buf16[2] = FloatToHalf(transient[2] / total4);
			buf16[3] = FloatToHalf(transient[3] / total4);
			buf16 += 4;
		}
		else
		{
			buf32[0] = transient[0] / total4;
			buf32[1] = transient[1] / total4;
			buf32[2] = transient[2] / total4;
			buf32[3] = transient[3] / total4;
			buf32 += 4;
		}
	}
	return CELL_OK;
}

int cellRescCreateInterlaceTable(u32 ea_addr, float srcH, CellRescTableElement depth, int length)
{
	cellResc.Warning("cellRescCreateInterlaceTable(ea_addr=0x%x, srcH=%f, depth=%d, length=%d)", ea_addr, srcH, depth, length);

	if (!s_rescInternalInstance->m_bInitialized)
	{
		cellResc.Error("cellRescCreateInterlaceTable : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	}

	if ((ea_addr == 0) || (srcH <= 0.f) || (!(depth == CELL_RESC_ELEMENT_HALF || depth == CELL_RESC_ELEMENT_FLOAT)) || (length <= 0))
	{
		cellResc.Error("cellRescCreateInterlaceTable : CELL_RESC_ERROR_NOT_INITIALIZED");
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	if (s_rescInternalInstance->m_dstHeight == 0)
	{
		cellResc.Error("cellRescCreateInterlaceTable : CELL_RESC_ERROR_BAD_COMBINATION");
		return CELL_RESC_ERROR_BAD_COMBINATION;
	}

	float ratioModeCoefficient = (s_rescInternalInstance->m_initConfig.ratioMode != CELL_RESC_LETTERBOX) ? 1.f : (1.f - 2.f * XY_DELTA_LB);
	float dstH = s_rescInternalInstance->m_dstHeight * ratioModeCoefficient * s_rescInternalInstance->m_ratioAdjY;

	if (int retValue = CreateInterlaceTable(ea_addr, srcH, dstH, depth, length) == CELL_OK)
	{
		s_rescInternalInstance->m_interlaceTableEA = ea_addr;
		s_rescInternalInstance->m_interlaceElement = depth;
		s_rescInternalInstance->m_interlaceTableLength = length;
		return CELL_OK;
	}
	else 
	{
		return retValue;
	}
}


Module cellResc("cellResc", []()
{
	s_rescInternalInstance = new CCellRescInternal();

	cellResc.on_stop = []()
	{
		delete s_rescInternalInstance;
	};

	REG_FUNC(cellResc, cellRescSetConvertAndFlip);
	REG_FUNC(cellResc, cellRescSetWaitFlip);
	REG_FUNC(cellResc, cellRescSetFlipHandler);
	REG_FUNC(cellResc, cellRescGcmSurface2RescSrc);
	REG_FUNC(cellResc, cellRescGetNumColorBuffers);
	REG_FUNC(cellResc, cellRescSetDsts);
	REG_FUNC(cellResc, cellRescResetFlipStatus);
	REG_FUNC(cellResc, cellRescSetPalInterpolateDropFlexRatio);
	REG_FUNC(cellResc, cellRescGetRegisterCount);
	REG_FUNC(cellResc, cellRescAdjustAspectRatio);
	REG_FUNC(cellResc, cellRescSetDisplayMode);
	REG_FUNC(cellResc, cellRescExit);
	REG_FUNC(cellResc, cellRescInit);
	REG_FUNC(cellResc, cellRescGetBufferSize);
	REG_FUNC(cellResc, cellRescGetLastFlipTime);
	REG_FUNC(cellResc, cellRescSetSrc);
	REG_FUNC(cellResc, cellRescSetRegisterCount);
	REG_FUNC(cellResc, cellRescSetBufferAddress);
	REG_FUNC(cellResc, cellRescGetFlipStatus);
	REG_FUNC(cellResc, cellRescVideoOutResolutionId2RescBufferMode);
	REG_FUNC(cellResc, cellRescSetVBlankHandler);
	REG_FUNC(cellResc, cellRescCreateInterlaceTable);
});
