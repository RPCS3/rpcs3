#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceLocation;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceLocation, #name, name)

psv_log_base sceLocation("SceLibLocation", []()
{
	sceLocation.on_load = nullptr;
	sceLocation.on_unload = nullptr;
	sceLocation.on_stop = nullptr;

	//REG_FUNC(0xDD271661, sceLocationOpen);
	//REG_FUNC(0x14FE76E8, sceLocationClose);
	//REG_FUNC(0xB1F55065, sceLocationReopen);
	//REG_FUNC(0x188CE004, sceLocationGetMethod);
	//REG_FUNC(0x15BC27C8, sceLocationGetLocation);
	//REG_FUNC(0x71503251, sceLocationCancelGetLocation);
	//REG_FUNC(0x12D1F0EA, sceLocationStartLocationCallback);
	//REG_FUNC(0xED378700, sceLocationStopLocationCallback);
	//REG_FUNC(0x4E9E5ED9, sceLocationGetHeading);
	//REG_FUNC(0x07D4DFE0, sceLocationStartHeadingCallback);
	//REG_FUNC(0x92E53F94, sceLocationStopHeadingCallback);
	//REG_FUNC(0xE055BCF5, sceLocationSetHeapAllocator);
	//REG_FUNC(0xC895E567, sceLocationConfirm);
	//REG_FUNC(0x730FF842, sceLocationConfirmGetStatus);
	//REG_FUNC(0xFF016C13, sceLocationConfirmGetResult);
	//REG_FUNC(0xE3CBF875, sceLocationConfirmAbort);
	//REG_FUNC(0x482622C6, sceLocationGetPermission);
	//REG_FUNC(0xDE0A9EA4, sceLocationSetGpsEmulationFile);
	//REG_FUNC(0x760D08FF, sceLocationConfirmSetMessage);
});
