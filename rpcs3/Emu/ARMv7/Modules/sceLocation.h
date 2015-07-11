#pragma once

enum SceLocationLocationMethod : s32
{
	SCE_LOCATION_LMETHOD_NONE = 0,
	SCE_LOCATION_LMETHOD_AGPS_AND_3G_AND_WIFI = 1,
	SCE_LOCATION_LMETHOD_GPS_AND_WIFI = 2,
	SCE_LOCATION_LMETHOD_WIFI = 3,
	SCE_LOCATION_LMETHOD_3G = 4,
	SCE_LOCATION_LMETHOD_GPS = 5
};

enum SceLocationHeadingMethod : s32
{
	SCE_LOCATION_HMETHOD_NONE = 0,
	SCE_LOCATION_HMETHOD_AUTO = 1,
	SCE_LOCATION_HMETHOD_VERTICAL = 2,
	SCE_LOCATION_HMETHOD_HORIZONTAL = 3,
	SCE_LOCATION_HMETHOD_CAMERA = 4
};

enum SceLocationDialogStatus : s32
{
	SCE_LOCATION_DIALOG_STATUS_IDLE = 0,
	SCE_LOCATION_DIALOG_STATUS_RUNNING = 1,
	SCE_LOCATION_DIALOG_STATUS_FINISHED = 2
};

enum SceLocationDialogResult : s32
{
	SCE_LOCATION_DIALOG_RESULT_NONE = 0,
	SCE_LOCATION_DIALOG_RESULT_DISABLE = 1,
	SCE_LOCATION_DIALOG_RESULT_ENABLE = 2
};

enum SceLocationPermissionApplicationStatus : s32
{
	SCE_LOCATION_PERMISSION_APPLICATION_NONE = 0,
	SCE_LOCATION_PERMISSION_APPLICATION_INIT = 1,
	SCE_LOCATION_PERMISSION_APPLICATION_DENY = 2,
	SCE_LOCATION_PERMISSION_APPLICATION_ALLOW = 3
};

enum SceLocationPermissionStatus : s32
{
	SCE_LOCATION_PERMISSION_DENY = 0,
	SCE_LOCATION_PERMISSION_ALLOW = 1
};

struct SceLocationLocationInfo
{
	le_t<double> latitude;
	le_t<double> longitude;
	le_t<double> altitude;
	le_t<float> accuracy;
	le_t<float> reserve;
	le_t<float> direction;
	le_t<float> speed;
	le_t<u64> timestamp;
};

struct SceLocationHeadingInfo
{
	le_t<float> trueHeading;
	le_t<float> headingVectorX;
	le_t<float> headingVectorY;
	le_t<float> headingVectorZ;
	le_t<float> reserve;
	le_t<float> reserve2;
	le_t<u64> timestamp;
};

using SceLocationLocationInfoCallback = func_def<void(s32 result, u8 handle, vm::cptr<SceLocationLocationInfo> location, vm::ptr<void> userdata)>;
using SceLocationHeadingInfoCallback = func_def<void(s32 result, u8 handle, vm::cptr<SceLocationHeadingInfo> heading, vm::ptr<void> userdata)>;

struct SceLocationPermissionInfo
{
	SceLocationPermissionStatus parentalstatus;
	SceLocationPermissionStatus mainstatus;
	SceLocationPermissionApplicationStatus applicationstatus;
};

extern psv_log_base sceLocation;
