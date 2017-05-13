#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceLocation.h"

logs::channel sceLocation("sceLocation");

s32 sceLocationOpen(vm::ptr<u8> handle, SceLocationLocationMethod lmethod, SceLocationHeadingMethod hmethod)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationClose(u8 handle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationReopen(u8 handle, SceLocationLocationMethod lmethod, SceLocationHeadingMethod hmethod)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationGetMethod(u8 handle, vm::ptr<s32> lmethod, vm::ptr<s32> hmethod)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationGetLocation(u8 handle, vm::ptr<SceLocationLocationInfo> linfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationCancelGetLocation(u8 handle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationStartLocationCallback(u8 handle, u32 distance, vm::ptr<SceLocationLocationInfoCallback> callback, vm::ptr<void> userdata)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationStopLocationCallback(u8 handle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationGetHeading(u8 handle, vm::ptr<SceLocationHeadingInfo> hinfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationStartHeadingCallback(u8 handle, u32 difference, vm::ptr<SceLocationHeadingInfoCallback> callback, vm::ptr<void> userdata)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationStopHeadingCallback(u8 handle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationConfirm(u8 handle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationConfirmGetStatus(u8 handle, vm::ptr<s32> status)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationConfirmGetResult(u8 handle, vm::ptr<s32> result)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationConfirmAbort(u8 handle)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationGetPermission(u8 handle, vm::ptr<SceLocationPermissionInfo> info)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceLocationSetGpsEmulationFile(vm::ptr<char> filename)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceLibLocation, nid, name)

DECLARE(arm_module_manager::SceLocation)("SceLibLocation", []()
{
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
