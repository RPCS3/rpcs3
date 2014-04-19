#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"
#include "Emu/GS/GCM.h"
#include "cellResc.h"

void cellResc_init();
void cellResc_load();
void cellResc_unload();
Module cellResc(0x001f, cellResc_init, cellResc_load, cellResc_unload);

// Error Codes
enum
{
	CELL_RESC_ERROR_NOT_INITIALIZED     = 0x80210301,
	CELL_RESC_ERROR_REINITIALIZED       = 0x80210302,
	CELL_RESC_ERROR_BAD_ALIGNMENT       = 0x80210303,
	CELL_RESC_ERROR_BAD_ARGUMENT        = 0x80210304,
	CELL_RESC_ERROR_LESS_MEMORY         = 0x80210305,
	CELL_RESC_ERROR_GCM_FLIP_QUE_FULL   = 0x80210306,
	CELL_RESC_ERROR_BAD_COMBINATION     = 0x80210307,
};

enum
{
	COLOR_BUFFER_ALIGNMENT        = 128,
	VERTEX_BUFFER_ALIGNMENT       = 4,
	FRAGMENT_SHADER_ALIGNMENT     = 64,
	VERTEX_NUMBER_NORMAL          = 4,

	SRC_BUFFER_NUM                = 8,
	MAX_DST_BUFFER_NUM            = 6,
};

static const float
	PICTURE_SIZE                  = (1.0f),
	UV_DELTA_PS                   = (1.f / 8.f),
	UV_DELTA_LB                   = (1.f / 6.f);


struct RescVertex_t
{
	be_t<float> Px, Py;
	be_t<float> u, v;
	be_t<float> u2, v2;
};

// Defines
#define roundup(x,a) (((x)+(a)-1)&(~((a)-1)))

struct CCellRescInternal
{
	CellRescInitConfig m_initConfig;
	CellRescSrc m_rescSrc[SRC_BUFFER_NUM];
	u32 m_dstMode;
	CellRescDsts m_rescDsts[4], *m_pRescDsts;

	u32 m_colorBuffersEA_addr, m_vertexArrayEA_addr, m_fragmentUcodeEA_addr;
	s32 m_dstWidth, m_dstHeight, m_dstPitch;
	u16 m_srcWidthInterlace, m_srcHeightInterlace;
	u32 m_dstBufInterval, m_dstOffsets[MAX_DST_BUFFER_NUM];
	s32 m_nVertex;
	float m_ratioAdjX, m_ratioAdjY;
	bool m_bInitialized, m_bNewlyAdjustRatio;
	
	float m_flexRatio;

	CCellRescInternal()
		: m_bInitialized(false)
	{
	}
};


CCellRescInternal* s_rescInternalInstance = nullptr;

// Extern Functions
extern int cellGcmSetFlipMode(u32 mode);
extern int cellGcmSetFlipHandler(u32 handler_addr);
extern int32_t cellGcmAddressToOffset(u64 address, mem32_t offset);
extern int cellGcmSetDisplayBuffer(u32 id, u32 offset, u32 pitch, u32 width, u32 height);
extern int cellGcmSetPrepareFlip(mem_ptr_t<CellGcmContextData> ctx, u32 id);
extern int cellGcmSetSecondVFrequency(u32 freq);
int cellRescGetNumColorBuffers(u32 dstMode, u32 palTemporalMode, u32 reserved);

// Help Functions
inline bool IsPal()            { return s_rescInternalInstance->m_dstMode == CELL_RESC_720x576; }
inline bool IsPal60Hsync()     { return (IsPal() && s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_FOR_HSYNC); }
inline bool IsPalDrop()        { return (IsPal() && s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_DROP); }
inline bool IsPalInterpolate() { return (IsPal() && ((s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_INTERPOLATE)
												  || (s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_INTERPOLATE_30_DROP)
												  || (s_rescInternalInstance->m_initConfig.palTemporalMode == CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE))); }
inline int GetNumColorBuffers(){ return IsPalInterpolate() ? 6 : (IsPalDrop() ? 3 : 2); }
inline bool IsInterlace()      { return s_rescInternalInstance->m_initConfig.interlaceMode == CELL_RESC_INTERLACE_FILTER; }
inline bool IsTextureNR()      { return !IsInterlace(); }

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

	mem_ptr_t<RescVertex_t> vv(s_rescInternalInstance->m_vertexArrayEA_addr);

	if(s_rescInternalInstance->m_dstMode == CELL_RESC_720x480 || s_rescInternalInstance->m_dstMode == CELL_RESC_720x576){
		switch(s_rescInternalInstance->m_initConfig.ratioMode){
		case CELL_RESC_LETTERBOX:  goto NR_LETTERBOX;
		case CELL_RESC_PANSCAN:    goto NR_PANSCAN;
		default:                   goto NR_FULLSCREEN;
		}
	} else {
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
	if(s_rescInternalInstance->m_bNewlyAdjustRatio){
		s_rescInternalInstance->m_srcWidthInterlace  = s_rescInternalInstance->m_rescSrc[srcIdx].width;
		s_rescInternalInstance->m_srcHeightInterlace = s_rescInternalInstance->m_rescSrc[srcIdx].height;
		s_rescInternalInstance->m_bNewlyAdjustRatio = false;
	} else {
		if(s_rescInternalInstance->m_srcWidthInterlace == s_rescInternalInstance->m_rescSrc[srcIdx].width 
		   && s_rescInternalInstance->m_srcHeightInterlace == s_rescInternalInstance->m_rescSrc[srcIdx].height){
			return;
		} else {
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
	float U2_FS1 = s_rescInternalInstance->m_dstWidth;
	float V2_FS1 = s_rescInternalInstance->m_dstHeight;

	mem_ptr_t<RescVertex_t> vv(s_rescInternalInstance->m_vertexArrayEA_addr);

	if(s_rescInternalInstance->m_dstMode == CELL_RESC_720x480 || s_rescInternalInstance->m_dstMode == CELL_RESC_720x576){
		switch(s_rescInternalInstance->m_initConfig.ratioMode){
		case CELL_RESC_LETTERBOX:  goto UN_LETTERBOX;
		case CELL_RESC_PANSCAN:    goto UN_PANSCAN;
		default:                   goto UN_FULLSCREEN;
		}
	} else {
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

inline int InternalVersion(mem_ptr_t<CellRescInitConfig> conf)
{
	switch (conf->size)
	{
		case 20: return 1;
		case 24: return 2;
		case 28: return 3;
		default: return -1;
	}
}

inline int InternalVersion() {
	switch (s_rescInternalInstance->m_initConfig.size)
	{
		case 20: return 1;
		case 24: return 2;
		case 28: return 3;
		default: return -1;
	}
}

u8 RescBufferMode2SysutilResolutionId(u32 bufferMode)
{
	switch (bufferMode)
	{
	case CELL_RESC_720x576:	  return CELL_VIDEO_OUT_RESOLUTION_576;
	case CELL_RESC_1280x720:  return CELL_VIDEO_OUT_RESOLUTION_720;
	case CELL_RESC_1920x1080: return CELL_VIDEO_OUT_RESOLUTION_1080;
	default:                  return CELL_VIDEO_OUT_RESOLUTION_480;
	}
}

u8 RescDstFormat2SysutilFormat(u32 dstFormat)
{
	switch (dstFormat) {
	case CELL_RESC_SURFACE_F_W16Z16Y16X16: return CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_R16G16B16X16_FLOAT;
	default:                               return CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	}
}

u8 GcmSurfaceFormat2GcmTextureFormat(u8 surfaceFormat, u8 surfaceType)
{
	u8 result = 0;

	switch(surfaceFormat){
	case 8:	 result = 0x85; break; //case CELL_GCM_SURFACE_A8R8G8B8:	      result = CELL_GCM_TEXTURE_A8R8G8B8;	            break;
	case 11: result = 0x9A;	break; //case CELL_GCM_SURFACE_F_W16Z16Y16X16:    result = CELL_GCM_TEXTURE_W16_Z16_Y16_X16_FLOAT;	break;
	default: return 0xFF; //Error
	}

	switch(surfaceType){
	case 1: result |= 0x20; break; //case CELL_GCM_SURFACE_PITCH:   result |= CELL_GCM_TEXTURE_LN; break; 
	case 2: result |= 0x00; break; //case CELL_GCM_SURFACE_SWIZZLE: result |= CELL_GCM_TEXTURE_SZ; break;
	default: return 0xFF; //Error
	}

	result |= 0x00; //result |= CELL_GCM_TEXTURE_NR
	return result;
}

int GetRescDestsIndex(u32 dstMode)
{
	switch(dstMode)
	{
	case CELL_RESC_720x480:   return 0;
	case CELL_RESC_720x576:   return 1;
	case CELL_RESC_1280x720:  return 2;
	case CELL_RESC_1920x1080: return 3;
	default: return -1;
	}
}

void GetScreenSize(u32 mode, s32 *width, s32 *height)
{	
	switch (mode){
	case CELL_RESC_720x480:   *width = 720;  *height = 480;  break;
	case CELL_RESC_720x576:   *width = 720;  *height = 576;  break;
	case CELL_RESC_1280x720:  *width = 1280; *height = 720;	 break;
	case CELL_RESC_1920x1080: *width = 1920; *height = 1080; break;
	default: *width = *height = 0; break;
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
	for(u32 bufMode = CELL_RESC_720x480; bufMode <= CELL_RESC_1920x1080; bufMode <<= 1){
		if(s_rescInternalInstance->m_initConfig.supportModes & bufMode) {
			oneBufSize   = CalculateSurfaceByteSize(bufMode, &(s_rescInternalInstance->m_rescDsts[GetRescDestsIndex(bufMode)]));
			bufNum       = cellRescGetNumColorBuffers(bufMode, s_rescInternalInstance->m_initConfig.palTemporalMode, 0);
			totalBufSize = oneBufSize * bufNum;
			maxBufSize   = (maxBufSize > totalBufSize) ? maxBufSize : totalBufSize;
		}
	}
	return maxBufSize;
}

bool CheckInitConfig(mem_ptr_t<CellRescInitConfig> initConfig)
{
	if( (initConfig->resourcePolicy & ~((u32)0x3)) ||
		(initConfig->supportModes & 0xF) == 0 ||
		(initConfig->ratioMode > 2) ||
		(initConfig->palTemporalMode > 5) )
		return false;

	if( InternalVersion() >= 2 ){
		if(InternalVersion() == 2 && initConfig->interlaceMode > 1) 
			return false; 
	}

	if( InternalVersion() >= 3 ){
		if(initConfig->interlaceMode > 4) return false;
		if(initConfig->flipMode > 1)      return false;
	}

	return true;
}

void InitMembers()
{
}

void InitContext(mem_ptr_t<CellGcmContextData>& cntxt)
{
	//TODO: use cntxt
	GSLockCurrent lock(GS_LOCK_WAIT_FLUSH);
	GSRender& r = Emu.GetGSManager().GetRender();
	r.m_set_color_mask = true; r.m_color_mask_a = r.m_color_mask_r = r.m_color_mask_g = r.m_color_mask_b = true;
	r.m_set_depth_mask = true; r.m_depth_mask = 0;
	r.m_set_alpha_test = false;
	r.m_set_blend = false;
	//GcmCmdTypePrefix::cellGcmSetBlendEnableMrt(con, CELL_GCM_FALSE, CELL_GCM_FALSE, CELL_GCM_FALSE);
	r.m_set_logic_op = false;
	r.m_set_cull_face_enable = false;
	r.m_set_depth_bounds_test = false;
	r.m_depth_test_enable = false;
	//GcmCmdTypePrefix::cellGcmSetPolygonOffsetFillEnable(con, CELL_GCM_FALSE);
	r.m_set_stencil_test = false;
	r.m_set_two_sided_stencil_test_enable = false;
	//GcmCmdTypePrefix::cellGcmSetPointSpriteControl(con, CELL_GCM_FALSE, 0, 0);
	r.m_set_dither = true;
	r.m_set_shade_mode = true; r.m_shade_mode = 0x1D01; //CELL_GCM_SMOOTH
	//GcmCmdTypePrefix::cellGcmSetFrequencyDividerOperation(con, 0);

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
	r.m_surface_colour_target = 1;

	if (IsPalInterpolate()) {
		//MRT
		//GcmCmdTypePrefix::cellGcmSetColorMaskMrt(con, CELL_GCM_COLOR_MASK_MRT1_A | CELL_GCM_COLOR_MASK_MRT1_R | CELL_GCM_COLOR_MASK_MRT1_G | CELL_GCM_COLOR_MASK_MRT1_B);
	}
}

void InitVertex(mem_ptr_t<CellGcmContextData>& cntxt)
{
	GSLockCurrent lock(GS_LOCK_WAIT_FLUSH);
	GSRender& r = Emu.GetGSManager().GetRender();
	
	//TODO
}

// Module Functions
int cellRescInit(mem_ptr_t<CellRescInitConfig> initConfig)
{
	cellResc.Warning("cellRescInit(initConfig_addr=0x%x)", initConfig.GetAddr());

	if(s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_REINITIALIZED;
	if(!initConfig.IsGood() || InternalVersion(initConfig.GetAddr()) == -1 || !CheckInitConfig(initConfig))
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	InitMembers();
	s_rescInternalInstance->m_initConfig = *initConfig; // TODO: This may be incompatible with older binaries
	s_rescInternalInstance->m_bInitialized = true;

	return CELL_OK;
}

void cellRescExit()
{
	if(!s_rescInternalInstance->m_bInitialized)
		return;

	// TODO: ?

	s_rescInternalInstance->m_bInitialized = false;
}

int cellRescVideoOutResolutionId2RescBufferMode(u32 resolutionId, mem32_t bufferMode)
{
	cellResc.Log("cellRescVideoOutResolutionId2RescBufferMode(resolutionId=%d, bufferMode_addr=0x%x)",
		resolutionId, bufferMode.GetAddr());

	if (!bufferMode.IsGood())
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	switch (resolutionId)
	{
	case CELL_VIDEO_OUT_RESOLUTION_1080: bufferMode = CELL_RESC_1920x1080; break;
	case CELL_VIDEO_OUT_RESOLUTION_720:  bufferMode = CELL_RESC_1280x720;  break;
	case CELL_VIDEO_OUT_RESOLUTION_480:  bufferMode = CELL_RESC_720x480;   break;
	case CELL_VIDEO_OUT_RESOLUTION_576:  bufferMode = CELL_RESC_720x576;   break;
	default: return CELL_RESC_ERROR_BAD_ARGUMENT;
	}

	return CELL_OK;
}

int cellRescSetDsts(u32 dstsMode, mem_ptr_t<CellRescDsts> dsts)
{
	cellResc.Log("cellRescSetDsts(dstsMode=%d, CellRescDsts_addr=0x%x)", dstsMode, dsts.GetAddr());

	if(!s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	if (!dsts.IsGood())
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	if((dstsMode != CELL_RESC_720x480)  && (dstsMode != CELL_RESC_720x576) && 
	   (dstsMode != CELL_RESC_1280x720) && (dstsMode != CELL_RESC_1920x1080))
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	s_rescInternalInstance->m_rescDsts[GetRescDestsIndex(dstsMode)] = *dsts;

	return CELL_OK;
}

int cellRescSetDisplayMode(u32 displayMode)
{
	cellResc.Warning("cellRescSetDisplayMode(displayMode=%d)", displayMode);

	if(!s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	if(!(s_rescInternalInstance->m_initConfig.supportModes & displayMode))
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	if((displayMode != CELL_RESC_720x480)  && (displayMode != CELL_RESC_720x576) && 
	   (displayMode != CELL_RESC_1280x720) && (displayMode != CELL_RESC_1920x1080))
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	s_rescInternalInstance->m_dstMode = displayMode;

	if((IsPalInterpolate() || IsPalDrop()) && s_rescInternalInstance->m_initConfig.flipMode==CELL_RESC_DISPLAY_HSYNC)
		return CELL_RESC_ERROR_BAD_COMBINATION;
	if(IsPal60Hsync() && s_rescInternalInstance->m_initConfig.flipMode==CELL_RESC_DISPLAY_VSYNC)
		return CELL_RESC_ERROR_BAD_COMBINATION;

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

	MemoryAllocator<CellVideoOutConfiguration> videocfg;
	videocfg->resolutionId = RescBufferMode2SysutilResolutionId(s_rescInternalInstance->m_dstMode);
	videocfg->format       = RescDstFormat2SysutilFormat(s_rescInternalInstance->m_pRescDsts->format );
	videocfg->aspect       = CELL_VIDEO_OUT_ASPECT_AUTO;
	videocfg->pitch        = s_rescInternalInstance->m_dstPitch;

	cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, videocfg.GetAddr(), NULL, 0);

	if (IsPalInterpolate())
	{
		//int ret = InitSystemResource();
		//if (ret) return ret;
		//InitLabels();
		cellGcmSetSecondVFrequency(CELL_GCM_DISPLAY_FREQUENCY_59_94HZ);
		//cellGcmSetVBlankHandler(CCellRescInternal::IntrHandler50);
		//cellGcmSetSecondVHandler(CCellRescInternal::IntrHandler60);
		cellGcmSetFlipHandler(NULL);
	}
	else if (IsPalDrop())
	{
		//InitLabels();
		cellGcmSetSecondVFrequency(CELL_GCM_DISPLAY_FREQUENCY_59_94HZ);
		//cellGcmSetVBlankHandler(NULL);
		//cellGcmSetSecondVHandler(CCellRescInternal::IntrHandler60Drop);
		cellGcmSetFlipHandler(NULL);
	} 
	else if (IsPal60Hsync())
	{
		cellGcmSetSecondVFrequency(CELL_GCM_DISPLAY_FREQUENCY_59_94HZ);
		//cellGcmSetVBlankHandler(NULL);
	}

	//if(s_applicationVBlankHandler) SetVBlankHandler(s_applicationVBlankHandler);
	//if(s_applicationFlipHandler)   SetFlipHandler(s_applicationFlipHandler);
	cellGcmSetFlipMode((s_rescInternalInstance->m_initConfig.flipMode == CELL_RESC_DISPLAY_VSYNC) 
						? CELL_GCM_DISPLAY_VSYNC : CELL_GCM_DISPLAY_HSYNC);
	return CELL_OK;
}

int cellRescAdjustAspectRatio(float horizontal, float vertical)
{
	cellResc.Warning("cellRescAdjustAspectRatio(horizontal=%f, vertical=%f)", horizontal, vertical);

	if(!s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	if((horizontal < 0.5f || 2.f < horizontal) || (vertical < 0.5f || 2.f < vertical))
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	s_rescInternalInstance->m_ratioAdjX = horizontal;
	s_rescInternalInstance->m_ratioAdjY = vertical;

	if(s_rescInternalInstance->m_vertexArrayEA_addr)
	{
		if(IsTextureNR())
			BuildupVertexBufferNR();
		else
			s_rescInternalInstance->m_bNewlyAdjustRatio = true;
	}

	return CELL_OK;
}

int cellRescSetPalInterpolateDropFlexRatio(float ratio)
{
	cellResc.Warning("cellRescSetPalInterpolateDropFlexRatio(ratio=%f)", ratio);

	if(!s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	if(ratio < 0.f || 1.f < ratio)
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	s_rescInternalInstance->m_flexRatio = ratio;

	return CELL_OK;
}

int cellRescGetBufferSize(mem32_t colorBuffers, mem32_t vertexArray, mem32_t fragmentShader)
{
	cellResc.Warning("cellRescGetBufferSize(colorBuffers_addr=0x%x, vertexArray_addr=0x%x, fragmentShader_addr=0x%x)",
		colorBuffers.GetAddr(), vertexArray.GetAddr(), fragmentShader.GetAddr());

	if(!s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_NOT_INITIALIZED;

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

	if(colorBuffers.IsGood())   colorBuffers = colorBuffersSize;
	if(vertexArray.IsGood())    vertexArray = vertexArraySize;
	if(fragmentShader.IsGood()) fragmentShader = fragmentUcodeSize;

	return CELL_OK;
}

int cellRescGetNumColorBuffers(u32 dstMode, u32 palTemporalMode, u32 reserved)
{
	cellResc.Log("cellRescGetNumColorBuffers(dstMode=%d, palTemporalMode=%d, reserved=%d)", dstMode, palTemporalMode, reserved);

	if(reserved != 0)
		return CELL_RESC_ERROR_BAD_ARGUMENT;

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

int cellRescGcmSurface2RescSrc(mem_ptr_t<CellGcmSurface> gcmSurface, mem_ptr_t<CellRescSrc> rescSrc)
{
	cellResc.Log("cellRescGcmSurface2RescSrc(gcmSurface_addr=0x%x, rescSrc_addr=0x%x)",
		gcmSurface.GetAddr(), rescSrc.GetAddr());

	if(!gcmSurface.IsGood() || !rescSrc.IsGood())
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	u8 textureFormat = GcmSurfaceFormat2GcmTextureFormat(gcmSurface->color_format, gcmSurface->type);

	s32 xW = 1, xH = 1;
	switch(gcmSurface->antialias)
	{
	case 5: xW=xH=2; break; //case CELL_GCM_SURFACE_SQUARE_ROTATED_4:
	case 4: xW=xH=2; break; //case CELL_GCM_SURFACE_SQUARE_CENTERED_4:
	case 3: xW=2;    break; //case CELL_GCM_SURFACE_DIAGONAL_CENTERED_2:
	default:         break;
	}

	rescSrc->format  = textureFormat;
	rescSrc->pitch   = re(gcmSurface->color_pitch[0]);
	rescSrc->width   = re(gcmSurface->width)  * xW;
	rescSrc->height  = re(gcmSurface->height) * xH;
	rescSrc->offset  = re(gcmSurface->color_offset[0]);

	return CELL_OK;
}

int cellRescSetSrc(s32 idx, mem_ptr_t<CellRescSrc> src)
{
	cellResc.Log("cellRescSetSrc(idx=0x%x, src_addr=0x%x)", idx, src.GetAddr());

	if(!s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	if(idx < 0 || SRC_BUFFER_NUM <= idx || !src.IsGood())
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	if(src->width < 1 || 4096 < src->width || src->height < 1 || 4096 < src->height)
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	cellResc.Log(" *** format=0x%x", src->format.ToLE());
	cellResc.Log(" *** pitch=%d", src->pitch.ToLE());
	cellResc.Log(" *** width=%d", src->width.ToLE());
	cellResc.Log(" *** height=%d", src->height.ToLE());
	cellResc.Log(" *** offset=0x%x", src->offset.ToLE());
	//Emu.GetGSManager().GetRender().SetData(src.offset, 800, 600);
	//Emu.GetGSManager().GetRender().Draw();

	s_rescInternalInstance->m_rescSrc[idx] = *src;

	cellGcmSetDisplayBuffer(idx, src->offset, src->pitch, src->width, src->height);

	return 0;
}

int cellRescSetConvertAndFlip(mem_ptr_t<CellGcmContextData> cntxt, s32 idx)
{
	cellResc.Log("cellRescSetConvertAndFlip(cntxt_addr=0x%x, indx=0x%x)", cntxt.GetAddr(), idx);

	if(!s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	if(idx < 0 || SRC_BUFFER_NUM <= idx)
		return CELL_RESC_ERROR_BAD_ARGUMENT;

	if(!IsTextureNR())
		BuildupVertexBufferUN(idx);

	InitContext(cntxt);
	InitVertex(cntxt);

	//TODO: ?

	cellGcmSetPrepareFlip(cntxt, idx);

	return CELL_OK;
}

int cellRescSetWaitFlip()
{
	cellResc.Log("cellRescSetWaitFlip()");
	GSLockCurrent lock(GS_LOCK_WAIT_FLIP); // could stall on exit

	return CELL_OK;
}

int cellRescSetBufferAddress(mem32_t colorBuffers, mem32_t vertexArray, mem32_t fragmentShader)
{
	cellResc.Warning("cellRescSetBufferAddress(colorBuffers_addr=0x%x, vertexArray_addr=0x%x, fragmentShader_addr=0x%x)",
		colorBuffers.GetAddr(), vertexArray.GetAddr(), fragmentShader.GetAddr());

	if(!s_rescInternalInstance->m_bInitialized)
		return CELL_RESC_ERROR_NOT_INITIALIZED;
	if(!colorBuffers.IsGood() || !vertexArray.IsGood() || !fragmentShader.IsGood()) 
		return CELL_RESC_ERROR_BAD_ARGUMENT;
	if(colorBuffers.GetAddr() % COLOR_BUFFER_ALIGNMENT ||
		vertexArray.GetAddr() % VERTEX_BUFFER_ALIGNMENT ||
		fragmentShader.GetAddr() % FRAGMENT_SHADER_ALIGNMENT)
		return CELL_RESC_ERROR_BAD_ALIGNMENT;

	s_rescInternalInstance->m_colorBuffersEA_addr  = colorBuffers.GetAddr();
	s_rescInternalInstance->m_vertexArrayEA_addr   = vertexArray.GetAddr();
	s_rescInternalInstance->m_fragmentUcodeEA_addr = fragmentShader.GetAddr();

	MemoryAllocator<be_t<u32>> dstOffset;
	cellGcmAddressToOffset(s_rescInternalInstance->m_colorBuffersEA_addr, dstOffset.GetAddr());

	for(int i=0; i<GetNumColorBuffers(); i++)
	{
		s_rescInternalInstance->m_dstOffsets[i] = dstOffset->ToLE() + i * s_rescInternalInstance->m_dstBufInterval;
	}

	for(int i=0; i<GetNumColorBuffers(); i++)
	{
		int ret = cellGcmSetDisplayBuffer(i, s_rescInternalInstance->m_dstOffsets[i], s_rescInternalInstance->m_dstPitch, s_rescInternalInstance->m_dstWidth, s_rescInternalInstance->m_dstHeight);
		if (ret) return ret;
	}

	if(IsTextureNR())
		BuildupVertexBufferNR();

	//TODO: ?

	return CELL_OK;
}

int cellRescSetFlipHandler(u32 handler_addr)
{
	cellResc.Warning("cellRescSetFlipHandler(handler_addr=0x%x)", handler_addr);

	if(handler_addr != 0 && !Memory.IsGoodAddr(handler_addr))
		return CELL_EFAULT;

	Emu.GetGSManager().GetRender().m_flip_handler.SetAddr(handler_addr);

	return CELL_OK;
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

void cellResc_init()
{
	cellResc.AddFunc(0x25c107e6, cellRescSetConvertAndFlip);
	cellResc.AddFunc(0x0d3c22ce, cellRescSetWaitFlip);
	cellResc.AddFunc(0x2ea94661, cellRescSetFlipHandler);
	cellResc.AddFunc(0x01220224, cellRescGcmSurface2RescSrc);
	cellResc.AddFunc(0x0a2069c7, cellRescGetNumColorBuffers);
	cellResc.AddFunc(0x10db5b1a, cellRescSetDsts);
	cellResc.AddFunc(0x129922a0, cellRescResetFlipStatus);
	cellResc.AddFunc(0x19a2a967, cellRescSetPalInterpolateDropFlexRatio);
	//cellResc.AddFunc(0x1dd3c4cd, cellRescGetRegisterCount);
	cellResc.AddFunc(0x22ae06d8, cellRescAdjustAspectRatio);
	cellResc.AddFunc(0x23134710, cellRescSetDisplayMode);
	cellResc.AddFunc(0x2ea3061e, cellRescExit);
	cellResc.AddFunc(0x516ee89e, cellRescInit);
	cellResc.AddFunc(0x5a338cdb, cellRescGetBufferSize);
	//cellResc.AddFunc(0x66f5e388, cellRescGetLastFlipTime);
	cellResc.AddFunc(0x6cd0f95f, cellRescSetSrc);
	//cellResc.AddFunc(0x7af8a37f, cellRescSetRegisterCount);
	cellResc.AddFunc(0x8107277c, cellRescSetBufferAddress);
	cellResc.AddFunc(0xc47c5c22, cellRescGetFlipStatus);
	cellResc.AddFunc(0xd1ca0503, cellRescVideoOutResolutionId2RescBufferMode);
	//cellResc.AddFunc(0xd3758645, cellRescSetVBlankHandler);
	//cellResc.AddFunc(0xe0cef79e, cellRescCreateInterlaceTable);
}

void cellResc_load()
{
	s_rescInternalInstance = new CCellRescInternal();
}

void cellResc_unload()
{
	// s_rescInternalInstance->m_bInitialized = false;
	delete s_rescInternalInstance;
}
