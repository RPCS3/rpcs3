#pragma once

#include "Emu/CPU/CPUDecoder.h"
#include "SPUAnalyser.h"

class SPUThread;

// SPU Recompiler instance base (must be global or PS3 process-local)
class SPURecompilerBase
{
protected:
	std::mutex m_mutex; // must be locked in compile()

	const spu_function_t* m_func; // current function

	u32 m_pos; // current position

public:
	virtual void compile(spu_function_t& f) = 0; // compile specified function
	virtual ~SPURecompilerBase() {};
};

// SPU Decoder instance (created per SPU thread)
class SPURecompilerDecoder final : public CPUDecoder
{
public:
	const std::shared_ptr<SPUDatabase> db; // associated SPU Analyser instance

	const std::shared_ptr<SPURecompilerBase> rec; // assiciated SPU Recompiler instance

	SPUThread& spu; // associated SPU Thread

	SPURecompilerDecoder(SPUThread& spu);

	u32 DecodeMemory(const u32 address) override; // non-virtual override (to avoid virtual call whenever possible)
};
