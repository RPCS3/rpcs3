#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceSulpha.h"

s32 sceSulphaNetworkInit()
{
	throw __FUNCTION__;
}

s32 sceSulphaNetworkShutdown()
{
	throw __FUNCTION__;
}

s32 sceSulphaGetDefaultConfig(vm::ptr<SceSulphaConfig> config)
{
	throw __FUNCTION__;
}

s32 sceSulphaGetNeededMemory(vm::ptr<const SceSulphaConfig> config, vm::ptr<u32> sizeInBytes)
{
	throw __FUNCTION__;
}

s32 sceSulphaInit(vm::ptr<const SceSulphaConfig> config, vm::ptr<void> buffer, u32 sizeInBytes)
{
	throw __FUNCTION__;
}

s32 sceSulphaShutdown()
{
	throw __FUNCTION__;
}

s32 sceSulphaUpdate()
{
	throw __FUNCTION__;
}

s32 sceSulphaFileConnect(vm::ptr<const char> filename)
{
	throw __FUNCTION__;
}

s32 sceSulphaFileDisconnect()
{
	throw __FUNCTION__;
}

s32 sceSulphaSetBookmark(vm::ptr<const char> name, s32 id)
{
	throw __FUNCTION__;
}

s32 sceSulphaAgentsGetNeededMemory(vm::ptr<const SceSulphaAgentsRegister> config, vm::ptr<u32> sizeInBytes)
{
	throw __FUNCTION__;
}

s32 sceSulphaAgentsRegister(vm::ptr<const SceSulphaAgentsRegister> config, vm::ptr<void> buffer, u32 sizeInBytes, vm::ptr<SceSulphaHandle> handles)
{
	throw __FUNCTION__;
}

s32 sceSulphaAgentsUnregister(vm::ptr<const SceSulphaHandle> handles, u32 agentCount)
{
	throw __FUNCTION__;
}


#define REG_FUNC(nid, name) reg_psv_func(nid, &sceSulpha, #name, name)

psv_log_base sceSulpha("SceSulpha", []()
{
	sceSulpha.on_load = nullptr;
	sceSulpha.on_unload = nullptr;
	sceSulpha.on_stop = nullptr;
	sceSulpha.on_error = nullptr;

	REG_FUNC(0xB4668AEA, sceSulphaNetworkInit);
	REG_FUNC(0x0FC71B72, sceSulphaNetworkShutdown);
	REG_FUNC(0xA6A05C50, sceSulphaGetDefaultConfig);
	REG_FUNC(0xD52E5A5A, sceSulphaGetNeededMemory);
	REG_FUNC(0x324F158F, sceSulphaInit);
	REG_FUNC(0x10770BA7, sceSulphaShutdown);
	REG_FUNC(0x920EC7BF, sceSulphaUpdate);
	REG_FUNC(0x7968A138, sceSulphaFileConnect);
	REG_FUNC(0xB16E7B88, sceSulphaFileDisconnect);
	REG_FUNC(0x5E15E164, sceSulphaSetBookmark);
	REG_FUNC(0xC5752B6B, sceSulphaAgentsGetNeededMemory);
	REG_FUNC(0x7ADB454D, sceSulphaAgentsRegister);
	REG_FUNC(0x2A8B74D7, sceSulphaAgentsUnregister);
	//REG_FUNC(0xDE7E2911, sceSulphaGetAgent);
	//REG_FUNC(0xA41B7402, sceSulphaNodeNew);
	//REG_FUNC(0xD44C9F86, sceSulphaNodeDelete);
	//REG_FUNC(0xBF61F3B8, sceSulphaEventNew);
	//REG_FUNC(0xD5D995A9, sceSulphaEventDelete);
	//REG_FUNC(0xB0C2B9CE, sceSulphaEventAdd);
	//REG_FUNC(0xBC6A2833, sceSulphaEventReport);
	//REG_FUNC(0x29F0DA12, sceSulphaGetTimestamp);
	//REG_FUNC(0x951D159D, sceSulphaLogSetLevel);
	//REG_FUNC(0x5C6815C6, sceSulphaLogHandler);
});
