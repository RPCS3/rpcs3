#pragma once

namespace vm { using namespace ps3; }

static const float CELL_GEM_SPHERE_RADIUS_MM = 22.5f;

// Error Codes
enum
{
	CELL_GEM_ERROR_RESOURCE_ALLOCATION_FAILED = 0x80121801,
	CELL_GEM_ERROR_ALREADY_INITIALIZED        = 0x80121802,
	CELL_GEM_ERROR_UNINITIALIZED              = 0x80121803,
	CELL_GEM_ERROR_INVALID_PARAMETER          = 0x80121804,
	CELL_GEM_ERROR_INVALID_ALIGNMENT          = 0x80121805,
	CELL_GEM_ERROR_UPDATE_NOT_FINISHED        = 0x80121806,
	CELL_GEM_ERROR_UPDATE_NOT_STARTED         = 0x80121807,
	CELL_GEM_ERROR_CONVERT_NOT_FINISHED       = 0x80121808,
	CELL_GEM_ERROR_CONVERT_NOT_STARTED        = 0x80121809,
	CELL_GEM_ERROR_WRITE_NOT_FINISHED         = 0x8012180A,
	CELL_GEM_ERROR_NOT_A_HUE                  = 0x8012180B,
};

// Runtime statuses
enum
{
	CELL_GEM_NOT_CONNECTED              = 1,
	CELL_GEM_SPHERE_NOT_CALIBRATED      = 2,
	CELL_GEM_SPHERE_CALIBRATING         = 3,
	CELL_GEM_COMPUTING_AVAILABLE_COLORS = 4,
	CELL_GEM_HUE_NOT_SET                = 5,
	CELL_GEM_NO_VIDEO                   = 6,
	CELL_GEM_TIME_OUT_OF_RANGE          = 7,
	CELL_GEM_NOT_CALIBRATED             = 8,
	CELL_GEM_NO_EXTERNAL_PORT_DEVICE    = 9,
};

// General constents
enum
{
	CELL_GEM_CTRL_CIRCLE                                 = 1 << 5,
	CELL_GEM_CTRL_CROSS                                  = 1 << 6,
	CELL_GEM_CTRL_MOVE                                   = 1 << 2,
	CELL_GEM_CTRL_SELECT                                 = 1 << 0,
	CELL_GEM_CTRL_SQUARE                                 = 1 << 7,
	CELL_GEM_CTRL_START                                  = 1 << 3,
	CELL_GEM_CTRL_T                                      = 1 << 1,
	CELL_GEM_CTRL_TRIANGLE                               = 1 << 4,
	CELL_GEM_DONT_CARE_HUE                               = 4 << 24,
	CELL_GEM_DONT_CHANGE_HUE                             = 8 << 24,
	CELL_GEM_DONT_TRACK_HUE                              = 2 << 24,
	CELL_GEM_EXT_CONNECTED                               = 1 << 0,
	CELL_GEM_EXT_EXT0                                    = 1 << 1,
	CELL_GEM_EXT_EXT1                                    = 1 << 2,
	CELL_GEM_EXTERNAL_PORT_DEVICE_INFO_SIZE              = 38,
	CELL_GEM_EXTERNAL_PORT_OUTPUT_SIZE                   = 40,
	CELL_GEM_FLAG_CALIBRATION_FAILED_BRIGHT_LIGHTING     = 1 << 4,
	CELL_GEM_FLAG_CALIBRATION_FAILED_CANT_FIND_SPHERE    = 1 << 2,
	CELL_GEM_FLAG_CALIBRATION_FAILED_MOTION_DETECTED     = 1 << 3,
	CELL_GEM_FLAG_CALIBRATION_OCCURRED                   = 1 << 0,
	CELL_GEM_FLAG_CALIBRATION_SUCCEEDED                  = 1 << 1,
	CELL_GEM_FLAG_CALIBRATION_WARNING_BRIGHT_LIGHTING    = 1 << 6,
	ELL_GEM_FLAG_CALIBRATION_WARNING_MOTION_DETECTED     = 1 << 5,
	CELL_GEM_FLAG_CAMERA_PITCH_ANGLE_CHANGED             = 1 << 9,
	CELL_GEM_FLAG_CURRENT_HUE_CONFLICTS_WITH_ENVIRONMENT = 1 << 13,
	CELL_GEM_FLAG_LIGHTING_CHANGED                       = 1 << 7,
	CELL_GEM_FLAG_VARIABLE_MAGNETIC_FIELD                = 1 << 10,
	CELL_GEM_FLAG_VERY_COLORFUL_ENVIRONMENT              = 1 << 12,
	CELL_GEM_FLAG_WEAK_MAGNETIC_FIELD                    = 1 << 11,
	CELL_GEM_FLAG_WRONG_FIELD_OF_VIEW_SETTING            = 1 << 8,
	CELL_GEM_INERTIAL_STATE_FLAG_LATEST                  = 0,
	CELL_GEM_INERTIAL_STATE_FLAG_NEXT                    = 2,
	CELL_GEM_INERTIAL_STATE_FLAG_PREVIOUS                = 1,
	CELL_GEM_LATENCY_OFFSET                              = -22000,
	CELL_GEM_MAX_CAMERA_EXPOSURE                         = 511,
	CELL_GEM_MAX_NUM                                     = 4,
	CELL_GEM_MIN_CAMERA_EXPOSURE                         = 40,
	CELL_GEM_STATE_FLAG_CURRENT_TIME                     = 0,
	CELL_GEM_STATE_FLAG_LATEST_IMAGE_TIME                = 1,
	CELL_GEM_STATE_FLAG_TIMESTAMP                        = 2,
	CELL_GEM_STATUS_DISCONNECTED                         = 0,
	CELL_GEM_STATUS_READY                                = 1,
	CELL_GEM_TRACKING_FLAG_POSITION_TRACKED              = 1 << 0,
	CELL_GEM_TRACKING_FLAG_VISIBLE                       = 1 << 1,
	CELL_GEM_VERSION                                     = 2,
};

struct CellGemAttribute
{
	be_t<u32> version;
	be_t<u32> max_connect;
	be_t<u32> memory_ptr;
	be_t<u32> spurs_addr;
	u8 spu_priorities[8];
};

struct CellGemCameraState
{
	be_t<s32> exposure;
	be_t<f32> exposure_time;
	be_t<f32> gain;
	be_t<f32> pitch_angle;
	be_t<f32> pitch_angle_estimate;
};

struct CellGemExtPortData
{
	be_t<u16> status;
	be_t<u16> digital1;
	be_t<u16> digital2;
	be_t<u16> analog_right_x;
	be_t<u16> analog_right_y;
	be_t<u16> analog_left_x;
	be_t<u16> analog_left_y;
	u8 custom[5];
};

struct CellGemImageState
{
	be_t<u64> frame_timestamp;
	be_t<u64> timestamp;
	be_t<f32> u;
	be_t<f32> v;
	be_t<f32> r;
	be_t<f32> projectionx;
	be_t<f32> projectiony;
	be_t<f32> distance;
	u8 visible;
	u8 r_valid;
};

struct CellGemPadData
{
	be_t<u16> digitalbuttons;
	be_t<u16> analog_T;
};

struct CellGemInertialState
{
	be_t<f32> accelerometer[4];
	be_t<f32> gyro[4];
	be_t<f32> accelerometer_bias[4];
	be_t<f32> gyro_bias[4];
	CellGemPadData pad;
	CellGemExtPortData ext;
	be_t<u64> timestamp;
	be_t<s32> counter;
	be_t<f32> temperature;
};

struct CellGemInfo
{
	be_t<u32> max_connect;
	be_t<u32> now_connect;
	be_t<u32> status[CELL_GEM_MAX_NUM];
	be_t<u32> port[CELL_GEM_MAX_NUM];
};

struct CellGemState
{
	be_t<f32> pos[4];
	be_t<f32> vel[4];
	be_t<f32> accel[4];
	be_t<f32> quat[4];
	be_t<f32> angvel[4];
	be_t<f32> angaccel[4];
	be_t<f32> handle_pos[4];
	be_t<f32> handle_vel[4];
	be_t<f32> handle_accel[4];
	CellGemPadData pad;
	CellGemExtPortData ext;
	be_t<u64> timestamp;
	be_t<f32> temperature;
	be_t<f32> camera_pitch_angle;
	be_t<u32> tracking_flags;
};

struct CellGemVideoConvertAttribute
{
	be_t<s32> version;
	be_t<s32> output_format;
	be_t<s32> conversion_flags;
	be_t<f32> gain;
	be_t<f32> red_gain;
	be_t<f32> green_gain;
	be_t<f32> blue_gain;
	be_t<u32> buffer_memory;
	be_t<u32> video_data_out;
	u8 alpha;
};
