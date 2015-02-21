#pragma once

struct SFunc;

class PPUThread;

class StaticFuncManager
{
public:
	void StaticAnalyse(void* ptr, u32 size, u32 base);
	void StaticFinalize();
};
