#pragma once

#include "PPUOpcodes.h"
#include <memory>
#include <vector>
#include <stdexcept>

class ppu_thread;

using ppu_intrp_func_t = void(*)(ppu_thread&, ppu_opcode_t, be_t<u32>*, ppu_intrp_func*);

struct ppu_intrp_func
{
    ppu_intrp_func_t fn;
};

class ppu_interpreter_rt
{
public:
    ppu_interpreter_rt();

    ppu_intrp_func_t decode(u32 op) const;

private:
    std::vector<ppu_intrp_func_t> m_functions;
};

