#include "stdafx.h"
#include "AArch64JIT.h"
#include "../Hypervisor.h"

LOG_CHANNEL(jit_log, "JIT");

#define STDOUT_DEBUG 0

#define DPRINT1(...)\
    do {\
        printf(__VA_ARGS__);\
        printf("\n");\
        fflush(stdout);\
    } while (0)

#if STDOUT_DEBUG
#define DPRINT DPRINT1
#else
#define DPRINT jit_log.trace
#endif

namespace aarch64
{
    using instruction_info_t = GHC_frame_preservation_pass::instruction_info_t;
    using function_info_t = GHC_frame_preservation_pass::function_info_t;

    GHC_frame_preservation_pass::GHC_frame_preservation_pass(const config_t& configuration)
        : m_config(configuration)
    {}

    void GHC_frame_preservation_pass::reset()
    {
        m_visited_functions.clear();
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

                if (llvm::isa<llvm::ReturnInst>(&*bit))
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

    function_info_t GHC_frame_preservation_pass::preprocess_function(const llvm::Function& f)
    {
        function_info_t result{};
        result.instruction_count = f.getInstructionCount();

        // Blanket exclusions. Stubs or dispatchers that do not compute anything themselves.
        if (f.getName() == "__spu-null")
        {
            // Don't waste the effort processing this stub. It has no points of concern
            result.num_external_calls = 1;
            return result;
        }

        if (m_config.use_stack_frames)
        {
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
        }

        result.instruction_count = f.getInstructionCount();
        result.num_external_calls = 0;

        // The LR is not spared by LLVM in cases where there is a lot of spilling.
        // This is much easier to manage with a custom LLVM branch as we can just mark X30 as off-limits as a GPR.
        // This is another thing to be moved to a MachineFunction pass. Ideally we should check the instruction stream for writes to LR and reload it on exit.
        // For now, assume it is dirtied if the function is of any reasonable length.
        result.clobbers_x30 = result.instruction_count > 32;
        result.is_leaf = true;

        for (auto& bb : f)
        {
            for (auto& inst : bb)
            {
                if (auto ci = llvm::dyn_cast<llvm::CallInst>(&inst))
                {
                    if (llvm::isa<llvm::InlineAsm>(ci->getCalledOperand()))
                    {
                        // Inline ASM blocks are ignored
                        continue;
                    }

                    result.num_external_calls++;
                    if (ci->isTailCall())
                    {
                        // This is not a leaf if it has at least one exit point / terminator that is not a return instruction.
                        result.is_leaf = false;
                    }
                    else
                    {
                        // Returning calls always clobber x30
                        result.clobbers_x30 = true;
                    }
                }
            }
        }

        return result;
    }

    instruction_info_t GHC_frame_preservation_pass::decode_instruction(const llvm::Function& f, const llvm::Instruction* i)
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

    gpr GHC_frame_preservation_pass::get_base_register_for_call(const std::string& callee_name, gpr default_reg)
    {
        // We go over the base_register_lookup table and find the first matching pattern
        for (const auto& pattern : m_config.base_register_lookup)
        {
            if (callee_name.starts_with(pattern.first))
            {
                return pattern.second;
            }
        }

        return default_reg;
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
        if (m_visited_functions.find(this_name) != m_visited_functions.end())
        {
            // Already processed. Only useful when recursing which is currently not used.
            DPRINT("Function %s was already processed. Skipping.\n", this_name.c_str());
            return;
        }

        if (this_name != "__spu-null") // This name is meaningless and doesn't uniquely identify a function
        {
            m_visited_functions.insert(this_name);
        }

        if (m_config.exclusion_callback && m_config.exclusion_callback(this_name))
        {
            // Function is explicitly excluded
            return;
        }

        // Preprocessing.
        auto function_info = preprocess_function(f);
        if (function_info.num_external_calls == 0 && function_info.stack_frame_size == 0)
        {
            // No stack frame injection and no external calls to patch up. This is a leaf function, nothing to do.
            DPRINT("Ignoring function %s", this_name.c_str());
            return;
        }

        // Force tail calls on all terminators
        force_tail_call_terminators(f);

        // Check for leaves
        if (function_info.is_leaf && !m_config.use_stack_frames)
        {
            // Sanity check. If this function had no returning calls, it should have been omitted from processing.
            ensure(function_info.clobbers_x30, "Function has no terminator and no non-tail calls but was allowed for frame processing!");
            DPRINT("Function %s is a leaf.", this_name.c_str());
            process_leaf_function(irb, f);
            return;
        }

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

                    // We're about to make a tail call. This means after this call, we're supposed to return immediately. In that case, don't link, lower to branch only.
                    // Note that branches have some undesirable side-effects. For one, we lose the argument inputs, which the callee is expecting.
                    // This means we burn some cycles on every exit, but in return we do not require one instruction on the prologue + the ret chain is eliminated.
                    // No ret-chain also means two BBs can call each other indefinitely without running out of stack without relying on llvm to optimize that away.

                    std::string exit_fn;
                    auto ci = ensure(llvm::dyn_cast<llvm::CallInst>(original_inst));
                    auto operand_count = ci->getNumOperands() - 1; // The last operand is the callee, not a real operand
                    std::vector<std::string> constraints;
                    std::vector<llvm::Value*> args;

                    // We now load the callee args in reverse order to avoid self-clobbering of dependencies.
                    // FIXME: This is often times redundant and wastes cycles, we'll clean this up in a MachineFunction pass later.
                    int args_base_reg = instruction_info.callee_is_GHC ? aarch64::x19 : aarch64::x0; // GHC args are always x19..x25
                    for (auto i = static_cast<int>(operand_count) - 1; i >= 0; --i)
                    {
                        args.push_back(ci->getOperand(i));
                        exit_fn += fmt::format("mov x%d, $%u;\n", (args_base_reg + i), ::size32(args) - 1);
                        constraints.push_back("r");
                    }

                    // Restore LR to the exit gate if we think it may have been trampled.
                    if (function_info.clobbers_x30)
                    {
                        // Load the context "base" thread register to restore the link register from
                        auto context_base_reg = get_base_register_for_call(instruction_info.callee_name);
                        if (!instruction_info.callee_is_GHC)
                        {
                            // For non-GHC calls, we have to remap the arguments to x0...
                            context_base_reg = static_cast<gpr>(context_base_reg - 19);
                        }

                        // We want to do this after loading the arguments in case there was any spilling involved.
                        DPRINT("Patching call from %s to %s on register %d...",
                            this_name.c_str(),
                            instruction_info.callee_name.c_str(),
                            static_cast<int>(context_base_reg));

                        const auto x30_tail_restore = fmt::format(
                            "ldr x30, [x%u, #%u];\n",       // Load x30 from thread context
                            static_cast<u32>(context_base_reg),
                            m_config.hypervisor_context_offset);

                        exit_fn += x30_tail_restore;
                    }

                    // Stack cleanup. We need to do this last to allow the spiller to find it's own spilled variables.
                    if (function_info.stack_frame_size > 0)
                    {
                        exit_fn += frame_epilogue;
                    }

                    if (m_config.debug_info)
                    {
                        // Store x27 as our current address taking the place of LR (for debugging since bt is now useless)
                        // x28 and x29 are used as breadcrumb registers in this mode to form a pseudo-backtrace.
                        exit_fn +=
                            "mov x29, x28;\n"
                            "mov x28, x27;\n"
                            "adr x27, .;\n";
                    }

                    auto target = ensure(ci->getCalledOperand());
                    args.push_back(target);

                    if (instruction_info.is_indirect)
                    {
                        // NOTE: For indirect calls, we read the callee register before we load the operands
                        // If we don't do that the operands will overwrite our callee address if it lies in the x19-x25 range
                        // There is no safe temp register to stuff the call address to either, you just have to stuff it below sp and load it after operands are all assigned.
                        constraints.push_back("r");
                        exit_fn = fmt::format("str $%u, [sp, #-8];\n", operand_count) + exit_fn;
                        exit_fn +=
                            "ldr x15, [sp, #-8];\n"
                            "br x15;\n";
                    }
                    else
                    {
                        constraints.push_back("i");
                        exit_fn += fmt::format("b $%u;\n", operand_count);
                    }

                    // Emit the branch
                    llvm_asm(irb, exit_fn, args, fmt::merge(constraints, ","), f.getContext());

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

        if (!terminator_found)
        {
            // If we got here, we must be using stack frames.
            ensure(function_info.is_leaf && function_info.stack_frame_size > 0, "Leaf function was processed without using stack frames!");

            // We want to insert a frame cleanup at the tail at every return instruction we find.
            for (auto& bb : f)
            {
                for (auto& i : bb)
                {
                    if (is_ret_instruction(&i))
                    {
                        irb->SetInsertPoint(&i);
                        LLVM_ASM_VOID(frame_epilogue, irb, f.getContext());
                    }
                }
            }
        }
    }

    bool GHC_frame_preservation_pass::is_ret_instruction(const llvm::Instruction* i)
    {
        if (llvm::isa<llvm::ReturnInst>(i))
        {
            return true;
        }

        // Check for inline asm invoking "ret". This really shouldn't be a thing, but it is present in SPULLVMRecompiler for some reason.
        if (auto ci = llvm::dyn_cast<llvm::CallInst>(i))
        {
            if (auto asm_ = llvm::dyn_cast<llvm::InlineAsm>(ci->getCalledOperand()))
            {
                if (asm_->getAsmString() == "ret")
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool GHC_frame_preservation_pass::is_inlined_call(const llvm::CallInst* ci)
    {
        const auto callee = ci->getCalledFunction();
        if (!callee)
        {
            // Indirect BLR
            return false;
        }

        const std::string callee_name = callee->getName().str();
        if (callee_name.starts_with("llvm."))
        {
            // Intrinsic
            return true;
        }

        if (callee->hasFnAttribute(llvm::Attribute::AlwaysInline))
        {
            // Assume LLVM always obeys this
            return true;
        }

        return false;
    }

    void GHC_frame_preservation_pass::process_leaf_function(llvm::IRBuilder<>* irb, llvm::Function& f)
    {
        // Leaf functions have no true external calls.
        // After every external call, restore LR from the thread context register.
        bool restore_LR = false;
        gpr thread_context_reg = aarch64::x19;

        const auto restore_x30 = [&](llvm::Instruction* where)
        {
            const auto x30_tail_restore = fmt::format(
                "ldr x30, [x%u, #%u];\n",       // Load x30 from thread context
                static_cast<u32>(thread_context_reg),
                m_config.hypervisor_context_offset);

            ensure(where);
            irb->SetInsertPoint(where);
            LLVM_ASM_VOID(x30_tail_restore, irb, f.getContext());
        };

        for (auto &bb : f)
        {
            for (auto bit = bb.begin(); bit != bb.end();)
            {
                if (restore_LR)
                {
                    restore_x30(llvm::dyn_cast<llvm::Instruction>(bit));
                    restore_LR = false;
                }

                if (auto ci = llvm::dyn_cast<llvm::CallInst>(bit))
                {
                    // Returning call?
                    if (is_inlined_call(ci))
                    {
                        ++bit;
                        continue;
                    }

                    const auto callee = ci->getCalledFunction();
                    const std::string callee_name = callee ? callee->getName().str() : "__indirect";
                    auto base_reg = get_base_register_for_call(callee_name, m_config.base_register_lookup.empty() ? gpr::x19 : gpr::xzr);

                    // Check if the call is unexpected. We cannot guarantee that the reload is well-formed in such scenarios
                    if (base_reg == gpr::xzr)
                    {
                        if (callee)
                        {
                            // We don't know what we're dealing with here
                            DPRINT("Unexpected call to %s. We cannot guarantee sane codegen!", callee_name.c_str());
                        }

                        // Assume x19
                        base_reg = gpr::x19;
                    }

                    restore_LR = true;
                    thread_context_reg = base_reg;
                }

                if (m_config.debug_info && is_ret_instruction(llvm::dyn_cast<llvm::Instruction>(bit)))
                {
                    // We need to save the chain return point.
                    LLVM_ASM_VOID(
                        "mov x29, x28;\n"
                        "mov x28, x27;\n"
                        "adr x27, .;\n",
                        irb, f.getContext());
                }

                if (bit != bb.end())
                {
                    ++bit;
                }
            }
        }

        ensure(!restore_LR, "Dangling function call found");
    }
}
