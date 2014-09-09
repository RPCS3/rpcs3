#pragma once

// Error Codes
enum
{
	CELL_CAMERA_ERROR_ALREADY_INIT = 0x80140801,
	CELL_CAMERA_ERROR_NOT_INIT = 0x80140803,
	CELL_CAMERA_ERROR_PARAM = 0x80140804,
	CELL_CAMERA_ERROR_ALREADY_OPEN = 0x80140805,
	CELL_CAMERA_ERROR_NOT_OPEN = 0x80140806,
	CELL_CAMERA_ERROR_DEVICE_NOT_FOUND = 0x80140807,
	CELL_CAMERA_ERROR_DEVICE_DEACTIVATED = 0x80140808,
	CELL_CAMERA_ERROR_NOT_STARTED = 0x80140809,
	CELL_CAMERA_ERROR_FORMAT_UNKNOWN = 0x8014080a,
	CELL_CAMERA_ERROR_RESOLUTION_UNKNOWN = 0x8014080b,
	CELL_CAMERA_ERROR_BAD_FRAMERATE = 0x8014080c,
	CELL_CAMERA_ERROR_TIMEOUT = 0x8014080d,
	CELL_CAMERA_ERROR_FATAL = 0x8014080f,
};

// Camera types
enum CellCameraType
{
	CELL_CAMERA_TYPE_UNKNOWN,
	CELL_CAMERA_EYETOY,
	CELL_CAMERA_EYETOY2,
	CELL_CAMERA_USBVIDEOCLASS,
};

// Image format
enum CellCameraFormat
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

// Image resolutiom
enum CellCameraResolution
{
	CELL_CAMERA_RESOLUTION_UNKNOWN,
	CELL_CAMERA_VGA,
	CELL_CAMERA_QVGA,
	CELL_CAMERA_WGA,
	CELL_CAMERA_SPECIFIED_WIDTH_HEIGHT,
};

struct CellCameraInfoEx
{
	CellCameraFormat format;
	CellCameraResolution resolution;
	be_t<s32> framerate;
	be_t<u32> buffer;
	be_t<s32> bytesize;
	be_t<s32> width;
	be_t<s32> height;
	be_t<s32> dev_num;
	be_t<s32> guid;
	be_t<s32> info_ver;
	be_t<u32> container;
	be_t<s32> read_mode;
	be_t<u32> pbuf[2];
};

struct CellCameraReadEx
{
	be_t<s32> version;
	be_t<u32> frame;
	be_t<u32> bytesread;
	//system_time_t timestamp; // TODO: Replace this with something
	be_t<u32> pbuf;
};