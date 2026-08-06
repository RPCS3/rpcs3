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
	MAX_DST_BUFFER_NUM = 6
};

enum CellRescBufferMode : u32 // CellRescDisplayBufferMode
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

enum CellRescTableElement
{
	CELL_RESC_ELEMENT_HALF = 0,
	CELL_RESC_ELEMENT_FLOAT = 1,
};

enum CellRescResourcePolicy
{
	CELL_RESC_CONSTANT_VRAM          = 0x0,
	CELL_RESC_MINIMUM_VRAM           = 0x1,
	CELL_RESC_CONSTANT_GPU_LOAD      = 0x0,
	CELL_RESC_MINIMUM_GPU_LOAD       = 0x2,
};

enum CellRescConvolutionFilterMode // CellRescInterlaceFilterMode
{
	CELL_RESC_NORMAL_BILINEAR        = 0,
	CELL_RESC_INTERLACE_FILTER       = 1,
	CELL_RESC_3X3_GAUSSIAN           = 2,
	CELL_RESC_2X3_QUINCUNX           = 3,
	CELL_RESC_2X3_QUINCUNX_ALT       = 4,
};

typedef CellRescConvolutionFilterMode CellRescInterlaceFilterMode;

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

typedef void(CellRescHandler)(u32 head);

struct fragment_shader // Not sure what this looks like
{
	u32 unk_0 = 0;
	u32 unk_1 = 0;
	vm::ptr<void> data = vm::null;
	u32 size = 0;
	u32 offset = 0;
};

struct cell_resc_manager
{
	CellRescInitConfig config {};
	CellRescSrc srcs[SRC_BUFFER_NUM] {};
	CellRescDsts dsts[4] {};
	u32 activeDst = 0; // 4 byte Pointer to CellRescDsts
	u32 bufferMode = 0;
	u32 tableLength = 0;
	vm::ptr<fragment_shader> fragmentShaders[5] {};
	u8 pad1[16];
	vm::ptr<fragment_shader> usedFragmentShader = vm::null;
	vm::ptr<void> colorBuffers = vm::null;
	vm::ptr<void> vertexArray = vm::null;
	vm::ptr<void> fragmentShader = vm::null;
	u32 table = 0;
	u32 field_0x110 = 0; // TODO
	u32 width = 0;
	u32 height = 0;
	u32 pitch = 0;
	u16 field_0x120 = 0; // TODO
	u16 field_0x122 = 0; // TODO
	u32 bufferSize = 0;
	u32 buffersOffsets[MAX_DST_BUFFER_NUM] {};
	u32 field_0x140 = 0; // TODO
	u32 depth = 0;
	f32 horizontal = 0.0f;
	f32 vertical = 0.0f;
	atomic_t<bool> is_initialized = false;
	u8 field_0x151 = 0; // TODO
	u8 field_0x152[15] {}; // TODO
	u8 pad2[7] {};
	u64 field_0x168 = 0; // TODO
	u8 pad3[64] {};
	u64 lastFlipTime = 0;
	u64 field_0x1b8 = 0; // TODO
	u64 field_0x1c0 = 0; // TODO
	u32 field_0x1c8 = 0; // TODO
	u32 field_0x1cc = 0; // TODO
	u32 field_0x1d0 = 0; // TODO
	u32 field_0x1d4 = 0; // TODO
	u8 flipStatus = 0;
	u8 pad4[3] {};
	u32 field_0x1dc = 0; // TODO
	f32 palInterpolateDropFlexRatio = 0.0f;
	u8 pad5[4] {};
	u64 field_0x1e8 = 0; // TODO
	u64 field_0x1f0 = 0; // TODO
	u64 field_0x1f8 = 0; // TODO
};

static_assert(offsetof(cell_resc_manager, is_initialized) == 336);
static_assert(offsetof(cell_resc_manager, palInterpolateDropFlexRatio) == 480);
