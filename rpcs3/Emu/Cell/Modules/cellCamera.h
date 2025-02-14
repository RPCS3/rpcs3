#pragma once

#include "Utilities/Thread.h"
#include "Emu/Io/camera_handler_base.h"
#include "Emu/Memory/vm_ptr.h"
#include "Utilities/mutex.h"

#include <map>


// Error Codes
enum CellCameraError : u32
{
	CELL_CAMERA_ERROR_ALREADY_INIT       = 0x80140801,
	CELL_CAMERA_ERROR_NOT_INIT           = 0x80140803,
	CELL_CAMERA_ERROR_PARAM              = 0x80140804,
	CELL_CAMERA_ERROR_ALREADY_OPEN       = 0x80140805,
	CELL_CAMERA_ERROR_NOT_OPEN           = 0x80140806,
	CELL_CAMERA_ERROR_DEVICE_NOT_FOUND   = 0x80140807,
	CELL_CAMERA_ERROR_DEVICE_DEACTIVATED = 0x80140808,
	CELL_CAMERA_ERROR_NOT_STARTED        = 0x80140809,
	CELL_CAMERA_ERROR_FORMAT_UNKNOWN     = 0x8014080a,
	CELL_CAMERA_ERROR_RESOLUTION_UNKNOWN = 0x8014080b,
	CELL_CAMERA_ERROR_BAD_FRAMERATE      = 0x8014080c,
	CELL_CAMERA_ERROR_TIMEOUT            = 0x8014080d,
	CELL_CAMERA_ERROR_BUSY               = 0x8014080e,
	CELL_CAMERA_ERROR_FATAL              = 0x8014080f,
	CELL_CAMERA_ERROR_MUTEX              = 0x80140810,
};

// Event masks
enum
{
	CELL_CAMERA_EFLAG_FRAME_UPDATE = 0x00000001,
	CELL_CAMERA_EFLAG_OPEN         = 0x00000002,
	CELL_CAMERA_EFLAG_CLOSE        = 0x00000004,
	CELL_CAMERA_EFLAG_START        = 0x00000008,
	CELL_CAMERA_EFLAG_STOP         = 0x00000010,
	CELL_CAMERA_EFLAG_RESET        = 0x00000020,
};

// Event types
enum
{
	CELL_CAMERA_DETACH       = 0,
	CELL_CAMERA_ATTACH       = 1,
	CELL_CAMERA_FRAME_UPDATE = 2,
	CELL_CAMERA_OPEN         = 3,
	CELL_CAMERA_CLOSE        = 4,
	CELL_CAMERA_START        = 5,
	CELL_CAMERA_STOP         = 6,
	CELL_CAMERA_RESET        = 7
};

// Read mode
enum
{
	CELL_CAMERA_READ_FUNCCALL = 0,
	CELL_CAMERA_READ_DIRECT   = 1,
};

// Colormatching
enum
{
	CELL_CAMERA_CM_CP_UNSPECIFIED = 0,
	CELL_CAMERA_CM_CP_BT709_sRGB  = 1,
	CELL_CAMERA_CM_CP_BT470_2M    = 2,
	CELL_CAMERA_CM_CP_BT470_2BG   = 3,
	CELL_CAMERA_CM_CP_SMPTE170M   = 4,
	CELL_CAMERA_CM_CP_SMPTE240M   = 5,

	CELL_CAMERA_CM_TC_UNSPECIFIED = 0,
	CELL_CAMERA_CM_TC_BT709       = 1,
	CELL_CAMERA_CM_TC_BT470_2M    = 2,
	CELL_CAMERA_CM_TC_BT470_2BG   = 3,
	CELL_CAMERA_CM_TC_SMPTE170M   = 4,
	CELL_CAMERA_CM_TC_SMPTE240M   = 5,
	CELL_CAMERA_CM_TC_LINEAR      = 6,
	CELL_CAMERA_CM_TC_sRGB        = 7,

	CELL_CAMERA_CM_MC_UNSPECIFIED = 0,
	CELL_CAMERA_CM_MC_BT709       = 1,
	CELL_CAMERA_CM_MC_FCC         = 2,
	CELL_CAMERA_CM_MC_BT470_2BG   = 3,
	CELL_CAMERA_CM_MC_SMPTE170M   = 4,
	CELL_CAMERA_CM_MC_SMPTE240M   = 5,
};

// Power Line Frequency
enum
{
	CELL_CAMERA_PLFREQ_DISABLED = 0,
	CELL_CAMERA_PLFREQ_50Hz     = 1,
	CELL_CAMERA_PLFREQ_60Hz     = 2,
};

// DEVICECAP
enum
{
	CELL_CAMERA_CTC_SCANNING_MODE                  = (1 << 0),
	CELL_CAMERA_CTC_AUTO_EXPOSURE_MODE             = (1 << 1),
	CELL_CAMERA_CTC_AUTO_EXPOSURE_PRIORITY         = (1 << 2),
	CELL_CAMERA_CTC_EXPOSURE_TIME_ABS              = (1 << 3),
	CELL_CAMERA_CTC_EXPOSURE_TIME_REL              = (1 << 4),
	CELL_CAMERA_CTC_FOCUS_ABS                      = (1 << 5),
	CELL_CAMERA_CTC_FOCUS_REL                      = (1 << 6),
	CELL_CAMERA_CTC_IRIS_ABS                       = (1 << 7),
	CELL_CAMERA_CTC_IRIS_REL                       = (1 << 8),
	CELL_CAMERA_CTC_ZOOM_ABS                       = (1 << 9),
	CELL_CAMERA_CTC_ZOOM_REL                       = (1 << 10),
	CELL_CAMERA_CTC_PANTILT_ABS                    = (1 << 11),
	CELL_CAMERA_CTC_PANTILT_REL                    = (1 << 12),
	CELL_CAMERA_CTC_ROLL_ABS                       = (1 << 13),
	CELL_CAMERA_CTC_ROLL_REL                       = (1 << 14),
	CELL_CAMERA_CTC_RESERVED_15                    = (1 << 15),
	CELL_CAMERA_CTC_RESERVED_16                    = (1 << 16),
	CELL_CAMERA_CTC_FOCUS_AUTO                     = (1 << 17),
	CELL_CAMERA_CTC_PRIVACY                        = (1 << 18),

	CELL_CAMERA_PUC_BRIGHTNESS                     = (1 << 0),
	CELL_CAMERA_PUC_CONTRAST                       = (1 << 1),
	CELL_CAMERA_PUC_HUE                            = (1 << 2),
	CELL_CAMERA_PUC_SATURATION                     = (1 << 3),
	CELL_CAMERA_PUC_SHARPNESS                      = (1 << 4),
	CELL_CAMERA_PUC_GAMMA                          = (1 << 5),
	CELL_CAMERA_PUC_WHITE_BALANCE_TEMPERATURE      = (1 << 6),
	CELL_CAMERA_PUC_WHITE_BALANCE_COMPONENT        = (1 << 7),
	CELL_CAMERA_PUC_BACKLIGHT_COMPENSATION         = (1 << 8),
	CELL_CAMERA_PUC_GAIN                           = (1 << 9),
	CELL_CAMERA_PUC_POWER_LINE_FREQUENCY           = (1 << 10),
	CELL_CAMERA_PUC_HUE_AUTO                       = (1 << 11),
	CELL_CAMERA_PUC_WHITE_BALANCE_TEMPERATURE_AUTO = (1 << 12),
	CELL_CAMERA_PUC_WHITE_BALANCE_COMPONENT_AUTO   = (1 << 13),
	CELL_CAMERA_PUC_DIGITAL_MULTIPLIER             = (1 << 14),
	CELL_CAMERA_PUC_DIGITAL_MULTIPLIER_LIMIT       = (1 << 15),
	CELL_CAMERA_PUC_ANALOG_VIDEO_STANDARD          = (1 << 16),
	CELL_CAMERA_PUC_ANALOG_VIDEO_LOCK_STATUS       = (1 << 17),
};

// UVCREQCODE Control Selector
enum
{
	CELL_CAMERA_CS_SHIFT  = 0,
	CELL_CAMERA_CS_BITS   = 0x000000ff,
	CELL_CAMERA_CAP_SHIFT = 8,
	CELL_CAMERA_CAP_BITS  = 0x0000ff00,
	CELL_CAMERA_NUM_SHIFT = 16,
	CELL_CAMERA_NUM_BITS  = 0x000f0000,
	CELL_CAMERA_NUM_1     = 0x00010000,
	CELL_CAMERA_NUM_2     = 0x00020000,
	CELL_CAMERA_NUM_3     = 0x00030000,
	CELL_CAMERA_NUM_4     = 0x00040000,
	CELL_CAMERA_LEN_SHIFT = 20,
	CELL_CAMERA_LEN_BITS  = 0x00f00000,
	CELL_CAMERA_LEN_1     = 0x00100000,
	CELL_CAMERA_LEN_2     = 0x00200000,
	CELL_CAMERA_LEN_4     = 0x00400000,
	CELL_CAMERA_ID_SHIFT  = 24,
	CELL_CAMERA_ID_BITS   = 0x0f000000,
	CELL_CAMERA_ID_CT     = 0x01000000,
	CELL_CAMERA_ID_SU     = 0x02000000,
	CELL_CAMERA_ID_PU     = 0x04000000,
};

// UVCREQCODE Camera Terminal Control
enum
{
	CELL_CAMERA_UVC_SCANNING_MODE          = 0x01110001,
	CELL_CAMERA_UVC_AUTO_EXPOSURE_MODE     = 0x01110102,
	CELL_CAMERA_UVC_AUTO_EXPOSURE_PRIORITY = 0x01110203,
	CELL_CAMERA_UVC_EXPOSURE_TIME_ABS      = 0x01410304,
	CELL_CAMERA_UVC_EXPOSURE_TIME_REL      = 0x01110405,
	CELL_CAMERA_UVC_FOCUS_ABS              = 0x01210506,
	CELL_CAMERA_UVC_FOCUS_REL              = 0x01120607,
	CELL_CAMERA_UVC_FOCUS_AUTO             = 0x01111108,
	CELL_CAMERA_UVC_IRIS_ABS               = 0x01210709,
	CELL_CAMERA_UVC_IRIS_REL               = 0x0111080a,
	CELL_CAMERA_UVC_ZOOM_ABS               = 0x0121090b,
	CELL_CAMERA_UVC_ZOOM_REL               = 0x01130a0c,
	CELL_CAMERA_UVC_PANTILT_ABS            = 0x01420b0d,
	CELL_CAMERA_UVC_PANTILT_REL            = 0x01140c0e,
	CELL_CAMERA_UVC_ROLL_ABS               = 0x01210d0f,
	CELL_CAMERA_UVC_ROLL_REL               = 0x01120e10,
	CELL_CAMERA_UVC_PRIVACY                = 0x01111211,
};

// UVCREQCODE Selector Unit Control/Processing Unit Control
enum
{
	CELL_CAMERA_UVC_INPUT_SELECT                   = 0x02110101,

	CELL_CAMERA_UVC_BACKLIGHT_COMPENSATION         = 0x04210801,
	CELL_CAMERA_UVC_BRIGHTNESS                     = 0x04210002,
	CELL_CAMERA_UVC_CONTRAST                       = 0x04210103,
	CELL_CAMERA_UVC_GAIN                           = 0x04210904,
	CELL_CAMERA_UVC_POWER_LINE_FREQUENCY           = 0x04110a05,
	CELL_CAMERA_UVC_HUE                            = 0x04210206,
	CELL_CAMERA_UVC_HUE_AUTO                       = 0x04110b10,
	CELL_CAMERA_UVC_SATURATION                     = 0x04210307,
	CELL_CAMERA_UVC_SHARPNESS                      = 0x04210408,
	CELL_CAMERA_UVC_GAMMA                          = 0x04210509,
	CELL_CAMERA_UVC_WHITE_BALANCE_TEMPERATURE      = 0x0421060a,
	CELL_CAMERA_UVC_WHITE_BALANCE_TEMPERATURE_AUTO = 0x04110c0b,
	CELL_CAMERA_UVC_WHITE_BALANCE_COMPONENT        = 0x0422070c,
	CELL_CAMERA_UVC_WHITE_BALANCE_COMPONENT_AUTO   = 0x04110d0d,
	CELL_CAMERA_UVC_DIGITAL_MULTIPLIER             = 0x04210e0e,
	CELL_CAMERA_UVC_DIGITAL_MULTIPLIER_LIMIT       = 0x04210f0f,
	CELL_CAMERA_UVC_ANALOG_VIDEO_STANDARD          = 0x04111011,
	CELL_CAMERA_UVC_ANALOG_VIDEO_LOCK_STATUS       = 0x04111112,
};

// UVCREQCODE Request code bits
enum
{
	CELL_CAMERA_RC_CUR  = 0x81,
	CELL_CAMERA_RC_MIN  = 0x82,
	CELL_CAMERA_RC_MAX  = 0x83,
	CELL_CAMERA_RC_RES  = 0x84,
	CELL_CAMERA_RC_LEN  = 0x85,
	CELL_CAMERA_RC_INFO = 0x86,
	CELL_CAMERA_RC_DEF  = 0x87,
};

// Camera types
enum CellCameraType : s32
{
	CELL_CAMERA_TYPE_UNKNOWN,
	CELL_CAMERA_EYETOY,
	CELL_CAMERA_EYETOY2,
	CELL_CAMERA_USBVIDEOCLASS,
};

// Image format
enum CellCameraFormat : s32
{
	CELL_CAMERA_FORMAT_UNKNOWN,
	CELL_CAMERA_JPG,
	CELL_CAMERA_RAW8,
	CELL_CAMERA_YUV422,
	CELL_CAMERA_RAW10,
	CELL_CAMERA_RGBA,
	CELL_CAMERA_YUV420,
	CELL_CAMERA_V_Y1_U_Y0,
	CELL_CAMERA_Y0_U_Y1_V = CELL_CAMERA_YUV422,
};

// Image resolution
enum CellCameraResolution : s32
{
	CELL_CAMERA_RESOLUTION_UNKNOWN,
	CELL_CAMERA_VGA,
	CELL_CAMERA_QVGA,
	CELL_CAMERA_WGA,
	CELL_CAMERA_SPECIFIED_WIDTH_HEIGHT,
};

// Camera attributes
enum CellCameraAttribute : s32
{
	CELL_CAMERA_GAIN,
	CELL_CAMERA_REDBLUEGAIN,
	CELL_CAMERA_SATURATION,
	CELL_CAMERA_EXPOSURE,
	CELL_CAMERA_BRIGHTNESS,
	CELL_CAMERA_AEC,
	CELL_CAMERA_AGC,
	CELL_CAMERA_AWB,
	CELL_CAMERA_ABC,
	CELL_CAMERA_LED,
	CELL_CAMERA_AUDIOGAIN,
	CELL_CAMERA_QS,
	CELL_CAMERA_NONZEROCOEFFS,
	CELL_CAMERA_YUVFLAG,
	CELL_CAMERA_JPEGFLAG,
	CELL_CAMERA_BACKLIGHTCOMP,
	CELL_CAMERA_MIRRORFLAG,
	CELL_CAMERA_MEASUREDQS,
	CELL_CAMERA_422FLAG,
	CELL_CAMERA_USBLOAD,
	CELL_CAMERA_GAMMA,
	CELL_CAMERA_GREENGAIN,
	CELL_CAMERA_AGCLIMIT,
	CELL_CAMERA_DENOISE,
	CELL_CAMERA_FRAMERATEADJUST,
	CELL_CAMERA_PIXELOUTLIERFILTER,
	CELL_CAMERA_AGCLOW,
	CELL_CAMERA_AGCHIGH,
	CELL_CAMERA_DEVICELOCATION,

	CELL_CAMERA_FORMATCAP         = 100,
	CELL_CAMERA_FORMATINDEX,
	CELL_CAMERA_NUMFRAME,
	CELL_CAMERA_FRAMEINDEX,
	CELL_CAMERA_FRAMESIZE,
	CELL_CAMERA_INTERVALTYPE,
	CELL_CAMERA_INTERVALINDEX,
	CELL_CAMERA_INTERVALVALUE,
	CELL_CAMERA_COLORMATCHING,
	CELL_CAMERA_PLFREQ,
	CELL_CAMERA_DEVICEID,
	CELL_CAMERA_DEVICECAP,
	CELL_CAMERA_DEVICESPEED,
	CELL_CAMERA_UVCREQCODE,
	CELL_CAMERA_UVCREQDATA,
	CELL_CAMERA_DEVICEID2,

	CELL_CAMERA_READMODE          = 300,
	CELL_CAMERA_GAMEPID,
	CELL_CAMERA_PBUFFER,
	CELL_CAMERA_READFINISH,

	CELL_CAMERA_ATTRIBUTE_UNKNOWN = 500,
};

// Request codes
enum
{
	SET_CUR  = 0x01,
	GET_CUR  = 0x81,
	GET_MIN  = 0x82,
	GET_MAX  = 0x83,
	GET_RES  = 0x84,
	GET_LEN  = 0x85,
	GET_INFO = 0x86,
	GET_DEF  = 0x87,
};

enum // version
{
	CELL_CAMERA_INFO_VER_100 = 0x0100,
	CELL_CAMERA_INFO_VER_101 = 0x0101,
	CELL_CAMERA_INFO_VER_200 = 0x0200,
	CELL_CAMERA_INFO_VER     = CELL_CAMERA_INFO_VER_200,

	CELL_CAMERA_READ_VER_100 = 0x0100,
	CELL_CAMERA_READ_VER     = CELL_CAMERA_READ_VER_100,
};

// Other
enum
{
	CELL_CAMERA_MAX_CAMERAS = 1
};

struct CellCameraInfo
{
	// filled in by application as inputs for open
	be_t<s32> format;
	be_t<s32> resolution;
	be_t<s32> framerate;

	// filled in by open
	vm::bptr<u8> buffer;
	be_t<s32> bytesize;
	be_t<s32> width;
	be_t<s32> height;
	be_t<s32> dev_num;
	be_t<s32> guid;
};

struct CellCameraInfoEx
{
	be_t<CellCameraFormat> format; // CellCameraFormat
	be_t<s32> resolution; // CellCameraResolution
	be_t<s32> framerate;

	vm::bptr<u8> buffer;
	be_t<s32> bytesize;
	be_t<s32> width;    // only used if resolution == CELL_CAMERA_SPECIFIED_WIDTH_HEIGHT
	be_t<s32> height;   // likewise
	be_t<s32> dev_num;
	be_t<s32> guid;

	be_t<s32> info_ver;
	be_t<u32> container;
	be_t<s32> read_mode;
	vm::bptr<u8> pbuf[2];

	ENABLE_BITWISE_SERIALIZATION;
};

struct CellCameraReadEx
{
	be_t<s32> version;
	be_t<u32> frame;
	be_t<u32> bytesread;
	be_t<u64> timestamp; // system_time_t (microseconds)
	vm::bptr<u8> pbuf;
};

class camera_context
{
	struct notify_event_data
	{
		u64 source;
		u64 flag;

		ENABLE_BITWISE_SERIALIZATION;
	};

public:
	void operator()();
	void reset_state();
	void send_attach_state(bool attached);
	void set_attr(s32 attrib, u32 arg1, u32 arg2);

	/**
	 * \brief Sets up notify event queue supplied and immediately sends an ATTACH event to it
	 * \param key Event queue key to add
	 * \param source Event source port
	 * \param flag Event flag (CELL_CAMERA_EFLAG_*)
	 */
	void add_queue(u64 key, u64 source, u64 flag);

	/**
	 * \brief Unsets/removes event queue specified
	 * \param key Event queue key to remove
	 */
	void remove_queue(u64 key);

	std::map<u64, notify_event_data> notify_data_map;

	shared_mutex mutex;
	shared_mutex mutex_notify_data_map;
	u64 start_timestamp_us = 0;

	atomic_t<u8> read_mode{CELL_CAMERA_READ_FUNCCALL};
	atomic_t<bool> is_streaming{false};
	atomic_t<bool> is_attached{false};
	atomic_t<bool> is_attached_dirty{false};
	atomic_t<bool> is_open{false};

	CellCameraInfoEx info{};
	atomic_t<u32> pbuf_write_index = 0;
	std::array<atomic_t<bool>, 2> pbuf_locked = { false, false };
	u32 pbuf_next_index() const;

	struct attr_t
	{
		u32 v1, v2;

		ENABLE_BITWISE_SERIALIZATION;
	};

	attr_t attr[500]{};
	atomic_t<bool> has_new_frame = false;
	atomic_t<u32> frame_num = 0;
	atomic_t<u64> frame_timestamp_us = 0;
	atomic_t<u32> bytes_read = 0;

	atomic_t<u8> init = 0;

	SAVESTATE_INIT_POS(16);

	camera_context() = default;
	camera_context(utils::serial& ar);
	void save(utils::serial& ar);

	static constexpr auto thread_name = "Camera Thread"sv;

	std::shared_ptr<camera_handler_base> handler;
	bool open_camera();
	bool start_camera();
	bool get_camera_frame(u8* dst, u32& width, u32& height, u64& frame_number, u64& bytes_read);
	void stop_camera();
	void close_camera();
	bool on_handler_state(camera_handler_base::camera_handler_state state);
};

using camera_thread = named_thread<camera_context>;

/// Shared data between cellGem and cellCamera
struct gem_camera_shared
{
	gem_camera_shared() {}
	gem_camera_shared(utils::serial& ar);

	void save(utils::serial& ar);

	SAVESTATE_INIT_POS(7);

	atomic_t<u64> frame_timestamp_us{}; // latest read timestamp from cellCamera (cellCameraRead(Ex))
	atomic_t<s32> width{640};
	atomic_t<s32> height{480};
	atomic_t<s32> size{0};
	atomic_t<CellCameraFormat> format{CELL_CAMERA_RAW8};
};
