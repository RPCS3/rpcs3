#pragma once

#include <util/types.hpp>
#include "../../CPUTranslator.h"

namespace aarch64
{
    enum gpr : s32
    {
        x0 = 0,
        x1, x2, x3, x4, x5, x6, x7, x8, x9,
        x10, x11, x12, x13, x14, x15, x16, x17, x18, x19,
        x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
    };

    enum spr : s32
    {
        xzr = 0,
        pc,
        sp
    };

    static const char* gpr_names[] =
    {
        "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9",
        "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17", "x18", "x19",
        "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "x29", "x30"
    };

    static const char* spr_names[] =
    {
        "xzr", "pc", "sp"
    };

    static const char* spr_asm_names[] =
    {
        "xzr", ".", "sp"
    };

    std::string get_cpu_name();
    std::string get_cpu_brand();
}
