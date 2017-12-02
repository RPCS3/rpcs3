#pragma once

#include "SPUAnalyser.h"

#include <mutex>

using spu_inter_func_t = void(*)(SPUThread& spu, spu_opcode_t op);

// SPU Recompiler instance base (must be global or PS3 process-local)
class spu_recompiler_base
{
protected:
	SPUThread &m_thread;

	std::mutex m_mutex; // must be locked in compile()

	const spu_function_t* m_func; // current function

	u32 m_pos; // current position

public:
	spu_recompiler_base(SPUThread &thread) : m_thread(thread) {};
	virtual ~spu_recompiler_base();

	// Compile specified function
	virtual void compile(spu_function_t& f) = 0;

	// Run
	static void enter(class SPUThread&);

	static u32 SPUFunctionCall(SPUThread * spu, u32 link) noexcept;
	static u32 SPUInterpreterCall(SPUThread * spu, u32 opcode, spu_inter_func_t func) noexcept;
};
