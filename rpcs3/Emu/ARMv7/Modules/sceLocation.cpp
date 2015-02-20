#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceLocation;

typedef u8 SceLocationHandle;

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
	double latitude;
	double longitude;
	double altitude;
	float accuracy;
	float reserve;
	float direction;
	float speed;
	u64 timestamp;
};

struct SceLocationHeadingInfo
{
	float trueHeading;
	float headingVectorX;
	float headingVectorY;
	float headingVectorZ;
	float reserve;
	float reserve2;
	u64 timestamp;
};

typedef vm::psv::ptr<void(s32 result, SceLocationHandle handle, vm::psv::ptr<const SceLocationLocationInfo> location, vm::psv::ptr<void> userdata)> SceLocationLocationInfoCallback;
typedef vm::psv::ptr<void(s32 result, SceLocationHandle handle, vm::psv::ptr<const SceLocationHeadingInfo> heading, vm::psv::ptr<void> userdata)> SceLocationHeadingInfoCallback;

struct SceLocationPermissionInfo
{
	SceLocationPermissionStatus parentalstatus;
	SceLocationPermissionStatus mainstatus;
	SceLocationPermissionApplicationStatus applicationstatus;
};

s32 sceLocationOpen(vm::psv::ptr<SceLocationHandle> handle, SceLocationLocationMethod lmethod, SceLocationHeadingMethod hmethod)
{
	throw __FUNCTION__;
}

s32 sceLocationClose(SceLocationHandle handle)
{
	throw __FUNCTION__;
}

s32 sceLocationReopen(SceLocationHandle handle, SceLocationLocationMethod lmethod, SceLocationHeadingMethod hmethod)
{
	throw __FUNCTION__;
}

s32 sceLocationGetMethod(SceLocationHandle handle, vm::psv::ptr<SceLocationLocationMethod> lmethod, vm::psv::ptr<SceLocationHeadingMethod> hmethod)
{
	throw __FUNCTION__;
}

s32 sceLocationGetLocation(SceLocationHandle handle, vm::psv::ptr<SceLocationLocationInfo> linfo)
{
	throw __FUNCTION__;
}

s32 sceLocationCancelGetLocation(SceLocationHandle handle)
{
	throw __FUNCTION__;
}

s32 sceLocationStartLocationCallback(SceLocationHandle handle, u32 distance, SceLocationLocationInfoCallback callback, vm::psv::ptr<void> userdata)
{
	throw __FUNCTION__;
}

s32 sceLocationStopLocationCallback(SceLocationHandle handle)
{
	throw __FUNCTION__;
}

s32 sceLocationGetHeading(SceLocationHandle handle, vm::psv::ptr<SceLocationHeadingInfo> hinfo)
{
	throw __FUNCTION__;
}

s32 sceLocationStartHeadingCallback(SceLocationHandle handle, u32 difference, SceLocationHeadingInfoCallback callback, vm::psv::ptr<void> userdata)
{
	throw __FUNCTION__;
}

s32 sceLocationStopHeadingCallback(SceLocationHandle handle)
{
	throw __FUNCTION__;
}

s32 sceLocationConfirm(SceLocationHandle handle)
{
	throw __FUNCTION__;
}

s32 sceLocationConfirmGetStatus(SceLocationHandle handle, vm::psv::ptr<SceLocationDialogStatus> status)
{
	throw __FUNCTION__;
}

s32 sceLocationConfirmGetResult(SceLocationHandle handle, vm::psv::ptr<SceLocationDialogResult> result)
{
	throw __FUNCTION__;
}

s32 sceLocationConfirmAbort(SceLocationHandle handle)
{
	throw __FUNCTION__;
}

s32 sceLocationGetPermission(SceLocationHandle handle, vm::psv::ptr<SceLocationPermissionInfo> info)
{
	throw __FUNCTION__;
}

s32 sceLocationSetGpsEmulationFile(vm::psv::ptr<char> filename)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func<(func_ptr)name>(nid, &sceLocation, #name, name)

psv_log_base sceLocation("SceLibLocation", []()
{
	sceLocation.on_load = nullptr;
	sceLocation.on_unload = nullptr;
	sceLocation.on_stop = nullptr;

	REG_FUNC(0xDD271661, sceLocationOpen);
	REG_FUNC(0x14FE76E8, sceLocationClose);
	REG_FUNC(0xB1F55065, sceLocationReopen);
	REG_FUNC(0x188CE004, sceLocationGetMethod);
	REG_FUNC(0x15BC27C8, sceLocationGetLocation);
	REG_FUNC(0x71503251, sceLocationCancelGetLocation);
	REG_FUNC(0x12D1F0EA, sceLocationStartLocationCallback);
	REG_FUNC(0xED378700, sceLocationStopLocationCallback);
	REG_FUNC(0x4E9E5ED9, sceLocationGetHeading);
	REG_FUNC(0x07D4DFE0, sceLocationStartHeadingCallback);
	REG_FUNC(0x92E53F94, sceLocationStopHeadingCallback);
	//REG_FUNC(0xE055BCF5, sceLocationSetHeapAllocator);
	REG_FUNC(0xC895E567, sceLocationConfirm);
	REG_FUNC(0x730FF842, sceLocationConfirmGetStatus);
	REG_FUNC(0xFF016C13, sceLocationConfirmGetResult);
	REG_FUNC(0xE3CBF875, sceLocationConfirmAbort);
	REG_FUNC(0x482622C6, sceLocationGetPermission);
	REG_FUNC(0xDE0A9EA4, sceLocationSetGpsEmulationFile);
	//REG_FUNC(0x760D08FF, sceLocationConfirmSetMessage);
});
