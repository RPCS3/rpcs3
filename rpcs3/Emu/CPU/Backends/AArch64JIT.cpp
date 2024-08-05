#include "stdafx.h"
#include "AArch64JIT.h"
#include "../Hypervisor.h"

LOG_CHANNEL(jit_log, "JIT");

#define STDOUT_DEBUG

#ifndef STDOUT_DEBUG
#define DPRINT jit_log.trace
#else
#define DPRINT(...)\
    do {\
        printf(__VA_ARGS__);\
        printf("\n");\
        fflush(stdout);\
    } while (0)
#endif

namespace aarch64
{
    // FIXME: This really should be part of fmt
    static std::string join_strings(const std::vector<std::string>& v, const char* delim)
    {
        std::string result;
        for (const auto& s : v)
        {
            if (!result.empty())
            {
                result += delim;
            }
            result += s;
        }
        return result;
    }

    using instruction_info_t = GHC_frame_preservation_pass::instruction_info_t;
    using function_info_t = GHC_frame_preservation_pass::function_info_t;

    GHC_frame_preservation_pass::GHC_frame_preservation_pass(
        u32 hv_ctx_offset,
        const std::vector<std::pair<std::string, gpr>>& base_register_lookup,
        std::function<bool(const std::string&)> exclusion_callback)
    {
        execution_context.base_register_lookup = base_register_lookup;
        execution_context.hypervisor_context_offset = hv_ctx_offset;
        this->exclusion_callback = exclusion_callback;
    }

    void GHC_frame_preservation_pass::reset()
    {
        visited_functions.clear();
    }

    void GHC_frame_preservation_pass::force_tail_call_terminators(llvm::Function& f)
    {
        // GHC functions are not call-stack preserving and can therefore never return if they make any external calls at all.
        // Replace every terminator clause with a tail call explicitly. This is already done for X64 to work, but better safe than sorry.
        for (auto& bb : f)
        {
            auto bit = bb.begin(), prev = bb.end();
            for (; bit != bb.end(); prev = bit, ++bit)
            {
                if (prev == bb.end())
                {
                    continue;
                }

                if (auto ri = llvm::dyn_cast<llvm::ReturnInst>(&*bit))
                {
                    if (auto ci = llvm::dyn_cast<llvm::CallInst>(&*prev))
                    {
                        // This is a "ret" that is coming after a "call" to another funciton.
                        // Enforce that it must be a tail call.
                        if (!ci->isTailCall())
                        {
                            ci->setTailCall();
                        }
                    }
                }
            }
        }
    }

    function_info_t GHC_frame_preservation_pass::preprocess_function(llvm::Function& f)
    {
        function_info_t result{};
        result.instruction_count = f.getInstructionCount();

        // Blanket exclusions. Stubs or dispatchers that do not compute anything themselves.
        if (f.getName() == "__spu-null")
        {
            // Don't waste the effort processing this stub. It has no points of concern
            return result;
        }

        // Stack frame estimation. SPU code can be very long and consumes several KB of stack.
        u32 stack_frame_size = 128u;
        // Actual ratio is usually around 1:4
        const u32 expected_compiled_instr_count = f.getInstructionCount() * 4;
        // Because GHC doesn't preserve stack (all stack is scratch), we know we'll start to spill once we go over the number of actual regs.
        // We use a naive allocator that just assumes each instruction consumes a register slot. We "spill" every 32 instructions.
        // FIXME: Aggressive spill is only really a thing with vector operations. We can detect those instead.
        // A proper fix is to port this to a MF pass, but I have PTSD from working at MF level.
        const u32 spill_pages = (expected_compiled_instr_count + 127u) / 128u;
        stack_frame_size *= std::min(spill_pages, 32u); // 128 to 4k dynamic. It is unlikely that any frame consumes more than 4096 bytes

        result.stack_frame_size = stack_frame_size;
        result.instruction_count = f.getInstructionCount();
        result.num_external_calls = 0;

        // The LR is not spared by LLVM in cases where there is a lot of spilling.
        // This is another thing to be moved to a MachineFunction pass.
        result.clobbers_x30 = result.instruction_count > 32;

        for (auto& bb : f)
        {
            for (auto& inst : bb)
            {
                if (auto ci = llvm::dyn_cast<llvm::CallInst>(&inst))
                {
                    result.num_external_calls++;
                    result.clobbers_x30 |= (!ci->isTailCall());
                }
            }
        }

        return result;
    }

    instruction_info_t GHC_frame_preservation_pass::decode_instruction(llvm::Function& f, llvm::Instruction* i)
    {
        instruction_info_t result{};
        if (auto ci = llvm::dyn_cast<llvm::CallInst>(i))
        {
            // Watch out for injected ASM blocks...
            if (llvm::isa<llvm::InlineAsm>(ci->getCalledOperand()))
            {
                // Not a real call. This is just an insert of inline asm
                return result;
            }

            result.is_call_inst = true;
            result.is_returning = true;
            result.preserve_stack = !ci->isTailCall();
            result.callee = ci->getCalledFunction();
            result.is_tail_call = ci->isTailCall();

            if (!result.callee)
            {
                // Indirect call (call from raw value).
                result.is_indirect = true;
                result.callee_is_GHC = ci->getCallingConv() == llvm::CallingConv::GHC;
                result.callee_name = "__indirect_call";
            }
            else
            {
                result.callee_is_GHC = result.callee->getCallingConv() == llvm::CallingConv::GHC;
                result.callee_name = result.callee->getName().str();
            }
            return result;
        }

        if (auto bi = llvm::dyn_cast<llvm::BranchInst>(i))
        {
            // More likely to jump out via an unconditional...
            if (!bi->isConditional())
            {
                ensure(bi->getNumSuccessors() == 1);
                auto targetbb = bi->getSuccessor(0);

                result.callee = targetbb->getParent();
                result.callee_name = result.callee->getName().str();
                result.is_call_inst = result.callee_name != f.getName();
            }

            return result;
        }

        if (auto bi = llvm::dyn_cast<llvm::IndirectBrInst>(i))
        {
            // Very unlikely to be the same function. Can be considered a function exit.
            ensure(bi->getNumDestinations() == 1);
            auto targetbb = ensure(bi->getSuccessor(0)); // This is guaranteed to fail but I've yet to encounter this

            result.callee = targetbb->getParent();
            result.callee_name = result.callee->getName().str();
            result.is_call_inst = result.callee_name != f.getName();
            return result;
        }

        if (auto bi = llvm::dyn_cast<llvm::CallBrInst>(i))
        {
            ensure(bi->getNumSuccessors() == 1);
            auto targetbb = bi->getSuccessor(0);

            result.callee = targetbb->getParent();
            result.callee_name = result.callee->getName().str();
            result.is_call_inst = result.callee_name != f.getName();
            return result;
        }

        if (auto bi = llvm::dyn_cast<llvm::InvokeInst>(i))
        {
            ensure(bi->getNumSuccessors() == 2);
            auto targetbb = bi->getSuccessor(0);

            result.callee = targetbb->getParent();
            result.callee_name = result.callee->getName().str();
            result.is_call_inst = result.callee_name != f.getName();
            return result;
        }

        return result;
    }

    gpr GHC_frame_preservation_pass::get_base_register_for_call(const std::string& callee_name)
    {
        // We go over the base_register_lookup table and find the first matching pattern
        for (const auto& pattern : execution_context.base_register_lookup)
        {
            if (callee_name.starts_with(pattern.first))
            {
                return pattern.second;
            }
        }

        // Default is x19
        return aarch64::x19;
    }

    void GHC_frame_preservation_pass::run(llvm::IRBuilder<>* irb, llvm::Function& f)
    {
        if (f.getCallingConv() != llvm::CallingConv::GHC)
        {
            // If we're not doing GHC, the calling conv will have stack fixup on its own via prologue/epilogue
            return;
        }

        if (f.getInstructionCount() == 0)
        {
            // Nothing to do. Happens with placeholder functions such as branch patchpoints
            return;
        }

        const auto this_name = f.getName().str();
        if (visited_functions.find(this_name) != visited_functions.end())
        {
            // Already processed. Only useful when recursing which is currently not used.
            DPRINT("Function %s was already processed. Skipping.\n", this_name.c_str());
            return;
        }
        visited_functions.insert(this_name);

        if (exclusion_callback && exclusion_callback(this_name))
        {
            // Function is explicitly excluded
            return;
        }

        // Preprocessing.
        auto function_info = preprocess_function(f);
        if (function_info.num_external_calls == 0 && function_info.stack_frame_size == 0)
        {
            // No stack frame injection and no external calls to patch up. This is a leaf function, nothing to do.
            return;
        }

        // Force tail calls on all terminators
        force_tail_call_terminators(f);

        // Asm snippets for patching stack frame
        std::string frame_prologue, frame_epilogue;

        if (function_info.stack_frame_size > 0)
        {
            // NOTE: The stack frame here is purely optional, we can pre-allocate scratch on the gateway.
            // However, that is an optimization for another time, this helps make debugging easier.
            frame_prologue = fmt::format("sub sp, sp, #%u;", function_info.stack_frame_size);
            frame_epilogue = fmt::format("add sp, sp, #%u;", function_info.stack_frame_size);

            // Emit the frame prologue. We use a BB here for extra safety as it solves the problem of backwards jumps re-executing the prologue.
            auto functionStart = &f.front();
            auto prologueBB = llvm::BasicBlock::Create(f.getContext(), "", &f, functionStart);
            irb->SetInsertPoint(prologueBB, prologueBB->begin());
            LLVM_ASM_VOID(frame_prologue, irb, f.getContext());
            irb->CreateBr(functionStart);
        }

        // Now we start processing
        bool terminator_found = false;
        for (auto& bb : f)
        {
            for (auto bit = bb.begin(); bit != bb.end();)
            {
                const auto instruction_info = decode_instruction(f, &(*bit));
                if (!instruction_info.is_call_inst)
                {
                    ++bit;
                    continue;
                }

                std::string callee_name = "__unknown";
                if (const auto cf = instruction_info.callee)
                {
                    callee_name = cf->getName().str();
                    if (cf->hasFnAttribute(llvm::Attribute::AlwaysInline) || callee_name.starts_with("llvm."))
                    {
                        // Always inlined call. Likely inline Asm. Skip
                        ++bit;
                        continue;
                    }

                    // Technically We should also ignore any host functions linked in, usually starting with ppu_ or spu_ prefix.
                    // However, there is not much guarantee that those are safe with only rare exceptions, and it doesn't hurt to patch the frame around them that much anyway.
                }

                terminator_found |= instruction_info.is_tail_call;

                if (!instruction_info.preserve_stack)
                {
                    // Now we patch the call if required. For normal calls that 'return' (i.e calls to C/C++ ABI), we do not patch them as they will manage the stack themselves (callee-managed)
                    llvm::Instruction* original_inst = llvm::dyn_cast<llvm::Instruction>(bit);
                    irb->SetInsertPoint(ensure(llvm::dyn_cast<llvm::Instruction>(bit)));

                    if (function_info.stack_frame_size > 0)
                    {
                        // 1. Nuke the local stack frame if any
                        LLVM_ASM_VOID(frame_epilogue, irb, f.getContext());
                    }

                    // 2. We're about to make a tail call. This means after this call, we're supposed to return immediately. In that case, don't link, lower to branch only.
                    // Note that branches have some undesirable side-effects. For one, we lose the argument inputs, which the callee is expecting.
                    // This means we burn some cycles on every exit, but in return we do not require one instruction on the prologue + the ret chain is eliminated.
                    // No ret-chain also means two BBs can call each other indefinitely without running out of stack without relying on llvm to optimize that away.

                    std::string exit_fn;
                    auto ci = ensure(llvm::dyn_cast<llvm::CallInst>(original_inst));
                    auto operand_count = ci->getNumOperands() - 1; // The last operand is the callee, not a real operand
                    std::vector<std::string> constraints;
                    std::vector<llvm::Value*> args;

                    // We now load the callee args.
                    // FIXME: This is often times redundant and wastes cycles, we'll clean this up in a MachineFunction pass later.
                    int args_base_reg = instruction_info.callee_is_GHC ? aarch64::x19 : aarch64::x0; // GHC args are always x19..x25
                    for (unsigned i = 0; i < operand_count; ++i)
                    {
                        args.push_back(ci->getOperand(i));
                        exit_fn += fmt::format("mov x%d, $%u;\n", args_base_reg++, i);
                        constraints.push_back("r");
                    }

                    auto context_base_reg = get_base_register_for_call(instruction_info.callee_name);
                    if (!instruction_info.callee_is_GHC)
                    {
                        // For non-GHC calls, we have to remap the arguments to x0...
                        context_base_reg = static_cast<gpr>(context_base_reg - 19);
                    }

                    if (function_info.clobbers_x30)
                    {
                        // 3. Restore the exit gate as the current return address
                        // We want to do this after loading the arguments in case there was any spilling involved.
                        DPRINT("Patching call from %s to %s on register %d...",
                            this_name.c_str(),
                            instruction_info.callee_name.c_str(),
                            static_cast<int>(context_base_reg));

                        const auto x30_tail_restore = fmt::format(
                            "ldr x30, [x%u, #%u];\n",       // Load x30 from thread context
                            static_cast<u32>(context_base_reg),
                            execution_context.hypervisor_context_offset);

                        exit_fn += x30_tail_restore;
                    }

                    auto target = ensure(ci->getCalledOperand());
                    args.push_back(target);

                    if (instruction_info.is_indirect)
                    {
                        constraints.push_back("r");
                        exit_fn += fmt::format("br $%u;\n", operand_count);
                    }
                    else
                    {
                        constraints.push_back("i");
                        exit_fn += fmt::format("b $%u;\n", operand_count);
                    }

                    // Emit the branch
                    llvm_asm(irb, exit_fn, args, join_strings(constraints, ","), f.getContext());

                    // Delete original call instruction
                    bit = ci->eraseFromParent();
                }

                // Next
                if (bit != bb.end())
                {
                    ++bit;
                }
            }
        }

        ensure(terminator_found, "Could not find terminator for function!");
    }
}
