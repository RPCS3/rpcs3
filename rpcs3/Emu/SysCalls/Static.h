#pragma once

class StaticFuncManager
{
public:
	void StaticAnalyse(void* ptr, u32 size, u32 base);
	void StaticFinalize();
};
