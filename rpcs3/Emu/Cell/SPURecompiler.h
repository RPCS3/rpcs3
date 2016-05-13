#pragma once

#include "SPUAnalyser.h"

#include <mutex>

// SPU Recompiler instance base (must be global or PS3 process-local)
class spu_recompiler_base
{
protected:
	std::mutex m_mutex; // must be locked in compile()

	const spu_function_t* m_func; // current function

	u32 m_pos; // current position

public:
	virtual ~spu_recompiler_base() = default;

	// Compile specified function
	virtual void compile(spu_function_t& f) = 0;

	// Run
	static void enter(class SPUThread&);
};
