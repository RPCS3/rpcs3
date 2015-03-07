#include "stdafx.h"
#include "Emu/SysCalls/Modules.h"
#include "sceNpSns.h"

extern Module sceNpSns;

Module sceNpSns("sceNpSns", []()
{
	// TODO: Register SNS module functions here
});
