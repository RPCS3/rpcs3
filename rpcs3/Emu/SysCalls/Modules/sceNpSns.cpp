#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"
#include "sceNpSns.h"

Module *sceNpSns = nullptr;

void sceNpSns_unload()
{
	// TODO: Unload SNS module
}

void sceNpSns_init(Module *pxThis)
{
	sceNpSns = pxThis;

	// TODO: Register SNS module functions here
}