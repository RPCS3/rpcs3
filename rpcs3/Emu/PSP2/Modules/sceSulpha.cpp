#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceSulpha.h"

logs::channel sceSulpha("sceSulpha");

s32 sceSulphaNetworkInit()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaNetworkShutdown()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaGetDefaultConfig(vm::ptr<SceSulphaConfig> config)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaGetNeededMemory(vm::cptr<SceSulphaConfig> config, vm::ptr<u32> sizeInBytes)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaInit(vm::cptr<SceSulphaConfig> config, vm::ptr<void> buffer, u32 sizeInBytes)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaShutdown()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaUpdate()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaFileConnect(vm::cptr<char> filename)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaFileDisconnect()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaSetBookmark(vm::cptr<char> name, s32 id)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaAgentsGetNeededMemory(vm::cptr<SceSulphaAgentsRegister> config, vm::ptr<u32> sizeInBytes)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaAgentsRegister(vm::cptr<SceSulphaAgentsRegister> config, vm::ptr<void> buffer, u32 sizeInBytes, vm::ptr<SceSulphaHandle> handles)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceSulphaAgentsUnregister(vm::cptr<SceSulphaHandle> handles, u32 agentCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceSulpha, nid, name)

DECLARE(arm_module_manager::SceSulpha)("SceSulpha", []()
{
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
