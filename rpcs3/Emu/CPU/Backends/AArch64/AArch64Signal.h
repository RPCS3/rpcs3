#pragma once

#include <util/types.hpp>

#ifndef _WIN32
#include <sys/ucontext.h>
#else
using ucontext_t = void;
#endif

namespace aarch64
{
    // Some renamed kernel definitions, we don't need to include kernel headers directly
#pragma pack(push, 1)

#if defined(__linux__)
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
#elif defined(__APPLE__)
    struct aarch64_exception_state
    {
        u64 FAR;      // Fault address reg
        u32 ESR;      // Exception syndrome reg (ESR_EL1)
        u32 exception_id;
    };

    struct aarch64_darwin_mcontext64
    {
        aarch64_exception_state es;
        // Other states we don't care about follow this field
    };
#endif

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
