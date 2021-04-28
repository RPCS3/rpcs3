#pragma once

enum CellRescError : u32
{
	CELL_RESC_ERROR_NOT_INITIALIZED   = 0x80210301,
	CELL_RESC_ERROR_REINITIALIZED     = 0x80210302,
	CELL_RESC_ERROR_BAD_ALIGNMENT     = 0x80210303,
	CELL_RESC_ERROR_BAD_ARGUMENT      = 0x80210304,
	CELL_RESC_ERROR_LESS_MEMORY       = 0x80210305,
	CELL_RESC_ERROR_GCM_FLIP_QUE_FULL = 0x80210306,
	CELL_RESC_ERROR_BAD_COMBINATION   = 0x80210307,
	CELL_RESC_ERROR_x308              = 0x80210308, // TODO: find proper name
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
	CELL_RESC_CONSTANT_GPU_LOAD      = 0x0,
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
	be_t<u32> size; // usz
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

struct cell_resc_manager
{
	atomic_t<bool> is_initialized = false;

	u32 buffer_mode{};

	struct
	{
		u32 size{};
		u32 resource_policy{};
		u32 support_modes{};
		u32 ratio_mode{};
		u32 pal_temporal_mode{};
		u32 interlace_mode{};
		u32 flip_mode{};
	} config;
};
