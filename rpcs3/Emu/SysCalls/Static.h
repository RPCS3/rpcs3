#pragma once

struct SFunc;

class PPUThread;

class StaticFuncManager
{
	std::vector<SFunc *> m_static_funcs_list; 
public:
	void StaticAnalyse(void* ptr, u32 size, u32 base);
	void StaticFinalize();
	void push_back(SFunc *ele);
	SFunc *operator[](size_t i);
	~StaticFuncManager();
};
