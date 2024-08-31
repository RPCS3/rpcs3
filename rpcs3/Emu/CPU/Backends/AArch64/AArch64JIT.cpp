#include "stdafx.h"

#include "AArch64JIT.h"
#include "AArch64ASM.h"

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
        ASMBlock frame_prologue, frame_epilogue;

        if (function_info.stack_frame_size > 0)
        {
            // NOTE: The stack frame here is purely optional, we can pre-allocate scratch on the gateway.
            // However, that is an optimization for another time, this helps make debugging easier.
            frame_prologue.sub(sp, sp, UASM::Imm(function_info.stack_frame_size));
            frame_epilogue.add(sp, sp, UASM::Imm(function_info.stack_frame_size));

            // Emit the frame prologue. We use a BB here for extra safety as it solves the problem of backwards jumps re-executing the prologue.
            auto functionStart = &f.front();
            auto prologueBB = llvm::BasicBlock::Create(f.getContext(), "", &f, functionStart);
            irb->SetInsertPoint(prologueBB, prologueBB->begin());
            frame_prologue.insert(irb, f.getContext());
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

                if (instruction_info.preserve_stack)
                {
                    // Non-tail call. If we have a stack allocated, we preserve it across the call
                    ++bit;
                    continue;
                }

                ensure(instruction_info.is_tail_call);
                terminator_found = true;

                // Now we patch the call if required. For normal calls that 'return' (i.e calls to C/C++ ABI), we do not patch them as they will manage the stack themselves (callee-managed)
                bit = patch_tail_call(irb, f, bit, instruction_info, function_info, frame_epilogue);

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
                        frame_epilogue.insert(irb, f.getContext());
                    }
                }
            }
        }
    }

    llvm::BasicBlock::iterator
    GHC_frame_preservation_pass::patch_tail_call(
        llvm::IRBuilder<>* irb,
        llvm::Function& f,
        llvm::BasicBlock::iterator where,
        const instruction_info_t& instruction_info,
        const function_info_t& function_info,
        const UASM& frame_epilogue)
    {
        auto ci = llvm::dyn_cast<llvm::CallInst>(where);
        irb->SetInsertPoint(ensure(ci));

        const auto this_name = f.getName().str();

        // Insert breadcrumb info before the call
        // WARNING: This can corrupt the call because LLVM somehow ignores the clobbered register during a call instruction for some reason
        // In case of a blr on x27..x29 you can end up corrupting the binary, but it is invaluable for debugging.
        // Debug frames are disabled in shipping code so this is not a big deal.
        if (m_config.debug_info)
        {
            // Call-chain tracing
            ASMBlock c;
            c.mov(x29, x28);
            c.mov(x28, x27);
            c.adr(x27, UASM::Reg(pc));
            c.insert(irb, f.getContext());
        }

        // Clean up any injected frames before the call
        if (function_info.stack_frame_size > 0)
        {
            frame_epilogue.insert(irb, f.getContext());
        }

        // Insert the next piece after the call, before the ret
        ++where;
        ensure(llvm::isa<llvm::ReturnInst>(where));
        irb->SetInsertPoint(llvm::dyn_cast<llvm::Instruction>(where));

        if (instruction_info.callee_is_GHC &&                    // Calls to C++ ABI will always return
            !instruction_info.is_indirect &&                     // We don't know enough when calling indirectly to know if we'll return or not
            !is_faux_function(instruction_info.callee_name))     // Ignore branch patch-points and imposter functions. Their behavior is unreliable.
        {
            // We're making a one-way call. This branch shouldn't even bother linking as it will never return here.
            ASMBlock c;
            c.brk(0x99);
            c.insert(irb, f.getContext());
            return where;
        }

        // Patch the return path. No GHC call shall ever return to another. If we reach the function endpoint, immediately abort to GW
        auto thread_base_reg = get_base_register_for_call(f.getName().str());
        auto arg_index = static_cast<int>(thread_base_reg) - static_cast<int>(x19);
        ASMBlock c;

        auto thread_arg = ensure(f.getArg(arg_index)); // Guaranteed to hold our original 'thread'
        c.mov(x30, UASM::Var(thread_arg));
        c.ldr(x30, x30, UASM::Imm(m_config.hypervisor_context_offset));
        c.insert(irb, f.getContext());

        // Next
        return where;
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

    bool GHC_frame_preservation_pass::is_faux_function(const std::string& function_name)
    {
        // Is it a branch patch-point?
        if (function_name.find("-pp-") != umax)
        {
            return true;
        }

        // Now we search the known imposters list
        if (m_config.faux_function_list.empty())
        {
            return false;
        }

        const auto& x = m_config.faux_function_list;
        return std::find(x.begin(), x.end(), function_name) != x.end();
    }

    void GHC_frame_preservation_pass::process_leaf_function(llvm::IRBuilder<>* irb, llvm::Function& f)
    {
        for (auto &bb : f)
        {
            for (auto bit = bb.begin(); bit != bb.end();)
            {
                auto i = llvm::dyn_cast<llvm::Instruction>(bit);
                if (!is_ret_instruction(i))
                {
                    ++bit;
                    continue;
                }

                // Insert sequence before the return
                irb->SetInsertPoint(llvm::dyn_cast<llvm::Instruction>(bit));

                if (m_config.debug_info)
                {
                    // We need to save the chain return point.
                    ASMBlock c;
                    c.mov(x29, x28);
                    c.mov(x28, x27);
                    c.adr(x27, UASM::Reg(pc));
                    c.insert(irb, f.getContext());
                }

                // Now we need to reload LR. We abuse the function's caller arg set for this to avoid messing with regs too much
                auto thread_base_reg = get_base_register_for_call(f.getName().str());
                auto arg_index = static_cast<int>(thread_base_reg) - static_cast<int>(x19);
                ASMBlock c;

                auto thread_arg = ensure(f.getArg(arg_index)); // Guaranteed to hold our original 'thread'
                c.mov(x30, UASM::Var(thread_arg));
                c.ldr(x30, x30, UASM::Imm(m_config.hypervisor_context_offset));
                c.insert(irb, f.getContext());

                if (bit != bb.end())
                {
                    ++bit;
                }
            }
        }
    }
}
