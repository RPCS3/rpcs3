#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
#include "Emu/Memory/Memory.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/CodeGen/MachineCodeInfo.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/IR/Verifier.h"

using namespace llvm;
using namespace ppu_recompiler_llvm;

u64  Compiler::s_rotate_mask[64][64];
bool Compiler::s_rotate_mask_inited = false;

Compiler::Compiler(RecompilationEngine & recompilation_engine, const Executable execute_unknown_function, const Executable execute_unknown_block)
    : m_recompilation_engine(recompilation_engine) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetDisassembler();

    m_llvm_context = new LLVMContext();
    m_ir_builder   = new IRBuilder<>(*m_llvm_context);
    m_module       = new llvm::Module("Module", *m_llvm_context);
    m_fpm          = new FunctionPassManager(m_module);

    EngineBuilder engine_builder(m_module);
    engine_builder.setMCPU(sys::getHostCPUName());
    engine_builder.setEngineKind(EngineKind::JIT);
    engine_builder.setOptLevel(CodeGenOpt::Default);
    m_execution_engine = engine_builder.create();

    m_fpm->add(new DataLayoutPass(m_module));
    m_fpm->add(createNoAAPass());
    m_fpm->add(createBasicAliasAnalysisPass());
    m_fpm->add(createNoTargetTransformInfoPass());
    m_fpm->add(createEarlyCSEPass());
    m_fpm->add(createTailCallEliminationPass());
    m_fpm->add(createReassociatePass());
    m_fpm->add(createInstructionCombiningPass());
    m_fpm->add(new DominatorTreeWrapperPass());
    m_fpm->add(new MemoryDependenceAnalysis());
    m_fpm->add(createGVNPass());
    m_fpm->add(createInstructionCombiningPass());
    m_fpm->add(new MemoryDependenceAnalysis());
    m_fpm->add(createDeadStoreEliminationPass());
    m_fpm->add(new LoopInfo());
    m_fpm->add(new ScalarEvolution());
    m_fpm->add(createSLPVectorizerPass());
    m_fpm->add(createInstructionCombiningPass());
    m_fpm->add(createCFGSimplificationPass());
    m_fpm->doInitialization();

    std::vector<Type *> arg_types;
    arg_types.push_back(m_ir_builder->getInt8PtrTy());
    arg_types.push_back(m_ir_builder->getInt8PtrTy());
    arg_types.push_back(m_ir_builder->getInt64Ty());
    m_compiled_function_type = FunctionType::get(m_ir_builder->getInt32Ty(), arg_types, false);

    m_execute_unknown_function = (Function *)m_module->getOrInsertFunction("execute_unknown_function", m_compiled_function_type);
    m_execute_unknown_function->setCallingConv(CallingConv::X86_64_Win64);
    m_execution_engine->addGlobalMapping(m_execute_unknown_function, (void *)execute_unknown_function);

    m_execute_unknown_block = (Function *)m_module->getOrInsertFunction("execute_unknown_block", m_compiled_function_type);
    m_execute_unknown_block->setCallingConv(CallingConv::X86_64_Win64);
    m_execution_engine->addGlobalMapping(m_execute_unknown_block, (void *)execute_unknown_block);

    if (!s_rotate_mask_inited) {
        InitRotateMask();
        s_rotate_mask_inited = true;
    }
}

Compiler::~Compiler() {
    delete m_execution_engine;
    delete m_fpm;
    delete m_ir_builder;
    delete m_llvm_context;
}

Executable Compiler::Compile(const std::string & name, const ControlFlowGraph & cfg, bool inline_all, bool generate_linkable_exits) {
    auto compilation_start = std::chrono::high_resolution_clock::now();

    m_state.cfg                     = &cfg;
    m_state.inline_all              = inline_all;
    m_state.generate_linkable_exits = generate_linkable_exits;

    // Create the function
    m_state.function = (Function *)m_module->getOrInsertFunction(name, m_compiled_function_type);
    m_state.function->setCallingConv(CallingConv::X86_64_Win64);
    auto arg_i = m_state.function->arg_begin();
    arg_i->setName("ppu_state");
    m_state.args[CompileTaskState::Args::State] = arg_i;
    (++arg_i)->setName("context");
    m_state.args[CompileTaskState::Args::Context] = arg_i;

    // Create the entry block and add code to branch to the first instruction
    m_ir_builder->SetInsertPoint(GetBasicBlockFromAddress(0));
    m_ir_builder->CreateBr(GetBasicBlockFromAddress(cfg.start_address));

    // Convert each instruction in the CFG to LLVM IR
    std::vector<PHINode *> exit_instr_list;
    for (auto instr_i = cfg.instruction_addresses.begin(); instr_i != cfg.instruction_addresses.end(); instr_i++) {
        m_state.hit_branch_instruction      = false;
        m_state.current_instruction_address = *instr_i;
        auto instr_bb                       = GetBasicBlockFromAddress(m_state.current_instruction_address);
        m_ir_builder->SetInsertPoint(instr_bb);

        if (!inline_all && *instr_i != cfg.start_address) {
            // Use an already compiled implementation of this block if available
            auto ordinal = m_recompilation_engine.GetOrdinal(*instr_i);
            if (ordinal != 0xFFFFFFFF) {
                auto exit_instr_i32 = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 0);
                exit_instr_list.push_back(exit_instr_i32);

                auto context_i64 = m_ir_builder->CreateZExt(exit_instr_i32, m_ir_builder->getInt64Ty());
                context_i64      = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
                auto ret_i32     = IndirectCall(*instr_i, context_i64, false);

                auto switch_instr = m_ir_builder->CreateSwitch(ret_i32, GetBasicBlockFromAddress(0xFFFFFFFF));
                auto branch_i     = cfg.branches.find(*instr_i);
                if (branch_i != cfg.branches.end()) {
                    for (auto next_instr_i = branch_i->second.begin(); next_instr_i != branch_i->second.end(); next_instr_i++) {
                        switch_instr->addCase(m_ir_builder->getInt32(*next_instr_i), GetBasicBlockFromAddress(*next_instr_i));
                    }
                }
            }
        }

        if (instr_bb->empty()) {
            u32 instr = re32(vm::get_ref<u32>(m_state.current_instruction_address));
            Decode(instr);
            if (!m_state.hit_branch_instruction) {
                m_ir_builder->CreateBr(GetBasicBlockFromAddress(m_state.current_instruction_address + 4));
            }
        }
    }

    // Generate exit logic for all empty blocks
    auto default_exit_block_name = GetBasicBlockNameFromAddress(0xFFFFFFFF);
    for (auto block_i = m_state.function->begin(); block_i != m_state.function->end(); block_i++) {
        if (!block_i->getInstList().empty() || block_i->getName() == default_exit_block_name) {
            continue;
        }

        // Found an empty block
        m_ir_builder->SetInsertPoint(block_i);
        auto exit_instr_i32 = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 0);
        exit_instr_list.push_back(exit_instr_i32);

        auto instr_address = GetAddressFromBasicBlockName(block_i->getName());
        SetPc(m_ir_builder->getInt32(instr_address));

        if (generate_linkable_exits) {
            auto context_i64 = m_ir_builder->CreateZExt(exit_instr_i32, m_ir_builder->getInt64Ty());
            context_i64      = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
            auto ret_i32     = IndirectCall(instr_address, context_i64, false);
            auto cmp_i1      = m_ir_builder->CreateICmpNE(ret_i32, m_ir_builder->getInt32(0));
            auto then_bb     = GetBasicBlockFromAddress(instr_address, "then");
            auto merge_bb    = GetBasicBlockFromAddress(instr_address, "merge");
            m_ir_builder->CreateCondBr(cmp_i1, then_bb, merge_bb);

            m_ir_builder->SetInsertPoint(then_bb);
            context_i64 = m_ir_builder->CreateZExt(ret_i32, m_ir_builder->getInt64Ty());
            context_i64 = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
            m_ir_builder->CreateCall2(m_execute_unknown_block, m_state.args[CompileTaskState::Args::State], context_i64);
            m_ir_builder->CreateBr(merge_bb);

            m_ir_builder->SetInsertPoint(merge_bb);
            m_ir_builder->CreateRet(m_ir_builder->getInt32(0));
        } else {
            m_ir_builder->CreateRet(exit_instr_i32);
        }
    }

    // If the function has a default exit block then generate code for it
    auto default_exit_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "", false);
    if (default_exit_bb) {
        m_ir_builder->SetInsertPoint(default_exit_bb);
        auto exit_instr_i32 = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 0);
        exit_instr_list.push_back(exit_instr_i32);

        if (generate_linkable_exits) {
            auto cmp_i1   = m_ir_builder->CreateICmpNE(exit_instr_i32, m_ir_builder->getInt32(0));
            auto then_bb  = GetBasicBlockFromAddress(0xFFFFFFFF, "then");
            auto merge_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "merge");
            m_ir_builder->CreateCondBr(cmp_i1, then_bb, merge_bb);

            m_ir_builder->SetInsertPoint(then_bb);
            auto context_i64 = m_ir_builder->CreateZExt(exit_instr_i32, m_ir_builder->getInt64Ty());
            context_i64      = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
            m_ir_builder->CreateCall2(m_execute_unknown_block, m_state.args[CompileTaskState::Args::State], context_i64);
            m_ir_builder->CreateBr(merge_bb);

            m_ir_builder->SetInsertPoint(merge_bb);
            m_ir_builder->CreateRet(m_ir_builder->getInt32(0));
        } else {
            m_ir_builder->CreateRet(exit_instr_i32);
        }
    }

    // Add incoming values for all exit instr PHI nodes
    for (auto exit_instr_i = exit_instr_list.begin(); exit_instr_i != exit_instr_list.end(); exit_instr_i++) {
        auto block = (*exit_instr_i)->getParent();
        for (auto pred_i = pred_begin(block); pred_i != pred_end(block); pred_i++) {
            auto pred_address = GetAddressFromBasicBlockName((*pred_i)->getName());
            (*exit_instr_i)->addIncoming(m_ir_builder->getInt32(pred_address), *pred_i);
        }
    }

#ifdef _DEBUG
    m_recompilation_engine.Log() << *m_state.function;

    std::string        verify;
    raw_string_ostream verify_ostream(verify);
    if (verifyFunction(*m_state.function, &verify_ostream)) {
        m_recompilation_engine.Log() << "Verification failed: " << verify << "\n";
    }
#endif

    auto ir_build_end      = std::chrono::high_resolution_clock::now();
    m_stats.ir_build_time += std::chrono::duration_cast<std::chrono::nanoseconds>(ir_build_end - compilation_start);

    // Optimize this function
    m_fpm->run(*m_state.function);
    auto optimize_end          = std::chrono::high_resolution_clock::now();
    m_stats.optimization_time += std::chrono::duration_cast<std::chrono::nanoseconds>(optimize_end - ir_build_end);

    // Translate to machine code
    MachineCodeInfo mci;
    m_execution_engine->runJITOnFunction(m_state.function, &mci);
    auto translate_end        = std::chrono::high_resolution_clock::now();
    m_stats.translation_time += std::chrono::duration_cast<std::chrono::nanoseconds>(translate_end - optimize_end);

#ifdef _DEBUG
    m_recompilation_engine.Log() << "\nDisassembly:\n";
    auto disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);
    for (size_t pc = 0; pc < mci.size();) {
        char str[1024];

        auto size = LLVMDisasmInstruction(disassembler, ((u8 *)mci.address()) + pc, mci.size() - pc, (uint64_t)(((u8 *)mci.address()) + pc), str, sizeof(str));
        m_recompilation_engine.Log() << fmt::Format("0x%08X: ", (u64)(((u8 *)mci.address()) + pc)) << str << '\n';
        pc += size;
    }

    LLVMDisasmDispose(disassembler);
#endif

    auto compilation_end  = std::chrono::high_resolution_clock::now();
    m_stats.total_time   += std::chrono::duration_cast<std::chrono::nanoseconds>(compilation_end - compilation_start);

    return (Executable)mci.address();
}

void Compiler::FreeExecutable(const std::string & name) {
    auto function = m_module->getFunction(name);
    if (function) {
        m_execution_engine->freeMachineCodeForFunction(function);
        function->eraseFromParent();
    }
}

Compiler::Stats Compiler::GetStats() {
    return m_stats;
}

void Compiler::Decode(const u32 code) {
    (*PPU_instr::main_list)(this, code);
}

void Compiler::NULL_OP() {
    CompilationError("NULL_OP");
}

void Compiler::NOP() {
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::TDI(u32 to, u32 ra, s32 simm16) {
    CompilationError("TDI");
}

void Compiler::TWI(u32 to, u32 ra, s32 simm16) {
    CompilationError("TWI");
}

void Compiler::MFVSCR(u32 vd) {
    auto vscr_i32  = GetVscr();
    auto vscr_i128 = m_ir_builder->CreateZExt(vscr_i32, m_ir_builder->getIntNTy(128));
    SetVr(vd, vscr_i128);
}

void Compiler::MTVSCR(u32 vb) {
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);
    auto vscr_i32 = m_ir_builder->CreateExtractElement(vb_v4i32, m_ir_builder->getInt32(0));
    vscr_i32      = m_ir_builder->CreateAnd(vscr_i32, 0x00010001);
    SetVscr(vscr_i32);
}

void Compiler::VADDCUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    va_v4i32      = m_ir_builder->CreateNot(va_v4i32);
    auto cmpv4i1  = m_ir_builder->CreateICmpULT(va_v4i32, vb_v4i32);
    auto cmpv4i32 = m_ir_builder->CreateZExt(cmpv4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmpv4i32);
}

void Compiler::VADDFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto sum_v4f32 = m_ir_builder->CreateFAdd(va_v4f32, vb_v4f32);
    SetVr(vd, sum_v4f32);
}

void Compiler::VADDSBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_padds_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set VSCR.SAT
}

void Compiler::VADDSHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_padds_w), va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);

    // TODO: Set VSCR.SAT
}

void Compiler::VADDSWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    // It looks like x86 does not have an instruction to add 32 bit intergers with signed/unsigned saturation.
    // To implement add with saturation, we first determine what the result would be if the operation were to cause
    // an overflow. If two -ve numbers are being added and cause an overflow, the result would be 0x80000000.
    // If two +ve numbers are being added and cause an overflow, the result would be 0x7FFFFFFF. Addition of a -ve
    // number and a +ve number cannot cause overflow. So the result in case of an overflow is 0x7FFFFFFF + sign bit
    // of any one of the operands.
    auto tmp1_v4i32 = m_ir_builder->CreateLShr(va_v4i32, 31);
    tmp1_v4i32      = m_ir_builder->CreateAdd(tmp1_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x7FFFFFFF)));
    auto tmp1_v16i8 = m_ir_builder->CreateBitCast(tmp1_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // Next, we find if the addition can actually result in an overflow. Since an overflow can only happen if the operands
    // have the same sign, we bitwise XOR both the operands. If the sign bit of the result is 0 then the operands have the
    // same sign and so may cause an overflow. We invert the result so that the sign bit is 1 when the operands have the
    // same sign.
    auto tmp2_v4i32 = m_ir_builder->CreateXor(va_v4i32, vb_v4i32);
    tmp2_v4i32      = m_ir_builder->CreateNot(tmp2_v4i32);

    // Perform the sum.
    auto sum_v4i32 = m_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    auto sum_v16i8 = m_ir_builder->CreateBitCast(sum_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // If an overflow occurs, then the sign of the sum will be different from the sign of the operands. So, we xor the
    // result with one of the operands. The sign bit of the result will be 1 if the sign bit of the sum and the sign bit of the
    // result is different. This result is again ANDed with tmp3 (the sign bit of tmp3 is 1 only if the operands have the same
    // sign and so can cause an overflow).
    auto tmp3_v4i32 = m_ir_builder->CreateXor(va_v4i32, sum_v4i32);
    tmp3_v4i32      = m_ir_builder->CreateAnd(tmp2_v4i32, tmp3_v4i32);
    tmp3_v4i32      = m_ir_builder->CreateAShr(tmp3_v4i32, 31);
    auto tmp3_v16i8 = m_ir_builder->CreateBitCast(tmp3_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // tmp4 is equal to 0xFFFFFFFF if an overflow occured and 0x00000000 otherwise.
    auto res_v16i8 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pblendvb), sum_v16i8, tmp1_v16i8, tmp3_v16i8);
    SetVr(vd, res_v16i8);

    // TODO: Set SAT
}

void Compiler::VADDUBM(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateAdd(va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);
}

void Compiler::VADDUBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_paddus_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set SAT
}

void Compiler::VADDUHM(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateAdd(va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);
}

void Compiler::VADDUHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_paddus_w), va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);

    // TODO: Set SAT
}

void Compiler::VADDUWM(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = m_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    SetVr(vd, sum_v4i32);
}

void Compiler::VADDUWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = m_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpULT(sum_v4i32, va_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto res_v4i32 = m_ir_builder->CreateOr(sum_v4i32, cmp_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set SAT
}

void Compiler::VAND(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VANDC(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    vb_v4i32       = m_ir_builder->CreateNot(vb_v4i32);
    auto res_v4i32 = m_ir_builder->CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VAVGSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto va_v16i16  = m_ir_builder->CreateSExt(va_v16i8, VectorType::get(m_ir_builder->getInt16Ty(), 16));
    auto vb_v16i16  = m_ir_builder->CreateSExt(vb_v16i8, VectorType::get(m_ir_builder->getInt16Ty(), 16));
    auto sum_v16i16 = m_ir_builder->CreateAdd(va_v16i16, vb_v16i16);
    sum_v16i16      = m_ir_builder->CreateAdd(sum_v16i16, m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt16(1)));
    auto avg_v16i16 = m_ir_builder->CreateAShr(sum_v16i16, 1);
    auto avg_v16i8  = m_ir_builder->CreateTrunc(avg_v16i16, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, avg_v16i8);
}

void Compiler::VAVGSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto va_v8i32  = m_ir_builder->CreateSExt(va_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vb_v8i32  = m_ir_builder->CreateSExt(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto sum_v8i32 = m_ir_builder->CreateAdd(va_v8i32, vb_v8i32);
    sum_v8i32      = m_ir_builder->CreateAdd(sum_v8i32, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt32(1)));
    auto avg_v8i32 = m_ir_builder->CreateAShr(sum_v8i32, 1);
    auto avg_v8i16 = m_ir_builder->CreateTrunc(avg_v8i32, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, avg_v8i16);
}

void Compiler::VAVGSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto va_v4i64  = m_ir_builder->CreateSExt(va_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto vb_v4i64  = m_ir_builder->CreateSExt(vb_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto sum_v4i64 = m_ir_builder->CreateAdd(va_v4i64, vb_v4i64);
    sum_v4i64      = m_ir_builder->CreateAdd(sum_v4i64, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(1)));
    auto avg_v4i64 = m_ir_builder->CreateAShr(sum_v4i64, 1);
    auto avg_v4i32 = m_ir_builder->CreateTrunc(avg_v4i64, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, avg_v4i32);
}

void Compiler::VAVGUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto avg_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pavg_b), va_v16i8, vb_v16i8);
    SetVr(vd, avg_v16i8);
}

void Compiler::VAVGUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto avg_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pavg_w), va_v8i16, vb_v8i16);
    SetVr(vd, avg_v8i16);
}

void Compiler::VAVGUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto va_v4i64  = m_ir_builder->CreateZExt(va_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto vb_v4i64  = m_ir_builder->CreateZExt(vb_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto sum_v4i64 = m_ir_builder->CreateAdd(va_v4i64, vb_v4i64);
    sum_v4i64      = m_ir_builder->CreateAdd(sum_v4i64, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(1)));
    auto avg_v4i64 = m_ir_builder->CreateLShr(sum_v4i64, 1);
    auto avg_v4i32 = m_ir_builder->CreateTrunc(avg_v4i64, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, avg_v4i32);
}

void Compiler::VCFSX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder->CreateSIToFP(vb_v4i32, VectorType::get(m_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale = (float)((u64)1 << uimm5);
        res_v4f32   = m_ir_builder->CreateFDiv(res_v4f32, m_ir_builder->CreateVectorSplat(4, ConstantFP::get(m_ir_builder->getFloatTy(), scale)));
    }

    SetVr(vd, res_v4f32);
}

void Compiler::VCFUX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder->CreateUIToFP(vb_v4i32, VectorType::get(m_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale = (float)((u64)1 << uimm5);
        res_v4f32   = m_ir_builder->CreateFDiv(res_v4f32, m_ir_builder->CreateVectorSplat(4, ConstantFP::get(m_ir_builder->getFloatTy(), scale)));
    }

    SetVr(vd, res_v4f32);
}

void Compiler::VCMPBFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32     = GetVrAsFloatVec(va);
    auto vb_v4f32     = GetVrAsFloatVec(vb);
    auto cmp_gt_v4i1  = m_ir_builder->CreateFCmpOGT(va_v4f32, vb_v4f32);
    vb_v4f32          = m_ir_builder->CreateFNeg(vb_v4f32);
    auto cmp_lt_v4i1  = m_ir_builder->CreateFCmpOLT(va_v4f32, vb_v4f32);
    auto cmp_gt_v4i32 = m_ir_builder->CreateZExt(cmp_gt_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto cmp_lt_v4i32 = m_ir_builder->CreateZExt(cmp_lt_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    cmp_gt_v4i32      = m_ir_builder->CreateShl(cmp_gt_v4i32, 31);
    cmp_lt_v4i32      = m_ir_builder->CreateShl(cmp_lt_v4i32, 30);
    auto res_v4i32    = m_ir_builder->CreateOr(cmp_gt_v4i32, cmp_lt_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Implement NJ mode
}

void Compiler::VCMPBFP_(u32 vd, u32 va, u32 vb) {
    VCMPBFP(vd, va, vb);

    auto vd_v16i8       = GetVrAsIntVec(vd, 8);
    u32 mask_v16i32[16] = {3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vd_v16i8            = m_ir_builder->CreateShuffleVector(vd_v16i8, UndefValue::get(VectorType::get(m_ir_builder->getInt8Ty(), 16)), ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    auto vd_v4i32       = m_ir_builder->CreateBitCast(vd_v16i8, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto vd_mask_i32    = m_ir_builder->CreateExtractElement(vd_v4i32, m_ir_builder->getInt32(0));
    auto cmp_i1         = m_ir_builder->CreateICmpEQ(vd_mask_i32, m_ir_builder->getInt32(0));
    SetCrField(6, nullptr, nullptr, cmp_i1, nullptr);
}

void Compiler::VCMPEQFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOEQ(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void Compiler::VCMPEQFP_(u32 vd, u32 va, u32 vb) {
    VCMPEQFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPEQUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpEQ(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void Compiler::VCMPEQUB_(u32 vd, u32 va, u32 vb) {
    VCMPEQUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPEQUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpEQ(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void Compiler::VCMPEQUH_(u32 vd, u32 va, u32 vb) {
    VCMPEQUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPEQUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpEQ(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void Compiler::VCMPEQUW_(u32 vd, u32 va, u32 vb) {
    VCMPEQUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPGEFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOGE(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void Compiler::VCMPGEFP_(u32 vd, u32 va, u32 vb) {
    VCMPGEFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPGTFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOGT(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void Compiler::VCMPGTFP_(u32 vd, u32 va, u32 vb) {
    VCMPGTFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPGTSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpSGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void Compiler::VCMPGTSB_(u32 vd, u32 va, u32 vb) {
    VCMPGTSB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPGTSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpSGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void Compiler::VCMPGTSH_(u32 vd, u32 va, u32 vb) {
    VCMPGTSH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPGTSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpSGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void Compiler::VCMPGTSW_(u32 vd, u32 va, u32 vb) {
    VCMPGTSW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPGTUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpUGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void Compiler::VCMPGTUB_(u32 vd, u32 va, u32 vb) {
    VCMPGTUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPGTUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpUGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void Compiler::VCMPGTUH_(u32 vd, u32 va, u32 vb) {
    VCMPGTUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCMPGTUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpUGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void Compiler::VCMPGTUW_(u32 vd, u32 va, u32 vb) {
    VCMPGTUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void Compiler::VCTSXS(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4f32 = GetVrAsFloatVec(vb);
    if (uimm5) {
        vb_v4f32 = m_ir_builder->CreateFMul(vb_v4f32, m_ir_builder->CreateVectorSplat(4, ConstantFP::get(m_ir_builder->getFloatTy(), 1 << uimm5)));
    }

    auto res_v4i32 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_cvtps2dq), vb_v4f32);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOGE(vb_v4f32, m_ir_builder->CreateVectorSplat(4, ConstantFP::get(m_ir_builder->getFloatTy(), 0x7FFFFFFF)));
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    res_v4i32      = m_ir_builder->CreateXor(cmp_v4i32, res_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set VSCR.SAT
}

void Compiler::VCTUXS(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4f32 = GetVrAsFloatVec(vb);
    if (uimm5) {
        vb_v4f32 = m_ir_builder->CreateFMul(vb_v4f32, m_ir_builder->CreateVectorSplat(4, ConstantFP::get(m_ir_builder->getFloatTy(), 1 << uimm5)));
    }

    auto res_v4f32 = (Value *)m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse_max_ps), vb_v4f32, m_ir_builder->CreateVectorSplat(4, ConstantFP::get(m_ir_builder->getFloatTy(), 0)));
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOGE(res_v4f32, m_ir_builder->CreateVectorSplat(4, ConstantFP::get(m_ir_builder->getFloatTy(), 0xFFFFFFFFu)));
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto res_v4i32 = m_ir_builder->CreateFPToUI(res_v4f32, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    res_v4i32      = m_ir_builder->CreateOr(res_v4i32, cmp_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set VSCR.SAT
}

void Compiler::VEXPTEFP(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = (Value *)m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::pow, VectorType::get(m_ir_builder->getFloatTy(), 4)),
                                                        m_ir_builder->CreateVectorSplat(4, ConstantFP::get(m_ir_builder->getFloatTy(), 2.0f)), vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VLOGEFP(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::log2, VectorType::get(m_ir_builder->getFloatTy(), 4)), vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto vc_v4f32  = GetVrAsFloatVec(vc);
    auto res_v4f32 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, VectorType::get(m_ir_builder->getFloatTy(), 4)), va_v4f32, vc_v4f32, vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VMAXFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse_max_ps), va_v4f32, vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VMAXSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pmaxsb), va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VMAXSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmaxs_w), va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMAXSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pmaxsd), va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VMAXUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmaxu_b), va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VMAXUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pmaxuw), va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMAXUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pmaxud), va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto vc_v8i16  = GetVrAsIntVec(vc, 16);
    auto va_v8i32  = m_ir_builder->CreateSExt(va_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vb_v8i32  = m_ir_builder->CreateSExt(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vc_v8i32  = m_ir_builder->CreateSExt(vc_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto res_v8i32 = m_ir_builder->CreateMul(va_v8i32, vb_v8i32);
    res_v8i32      = m_ir_builder->CreateAShr(res_v8i32, 15);
    res_v8i32      = m_ir_builder->CreateAdd(res_v8i32, vc_v8i32);

    u32 mask1_v4i32[4] = {0, 1, 2, 3};
    auto res1_v4i32    = m_ir_builder->CreateShuffleVector(res_v8i32, UndefValue::get(VectorType::get(m_ir_builder->getInt32Ty(), 8)), ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    u32 mask2_v4i32[4] = {4, 5, 6, 7};
    auto res2_v4i32    = m_ir_builder->CreateShuffleVector(res_v8i32, UndefValue::get(VectorType::get(m_ir_builder->getInt32Ty(), 8)), ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));
    auto res_v8i16     = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_packssdw_128), res1_v4i32, res2_v4i32);
    SetVr(vd, res_v8i16);

    // TODO: Set VSCR.SAT
}

void Compiler::VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto vc_v8i16  = GetVrAsIntVec(vc, 16);
    auto va_v8i32  = m_ir_builder->CreateSExt(va_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vb_v8i32  = m_ir_builder->CreateSExt(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vc_v8i32  = m_ir_builder->CreateSExt(vc_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto res_v8i32 = m_ir_builder->CreateMul(va_v8i32, vb_v8i32);
    res_v8i32      = m_ir_builder->CreateAdd(res_v8i32, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt32(0x4000)));
    res_v8i32      = m_ir_builder->CreateAShr(res_v8i32, 15);
    res_v8i32      = m_ir_builder->CreateAdd(res_v8i32, vc_v8i32);

    u32 mask1_v4i32[4] = {0, 1, 2, 3};
    auto res1_v4i32    = m_ir_builder->CreateShuffleVector(res_v8i32, UndefValue::get(VectorType::get(m_ir_builder->getInt32Ty(), 8)), ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    u32 mask2_v4i32[4] = {4, 5, 6, 7};
    auto res2_v4i32    = m_ir_builder->CreateShuffleVector(res_v8i32, UndefValue::get(VectorType::get(m_ir_builder->getInt32Ty(), 8)), ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));
    auto res_v8i16     = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_packssdw_128), res1_v4i32, res2_v4i32);
    SetVr(vd, res_v8i16);

    // TODO: Set VSCR.SAT
}

void Compiler::VMINFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse_min_ps), va_v4f32, vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VMINSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminsb), va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VMINSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmins_w), va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMINSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminsd), va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VMINUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pminu_b), va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VMINUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminuw), va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMINUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminud), va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto vc_v8i16  = GetVrAsIntVec(vc, 16);
    auto res_v8i16 = m_ir_builder->CreateMul(va_v8i16, vb_v8i16);
    res_v8i16      = m_ir_builder->CreateAdd(res_v8i16, vc_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMRGHB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8        = GetVrAsIntVec(va, 8);
    auto vb_v16i8        = GetVrAsIntVec(vb, 8);
    u32  mask_v16i32[16] = {24, 8, 25, 9, 26, 10, 27, 11, 28, 12, 29, 13, 30, 14, 31, 15};
    auto vd_v16i8        = m_ir_builder->CreateShuffleVector(va_v16i8, vb_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    SetVr(vd, vd_v16i8);
}

void Compiler::VMRGHH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16      = GetVrAsIntVec(va, 16);
    auto vb_v8i16      = GetVrAsIntVec(vb, 16);
    u32  mask_v8i32[8] = {12, 4, 13, 5, 14, 6, 15, 7};
    auto vd_v8i16      = m_ir_builder->CreateShuffleVector(va_v8i16, vb_v8i16, ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));
    SetVr(vd, vd_v8i16);
}

void Compiler::VMRGHW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32      = GetVrAsIntVec(va, 32);
    auto vb_v4i32      = GetVrAsIntVec(vb, 32);
    u32  mask_v4i32[4] = {6, 2, 7, 3};
    auto vd_v4i32      = m_ir_builder->CreateShuffleVector(va_v4i32, vb_v4i32, ConstantDataVector::get(m_ir_builder->getContext(), mask_v4i32));
    SetVr(vd, vd_v4i32);
}

void Compiler::VMRGLB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8        = GetVrAsIntVec(va, 8);
    auto vb_v16i8        = GetVrAsIntVec(vb, 8);
    u32  mask_v16i32[16] = {16, 0, 17, 1, 18, 2, 19, 3, 20, 4, 21, 5, 22, 6, 23, 7};
    auto vd_v16i8        = m_ir_builder->CreateShuffleVector(va_v16i8, vb_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    SetVr(vd, vd_v16i8);
}

void Compiler::VMRGLH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16      = GetVrAsIntVec(va, 16);
    auto vb_v8i16      = GetVrAsIntVec(vb, 16);
    u32  mask_v8i32[8] = {8, 0, 9, 1, 10, 2, 11, 3};
    auto vd_v8i16      = m_ir_builder->CreateShuffleVector(va_v8i16, vb_v8i16, ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));
    SetVr(vd, vd_v8i16);
}

void Compiler::VMRGLW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32      = GetVrAsIntVec(va, 32);
    auto vb_v4i32      = GetVrAsIntVec(vb, 32);
    u32  mask_v4i32[4] = {4, 0, 5, 1};
    auto vd_v4i32      = m_ir_builder->CreateShuffleVector(va_v4i32, vb_v4i32, ConstantDataVector::get(m_ir_builder->getContext(), mask_v4i32));
    SetVr(vd, vd_v4i32);
}

void Compiler::VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto va_v16i16  = m_ir_builder->CreateSExt(va_v16i8, VectorType::get(m_ir_builder->getInt16Ty(), 16));
    auto vb_v16i16  = m_ir_builder->CreateZExt(vb_v16i8, VectorType::get(m_ir_builder->getInt16Ty(), 16));
    auto tmp_v16i16 = m_ir_builder->CreateMul(va_v16i16, vb_v16i16);

    auto undef_v16i16   = UndefValue::get(VectorType::get(m_ir_builder->getInt16Ty(), 16));
    u32  mask1_v4i32[4] = {0, 4, 8, 12};
    auto tmp1_v4i16     = m_ir_builder->CreateShuffleVector(tmp_v16i16, undef_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    auto tmp1_v4i32     = m_ir_builder->CreateSExt(tmp1_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    u32  mask2_v4i32[4] = {1, 5, 9, 13};
    auto tmp2_v4i16     = m_ir_builder->CreateShuffleVector(tmp_v16i16, undef_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));
    auto tmp2_v4i32     = m_ir_builder->CreateSExt(tmp2_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    u32  mask3_v4i32[4] = {2, 6, 10, 14};
    auto tmp3_v4i16     = m_ir_builder->CreateShuffleVector(tmp_v16i16, undef_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), mask3_v4i32));
    auto tmp3_v4i32     = m_ir_builder->CreateSExt(tmp3_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    u32  mask4_v4i32[4] = {3, 7, 11, 15};
    auto tmp4_v4i16     = m_ir_builder->CreateShuffleVector(tmp_v16i16, undef_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), mask4_v4i32));
    auto tmp4_v4i32     = m_ir_builder->CreateSExt(tmp4_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));

    auto vc_v4i32  = GetVrAsIntVec(vc, 32);
    auto res_v4i32 = m_ir_builder->CreateAdd(tmp1_v4i32, tmp2_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, tmp3_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, tmp4_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, vc_v4i32);

    SetVr(vd, res_v4i32);

    // TODO: Try to optimize with horizontal add
}

void Compiler::VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto vc_v4i32  = GetVrAsIntVec(vc, 32);
    auto res_v4i32 = (Value *)m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmadd_wd), va_v8i16, vb_v8i16);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, vc_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto vc_v4i32  = GetVrAsIntVec(vc, 32);
    auto res_v4i32 = (Value *)m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmadd_wd), va_v8i16, vb_v8i16);

    auto tmp1_v4i32 = m_ir_builder->CreateLShr(vc_v4i32, 31);
    tmp1_v4i32      = m_ir_builder->CreateAdd(tmp1_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x7FFFFFFF)));
    auto tmp1_v16i8 = m_ir_builder->CreateBitCast(tmp1_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    auto tmp2_v4i32 = m_ir_builder->CreateXor(vc_v4i32, res_v4i32);
    tmp2_v4i32      = m_ir_builder->CreateNot(tmp2_v4i32);
    auto sum_v4i32  = m_ir_builder->CreateAdd(vc_v4i32, res_v4i32);
    auto sum_v16i8  = m_ir_builder->CreateBitCast(sum_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    auto tmp3_v4i32 = m_ir_builder->CreateXor(vc_v4i32, sum_v4i32);
    tmp3_v4i32      = m_ir_builder->CreateAnd(tmp2_v4i32, tmp3_v4i32);
    tmp3_v4i32      = m_ir_builder->CreateAShr(tmp3_v4i32, 31);
    auto tmp3_v16i8 = m_ir_builder->CreateBitCast(tmp3_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    auto res_v16i8  = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pblendvb), sum_v16i8, tmp1_v16i8, tmp3_v16i8);
    SetVr(vd, res_v16i8);

    // TODO: Set VSCR.SAT
}

void Compiler::VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto va_v16i16  = m_ir_builder->CreateZExt(va_v16i8, VectorType::get(m_ir_builder->getInt16Ty(), 16));
    auto vb_v16i16  = m_ir_builder->CreateZExt(vb_v16i8, VectorType::get(m_ir_builder->getInt16Ty(), 16));
    auto tmp_v16i16 = m_ir_builder->CreateMul(va_v16i16, vb_v16i16);

    auto undef_v16i16   = UndefValue::get(VectorType::get(m_ir_builder->getInt16Ty(), 16));
    u32  mask1_v4i32[4] = {0, 4, 8, 12};
    auto tmp1_v4i16     = m_ir_builder->CreateShuffleVector(tmp_v16i16, undef_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    auto tmp1_v4i32     = m_ir_builder->CreateZExt(tmp1_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    u32  mask2_v4i32[4] = {1, 5, 9, 13};
    auto tmp2_v4i16     = m_ir_builder->CreateShuffleVector(tmp_v16i16, undef_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));
    auto tmp2_v4i32     = m_ir_builder->CreateZExt(tmp2_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    u32  mask3_v4i32[4] = {2, 6, 10, 14};
    auto tmp3_v4i16     = m_ir_builder->CreateShuffleVector(tmp_v16i16, undef_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), mask3_v4i32));
    auto tmp3_v4i32     = m_ir_builder->CreateZExt(tmp3_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    u32  mask4_v4i32[4] = {3, 7, 11, 15};
    auto tmp4_v4i16     = m_ir_builder->CreateShuffleVector(tmp_v16i16, undef_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), mask4_v4i32));
    auto tmp4_v4i32     = m_ir_builder->CreateZExt(tmp4_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));

    auto vc_v4i32  = GetVrAsIntVec(vc, 32);
    auto res_v4i32 = m_ir_builder->CreateAdd(tmp1_v4i32, tmp2_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, tmp3_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, tmp4_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, vc_v4i32);

    SetVr(vd, res_v4i32);

    // TODO: Try to optimize with horizontal add
}

void Compiler::VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto va_v8i32  = m_ir_builder->CreateZExt(va_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vb_v8i32  = m_ir_builder->CreateZExt(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto tmp_v8i32 = m_ir_builder->CreateMul(va_v8i32, vb_v8i32);

    auto undef_v8i32    = UndefValue::get(VectorType::get(m_ir_builder->getInt32Ty(), 8));
    u32  mask1_v4i32[4] = {0, 2, 4, 6};
    auto tmp1_v4i32     = m_ir_builder->CreateShuffleVector(tmp_v8i32, undef_v8i32, ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    u32  mask2_v4i32[4] = {1, 3, 5, 7};
    auto tmp2_v4i32     = m_ir_builder->CreateShuffleVector(tmp_v8i32, undef_v8i32, ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));

    auto vc_v4i32  = GetVrAsIntVec(vc, 32);
    auto res_v4i32 = m_ir_builder->CreateAdd(tmp1_v4i32, tmp2_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, vc_v4i32);

    SetVr(vd, res_v4i32);

    // TODO: Try to optimize with horizontal add
}

void Compiler::VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto va_v8i32  = m_ir_builder->CreateZExt(va_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vb_v8i32  = m_ir_builder->CreateZExt(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto tmp_v8i32 = m_ir_builder->CreateMul(va_v8i32, vb_v8i32);
    auto tmp_v8i64 = m_ir_builder->CreateZExt(tmp_v8i32, VectorType::get(m_ir_builder->getInt64Ty(), 8));

    u32  mask1_v4i32[4] = {0, 2, 4, 6};
    u32  mask2_v4i32[4] = {1, 3, 5, 7};
    auto tmp1_v4i64     = m_ir_builder->CreateShuffleVector(tmp_v8i64, UndefValue::get(tmp_v8i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    auto tmp2_v4i64     = m_ir_builder->CreateShuffleVector(tmp_v8i64, UndefValue::get(tmp_v8i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));

    auto vc_v4i32  = GetVrAsIntVec(vc, 32);
    auto vc_v4i64  = m_ir_builder->CreateZExt(vc_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto res_v4i64 = m_ir_builder->CreateAdd(tmp1_v4i64, tmp2_v4i64);
    res_v4i64      = m_ir_builder->CreateAdd(res_v4i64, vc_v4i64);
    auto gt_v4i1   = m_ir_builder->CreateICmpUGT(res_v4i64, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0xFFFFFFFF)));
    auto gt_v4i64  = m_ir_builder->CreateSExt(gt_v4i1, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    res_v4i64      = m_ir_builder->CreateOr(res_v4i64, gt_v4i64);
    auto res_v4i32 = m_ir_builder->CreateTrunc(res_v4i64, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, res_v4i32);

    // TODO: Set VSCR.SAT
}

void Compiler::VMULESB(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    va_v8i16       = m_ir_builder->CreateAShr(va_v8i16, 8);
    vb_v8i16       = m_ir_builder->CreateAShr(vb_v8i16, 8);
    auto res_v8i16 = m_ir_builder->CreateMul(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMULESH(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    va_v4i32       = m_ir_builder->CreateAShr(va_v4i32, 16);
    vb_v4i32       = m_ir_builder->CreateAShr(vb_v4i32, 16);
    auto res_v4i32 = m_ir_builder->CreateMul(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VMULEUB(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    va_v8i16       = m_ir_builder->CreateLShr(va_v8i16, 8);
    vb_v8i16       = m_ir_builder->CreateLShr(vb_v8i16, 8);
    auto res_v8i16 = m_ir_builder->CreateMul(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMULEUH(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    va_v4i32       = m_ir_builder->CreateLShr(va_v4i32, 16);
    vb_v4i32       = m_ir_builder->CreateLShr(vb_v4i32, 16);
    auto res_v4i32 = m_ir_builder->CreateMul(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VMULOSB(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    va_v8i16       = m_ir_builder->CreateShl(va_v8i16, 8);
    va_v8i16       = m_ir_builder->CreateAShr(va_v8i16, 8);
    vb_v8i16       = m_ir_builder->CreateShl(vb_v8i16, 8);
    vb_v8i16       = m_ir_builder->CreateAShr(vb_v8i16, 8);
    auto res_v8i16 = m_ir_builder->CreateMul(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMULOSH(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    va_v4i32       = m_ir_builder->CreateShl(va_v4i32, 16);
    va_v4i32       = m_ir_builder->CreateAShr(va_v4i32, 16);
    vb_v4i32       = m_ir_builder->CreateShl(vb_v4i32, 16);
    vb_v4i32       = m_ir_builder->CreateAShr(vb_v4i32, 16);
    auto res_v4i32 = m_ir_builder->CreateMul(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VMULOUB(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    va_v8i16       = m_ir_builder->CreateShl(va_v8i16, 8);
    va_v8i16       = m_ir_builder->CreateLShr(va_v8i16, 8);
    vb_v8i16       = m_ir_builder->CreateShl(vb_v8i16, 8);
    vb_v8i16       = m_ir_builder->CreateLShr(vb_v8i16, 8);
    auto res_v8i16 = m_ir_builder->CreateMul(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VMULOUH(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    va_v4i32       = m_ir_builder->CreateShl(va_v4i32, 16);
    va_v4i32       = m_ir_builder->CreateLShr(va_v4i32, 16);
    vb_v4i32       = m_ir_builder->CreateShl(vb_v4i32, 16);
    vb_v4i32       = m_ir_builder->CreateLShr(vb_v4i32, 16);
    auto res_v4i32 = m_ir_builder->CreateMul(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto vc_v4f32  = GetVrAsFloatVec(vc);
    vc_v4f32       = m_ir_builder->CreateFNeg(vc_v4f32);
    auto res_v4f32 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, VectorType::get(m_ir_builder->getFloatTy(), 4)), va_v4f32, vc_v4f32, vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VNOR(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateOr(va_v8i16, vb_v8i16);
    res_v8i16      = m_ir_builder->CreateNot(res_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VOR(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateOr(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VPERM(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v16i8 = GetVrAsIntVec(va, 8);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto vc_v16i8 = GetVrAsIntVec(vc, 8);

    auto thrity_one_v16i8 = m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(31));
    vc_v16i8              = m_ir_builder->CreateAnd(vc_v16i8, thrity_one_v16i8);

    auto fifteen_v16i8 = m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(15));
    auto vc_le15_v16i8 = m_ir_builder->CreateSub(fifteen_v16i8, vc_v16i8);
    auto res_va_v16i8  = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_ssse3_pshuf_b_128), va_v16i8, vc_le15_v16i8);

    auto vc_gt15_v16i8 = m_ir_builder->CreateSub(thrity_one_v16i8, vc_v16i8);
    auto cmp_i1        = m_ir_builder->CreateICmpUGT(vc_gt15_v16i8, fifteen_v16i8);
    auto cmp_i8        = m_ir_builder->CreateSExt(cmp_i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    vc_gt15_v16i8      = m_ir_builder->CreateOr(cmp_i8, vc_gt15_v16i8);
    auto res_vb_v16i8  = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_ssse3_pshuf_b_128), vb_v16i8, vc_gt15_v16i8);

    auto res_v16i8 = m_ir_builder->CreateOr(res_vb_v16i8, res_va_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VPKPX(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    auto tmpa_v4i32 = m_ir_builder->CreateShl(va_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(7)));
    tmpa_v4i32      = m_ir_builder->CreateAnd(tmpa_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0xFC000000)));
    va_v4i32        = m_ir_builder->CreateShl(va_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(10)));
    va_v4i32        = m_ir_builder->CreateAnd(va_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(~0xFC000000)));
    tmpa_v4i32      = m_ir_builder->CreateOr(tmpa_v4i32, va_v4i32);
    tmpa_v4i32      = m_ir_builder->CreateAnd(tmpa_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0xFFE00000)));
    va_v4i32        = m_ir_builder->CreateShl(va_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(3)));
    va_v4i32        = m_ir_builder->CreateAnd(va_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(~0xFFE00000)));
    tmpa_v4i32      = m_ir_builder->CreateOr(tmpa_v4i32, va_v4i32);
    auto tmpa_v8i16 = m_ir_builder->CreateBitCast(tmpa_v4i32, VectorType::get(m_ir_builder->getInt16Ty(), 8));

    auto tmpb_v4i32 = m_ir_builder->CreateShl(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(7)));
    tmpb_v4i32      = m_ir_builder->CreateAnd(tmpb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0xFC000000)));
    vb_v4i32        = m_ir_builder->CreateShl(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(10)));
    vb_v4i32        = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(~0xFC000000)));
    tmpb_v4i32      = m_ir_builder->CreateOr(tmpb_v4i32, vb_v4i32);
    tmpb_v4i32      = m_ir_builder->CreateAnd(tmpb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0xFFE00000)));
    vb_v4i32        = m_ir_builder->CreateShl(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(3)));
    vb_v4i32        = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(~0xFFE00000)));
    tmpb_v4i32      = m_ir_builder->CreateOr(tmpb_v4i32, vb_v4i32);
    auto tmpb_v8i16 = m_ir_builder->CreateBitCast(tmpb_v4i32, VectorType::get(m_ir_builder->getInt16Ty(), 8));

    u32  mask_v8i32[8] = {1, 3, 5, 7, 9, 11, 13, 15};
    auto res_v8i16     = m_ir_builder->CreateShuffleVector(tmpb_v8i16, tmpa_v8i16, ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));

    SetVr(vd, res_v8i16);

    // TODO: Implement with pext on CPUs with BMI
}

void Compiler::VPKSHSS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_packsswb_128), vb_v8i16, va_v8i16);
    SetVr(vd, res_v16i8);

    // TODO: VSCR.SAT
}

void Compiler::VPKSHUS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_packuswb_128), vb_v8i16, va_v8i16);
    SetVr(vd, res_v16i8);

    // TODO: VSCR.SAT
}

void Compiler::VPKSWSS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_packssdw_128), vb_v4i32, va_v4i32);
    SetVr(vd, res_v8i16);

    // TODO: VSCR.SAT
}

void Compiler::VPKSWUS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_packusdw), vb_v4i32, va_v4i32);
    SetVr(vd, res_v8i16);

    // TODO: VSCR.SAT
}

void Compiler::VPKUHUM(u32 vd, u32 va, u32 vb) {
    auto va_v16i8 = GetVrAsIntVec(va, 8);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);

    u32  mask_v16i32[16] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
    auto res_v16i8       = m_ir_builder->CreateShuffleVector(vb_v16i8, va_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    SetVr(vd, res_v16i8);
}

void Compiler::VPKUHUS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16 = GetVrAsIntVec(va, 16);
    auto vb_v8i16 = GetVrAsIntVec(vb, 16);
    va_v8i16      = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminuw), va_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xFF)));
    vb_v8i16      = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminuw), vb_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xFF)));
    auto va_v16i8 = m_ir_builder->CreateBitCast(va_v8i16, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    auto vb_v16i8 = m_ir_builder->CreateBitCast(vb_v8i16, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    
    u32  mask_v16i32[16] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30};
    auto res_v16i8       = m_ir_builder->CreateShuffleVector(vb_v16i8, va_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    SetVr(vd, res_v16i8);

    // TODO: Set VSCR.SAT
}

void Compiler::VPKUWUM(u32 vd, u32 va, u32 vb) {
    auto va_v8i16 = GetVrAsIntVec(va, 16);
    auto vb_v8i16 = GetVrAsIntVec(vb, 16);

    u32  mask_v8i32[8] = {0, 2, 4, 6, 8, 10, 12, 14};
    auto res_v8i16     = m_ir_builder->CreateShuffleVector(vb_v8i16, va_v8i16, ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));
    SetVr(vd, res_v8i16);
}

void Compiler::VPKUWUS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);
    va_v4i32      = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminud), va_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0xFFFF)));
    vb_v4i32      = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminud), vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0xFFFF)));
    auto va_v8i16 = m_ir_builder->CreateBitCast(va_v4i32, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    auto vb_v8i16 = m_ir_builder->CreateBitCast(vb_v4i32, VectorType::get(m_ir_builder->getInt16Ty(), 8));

    u32  mask_v8i32[8] = {0, 2, 4, 6, 8, 10, 12, 14};
    auto res_v8i16     = m_ir_builder->CreateShuffleVector(vb_v8i16, va_v8i16, ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));
    SetVr(vd, res_v8i16);

    // TODO: Set VSCR.SAT
}

void Compiler::VREFP(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse_rcp_ps), vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VRFIM(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::floor, VectorType::get(m_ir_builder->getFloatTy(), 4)), vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VRFIN(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::nearbyint, VectorType::get(m_ir_builder->getFloatTy(), 4)), vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VRFIP(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::ceil, VectorType::get(m_ir_builder->getFloatTy(), 4)), vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VRFIZ(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::trunc, VectorType::get(m_ir_builder->getFloatTy(), 4)), vb_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VRLB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    vb_v16i8        = m_ir_builder->CreateAnd(vb_v16i8, m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(7)));
    auto tmp1_v16i8 = m_ir_builder->CreateShl(va_v16i8, vb_v16i8);
    vb_v16i8        = m_ir_builder->CreateSub(m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(8)), vb_v16i8);
    auto tmp2_v16i8 = m_ir_builder->CreateLShr(va_v16i8, vb_v16i8);
    auto res_v16i8  = m_ir_builder->CreateOr(tmp1_v16i8, tmp2_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VRLH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16   = GetVrAsIntVec(va, 16);
    auto vb_v8i16   = GetVrAsIntVec(vb, 16);
    vb_v8i16        = m_ir_builder->CreateAnd(vb_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xF)));
    auto tmp1_v8i16 = m_ir_builder->CreateShl(va_v8i16, vb_v8i16);
    vb_v8i16        = m_ir_builder->CreateSub(m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0x10)), vb_v8i16);
    auto tmp2_v8i16 = m_ir_builder->CreateLShr(va_v8i16, vb_v8i16);
    auto res_v8i16  = m_ir_builder->CreateOr(tmp1_v8i16, tmp2_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VRLW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32   = GetVrAsIntVec(va, 32);
    auto vb_v4i32   = GetVrAsIntVec(vb, 32);
    vb_v4i32        = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x1F)));
    auto tmp1_v4i32 = m_ir_builder->CreateShl(va_v4i32, vb_v4i32);
    vb_v4i32        = m_ir_builder->CreateSub(m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x20)), vb_v4i32);
    auto tmp2_v4i32 = m_ir_builder->CreateLShr(va_v4i32, vb_v4i32);
    auto res_v4i32  = m_ir_builder->CreateOr(tmp1_v4i32, tmp2_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VRSQRTEFP(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::sqrt, VectorType::get(m_ir_builder->getFloatTy(), 4)), vb_v4f32);
    res_v4f32      = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse_rcp_ps), res_v4f32);
    SetVr(vd, res_v4f32);
}

void Compiler::VSEL(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);
    auto vc_v4i32 = GetVrAsIntVec(vc, 32);
    vb_v4i32      = m_ir_builder->CreateAnd(vb_v4i32, vc_v4i32);
    vc_v4i32      = m_ir_builder->CreateNot(vc_v4i32);
    va_v4i32      = m_ir_builder->CreateAnd(va_v4i32, vc_v4i32);
    auto vd_v4i32 = m_ir_builder->CreateOr(va_v4i32, vb_v4i32);
    SetVr(vd, vd_v4i32);
}

void Compiler::VSL(u32 vd, u32 va, u32 vb) {
    auto va_i128  = GetVr(va);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto sh_i8    = m_ir_builder->CreateExtractElement(vb_v16i8, m_ir_builder->getInt8(0));
    sh_i8         = m_ir_builder->CreateAnd(sh_i8, 0x7);
    auto sh_i128  = m_ir_builder->CreateZExt(sh_i8, m_ir_builder->getIntNTy(128));
    va_i128       = m_ir_builder->CreateShl(va_i128, sh_i128);
    SetVr(vd, va_i128);
}

void Compiler::VSLB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    vb_v16i8       = m_ir_builder->CreateAnd(vb_v16i8, m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(0x7)));
    auto res_v16i8 = m_ir_builder->CreateShl(va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) {
    auto va_v16i8        = GetVrAsIntVec(va, 8);
    auto vb_v16i8        = GetVrAsIntVec(vb, 8);
    sh                   = 16 - sh;
    u32  mask_v16i32[16] = {sh, sh + 1, sh + 2, sh + 3, sh + 4, sh + 5, sh + 6, sh + 7, sh + 8, sh + 9, sh + 10, sh + 11, sh + 12, sh + 13, sh + 14, sh + 15};
    auto vd_v16i8        = m_ir_builder->CreateShuffleVector(vb_v16i8, va_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    SetVr(vd, vd_v16i8);
}

void Compiler::VSLH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    vb_v8i16       = m_ir_builder->CreateAnd(vb_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xF)));
    auto res_v8i16 = m_ir_builder->CreateShl(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VSLO(u32 vd, u32 va, u32 vb) {
    auto va_i128  = GetVr(va);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto sh_i8    = m_ir_builder->CreateExtractElement(vb_v16i8, m_ir_builder->getInt8(0));
    sh_i8         = m_ir_builder->CreateAnd(sh_i8, 0x78);
    auto sh_i128  = m_ir_builder->CreateZExt(sh_i8, m_ir_builder->getIntNTy(128));
    va_i128       = m_ir_builder->CreateShl(va_i128, sh_i128);
    SetVr(vd, va_i128);
}

void Compiler::VSLW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    vb_v4i32       = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x1F)));
    auto res_v4i32 = m_ir_builder->CreateShl(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VSPLTB(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v16i8    = GetVrAsIntVec(vb, 8);
    auto undef_v16i8 = UndefValue::get(VectorType::get(m_ir_builder->getInt8Ty(), 16));
    auto mask_v16i32 = m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt32(15 - uimm5));
    auto res_v16i8   = m_ir_builder->CreateShuffleVector(vb_v16i8, undef_v16i8, mask_v16i32);
    SetVr(vd, res_v16i8);
}

void Compiler::VSPLTH(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v8i16    = GetVrAsIntVec(vb, 16);
    auto undef_v8i16 = UndefValue::get(VectorType::get(m_ir_builder->getInt16Ty(), 8));
    auto mask_v8i32  = m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt32(7 - uimm5));
    auto res_v8i16   = m_ir_builder->CreateShuffleVector(vb_v8i16, undef_v8i16, mask_v8i32);
    SetVr(vd, res_v8i16);
}

void Compiler::VSPLTISB(u32 vd, s32 simm5) {
    auto vd_v16i8 = m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8((s8)simm5));
    SetVr(vd, vd_v16i8);
}

void Compiler::VSPLTISH(u32 vd, s32 simm5) {
    auto vd_v8i16 = m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16((s16)simm5));
    SetVr(vd, vd_v8i16);
}

void Compiler::VSPLTISW(u32 vd, s32 simm5) {
    auto vd_v4i32 = m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32((s32)simm5));
    SetVr(vd, vd_v4i32);
}

void Compiler::VSPLTW(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32    = GetVrAsIntVec(vb, 32);
    auto undef_v4i32 = UndefValue::get(VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto mask_v4i32  = m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(3 - uimm5));
    auto res_v4i32   = m_ir_builder->CreateShuffleVector(vb_v4i32, undef_v4i32, mask_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VSR(u32 vd, u32 va, u32 vb) {
    auto va_i128  = GetVr(va);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto sh_i8    = m_ir_builder->CreateExtractElement(vb_v16i8, m_ir_builder->getInt8(0));
    sh_i8         = m_ir_builder->CreateAnd(sh_i8, 0x7);
    auto sh_i128  = m_ir_builder->CreateZExt(sh_i8, m_ir_builder->getIntNTy(128));
    va_i128       = m_ir_builder->CreateLShr(va_i128, sh_i128);
    SetVr(vd, va_i128);
}

void Compiler::VSRAB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    vb_v16i8       = m_ir_builder->CreateAnd(vb_v16i8, m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(0x7)));
    auto res_v16i8 = m_ir_builder->CreateAShr(va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VSRAH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    vb_v8i16       = m_ir_builder->CreateAnd(vb_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xF)));
    auto res_v8i16 = m_ir_builder->CreateAShr(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VSRAW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    vb_v4i32       = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x1F)));
    auto res_v4i32 = m_ir_builder->CreateAShr(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VSRB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    vb_v16i8       = m_ir_builder->CreateAnd(vb_v16i8, m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(0x7)));
    auto res_v16i8 = m_ir_builder->CreateLShr(va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void Compiler::VSRH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    vb_v8i16       = m_ir_builder->CreateAnd(vb_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xF)));
    auto res_v8i16 = m_ir_builder->CreateLShr(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::VSRO(u32 vd, u32 va, u32 vb) {
    auto va_i128  = GetVr(va);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto sh_i8    = m_ir_builder->CreateExtractElement(vb_v16i8, m_ir_builder->getInt8(0));
    sh_i8         = m_ir_builder->CreateAnd(sh_i8, 0x78);
    auto sh_i128  = m_ir_builder->CreateZExt(sh_i8, m_ir_builder->getIntNTy(128));
    va_i128       = m_ir_builder->CreateLShr(va_i128, sh_i128);
    SetVr(vd, va_i128);
}

void Compiler::VSRW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    vb_v4i32       = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x1F)));
    auto res_v4i32 = m_ir_builder->CreateLShr(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VSUBCUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    auto cmpv4i1  = m_ir_builder->CreateICmpUGE(va_v4i32, vb_v4i32);
    auto cmpv4i32 = m_ir_builder->CreateZExt(cmpv4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmpv4i32);
}

void Compiler::VSUBFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32   = GetVrAsFloatVec(va);
    auto vb_v4f32   = GetVrAsFloatVec(vb);
    auto diff_v4f32 = m_ir_builder->CreateFSub(va_v4f32, vb_v4f32);
    SetVr(vd, diff_v4f32);
}

void Compiler::VSUBSBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto diff_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_psubs_b), va_v16i8, vb_v16i8);
    SetVr(vd, diff_v16i8);

    // TODO: Set VSCR.SAT
}

void Compiler::VSUBSHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16   = GetVrAsIntVec(va, 16);
    auto vb_v8i16   = GetVrAsIntVec(vb, 16);
    auto diff_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_psubs_w), va_v8i16, vb_v8i16);
    SetVr(vd, diff_v8i16);

    // TODO: Set VSCR.SAT
}

void Compiler::VSUBSWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    // See the comments for VADDSWS for a detailed description of how this works

    // Find the result in case of an overflow
    auto tmp1_v4i32    = m_ir_builder->CreateLShr(va_v4i32, 31);
    tmp1_v4i32         = m_ir_builder->CreateAdd(tmp1_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x7FFFFFFF)));
    auto tmp1_v16i8    = m_ir_builder->CreateBitCast(tmp1_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // Find the elements that can overflow (elements with opposite sign bits)
    auto tmp2_v4i32 = m_ir_builder->CreateXor(va_v4i32, vb_v4i32);

    // Perform the sub
    auto diff_v4i32 = m_ir_builder->CreateSub(va_v4i32, vb_v4i32);
    auto diff_v16i8 = m_ir_builder->CreateBitCast(diff_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // Find the elements that overflowed
    auto tmp3_v4i32 = m_ir_builder->CreateXor(va_v4i32, diff_v4i32);
    tmp3_v4i32      = m_ir_builder->CreateAnd(tmp2_v4i32, tmp3_v4i32);
    tmp3_v4i32      = m_ir_builder->CreateAShr(tmp3_v4i32, 31);
    auto tmp3_v16i8 = m_ir_builder->CreateBitCast(tmp3_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // tmp4 is equal to 0xFFFFFFFF if an overflow occured and 0x00000000 otherwise.
    auto res_v16i8 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pblendvb), diff_v16i8, tmp1_v16i8, tmp3_v16i8);
    SetVr(vd, res_v16i8);

    // TODO: Set SAT
}

void Compiler::VSUBUBM(u32 vd, u32 va, u32 vb) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto diff_v16i8 = m_ir_builder->CreateSub(va_v16i8, vb_v16i8);
    SetVr(vd, diff_v16i8);
}

void Compiler::VSUBUBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto diff_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_psubus_b), va_v16i8, vb_v16i8);
    SetVr(vd, diff_v16i8);

    // TODO: Set SAT
}

void Compiler::VSUBUHM(u32 vd, u32 va, u32 vb) {
    auto va_v8i16   = GetVrAsIntVec(va, 16);
    auto vb_v8i16   = GetVrAsIntVec(vb, 16);
    auto diff_v8i16 = m_ir_builder->CreateSub(va_v8i16, vb_v8i16);
    SetVr(vd, diff_v8i16);
}

void Compiler::VSUBUHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16   = GetVrAsIntVec(va, 16);
    auto vb_v8i16   = GetVrAsIntVec(vb, 16);
    auto diff_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_psubus_w), va_v8i16, vb_v8i16);
    SetVr(vd, diff_v8i16);

    // TODO: Set SAT
}

void Compiler::VSUBUWM(u32 vd, u32 va, u32 vb) {
    auto va_v4i32   = GetVrAsIntVec(va, 32);
    auto vb_v4i32   = GetVrAsIntVec(vb, 32);
    auto diff_v4i32 = m_ir_builder->CreateSub(va_v4i32, vb_v4i32);
    SetVr(vd, diff_v4i32);
}

void Compiler::VSUBUWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32   = GetVrAsIntVec(va, 32);
    auto vb_v4i32   = GetVrAsIntVec(vb, 32);
    auto diff_v4i32 = m_ir_builder->CreateSub(va_v4i32, vb_v4i32);
    auto cmp_v4i1   = m_ir_builder->CreateICmpULE(diff_v4i32, va_v4i32);
    auto cmp_v4i32  = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto res_v4i32  = m_ir_builder->CreateAnd(diff_v4i32, cmp_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set SAT
}

void Compiler::VSUMSWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    auto res_i32 = m_ir_builder->CreateExtractElement(vb_v4i32, m_ir_builder->getInt32(3));
    auto res_i64 = m_ir_builder->CreateSExt(res_i32, m_ir_builder->getInt64Ty());
    for (auto i = 0; i < 4; i++) {
        auto va_i32 = m_ir_builder->CreateExtractElement(va_v4i32, m_ir_builder->getInt32(i));
        auto va_i64 = m_ir_builder->CreateSExt(va_i32, m_ir_builder->getInt64Ty());
        res_i64     = m_ir_builder->CreateAdd(res_i64, va_i64);
    }

    auto gt_i1    = m_ir_builder->CreateICmpSGT(res_i64, m_ir_builder->getInt64(0x7FFFFFFFull));
    auto lt_i1    = m_ir_builder->CreateICmpSLT(res_i64, m_ir_builder->getInt64(0xFFFFFFFF80000000ull));
    res_i64       = m_ir_builder->CreateSelect(gt_i1, m_ir_builder->getInt64(0x7FFFFFFFull), res_i64);
    res_i64       = m_ir_builder->CreateSelect(lt_i1, m_ir_builder->getInt64(0xFFFFFFFF80000000ull), res_i64);
    auto res_i128 = m_ir_builder->CreateZExt(res_i64, m_ir_builder->getIntNTy(128));

    SetVr(vd, res_i128);

    // TODO: Set VSCR.SAT
}

void Compiler::VSUM2SWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    u32  mask1_v2i32[2] = { 0, 2 };
    u32  mask2_v2i32[2] = { 1, 3 };
    auto va_v4i64       = m_ir_builder->CreateSExt(va_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto va1_v2i64      = m_ir_builder->CreateShuffleVector(va_v4i64, UndefValue::get(va_v4i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask1_v2i32));
    auto va2_v2i64      = m_ir_builder->CreateShuffleVector(va_v4i64, UndefValue::get(va_v4i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask2_v2i32));
    auto vb_v4i64       = m_ir_builder->CreateSExt(vb_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto vb_v2i64       = m_ir_builder->CreateShuffleVector(vb_v4i64, UndefValue::get(vb_v4i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask1_v2i32));

    auto res_v2i64 = m_ir_builder->CreateAdd(va1_v2i64, va2_v2i64);
    res_v2i64      = m_ir_builder->CreateAdd(res_v2i64, vb_v2i64);
    auto gt_v2i1   = m_ir_builder->CreateICmpSGT(res_v2i64, m_ir_builder->CreateVectorSplat(2, m_ir_builder->getInt64(0x7FFFFFFFull)));
    auto lt_v2i1   = m_ir_builder->CreateICmpSLT(res_v2i64, m_ir_builder->CreateVectorSplat(2, m_ir_builder->getInt64(0xFFFFFFFF80000000ull)));
    res_v2i64      = m_ir_builder->CreateSelect(gt_v2i1, m_ir_builder->CreateVectorSplat(2, m_ir_builder->getInt64(0x7FFFFFFFull)), res_v2i64);
    res_v2i64      = m_ir_builder->CreateSelect(lt_v2i1, m_ir_builder->CreateVectorSplat(2, m_ir_builder->getInt64(0x80000000ull)), res_v2i64);
    SetVr(vd, res_v2i64);

    // TODO: Set VSCR.SAT
}

void Compiler::VSUM4SBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8 = GetVrAsIntVec(va, 8);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    u32  mask1_v4i32[4] = { 0, 4, 8, 12 };
    u32  mask2_v4i32[4] = { 1, 5, 9, 13 };
    u32  mask3_v4i32[4] = { 2, 6, 10, 14 };
    u32  mask4_v4i32[4] = { 3, 7, 11, 15 };
    auto va_v16i64      = m_ir_builder->CreateSExt(va_v16i8, VectorType::get(m_ir_builder->getInt64Ty(), 16));
    auto va1_v4i64      = m_ir_builder->CreateShuffleVector(va_v16i64, UndefValue::get(va_v16i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    auto va2_v4i64      = m_ir_builder->CreateShuffleVector(va_v16i64, UndefValue::get(va_v16i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));
    auto va3_v4i64      = m_ir_builder->CreateShuffleVector(va_v16i64, UndefValue::get(va_v16i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask3_v4i32));
    auto va4_v4i64      = m_ir_builder->CreateShuffleVector(va_v16i64, UndefValue::get(va_v16i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask4_v4i32));
    auto vb_v4i64       = m_ir_builder->CreateSExt(vb_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));

    auto res_v4i64 = m_ir_builder->CreateAdd(va1_v4i64, va2_v4i64);
    res_v4i64      = m_ir_builder->CreateAdd(res_v4i64, va3_v4i64);
    res_v4i64      = m_ir_builder->CreateAdd(res_v4i64, va4_v4i64);
    res_v4i64      = m_ir_builder->CreateAdd(res_v4i64, vb_v4i64);
    auto gt_v4i1   = m_ir_builder->CreateICmpSGT(res_v4i64, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0x7FFFFFFFull)));
    auto lt_v4i1   = m_ir_builder->CreateICmpSLT(res_v4i64, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0xFFFFFFFF80000000ull)));
    res_v4i64      = m_ir_builder->CreateSelect(gt_v4i1, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0x7FFFFFFFull)), res_v4i64);
    res_v4i64      = m_ir_builder->CreateSelect(lt_v4i1, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0x80000000ull)), res_v4i64);
    auto res_v4i32 = m_ir_builder->CreateTrunc(res_v4i64, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, res_v4i32);

    // TODO: Set VSCR.SAT
}

void Compiler::VSUM4SHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16 = GetVrAsIntVec(va, 16);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    u32  mask1_v4i32[4] = { 0, 2, 4, 6 };
    u32  mask2_v4i32[4] = { 1, 3, 5, 7 };
    auto va_v8i64       = m_ir_builder->CreateSExt(va_v8i16, VectorType::get(m_ir_builder->getInt64Ty(), 8));
    auto va1_v4i64      = m_ir_builder->CreateShuffleVector(va_v8i64, UndefValue::get(va_v8i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    auto va2_v4i64      = m_ir_builder->CreateShuffleVector(va_v8i64, UndefValue::get(va_v8i64->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));
    auto vb_v4i64       = m_ir_builder->CreateSExt(vb_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));

    auto res_v4i64 = m_ir_builder->CreateAdd(va1_v4i64, va2_v4i64);
    res_v4i64      = m_ir_builder->CreateAdd(res_v4i64, vb_v4i64);
    auto gt_v4i1   = m_ir_builder->CreateICmpSGT(res_v4i64, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0x7FFFFFFFull)));
    auto lt_v4i1   = m_ir_builder->CreateICmpSLT(res_v4i64, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0xFFFFFFFF80000000ull)));
    res_v4i64      = m_ir_builder->CreateSelect(gt_v4i1, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0x7FFFFFFFull)), res_v4i64);
    res_v4i64      = m_ir_builder->CreateSelect(lt_v4i1, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt64(0x80000000ull)), res_v4i64);
    auto res_v4i32 = m_ir_builder->CreateTrunc(res_v4i64, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, res_v4i32);

    // TODO: Set VSCR.SAT
}

void Compiler::VSUM4UBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8 = GetVrAsIntVec(va, 8);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    u32  mask1_v4i32[4] = { 0, 4, 8, 12 };
    u32  mask2_v4i32[4] = { 1, 5, 9, 13 };
    u32  mask3_v4i32[4] = { 2, 6, 10, 14 };
    u32  mask4_v4i32[4] = { 3, 7, 11, 15 };
    auto va1_v4i8       = m_ir_builder->CreateShuffleVector(va_v16i8, UndefValue::get(va_v16i8->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask1_v4i32));
    auto va1_v4i32      = m_ir_builder->CreateZExt(va1_v4i8, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto va2_v4i8       = m_ir_builder->CreateShuffleVector(va_v16i8, UndefValue::get(va_v16i8->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask2_v4i32));
    auto va2_v4i32      = m_ir_builder->CreateZExt(va2_v4i8, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto va3_v4i8       = m_ir_builder->CreateShuffleVector(va_v16i8, UndefValue::get(va_v16i8->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask3_v4i32));
    auto va3_v4i32      = m_ir_builder->CreateZExt(va3_v4i8, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto va4_v4i8       = m_ir_builder->CreateShuffleVector(va_v16i8, UndefValue::get(va_v16i8->getType()), ConstantDataVector::get(m_ir_builder->getContext(), mask4_v4i32));
    auto va4_v4i32      = m_ir_builder->CreateZExt(va4_v4i8, VectorType::get(m_ir_builder->getInt32Ty(), 4));

    auto res_v4i32 = m_ir_builder->CreateAdd(va1_v4i32, va2_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, va3_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, va4_v4i32);
    res_v4i32      = m_ir_builder->CreateAdd(res_v4i32, vb_v4i32);
    auto lt_v4i1   = m_ir_builder->CreateICmpULT(res_v4i32, vb_v4i32);
    auto lt_v4i32  = m_ir_builder->CreateSExt(lt_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    res_v4i32      = m_ir_builder->CreateOr(lt_v4i32, res_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set VSCR.SAT
}

void Compiler::VUPKHPX(u32 vd, u32 vb) {
    auto vb_v8i16      = GetVrAsIntVec(vb, 16);
    u32  mask_v8i32[8] = { 4, 4, 5, 5, 6, 6, 7, 7 };
    vb_v8i16           = m_ir_builder->CreateShuffleVector(vb_v8i16, UndefValue::get(VectorType::get(m_ir_builder->getInt16Ty(), 8)), ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));

    auto vb_v4i32   = m_ir_builder->CreateBitCast(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    vb_v4i32        = m_ir_builder->CreateAShr(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(10)));
    auto tmp1_v4i32 = m_ir_builder->CreateLShr(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(3)));
    tmp1_v4i32      = m_ir_builder->CreateAnd(tmp1_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x00001F00)));
    auto tmp2_v4i32 = m_ir_builder->CreateLShr(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(6)));
    tmp2_v4i32      = m_ir_builder->CreateAnd(tmp2_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x0000001F)));
    auto res_v4i32  = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0xFF1F0000)));
    res_v4i32       = m_ir_builder->CreateOr(res_v4i32, tmp1_v4i32);
    res_v4i32       = m_ir_builder->CreateOr(res_v4i32, tmp2_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VUPKHSB(u32 vd, u32 vb) {
    auto vb_v16i8      = GetVrAsIntVec(vb, 8);
    u32  mask_v8i32[8] = { 8, 9, 10, 11, 12, 13, 14, 15 };
    auto vb_v8i8       = m_ir_builder->CreateShuffleVector(vb_v16i8, UndefValue::get(VectorType::get(m_ir_builder->getInt8Ty(), 16)), ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));
    auto res_v8i16     = m_ir_builder->CreateSExt(vb_v8i8, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, res_v8i16);
}

void Compiler::VUPKHSH(u32 vd, u32 vb) {
    auto vb_v8i16      = GetVrAsIntVec(vb, 16);
    u32  mask_v4i32[4] = { 4, 5, 6, 7 };
    auto vb_v4i16      = m_ir_builder->CreateShuffleVector(vb_v8i16, UndefValue::get(VectorType::get(m_ir_builder->getInt16Ty(), 8)), ConstantDataVector::get(m_ir_builder->getContext(), mask_v4i32));
    auto res_v4i32     = m_ir_builder->CreateSExt(vb_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, res_v4i32);
}

void Compiler::VUPKLPX(u32 vd, u32 vb) {
    auto vb_v8i16      = GetVrAsIntVec(vb, 16);
    u32  mask_v8i32[8] = { 0, 0, 1, 1, 2, 2, 3, 3 };
    vb_v8i16           = m_ir_builder->CreateShuffleVector(vb_v8i16, UndefValue::get(VectorType::get(m_ir_builder->getInt16Ty(), 8)), ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));

    auto vb_v4i32   = m_ir_builder->CreateBitCast(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    vb_v4i32        = m_ir_builder->CreateAShr(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(10)));
    auto tmp1_v4i32 = m_ir_builder->CreateLShr(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(3)));
    tmp1_v4i32      = m_ir_builder->CreateAnd(tmp1_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x00001F00)));
    auto tmp2_v4i32 = m_ir_builder->CreateLShr(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(6)));
    tmp2_v4i32      = m_ir_builder->CreateAnd(tmp2_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x0000001F)));
    auto res_v4i32  = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0xFF1F0000)));
    res_v4i32       = m_ir_builder->CreateOr(res_v4i32, tmp1_v4i32);
    res_v4i32       = m_ir_builder->CreateOr(res_v4i32, tmp2_v4i32);
    SetVr(vd, res_v4i32);
}

void Compiler::VUPKLSB(u32 vd, u32 vb) {
    auto vb_v16i8      = GetVrAsIntVec(vb, 8);
    u32  mask_v8i32[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    auto vb_v8i8       = m_ir_builder->CreateShuffleVector(vb_v16i8, UndefValue::get(VectorType::get(m_ir_builder->getInt8Ty(), 16)), ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));
    auto res_v8i16     = m_ir_builder->CreateSExt(vb_v8i8, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, res_v8i16);
}

void Compiler::VUPKLSH(u32 vd, u32 vb) {
    auto vb_v8i16      = GetVrAsIntVec(vb, 16);
    u32  mask_v4i32[4] = { 0, 1, 2, 3 };
    auto vb_v4i16      = m_ir_builder->CreateShuffleVector(vb_v8i16, UndefValue::get(VectorType::get(m_ir_builder->getInt16Ty(), 8)), ConstantDataVector::get(m_ir_builder->getContext(), mask_v4i32));
    auto res_v4i32     = m_ir_builder->CreateSExt(vb_v4i16, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, res_v4i32);
}

void Compiler::VXOR(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateXor(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void Compiler::MULLI(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64  = GetGpr(ra);
    auto res_i64 = m_ir_builder->CreateMul(ra_i64, m_ir_builder->getInt64((s64)simm16));
    SetGpr(rd, res_i64);
}

void Compiler::SUBFIC(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64   = GetGpr(ra);
    ra_i64        = m_ir_builder->CreateNeg(ra_i64);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, m_ir_builder->getInt64((s64)simm16));
    auto diff_i64 = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, diff_i64);
    SetXerCa(carry_i1);
}

void Compiler::CMPLI(u32 crfd, u32 l, u32 ra, u32 uimm16) {
    Value * ra_i64;
    if (l == 0) {
        ra_i64 = m_ir_builder->CreateZExt(GetGpr(ra, 32), m_ir_builder->getInt64Ty());
    } else {
        ra_i64 = GetGpr(ra);
    }

    SetCrFieldUnsignedCmp(crfd, ra_i64, m_ir_builder->getInt64(uimm16));
}

void Compiler::CMPI(u32 crfd, u32 l, u32 ra, s32 simm16) {
    Value * ra_i64;
    if (l == 0) {
        ra_i64 = m_ir_builder->CreateSExt(GetGpr(ra, 32), m_ir_builder->getInt64Ty());
    } else {
        ra_i64 = GetGpr(ra);
    }

    SetCrFieldSignedCmp(crfd, ra_i64, m_ir_builder->getInt64((s64)simm16));
}

void Compiler::ADDIC(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64   = GetGpr(ra);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), m_ir_builder->getInt64((s64)simm16), ra_i64);
    auto sum_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, sum_i64);
    SetXerCa(carry_i1);
}

void Compiler::ADDIC_(u32 rd, u32 ra, s32 simm16) {
    ADDIC(rd, ra, simm16);
    SetCrFieldSignedCmp(0, GetGpr(rd), m_ir_builder->getInt64(0));
}

void Compiler::ADDI(u32 rd, u32 ra, s32 simm16) {
    if (ra == 0) {
        SetGpr(rd, m_ir_builder->getInt64((s64)simm16));
    } else {
        auto ra_i64  = GetGpr(ra);
        auto sum_i64 = m_ir_builder->CreateAdd(ra_i64, m_ir_builder->getInt64((s64)simm16));
        SetGpr(rd, sum_i64);
    }
}

void Compiler::ADDIS(u32 rd, u32 ra, s32 simm16) {
    if (ra == 0) {
        SetGpr(rd, m_ir_builder->getInt64((s64)simm16 << 16));
    } else {
        auto ra_i64  = GetGpr(ra);
        auto sum_i64 = m_ir_builder->CreateAdd(ra_i64, m_ir_builder->getInt64((s64)simm16 << 16));
        SetGpr(rd, sum_i64);
    }
}

void Compiler::BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) {
    auto target_i64 = m_ir_builder->getInt64(branchTarget(aa ? 0 : m_state.current_instruction_address, bd));
    auto target_i32 = m_ir_builder->CreateTrunc(target_i64, m_ir_builder->getInt32Ty());
    CreateBranch(CheckBranchCondition(bo, bi), target_i32, lk ? true : false);
}

void Compiler::SC(u32 lev) {
    switch (lev) {
    case 0:
        Call<void>("SysCalls.DoSyscall", SysCalls::DoSyscall, m_state.args[CompileTaskState::Args::State], GetGpr(11));
        break;
    case 2:
        Call<void>("StaticFuncManager.StaticExecute", &StaticFuncManager::StaticExecute,
                   m_ir_builder->getInt64((u64)&Emu.GetSFuncManager()), m_state.args[CompileTaskState::Args::State], GetGpr(11, 32));
        break;
    case 3:
        Call<void>("PPUThread.FastStop", &PPUThread::FastStop, m_state.args[CompileTaskState::Args::State]);
        break;
    default:
        CompilationError(fmt::Format("SC %u", lev));
        break;
    }
}

void Compiler::B(s32 ll, u32 aa, u32 lk) {
    auto target_i64 = m_ir_builder->getInt64(branchTarget(aa ? 0 : m_state.current_instruction_address, ll));
    auto target_i32 = m_ir_builder->CreateTrunc(target_i64, m_ir_builder->getInt32Ty());
    CreateBranch(nullptr, target_i32, lk ? true : false);
}

void Compiler::MCRF(u32 crfd, u32 crfs) {
    if (crfd != crfs) {
        auto cr_i32  = GetCr();
        auto crf_i32 = GetNibble(cr_i32, crfs);
        cr_i32       = SetNibble(cr_i32, crfd, crf_i32);
        SetCr(cr_i32);
    }
}

void Compiler::BCLR(u32 bo, u32 bi, u32 bh, u32 lk) {
    auto lr_i64 = GetLr();
    lr_i64      = m_ir_builder->CreateAnd(lr_i64, ~0x3ULL);
    auto lr_i32 = m_ir_builder->CreateTrunc(lr_i64, m_ir_builder->getInt32Ty());
    CreateBranch(CheckBranchCondition(bo, bi), lr_i32, lk ? true : false, true);
}

void Compiler::CRNOR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateOr(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
}

void Compiler::CRANDC(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(bb_i32, 1);
    res_i32      = m_ir_builder->CreateAnd(ba_i32, res_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
}

void Compiler::ISYNC() {
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_mfence));
}

void Compiler::CRXOR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
}

void Compiler::DCBI(u32 ra, u32 rb) {
    // TODO: See if this can be translated to cache flush
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::CRNAND(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateAnd(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
}

void Compiler::CRAND(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateAnd(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
}

void Compiler::CREQV(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
}

void Compiler::CRORC(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(bb_i32, 1);
    res_i32      = m_ir_builder->CreateOr(ba_i32, res_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
}

void Compiler::CROR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateOr(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
}

void Compiler::BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) {
    auto ctr_i64 = GetCtr();
    ctr_i64      = m_ir_builder->CreateAnd(ctr_i64, ~0x3ULL);
    auto ctr_i32 = m_ir_builder->CreateTrunc(ctr_i64, m_ir_builder->getInt32Ty());
    CreateBranch(CheckBranchCondition(bo, bi), ctr_i32, lk ? true : false);
}

void Compiler::RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    auto rs_i32  = GetGpr(rs, 32);
    auto rs_i64  = m_ir_builder->CreateZExt(rs_i32, m_ir_builder->getInt64Ty());
    auto rsh_i64 = m_ir_builder->CreateShl(rs_i64, 32);
    rs_i64       = m_ir_builder->CreateOr(rs_i64, rsh_i64);
    auto ra_i64  = GetGpr(ra);
    auto res_i64 = rs_i64;
    if (sh) {
        auto resl_i64 = m_ir_builder->CreateLShr(rs_i64, 64 - sh);
        auto resh_i64 = m_ir_builder->CreateShl(rs_i64, sh);
        res_i64       = m_ir_builder->CreateOr(resh_i64, resl_i64);
    }

    u64 mask = s_rotate_mask[32 + mb][32 + me];
    res_i64  = m_ir_builder->CreateAnd(res_i64, mask);
    ra_i64   = m_ir_builder->CreateAnd(ra_i64, ~mask);
    res_i64  = m_ir_builder->CreateOr(res_i64, ra_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    auto rs_i32  = GetGpr(rs, 32);
    auto rs_i64  = m_ir_builder->CreateZExt(rs_i32, m_ir_builder->getInt64Ty());
    auto rsh_i64 = m_ir_builder->CreateShl(rs_i64, 32);
    rs_i64       = m_ir_builder->CreateOr(rs_i64, rsh_i64);
    auto res_i64 = rs_i64;
    if (sh) {
        auto resl_i64 = m_ir_builder->CreateLShr(rs_i64, 64 - sh);
        auto resh_i64 = m_ir_builder->CreateShl(rs_i64, sh);
        res_i64       = m_ir_builder->CreateOr(resh_i64, resl_i64);
    }

    res_i64 = m_ir_builder->CreateAnd(res_i64, s_rotate_mask[32 + mb][32 + me]);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc) {
    auto rs_i32   = GetGpr(rs, 32);
    auto rs_i64   = m_ir_builder->CreateZExt(rs_i32, m_ir_builder->getInt64Ty());
    auto rsh_i64  = m_ir_builder->CreateShl(rs_i64, 32);
    rs_i64        = m_ir_builder->CreateOr(rs_i64, rsh_i64);
    auto rb_i64   = GetGpr(rb);
    auto shl_i64  = m_ir_builder->CreateAnd(rb_i64, 0x1F);
    auto shr_i64  = m_ir_builder->CreateSub(m_ir_builder->getInt64(32), shl_i64);
    auto resl_i64 = m_ir_builder->CreateLShr(rs_i64, shr_i64);
    auto resh_i64 = m_ir_builder->CreateShl(rs_i64, shl_i64);
    auto res_i64  = m_ir_builder->CreateOr(resh_i64, resl_i64);
    res_i64       = m_ir_builder->CreateAnd(res_i64, s_rotate_mask[32 + mb][32 + me]);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::ORI(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, uimm16);
    SetGpr(ra, res_i64);
}

void Compiler::ORIS(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
}

void Compiler::XORI(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, uimm16);
    SetGpr(ra, res_i64);
}

void Compiler::XORIS(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
}

void Compiler::ANDI_(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, uimm16);
    SetGpr(ra, res_i64);
    SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
}

void Compiler::ANDIS_(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
    SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
}

void Compiler::RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = rs_i64;
    if (sh) {
        auto resl_i64 = m_ir_builder->CreateLShr(rs_i64, 64 - sh);
        auto resh_i64 = m_ir_builder->CreateShl(rs_i64, sh);
        res_i64       = m_ir_builder->CreateOr(resh_i64, resl_i64);
    }

    res_i64 = m_ir_builder->CreateAnd(res_i64, s_rotate_mask[mb][63]);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = rs_i64;
    if (sh) {
        auto resl_i64 = m_ir_builder->CreateLShr(rs_i64, 64 - sh);
        auto resh_i64 = m_ir_builder->CreateShl(rs_i64, sh);
        res_i64       = m_ir_builder->CreateOr(resh_i64, resl_i64);
    }

    res_i64 = m_ir_builder->CreateAnd(res_i64, s_rotate_mask[0][me]);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = rs_i64;
    if (sh) {
        auto resl_i64 = m_ir_builder->CreateLShr(rs_i64, 64 - sh);
        auto resh_i64 = m_ir_builder->CreateShl(rs_i64, sh);
        res_i64       = m_ir_builder->CreateOr(resh_i64, resl_i64);
    }

    res_i64 = m_ir_builder->CreateAnd(res_i64, s_rotate_mask[mb][63 - sh]);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto ra_i64  = GetGpr(ra);
    auto res_i64 = rs_i64;
    if (sh) {
        auto resl_i64 = m_ir_builder->CreateLShr(rs_i64, 64 - sh);
        auto resh_i64 = m_ir_builder->CreateShl(rs_i64, sh);
        res_i64       = m_ir_builder->CreateOr(resh_i64, resl_i64);
    }

    u64 mask = s_rotate_mask[mb][63 - sh];
    res_i64  = m_ir_builder->CreateAnd(res_i64, mask);
    ra_i64   = m_ir_builder->CreateAnd(ra_i64, ~mask);
    res_i64  = m_ir_builder->CreateOr(res_i64, ra_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) {
    auto rs_i64   = GetGpr(rs);
    auto rb_i64   = GetGpr(rb);
    auto shl_i64  = m_ir_builder->CreateAnd(rb_i64, 0x3F);
    auto shr_i64  = m_ir_builder->CreateSub(m_ir_builder->getInt64(64), shl_i64);
    auto resl_i64 = m_ir_builder->CreateLShr(rs_i64, shr_i64);
    auto resh_i64 = m_ir_builder->CreateShl(rs_i64, shl_i64);
    auto res_i64  = m_ir_builder->CreateOr(resh_i64, resl_i64);

    if (is_r) {
        res_i64 = m_ir_builder->CreateAnd(res_i64, s_rotate_mask[0][m_eb]);
    } else {
        res_i64 = m_ir_builder->CreateAnd(res_i64, s_rotate_mask[m_eb][63]);
    }

    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::CMP(u32 crfd, u32 l, u32 ra, u32 rb) {
    Value * ra_i64;
    Value * rb_i64;
    if (l == 0) {
        ra_i64 = m_ir_builder->CreateSExt(GetGpr(ra, 32), m_ir_builder->getInt64Ty());
        rb_i64 = m_ir_builder->CreateSExt(GetGpr(rb, 32), m_ir_builder->getInt64Ty());
    } else {
        ra_i64 = GetGpr(ra);
        rb_i64 = GetGpr(rb);
    }

    SetCrFieldSignedCmp(crfd, ra_i64, rb_i64);
}

void Compiler::TW(u32 to, u32 ra, u32 rb) {
    CompilationError("TW");
}

void Compiler::LVSL(u32 vd, u32 ra, u32 rb) {
    static const u128 s_lvsl_values[] = {
        {0x08090A0B0C0D0E0F, 0x0001020304050607},
        {0x090A0B0C0D0E0F10, 0x0102030405060708},
        {0x0A0B0C0D0E0F1011, 0x0203040506070809},
        {0x0B0C0D0E0F101112, 0x030405060708090A},
        {0x0C0D0E0F10111213, 0x0405060708090A0B},
        {0x0D0E0F1011121314, 0x05060708090A0B0C},
        {0x0E0F101112131415, 0x060708090A0B0C0D},
        {0x0F10111213141516, 0x0708090A0B0C0D0E},
        {0x1011121314151617, 0x08090A0B0C0D0E0F},
        {0x1112131415161718, 0x090A0B0C0D0E0F10},
        {0x1213141516171819, 0x0A0B0C0D0E0F1011},
        {0x131415161718191A, 0x0B0C0D0E0F101112},
        {0x1415161718191A1B, 0x0C0D0E0F10111213},
        {0x15161718191A1B1C, 0x0D0E0F1011121314},
        {0x161718191A1B1C1D, 0x0E0F101112131415},
        {0x1718191A1B1C1D1E, 0x0F10111213141516},
    };

    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto index_i64             = m_ir_builder->CreateAnd(addr_i64, 0xF);
    auto lvsl_values_v16i8_ptr = m_ir_builder->CreateIntToPtr(m_ir_builder->getInt64((u64)s_lvsl_values), VectorType::get(m_ir_builder->getInt8Ty(), 16)->getPointerTo());
    lvsl_values_v16i8_ptr      = m_ir_builder->CreateGEP(lvsl_values_v16i8_ptr, index_i64);
    auto val_v16i8             = m_ir_builder->CreateAlignedLoad(lvsl_values_v16i8_ptr, 16);
    SetVr(vd, val_v16i8);
}

void Compiler::LVEBX(u32 vd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto val_i8    = ReadMemory(addr_i64, 8);
    auto index_i64 = m_ir_builder->CreateAnd(addr_i64, 0xf);
    index_i64      = m_ir_builder->CreateSub(m_ir_builder->getInt64(15), index_i64);
    auto vd_v16i8  = GetVrAsIntVec(vd, 8);
    vd_v16i8       = m_ir_builder->CreateInsertElement(vd_v16i8, val_i8, index_i64);
    SetVr(vd, vd_v16i8);
}

void Compiler::SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64    = GetGpr(ra);
    ra_i64         = m_ir_builder->CreateNeg(ra_i64);
    auto rb_i64    = GetGpr(rb);
    auto res_s     = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, rb_i64);
    auto diff_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1  = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, diff_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, diff_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("SUBFCO");
    }
}

void Compiler::ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto rb_i64   = GetGpr(rb);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, rb_i64);
    auto sum_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, sum_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, sum_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
    }
}

void Compiler::MULHDU(u32 rd, u32 ra, u32 rb, bool rc) {
    auto ra_i64    = GetGpr(ra);
    auto rb_i64    = GetGpr(rb);
    auto ra_i128   = m_ir_builder->CreateZExt(ra_i64, m_ir_builder->getIntNTy(128));
    auto rb_i128   = m_ir_builder->CreateZExt(rb_i64, m_ir_builder->getIntNTy(128));
    auto prod_i128 = m_ir_builder->CreateMul(ra_i128, rb_i128);
    prod_i128      = m_ir_builder->CreateLShr(prod_i128, 64);
    auto prod_i64  = m_ir_builder->CreateTrunc(prod_i128, m_ir_builder->getInt64Ty());
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::MULHWU(u32 rd, u32 ra, u32 rb, bool rc) {
    auto ra_i32   = GetGpr(ra, 32);
    auto rb_i32   = GetGpr(rb, 32);
    auto ra_i64   = m_ir_builder->CreateZExt(ra_i32, m_ir_builder->getInt64Ty());
    auto rb_i64   = m_ir_builder->CreateZExt(rb_i32, m_ir_builder->getInt64Ty());
    auto prod_i64 = m_ir_builder->CreateMul(ra_i64, rb_i64);
    prod_i64      = m_ir_builder->CreateLShr(prod_i64, 32);
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::MFOCRF(u32 a, u32 rd, u32 crm) {
    auto cr_i32 = GetCr();
    auto cr_i64 = m_ir_builder->CreateZExt(cr_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, cr_i64);
}

void Compiler::LWARX(u32 rd, u32 ra, u32 rb) {
	throw __FUNCTION__;
	
    //auto addr_i64 = GetGpr(rb);
    //if (ra) {
    //    auto ra_i64 = GetGpr(ra);
    //    addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    //}

    //auto resv_addr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, R_ADDR));
    //auto resv_addr_i64_ptr = m_ir_builder->CreateBitCast(resv_addr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    //m_ir_builder->CreateAlignedStore(addr_i64, resv_addr_i64_ptr, 8);

    //auto resv_val_i32     = ReadMemory(addr_i64, 32, 4, false, false);
    //auto resv_val_i64     = m_ir_builder->CreateZExt(resv_val_i32, m_ir_builder->getInt64Ty());
    //auto resv_val_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, R_VALUE));
    //auto resv_val_i64_ptr = m_ir_builder->CreateBitCast(resv_val_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    //m_ir_builder->CreateAlignedStore(resv_val_i64, resv_val_i64_ptr, 8);

    //resv_val_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, m_ir_builder->getInt32Ty()), resv_val_i32);
    //resv_val_i64 = m_ir_builder->CreateZExt(resv_val_i32, m_ir_builder->getInt64Ty());
    //SetGpr(rd, resv_val_i64);
}

void Compiler::LDX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
}

void Compiler::LWZX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::SLW(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i32  = GetGpr(rs, 32);
    auto rs_i64  = m_ir_builder->CreateZExt(rs_i32, m_ir_builder->getInt64Ty());
    auto rb_i8   = GetGpr(rb, 8);
    rb_i8        = m_ir_builder->CreateAnd(rb_i8, 0x3F);
    auto rb_i64  = m_ir_builder->CreateZExt(rb_i8, m_ir_builder->getInt64Ty());
    auto res_i64 = m_ir_builder->CreateShl(rs_i64, rb_i64);
    auto res_i32 = m_ir_builder->CreateTrunc(res_i64, m_ir_builder->getInt32Ty());
    res_i64      = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::CNTLZW(u32 ra, u32 rs, bool rc) {
    auto rs_i32  = GetGpr(rs, 32);
    auto res_i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::ctlz, m_ir_builder->getInt32Ty()), rs_i32, m_ir_builder->getInt1(false));
    auto res_i64 = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::SLD(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64   = GetGpr(rs);
    auto rs_i128  = m_ir_builder->CreateZExt(rs_i64, m_ir_builder->getIntNTy(128));
    auto rb_i8    = GetGpr(rb, 8);
    rb_i8         = m_ir_builder->CreateAnd(rb_i8, 0x7F);
    auto rb_i128  = m_ir_builder->CreateZExt(rb_i8, m_ir_builder->getIntNTy(128));
    auto res_i128 = m_ir_builder->CreateShl(rs_i128, rb_i128);
    auto res_i64  = m_ir_builder->CreateTrunc(res_i128, m_ir_builder->getInt64Ty());
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::AND(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::CMPL(u32 crfd, u32 l, u32 ra, u32 rb) {
    Value * ra_i64;
    Value * rb_i64;
    if (l == 0) {
        ra_i64 = m_ir_builder->CreateZExt(GetGpr(ra, 32), m_ir_builder->getInt64Ty());
        rb_i64 = m_ir_builder->CreateZExt(GetGpr(rb, 32), m_ir_builder->getInt64Ty());
    } else {
        ra_i64 = GetGpr(ra);
        rb_i64 = GetGpr(rb);
    }

    SetCrFieldUnsignedCmp(crfd, ra_i64, rb_i64);
}

void Compiler::LVSR(u32 vd, u32 ra, u32 rb) {
    static const u128 s_lvsr_values[] = {
        {0x18191A1B1C1D1E1F, 0x1011121314151617},
        {0x1718191A1B1C1D1E, 0x0F10111213141516},
        {0x161718191A1B1C1D, 0x0E0F101112131415},
        {0x15161718191A1B1C, 0x0D0E0F1011121314},
        {0x1415161718191A1B, 0x0C0D0E0F10111213},
        {0x131415161718191A, 0x0B0C0D0E0F101112},
        {0x1213141516171819, 0x0A0B0C0D0E0F1011},
        {0x1112131415161718, 0x090A0B0C0D0E0F10},
        {0x1011121314151617, 0x08090A0B0C0D0E0F},
        {0x0F10111213141516, 0x0708090A0B0C0D0E},
        {0x0E0F101112131415, 0x060708090A0B0C0D},
        {0x0D0E0F1011121314, 0x05060708090A0B0C},
        {0x0C0D0E0F10111213, 0x0405060708090A0B},
        {0x0B0C0D0E0F101112, 0x030405060708090A},
        {0x0A0B0C0D0E0F1011, 0x0203040506070809},
        {0x090A0B0C0D0E0F10, 0x0102030405060708},
    };

    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto index_i64             = m_ir_builder->CreateAnd(addr_i64, 0xF);
    auto lvsr_values_v16i8_ptr = m_ir_builder->CreateIntToPtr(m_ir_builder->getInt64((u64)s_lvsr_values), VectorType::get(m_ir_builder->getInt8Ty(), 16)->getPointerTo());
    lvsr_values_v16i8_ptr      = m_ir_builder->CreateGEP(lvsr_values_v16i8_ptr, index_i64);
    auto val_v16i8             = m_ir_builder->CreateAlignedLoad(lvsr_values_v16i8_ptr, 16);
    SetVr(vd, val_v16i8);
}

void Compiler::LVEHX(u32 vd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64       = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFFEULL);
    auto val_i16   = ReadMemory(addr_i64, 16, 2);
    auto index_i64 = m_ir_builder->CreateAnd(addr_i64, 0xf);
    index_i64      = m_ir_builder->CreateLShr(index_i64, 1);
    index_i64      = m_ir_builder->CreateSub(m_ir_builder->getInt64(7), index_i64);
    auto vd_v8i16  = GetVrAsIntVec(vd, 16);
    vd_v8i16       = m_ir_builder->CreateInsertElement(vd_v8i16, val_i16, index_i64);
    SetVr(vd, vd_v8i16);
}

void Compiler::SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto rb_i64   = GetGpr(rb);
    auto diff_i64 = m_ir_builder->CreateSub(rb_i64, ra_i64);
    SetGpr(rd, diff_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, diff_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("SUBFO");
    }
}

void Compiler::LDUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::DCBST(u32 ra, u32 rb) {
    // TODO: Implement this
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::LWZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::CNTLZD(u32 ra, u32 rs, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::ctlz, m_ir_builder->getInt64Ty()), rs_i64, m_ir_builder->getInt1(false));
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::ANDC(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64 = GetGpr(rs);
    auto rb_i64 = GetGpr(rb);
    rb_i64 = m_ir_builder->CreateNot(rb_i64);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::TD(u32 to, u32 ra, u32 rb) {
    CompilationError("TD");
}

void Compiler::LVEWX(u32 vd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64       = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFFCULL);
    auto val_i32   = ReadMemory(addr_i64, 32, 4);
    auto index_i64 = m_ir_builder->CreateAnd(addr_i64, 0xf);
    index_i64      = m_ir_builder->CreateLShr(index_i64, 2);
    index_i64      = m_ir_builder->CreateSub(m_ir_builder->getInt64(3), index_i64);
    auto vd_v4i32  = GetVrAsIntVec(vd, 32);
    vd_v4i32       = m_ir_builder->CreateInsertElement(vd_v4i32, val_i32, index_i64);
    SetVr(vd, vd_v4i32);
}

void Compiler::MULHD(u32 rd, u32 ra, u32 rb, bool rc) {
    auto ra_i64    = GetGpr(ra);
    auto rb_i64    = GetGpr(rb);
    auto ra_i128   = m_ir_builder->CreateSExt(ra_i64, m_ir_builder->getIntNTy(128));
    auto rb_i128   = m_ir_builder->CreateSExt(rb_i64, m_ir_builder->getIntNTy(128));
    auto prod_i128 = m_ir_builder->CreateMul(ra_i128, rb_i128);
    prod_i128      = m_ir_builder->CreateLShr(prod_i128, 64);
    auto prod_i64  = m_ir_builder->CreateTrunc(prod_i128, m_ir_builder->getInt64Ty());
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::MULHW(u32 rd, u32 ra, u32 rb, bool rc) {
    auto ra_i32   = GetGpr(ra, 32);
    auto rb_i32   = GetGpr(rb, 32);
    auto ra_i64   = m_ir_builder->CreateSExt(ra_i32, m_ir_builder->getInt64Ty());
    auto rb_i64   = m_ir_builder->CreateSExt(rb_i32, m_ir_builder->getInt64Ty());
    auto prod_i64 = m_ir_builder->CreateMul(ra_i64, rb_i64);
    prod_i64      = m_ir_builder->CreateAShr(prod_i64, 32);
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::LDARX(u32 rd, u32 ra, u32 rb) {
	throw __FUNCTION__;

    //auto addr_i64 = GetGpr(rb);
    //if (ra) {
    //    auto ra_i64 = GetGpr(ra);
    //    addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    //}

    //auto resv_addr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, R_ADDR));
    //auto resv_addr_i64_ptr = m_ir_builder->CreateBitCast(resv_addr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    //m_ir_builder->CreateAlignedStore(addr_i64, resv_addr_i64_ptr, 8);

    //auto resv_val_i64     = ReadMemory(addr_i64, 64, 8, false);
    //auto resv_val_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, R_VALUE));
    //auto resv_val_i64_ptr = m_ir_builder->CreateBitCast(resv_val_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    //m_ir_builder->CreateAlignedStore(resv_val_i64, resv_val_i64_ptr, 8);

    //resv_val_i64      = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, m_ir_builder->getInt64Ty()), resv_val_i64);
    //SetGpr(rd, resv_val_i64);
}

void Compiler::DCBF(u32 ra, u32 rb) {
    // TODO: Implement this
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::LBZX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::LVX(u32 vd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64      = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFF0ULL);
    auto mem_i128 = ReadMemory(addr_i64, 128, 16);
    SetVr(vd, mem_i128);
}

void Compiler::NEG(u32 rd, u32 ra, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto diff_i64 = m_ir_builder->CreateSub(m_ir_builder->getInt64(0), ra_i64);
    SetGpr(rd, diff_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, diff_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("NEGO");
    }
}

void Compiler::LBZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::NOR(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, rb_i64);
    res_i64      = m_ir_builder->CreateXor(res_i64, (s64)-1);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::STVEBX(u32 vs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto index_i64 = m_ir_builder->CreateAnd(addr_i64, 0xf);
    index_i64      = m_ir_builder->CreateSub(m_ir_builder->getInt64(15), index_i64);
    auto vs_v16i8  = GetVrAsIntVec(vs, 8);
    auto val_i8    = m_ir_builder->CreateExtractElement(vs_v16i8, index_i64);
    WriteMemory(addr_i64, val_i8);
}

void Compiler::SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ca_i64    = GetXerCa();
    auto ra_i64    = GetGpr(ra);
    auto rb_i64    = GetGpr(rb);
    ra_i64         = m_ir_builder->CreateNot(ra_i64);
    auto res_s     = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, ca_i64);
    auto res_i64   = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry1_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    res_s          = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), res_i64, rb_i64);
    res_i64        = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry2_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    auto carry_i1  = m_ir_builder->CreateOr(carry1_i1, carry2_i1);
    SetGpr(rd, res_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("SUBFEO");
    }
}

void Compiler::ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ca_i64    = GetXerCa();
    auto ra_i64    = GetGpr(ra);
    auto rb_i64    = GetGpr(rb);
    auto res_s     = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, ca_i64);
    auto res_i64   = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry1_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    res_s          = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), res_i64, rb_i64);
    res_i64        = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry2_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    auto carry_i1  = m_ir_builder->CreateOr(carry1_i1, carry2_i1);
    SetGpr(rd, res_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("ADDEO");
    }
}

void Compiler::MTOCRF(u32 l, u32 crm, u32 rs) {
    auto rs_i32 = GetGpr(rs, 32);
    auto cr_i32 = GetCr();
    u32  mask   = 0;

    for (u32 i = 0; i < 8; i++) {
        if (crm & (1 << i)) {
            mask |= 0xF << ((7 - i) * 4);
            if (l) {
                break;
            }
        }
    }

    cr_i32 = m_ir_builder->CreateAnd(cr_i32, ~mask);
    rs_i32 = m_ir_builder->CreateAnd(rs_i32, ~mask);
    cr_i32 = m_ir_builder->CreateOr(cr_i32, rs_i32);
    SetCr(cr_i32);
}

void Compiler::STDX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 64));
}

void Compiler::STWCX_(u32 rs, u32 ra, u32 rb) {
	throw __FUNCTION__;

    //auto addr_i64 = GetGpr(rb);
    //if (ra) {
    //    auto ra_i64 = GetGpr(ra);
    //    addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    //}

    //auto resv_addr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, R_ADDR));
    //auto resv_addr_i64_ptr = m_ir_builder->CreateBitCast(resv_addr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    //auto resv_addr_i64     = (Value *)m_ir_builder->CreateAlignedLoad(resv_addr_i64_ptr, 8);
    //auto cmp_i1            = m_ir_builder->CreateICmpEQ(addr_i64, resv_addr_i64);

    //auto then_bb  = GetBasicBlockFromAddress(m_state.current_instruction_address, "then");
    //auto else_bb  = GetBasicBlockFromAddress(m_state.current_instruction_address, "else");
    //auto merge_bb = GetBasicBlockFromAddress(m_state.current_instruction_address, "merge");
    //m_ir_builder->CreateCondBr(cmp_i1, then_bb, else_bb);

    //m_ir_builder->SetInsertPoint(then_bb);
    //auto rs_i32                = GetGpr(rs, 32);
    //rs_i32                     = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, rs_i32->getType()), rs_i32);
    //resv_addr_i64              = m_ir_builder->CreateAdd(resv_addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
    //auto resv_addr_val_i32_ptr = m_ir_builder->CreateIntToPtr(resv_addr_i64, m_ir_builder->getInt32Ty()->getPointerTo());
    //auto resv_val_i8_ptr       = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, R_VALUE));
    //auto resv_val_i32_ptr      = m_ir_builder->CreateBitCast(resv_val_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    //auto resv_val_i32          = m_ir_builder->CreateAlignedLoad(resv_val_i32_ptr, 8);

    //auto res_s      = m_ir_builder->CreateAtomicCmpXchg(resv_addr_val_i32_ptr, resv_val_i32, rs_i32, AtomicOrdering::AcquireRelease, AtomicOrdering::Monotonic);
    //auto success_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    //auto cr_i32     = GetCr();
    //cr_i32          = SetBit(cr_i32, 2, success_i1);
    //SetCr(cr_i32);
    //m_ir_builder->CreateAlignedStore(m_ir_builder->getInt64(0), resv_addr_i64_ptr, 8);
    //m_ir_builder->CreateBr(merge_bb);

    //m_ir_builder->SetInsertPoint(else_bb);
    //cr_i32 = GetCr();
    //cr_i32 = ClrBit(cr_i32, 2);
    //SetCr(cr_i32);
    //m_ir_builder->CreateBr(merge_bb);
    //m_ir_builder->SetInsertPoint(merge_bb);
}

void Compiler::STWX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32));
}

void Compiler::STVEHX(u32 vs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64       = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFFEULL);
    auto index_i64 = m_ir_builder->CreateAnd(addr_i64, 0xf);
    index_i64      = m_ir_builder->CreateLShr(index_i64, 1);
    index_i64      = m_ir_builder->CreateSub(m_ir_builder->getInt64(7), index_i64);
    auto vs_v8i16  = GetVrAsIntVec(vs, 16);
    auto val_i16   = m_ir_builder->CreateExtractElement(vs_v8i16, index_i64);
    WriteMemory(addr_i64, val_i16, 2);
}

void Compiler::STDUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 64));
    SetGpr(ra, addr_i64);
}

void Compiler::STWUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 32));
    SetGpr(ra, addr_i64);
}

void Compiler::STVEWX(u32 vs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64       = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFFCULL);
    auto index_i64 = m_ir_builder->CreateAnd(addr_i64, 0xf);
    index_i64      = m_ir_builder->CreateLShr(index_i64, 2);
    index_i64      = m_ir_builder->CreateSub(m_ir_builder->getInt64(3), index_i64);
    auto vs_v4i32  = GetVrAsIntVec(vs, 32);
    auto val_i32   = m_ir_builder->CreateExtractElement(vs_v4i32, index_i64);
    WriteMemory(addr_i64, val_i32, 4);
}

void Compiler::ADDZE(u32 rd, u32 ra, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto ca_i64   = GetXerCa();
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, ca_i64);
    auto sum_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, sum_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, sum_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("ADDZEO");
    }
}

void Compiler::SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    ra_i64        = m_ir_builder->CreateNot(ra_i64);
    auto ca_i64   = GetXerCa();
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, ca_i64);
    auto res_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, res_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("SUBFZEO");
    }
}

void Compiler::STDCX_(u32 rs, u32 ra, u32 rb) {
	throw __FUNCTION__;

    //auto addr_i64 = GetGpr(rb);
    //if (ra) {
    //    auto ra_i64 = GetGpr(ra);
    //    addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    //}

    //auto resv_addr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, R_ADDR));
    //auto resv_addr_i64_ptr = m_ir_builder->CreateBitCast(resv_addr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    //auto resv_addr_i64     = (Value *)m_ir_builder->CreateAlignedLoad(resv_addr_i64_ptr, 8);
    //auto cmp_i1            = m_ir_builder->CreateICmpEQ(addr_i64, resv_addr_i64);

    //auto then_bb  = GetBasicBlockFromAddress(m_state.current_instruction_address, "then");
    //auto else_bb  = GetBasicBlockFromAddress(m_state.current_instruction_address, "else");
    //auto merge_bb = GetBasicBlockFromAddress(m_state.current_instruction_address, "merge");
    //m_ir_builder->CreateCondBr(cmp_i1, then_bb, else_bb);

    //m_ir_builder->SetInsertPoint(then_bb);
    //auto rs_i64                = GetGpr(rs, 64);
    //rs_i64                     = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, rs_i64->getType()), rs_i64);
    //resv_addr_i64              = m_ir_builder->CreateAdd(resv_addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
    //auto resv_addr_val_i64_ptr = m_ir_builder->CreateIntToPtr(resv_addr_i64, m_ir_builder->getInt64Ty()->getPointerTo());
    //auto resv_val_i8_ptr       = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, R_VALUE));
    //auto resv_val_i64_ptr      = m_ir_builder->CreateBitCast(resv_val_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    //auto resv_val_i64          = m_ir_builder->CreateAlignedLoad(resv_val_i64_ptr, 8);

    //auto res_s      = m_ir_builder->CreateAtomicCmpXchg(resv_addr_val_i64_ptr, resv_val_i64, rs_i64, AtomicOrdering::AcquireRelease, AtomicOrdering::Monotonic);
    //auto success_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    //auto cr_i32     = GetCr();
    //cr_i32          = SetBit(cr_i32, 2, success_i1);
    //SetCr(cr_i32);
    //m_ir_builder->CreateAlignedStore(m_ir_builder->getInt64(0), resv_addr_i64_ptr, 8);
    //m_ir_builder->CreateBr(merge_bb);

    //m_ir_builder->SetInsertPoint(else_bb);
    //cr_i32 = GetCr();
    //cr_i32 = ClrBit(cr_i32, 2);
    //SetCr(cr_i32);
    //m_ir_builder->CreateBr(merge_bb);
    //m_ir_builder->SetInsertPoint(merge_bb);
}

void Compiler::STBX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 8));
}

void Compiler::STVX(u32 vs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64 = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFF0ULL);
    WriteMemory(addr_i64, GetVr(vs), 16);
}

void Compiler::SUBFME(u32 rd, u32 ra, u32 oe, bool rc) {
    auto ca_i64    = GetXerCa();
    auto ra_i64    = GetGpr(ra);
    ra_i64         = m_ir_builder->CreateNot(ra_i64);
    auto res_s     = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, ca_i64);
    auto res_i64   = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry1_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    res_s          = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), res_i64, m_ir_builder->getInt64((s64)-1));
    res_i64        = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry2_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    auto carry_i1  = m_ir_builder->CreateOr(carry1_i1, carry2_i1);
    SetGpr(rd, res_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("SUBFMEO");
    }
}

void Compiler::MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto rb_i64   = GetGpr(rb);
    auto prod_i64 = m_ir_builder->CreateMul(ra_i64, rb_i64);
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO implement oe
        CompilationError("MULLDO");
    }
}

void Compiler::ADDME(u32 rd, u32 ra, u32 oe, bool rc) {
    auto ca_i64    = GetXerCa();
    auto ra_i64    = GetGpr(ra);
    auto res_s     = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), ra_i64, ca_i64);
    auto res_i64   = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry1_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    res_s          = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, m_ir_builder->getInt64Ty()), res_i64, m_ir_builder->getInt64((s64)-1));
    res_i64        = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry2_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    auto carry_i1  = m_ir_builder->CreateOr(carry1_i1, carry2_i1);
    SetGpr(rd, res_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("ADDMEO");
    }
}

void Compiler::MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i32   = GetGpr(ra, 32);
    auto rb_i32   = GetGpr(rb, 32);
    auto ra_i64   = m_ir_builder->CreateSExt(ra_i32, m_ir_builder->getInt64Ty());
    auto rb_i64   = m_ir_builder->CreateSExt(rb_i32, m_ir_builder->getInt64Ty());
    auto prod_i64 = m_ir_builder->CreateMul(ra_i64, rb_i64);
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO implement oe
        CompilationError("MULLWO");
    }
}

void Compiler::DCBTST(u32 ra, u32 rb, u32 th) {
    // TODO: Implement this
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::STBUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 8));
    SetGpr(ra, addr_i64);
}

void Compiler::ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64  = GetGpr(ra);
    auto rb_i64  = GetGpr(rb);
    auto sum_i64 = m_ir_builder->CreateAdd(ra_i64, rb_i64);
    SetGpr(rd, sum_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, sum_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
        CompilationError("ADDO");
    }
}

void Compiler::DCBT(u32 ra, u32 rb, u32 th) {
    // TODO: Implement this using prefetch
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::LHZX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::EQV(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, rb_i64);
    res_i64      = m_ir_builder->CreateNot(res_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::ECIWX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::LHZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::XOR(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::MFSPR(u32 rd, u32 spr) {
    Value * rd_i64;
    auto    n      = (spr >> 5) | ((spr & 0x1f) << 5);

    switch (n) {
    case 0x001:
        rd_i64 = GetXer();
        break;
    case 0x008:
        rd_i64 = GetLr();
        break;
    case 0x009:
        rd_i64 = GetCtr();
        break;
    case 0x100:
        rd_i64 = GetVrsave();
        break;
    case 0x10C:
        rd_i64 = Call<u64>("get_time", get_time);
        break;
    case 0x10D:
        rd_i64 = Call<u64>("get_time", get_time);
        rd_i64 = m_ir_builder->CreateLShr(rd_i64, 32);
        break;
    default:
        assert(0);
        break;
    }

    SetGpr(rd, rd_i64);
}

void Compiler::LWAX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::DST(u32 ra, u32 rb, u32 strm, u32 t) {
    // TODO: Revisit
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::LHAX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::LVXL(u32 vd, u32 ra, u32 rb) {
    LVX(vd, ra, rb);
}

void Compiler::MFTB(u32 rd, u32 spr) {
    auto tb_i64 = Call<u64>("get_time", get_time);

    u32 n = (spr >> 5) | ((spr & 0x1f) << 5);
    if (n == 0x10D) {
        tb_i64 = m_ir_builder->CreateLShr(tb_i64, 32);
    }

    SetGpr(rd, tb_i64);
}

void Compiler::LWAUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::DSTST(u32 ra, u32 rb, u32 strm, u32 t) {
    // TODO: Revisit
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::LHAUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::STHX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 16));
}

void Compiler::ORC(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    rb_i64       = m_ir_builder->CreateNot(rb_i64);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::ECOWX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32));
}

void Compiler::STHUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 16));
    SetGpr(ra, addr_i64);
}

void Compiler::OR(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64  = GetGpr(ra);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateUDiv(ra_i64, rb_i64);
    SetGpr(rd, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO implement oe
        CompilationError("DIVDUO");
    }

    // TODO make sure an exception does not occur on divide by 0 and overflow
}

void Compiler::DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i32  = GetGpr(ra, 32);
    auto rb_i32  = GetGpr(rb, 32);
    auto res_i32 = m_ir_builder->CreateUDiv(ra_i32, rb_i32);
    auto res_i64 = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO implement oe
        CompilationError("DIVWUO");
    }

    // TODO make sure an exception does not occur on divide by 0 and overflow
}

void Compiler::MTSPR(u32 spr, u32 rs) {
    auto rs_i64 = GetGpr(rs);
    auto n      = (spr >> 5) | ((spr & 0x1f) << 5);

    switch (n) {
    case 0x001:
        SetXer(rs_i64);
        break;
    case 0x008:
        SetLr(rs_i64);
        break;
    case 0x009:
        SetCtr(rs_i64);
        break;
    case 0x100:
        SetVrsave(rs_i64);
        break;
    default:
        assert(0);
        break;
    }

}

void Compiler::NAND(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, rb_i64);
    res_i64      = m_ir_builder->CreateNot(res_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::STVXL(u32 vs, u32 ra, u32 rb) {
    STVX(vs, ra, rb);
}

void Compiler::DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64  = GetGpr(ra);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateSDiv(ra_i64, rb_i64);
    SetGpr(rd, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO implement oe
        CompilationError("DIVDO");
    }

    // TODO make sure an exception does not occur on divide by 0 and overflow
}

void Compiler::DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i32  = GetGpr(ra, 32);
    auto rb_i32  = GetGpr(rb, 32);
    auto res_i32 = m_ir_builder->CreateSDiv(ra_i32, rb_i32);
    auto res_i64 = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO implement oe
        CompilationError("DIVWO");
    }

    // TODO make sure an exception does not occur on divide by 0 and overflow
}

void Compiler::LVLX(u32 vd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto eb_i64   = m_ir_builder->CreateAnd(addr_i64, 0xF);
    eb_i64        = m_ir_builder->CreateShl(eb_i64, 3);
    auto eb_i128  = m_ir_builder->CreateZExt(eb_i64, m_ir_builder->getIntNTy(128));
    addr_i64      = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFF0ULL);
    auto mem_i128 = ReadMemory(addr_i64, 128, 16);
    mem_i128      = m_ir_builder->CreateShl(mem_i128, eb_i128);
    SetVr(vd, mem_i128);
}

void Compiler::LDBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64, 0, false);
    SetGpr(rd, mem_i64);
}

void Compiler::LSWX(u32 rd, u32 ra, u32 rb) {
    CompilationError("LSWX");
}

void Compiler::LWBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32, 0, false);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::LFSX(u32 frd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    SetFpr(frd, mem_i32);
}

void Compiler::SRW(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i32  = GetGpr(rs, 32);
    auto rs_i64  = m_ir_builder->CreateZExt(rs_i32, m_ir_builder->getInt64Ty());
    auto rb_i8   = GetGpr(rb, 8);
    rb_i8        = m_ir_builder->CreateAnd(rb_i8, 0x3F);
    auto rb_i64  = m_ir_builder->CreateZExt(rb_i8, m_ir_builder->getInt64Ty());
    auto res_i64 = m_ir_builder->CreateLShr(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::SRD(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64   = GetGpr(rs);
    auto rs_i128  = m_ir_builder->CreateZExt(rs_i64, m_ir_builder->getIntNTy(128));
    auto rb_i8    = GetGpr(rb, 8);
    rb_i8         = m_ir_builder->CreateAnd(rb_i8, 0x7F);
    auto rb_i128  = m_ir_builder->CreateZExt(rb_i8, m_ir_builder->getIntNTy(128));
    auto res_i128 = m_ir_builder->CreateLShr(rs_i128, rb_i128);
    auto res_i64  = m_ir_builder->CreateTrunc(res_i128, m_ir_builder->getInt64Ty());
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::LVRX(u32 vd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto eb_i64   = m_ir_builder->CreateSub(m_ir_builder->getInt64(16), addr_i64);
    eb_i64        = m_ir_builder->CreateAnd(eb_i64, 0xF);
    eb_i64        = m_ir_builder->CreateShl(eb_i64, 3);
    auto eb_i128  = m_ir_builder->CreateZExt(eb_i64, m_ir_builder->getIntNTy(128));
    addr_i64      = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFF0ULL);
    auto mem_i128 = ReadMemory(addr_i64, 128, 16);
    mem_i128      = m_ir_builder->CreateLShr(mem_i128, eb_i128);
    auto cmp_i1   = m_ir_builder->CreateICmpNE(eb_i64, m_ir_builder->getInt64(0));
    auto cmp_i128 = m_ir_builder->CreateSExt(cmp_i1, m_ir_builder->getIntNTy(128));
    mem_i128      = m_ir_builder->CreateAnd(mem_i128, cmp_i128);
    SetVr(vd, mem_i128);
}

void Compiler::LSWI(u32 rd, u32 ra, u32 nb) {
    auto addr_i64 = ra ? GetGpr(ra) : m_ir_builder->getInt64(0);

    nb = nb ? nb : 32;
    for (u32 i = 0; i < nb; i += 4) {
        auto val_i32 = ReadMemory(addr_i64, 32, 0, true, false);

        if (i + 4 <= nb) {
            addr_i64 = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64(4));
        } else {
            u32 mask = 0xFFFFFFFF << ((4 - (nb - i)) * 8);
            val_i32  = m_ir_builder->CreateAnd(val_i32, mask);
        }

        auto val_i64 = m_ir_builder->CreateZExt(val_i32, m_ir_builder->getInt64Ty());
        SetGpr(rd, val_i64);
        rd = (rd + 1) % 32;
    }
}

void Compiler::LFSUX(u32 frd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    auto mem_i32  = ReadMemory(addr_i64, 32);
    SetFpr(frd, mem_i32);
    SetGpr(ra, addr_i64);
}

void Compiler::SYNC(u32 l) {
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_mfence));
}

void Compiler::LFDX(u32 frd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetFpr(frd, mem_i64);
}

void Compiler::LFDUX(u32 frd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    auto mem_i64  = ReadMemory(addr_i64, 64);
    SetFpr(frd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::STVLX(u32 vs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto index_i64   = m_ir_builder->CreateAnd(addr_i64, 0xf);
    auto size_i64    = m_ir_builder->CreateSub(m_ir_builder->getInt64(16), index_i64);
    addr_i64         = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFF);
    addr_i64         = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
    auto addr_i8_ptr = m_ir_builder->CreateIntToPtr(addr_i64, m_ir_builder->getInt8PtrTy());

    auto vs_i128     = GetVr(vs);
    vs_i128          = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, vs_i128->getType()), vs_i128);
    auto vs_i128_ptr = m_ir_builder->CreateAlloca(vs_i128->getType());
    vs_i128_ptr->setAlignment(16);
    m_ir_builder->CreateAlignedStore(vs_i128, vs_i128_ptr, 16);
    auto vs_i8_ptr = m_ir_builder->CreateBitCast(vs_i128_ptr, m_ir_builder->getInt8PtrTy());

    Type * types[3] = { m_ir_builder->getInt8PtrTy(), m_ir_builder->getInt8PtrTy(), m_ir_builder->getInt64Ty() };
    m_ir_builder->CreateCall5(Intrinsic::getDeclaration(m_module, Intrinsic::memcpy, types),
                              addr_i8_ptr, vs_i8_ptr, size_i64, m_ir_builder->getInt32(1), m_ir_builder->getInt1(false));
}

void Compiler::STDBRX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs), 0, false);
}

void Compiler::STSWX(u32 rs, u32 ra, u32 rb) {
    CompilationError("STSWX");
}

void Compiler::STWBRX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32), 0, false);
}

void Compiler::STFSX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i32 = m_ir_builder->CreateBitCast(GetFpr(frs, 32), m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
}

void Compiler::STVRX(u32 vs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto size_i64    = m_ir_builder->CreateAnd(addr_i64, 0xf);
    auto index_i64   = m_ir_builder->CreateSub(m_ir_builder->getInt64(16), size_i64);
    addr_i64         = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFF0);
    addr_i64         = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
    auto addr_i8_ptr = m_ir_builder->CreateIntToPtr(addr_i64, m_ir_builder->getInt8PtrTy());

    auto vs_i128     = GetVr(vs);
    vs_i128          = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, vs_i128->getType()), vs_i128);
    auto vs_i128_ptr = m_ir_builder->CreateAlloca(vs_i128->getType());
    vs_i128_ptr->setAlignment(16);
    m_ir_builder->CreateAlignedStore(vs_i128, vs_i128_ptr, 16);
    auto vs_i8_ptr = m_ir_builder->CreateBitCast(vs_i128_ptr, m_ir_builder->getInt8PtrTy());
    vs_i8_ptr      = m_ir_builder->CreateGEP(vs_i8_ptr, index_i64);

    Type * types[3] = { m_ir_builder->getInt8PtrTy(), m_ir_builder->getInt8PtrTy(), m_ir_builder->getInt64Ty() };
    m_ir_builder->CreateCall5(Intrinsic::getDeclaration(m_module, Intrinsic::memcpy, types),
                              addr_i8_ptr, vs_i8_ptr, size_i64, m_ir_builder->getInt32(1), m_ir_builder->getInt1(false));
}

void Compiler::STFSUX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto frs_i32 = m_ir_builder->CreateBitCast(GetFpr(frs, 32), m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
    SetGpr(ra, addr_i64);
}

void Compiler::STSWI(u32 rd, u32 ra, u32 nb) {
    auto addr_i64 = ra ? GetGpr(ra) : m_ir_builder->getInt64(0);

    nb = nb ? nb : 32;
    for (u32 i = 0; i < nb; i += 4) {
        auto val_i32 = GetGpr(rd, 32);

        if (i + 4 <= nb) {
            WriteMemory(addr_i64, val_i32, 0, true, false);
            addr_i64 = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64(4));
            rd = (rd + 1) % 32;
        } else {
            u32 n = nb - i;
            if (n >= 2) {
                auto val_i16 = m_ir_builder->CreateLShr(val_i32, 16);
                val_i16      = m_ir_builder->CreateTrunc(val_i16, m_ir_builder->getInt16Ty());
                WriteMemory(addr_i64, val_i16);

                if (n == 3) {
                    auto val_i8 = m_ir_builder->CreateLShr(val_i32, 8);
                    val_i8      = m_ir_builder->CreateTrunc(val_i8, m_ir_builder->getInt8Ty());
                    addr_i64    = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64(2));
                    WriteMemory(addr_i64, val_i8);
                }
            } else {
                auto val_i8 = m_ir_builder->CreateLShr(val_i32, 24);
                val_i8      = m_ir_builder->CreateTrunc(val_i8, m_ir_builder->getInt8Ty());
                WriteMemory(addr_i64, val_i8);
            }
        }
    }
}

void Compiler::STFDX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    WriteMemory(addr_i64, frs_i64);
}

void Compiler::STFDUX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    WriteMemory(addr_i64, frs_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::LVLXL(u32 vd, u32 ra, u32 rb) {
    LVLX(vd, ra, rb);
}

void Compiler::LHBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16, 0, false);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::SRAW(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i32  = GetGpr(rs, 32);
    auto rs_i64  = m_ir_builder->CreateZExt(rs_i32, m_ir_builder->getInt64Ty());
    rs_i64       = m_ir_builder->CreateShl(rs_i64, 32);
    auto rb_i8   = GetGpr(rb, 8);
    rb_i8        = m_ir_builder->CreateAnd(rb_i8, 0x3F);
    auto rb_i64  = m_ir_builder->CreateZExt(rb_i8, m_ir_builder->getInt64Ty());
    auto res_i64 = m_ir_builder->CreateAShr(rs_i64, rb_i64);
    auto ra_i64  = m_ir_builder->CreateAShr(res_i64, 32);
    SetGpr(ra, ra_i64);

    auto res_i32 = m_ir_builder->CreateTrunc(res_i64, m_ir_builder->getInt32Ty());
    auto ca1_i1  = m_ir_builder->CreateICmpSLT(ra_i64, m_ir_builder->getInt64(0));
    auto ca2_i1  = m_ir_builder->CreateICmpNE(res_i32, m_ir_builder->getInt32(0));
    auto ca_i1   = m_ir_builder->CreateAnd(ca1_i1, ca2_i1);
    SetXerCa(ca_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, ra_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::SRAD(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64   = GetGpr(rs);
    auto rs_i128  = m_ir_builder->CreateZExt(rs_i64, m_ir_builder->getIntNTy(128));
    rs_i128       = m_ir_builder->CreateShl(rs_i128, 64);
    auto rb_i8    = GetGpr(rb, 8);
    rb_i8         = m_ir_builder->CreateAnd(rb_i8, 0x7F);
    auto rb_i128  = m_ir_builder->CreateZExt(rb_i8, m_ir_builder->getIntNTy(128));
    auto res_i128 = m_ir_builder->CreateAShr(rs_i128, rb_i128);
    auto ra_i128  = m_ir_builder->CreateAShr(res_i128, 64);
    auto ra_i64   = m_ir_builder->CreateTrunc(ra_i128, m_ir_builder->getInt64Ty());
    SetGpr(ra, ra_i64);

    auto res_i64 = m_ir_builder->CreateTrunc(res_i128, m_ir_builder->getInt64Ty());
    auto ca1_i1  = m_ir_builder->CreateICmpSLT(ra_i64, m_ir_builder->getInt64(0));
    auto ca2_i1  = m_ir_builder->CreateICmpNE(res_i64, m_ir_builder->getInt64(0));
    auto ca_i1   = m_ir_builder->CreateAnd(ca1_i1, ca2_i1);
    SetXerCa(ca_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, ra_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::LVRXL(u32 vd, u32 ra, u32 rb) {
    LVRX(vd, ra, rb);
}

void Compiler::DSS(u32 strm, u32 a) {
    // TODO: Revisit
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::SRAWI(u32 ra, u32 rs, u32 sh, bool rc) {
    auto rs_i32  = GetGpr(rs, 32);
    auto rs_i64  = m_ir_builder->CreateZExt(rs_i32, m_ir_builder->getInt64Ty());
    rs_i64       = m_ir_builder->CreateShl(rs_i64, 32);
    auto res_i64 = m_ir_builder->CreateAShr(rs_i64, sh);
    auto ra_i64  = m_ir_builder->CreateAShr(res_i64, 32);
    SetGpr(ra, ra_i64);

    auto res_i32 = m_ir_builder->CreateTrunc(res_i64, m_ir_builder->getInt32Ty());
    auto ca1_i1  = m_ir_builder->CreateICmpSLT(ra_i64, m_ir_builder->getInt64(0));
    auto ca2_i1  = m_ir_builder->CreateICmpNE(res_i32, m_ir_builder->getInt32(0));
    auto ca_i1   = m_ir_builder->CreateAnd(ca1_i1, ca2_i1);
    SetXerCa(ca_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, ra_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::SRADI1(u32 ra, u32 rs, u32 sh, bool rc) {
    auto rs_i64   = GetGpr(rs);
    auto rs_i128  = m_ir_builder->CreateZExt(rs_i64, m_ir_builder->getIntNTy(128));
    rs_i128       = m_ir_builder->CreateShl(rs_i128, 64);
    auto res_i128 = m_ir_builder->CreateAShr(rs_i128, sh);
    auto ra_i128  = m_ir_builder->CreateAShr(res_i128, 64);
    auto ra_i64   = m_ir_builder->CreateTrunc(ra_i128, m_ir_builder->getInt64Ty());
    SetGpr(ra, ra_i64);

    auto res_i64 = m_ir_builder->CreateTrunc(res_i128, m_ir_builder->getInt64Ty());
    auto ca1_i1  = m_ir_builder->CreateICmpSLT(ra_i64, m_ir_builder->getInt64(0));
    auto ca2_i1  = m_ir_builder->CreateICmpNE(res_i64, m_ir_builder->getInt64(0));
    auto ca_i1   = m_ir_builder->CreateAnd(ca1_i1, ca2_i1);
    SetXerCa(ca_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, ra_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::SRADI2(u32 ra, u32 rs, u32 sh, bool rc) {
    SRADI1(ra, rs, sh, rc);
}

void Compiler::EIEIO() {
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_mfence));
}

void Compiler::STVLXL(u32 vs, u32 ra, u32 rb) {
    STVLX(vs, ra, rb);
}

void Compiler::STHBRX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 16), 0, false);
}

void Compiler::EXTSH(u32 ra, u32 rs, bool rc) {
    auto rs_i16 = GetGpr(rs, 16);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i16, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::STVRXL(u32 vs, u32 ra, u32 rb) {
    STVRX(vs, ra, rb);
}

void Compiler::EXTSB(u32 ra, u32 rs, bool rc) {
    auto rs_i8  = GetGpr(rs, 8);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i8, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::STFIWX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    auto frs_i32 = m_ir_builder->CreateTrunc(frs_i64, m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
}

void Compiler::EXTSW(u32 ra, u32 rs, bool rc) {
    auto rs_i32 = GetGpr(rs, 32);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i32, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
}

void Compiler::ICBI(u32 ra, u32 rs) {
    // TODO: Revisit
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::donothing));
}

void Compiler::DCBZ(u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64         = m_ir_builder->CreateAnd(addr_i64, ~(127ULL));
    addr_i64         = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
    auto addr_i8_ptr = m_ir_builder->CreateIntToPtr(addr_i64, m_ir_builder->getInt8PtrTy());

    std::vector<Type *> types = {(Type *)m_ir_builder->getInt8PtrTy(), (Type *)m_ir_builder->getInt32Ty()};
    m_ir_builder->CreateCall5(Intrinsic::getDeclaration(m_module, Intrinsic::memset, types),
                              addr_i8_ptr, m_ir_builder->getInt8(0), m_ir_builder->getInt32(128), m_ir_builder->getInt32(128), m_ir_builder->getInt1(true));
}

void Compiler::LWZ(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::LWZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::LBZ(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::LBZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::STW(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32));
}

void Compiler::STWU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 32));
    SetGpr(ra, addr_i64);
}

void Compiler::STB(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 8));
}

void Compiler::STBU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 8));
    SetGpr(ra, addr_i64);
}

void Compiler::LHZ(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::LHZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::LHA(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::LHAU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::STH(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 16));
}

void Compiler::STHU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 16));
    SetGpr(ra, addr_i64);
}

void Compiler::LMW(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        addr_i64 = m_ir_builder->CreateAdd(addr_i64, GetGpr(ra));
    }

    for (u32 i = rd; i < 32; i++) {
        auto val_i32 = ReadMemory(addr_i64, 32);
        auto val_i64 = m_ir_builder->CreateZExt(val_i32, m_ir_builder->getInt64Ty());
        SetGpr(i, val_i64);
        addr_i64 = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64(4));
    }
}

void Compiler::STMW(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        addr_i64 = m_ir_builder->CreateAdd(addr_i64, GetGpr(ra));
    }

    for (u32 i = rs; i < 32; i++) {
        auto val_i32 = GetGpr(i, 32);
        WriteMemory(addr_i64, val_i32);
        addr_i64 = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64(4));
    }
}

void Compiler::LFS(u32 frd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    SetFpr(frd, mem_i32);
}

void Compiler::LFSU(u32 frd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    auto mem_i32  = ReadMemory(addr_i64, 32);
    SetFpr(frd, mem_i32);
    SetGpr(ra, addr_i64);
}

void Compiler::LFD(u32 frd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetFpr(frd, mem_i64);
}

void Compiler::LFDU(u32 frd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetFpr(frd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::STFS(u32 frs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i32 = m_ir_builder->CreateBitCast(GetFpr(frs, 32), m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
}

void Compiler::STFSU(u32 frs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto frs_i32 = m_ir_builder->CreateBitCast(GetFpr(frs, 32), m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
    SetGpr(ra, addr_i64);
}

void Compiler::STFD(u32 frs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    WriteMemory(addr_i64, frs_i64);
}

void Compiler::STFDU(u32 frs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    WriteMemory(addr_i64, frs_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::LD(u32 rd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
}

void Compiler::LDU(u32 rd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
}

void Compiler::LWA(u32 rd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
}

void Compiler::FDIVS(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFDiv(ra_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FDIVS.");
    }

    // TODO: Set flags
}

void Compiler::FSUBS(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFSub(ra_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FSUBS.");
    }

    // TODO: Set flags
}

void Compiler::FADDS(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFAdd(ra_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FADDS.");
    }

    // TODO: Set flags
}

void Compiler::FSQRTS(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::sqrt, m_ir_builder->getDoubleTy()), rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FSQRTS.");
    }

    // TODO: Set flags
}

void Compiler::FRES(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFDiv(ConstantFP::get(m_ir_builder->getDoubleTy(), 1.0), rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FRES.");
    }

    // TODO: Set flags
}

void Compiler::FMULS(u32 frd, u32 fra, u32 frc, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = m_ir_builder->CreateFMul(ra_f64, rc_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FMULS.");
    }

    // TODO: Set flags
}

void Compiler::FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, m_ir_builder->getDoubleTy()), ra_f64, rc_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FMADDS.");
    }

    // TODO: Set flags
}

void Compiler::FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, m_ir_builder->getDoubleTy()), ra_f64, rc_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FMSUBS.");
    }

    // TODO: Set flags
}

void Compiler::FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, m_ir_builder->getDoubleTy()), ra_f64, rc_f64, rb_f64);
    res_f64      = m_ir_builder->CreateFNeg(res_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FNMSUBS.");
    }

    // TODO: Set flags
}

void Compiler::FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, m_ir_builder->getDoubleTy()), ra_f64, rc_f64, rb_f64);
    res_f64      = m_ir_builder->CreateFNeg(res_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    SetFpr(frd, res_f32);

    if (rc) {
        // TODO: Implement this
        CompilationError("FNMADDS.");
    }

    // TODO: Set flags
}

void Compiler::STD(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 64));
}

void Compiler::STDU(u32 rs, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 64));
    SetGpr(ra, addr_i64);
}

void Compiler::MTFSB1(u32 crbd, bool rc) {
    auto fpscr_i32 = GetFpscr();
    fpscr_i32      = SetBit(fpscr_i32, crbd, m_ir_builder->getInt32(1), false);
    SetFpscr(fpscr_i32);

    if (rc) {
        // TODO: Implement this
        CompilationError("MTFSB1.");
    }
}

void Compiler::MCRFS(u32 crbd, u32 crbs) {
    auto fpscr_i32 = GetFpscr();
    auto val_i32   = GetNibble(fpscr_i32, crbs);
    SetCrField(crbd, val_i32);

    switch (crbs) {
    case 0:
        fpscr_i32 = ClrBit(fpscr_i32, 0);
        fpscr_i32 = ClrBit(fpscr_i32, 3);
        break;
    case 1:
        fpscr_i32 = ClrNibble(fpscr_i32, 1);
        break;
    case 2:
        fpscr_i32 = ClrNibble(fpscr_i32, 2);
        break;
    case 3:
        fpscr_i32 = ClrBit(fpscr_i32, 12);
        break;
    case 5:
        fpscr_i32 = ClrBit(fpscr_i32, 21);
        fpscr_i32 = ClrBit(fpscr_i32, 22);
        fpscr_i32 = ClrBit(fpscr_i32, 23);
        break;
    default:
        break;
    }

    SetFpscr(fpscr_i32);
}

void Compiler::MTFSB0(u32 crbd, bool rc) {
    auto fpscr_i32 = GetFpscr();
    fpscr_i32      = ClrBit(fpscr_i32, crbd);
    SetFpscr(fpscr_i32);

    if (rc) {
        // TODO: Implement this
        CompilationError("MTFSB0.");
    }
}

void Compiler::MTFSFI(u32 crfd, u32 i, bool rc) {
    auto fpscr_i32 = GetFpscr();
    fpscr_i32      = SetNibble(fpscr_i32, crfd, m_ir_builder->getInt32(i & 0xF));
    SetFpscr(fpscr_i32);

    if (rc) {
        // TODO: Implement this
        CompilationError("MTFSFI.");
    }
}

void Compiler::MFFS(u32 frd, bool rc) {
    auto fpscr_i32 = GetFpscr();
    auto fpscr_i64 = m_ir_builder->CreateZExt(fpscr_i32, m_ir_builder->getInt64Ty());
    SetFpr(frd, fpscr_i64);

    if (rc) {
        // TODO: Implement this
        CompilationError("MFFS.");
    }
}

void Compiler::MTFSF(u32 flm, u32 frb, bool rc) {
    u32 mask = 0;
    for(u32 i = 0; i < 8; i++) {
        if (flm & (1 << i)) {
            mask |= 0xF << (i * 4);
        }
    }

    auto rb_i32    = GetFpr(frb, 32, true);
    auto fpscr_i32 = GetFpscr();
    fpscr_i32      = m_ir_builder->CreateAnd(fpscr_i32, ~mask);
    rb_i32         = m_ir_builder->CreateAnd(rb_i32, mask);
    fpscr_i32      = m_ir_builder->CreateOr(fpscr_i32, rb_i32);
    SetFpscr(fpscr_i32);

    if (rc) {
        // TODO: Implement this
        CompilationError("MTFSF.");
    }
}

void Compiler::FCMPU(u32 crfd, u32 fra, u32 frb) {
    auto ra_f64 = GetFpr(fra);
    auto rb_f64 = GetFpr(frb);
    auto lt_i1  = m_ir_builder->CreateFCmpOLT(ra_f64, rb_f64);
    auto gt_i1  = m_ir_builder->CreateFCmpOGT(ra_f64, rb_f64);
    auto eq_i1  = m_ir_builder->CreateFCmpOEQ(ra_f64, rb_f64);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, crfd, lt_i1, gt_i1, eq_i1, m_ir_builder->getInt1(false));
    SetCr(cr_i32);

    // TODO: Set flags / Handle NaN
}

void Compiler::FRSP(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f32 = m_ir_builder->CreateFPTrunc(rb_f64, m_ir_builder->getFloatTy());
    auto res_f64 = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FRSP.");
    }

    // TODO: Revisit this
    // TODO: Set flags
}

void Compiler::FCTIW(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto max_i1  = m_ir_builder->CreateFCmpOGT(rb_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), 2147483647.0));
    auto min_i1  = m_ir_builder->CreateFCmpULT(rb_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), -2147483648.0));
    auto res_i32 = m_ir_builder->CreateFPToSI(rb_f64, m_ir_builder->getInt32Ty());
    auto res_i64 = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    res_i64      = m_ir_builder->CreateSelect(max_i1, m_ir_builder->getInt64(0x7FFFFFFF), res_i64);
    res_i64      = m_ir_builder->CreateSelect(min_i1, m_ir_builder->getInt64(0x80000000), res_i64);
    SetFpr(frd, res_i64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FCTIW.");
    }

    // TODO: Set flags / Implement rounding modes
}

void Compiler::FCTIWZ(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto max_i1  = m_ir_builder->CreateFCmpOGT(rb_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), 2147483647.0));
    auto min_i1  = m_ir_builder->CreateFCmpULT(rb_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), -2147483648.0));
    auto res_i32 = m_ir_builder->CreateFPToSI(rb_f64, m_ir_builder->getInt32Ty());
    auto res_i64 = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    res_i64      = m_ir_builder->CreateSelect(max_i1, m_ir_builder->getInt64(0x7FFFFFFF), res_i64);
    res_i64      = m_ir_builder->CreateSelect(min_i1, m_ir_builder->getInt64(0x80000000), res_i64);
    SetFpr(frd, res_i64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FCTIWZ.");
    }

    // TODO: Set flags
}

void Compiler::FDIV(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFDiv(ra_f64, rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FDIV.");
    }

    // TODO: Set flags
}

void Compiler::FSUB(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFSub(ra_f64, rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FSUB.");
    }

    // TODO: Set flags
}

void Compiler::FADD(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFAdd(ra_f64, rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FADD.");
    }

    // TODO: Set flags
}

void Compiler::FSQRT(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::sqrt, m_ir_builder->getDoubleTy()), rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FSQRT.");
    }

    // TODO: Set flags
}

void Compiler::FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    auto cmp_i1  = m_ir_builder->CreateFCmpOGE(ra_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), 0.0));
    auto res_f64 = m_ir_builder->CreateSelect(cmp_i1, rc_f64, rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FSEL.");
    }

    // TODO: Set flags
}

void Compiler::FMUL(u32 frd, u32 fra, u32 frc, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = m_ir_builder->CreateFMul(ra_f64, rc_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FMUL.");
    }

    // TODO: Set flags
}

void Compiler::FRSQRTE(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::sqrt, m_ir_builder->getDoubleTy()), rb_f64);
    res_f64      = m_ir_builder->CreateFDiv(ConstantFP::get(m_ir_builder->getDoubleTy(), 1.0), res_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FRSQRTE.");
    }
}

void Compiler::FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    auto res_f64 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, m_ir_builder->getDoubleTy()), ra_f64, rc_f64, rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FMSUB.");
    }

    // TODO: Set flags
}

void Compiler::FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, m_ir_builder->getDoubleTy()), ra_f64, rc_f64, rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FMADD.");
    }

    // TODO: Set flags
}

void Compiler::FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rc_f64       = m_ir_builder->CreateFNeg(rc_f64);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, m_ir_builder->getDoubleTy()), ra_f64, rc_f64, rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FNMSUB.");
    }

    // TODO: Set flags
}

void Compiler::FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    rc_f64       = m_ir_builder->CreateFNeg(rc_f64);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, m_ir_builder->getDoubleTy()), ra_f64, rc_f64, rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FNMADD.");
    }

    // TODO: Set flags
}

void Compiler::FCMPO(u32 crfd, u32 fra, u32 frb) {
    auto ra_f64 = GetFpr(fra);
    auto rb_f64 = GetFpr(frb);
    auto lt_i1  = m_ir_builder->CreateFCmpOLT(ra_f64, rb_f64);
    auto gt_i1  = m_ir_builder->CreateFCmpOGT(ra_f64, rb_f64);
    auto eq_i1  = m_ir_builder->CreateFCmpOEQ(ra_f64, rb_f64);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, crfd, lt_i1, gt_i1, eq_i1, m_ir_builder->getInt1(false));
    SetCr(cr_i32);

    // TODO: Set flags / Handle NaN
}

void Compiler::FNEG(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    SetFpr(frd, rb_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FNEG.");
    }

    // TODO: Set flags
}

void Compiler::FMR(u32 frd, u32 frb, bool rc) {
    SetFpr(frd, GetFpr(frb));

    if (rc) {
        // TODO: Implement this
        CompilationError("FMR.");
    }

    // TODO: Set flags
}

void Compiler::FNABS(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::fabs, m_ir_builder->getDoubleTy()), rb_f64);
    res_f64      = m_ir_builder->CreateFNeg(res_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FNABS.");
    }

    // TODO: Set flags
}

void Compiler::FABS(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::fabs, m_ir_builder->getDoubleTy()), rb_f64);
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FABS.");
    }

    // TODO: Set flags
}

void Compiler::FCTID(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto max_i1  = m_ir_builder->CreateFCmpOGT(rb_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), 9223372036854775807.0));
    auto min_i1  = m_ir_builder->CreateFCmpULT(rb_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), -9223372036854775808.0));
    auto res_i64 = m_ir_builder->CreateFPToSI(rb_f64, m_ir_builder->getInt64Ty());
    res_i64      = m_ir_builder->CreateSelect(max_i1, m_ir_builder->getInt64(0x7FFFFFFFFFFFFFFF), res_i64);
    res_i64      = m_ir_builder->CreateSelect(min_i1, m_ir_builder->getInt64(0x8000000000000000), res_i64);
    SetFpr(frd, res_i64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FCTID.");
    }

    // TODO: Set flags / Implement rounding modes
}

void Compiler::FCTIDZ(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto max_i1  = m_ir_builder->CreateFCmpOGT(rb_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), 9223372036854775807.0));
    auto min_i1  = m_ir_builder->CreateFCmpULT(rb_f64, ConstantFP::get(m_ir_builder->getDoubleTy(), -9223372036854775808.0));
    auto res_i64 = m_ir_builder->CreateFPToSI(rb_f64, m_ir_builder->getInt64Ty());
    res_i64      = m_ir_builder->CreateSelect(max_i1, m_ir_builder->getInt64(0x7FFFFFFFFFFFFFFF), res_i64);
    res_i64      = m_ir_builder->CreateSelect(min_i1, m_ir_builder->getInt64(0x8000000000000000), res_i64);
    SetFpr(frd, res_i64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FCTIDZ.");
    }

    // TODO: Set flags
}

void Compiler::FCFID(u32 frd, u32 frb, bool rc) {
    auto rb_i64  = GetFpr(frb, 64, true);
    auto res_f64 = m_ir_builder->CreateSIToFP(rb_i64, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    if (rc) {
        // TODO: Implement this
        CompilationError("FCFID.");
    }

    // TODO: Set flags
}

void Compiler::UNK(const u32 code, const u32 opcode, const u32 gcode) {
    CompilationError(fmt::Format("Unknown/Illegal opcode! (0x%08x : 0x%x : 0x%x)", code, opcode, gcode));
}

std::string Compiler::GetBasicBlockNameFromAddress(u32 address, const std::string & suffix) const {
    std::string name;

    if (address == 0) {
        name = "entry";
    } else if (address == 0xFFFFFFFF) {
        name = "default_exit";
    } else {
        name = fmt::Format("instr_0x%08X", address);
    }

    if (suffix != "") {
        name += "_" + suffix;
    }

    return name;
}

u32 Compiler::GetAddressFromBasicBlockName(const std::string & name) const {
    if (name.compare(0, 6, "instr_") == 0) {
        return strtoul(name.c_str() + 6, nullptr, 0);
    } else if (name == GetBasicBlockNameFromAddress(0)) {
        return 0;
    } else if (name == GetBasicBlockNameFromAddress(0xFFFFFFFF)) {
        return 0xFFFFFFFF;
    }

    return 0;
}

BasicBlock * Compiler::GetBasicBlockFromAddress(u32 address, const std::string & suffix, bool create_if_not_exist) {
    auto         block_name = GetBasicBlockNameFromAddress(address, suffix);
    BasicBlock * block      = nullptr;
    BasicBlock * next_block = nullptr;
    for (auto i = m_state.function->begin(); i != m_state.function->end(); i++) {
        if (i->getName() == block_name) {
            block = &(*i);
            break;
        }

#ifdef _DEBUG
        auto block_address = GetAddressFromBasicBlockName(i->getName());
        if (block_address > address) {
            next_block = &(*i);
            break;
        }
#endif
    }

    if (!block && create_if_not_exist) {
        block = BasicBlock::Create(m_ir_builder->getContext(), block_name, m_state.function, next_block);
    }

    return block;
}

Value * Compiler::GetBit(Value * val, u32 n) {
    Value * bit;

#ifdef PPU_LLVM_RECOMPILER_USE_BMI
    if (val->getType()->isIntegerTy(32)) {
        bit = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_32), val, m_ir_builder->getInt32(1 << (31- n)));
    } else if (val->getType()->isIntegerTy(64)) {
        bit = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_64), val, m_ir_builder->getInt64((u64)1 << (63 - n)));
    } else {
#endif
        if (val->getType()->getIntegerBitWidth() != (n + 1)) {
            bit = m_ir_builder->CreateLShr(val, val->getType()->getIntegerBitWidth() - n - 1);
        }

        bit = m_ir_builder->CreateAnd(bit, 1);
#ifdef PPU_LLVM_RECOMPILER_USE_BMI
    }
#endif

    return bit;
}

Value * Compiler::ClrBit(Value * val, u32 n) {
    return m_ir_builder->CreateAnd(val, ~((u64)1 << (val->getType()->getIntegerBitWidth() - n - 1)));
}

Value * Compiler::SetBit(Value * val, u32 n, Value * bit, bool doClear) {
    if (doClear) {
        val = ClrBit(val, n);
    }

    if (bit->getType()->getIntegerBitWidth() < val->getType()->getIntegerBitWidth()) {
        bit = m_ir_builder->CreateZExt(bit, val->getType());
    } else if (bit->getType()->getIntegerBitWidth() > val->getType()->getIntegerBitWidth()) {
        bit = m_ir_builder->CreateTrunc(bit, val->getType());
    }

    if (val->getType()->getIntegerBitWidth() != (n + 1)) {
        bit = m_ir_builder->CreateShl(bit, bit->getType()->getIntegerBitWidth() - n - 1);
    }

    return m_ir_builder->CreateOr(val, bit);
}

Value * Compiler::GetNibble(Value * val, u32 n) {
    Value * nibble;

#ifdef PPU_LLVM_RECOMPILER_USE_BMI
    if (val->getType()->isIntegerTy(32)) {
        nibble = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_32), val, m_ir_builder->getInt32((u64)0xF << ((7 - n) * 4)));
    } else if (val->getType()->isIntegerTy(64)) {
        nibble = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_64), val, m_ir_builder->getInt64((u64)0xF << ((15 - n) * 4)));
    } else {
#endif
        if ((val->getType()->getIntegerBitWidth() >> 2) != (n + 1)) {
            val = m_ir_builder->CreateLShr(val, (((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4);
        }

        nibble = m_ir_builder->CreateAnd(val, 0xF);
#ifdef PPU_LLVM_RECOMPILER_USE_BMI
    }
#endif

    return nibble;
}

Value * Compiler::ClrNibble(Value * val, u32 n) {
    return m_ir_builder->CreateAnd(val, ~((u64)0xF << ((((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4)));
}

Value * Compiler::SetNibble(Value * val, u32 n, Value * nibble, bool doClear) {
    if (doClear) {
        val = ClrNibble(val, n);
    }

    if (nibble->getType()->getIntegerBitWidth() < val->getType()->getIntegerBitWidth()) {
        nibble = m_ir_builder->CreateZExt(nibble, val->getType());
    } else if (nibble->getType()->getIntegerBitWidth() > val->getType()->getIntegerBitWidth()) {
        nibble = m_ir_builder->CreateTrunc(nibble, val->getType());
    }

    if ((val->getType()->getIntegerBitWidth() >> 2) != (n + 1)) {
        nibble = m_ir_builder->CreateShl(nibble, (((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4);
    }

    return m_ir_builder->CreateOr(val, nibble);
}

Value * Compiler::SetNibble(Value * val, u32 n, Value * b0, Value * b1, Value * b2, Value * b3, bool doClear) {
    if (doClear) {
        val = ClrNibble(val, n);
    }

    if (b0) {
        val = SetBit(val, n * 4, b0, false);
    }

    if (b1) {
        val = SetBit(val, (n * 4) + 1, b1, false);
    }

    if (b2) {
        val = SetBit(val, (n * 4) + 2, b2, false);
    }

    if (b3) {
        val = SetBit(val, (n * 4) + 3, b3, false);
    }

    return val;
}

Value * Compiler::GetPc() {
    auto pc_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, PC));
    auto pc_i32_ptr = m_ir_builder->CreateBitCast(pc_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(pc_i32_ptr, 4);
}

void Compiler::SetPc(Value * val_ix) {
    auto pc_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, PC));
    auto pc_i32_ptr = m_ir_builder->CreateBitCast(pc_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    auto val_i32    = m_ir_builder->CreateZExtOrTrunc(val_ix, m_ir_builder->getInt32Ty());
    m_ir_builder->CreateAlignedStore(val_i32, pc_i32_ptr, 4);
}

Value * Compiler::GetGpr(u32 r, u32 num_bits) {
    auto r_i8_ptr = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, GPR[r]));
    auto r_ix_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getIntNTy(num_bits)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(r_ix_ptr, 8);
}

void Compiler::SetGpr(u32 r, Value * val_x64) {
    auto r_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, GPR[r]));
    auto r_i64_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    auto val_i64   = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    m_ir_builder->CreateAlignedStore(val_i64, r_i64_ptr, 8);
}

Value * Compiler::GetCr() {
    auto cr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, CR));
    auto cr_i32_ptr = m_ir_builder->CreateBitCast(cr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(cr_i32_ptr, 4);
}

Value * Compiler::GetCrField(u32 n) {
    return GetNibble(GetCr(), n);
}

void Compiler::SetCr(Value * val_x32) {
    auto val_i32    = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto cr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, CR));
    auto cr_i32_ptr = m_ir_builder->CreateBitCast(cr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i32, cr_i32_ptr, 4);
}

void Compiler::SetCrField(u32 n, Value * field) {
    SetCr(SetNibble(GetCr(), n, field));
}

void Compiler::SetCrField(u32 n, Value * b0, Value * b1, Value * b2, Value * b3) {
    SetCr(SetNibble(GetCr(), n, b0, b1, b2, b3));
}

void Compiler::SetCrFieldSignedCmp(u32 n, Value * a, Value * b) {
    auto lt_i1  = m_ir_builder->CreateICmpSLT(a, b);
    auto gt_i1  = m_ir_builder->CreateICmpSGT(a, b);
    auto eq_i1  = m_ir_builder->CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void Compiler::SetCrFieldUnsignedCmp(u32 n, Value * a, Value * b) {
    auto lt_i1  = m_ir_builder->CreateICmpULT(a, b);
    auto gt_i1  = m_ir_builder->CreateICmpUGT(a, b);
    auto eq_i1  = m_ir_builder->CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void Compiler::SetCr6AfterVectorCompare(u32 vr) {
    auto vr_v16i8    = GetVrAsIntVec(vr, 8);
    auto vr_mask_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmovmskb_128), vr_v16i8);
    auto cmp0_i1     = m_ir_builder->CreateICmpEQ(vr_mask_i32, m_ir_builder->getInt32(0));
    auto cmp1_i1     = m_ir_builder->CreateICmpEQ(vr_mask_i32, m_ir_builder->getInt32(0xFFFF));
    auto cr_i32      = GetCr();
    cr_i32           = SetNibble(cr_i32, 6, cmp1_i1, nullptr, cmp0_i1, nullptr);
    SetCr(cr_i32);
}

Value * Compiler::GetLr() {
    auto lr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, LR));
    auto lr_i64_ptr = m_ir_builder->CreateBitCast(lr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(lr_i64_ptr, 8);
}

void Compiler::SetLr(Value * val_x64) {
    auto val_i64     = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto lr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, LR));
    auto lr_i64_ptr = m_ir_builder->CreateBitCast(lr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i64, lr_i64_ptr, 8);
}

Value * Compiler::GetCtr() {
    auto ctr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, CTR));
    auto ctr_i64_ptr = m_ir_builder->CreateBitCast(ctr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(ctr_i64_ptr, 8);
}

void Compiler::SetCtr(Value * val_x64) {
    auto val_i64     = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto ctr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, CTR));
    auto ctr_i64_ptr = m_ir_builder->CreateBitCast(ctr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i64, ctr_i64_ptr, 8);
}

Value * Compiler::GetXer() {
    auto xer_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, XER));
    auto xer_i64_ptr = m_ir_builder->CreateBitCast(xer_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(xer_i64_ptr, 8);
}

Value * Compiler::GetXerCa() {
    return GetBit(GetXer(), 34);
}

Value * Compiler::GetXerSo() {
    return GetBit(GetXer(), 32);
}

void Compiler::SetXer(Value * val_x64) {
    auto val_i64     = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto xer_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, XER));
    auto xer_i64_ptr = m_ir_builder->CreateBitCast(xer_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i64, xer_i64_ptr, 8);
}

void Compiler::SetXerCa(Value * ca) {
    auto xer_i64 = GetXer();
    xer_i64      = SetBit(xer_i64, 34, ca);
    SetXer(xer_i64);
}

void Compiler::SetXerSo(Value * so) {
    auto xer_i64 = GetXer();
    xer_i64      = SetBit(xer_i64, 32, so);
    SetXer(xer_i64);
}

Value * Compiler::GetVrsave() {
    auto vrsave_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VRSAVE));
    auto vrsave_i32_ptr = m_ir_builder->CreateBitCast(vrsave_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    auto val_i32        = m_ir_builder->CreateAlignedLoad(vrsave_i32_ptr, 4);
    return m_ir_builder->CreateZExtOrTrunc(val_i32, m_ir_builder->getInt64Ty());
}

void Compiler::SetVrsave(Value * val_x64) {
    auto val_i64        = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto val_i32        = m_ir_builder->CreateZExtOrTrunc(val_i64, m_ir_builder->getInt32Ty());
    auto vrsave_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VRSAVE));
    auto vrsave_i32_ptr = m_ir_builder->CreateBitCast(vrsave_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i32, vrsave_i32_ptr, 8);
}

Value * Compiler::GetFpscr() {
    auto fpscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, FPSCR));
    auto fpscr_i32_ptr = m_ir_builder->CreateBitCast(fpscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(fpscr_i32_ptr, 4);
}

void Compiler::SetFpscr(Value * val_x32) {
    auto val_i32       = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto fpscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, FPSCR));
    auto fpscr_i32_ptr = m_ir_builder->CreateBitCast(fpscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i32, fpscr_i32_ptr, 4);
}

Value * Compiler::GetFpr(u32 r, u32 bits, bool as_int) {
    auto r_i8_ptr = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, FPR[r]));
    if (!as_int) {
        auto r_f64_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getDoubleTy()->getPointerTo());
        auto r_f64     = m_ir_builder->CreateAlignedLoad(r_f64_ptr, 8);
        if (bits == 32) {
            return m_ir_builder->CreateFPTrunc(r_f64, m_ir_builder->getFloatTy());
        } else {
            return r_f64;
        }
    } else {
        auto r_i64_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
        auto r_i64     = m_ir_builder->CreateAlignedLoad(r_i64_ptr, 8);
        if (bits == 32) {
            return m_ir_builder->CreateTrunc(r_i64, m_ir_builder->getInt32Ty());
        } else {
            return r_i64;
        }
    }
}

void Compiler::SetFpr(u32 r, Value * val) {
    auto r_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, FPR[r]));
    auto r_f64_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getDoubleTy()->getPointerTo());

    Value* val_f64;
    if (val->getType()->isDoubleTy() || val->getType()->isIntegerTy(64)) {
        val_f64 = m_ir_builder->CreateBitCast(val, m_ir_builder->getDoubleTy());
    } else if (val->getType()->isFloatTy() || val->getType()->isIntegerTy(32)) {
        auto val_f32 = m_ir_builder->CreateBitCast(val, m_ir_builder->getFloatTy());
        val_f64      = m_ir_builder->CreateFPExt(val_f32, m_ir_builder->getDoubleTy());
    } else {
        assert(0);
    }

    m_ir_builder->CreateAlignedStore(val_f64, r_f64_ptr, 8);
}

Value * Compiler::GetVscr() {
    auto vscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = m_ir_builder->CreateBitCast(vscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vscr_i32_ptr, 4);
}

void Compiler::SetVscr(Value * val_x32) {
    auto val_i32      = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto vscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = m_ir_builder->CreateBitCast(vscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i32, vscr_i32_ptr, 4);
}

Value * Compiler::GetVr(u32 vr) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vr_i128_ptr, 16);
}

Value * Compiler::GetVrAsIntVec(u32 vr, u32 vec_elt_num_bits) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_vec_ptr  = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getIntNTy(vec_elt_num_bits), 128 / vec_elt_num_bits)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vr_vec_ptr, 16);
}

Value * Compiler::GetVrAsFloatVec(u32 vr) {
    auto vr_i8_ptr    = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v4f32_ptr = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getFloatTy(), 4)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vr_v4f32_ptr, 16);
}

Value * Compiler::GetVrAsDoubleVec(u32 vr) {
    auto vr_i8_ptr    = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v2f64_ptr = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getDoubleTy(), 2)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vr_v2f64_ptr, 16);
}

void Compiler::SetVr(u32 vr, Value * val_x128) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(m_state.args[CompileTaskState::Args::State], (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto val_i128    = m_ir_builder->CreateBitCast(val_x128, m_ir_builder->getIntNTy(128));
    m_ir_builder->CreateAlignedStore(val_i128, vr_i128_ptr, 16);
}

Value * Compiler::CheckBranchCondition(u32 bo, u32 bi) {
    bool bo0 = bo & 0x10 ? true : false;
    bool bo1 = bo & 0x08 ? true : false;
    bool bo2 = bo & 0x04 ? true : false;
    bool bo3 = bo & 0x02 ? true : false;

    auto ctr_i64 = GetCtr();
    if (!bo2) {
        ctr_i64 = m_ir_builder->CreateSub(ctr_i64, m_ir_builder->getInt64(1));
        SetCtr(ctr_i64);
    }

    Value * ctr_ok_i1 = nullptr;
    if (!bo2) {
        // TODO: Check if we should compare all bits or just the lower 32 bits. This depends on MSR[SF]. Not sure what it is for PS3.
        ctr_ok_i1 = m_ir_builder->CreateICmpNE(ctr_i64, m_ir_builder->getInt64(0));
        if (bo3) {
            ctr_ok_i1 = m_ir_builder->CreateXor(ctr_ok_i1, m_ir_builder->getInt1(bo3));
        }
    }

    Value * cond_ok_i1 = nullptr;
    if (!bo0) {
        auto cr_bi_i32  = GetBit(GetCr(), bi);
        cond_ok_i1      = m_ir_builder->CreateTrunc(cr_bi_i32, m_ir_builder->getInt1Ty());
        if (!bo1) {
            cond_ok_i1 = m_ir_builder->CreateXor(cond_ok_i1, m_ir_builder->getInt1(!bo1));
        }
    }

    Value * cmp_i1 = nullptr;
    if (ctr_ok_i1 && cond_ok_i1) {
        cmp_i1 = m_ir_builder->CreateAnd(ctr_ok_i1, cond_ok_i1);
    } else if (ctr_ok_i1) {
        cmp_i1 = ctr_ok_i1;
    } else if (cond_ok_i1) {
        cmp_i1 = cond_ok_i1;
    }

    return cmp_i1;
}

void Compiler::CreateBranch(llvm::Value * cmp_i1, llvm::Value * target_i32, bool lk, bool target_is_lr) {
    if (lk) {
        SetLr(m_ir_builder->getInt64(m_state.current_instruction_address + 4));
    }

    auto current_block = m_ir_builder->GetInsertBlock();

    BasicBlock * target_block  = nullptr;
    if (dyn_cast<ConstantInt>(target_i32)) {
        // Target address is an immediate value.
        u32 target_address = (u32)(dyn_cast<ConstantInt>(target_i32)->getLimitedValue());
        if (lk) {
            // Function call
            if (cmp_i1) { // There is no need to create a new block for an unconditional jump
                target_block = GetBasicBlockFromAddress(m_state.current_instruction_address, "target");
                m_ir_builder->SetInsertPoint(target_block);
            }

            SetPc(target_i32);
            IndirectCall(target_address, m_ir_builder->getInt64(0), true);
            m_ir_builder->CreateBr(GetBasicBlockFromAddress(m_state.current_instruction_address + 4));
        } else {
            // Local branch
            target_block = GetBasicBlockFromAddress(target_address);
        }
    } else {
        // Target address is in a register
        if (cmp_i1) { // There is no need to create a new block for an unconditional jump
            target_block = GetBasicBlockFromAddress(m_state.current_instruction_address, "target");
            m_ir_builder->SetInsertPoint(target_block);
        }

        SetPc(target_i32);
        if (target_is_lr && !lk) {
            // Return from this function
            m_ir_builder->CreateRet(m_ir_builder->getInt32(0));
        } else if (lk) {
            auto next_block             = GetBasicBlockFromAddress(m_state.current_instruction_address + 4);
            auto unknown_function_block = GetBasicBlockFromAddress(m_state.current_instruction_address, "unknown_function");

            auto switch_instr = m_ir_builder->CreateSwitch(target_i32, unknown_function_block);
            m_ir_builder->SetInsertPoint(unknown_function_block);
            m_ir_builder->CreateCall2(m_execute_unknown_function, m_state.args[CompileTaskState::Args::State], m_ir_builder->getInt64(0));
            m_ir_builder->CreateBr(next_block);

            auto call_i = m_state.cfg->calls.find(m_state.current_instruction_address);
            if (call_i != m_state.cfg->calls.end()) {
                for (auto function_i = call_i->second.begin(); function_i != call_i->second.end(); function_i++) {
                    auto block = GetBasicBlockFromAddress(m_state.current_instruction_address, fmt::Format("0x%08X", *function_i));
                    m_ir_builder->SetInsertPoint(block);
                    IndirectCall(*function_i, m_ir_builder->getInt64(0), true);
                    m_ir_builder->CreateBr(next_block);
                    switch_instr->addCase(m_ir_builder->getInt32(*function_i), block);
                }
            }
        } else {
            auto switch_instr = m_ir_builder->CreateSwitch(target_i32, GetBasicBlockFromAddress(0xFFFFFFFF));
            auto branch_i     = m_state.cfg->branches.find(m_state.current_instruction_address);
            if (branch_i != m_state.cfg->branches.end()) {
                for (auto next_instr_i = branch_i->second.begin(); next_instr_i != branch_i->second.end(); next_instr_i++) {
                    switch_instr->addCase(m_ir_builder->getInt32(*next_instr_i), GetBasicBlockFromAddress(*next_instr_i));
                }
            }
        }
    }

    if (cmp_i1) {
        // Conditional branch
        auto next_block = GetBasicBlockFromAddress(m_state.current_instruction_address + 4);
        m_ir_builder->SetInsertPoint(current_block);
        m_ir_builder->CreateCondBr(cmp_i1, target_block, next_block);
    } else {
        // Unconditional branch
        if (target_block) {
            m_ir_builder->SetInsertPoint(current_block);
            m_ir_builder->CreateBr(target_block);
        }
    }

    m_state.hit_branch_instruction = true;
}

Value * Compiler::ReadMemory(Value * addr_i64, u32 bits, u32 alignment, bool bswap, bool could_be_mmio) {
    if (bits != 32 || could_be_mmio == false) {
        auto eaddr_i64    = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
        auto eaddr_ix_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getIntNTy(bits)->getPointerTo());
        auto val_ix       = (Value *)m_ir_builder->CreateLoad(eaddr_ix_ptr, alignment);
        if (bits > 8 && bswap) {
            val_ix = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, m_ir_builder->getIntNTy(bits)), val_ix);
        }

        return val_ix;
    } else {
        static u32 next_basic_block_id = 0;

        next_basic_block_id++;
        auto cmp_i1   = m_ir_builder->CreateICmpULT(addr_i64, m_ir_builder->getInt64(RAW_SPU_BASE_ADDR));
        auto then_bb  = GetBasicBlockFromAddress(m_state.current_instruction_address, fmt::Format("then_%u", next_basic_block_id));
        auto else_bb  = GetBasicBlockFromAddress(m_state.current_instruction_address, fmt::Format("else_%u", next_basic_block_id));
        auto merge_bb = GetBasicBlockFromAddress(m_state.current_instruction_address, fmt::Format("merge_%u", next_basic_block_id));
        m_ir_builder->CreateCondBr(cmp_i1, then_bb, else_bb);

        m_ir_builder->SetInsertPoint(then_bb);
        auto eaddr_i64     = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
        auto eaddr_i32_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getInt32Ty()->getPointerTo());
        auto val_then_i32  = (Value *)m_ir_builder->CreateAlignedLoad(eaddr_i32_ptr, alignment);
        if (bswap) {
            val_then_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, m_ir_builder->getInt32Ty()), val_then_i32);
        }

        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->SetInsertPoint(else_bb);
        auto val_else_i32 = Call<u32>("vm.read32", (u32(*)(u64))vm::read32, addr_i64);
        if (!bswap) {
            val_else_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, m_ir_builder->getInt32Ty()), val_else_i32);
        }
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->SetInsertPoint(merge_bb);
        auto phi = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 2);
        phi->addIncoming(val_then_i32, then_bb);
        phi->addIncoming(val_else_i32, else_bb);
        return phi;
    }
}

void Compiler::WriteMemory(Value * addr_i64, Value * val_ix, u32 alignment, bool bswap, bool could_be_mmio) {
    addr_i64 = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFF);
    if (val_ix->getType()->getIntegerBitWidth() != 32 || could_be_mmio == false) {
        if (val_ix->getType()->getIntegerBitWidth() > 8 && bswap) {
            val_ix = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, val_ix->getType()), val_ix);
        }

        auto eaddr_i64    = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
        auto eaddr_ix_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, val_ix->getType()->getPointerTo());
        m_ir_builder->CreateAlignedStore(val_ix, eaddr_ix_ptr, alignment);
    } else {
        static u32 next_basic_block_id;

        next_basic_block_id++;
        auto cmp_i1   = m_ir_builder->CreateICmpULT(addr_i64, m_ir_builder->getInt64(RAW_SPU_BASE_ADDR));
        auto then_bb  = GetBasicBlockFromAddress(m_state.current_instruction_address, fmt::Format("then_%u", next_basic_block_id));
        auto else_bb  = GetBasicBlockFromAddress(m_state.current_instruction_address, fmt::Format("else_%u", next_basic_block_id));
        auto merge_bb = GetBasicBlockFromAddress(m_state.current_instruction_address, fmt::Format("merge_%u", next_basic_block_id));
        m_ir_builder->CreateCondBr(cmp_i1, then_bb, else_bb);

        m_ir_builder->SetInsertPoint(then_bb);
        Value * val_then_i32 = val_ix;
        if (bswap) {
            val_then_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, m_ir_builder->getInt32Ty()), val_then_i32);
        }

        auto eaddr_i64     = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
        auto eaddr_i32_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getInt32Ty()->getPointerTo());
        m_ir_builder->CreateAlignedStore(val_then_i32, eaddr_i32_ptr, alignment);
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->SetInsertPoint(else_bb);
        Value * val_else_i32 = val_ix;
        if (!bswap) {
            val_else_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, m_ir_builder->getInt32Ty()), val_else_i32);
        }

        Call<void>("vm.write32", (void(*)(u32, u32))vm::write32, addr_i64, val_else_i32);
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->SetInsertPoint(merge_bb);
    }
}

template<class T>
Type * Compiler::CppToLlvmType() {
    if (std::is_void<T>::value) {
        return m_ir_builder->getVoidTy();
    } else if (std::is_same<T, long long>::value || std::is_same<T, unsigned long long>::value) {
        return m_ir_builder->getInt64Ty();
    } else if (std::is_same<T, int>::value || std::is_same<T, unsigned int>::value) {
        return m_ir_builder->getInt32Ty();
    } else if (std::is_same<T, short>::value || std::is_same<T, unsigned short>::value) {
        return m_ir_builder->getInt16Ty();
    } else if (std::is_same<T, char>::value || std::is_same<T, unsigned char>::value) {
        return m_ir_builder->getInt8Ty();
    } else if (std::is_same<T, float>::value) {
        return m_ir_builder->getFloatTy();
    } else if (std::is_same<T, double>::value) {
        return m_ir_builder->getDoubleTy();
    } else if (std::is_pointer<T>::value) {
        return m_ir_builder->getInt8PtrTy();
    } else {
        assert(0);
    }

    return nullptr;
}

template<class ReturnType, class Func, class... Args>
Value * Compiler::Call(const char * name, Func function, Args... args) {
    auto fn = m_module->getFunction(name);
    if (!fn) {
        std::vector<Type *> fn_args_type = {args->getType()...};
        auto fn_type = FunctionType::get(CppToLlvmType<ReturnType>(), fn_args_type, false);
        fn           = cast<Function>(m_module->getOrInsertFunction(name, fn_type));
        fn->setCallingConv(CallingConv::X86_64_Win64);
        m_execution_engine->addGlobalMapping(fn, (void *&)function);
    }

    std::vector<Value *> fn_args = {args...};
    return m_ir_builder->CreateCall(fn, fn_args);
}

llvm::Value * Compiler::IndirectCall(u32 address, Value * context_i64, bool is_function) {
    auto ordinal          = m_recompilation_engine.AllocateOrdinal(address, is_function);
    auto location_i64     = m_ir_builder->getInt64(m_recompilation_engine.GetAddressOfExecutableLookup() + (ordinal * sizeof(u64)));
    auto location_i64_ptr = m_ir_builder->CreateIntToPtr(location_i64, m_ir_builder->getInt64Ty()->getPointerTo());
    auto executable_i64   = m_ir_builder->CreateLoad(location_i64_ptr);
    auto executable_ptr   = m_ir_builder->CreateIntToPtr(executable_i64, m_compiled_function_type->getPointerTo());
    return m_ir_builder->CreateCall2(executable_ptr, m_state.args[CompileTaskState::Args::State], context_i64);
}

void Compiler::CompilationError(const std::string & error) {
    LOG_ERROR(PPU, "[0x%08X] %s", m_state.current_instruction_address, error.c_str());
    Emu.Pause();
}

void Compiler::InitRotateMask() {
    for (u32 mb = 0; mb < 64; mb++) {
        for (u32 me = 0; me < 64; me++) {
            u64 mask = ((u64)-1 >> mb) ^ ((me >= 63) ? 0 : (u64)-1 >> (me + 1));
            s_rotate_mask[mb][me] = mb > me ? ~mask : mask;
        }
    }
}

std::mutex                           RecompilationEngine::s_mutex;
std::shared_ptr<RecompilationEngine> RecompilationEngine::s_the_instance = nullptr;

RecompilationEngine::RecompilationEngine()
    : ThreadBase("PPU Recompilation Engine")
    , m_log(nullptr)
    , m_next_ordinal(0)
    , m_compiler(*this, ExecutionEngine::ExecuteFunction, ExecutionEngine::ExecuteTillReturn) {
    m_compiler.RunAllTests();
}

RecompilationEngine::~RecompilationEngine() {
    Stop();
}

u32 RecompilationEngine::AllocateOrdinal(u32 address, bool is_function) {
    std::lock_guard<std::mutex> lock(m_address_to_ordinal_lock);

    auto i = m_address_to_ordinal.find(address);
    if (i == m_address_to_ordinal.end()) {
        assert(m_next_ordinal < (sizeof(m_executable_lookup) / sizeof(m_executable_lookup[0])));

        m_executable_lookup[m_next_ordinal] = is_function ? ExecutionEngine::ExecuteFunction : ExecutionEngine::ExecuteTillReturn;
        std::atomic_thread_fence(std::memory_order_release);
        i = m_address_to_ordinal.insert(m_address_to_ordinal.end(), std::make_pair(address, m_next_ordinal++));
    }

    return i->second;
}

u32 RecompilationEngine::GetOrdinal(u32 address) const {
    std::lock_guard<std::mutex> lock(m_address_to_ordinal_lock);

    auto i = m_address_to_ordinal.find(address);
    if (i != m_address_to_ordinal.end()) {
        return i->second;
    } else {
        return 0xFFFFFFFF;
    }
}

const Executable RecompilationEngine::GetExecutable(u32 ordinal) const {
    std::atomic_thread_fence(std::memory_order_acquire);
    return m_executable_lookup[ordinal];
}

u64 RecompilationEngine::GetAddressOfExecutableLookup() const {
    return (u64)m_executable_lookup;
}

void RecompilationEngine::NotifyTrace(ExecutionTrace * execution_trace) {
    {
        std::lock_guard<std::mutex> lock(m_pending_execution_traces_lock);
        m_pending_execution_traces.push_back(execution_trace);
    }

    if (!IsAlive()) {
        Start();
    }

    Notify();
    // TODO: Increase the priority of the recompilation engine thread
}

raw_fd_ostream & RecompilationEngine::Log() {
    if (!m_log) {
        std::string error;
        m_log = new raw_fd_ostream("PPULLVMRecompiler.log", error, sys::fs::F_Text);
        m_log->SetUnbuffered();
    }

    return *m_log;
}

void RecompilationEngine::Task() {
    bool                     is_idling = false;
    std::chrono::nanoseconds idling_time(0);
    std::chrono::nanoseconds recompiling_time(0);

    auto start = std::chrono::high_resolution_clock::now();
    while (!TestDestroy() && !Emu.IsStopped()) {
        bool             work_done_this_iteration = false;
        ExecutionTrace * execution_trace          = nullptr;

        {
            std::lock_guard<std::mutex> lock(m_pending_execution_traces_lock);

            auto i = m_pending_execution_traces.begin();
            if (i != m_pending_execution_traces.end()) {
                execution_trace = *i;
                m_pending_execution_traces.erase(i);
            }
        }

        if (execution_trace) {
            ProcessExecutionTrace(*execution_trace);
            delete execution_trace;
            work_done_this_iteration = true;
        }

        if (!work_done_this_iteration) {
            // TODO: Reduce the priority of the recompilation engine thread if its set to high priority
        } else {
            is_idling = false;
        }

        if (is_idling) {
            auto recompiling_start = std::chrono::high_resolution_clock::now();

            // Recompile the function whose CFG has changed the most since the last time it was compiled
            auto   candidate = (BlockEntry *)nullptr;
            size_t max_diff  = 0;
            for (auto block : m_block_table) {
                if (block->IsFunction() && block->is_compiled) {
                    auto diff = block->cfg.GetSize() - block->last_compiled_cfg_size;
                    if (diff > max_diff) {
                        candidate = block;
                        max_diff  = diff;
                    }
                }
            }

            if (candidate != nullptr) {
                Log() << "Recompiling: " << candidate->ToString() << "\n";
                CompileBlock(*candidate);
                work_done_this_iteration = true;
            }

            auto recompiling_end  = std::chrono::high_resolution_clock::now();
            recompiling_time     += std::chrono::duration_cast<std::chrono::nanoseconds>(recompiling_end - recompiling_start);
        }

        if (!work_done_this_iteration) {
            is_idling = true;

            // Wait a few ms for something to happen
            auto idling_start = std::chrono::high_resolution_clock::now();
            WaitForAnySignal(250);
            auto idling_end  = std::chrono::high_resolution_clock::now();
            idling_time     += std::chrono::duration_cast<std::chrono::nanoseconds>(idling_end - idling_start);
        }
    }

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    auto total_time     = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    auto compiler_stats = m_compiler.GetStats();

    Log() << "Total time                      = " << total_time.count() / 1000000 << "ms\n";
    Log() << "    Time spent compiling        = " << compiler_stats.total_time.count() / 1000000 << "ms\n";
    Log() << "        Time spent building IR  = " << compiler_stats.ir_build_time.count() / 1000000 << "ms\n";
    Log() << "        Time spent optimizing   = " << compiler_stats.optimization_time.count() / 1000000 << "ms\n";
    Log() << "        Time spent translating  = " << compiler_stats.translation_time.count() / 1000000 << "ms\n";
    Log() << "    Time spent recompiling      = " << recompiling_time.count() / 1000000 << "ms\n";
    Log() << "    Time spent idling           = " << idling_time.count() / 1000000 << "ms\n";
    Log() << "    Time spent doing misc tasks = " << (total_time.count() - idling_time.count() - compiler_stats.total_time.count()) / 1000000 << "ms\n";
    Log() << "Ordinals allocated              = " << m_next_ordinal << "\n";

    LOG_NOTICE(PPU, "PPU LLVM Recompilation thread exiting.");
    s_the_instance = nullptr; // Can cause deadlock if this is the last instance. Need to fix this.
}

void RecompilationEngine::ProcessExecutionTrace(const ExecutionTrace & execution_trace) {
    auto execution_trace_id          = execution_trace.GetId();
    auto processed_execution_trace_i = m_processed_execution_traces.find(execution_trace_id);
    if (processed_execution_trace_i == m_processed_execution_traces.end()) {
#ifdef _DEBUG
        Log() << "Trace: " << execution_trace.ToString() << "\n";
#endif
        // Find the function block
        BlockEntry key(execution_trace.function_address, execution_trace.function_address);
        auto       block_i = m_block_table.find(&key);
        if (block_i == m_block_table.end()) {
            block_i = m_block_table.insert(m_block_table.end(), new BlockEntry(key.cfg.start_address, key.cfg.function_address));
        }

        auto function_block = *block_i;
        block_i             = m_block_table.end();
        auto split_trace    = false;
        std::vector<BlockEntry *> tmp_block_list;
        for (auto trace_i = execution_trace.entries.begin(); trace_i != execution_trace.entries.end(); trace_i++) {
            if (trace_i->type == ExecutionTraceEntry::Type::CompiledBlock) {
                block_i     = m_block_table.end();
                split_trace = true;
            }

            if (block_i == m_block_table.end()) {
                BlockEntry key(trace_i->GetPrimaryAddress(), execution_trace.function_address);
                block_i = m_block_table.find(&key);
                if (block_i == m_block_table.end()) {
                    block_i = m_block_table.insert(m_block_table.end(), new BlockEntry(key.cfg.start_address, key.cfg.function_address));
                }

                tmp_block_list.push_back(*block_i);
            }

            const ExecutionTraceEntry * next_trace = nullptr;
            if (trace_i + 1 != execution_trace.entries.end()) {
                next_trace = &(*(trace_i + 1));
            } else if (!split_trace && execution_trace.type == ExecutionTrace::Type::Loop) {
                next_trace = &(*(execution_trace.entries.begin()));
            }

            UpdateControlFlowGraph((*block_i)->cfg, *trace_i, next_trace);
            if (*block_i != function_block) {
                UpdateControlFlowGraph(function_block->cfg, *trace_i, next_trace);
            }
        }

        processed_execution_trace_i = m_processed_execution_traces.insert(m_processed_execution_traces.end(), std::make_pair(execution_trace_id, std::move(tmp_block_list)));
    }

    for (auto i = processed_execution_trace_i->second.begin(); i != processed_execution_trace_i->second.end(); i++) {
        if (!(*i)->is_compiled) {
            (*i)->num_hits++;
            if ((*i)->num_hits >= 1000) { // TODO: Make this configurable
                CompileBlock(*(*i));
            }
        }
    }

    std::remove_if(processed_execution_trace_i->second.begin(), processed_execution_trace_i->second.end(), [](const BlockEntry * b)->bool { return b->is_compiled; });
}

void RecompilationEngine::UpdateControlFlowGraph(ControlFlowGraph & cfg, const ExecutionTraceEntry & this_entry, const ExecutionTraceEntry * next_entry) {
    if (this_entry.type == ExecutionTraceEntry::Type::Instruction) {
        cfg.instruction_addresses.insert(this_entry.GetPrimaryAddress());

        if (next_entry) {
            if (next_entry->type == ExecutionTraceEntry::Type::Instruction || next_entry->type == ExecutionTraceEntry::Type::CompiledBlock) {
                if (next_entry->GetPrimaryAddress() != (this_entry.GetPrimaryAddress() + 4)) {
                    cfg.branches[this_entry.GetPrimaryAddress()].insert(next_entry->GetPrimaryAddress());
                }
            } else if (next_entry->type == ExecutionTraceEntry::Type::FunctionCall) {
                cfg.calls[this_entry.data.instruction.address].insert(next_entry->GetPrimaryAddress());
            }
        }
    } else if (this_entry.type == ExecutionTraceEntry::Type::CompiledBlock) {
        if (next_entry) {
            if (next_entry->type == ExecutionTraceEntry::Type::Instruction || next_entry->type == ExecutionTraceEntry::Type::CompiledBlock) {
                cfg.branches[this_entry.data.compiled_block.exit_address].insert(next_entry->GetPrimaryAddress());
            } else if (next_entry->type == ExecutionTraceEntry::Type::FunctionCall) {
                cfg.calls[this_entry.data.compiled_block.exit_address].insert(next_entry->GetPrimaryAddress());
            }
        }
    }
}

void RecompilationEngine::CompileBlock(BlockEntry & block_entry) {
#ifdef _DEBUG
    Log() << "Compile: " << block_entry.ToString() << "\n";
    Log() << "CFG: " << block_entry.cfg.ToString() << "\n";
#endif

    auto ordinal    = AllocateOrdinal(block_entry.cfg.start_address, block_entry.IsFunction());
    auto executable = m_compiler.Compile(fmt::Format("fn_0x%08X_%u", block_entry.cfg.start_address, block_entry.revision++), block_entry.cfg, true,
                                         block_entry.IsFunction() ? true : false /*generate_linkable_exits*/);
    m_executable_lookup[ordinal]       = executable;
    block_entry.last_compiled_cfg_size = block_entry.cfg.GetSize();
    block_entry.is_compiled            = true;
}

std::shared_ptr<RecompilationEngine> RecompilationEngine::GetInstance() {
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_the_instance == nullptr) {
        s_the_instance = std::shared_ptr<RecompilationEngine>(new RecompilationEngine());
    }

    return s_the_instance;
}

Tracer::Tracer()
    : m_recompilation_engine(RecompilationEngine::GetInstance()) {
    m_stack.reserve(100);
}

Tracer::~Tracer() {
    Terminate();
}

void Tracer::Trace(TraceType trace_type, u32 arg1, u32 arg2) {
    ExecutionTrace * execution_trace = nullptr;

    switch (trace_type) {
    case TraceType::CallFunction:
        // arg1 is address of the function
        m_stack.back()->entries.push_back(ExecutionTraceEntry(ExecutionTraceEntry::Type::FunctionCall, arg1));
        break;
    case TraceType::EnterFunction:
        // arg1 is address of the function
        m_stack.push_back(new ExecutionTrace(arg1));
        break;
    case TraceType::ExitFromCompiledFunction:
        // arg1 is address of function.
        // arg2 is the address of the exit instruction.
        if (arg2) {
            m_stack.push_back(new ExecutionTrace(arg1));
            m_stack.back()->entries.push_back(ExecutionTraceEntry(ExecutionTraceEntry::Type::CompiledBlock, arg1, arg2));
        }
        break;
    case TraceType::Return:
        // No args used
        execution_trace       = m_stack.back();
        execution_trace->type = ExecutionTrace::Type::Linear;
        m_stack.pop_back();
        break;
    case TraceType::Instruction:
        // arg1 is the address of the instruction
        for (int i = (int)m_stack.back()->entries.size() - 1; i >= 0; i--) {
            if ((m_stack.back()->entries[i].type == ExecutionTraceEntry::Type::Instruction && m_stack.back()->entries[i].data.instruction.address == arg1) ||
                (m_stack.back()->entries[i].type == ExecutionTraceEntry::Type::CompiledBlock && m_stack.back()->entries[i].data.compiled_block.entry_address == arg1)) {
                // Found a loop
                execution_trace       = new ExecutionTrace(m_stack.back()->function_address);
                execution_trace->type = ExecutionTrace::Type::Loop;
                std::copy(m_stack.back()->entries.begin() + i, m_stack.back()->entries.end(), std::back_inserter(execution_trace->entries));
                m_stack.back()->entries.erase(m_stack.back()->entries.begin() + i + 1, m_stack.back()->entries.end());
                break;
            }
        }

        if (!execution_trace) {
            // A loop was not found
            m_stack.back()->entries.push_back(ExecutionTraceEntry(ExecutionTraceEntry::Type::Instruction, arg1));
        }
        break;
    case TraceType::ExitFromCompiledBlock:
        // arg1 is address of the compiled block.
        // arg2 is the address of the exit instruction.
        m_stack.back()->entries.push_back(ExecutionTraceEntry(ExecutionTraceEntry::Type::CompiledBlock, arg1, arg2));

        if (arg2 == 0) {
            // Return from function
            execution_trace       = m_stack.back();
            execution_trace->type = ExecutionTrace::Type::Linear;
            m_stack.pop_back();
        }
        break;
    default:
        assert(0);
        break;
    }

    if (execution_trace) {
        m_recompilation_engine->NotifyTrace(execution_trace);
    }
}

void Tracer::Terminate() {
    // TODO: Notify recompilation engine
}

ppu_recompiler_llvm::ExecutionEngine::ExecutionEngine(PPUThread & ppu)
    : m_ppu(ppu)
    , m_interpreter(new PPUInterpreter(ppu))
    , m_decoder(m_interpreter)
    , m_last_cache_clear_time(std::chrono::high_resolution_clock::now())
    , m_recompilation_engine(RecompilationEngine::GetInstance()) {
}

ppu_recompiler_llvm::ExecutionEngine::~ExecutionEngine() {

}

u32 ppu_recompiler_llvm::ExecutionEngine::DecodeMemory(const u32 address) {
    ExecuteFunction(&m_ppu, 0);
    return 0;
}

void ppu_recompiler_llvm::ExecutionEngine::RemoveUnusedEntriesFromCache() const {
    auto now = std::chrono::high_resolution_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_cache_clear_time).count() > 10000) {
        for (auto i = m_address_to_ordinal.begin(); i != m_address_to_ordinal.end();) {
            auto tmp = i;
            i++;
            if (tmp->second.second == 0) {
                m_address_to_ordinal.erase(tmp);
            } else {
                tmp->second.second = 0;
            }
        }

        m_last_cache_clear_time = now;
    }
}

Executable ppu_recompiler_llvm::ExecutionEngine::GetExecutable(u32 address, Executable default_executable) const {
    // Find the ordinal for the specified address and insert it to the cache
    auto i = m_address_to_ordinal.find(address);
    if (i == m_address_to_ordinal.end()) {
        auto ordinal = m_recompilation_engine->GetOrdinal(address);
        if (ordinal != 0xFFFFFFFF) {
            i = m_address_to_ordinal.insert(m_address_to_ordinal.end(), std::make_pair(address, std::make_pair(ordinal, 0)));
        }
    }

    Executable executable = default_executable;
    if (i != m_address_to_ordinal.end()) {
        i->second.second++;
        executable = m_recompilation_engine->GetExecutable(i->second.first);
    }

    RemoveUnusedEntriesFromCache();
    return executable;
}

u32 ppu_recompiler_llvm::ExecutionEngine::ExecuteFunction(PPUThread * ppu_state, u64 context) {
    auto execution_engine = (ExecutionEngine *)ppu_state->GetDecoder();
    execution_engine->m_tracer.Trace(Tracer::TraceType::EnterFunction, ppu_state->PC, 0);
    return ExecuteTillReturn(ppu_state, 0);
}

u32 ppu_recompiler_llvm::ExecutionEngine::ExecuteTillReturn(PPUThread * ppu_state, u64 context) {
    auto execution_engine = (ExecutionEngine *)ppu_state->GetDecoder();
    auto terminate        = false;
    auto branch_type      = BranchType::NonBranch;

    if (context) {
        execution_engine->m_tracer.Trace(Tracer::TraceType::ExitFromCompiledFunction, context >> 32, context & 0xFFFFFFFF);
    }

    while (!terminate && !Emu.IsStopped()) {
        if (Emu.IsPaused()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        auto executable = execution_engine->GetExecutable(ppu_state->PC, ExecuteTillReturn);
        if (executable != ExecuteTillReturn && executable != ExecuteFunction) {
            auto entry = ppu_state->PC;
            auto exit  = (u32)executable(ppu_state, 0);
            execution_engine->m_tracer.Trace(Tracer::TraceType::ExitFromCompiledBlock, entry, exit);
            if (exit == 0) {
                terminate = true;
            }
        } else {
            execution_engine->m_tracer.Trace(Tracer::TraceType::Instruction, ppu_state->PC, 0);
            auto instruction = re32(vm::get_ref<u32>(ppu_state->PC));
            execution_engine->m_decoder.Decode(instruction);
            branch_type = ppu_state->m_is_branch ? GetBranchTypeFromInstruction(instruction) : BranchType::NonBranch;
            ppu_state->NextPc(4);

            switch (branch_type) {
            case BranchType::Return:
                execution_engine->m_tracer.Trace(Tracer::TraceType::Return, 0, 0);
                terminate = true;
                break;
            case BranchType::FunctionCall:
                execution_engine->m_tracer.Trace(Tracer::TraceType::CallFunction, ppu_state->PC, 0);
                executable = execution_engine->GetExecutable(ppu_state->PC, ExecuteFunction);
                executable(ppu_state, 0);
                break;
            case BranchType::LocalBranch:
                break;
            case BranchType::NonBranch:
                break;
            default:
                assert(0);
                break;
            }
        }
    }

    return 0;
}

BranchType ppu_recompiler_llvm::GetBranchTypeFromInstruction(u32 instruction) {
    auto type   = BranchType::NonBranch;
    auto field1 = instruction >> 26;
    auto lk     = instruction & 1;

    if (field1 == 16 || field1 == 18) {
        type = lk ? BranchType::FunctionCall : BranchType::LocalBranch;
    } else if (field1 == 19) {
        u32 field2 = (instruction >> 1) & 0x3FF;
        if (field2 == 16) {
            type = lk ? BranchType::FunctionCall : BranchType::Return;
        } else if (field2 == 528) {
            type = lk ? BranchType::FunctionCall : BranchType::LocalBranch;
        }
    }

    return type;
}
