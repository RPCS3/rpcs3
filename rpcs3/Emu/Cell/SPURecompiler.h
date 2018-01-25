#pragma once

#include "SPUAnalyser.h"

#include <mutex>

// SPU Recompiler instance base (must be global or PS3 process-local)
class spu_recompiler_base
{
protected:
	std::mutex m_mutex; // must be locked in compile()

	std::shared_ptr<const spu_function_contents_t> m_func; // current function

	u32 m_pos; // current position

public:
	virtual ~spu_recompiler_base();

	// Compile specified function
	virtual bool compile(std::shared_ptr<spu_function_contents_t>) = 0;

	// Run
	static void enter(class SPUThread&);
};
