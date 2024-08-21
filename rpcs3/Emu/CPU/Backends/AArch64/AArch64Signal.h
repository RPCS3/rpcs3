#pragma once

#include <util/types.hpp>
#include <sys/ucontext.h>

namespace aarch64
{
    // Some renamed kernel definitions, we don't need to include kernel headers directly
#pragma pack(push, 1)

    struct aarch64_cpu_ctx_block
    {
        u32 magic;
        u32 size;
    };

    struct aarch64_esr_ctx
    {
        aarch64_cpu_ctx_block head;
        u64 esr;      // Exception syndrome register
    };

#pragma pack(pop)

    // Fault reason
    enum class fault_reason
    {
        undefined = 0,
        data_read,
        data_write,
        instruction_execute,
        illegal_instruction,
        breakpoint
    };

    fault_reason decode_fault_reason(const ucontext_t* uctx);
}
