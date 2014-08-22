#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/SysCalls/Modules.h"
#include "sceNpSns.h"

//void sceNpSns_unload();
//void sceNpSns_init();
//Module sceNpSns(0x0059, sceNpSns_init, nullptr, sceNpSns_unload);
Module *sceNpSns = nullptr;

void sceNpSns_unload()
{
	// TODO: Unload SNS module
}

void sceNpSns_init()
{
	// TODO: Register SNS module functions here
}