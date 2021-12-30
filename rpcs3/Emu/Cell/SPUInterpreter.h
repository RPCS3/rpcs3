#pragma once

#include "SPUOpcodes.h"

class spu_thread;

using spu_intrp_func_t = bool(*)(spu_thread& spu, spu_opcode_t op);

template <typename IT>
struct spu_interpreter_t;

struct spu_interpreter
{
	static void set_interrupt_status(spu_thread&, spu_opcode_t);
};

struct spu_interpreter_rt_base
{
protected:
	std::unique_ptr<spu_interpreter_t<spu_intrp_func_t>> ptrs;

	spu_interpreter_rt_base() noexcept;

	spu_interpreter_rt_base(const spu_interpreter_rt_base&) = delete;

	spu_interpreter_rt_base& operator=(const spu_interpreter_rt_base&) = delete;

	virtual ~spu_interpreter_rt_base();
};

struct spu_interpreter_rt : spu_interpreter_rt_base
{
	spu_interpreter_rt() noexcept;

	spu_intrp_func_t decode(u32 op) const noexcept
	{
		return table.decode(op);
	}

private:
	spu_decoder<spu_interpreter_t<spu_intrp_func_t>, spu_intrp_func_t> table;
};
