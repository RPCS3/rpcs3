#include <stdafx.h>
#include "AArch64Signal.h"

namespace aarch64
{
    // Some of the EC codes we care about
    enum class EL1_exception_class
    {
        undefined = 0,

        instr_abort_0 = 32,     // PAGE_FAULT - Execute, change in EL
        instr_abort_1 = 33,     // PAGE_FAULT - Execute, same EL
        data_abort_0 = 36,      // PAGE_FAULT - Generic, causing change in EL (e.g kernel sig handler back to EL0)
        data_abort_1 = 37,      // PAGE_FAULT - Generic, no change in EL, e.g EL1 driver fault

        illegal_execution = 14, // BUS_ERROR
        unaligned_pc = 34,      // BUS_ERROR
        unaligned_sp = 38,      // BUS_ERROR

        breakpoint = 60,        // BRK
    };

#ifdef __linux__
    constexpr u32 ESR_CTX_MAGIC = 0x45535201;

    const aarch64_esr_ctx* find_EL1_esr_context(const ucontext_t* ctx)
    {
        u32 offset = 0;
        const auto& mctx = ctx->uc_mcontext;

        while ((offset + 4) < sizeof(mctx.__reserved))
        {
            const auto head = reinterpret_cast<const aarch64_cpu_ctx_block*>(&mctx.__reserved[offset]);
            if (!head->magic)
            {
                // End of linked list
                return nullptr;
            }

            if (head->magic == ESR_CTX_MAGIC)
            {
                return reinterpret_cast<const aarch64_esr_ctx*>(head);
            }

            offset += head->size;
        }

        return nullptr;
    }

    u64 _read_ESR_EL1(const ucontext_t* uctx)
    {
        auto esr_ctx = find_EL1_esr_context(uctx);
        return esr_ctx ? esr_ctx->esr : 0;
    }
#elif defined(__APPLE__)
    u64 _read_ESR_EL1(const ucontext_t* uctx)
    {
        // Easy to read from mcontext
        const auto darwin_ctx = reinterpret_cast<aarch64_darwin_mcontext64*>(uctx->uc_mcontext);
        return darwin_ctx->es.ESR;
    }
#else
    u64 _read_ESR_EL1(const ucontext_t*)
    {
        // Unimplemented
        return 0;
    }
#endif

    fault_reason decode_fault_reason(const ucontext_t* uctx)
    {
        auto esr = _read_ESR_EL1(uctx);
        if (!esr)
        {
            return fault_reason::undefined;
        }

        // We don't really care about most of the register fields, but we can check for a few things.
        const auto exception_class = (esr >> 26) & 0b111111;
        switch (static_cast<EL1_exception_class>(exception_class))
        {
        case EL1_exception_class::breakpoint:
            // Debug break
            return fault_reason::breakpoint;
        case EL1_exception_class::illegal_execution:
        case EL1_exception_class::unaligned_pc:
        case EL1_exception_class::unaligned_sp:
            return fault_reason::illegal_instruction;
        case EL1_exception_class::instr_abort_0:
        case EL1_exception_class::instr_abort_1:
            return fault_reason::instruction_execute;
        case EL1_exception_class::data_abort_0:
        case EL1_exception_class::data_abort_1:
            // Page fault
            break;
        default:
            return fault_reason::undefined;
        }

        // Check direction bit
        const auto direction = (esr >> 6u) & 1u;
        return direction ? fault_reason::data_write : fault_reason::data_read;
    }
}
