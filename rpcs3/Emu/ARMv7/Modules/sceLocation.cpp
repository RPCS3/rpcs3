#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceLocation.h"

s32 sceLocationOpen(vm::ptr<u8> handle, SceLocationLocationMethod lmethod, SceLocationHeadingMethod hmethod)
{
	throw EXCEPTION("");
}

s32 sceLocationClose(u8 handle)
{
	throw EXCEPTION("");
}

s32 sceLocationReopen(u8 handle, SceLocationLocationMethod lmethod, SceLocationHeadingMethod hmethod)
{
	throw EXCEPTION("");
}

s32 sceLocationGetMethod(u8 handle, vm::ptr<s32> lmethod, vm::ptr<s32> hmethod)
{
	throw EXCEPTION("");
}

s32 sceLocationGetLocation(u8 handle, vm::ptr<SceLocationLocationInfo> linfo)
{
	throw EXCEPTION("");
}

s32 sceLocationCancelGetLocation(u8 handle)
{
	throw EXCEPTION("");
}

s32 sceLocationStartLocationCallback(u8 handle, u32 distance, vm::ptr<SceLocationLocationInfoCallback> callback, vm::ptr<void> userdata)
{
	throw EXCEPTION("");
}

s32 sceLocationStopLocationCallback(u8 handle)
{
	throw EXCEPTION("");
}

s32 sceLocationGetHeading(u8 handle, vm::ptr<SceLocationHeadingInfo> hinfo)
{
	throw EXCEPTION("");
}

s32 sceLocationStartHeadingCallback(u8 handle, u32 difference, vm::ptr<SceLocationHeadingInfoCallback> callback, vm::ptr<void> userdata)
{
	throw EXCEPTION("");
}

s32 sceLocationStopHeadingCallback(u8 handle)
{
	throw EXCEPTION("");
}

s32 sceLocationConfirm(u8 handle)
{
	throw EXCEPTION("");
}

s32 sceLocationConfirmGetStatus(u8 handle, vm::ptr<s32> status)
{
	throw EXCEPTION("");
}

s32 sceLocationConfirmGetResult(u8 handle, vm::ptr<s32> result)
{
	throw EXCEPTION("");
}

s32 sceLocationConfirmAbort(u8 handle)
{
	throw EXCEPTION("");
}

s32 sceLocationGetPermission(u8 handle, vm::ptr<SceLocationPermissionInfo> info)
{
	throw EXCEPTION("");
}

s32 sceLocationSetGpsEmulationFile(vm::ptr<char> filename)
{
	throw EXCEPTION("");
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceLocation, #name, name)

psv_log_base sceLocation("SceLibLocation", []()
{
	sceLocation.on_load = nullptr;
	sceLocation.on_unload = nullptr;
	sceLocation.on_stop = nullptr;
	sceLocation.on_error = nullptr;

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
