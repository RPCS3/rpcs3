#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNp.h"

extern Module sceNp2;

struct sceNp2Internal
{
	bool m_bSceNp2Initialized;
	bool m_bSceNp2Matching2Initialized;
	bool m_bSceNp2Matching2Initialized2;

	sceNp2Internal()
		: m_bSceNp2Initialized(false),
		  m_bSceNp2Matching2Initialized(false),
		  m_bSceNp2Matching2Initialized2(false)
	{
	}
};

sceNp2Internal sceNp2Instance;

s32 sceNp2Init(u32 poolsize, vm::ptr<u32> poolptr)
{
	sceNp2.Warning("sceNp2Init(poolsize=%d, poolptr=0x%x)", poolsize, poolptr);

	if (sceNp2Instance.m_bSceNp2Initialized)
	{
		sceNp2.Error("sceNp2Init(): sceNp2 has been already initialized.");
		return SCE_NP_ERROR_ALREADY_INITIALIZED;
	}

	if (poolsize == 0)
	{
		sceNp2.Error("sceNp2Init(): poolsize given is 0.");
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}
	else if (poolsize < 128 * 1024)
	{
		sceNp2.Error("sceNp2Init(): poolsize given is under 131072 bytes.");
		return SCE_NP_ERROR_INSUFFICIENT_BUFFER;
	}

	if (!poolptr)
	{
		sceNp2.Error("sceNp2Init(): poolptr is invalid.");
		return SCE_NP_ERROR_INVALID_ARGUMENT;
	}

	sceNpInstance.m_bSceNpInitialized = true;
	sceNp2Instance.m_bSceNp2Initialized = true;

	return CELL_OK;
}

s32 sceNpMatching2Init(u32 poolsize, s32 priority)
{
	sceNp2.Todo("sceNpMatching2Init(poolsize=%d, priority=%d)", poolsize, priority);

	if (!sceNp2Instance.m_bSceNp2Initialized)
	{
		sceNp2.Error("sceNpMatching2Init(): sceNp2 has not been intialized.");
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (sceNp2Instance.m_bSceNp2Matching2Initialized)
	{
		sceNp2.Error("sceNpMatching2Init(): sceNpMatching2 has already been intialized.");
		return SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
	}

	sceNp2Instance.m_bSceNp2Matching2Initialized = true;

	return CELL_OK;
}

s32 sceNpMatching2Init2(u32 poolsize, s32 priority, vm::ptr<SceNpMatching2UtilityInitParam> param)
{
	sceNp2.Todo("sceNpMatching2Init2(poolsize=%d, priority=%d, param_addr=0x%x)", poolsize, priority, param.addr());

	if (!sceNp2Instance.m_bSceNp2Initialized)
	{
		sceNp2.Error("sceNpMatching2Init2(): sceNp2 has not been intialized.");
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (sceNp2Instance.m_bSceNp2Matching2Initialized2)
	{
		sceNp2.Error("sceNpMatching2Init2(): new sceNpMatching2 has already been intialized.");
		return SCE_NP_MATCHING2_ERROR_ALREADY_INITIALIZED;
	}

	sceNp2Instance.m_bSceNp2Matching2Initialized2 = true;

	// TODO:
	// 1. Create an internal thread
	// 2. Create heap area to be used by the NP matching 2 utility
	// 3. Set maximum lengths for the event data queues in the system

	return CELL_OK;
}

s32 sceNp2Term()
{
	sceNp2.Warning("sceNp2Term()");

	if (!sceNp2Instance.m_bSceNp2Initialized)
	{
		sceNp2.Error("sceNp2Term(): sceNp2 has not been intialized.");
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	sceNp2Instance.m_bSceNp2Initialized = false;

	return CELL_OK;
}

s32 sceNpMatching2Term(PPUThread& ppu)
{
	sceNp2.Warning("sceNpMatching2Term()");

	if (!sceNp2Instance.m_bSceNp2Initialized)
	{
		sceNp2.Error("sceNpMatching2Term(): sceNp2 has not been intialized.");
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!sceNp2Instance.m_bSceNp2Matching2Initialized)
	{
		sceNp2.Error("sceNpMatching2Term(): sceNpMatching2 has not been intialized.");
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	sceNp2Instance.m_bSceNp2Matching2Initialized = false;

	return CELL_OK;
}

s32 sceNpMatching2Term2()
{
	sceNp2.Warning("sceNpMatching2Term2()");

	if (!sceNp2Instance.m_bSceNp2Initialized)
	{
		sceNp2.Error("sceNpMatching2Term2(): sceNp2 has not been intialized.");
		return SCE_NP_ERROR_NOT_INITIALIZED;
	}

	if (!sceNp2Instance.m_bSceNp2Matching2Initialized2)
	{
		sceNp2.Error("sceNpMatching2Term(): new sceNpMatching2 has not been intialized.");
		return SCE_NP_MATCHING2_ERROR_NOT_INITIALIZED;
	}

	sceNp2Instance.m_bSceNp2Matching2Initialized2 = false;

	return CELL_OK;
}

Module sceNp2("sceNp2", []()
{
	sceNp2Instance.m_bSceNp2Initialized = false;
	sceNp2Instance.m_bSceNp2Matching2Initialized = false;
	sceNp2Instance.m_bSceNp2Matching2Initialized2 = false;

	REG_FUNC(sceNp2, sceNp2Init);
	REG_FUNC(sceNp2, sceNpMatching2Init);
	REG_FUNC(sceNp2, sceNpMatching2Init2);
	REG_FUNC(sceNp2, sceNp2Term);
	REG_FUNC(sceNp2, sceNpMatching2Term);
	REG_FUNC(sceNp2, sceNpMatching2Term2);
});