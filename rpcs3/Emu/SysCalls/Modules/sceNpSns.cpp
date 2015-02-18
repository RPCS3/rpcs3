#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"
#include "sceNpSns.h"

extern Module sceNpSns;

void sceNpSns_unload()
{
	// TODO: Unload SNS module
}

Module sceNpSns("sceNpSns", []()
{
	// TODO: Register SNS module functions here
});
