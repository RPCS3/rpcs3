#pragma once

#ifndef ARCH_ARM64
#error "You have included an arm-only header"
#endif

#include "AArch64Common.h"

#include <unordered_set>

namespace aarch64
{
    class UASM;
    using ASMBlock = UASM;

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
            bool is_leaf;
        };

        struct instruction_info_t
        {
            bool is_call_inst;        // Is a function call. This includes a branch to external code.
            bool preserve_stack;      // Preserve the stack around this call.
            bool is_returning;        // This instruction "returns" to the next instruction (typically just llvm::CallInst*)
            bool callee_is_GHC;       // The other function is GHC
            bool is_tail_call;        // Tail call. Assume it is an exit/terminator.
            bool is_indirect;         // Indirect call. Target is the first operand.
            llvm::Function* callee;   // Callee if any
            std::string callee_name;  // Name of the callee.
        };

        struct config_t
        {
            bool debug_info = false;           // Record debug information
            bool use_stack_frames = true;      // Allocate a stack frame for each function. The gateway can alternatively manage a global stack to use as scratch.
            bool optimize = true;              // Optimize instructions when possible. Set to false when debugging.
            u32 hypervisor_context_offset = 0; // Offset within the "thread" object where we can find the hypervisor context (registers configured at gateway).
            std::function<bool(const std::string&)> exclusion_callback;    // [Optional] Callback run on each function before transform. Return "true" to exclude from frame processing.
            std::vector<std::pair<std::string, gpr>> base_register_lookup; // [Optional] Function lookup table to determine the location of the "thread" context.
            std::vector<std::string> faux_function_list;                   // [Optional] List of faux block names to treat as untrusted - typically fake functions representing codecaves.
        };

    protected:
        std::unordered_set<std::string> m_visited_functions;

        config_t m_config;

        void force_tail_call_terminators(llvm::Function& f);

        function_info_t preprocess_function(const llvm::Function& f);

        instruction_info_t decode_instruction(const llvm::Function& f, const llvm::Instruction* i);

        bool is_ret_instruction(const llvm::Instruction* i);

        bool is_inlined_call(const llvm::CallInst* ci);

        bool is_faux_function(const std::string& function_name);

        gpr get_base_register_for_call(const std::string& callee_name, gpr default_reg = gpr::x19);

        void process_leaf_function(llvm::IRBuilder<>* irb, llvm::Function& f);

        llvm::BasicBlock::iterator patch_tail_call(
            llvm::IRBuilder<>* irb,
            llvm::Function& f,
            llvm::BasicBlock::iterator where,
            const instruction_info_t& instruction_info,
            const function_info_t& function_info,
            const ASMBlock& frame_epilogue);
    public:

        GHC_frame_preservation_pass(const config_t& configuration);
        ~GHC_frame_preservation_pass() = default;

        void run(llvm::IRBuilder<>* irb, llvm::Function& f) override;
        void reset() override;
    };
}
