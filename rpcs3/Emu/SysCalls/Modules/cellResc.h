#pragma once

#define roundup(x,a) (((x)+(a)-1)&(~((a)-1)))
#define SEVIRITY 80.f

#include "Emu/RSX/GCM.h"

enum
{
	CELL_RESC_ERROR_NOT_INITIALIZED = 0x80210301,
	CELL_RESC_ERROR_REINITIALIZED = 0x80210302,
	CELL_RESC_ERROR_BAD_ALIGNMENT = 0x80210303,
	CELL_RESC_ERROR_BAD_ARGUMENT = 0x80210304,
	CELL_RESC_ERROR_LESS_MEMORY = 0x80210305,
	CELL_RESC_ERROR_GCM_FLIP_QUE_FULL = 0x80210306,
	CELL_RESC_ERROR_BAD_COMBINATION = 0x80210307,
};

enum
{
	COLOR_BUFFER_ALIGNMENT = 128,
	VERTEX_BUFFER_ALIGNMENT = 4,
	FRAGMENT_SHADER_ALIGNMENT = 64,
	VERTEX_NUMBER_NORMAL = 4,

	SRC_BUFFER_NUM = 8,
	MAX_DST_BUFFER_NUM = 6,
	RESC_PARAM_NUM
};

enum CellRescBufferMode
{
	CELL_RESC_720x480   = 0x1, 
	CELL_RESC_720x576   = 0x2, 
	CELL_RESC_1280x720  = 0x4, 
	CELL_RESC_1920x1080 = 0x8, 
};

enum CellRescPalTemporalMode
{
	CELL_RESC_PAL_50                            = 0, 
	CELL_RESC_PAL_60_DROP                       = 1, 
	CELL_RESC_PAL_60_INTERPOLATE                = 2, 
	CELL_RESC_PAL_60_INTERPOLATE_30_DROP        = 3, 
	CELL_RESC_PAL_60_INTERPOLATE_DROP_FLEXIBLE  = 4, 
	CELL_RESC_PAL_60_FOR_HSYNC                  = 5, 
};

enum CellRescRatioConvertMode
{
	CELL_RESC_FULLSCREEN             = 0, 
	CELL_RESC_LETTERBOX              = 1, 
	CELL_RESC_PANSCAN                = 2, 
};

enum CellRescFlipMode
{
	CELL_RESC_DISPLAY_VSYNC          = 0, 
	CELL_RESC_DISPLAY_HSYNC          = 1, 
};

enum CellRescDstFormat
{
	CELL_RESC_SURFACE_A8R8G8B8       = 8,  // == CELL_GCM_SURFACE_A8R8G8B8
	CELL_RESC_SURFACE_F_W16Z16Y16X16 = 11, // == CELL_GCM_SURFACE_F_W16Z16Y16X16
};

enum CellRescResourcePolicy
{
	CELL_RESC_CONSTANT_VRAM          = 0x0, 
	CELL_RESC_MINIMUM_VRAM           = 0x1, 
	CELL_RESC_MINIMUM_GPU_LOAD       = 0x2, 
};

enum CellRescConvolutionFilterMode
{
	CELL_RESC_NORMAL_BILINEAR        = 0, 
	CELL_RESC_INTERLACE_FILTER       = 1, 
	CELL_RESC_3X3_GAUSSIAN           = 2, 
	CELL_RESC_2X3_QUINCUNX           = 3, 
	CELL_RESC_2X3_QUINCUNX_ALT       = 4, 
};

struct CellRescDsts
{
	be_t<u32> format;
	be_t<u32> pitch;
	be_t<u32> heightAlign;
};

struct CellRescInitConfig
{
	be_t<u32> size; // size_t
	be_t<u32> resourcePolicy;
	be_t<u32> supportModes;
	be_t<u32> ratioMode;
	be_t<u32> palTemporalMode;
	be_t<u32> interlaceMode;
	be_t<u32> flipMode;
};

struct CellRescSrc
{
	be_t<u32> format;
	be_t<u32> pitch;
	be_t<u16> width;
	be_t<u16> height;
	be_t<u32> offset;
};

struct RescVertex_t
{
	be_t<float> Px, Py;
	be_t<float> u, v;
	be_t<float> u2, v2;
};

struct CCellRescInternal
{
	CellRescInitConfig m_initConfig;
	CellRescSrc m_rescSrc[SRC_BUFFER_NUM];
	u32 m_dstMode;
	CellRescDsts m_rescDsts[4], *m_pRescDsts;
	CellRescTableElement m_interlaceElement;

	u32 m_colorBuffersEA, m_vertexArrayEA, m_fragmentUcodeEA;
	u32 m_bufIdFront;
	s32 m_dstWidth, m_dstHeight, m_dstPitch;
	u16 m_srcWidthInterlace, m_srcHeightInterlace;
	u32 m_dstBufInterval, m_dstOffsets[MAX_DST_BUFFER_NUM];
	s32 m_nVertex;
	u32 m_bufIdFrontPrevDrop, m_bufIdPalMidPrev, m_bufIdPalMidNow;
	u32 m_interlaceTableEA;
	int m_interlaceTableLength;
	float m_ratioAdjX, m_ratioAdjY, m_flexRatio;
	bool m_bInitialized, m_bNewlyAdjustRatio;
	bool m_isDummyFlipped;
	u8 m_cgParamIndex[RESC_PARAM_NUM];
	u64 m_commandIdxCaF, m_rcvdCmdIdx;
	u32 s_applicationFlipHandler;
	u32 s_applicationVBlankHandler;

	CCellRescInternal()
		: m_bInitialized(false)
	{
	}
};


CCellRescInternal* s_rescInternalInstance = nullptr;

// Extern Functions
extern int cellGcmSetFlipMode(u32 mode);
extern void cellGcmSetFlipHandler(u32 handler_addr);
extern void cellGcmSetVBlankHandler(u32 handler_addr);
extern int cellGcmAddressToOffset(u64 address, vm::ptr<be_t<u32>> offset);
extern int cellGcmSetDisplayBuffer(u32 id, u32 offset, u32 pitch, u32 width, u32 height);
extern int cellGcmSetPrepareFlip(vm::ptr<CellGcmContextData> ctx, u32 id);
extern int cellGcmSetSecondVFrequency(u32 freq);
extern u32 cellGcmGetLabelAddress(u8 index);
extern u32 cellGcmGetTiledPitchSize(u32 size);

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