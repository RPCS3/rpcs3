#pragma once

#ifndef ARCH_ARM64
#error "You have included an arm-only header"
#endif

#include <util/types.hpp>
#include "../CPUTranslator.h"

#include <unordered_set>

namespace aarch64
{
    enum gpr : s32
    {
        x0 = 0,
        x1, x2, x3, x4, x5, x6, x7, x8, x9,
        x10, x11, x12, x13, x14, x15, x16, x17, x18, x19,
        x20, x21, x22, x23, x24, x25, x26, x27, x28, x29, x30
    };

    // On non-x86 architectures GHC runs stackless. SP is treated as a pointer to scratchpad memory.
    // This pass keeps this behavior intact while preserving the expectations of the host's C++ ABI.
    class GHC_frame_preservation_pass : public translator_pass
    {
    public:
        struct function_info_t
        {
            u32 instruction_count;
            u32 num_external_calls;
            u32 stack_frame_size;     // Guessing this properly is critical for vector-heavy functions where spilling is a lot more common
            bool clobbers_x30;
        };

        struct instruction_info_t
        {
            bool is_call_inst;        // Is a function call. This includes a branch to external code.
            bool preserve_stack;      // Preserve the stack around this call.
            bool is_returning;        // This instruction "returns" to the next instruction (typically just llvm::CallInst*)
            bool callee_is_GHC;       // The other function is GHC
            bool is_tail_call;        // Tail call. Assume it is an exit/terminator.
            llvm::Function* callee;   // Callee if any
        };
    protected:
        std::unordered_set<std::string> visited_functions;

        struct
        {
            gpr base_register;
            u32  hypervisor_context_offset;
        } execution_context;

        std::function<bool(const std::string&)> exclusion_callback;

        void force_tail_call_terminators(llvm::Function& f);

        function_info_t preprocess_function(llvm::Function& f);

        instruction_info_t decode_instruction(llvm::Function& f, llvm::Instruction* i);
    public:

        GHC_frame_preservation_pass(
            gpr base_reg,
            u32 hv_ctx_offset,
            std::function<bool(const std::string&)> exclusion_callback = {});
        ~GHC_frame_preservation_pass() = default;

        void run(llvm::IRBuilder<>* irb, llvm::Function& f) override;
        void reset() override;
    };
}
