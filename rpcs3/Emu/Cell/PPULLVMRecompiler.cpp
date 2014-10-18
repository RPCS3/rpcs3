#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
#include "Emu/Memory/Memory.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
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

using namespace llvm;

u64  PPULLVMRecompiler::s_rotate_mask[64][64];
bool PPULLVMRecompiler::s_rotate_mask_inited = false;

PPULLVMRecompiler::PPULLVMRecompiler()
    : ThreadBase("PPULLVMRecompiler")
    , m_revision(0) {
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

    if (!s_rotate_mask_inited) {
        InitRotateMask();
        s_rotate_mask_inited = true;
    }
}

PPULLVMRecompiler::~PPULLVMRecompiler() {
    Stop();

    delete m_execution_engine;
    delete m_fpm;
    delete m_ir_builder;
    delete m_llvm_context;
}

std::pair<PPULLVMRecompiler::Executable, u32> PPULLVMRecompiler::GetExecutable(u32 address) {
    std::lock_guard<std::mutex> lock(m_compiled_shared_lock);

    auto compiled = m_compiled_shared.lower_bound(std::make_pair(address, 0));
    if (compiled != m_compiled_shared.end() && compiled->first.first == address) {
        compiled->second.second++;
        return std::make_pair(compiled->second.first, compiled->first.second);
    }

    return std::make_pair(nullptr, 0);
}

void PPULLVMRecompiler::ReleaseExecutable(u32 address, u32 revision) {
    std::lock_guard<std::mutex> lock(m_compiled_shared_lock);

    auto compiled = m_compiled_shared.find(std::make_pair(address, revision));
    if (compiled != m_compiled_shared.end()) {
        compiled->second.second--;
    }
}

void PPULLVMRecompiler::RequestCompilation(u32 address) {
    {
        std::lock_guard<std::mutex> lock(m_uncompiled_shared_lock);
        m_uncompiled_shared.push_back(address);
    }

    if (!IsAlive()) {
        Start();
    }

    Notify();
}

u32 PPULLVMRecompiler::GetCurrentRevision() {
    return m_revision.load(std::memory_order_relaxed);
}

void PPULLVMRecompiler::Task() {
    auto start = std::chrono::high_resolution_clock::now();

    while (!TestDestroy() && !Emu.IsStopped()) {
        // Wait a few ms for something to happen
        auto idling_start = std::chrono::high_resolution_clock::now();
        WaitForAnySignal(250);
        auto idling_end = std::chrono::high_resolution_clock::now();
        m_idling_time += std::chrono::duration_cast<std::chrono::nanoseconds>(idling_end - idling_start);

        // Update the set of blocks that have been hit with the set of blocks that have been requested for compilation.
        {
            std::lock_guard<std::mutex> lock(m_uncompiled_shared_lock);
            for (auto i = m_uncompiled_shared.begin(); i != m_uncompiled_shared.end(); i++) {
                m_hit_blocks.insert(*i);
            }
        }

        u32 num_compiled = 0;
        while (!TestDestroy() && !Emu.IsStopped()) {
            u32 address;

            {
                std::lock_guard<std::mutex> lock(m_uncompiled_shared_lock);

                auto i = m_uncompiled_shared.begin();
                if (i != m_uncompiled_shared.end()) {
                    address = *i;
                    m_uncompiled_shared.erase(i);
                } else {
                    break;
                }
            }

            m_hit_blocks.insert(address);
            if (NeedsCompiling(address)) {
                Compile(address);
                num_compiled++;
            }
        }

        if (num_compiled == 0) {
            // If we get here, it means the recompilation thread is idling.
            // We use this oppurtunity to optimize the code.
            RemoveUnusedOldVersions();
            for (auto i = m_compiled.begin(); i != m_compiled.end(); i++) {
                if (NeedsCompiling(i->first.first)) {
                    Compile(i->first.first);
                    num_compiled++;
                }
            }
        }
    }

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    m_total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

    std::string error;
    raw_fd_ostream log_file("PPULLVMRecompiler.log", error, sys::fs::F_Text);
    log_file << "Total time                      = " << m_total_time.count() / 1000000 << "ms\n";
    log_file << "    Time spent compiling        = " << m_compilation_time.count() / 1000000 << "ms\n";
    log_file << "        Time spent building IR  = " << m_ir_build_time.count() / 1000000 << "ms\n";
    log_file << "        Time spent optimizing   = " << m_optimizing_time.count() / 1000000 << "ms\n";
    log_file << "        Time spent translating  = " << m_translation_time.count() / 1000000 << "ms\n";
    log_file << "    Time spent idling           = " << m_idling_time.count() / 1000000 << "ms\n";
    log_file << "    Time spent doing misc tasks = " << (m_total_time.count() - m_idling_time.count() - m_compilation_time.count()) / 1000000 << "ms\n";
    log_file << "Revision                        = " << m_revision << "\n";
    log_file << "\nInterpreter fallback stats:\n";
    for (auto i = m_interpreter_fallback_stats.begin(); i != m_interpreter_fallback_stats.end(); i++) {
        log_file << i->first << " = " << i->second << "\n";
    }

    log_file << "\nDisassembly:\n";
    //auto disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);
    for (auto i = m_compiled.begin(); i != m_compiled.end(); i++) {
        log_file << fmt::Format("%s: Size = %u bytes, Number of instructions = %u\n", i->second.llvm_function->getName().str().c_str(), i->second.size, i->second.num_instructions);

        //uint8_t * fn_ptr = (uint8_t *)i->second.executable;
        //for (size_t pc = 0; pc < i->second.size;) {
        //    char str[1024];

        //    auto size = LLVMDisasmInstruction(disassembler, fn_ptr + pc, i->second.size - pc, (uint64_t)(fn_ptr + pc), str, sizeof(str));
        //    log_file << str << '\n';
        //    pc += size;
        //}
    }

    //LLVMDisasmDispose(disassembler);

    //log_file << "\nLLVM IR:\n" << *m_module;

    LOG_NOTICE(PPU, "PPU LLVM compiler thread exiting.");
}

void PPULLVMRecompiler::Decode(const u32 code) {
    (*PPU_instr::main_list)(this, code);
}

void PPULLVMRecompiler::NULL_OP() {
    InterpreterCall("NULL_OP", &PPUInterpreter::NULL_OP);
}

void PPULLVMRecompiler::NOP() {
    InterpreterCall("NOP", &PPUInterpreter::NOP);
}

void PPULLVMRecompiler::TDI(u32 to, u32 ra, s32 simm16) {
    InterpreterCall("TDI", &PPUInterpreter::TDI, to, ra, simm16);
}

void PPULLVMRecompiler::TWI(u32 to, u32 ra, s32 simm16) {
    InterpreterCall("TWI", &PPUInterpreter::TWI, to, ra, simm16);
}

void PPULLVMRecompiler::MFVSCR(u32 vd) {
    auto vscr_i32  = GetVscr();
    auto vscr_i128 = m_ir_builder->CreateZExt(vscr_i32, m_ir_builder->getIntNTy(128));
    SetVr(vd, vscr_i128);
}

void PPULLVMRecompiler::MTVSCR(u32 vb) {
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);
    auto vscr_i32 = m_ir_builder->CreateExtractElement(vb_v4i32, m_ir_builder->getInt32(0));
    vscr_i32      = m_ir_builder->CreateAnd(vscr_i32, 0x00010001);
    SetVscr(vscr_i32);
}

void PPULLVMRecompiler::VADDCUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    va_v4i32      = m_ir_builder->CreateNot(va_v4i32);
    auto cmpv4i1  = m_ir_builder->CreateICmpULT(va_v4i32, vb_v4i32);
    auto cmpv4i32 = m_ir_builder->CreateZExt(cmpv4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmpv4i32);
}

void PPULLVMRecompiler::VADDFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto sum_v4f32 = m_ir_builder->CreateFAdd(va_v4f32, vb_v4f32);
    SetVr(vd, sum_v4f32);
}

void PPULLVMRecompiler::VADDSBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_padds_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set VSCR.SAT
}

void PPULLVMRecompiler::VADDSHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_padds_w), va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);

    // TODO: Set VSCR.SAT
}

void PPULLVMRecompiler::VADDSWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    // It looks like x86 does not have an instruction to add 32 bit intergers with signed/unsigned saturation.
    // To implement add with saturation, we first determine what the result would be if the operation were to cause
    // an overflow. If two -ve numbers are being added and cause an overflow, the result would be 0x80000000.
    // If two +ve numbers are being added and cause an overflow, the result would be 0x7FFFFFFF. Addition of a -ve
    // number and a +ve number cannot cause overflow. So the result in case of an overflow is 0x7FFFFFFF + sign bit
    // of any one of the operands.
    u32  tmp1_v4i32[4] = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    auto tmp2_v4i32    = m_ir_builder->CreateLShr(va_v4i32, 31);
    tmp2_v4i32         = m_ir_builder->CreateAdd(tmp2_v4i32, ConstantDataVector::get(m_ir_builder->getContext(), tmp1_v4i32));
    auto tmp2_v16i8    = m_ir_builder->CreateBitCast(tmp2_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // Next, we find if the addition can actually result in an overflow. Since an overflow can only happen if the operands
    // have the same sign, we bitwise XOR both the operands. If the sign bit of the result is 0 then the operands have the
    // same sign and so may cause an overflow. We invert the result so that the sign bit is 1 when the operands have the
    // same sign.
    auto tmp3_v4i32        = m_ir_builder->CreateXor(va_v4i32, vb_v4i32);
    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    tmp3_v4i32             = m_ir_builder->CreateXor(tmp3_v4i32, ConstantDataVector::get(m_ir_builder->getContext(), not_mask_v4i32));

    // Perform the sum.
    auto sum_v4i32 = m_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    auto sum_v16i8 = m_ir_builder->CreateBitCast(sum_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // If an overflow occurs, then the sign of the sum will be different from the sign of the operands. So, we xor the
    // result with one of the operands. The sign bit of the result will be 1 if the sign bit of the sum and the sign bit of the
    // result is different. This result is again ANDed with tmp3 (the sign bit of tmp3 is 1 only if the operands have the same
    // sign and so can cause an overflow).
    auto tmp4_v4i32 = m_ir_builder->CreateXor(va_v4i32, sum_v4i32);
    tmp4_v4i32      = m_ir_builder->CreateAnd(tmp3_v4i32, tmp4_v4i32);
    tmp4_v4i32      = m_ir_builder->CreateAShr(tmp4_v4i32, 31);
    auto tmp4_v16i8 = m_ir_builder->CreateBitCast(tmp4_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // tmp4 is equal to 0xFFFFFFFF if an overflow occured and 0x00000000 otherwise.
    auto res_v16i8 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pblendvb), sum_v16i8, tmp2_v16i8, tmp4_v16i8);
    SetVr(vd, res_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUBM(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateAdd(va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);
}

void PPULLVMRecompiler::VADDUBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_paddus_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUHM(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateAdd(va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);
}

void PPULLVMRecompiler::VADDUHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_paddus_w), va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUWM(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = m_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    SetVr(vd, sum_v4i32);
}

void PPULLVMRecompiler::VADDUWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = m_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpULT(sum_v4i32, va_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto res_v4i32 = m_ir_builder->CreateOr(sum_v4i32, cmp_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VAND(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VANDC(u32 vd, u32 va, u32 vb) {
    auto va_v4i32          = GetVrAsIntVec(va, 32);
    auto vb_v4i32          = GetVrAsIntVec(vb, 32);
    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    vb_v4i32               = m_ir_builder->CreateXor(vb_v4i32, ConstantDataVector::get(m_ir_builder->getContext(), not_mask_v4i32));
    auto res_v4i32         = m_ir_builder->CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VAVGSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8       = GetVrAsIntVec(va, 8);
    auto vb_v16i8       = GetVrAsIntVec(vb, 8);
    auto va_v16i16      = m_ir_builder->CreateSExt(va_v16i8, VectorType::get(m_ir_builder->getInt16Ty(), 16));
    auto vb_v16i16      = m_ir_builder->CreateSExt(vb_v16i8, VectorType::get(m_ir_builder->getInt16Ty(), 16));
    auto sum_v16i16     = m_ir_builder->CreateAdd(va_v16i16, vb_v16i16);
    u16  one_v16i16[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    sum_v16i16          = m_ir_builder->CreateAdd(sum_v16i16, ConstantDataVector::get(m_ir_builder->getContext(), one_v16i16));
    auto avg_v16i16     = m_ir_builder->CreateAShr(sum_v16i16, 1);
    auto avg_v16i8      = m_ir_builder->CreateTrunc(avg_v16i16, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, avg_v16i8);
}

void PPULLVMRecompiler::VAVGSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16     = GetVrAsIntVec(va, 16);
    auto vb_v8i16     = GetVrAsIntVec(vb, 16);
    auto va_v8i32     = m_ir_builder->CreateSExt(va_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vb_v8i32     = m_ir_builder->CreateSExt(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto sum_v8i32    = m_ir_builder->CreateAdd(va_v8i32, vb_v8i32);
    u32  one_v8i32[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    sum_v8i32         = m_ir_builder->CreateAdd(sum_v8i32, ConstantDataVector::get(m_ir_builder->getContext(), one_v8i32));
    auto avg_v8i32    = m_ir_builder->CreateAShr(sum_v8i32, 1);
    auto avg_v8i16    = m_ir_builder->CreateTrunc(avg_v8i32, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, avg_v8i16);
}

void PPULLVMRecompiler::VAVGSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32     = GetVrAsIntVec(va, 32);
    auto vb_v4i32     = GetVrAsIntVec(vb, 32);
    auto va_v4i64     = m_ir_builder->CreateSExt(va_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto vb_v4i64     = m_ir_builder->CreateSExt(vb_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto sum_v4i64    = m_ir_builder->CreateAdd(va_v4i64, vb_v4i64);
    u64  one_v4i64[4] = {1, 1, 1, 1};
    sum_v4i64         = m_ir_builder->CreateAdd(sum_v4i64, ConstantDataVector::get(m_ir_builder->getContext(), one_v4i64));
    auto avg_v4i64    = m_ir_builder->CreateAShr(sum_v4i64, 1);
    auto avg_v4i32    = m_ir_builder->CreateTrunc(avg_v4i64, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, avg_v4i32);
}

void PPULLVMRecompiler::VAVGUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto avg_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pavg_b), va_v16i8, vb_v16i8);
    SetVr(vd, avg_v16i8);
}

void PPULLVMRecompiler::VAVGUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto avg_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pavg_w), va_v8i16, vb_v8i16);
    SetVr(vd, avg_v8i16);
}

void PPULLVMRecompiler::VAVGUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32     = GetVrAsIntVec(va, 32);
    auto vb_v4i32     = GetVrAsIntVec(vb, 32);
    auto va_v4i64     = m_ir_builder->CreateZExt(va_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto vb_v4i64     = m_ir_builder->CreateZExt(vb_v4i32, VectorType::get(m_ir_builder->getInt64Ty(), 4));
    auto sum_v4i64    = m_ir_builder->CreateAdd(va_v4i64, vb_v4i64);
    u64  one_v4i64[4] = {1, 1, 1, 1};
    sum_v4i64         = m_ir_builder->CreateAdd(sum_v4i64, ConstantDataVector::get(m_ir_builder->getContext(), one_v4i64));
    auto avg_v4i64    = m_ir_builder->CreateLShr(sum_v4i64, 1);
    auto avg_v4i32    = m_ir_builder->CreateTrunc(avg_v4i64, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, avg_v4i32);
}

void PPULLVMRecompiler::VCFSX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder->CreateSIToFP(vb_v4i32, VectorType::get(m_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale          = (float)((u64)1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = m_ir_builder->CreateFDiv(res_v4f32, ConstantDataVector::get(m_ir_builder->getContext(), scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VCFUX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder->CreateUIToFP(vb_v4i32, VectorType::get(m_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale          = (float)((u64)1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = m_ir_builder->CreateFDiv(res_v4f32, ConstantDataVector::get(m_ir_builder->getContext(), scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VCMPBFP(u32 vd, u32 va, u32 vb) {
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

void PPULLVMRecompiler::VCMPBFP_(u32 vd, u32 va, u32 vb) {
    VCMPBFP(vd, va, vb);

    auto vd_v16i8     = GetVrAsIntVec(vd, 8);
    u8 mask_v16i8[16] = {3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vd_v16i8          = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_ssse3_pshuf_b_128), vd_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i8));
    auto vd_v4i32     = m_ir_builder->CreateBitCast(vd_v16i8, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto vd_mask_i32  = m_ir_builder->CreateExtractElement(vd_v4i32, m_ir_builder->getInt32(0));
    auto cmp_i1       = m_ir_builder->CreateICmpEQ(vd_mask_i32, m_ir_builder->getInt32(0));
    SetCrField(6, nullptr, nullptr, cmp_i1, nullptr);
}

void PPULLVMRecompiler::VCMPEQFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOEQ(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPEQFP_(u32 vd, u32 va, u32 vb) {
    VCMPEQFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpEQ(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPEQUB_(u32 vd, u32 va, u32 vb) {
    VCMPEQUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpEQ(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPEQUH_(u32 vd, u32 va, u32 vb) {
    VCMPEQUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpEQ(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPEQUW_(u32 vd, u32 va, u32 vb) {
    VCMPEQUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGEFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOGE(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGEFP_(u32 vd, u32 va, u32 vb) {
    VCMPGEFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOGT(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTFP_(u32 vd, u32 va, u32 vb) {
    VCMPGTFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpSGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPGTSB_(u32 vd, u32 va, u32 vb) {
    VCMPGTSB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpSGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPGTSH_(u32 vd, u32 va, u32 vb) {
    VCMPGTSH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpSGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTSW_(u32 vd, u32 va, u32 vb) {
    VCMPGTSW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpUGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPGTUB_(u32 vd, u32 va, u32 vb) {
    VCMPGTUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpUGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPGTUH_(u32 vd, u32 va, u32 vb) {
    VCMPGTUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpUGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTUW_(u32 vd, u32 va, u32 vb) {
    VCMPGTUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCTSXS(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VCTSXS", &PPUInterpreter::VCTSXS, vd, uimm5, vb);
}

void PPULLVMRecompiler::VCTUXS(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VCTUXS", &PPUInterpreter::VCTUXS, vd, uimm5, vb);
}

void PPULLVMRecompiler::VEXPTEFP(u32 vd, u32 vb) {
    InterpreterCall("VEXPTEFP", &PPUInterpreter::VEXPTEFP, vd, vb);
}

void PPULLVMRecompiler::VLOGEFP(u32 vd, u32 vb) {
    InterpreterCall("VLOGEFP", &PPUInterpreter::VLOGEFP, vd, vb);
}

void PPULLVMRecompiler::VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto vc_v4f32  = GetVrAsFloatVec(vc);
    auto res_v4f32 = m_ir_builder->CreateFMul(va_v4f32, vc_v4f32);
    res_v4f32      = m_ir_builder->CreateFAdd(res_v4f32, vb_v4f32);
    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VMAXFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse_max_ps), va_v4f32, vb_v4f32);
    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VMAXSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pmaxsb), va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VMAXSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmaxs_w), va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VMAXSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pmaxsd), va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VMAXUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmaxu_b), va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VMAXUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pmaxuw), va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VMAXUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pmaxud), va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMHADDSHS", &PPUInterpreter::VMHADDSHS, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMHRADDSHS", &PPUInterpreter::VMHRADDSHS, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMINFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse_min_ps), va_v4f32, vb_v4f32);
    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VMINSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminsb), va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VMINSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmins_w), va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VMINSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminsd), va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VMINUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto res_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pminu_b), va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VMINUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminuw), va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VMINUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pminud), va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMLADDUHM", &PPUInterpreter::VMLADDUHM, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMRGHB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8        = GetVrAsIntVec(va, 8);
    auto vb_v16i8        = GetVrAsIntVec(vb, 8);
    u32  mask_v16i32[16] = {24, 8, 25, 9, 26, 10, 27, 11, 28, 12, 29, 13, 30, 14, 31, 15};
    auto vd_v16i8        = m_ir_builder->CreateShuffleVector(va_v16i8, vb_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    SetVr(vd, vd_v16i8);
}

void PPULLVMRecompiler::VMRGHH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16      = GetVrAsIntVec(va, 16);
    auto vb_v8i16      = GetVrAsIntVec(vb, 16);
    u32  mask_v8i32[8] = {12, 4, 13, 5, 14, 6, 15, 7};
    auto vd_v8i16      = m_ir_builder->CreateShuffleVector(va_v8i16, vb_v8i16, ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));
    SetVr(vd, vd_v8i16);
}

void PPULLVMRecompiler::VMRGHW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32      = GetVrAsIntVec(va, 32);
    auto vb_v4i32      = GetVrAsIntVec(vb, 32);
    u32  mask_v4i32[4] = {6, 2, 7, 3};
    auto vd_v4i32      = m_ir_builder->CreateShuffleVector(va_v4i32, vb_v4i32, ConstantDataVector::get(m_ir_builder->getContext(), mask_v4i32));
    SetVr(vd, vd_v4i32);
}

void PPULLVMRecompiler::VMRGLB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8        = GetVrAsIntVec(va, 8);
    auto vb_v16i8        = GetVrAsIntVec(vb, 8);
    u32  mask_v16i32[16] = {16, 0, 17, 1, 18, 2, 19, 3, 20, 4, 21, 5, 22, 6, 23, 7};
    auto vd_v16i8        = m_ir_builder->CreateShuffleVector(va_v16i8, vb_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    SetVr(vd, vd_v16i8);
}

void PPULLVMRecompiler::VMRGLH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16      = GetVrAsIntVec(va, 16);
    auto vb_v8i16      = GetVrAsIntVec(vb, 16);
    u32  mask_v8i32[8] = {8, 0, 9, 1, 10, 2, 11, 3};
    auto vd_v8i16      = m_ir_builder->CreateShuffleVector(va_v8i16, vb_v8i16, ConstantDataVector::get(m_ir_builder->getContext(), mask_v8i32));
    SetVr(vd, vd_v8i16);
}

void PPULLVMRecompiler::VMRGLW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32      = GetVrAsIntVec(va, 32);
    auto vb_v4i32      = GetVrAsIntVec(vb, 32);
    u32  mask_v4i32[4] = {4, 0, 5, 1};
    auto vd_v4i32      = m_ir_builder->CreateShuffleVector(va_v4i32, vb_v4i32, ConstantDataVector::get(m_ir_builder->getContext(), mask_v4i32));
    SetVr(vd, vd_v4i32);
}

void PPULLVMRecompiler::VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) {
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

void PPULLVMRecompiler::VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto va_v8i32  = m_ir_builder->CreateSExt(va_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
    auto vb_v8i32  = m_ir_builder->CreateSExt(vb_v8i16, VectorType::get(m_ir_builder->getInt32Ty(), 8));
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

void PPULLVMRecompiler::VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMSHS", &PPUInterpreter::VMSUMSHS, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) {
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

void PPULLVMRecompiler::VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) {
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

void PPULLVMRecompiler::VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMUHS", &PPUInterpreter::VMSUMUHS, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMULESB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULESB", &PPUInterpreter::VMULESB, vd, va, vb);
}

void PPULLVMRecompiler::VMULESH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULESH", &PPUInterpreter::VMULESH, vd, va, vb);
}

void PPULLVMRecompiler::VMULEUB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULEUB", &PPUInterpreter::VMULEUB, vd, va, vb);
}

void PPULLVMRecompiler::VMULEUH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULEUH", &PPUInterpreter::VMULEUH, vd, va, vb);
}

void PPULLVMRecompiler::VMULOSB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULOSB", &PPUInterpreter::VMULOSB, vd, va, vb);
}

void PPULLVMRecompiler::VMULOSH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULOSH", &PPUInterpreter::VMULOSH, vd, va, vb);
}

void PPULLVMRecompiler::VMULOUB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULOUB", &PPUInterpreter::VMULOUB, vd, va, vb);
}

void PPULLVMRecompiler::VMULOUH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULOUH", &PPUInterpreter::VMULOUH, vd, va, vb);
}

void PPULLVMRecompiler::VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto vc_v4f32  = GetVrAsFloatVec(vc);
    auto res_v4f32 = m_ir_builder->CreateFMul(va_v4f32, vc_v4f32);
    res_v4f32      = m_ir_builder->CreateFSub(res_v4f32, vb_v4f32);
    res_v4f32      = m_ir_builder->CreateFNeg(res_v4f32);
    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VNOR(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateOr(va_v8i16, vb_v8i16);
    res_v8i16      = m_ir_builder->CreateNot(res_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VOR(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateOr(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VPERM(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v16i8 = GetVrAsIntVec(va, 8);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto vc_v16i8 = GetVrAsIntVec(vc, 8);

    u8 thrity_one_v16i8[16] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};
    vc_v16i8                = m_ir_builder->CreateAnd(vc_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), thrity_one_v16i8));

    u8   fifteen_v16i8[16] = {15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15};
    auto vc_le15_v16i8     = m_ir_builder->CreateSub(ConstantDataVector::get(m_ir_builder->getContext(), fifteen_v16i8), vc_v16i8);
    auto res_va_v16i8      = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_ssse3_pshuf_b_128), va_v16i8, vc_le15_v16i8);

    auto vc_gt15_v16i8 = m_ir_builder->CreateSub(ConstantDataVector::get(m_ir_builder->getContext(), thrity_one_v16i8), vc_v16i8);
    auto cmp_i1        = m_ir_builder->CreateICmpUGT(vc_gt15_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), fifteen_v16i8));
    auto cmp_i8        = m_ir_builder->CreateSExt(cmp_i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    vc_gt15_v16i8      = m_ir_builder->CreateOr(cmp_i8, vc_gt15_v16i8);
    auto res_vb_v16i8  = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_ssse3_pshuf_b_128), vb_v16i8, vc_gt15_v16i8);

    auto res_v16i8 = m_ir_builder->CreateOr(res_vb_v16i8, res_va_v16i8);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VPKPX(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKPX", &PPUInterpreter::VPKPX, vd, va, vb);
}

void PPULLVMRecompiler::VPKSHSS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKSHSS", &PPUInterpreter::VPKSHSS, vd, va, vb);
}

void PPULLVMRecompiler::VPKSHUS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKSHUS", &PPUInterpreter::VPKSHUS, vd, va, vb);
}

void PPULLVMRecompiler::VPKSWSS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKSWSS", &PPUInterpreter::VPKSWSS, vd, va, vb);
}

void PPULLVMRecompiler::VPKSWUS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKSWUS", &PPUInterpreter::VPKSWUS, vd, va, vb);
}

void PPULLVMRecompiler::VPKUHUM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKUHUM", &PPUInterpreter::VPKUHUM, vd, va, vb);
}

void PPULLVMRecompiler::VPKUHUS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKUHUS", &PPUInterpreter::VPKUHUS, vd, va, vb);
}

void PPULLVMRecompiler::VPKUWUM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKUWUM", &PPUInterpreter::VPKUWUM, vd, va, vb);
}

void PPULLVMRecompiler::VPKUWUS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKUWUS", &PPUInterpreter::VPKUWUS, vd, va, vb);
}

void PPULLVMRecompiler::VREFP(u32 vd, u32 vb) {
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto res_v4f32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse_rcp_ps), vb_v4f32);
    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VRFIM(u32 vd, u32 vb) {
    InterpreterCall("VRFIM", &PPUInterpreter::VRFIM, vd, vb);
}

void PPULLVMRecompiler::VRFIN(u32 vd, u32 vb) {
    InterpreterCall("VRFIN", &PPUInterpreter::VRFIN, vd, vb);
}

void PPULLVMRecompiler::VRFIP(u32 vd, u32 vb) {
    InterpreterCall("VRFIP", &PPUInterpreter::VRFIP, vd, vb);
}

void PPULLVMRecompiler::VRFIZ(u32 vd, u32 vb) {
    InterpreterCall("VRFIZ", &PPUInterpreter::VRFIZ, vd, vb);
}

void PPULLVMRecompiler::VRLB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VRLB", &PPUInterpreter::VRLB, vd, va, vb);
}

void PPULLVMRecompiler::VRLH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VRLH", &PPUInterpreter::VRLH, vd, va, vb);
}

void PPULLVMRecompiler::VRLW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VRLW", &PPUInterpreter::VRLW, vd, va, vb);
}

void PPULLVMRecompiler::VRSQRTEFP(u32 vd, u32 vb) {
    InterpreterCall("VRSQRTEFP", &PPUInterpreter::VRSQRTEFP, vd, vb);
}

void PPULLVMRecompiler::VSEL(u32 vd, u32 va, u32 vb, u32 vc) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);
    auto vc_v4i32 = GetVrAsIntVec(vc, 32);
    vb_v4i32      = m_ir_builder->CreateAnd(vb_v4i32, vc_v4i32);
    vc_v4i32      = m_ir_builder->CreateNot(vc_v4i32);
    va_v4i32      = m_ir_builder->CreateAnd(va_v4i32, vc_v4i32);
    auto vd_v4i32 = m_ir_builder->CreateOr(va_v4i32, vb_v4i32);
    SetVr(vd, vd_v4i32);
}

void PPULLVMRecompiler::VSL(u32 vd, u32 va, u32 vb) {
    auto va_i128  = GetVr(va);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto sh_i8    = m_ir_builder->CreateExtractElement(vb_v16i8, m_ir_builder->getInt8(0));
    sh_i8         = m_ir_builder->CreateAnd(sh_i8, 0x7);
    auto sh_i128  = m_ir_builder->CreateZExt(sh_i8, m_ir_builder->getIntNTy(128));
    va_i128       = m_ir_builder->CreateShl(va_i128, sh_i128);
    SetVr(vd, va_i128);
}

void PPULLVMRecompiler::VSLB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    vb_v16i8       = m_ir_builder->CreateAnd(vb_v16i8, m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(0x7)));
    auto res_v16i8 = m_ir_builder->CreateShl(va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) {
    auto va_v16i8        = GetVrAsIntVec(va, 8);
    auto vb_v16i8        = GetVrAsIntVec(vb, 8);
    sh                   = 16 - sh;
    u32  mask_v16i32[16] = {sh, sh + 1, sh + 2, sh + 3, sh + 4, sh + 5, sh + 6, sh + 7, sh + 8, sh + 9, sh + 10, sh + 11, sh + 12, sh + 13, sh + 14, sh + 15};
    auto vd_v16i8        = m_ir_builder->CreateShuffleVector(vb_v16i8, va_v16i8, ConstantDataVector::get(m_ir_builder->getContext(), mask_v16i32));
    SetVr(vd, vd_v16i8);
}

void PPULLVMRecompiler::VSLH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    vb_v8i16       = m_ir_builder->CreateAnd(vb_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xF)));
    auto res_v8i16 = m_ir_builder->CreateShl(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VSLO(u32 vd, u32 va, u32 vb) {
    auto va_i128  = GetVr(va);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto sh_i8    = m_ir_builder->CreateExtractElement(vb_v16i8, m_ir_builder->getInt8(0));
    sh_i8         = m_ir_builder->CreateAnd(sh_i8, 0x78);
    auto sh_i128  = m_ir_builder->CreateZExt(sh_i8, m_ir_builder->getIntNTy(128));
    va_i128       = m_ir_builder->CreateShl(va_i128, sh_i128);
    SetVr(vd, va_i128);
}

void PPULLVMRecompiler::VSLW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    vb_v4i32       = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x1F)));
    auto res_v4i32 = m_ir_builder->CreateShl(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VSPLTB(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v16i8    = GetVrAsIntVec(vb, 8);
    auto undef_v16i8 = UndefValue::get(VectorType::get(m_ir_builder->getInt8Ty(), 16));
    auto mask_v16i32 = m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt32(15 - uimm5));
    auto res_v16i8   = m_ir_builder->CreateShuffleVector(vb_v16i8, undef_v16i8, mask_v16i32);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VSPLTH(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v8i16    = GetVrAsIntVec(vb, 16);
    auto undef_v8i16 = UndefValue::get(VectorType::get(m_ir_builder->getInt16Ty(), 8));
    auto mask_v8i32  = m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt32(7 - uimm5));
    auto res_v8i16   = m_ir_builder->CreateShuffleVector(vb_v8i16, undef_v8i16, mask_v8i32);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VSPLTISB(u32 vd, s32 simm5) {
    auto vd_v16i8 = m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8((s8)simm5));
    SetVr(vd, vd_v16i8);
}

void PPULLVMRecompiler::VSPLTISH(u32 vd, s32 simm5) {
    auto vd_v8i16 = m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16((s16)simm5));
    SetVr(vd, vd_v8i16);
}

void PPULLVMRecompiler::VSPLTISW(u32 vd, s32 simm5) {
    auto vd_v4i32 = m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32((s32)simm5));
    SetVr(vd, vd_v4i32);
}

void PPULLVMRecompiler::VSPLTW(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32    = GetVrAsIntVec(vb, 32);
    auto undef_v4i32 = UndefValue::get(VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto mask_v4i32  = m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(3 - uimm5));
    auto res_v4i32   = m_ir_builder->CreateShuffleVector(vb_v4i32, undef_v4i32, mask_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VSR(u32 vd, u32 va, u32 vb) {
    auto va_i128  = GetVr(va);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto sh_i8    = m_ir_builder->CreateExtractElement(vb_v16i8, m_ir_builder->getInt8(0));
    sh_i8         = m_ir_builder->CreateAnd(sh_i8, 0x7);
    auto sh_i128  = m_ir_builder->CreateZExt(sh_i8, m_ir_builder->getIntNTy(128));
    va_i128       = m_ir_builder->CreateLShr(va_i128, sh_i128);
    SetVr(vd, va_i128);
}

void PPULLVMRecompiler::VSRAB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    vb_v16i8       = m_ir_builder->CreateAnd(vb_v16i8, m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(0x7)));
    auto res_v16i8 = m_ir_builder->CreateAShr(va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VSRAH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    vb_v8i16       = m_ir_builder->CreateAnd(vb_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xF)));
    auto res_v8i16 = m_ir_builder->CreateAShr(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VSRAW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    vb_v4i32       = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x1F)));
    auto res_v4i32 = m_ir_builder->CreateAShr(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VSRB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    vb_v16i8       = m_ir_builder->CreateAnd(vb_v16i8, m_ir_builder->CreateVectorSplat(16, m_ir_builder->getInt8(0x7)));
    auto res_v16i8 = m_ir_builder->CreateLShr(va_v16i8, vb_v16i8);
    SetVr(vd, res_v16i8);
}

void PPULLVMRecompiler::VSRH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    vb_v8i16       = m_ir_builder->CreateAnd(vb_v8i16, m_ir_builder->CreateVectorSplat(8, m_ir_builder->getInt16(0xF)));
    auto res_v8i16 = m_ir_builder->CreateLShr(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::VSRO(u32 vd, u32 va, u32 vb) {
    auto va_i128  = GetVr(va);
    auto vb_v16i8 = GetVrAsIntVec(vb, 8);
    auto sh_i8    = m_ir_builder->CreateExtractElement(vb_v16i8, m_ir_builder->getInt8(0));
    sh_i8         = m_ir_builder->CreateAnd(sh_i8, 0x78);
    auto sh_i128  = m_ir_builder->CreateZExt(sh_i8, m_ir_builder->getIntNTy(128));
    va_i128       = m_ir_builder->CreateLShr(va_i128, sh_i128);
    SetVr(vd, va_i128);
}

void PPULLVMRecompiler::VSRW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    vb_v4i32       = m_ir_builder->CreateAnd(vb_v4i32, m_ir_builder->CreateVectorSplat(4, m_ir_builder->getInt32(0x1F)));
    auto res_v4i32 = m_ir_builder->CreateLShr(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VSUBCUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    auto cmpv4i1  = m_ir_builder->CreateICmpUGE(va_v4i32, vb_v4i32);
    auto cmpv4i32 = m_ir_builder->CreateZExt(cmpv4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmpv4i32);
}

void PPULLVMRecompiler::VSUBFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32   = GetVrAsFloatVec(va);
    auto vb_v4f32   = GetVrAsFloatVec(vb);
    auto diff_v4f32 = m_ir_builder->CreateFSub(va_v4f32, vb_v4f32);
    SetVr(vd, diff_v4f32);
}

void PPULLVMRecompiler::VSUBSBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto diff_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_psubs_b), va_v16i8, vb_v16i8);
    SetVr(vd, diff_v16i8);

    // TODO: Set VSCR.SAT
}

void PPULLVMRecompiler::VSUBSHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16   = GetVrAsIntVec(va, 16);
    auto vb_v8i16   = GetVrAsIntVec(vb, 16);
    auto diff_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_psubs_w), va_v8i16, vb_v8i16);
    SetVr(vd, diff_v8i16);

    // TODO: Set VSCR.SAT
}

void PPULLVMRecompiler::VSUBSWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    // See the comments for VADDSWS for a detailed description of how this works

    // Find the result in case of an overflow
    u32  tmp1_v4i32[4] = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    auto tmp2_v4i32    = m_ir_builder->CreateLShr(va_v4i32, 31);
    tmp2_v4i32         = m_ir_builder->CreateAdd(tmp2_v4i32, ConstantDataVector::get(m_ir_builder->getContext(), tmp1_v4i32));
    auto tmp2_v16i8    = m_ir_builder->CreateBitCast(tmp2_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // Find the elements that can overflow (elements with opposite sign bits)
    auto tmp3_v4i32 = m_ir_builder->CreateXor(va_v4i32, vb_v4i32);

    // Perform the sub
    auto diff_v4i32 = m_ir_builder->CreateSub(va_v4i32, vb_v4i32);
    auto diff_v16i8 = m_ir_builder->CreateBitCast(diff_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // Find the elements that overflowed
    auto tmp4_v4i32 = m_ir_builder->CreateXor(va_v4i32, diff_v4i32);
    tmp4_v4i32      = m_ir_builder->CreateAnd(tmp3_v4i32, tmp4_v4i32);
    tmp4_v4i32      = m_ir_builder->CreateAShr(tmp4_v4i32, 31);
    auto tmp4_v16i8 = m_ir_builder->CreateBitCast(tmp4_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // tmp4 is equal to 0xFFFFFFFF if an overflow occured and 0x00000000 otherwise.
    auto res_v16i8 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pblendvb), diff_v16i8, tmp2_v16i8, tmp4_v16i8);
    SetVr(vd, res_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VSUBUBM(u32 vd, u32 va, u32 vb) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto diff_v16i8 = m_ir_builder->CreateSub(va_v16i8, vb_v16i8);
    SetVr(vd, diff_v16i8);
}

void PPULLVMRecompiler::VSUBUBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8   = GetVrAsIntVec(va, 8);
    auto vb_v16i8   = GetVrAsIntVec(vb, 8);
    auto diff_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_psubus_b), va_v16i8, vb_v16i8);
    SetVr(vd, diff_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VSUBUHM(u32 vd, u32 va, u32 vb) {
    auto va_v8i16   = GetVrAsIntVec(va, 16);
    auto vb_v8i16   = GetVrAsIntVec(vb, 16);
    auto diff_v8i16 = m_ir_builder->CreateSub(va_v8i16, vb_v8i16);
    SetVr(vd, diff_v8i16);
}

void PPULLVMRecompiler::VSUBUHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16   = GetVrAsIntVec(va, 16);
    auto vb_v8i16   = GetVrAsIntVec(vb, 16);
    auto diff_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_psubus_w), va_v8i16, vb_v8i16);
    SetVr(vd, diff_v8i16);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VSUBUWM(u32 vd, u32 va, u32 vb) {
    auto va_v4i32   = GetVrAsIntVec(va, 32);
    auto vb_v4i32   = GetVrAsIntVec(vb, 32);
    auto diff_v4i32 = m_ir_builder->CreateSub(va_v4i32, vb_v4i32);
    SetVr(vd, diff_v4i32);
}

void PPULLVMRecompiler::VSUBUWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32   = GetVrAsIntVec(va, 32);
    auto vb_v4i32   = GetVrAsIntVec(vb, 32);
    auto diff_v4i32 = m_ir_builder->CreateSub(va_v4i32, vb_v4i32);
    auto cmp_v4i1   = m_ir_builder->CreateICmpULE(diff_v4i32, va_v4i32);
    auto cmp_v4i32  = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto res_v4i32  = m_ir_builder->CreateAnd(diff_v4i32, cmp_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VSUMSWS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUMSWS", &PPUInterpreter::VSUMSWS, vd, va, vb);
}

void PPULLVMRecompiler::VSUM2SWS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUM2SWS", &PPUInterpreter::VSUM2SWS, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4SBS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUM4SBS", &PPUInterpreter::VSUM4SBS, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4SHS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUM4SHS", &PPUInterpreter::VSUM4SHS, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4UBS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUM4UBS", &PPUInterpreter::VSUM4UBS, vd, va, vb);
}

void PPULLVMRecompiler::VUPKHPX(u32 vd, u32 vb) {
    InterpreterCall("VUPKHPX", &PPUInterpreter::VUPKHPX, vd, vb);
}

void PPULLVMRecompiler::VUPKHSB(u32 vd, u32 vb) {
    InterpreterCall("VUPKHSB", &PPUInterpreter::VUPKHSB, vd, vb);
}

void PPULLVMRecompiler::VUPKHSH(u32 vd, u32 vb) {
    InterpreterCall("VUPKHSH", &PPUInterpreter::VUPKHSH, vd, vb);
}

void PPULLVMRecompiler::VUPKLPX(u32 vd, u32 vb) {
    InterpreterCall("VUPKLPX", &PPUInterpreter::VUPKLPX, vd, vb);
}

void PPULLVMRecompiler::VUPKLSB(u32 vd, u32 vb) {
    InterpreterCall("VUPKLSB", &PPUInterpreter::VUPKLSB, vd, vb);
}

void PPULLVMRecompiler::VUPKLSH(u32 vd, u32 vb) {
    InterpreterCall("VUPKLSH", &PPUInterpreter::VUPKLSH, vd, vb);
}

void PPULLVMRecompiler::VXOR(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto res_v8i16 = m_ir_builder->CreateXor(va_v8i16, vb_v8i16);
    SetVr(vd, res_v8i16);
}

void PPULLVMRecompiler::MULLI(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64  = GetGpr(ra);
    auto res_i64 = m_ir_builder->CreateMul(ra_i64, m_ir_builder->getInt64((s64)simm16));
    SetGpr(rd, res_i64);
    //InterpreterCall("MULLI", &PPUInterpreter::MULLI, rd, ra, simm16);
}

void PPULLVMRecompiler::SUBFIC(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64   = GetGpr(ra);
    ra_i64        = m_ir_builder->CreateNeg(ra_i64);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, {m_ir_builder->getInt64Ty()}), ra_i64, m_ir_builder->getInt64((s64)simm16));
    auto diff_i64 = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, diff_i64);
    SetXerCa(carry_i1);
    //InterpreterCall("SUBFIC", &PPUInterpreter::SUBFIC, rd, ra, simm16);
}

void PPULLVMRecompiler::CMPLI(u32 crfd, u32 l, u32 ra, u32 uimm16) {
    Value * ra_i64;
    if (l == 0) {
        ra_i64 = m_ir_builder->CreateZExt(GetGpr(ra, 32), m_ir_builder->getInt64Ty());
    } else {
        ra_i64 = GetGpr(ra);
    }

    SetCrFieldUnsignedCmp(crfd, ra_i64, m_ir_builder->getInt64(uimm16));
    //InterpreterCall("CMPLI", &PPUInterpreter::CMPLI, crfd, l, ra, uimm16);
}

void PPULLVMRecompiler::CMPI(u32 crfd, u32 l, u32 ra, s32 simm16) {
    Value * ra_i64;
    if (l == 0) {
        ra_i64 = m_ir_builder->CreateSExt(GetGpr(ra, 32), m_ir_builder->getInt64Ty());
    } else {
        ra_i64 = GetGpr(ra);
    }

    SetCrFieldSignedCmp(crfd, ra_i64, m_ir_builder->getInt64((s64)simm16));
    //InterpreterCall("CMPI", &PPUInterpreter::CMPI, crfd, l, ra, simm16);
}

void PPULLVMRecompiler::ADDIC(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64   = GetGpr(ra);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, {m_ir_builder->getInt64Ty()}), m_ir_builder->getInt64((s64)simm16), ra_i64);
    auto sum_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, sum_i64);
    SetXerCa(carry_i1);
    //InterpreterCall("ADDIC", &PPUInterpreter::ADDIC, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDIC_(u32 rd, u32 ra, s32 simm16) {
    ADDIC(rd, ra, simm16);
    SetCrFieldSignedCmp(0, GetGpr(rd), m_ir_builder->getInt64(0));
    //InterpreterCall("ADDIC_", &PPUInterpreter::ADDIC_, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDI(u32 rd, u32 ra, s32 simm16) {
    if (ra == 0) {
        SetGpr(rd, m_ir_builder->getInt64((s64)simm16));
    } else {
        auto ra_i64  = GetGpr(ra);
        auto sum_i64 = m_ir_builder->CreateAdd(ra_i64, m_ir_builder->getInt64((s64)simm16));
        SetGpr(rd, sum_i64);
    }
    //InterpreterCall("ADDI", &PPUInterpreter::ADDI, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDIS(u32 rd, u32 ra, s32 simm16) {
    if (ra == 0) {
        SetGpr(rd, m_ir_builder->getInt64((s64)simm16 << 16));
    } else {
        auto ra_i64  = GetGpr(ra);
        auto sum_i64 = m_ir_builder->CreateAdd(ra_i64, m_ir_builder->getInt64((s64)simm16 << 16));
        SetGpr(rd, sum_i64);
    }
    //InterpreterCall("ADDIS", &PPUInterpreter::ADDIS, rd, ra, simm16);
}

void PPULLVMRecompiler::BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) {
    auto target_i64 = m_ir_builder->getInt64(branchTarget(aa ? 0 : m_current_instruction_address, bd));
    CreateBranch(CheckBranchCondition(bo, bi), target_i64, lk ? true : false);
    //m_hit_branch_instruction = true;
    //SetPc(m_ir_builder->getInt32(m_current_instruction_address));
    //InterpreterCall("BC", &PPUInterpreter::BC, bo, bi, bd, aa, lk);
    //SetPc(m_ir_builder->getInt32(m_current_instruction_address + 4));
    //m_ir_builder->CreateRetVoid();
}

void PPULLVMRecompiler::SC(u32 sc_code) {
    InterpreterCall("SC", &PPUInterpreter::SC, sc_code);
}

void PPULLVMRecompiler::B(s32 ll, u32 aa, u32 lk) {
    auto target_i64 = m_ir_builder->getInt64(branchTarget(aa ? 0 : m_current_instruction_address, ll));
    CreateBranch(nullptr, target_i64, lk ? true : false);
    //m_hit_branch_instruction = true;
    //SetPc(m_ir_builder->getInt32(m_current_instruction_address));
    //InterpreterCall("B", &PPUInterpreter::B, ll, aa, lk);
    //m_ir_builder->CreateRetVoid();
}

void PPULLVMRecompiler::MCRF(u32 crfd, u32 crfs) {
    if (crfd != crfs) {
        auto cr_i32  = GetCr();
        auto crf_i32 = GetNibble(cr_i32, crfs);
        cr_i32       = SetNibble(cr_i32, crfd, crf_i32);
        SetCr(cr_i32);
    }
    //InterpreterCall("MCRF", &PPUInterpreter::MCRF, crfd, crfs);
}

void PPULLVMRecompiler::BCLR(u32 bo, u32 bi, u32 bh, u32 lk) {
    auto lr_i64 = GetLr();
    lr_i64      = m_ir_builder->CreateAnd(lr_i64, ~0x3ULL);
    CreateBranch(CheckBranchCondition(bo, bi), lr_i64, lk ? true : false);
    //m_hit_branch_instruction = true;
    //SetPc(m_ir_builder->getInt32(m_current_instruction_address));
    //InterpreterCall("BCLR", &PPUInterpreter::BCLR, bo, bi, bh, lk);
    //SetPc(m_ir_builder->getInt32(m_current_instruction_address + 4));
    //m_ir_builder->CreateRetVoid();
}

void PPULLVMRecompiler::CRNOR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateOr(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRNOR", &PPUInterpreter::CRNOR, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRANDC(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(bb_i32, 1);
    res_i32      = m_ir_builder->CreateAnd(ba_i32, res_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRANDC", &PPUInterpreter::CRANDC, crbd, crba, crbb);
}

void PPULLVMRecompiler::ISYNC() {
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_mfence));
    //InterpreterCall("ISYNC", &PPUInterpreter::ISYNC);
}

void PPULLVMRecompiler::CRXOR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRXOR", &PPUInterpreter::CRXOR, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRNAND(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateAnd(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRNAND", &PPUInterpreter::CRNAND, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRAND(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateAnd(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRAND", &PPUInterpreter::CRAND, crbd, crba, crbb);
}

void PPULLVMRecompiler::CREQV(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CREQV", &PPUInterpreter::CREQV, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRORC(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(bb_i32, 1);
    res_i32      = m_ir_builder->CreateOr(ba_i32, res_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRORC", &PPUInterpreter::CRORC, crbd, crba, crbb);
}

void PPULLVMRecompiler::CROR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateOr(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CROR", &PPUInterpreter::CROR, crbd, crba, crbb);
}

void PPULLVMRecompiler::BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) {
    auto ctr_i64 = GetCtr();
    ctr_i64      = m_ir_builder->CreateAnd(ctr_i64, ~0x3ULL);
    CreateBranch(CheckBranchCondition(bo, bi), ctr_i64, lk ? true : false);
    //m_hit_branch_instruction = true;
    //SetPc(m_ir_builder->getInt32(m_current_instruction_address));
    //InterpreterCall("BCCTR", &PPUInterpreter::BCCTR, bo, bi, bh, lk);
    //SetPc(m_ir_builder->getInt32(m_current_instruction_address + 4));
    //m_ir_builder->CreateRetVoid();
}

void PPULLVMRecompiler::RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
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
    //InterpreterCall("RLWIMI", &PPUInterpreter::RLWIMI, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompiler::RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
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
    //InterpreterCall("RLWINM", &PPUInterpreter::RLWINM, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompiler::RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc) {
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
    //InterpreterCall("RLWNM", &PPUInterpreter::RLWNM, ra, rs, rb, mb, me, rc);
}

void PPULLVMRecompiler::ORI(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, uimm16);
    SetGpr(ra, res_i64);
    //InterpreterCall("ORI", &PPUInterpreter::ORI, ra, rs, uimm16);
}

void PPULLVMRecompiler::ORIS(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
    //InterpreterCall("ORIS", &PPUInterpreter::ORIS, ra, rs, uimm16);
}

void PPULLVMRecompiler::XORI(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, uimm16);
    SetGpr(ra, res_i64);
    //InterpreterCall("XORI", &PPUInterpreter::XORI, ra, rs, uimm16);
}

void PPULLVMRecompiler::XORIS(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
    //InterpreterCall("XORIS", &PPUInterpreter::XORIS, ra, rs, uimm16);
}

void PPULLVMRecompiler::ANDI_(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, uimm16);
    SetGpr(ra, res_i64);
    SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    //InterpreterCall("ANDI_", &PPUInterpreter::ANDI_, ra, rs, uimm16);
}

void PPULLVMRecompiler::ANDIS_(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
    SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    //InterpreterCall("ANDIS_", &PPUInterpreter::ANDIS_, ra, rs, uimm16);
}

void PPULLVMRecompiler::RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
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
    //InterpreterCall("RLDICL", &PPUInterpreter::RLDICL, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) {
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
    //InterpreterCall("RLDICR", &PPUInterpreter::RLDICR, ra, rs, sh, me, rc);
}

void PPULLVMRecompiler::RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
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
    //InterpreterCall("RLDIC", &PPUInterpreter::RLDIC, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
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
    //InterpreterCall("RLDIMI", &PPUInterpreter::RLDIMI, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) {
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
    //InterpreterCall("RLDC_LR", &PPUInterpreter::RLDC_LR, ra, rs, rb, m_eb, is_r, rc);
}

void PPULLVMRecompiler::CMP(u32 crfd, u32 l, u32 ra, u32 rb) {
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
    //InterpreterCall("CMP", &PPUInterpreter::CMP, crfd, l, ra, rb);
}

void PPULLVMRecompiler::TW(u32 to, u32 ra, u32 rb) {
    InterpreterCall("TW", &PPUInterpreter::TW, to, ra, rb);
}

void PPULLVMRecompiler::LVSL(u32 vd, u32 ra, u32 rb) {
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

    //InterpreterCall("LVSL", &PPUInterpreter::LVSL, vd, ra, rb);
}

void PPULLVMRecompiler::LVEBX(u32 vd, u32 ra, u32 rb) {
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

    //InterpreterCall("LVEBX", &PPUInterpreter::LVEBX, vd, ra, rb);
}

void PPULLVMRecompiler::SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64    = GetGpr(ra);
    ra_i64         = m_ir_builder->CreateNeg(ra_i64);
    auto rb_i64    = GetGpr(rb);
    auto res_s     = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, {m_ir_builder->getInt64Ty()}), ra_i64, rb_i64);
    auto diff_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1  = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, diff_i64);
    SetXerCa(carry_i1);

    if (rc) {
        SetCrFieldSignedCmp(0, diff_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
    }
    //InterpreterCall("SUBFC", &PPUInterpreter::SUBFC, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto rb_i64   = GetGpr(rb);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, {m_ir_builder->getInt64Ty()}), ra_i64, rb_i64);
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
    //InterpreterCall("ADDC", &PPUInterpreter::ADDC, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MULHDU(u32 rd, u32 ra, u32 rb, bool rc) {
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

    //InterpreterCall("MULHDU", &PPUInterpreter::MULHDU, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MULHWU(u32 rd, u32 ra, u32 rb, bool rc) {
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
    //InterpreterCall("MULHWU", &PPUInterpreter::MULHWU, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MFOCRF(u32 a, u32 rd, u32 crm) {
    auto cr_i32 = GetCr();
    auto cr_i64 = m_ir_builder->CreateZExt(cr_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, cr_i64);
    //InterpreterCall("MFOCRF", &PPUInterpreter::MFOCRF, a, rd, crm);
}

void PPULLVMRecompiler::LWARX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto resv_addr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, R_ADDR));
    auto resv_addr_i64_ptr = m_ir_builder->CreateBitCast(resv_addr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(addr_i64, resv_addr_i64_ptr, 8);

    auto resv_val_i32     = ReadMemory(addr_i64, 32, 4, false, false);
    auto resv_val_i64     = m_ir_builder->CreateZExt(resv_val_i32, m_ir_builder->getInt64Ty());
    auto resv_val_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, R_VALUE));
    auto resv_val_i64_ptr = m_ir_builder->CreateBitCast(resv_val_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(resv_val_i64, resv_val_i64_ptr, 8);

    resv_val_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), resv_val_i32);
    resv_val_i64 = m_ir_builder->CreateZExt(resv_val_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, resv_val_i64);
    //InterpreterCall("LWARX", &PPUInterpreter::LWARX, rd, ra, rb);
}

void PPULLVMRecompiler::LDX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    //InterpreterCall("LDX", &PPUInterpreter::LDX, rd, ra, rb);
}

void PPULLVMRecompiler::LWZX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LWZX", &PPUInterpreter::LWZX, rd, ra, rb);
}

void PPULLVMRecompiler::SLW(u32 ra, u32 rs, u32 rb, bool rc) {
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

    //InterpreterCall("SLW", &PPUInterpreter::SLW, ra, rs, rb, rc);
}

void PPULLVMRecompiler::CNTLZW(u32 ra, u32 rs, bool rc) {
    auto rs_i32  = GetGpr(rs, 32);
    auto res_i32 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::ctlz, {m_ir_builder->getInt32Ty()}), rs_i32, m_ir_builder->getInt1(false));
    auto res_i64 = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    //InterpreterCall("CNTLZW", &PPUInterpreter::CNTLZW, ra, rs, rc);
}

void PPULLVMRecompiler::SLD(u32 ra, u32 rs, u32 rb, bool rc) {
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

    //InterpreterCall("SLD", &PPUInterpreter::SLD, ra, rs, rb, rc);
}

void PPULLVMRecompiler::AND(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("AND", &PPUInterpreter::AND, ra, rs, rb, rc);
}

void PPULLVMRecompiler::CMPL(u32 crfd, u32 l, u32 ra, u32 rb) {
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
    //InterpreterCall("CMPL", &PPUInterpreter::CMPL, crfd, l, ra, rb);
}

void PPULLVMRecompiler::LVSR(u32 vd, u32 ra, u32 rb) {
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

    //InterpreterCall("LVSR", &PPUInterpreter::LVSR, vd, ra, rb);
}

void PPULLVMRecompiler::LVEHX(u32 vd, u32 ra, u32 rb) {
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

    //InterpreterCall("LVEHX", &PPUInterpreter::LVEHX, vd, ra, rb);
}

void PPULLVMRecompiler::SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto rb_i64   = GetGpr(rb);
    auto diff_i64 = m_ir_builder->CreateSub(rb_i64, ra_i64);
    SetGpr(rd, diff_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, diff_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
    }
    //InterpreterCall("SUBF", &PPUInterpreter::SUBF, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::LDUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LDUX", &PPUInterpreter::LDUX, rd, ra, rb);
}

void PPULLVMRecompiler::DCBST(u32 ra, u32 rb) {
    // TODO: Implement this
    //InterpreterCall("DCBST", &PPUInterpreter::DCBST, ra, rb);
}

void PPULLVMRecompiler::LWZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LWZUX", &PPUInterpreter::LWZUX, rd, ra, rb);
}

void PPULLVMRecompiler::CNTLZD(u32 ra, u32 rs, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::ctlz, {m_ir_builder->getInt64Ty()}), rs_i64, m_ir_builder->getInt1(false));
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    //InterpreterCall("CNTLZD", &PPUInterpreter::CNTLZD, ra, rs, rc);
}

void PPULLVMRecompiler::ANDC(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("ANDC", &PPUInterpreter::ANDC, ra, rs, rb, rc);
}

void PPULLVMRecompiler::TD(u32 to, u32 ra, u32 rb) {
    InterpreterCall("TD", &PPUInterpreter::TD, to, ra, rb);
}

void PPULLVMRecompiler::LVEWX(u32 vd, u32 ra, u32 rb) {
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

    //InterpreterCall("LVEWX", &PPUInterpreter::LVEWX, vd, ra, rb);
}

void PPULLVMRecompiler::MULHD(u32 rd, u32 ra, u32 rb, bool rc) {
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

    //InterpreterCall("MULHD", &PPUInterpreter::MULHD, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MULHW(u32 rd, u32 ra, u32 rb, bool rc) {
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
    //InterpreterCall("MULHW", &PPUInterpreter::MULHW, rd, ra, rb, rc);
}

void PPULLVMRecompiler::LDARX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto resv_addr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, R_ADDR));
    auto resv_addr_i64_ptr = m_ir_builder->CreateBitCast(resv_addr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(addr_i64, resv_addr_i64_ptr, 8);

    auto resv_val_i64     = ReadMemory(addr_i64, 64, 8, false);
    auto resv_val_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, R_VALUE));
    auto resv_val_i64_ptr = m_ir_builder->CreateBitCast(resv_val_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(resv_val_i64, resv_val_i64_ptr, 8);

    resv_val_i64      = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt64Ty()}), resv_val_i64);
    SetGpr(rd, resv_val_i64);
    //InterpreterCall("LDARX", &PPUInterpreter::LDARX, rd, ra, rb);
}

void PPULLVMRecompiler::DCBF(u32 ra, u32 rb) {
    // TODO: Implement this
    //InterpreterCall("DCBF", &PPUInterpreter::DCBF, ra, rb);
}

void PPULLVMRecompiler::LBZX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LBZX", &PPUInterpreter::LBZX, rd, ra, rb);
}

void PPULLVMRecompiler::LVX(u32 vd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64      = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFF0ULL);
    auto mem_i128 = ReadMemory(addr_i64, 128, 16);
    SetVr(vd, mem_i128);
    //InterpreterCall("LVX", &PPUInterpreter::LVX, vd, ra, rb);
}

void PPULLVMRecompiler::NEG(u32 rd, u32 ra, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto diff_i64 = m_ir_builder->CreateSub(m_ir_builder->getInt64(0), ra_i64);
    SetGpr(rd, diff_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, diff_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
    }
    //InterpreterCall("NEG", &PPUInterpreter::NEG, rd, ra, oe, rc);
}

void PPULLVMRecompiler::LBZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LBZUX", &PPUInterpreter::LBZUX, rd, ra, rb);
}

void PPULLVMRecompiler::NOR(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, rb_i64);
    res_i64      = m_ir_builder->CreateXor(res_i64, (s64)-1);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("NOR", &PPUInterpreter::NOR, ra, rs, rb, rc);
}

void PPULLVMRecompiler::STVEBX(u32 vs, u32 ra, u32 rb) {
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
    //InterpreterCall("STVEBX", &PPUInterpreter::STVEBX, vs, ra, rb);
}

void PPULLVMRecompiler::SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("SUBFE", &PPUInterpreter::SUBFE, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("ADDE", &PPUInterpreter::ADDE, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MTOCRF(u32 l, u32 crm, u32 rs) {
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
    //InterpreterCall("MTOCRF", &PPUInterpreter::MTOCRF, l, crm, rs);
}

void PPULLVMRecompiler::STDX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 64));
    //InterpreterCall("STDX", &PPUInterpreter::STDX, rs, ra, rb);
}

void PPULLVMRecompiler::STWCX_(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("STWCX_", &PPUInterpreter::STWCX_, rs, ra, rb);
}

void PPULLVMRecompiler::STWX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32));
    //InterpreterCall("STWX", &PPUInterpreter::STWX, rs, ra, rb);
}

void PPULLVMRecompiler::STVEHX(u32 vs, u32 ra, u32 rb) {
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
    //InterpreterCall("STVEHX", &PPUInterpreter::STVEHX, vs, ra, rb);
}

void PPULLVMRecompiler::STDUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 64));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STDUX", &PPUInterpreter::STDUX, rs, ra, rb);
}

void PPULLVMRecompiler::STWUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 32));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STWUX", &PPUInterpreter::STWUX, rs, ra, rb);
}

void PPULLVMRecompiler::STVEWX(u32 vs, u32 ra, u32 rb) {
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
    //InterpreterCall("STVEWX", &PPUInterpreter::STVEWX, vs, ra, rb);
}

void PPULLVMRecompiler::ADDZE(u32 rd, u32 ra, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto ca_i64   = GetXerCa();
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::uadd_with_overflow, {m_ir_builder->getInt64Ty()}), ra_i64, ca_i64);
    auto sum_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, sum_i64);
    SetXerCa(carry_i1);
     
    if (rc) {
        SetCrFieldSignedCmp(0, sum_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("ADDZE", &PPUInterpreter::ADDZE, rd, ra, oe, rc);
}

void PPULLVMRecompiler::SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("SUBFZE", &PPUInterpreter::SUBFZE, rd, ra, oe, rc);
}

void PPULLVMRecompiler::STDCX_(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("STDCX_", &PPUInterpreter::STDCX_, rs, ra, rb);
}

void PPULLVMRecompiler::STBX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 8));
    //InterpreterCall("STBX", &PPUInterpreter::STBX, rs, ra, rb);
}

void PPULLVMRecompiler::STVX(u32 vs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    addr_i64 = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFFFFFFFFF0ULL);
    WriteMemory(addr_i64, GetVr(vs), 16);
    //InterpreterCall("STVX", &PPUInterpreter::STVX, vs, ra, rb);
}

void PPULLVMRecompiler::SUBFME(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("SUBFME", &PPUInterpreter::SUBFME, rd, ra, oe, rc);
}

void PPULLVMRecompiler::MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64   = GetGpr(ra);
    auto rb_i64   = GetGpr(rb);
    auto prod_i64 = m_ir_builder->CreateMul(ra_i64, rb_i64);
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }

    // TODO implement oe
    //InterpreterCall("MULLD", &PPUInterpreter::MULLD, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDME(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("ADDME", &PPUInterpreter::ADDME, rd, ra, oe, rc);
}

void PPULLVMRecompiler::MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i32   = GetGpr(ra, 32);
    auto rb_i32   = GetGpr(rb, 32);
    auto ra_i64   = m_ir_builder->CreateSExt(ra_i32, m_ir_builder->getInt64Ty());
    auto rb_i64   = m_ir_builder->CreateSExt(rb_i32, m_ir_builder->getInt64Ty());
    auto prod_i64 = m_ir_builder->CreateMul(ra_i64, rb_i64);
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }

    // TODO implement oe
    //InterpreterCall("MULLW", &PPUInterpreter::MULLW, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DCBTST(u32 ra, u32 rb, u32 th) {
    // TODO: Implement this
    //InterpreterCall("DCBTST", &PPUInterpreter::DCBTST, ra, rb, th);
}

void PPULLVMRecompiler::STBUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 8));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STBUX", &PPUInterpreter::STBUX, rs, ra, rb);
}

void PPULLVMRecompiler::ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64  = GetGpr(ra);
    auto rb_i64  = GetGpr(rb);
    auto sum_i64 = m_ir_builder->CreateAdd(ra_i64, rb_i64);
    SetGpr(rd, sum_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, sum_i64, m_ir_builder->getInt64(0));
    }

    if (oe) {
        // TODO: Implement this
    }
    //InterpreterCall("ADD", &PPUInterpreter::ADD, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DCBT(u32 ra, u32 rb, u32 th) {
    // TODO: Implement this using prefetch
    //InterpreterCall("DCBT", &PPUInterpreter::DCBT, ra, rb, th);
}

void PPULLVMRecompiler::LHZX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LHZX", &PPUInterpreter::LHZX, rd, ra, rb);
}

void PPULLVMRecompiler::EQV(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("EQV", &PPUInterpreter::EQV, ra, rs, rb, rc);
}

void PPULLVMRecompiler::ECIWX(u32 rd, u32 ra, u32 rb) {
    InterpreterCall("ECIWX", &PPUInterpreter::ECIWX, rd, ra, rb);
}

void PPULLVMRecompiler::LHZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LHZUX", &PPUInterpreter::LHZUX, rd, ra, rb);
}

void PPULLVMRecompiler::XOR(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("XOR", &PPUInterpreter::XOR, ra, rs, rb, rc);
}

void PPULLVMRecompiler::MFSPR(u32 rd, u32 spr) {
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
        rd_i64 = GetUsprg0();
        break;
    default:
        assert(0);
        break;
    }

    SetGpr(rd, rd_i64);
    //InterpreterCall("MFSPR", &PPUInterpreter::MFSPR, rd, spr);
}

void PPULLVMRecompiler::LWAX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LWAX", &PPUInterpreter::LWAX, rd, ra, rb);
}

void PPULLVMRecompiler::DST(u32 ra, u32 rb, u32 strm, u32 t) {
    InterpreterCall("DST", &PPUInterpreter::DST, ra, rb, strm, t);
}

void PPULLVMRecompiler::LHAX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LHAX", &PPUInterpreter::LHAX, rd, ra, rb);
}

void PPULLVMRecompiler::LVXL(u32 vd, u32 ra, u32 rb) {
    LVX(vd, ra, rb);
    //InterpreterCall("LVXL", &PPUInterpreter::LVXL, vd, ra, rb);
}

void PPULLVMRecompiler::MFTB(u32 rd, u32 spr) {
    static Function * s_get_time_fn = nullptr;

    if (s_get_time_fn == nullptr) {
        s_get_time_fn = (Function *)m_module->getOrInsertFunction("get_time", m_ir_builder->getInt64Ty(), nullptr);
        s_get_time_fn->setCallingConv(CallingConv::X86_64_Win64);
        m_execution_engine->addGlobalMapping(s_get_time_fn, (void *)get_time);
    }
    
    auto tb_i64 = (Value *)m_ir_builder->CreateCall(s_get_time_fn);

    u32 n = (spr >> 5) | ((spr & 0x1f) << 5);
    if (n == 0x10D) {
        tb_i64 = m_ir_builder->CreateLShr(tb_i64, 32);
    }

    SetGpr(rd, tb_i64);
}

void PPULLVMRecompiler::LWAUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LWAUX", &PPUInterpreter::LWAUX, rd, ra, rb);
}

void PPULLVMRecompiler::DSTST(u32 ra, u32 rb, u32 strm, u32 t) {
    InterpreterCall("DSTST", &PPUInterpreter::DSTST, ra, rb, strm, t);
}

void PPULLVMRecompiler::LHAUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LHAUX", &PPUInterpreter::LHAUX, rd, ra, rb);
}

void PPULLVMRecompiler::STHX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 16));
    //InterpreterCall("STHX", &PPUInterpreter::STHX, rs, ra, rb);
}

void PPULLVMRecompiler::ORC(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("ORC", &PPUInterpreter::ORC, ra, rs, rb, rc);
}

void PPULLVMRecompiler::ECOWX(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("ECOWX", &PPUInterpreter::ECOWX, rs, ra, rb);
}

void PPULLVMRecompiler::STHUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 16));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STHUX", &PPUInterpreter::STHUX, rs, ra, rb);
}

void PPULLVMRecompiler::OR(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("OR", &PPUInterpreter::OR, ra, rs, rb, rc);
}

void PPULLVMRecompiler::DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64  = GetGpr(ra);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateUDiv(ra_i64, rb_i64);
    SetGpr(rd, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    // TODO implement oe
    // TODO make sure an exception does not occur on divide by 0 and overflow
    //InterpreterCall("DIVDU", &PPUInterpreter::DIVDU, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i32  = GetGpr(ra, 32);
    auto rb_i32  = GetGpr(rb, 32);
    auto res_i32 = m_ir_builder->CreateUDiv(ra_i32, rb_i32);
    auto res_i64 = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    // TODO implement oe
    // TODO make sure an exception does not occur on divide by 0 and overflow
    //InterpreterCall("DIVWU", &PPUInterpreter::DIVWU, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MTSPR(u32 spr, u32 rs) {
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
        SetUsprg0(rs_i64);
        break;
    default:
        assert(0);
        break;
    }

    //InterpreterCall("MTSPR", &PPUInterpreter::MTSPR, spr, rs);
}

void PPULLVMRecompiler::NAND(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("NAND", &PPUInterpreter::NAND, ra, rs, rb, rc);
}

void PPULLVMRecompiler::STVXL(u32 vs, u32 ra, u32 rb) {
    STVX(vs, ra, rb);
    //InterpreterCall("STVXL", &PPUInterpreter::STVXL, vs, ra, rb);
}

void PPULLVMRecompiler::DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i64  = GetGpr(ra);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateSDiv(ra_i64, rb_i64);
    SetGpr(rd, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    // TODO implement oe
    // TODO make sure an exception does not occur on divide by 0 and overflow
    //InterpreterCall("DIVD", &PPUInterpreter::DIVD, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    auto ra_i32  = GetGpr(ra, 32);
    auto rb_i32  = GetGpr(rb, 32);
    auto res_i32 = m_ir_builder->CreateSDiv(ra_i32, rb_i32);
    auto res_i64 = m_ir_builder->CreateZExt(res_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }

    // TODO implement oe
    // TODO make sure an exception does not occur on divide by 0 and overflow
    //InterpreterCall("DIVW", &PPUInterpreter::DIVW, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::LVLX(u32 vd, u32 ra, u32 rb) {
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
    //InterpreterCall("LVLX", &PPUInterpreter::LVLX, vd, ra, rb);
}

void PPULLVMRecompiler::LDBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64, 0, false);
    SetGpr(rd, mem_i64);
    //InterpreterCall("LDBRX", &PPUInterpreter::LDBRX, rd, ra, rb);
}

void PPULLVMRecompiler::LSWX(u32 rd, u32 ra, u32 rb) {
    InterpreterCall("LSWX", &PPUInterpreter::LSWX, rd, ra, rb);
}

void PPULLVMRecompiler::LWBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32, 0, false);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LWBRX", &PPUInterpreter::LWBRX, rd, ra, rb);
}

void PPULLVMRecompiler::LFSX(u32 frd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    SetFpr(frd, mem_i32);
    //InterpreterCall("LFSX", &PPUInterpreter::LFSX, frd, ra, rb);
}

void PPULLVMRecompiler::SRW(u32 ra, u32 rs, u32 rb, bool rc) {
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

    //InterpreterCall("SRW", &PPUInterpreter::SRW, ra, rs, rb, rc);
}

void PPULLVMRecompiler::SRD(u32 ra, u32 rs, u32 rb, bool rc) {
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

    //InterpreterCall("SRD", &PPUInterpreter::SRD, ra, rs, rb, rc);
}

void PPULLVMRecompiler::LVRX(u32 vd, u32 ra, u32 rb) {
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

    //InterpreterCall("LVRX", &PPUInterpreter::LVRX, vd, ra, rb);
}

void PPULLVMRecompiler::LSWI(u32 rd, u32 ra, u32 nb) {
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

    //InterpreterCall("LSWI", &PPUInterpreter::LSWI, rd, ra, nb);
}

void PPULLVMRecompiler::LFSUX(u32 frd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    auto mem_i32  = ReadMemory(addr_i64, 32);
    SetFpr(frd, mem_i32);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LFSUX", &PPUInterpreter::LFSUX, frd, ra, rb);
}

void PPULLVMRecompiler::SYNC(u32 l) {
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_mfence));
    //InterpreterCall("SYNC", &PPUInterpreter::SYNC, l);
}

void PPULLVMRecompiler::LFDX(u32 frd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetFpr(frd, mem_i64);
    //InterpreterCall("LFDX", &PPUInterpreter::LFDX, frd, ra, rb);
}

void PPULLVMRecompiler::LFDUX(u32 frd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    auto mem_i64  = ReadMemory(addr_i64, 64);
    SetFpr(frd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LFDUX", &PPUInterpreter::LFDUX, frd, ra, rb);
}

void PPULLVMRecompiler::STVLX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVLX", &PPUInterpreter::STVLX, vs, ra, rb);
}

void PPULLVMRecompiler::STSWX(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("STSWX", &PPUInterpreter::STSWX, rs, ra, rb);
}

void PPULLVMRecompiler::STWBRX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32), 0, false);
    //InterpreterCall("STWBRX", &PPUInterpreter::STWBRX, rs, ra, rb);
}

void PPULLVMRecompiler::STFSX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i32 = m_ir_builder->CreateBitCast(GetFpr(frs, 32), m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
    //InterpreterCall("STFSX", &PPUInterpreter::STFSX, frs, ra, rb);
}

void PPULLVMRecompiler::STVRX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVRX", &PPUInterpreter::STVRX, vs, ra, rb);
}

void PPULLVMRecompiler::STFSUX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto frs_i32 = m_ir_builder->CreateBitCast(GetFpr(frs, 32), m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
    SetGpr(ra, addr_i64);
    //InterpreterCall("STFSUX", &PPUInterpreter::STFSUX, frs, ra, rb);
}

void PPULLVMRecompiler::STSWI(u32 rd, u32 ra, u32 nb) {
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

    //InterpreterCall("STSWI", &PPUInterpreter::STSWI, rd, ra, nb);
}

void PPULLVMRecompiler::STFDX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    WriteMemory(addr_i64, frs_i64);
    //InterpreterCall("STFDX", &PPUInterpreter::STFDX, frs, ra, rb);
}

void PPULLVMRecompiler::STFDUX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    WriteMemory(addr_i64, frs_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("STFDUX", &PPUInterpreter::STFDUX, frs, ra, rb);
}

void PPULLVMRecompiler::LVLXL(u32 vd, u32 ra, u32 rb) {
    LVLX(vd, ra, rb);
    //InterpreterCall("LVLXL", &PPUInterpreter::LVLXL, vd, ra, rb);
}

void PPULLVMRecompiler::LHBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16, 0, false);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LHBRX", &PPUInterpreter::LHBRX, rd, ra, rb);
}

void PPULLVMRecompiler::SRAW(u32 ra, u32 rs, u32 rb, bool rc) {
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

    //InterpreterCall("SRAW", &PPUInterpreter::SRAW, ra, rs, rb, rc);
}

void PPULLVMRecompiler::SRAD(u32 ra, u32 rs, u32 rb, bool rc) {
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

    //InterpreterCall("SRAD", &PPUInterpreter::SRAD, ra, rs, rb, rc);
}

void PPULLVMRecompiler::LVRXL(u32 vd, u32 ra, u32 rb) {
    LVRX(vd, ra, rb);
    //InterpreterCall("LVRXL", &PPUInterpreter::LVRXL, vd, ra, rb);
}

void PPULLVMRecompiler::DSS(u32 strm, u32 a) {
    InterpreterCall("DSS", &PPUInterpreter::DSS, strm, a);
}

void PPULLVMRecompiler::SRAWI(u32 ra, u32 rs, u32 sh, bool rc) {
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

    //InterpreterCall("SRAWI", &PPUInterpreter::SRAWI, ra, rs, sh, rc);
}

void PPULLVMRecompiler::SRADI1(u32 ra, u32 rs, u32 sh, bool rc) {
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

    //InterpreterCall("SRADI1", &PPUInterpreter::SRADI1, ra, rs, sh, rc);
}

void PPULLVMRecompiler::SRADI2(u32 ra, u32 rs, u32 sh, bool rc) {
    SRADI1(ra, rs, sh, rc);
    //InterpreterCall("SRADI2", &PPUInterpreter::SRADI2, ra, rs, sh, rc);
}

void PPULLVMRecompiler::EIEIO() {
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_mfence));
    //InterpreterCall("EIEIO", &PPUInterpreter::EIEIO);
}

void PPULLVMRecompiler::STVLXL(u32 vs, u32 ra, u32 rb) {
    STVLX(vs, ra, rb);
    //InterpreterCall("STVLXL", &PPUInterpreter::STVLXL, vs, ra, rb);
}

void PPULLVMRecompiler::STHBRX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 16), 0, false);
    //InterpreterCall("STHBRX", &PPUInterpreter::STHBRX, rs, ra, rb);
}

void PPULLVMRecompiler::EXTSH(u32 ra, u32 rs, bool rc) {
    auto rs_i16 = GetGpr(rs, 16);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i16, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("EXTSH", &PPUInterpreter::EXTSH, ra, rs, rc);
}

void PPULLVMRecompiler::STVRXL(u32 vs, u32 ra, u32 rb) {
    STVRX(vs, ra, rb);
    //InterpreterCall("STVRXL", &PPUInterpreter::STVRXL, vs, ra, rb);
}

void PPULLVMRecompiler::EXTSB(u32 ra, u32 rs, bool rc) {
    auto rs_i8  = GetGpr(rs, 8);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i8, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("EXTSB", &PPUInterpreter::EXTSB, ra, rs, rc);
}

void PPULLVMRecompiler::STFIWX(u32 frs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    auto frs_i32 = m_ir_builder->CreateTrunc(frs_i64, m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
    //InterpreterCall("STFIWX", &PPUInterpreter::STFIWX, frs, ra, rb);
}

void PPULLVMRecompiler::EXTSW(u32 ra, u32 rs, bool rc) {
    auto rs_i32 = GetGpr(rs, 32);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i32, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("EXTSW", &PPUInterpreter::EXTSW, ra, rs, rc);
}

void PPULLVMRecompiler::ICBI(u32 ra, u32 rs) {
    InterpreterCall("ICBI", &PPUInterpreter::ICBI, ra, rs);
}

void PPULLVMRecompiler::DCBZ(u32 ra, u32 rb) {
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
    //InterpreterCall("DCBZ", &PPUInterpreter::DCBZ, ra, rb);L
}

void PPULLVMRecompiler::LWZ(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LWZ", &PPUInterpreter::LWZ, rd, ra, d);
}

void PPULLVMRecompiler::LWZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LWZU", &PPUInterpreter::LWZU, rd, ra, d);
}

void PPULLVMRecompiler::LBZ(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LBZ", &PPUInterpreter::LBZ, rd, ra, d);
}

void PPULLVMRecompiler::LBZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LBZU", &PPUInterpreter::LBZU, rd, ra, d);
}

void PPULLVMRecompiler::STW(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32));
    //InterpreterCall("STW", &PPUInterpreter::STW, rs, ra, d);
}

void PPULLVMRecompiler::STWU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 32));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STWU", &PPUInterpreter::STWU, rs, ra, d);
}

void PPULLVMRecompiler::STB(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 8));
    //InterpreterCall("STB", &PPUInterpreter::STB, rs, ra, d);
}

void PPULLVMRecompiler::STBU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 8));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STBU", &PPUInterpreter::STBU, rs, ra, d);
}

void PPULLVMRecompiler::LHZ(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LHZ", &PPUInterpreter::LHZ, rd, ra, d);
}

void PPULLVMRecompiler::LHZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LHZU", &PPUInterpreter::LHZU, rd, ra, d);
}

void PPULLVMRecompiler::LHA(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LHA", &PPUInterpreter::LHA, rd, ra, d);
}

void PPULLVMRecompiler::LHAU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LHAU", &PPUInterpreter::LHAU, rd, ra, d);
}

void PPULLVMRecompiler::STH(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 16));
    //InterpreterCall("STH", &PPUInterpreter::STH, rs, ra, d);
}

void PPULLVMRecompiler::STHU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 16));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STHU", &PPUInterpreter::STHU, rs, ra, d);
}

void PPULLVMRecompiler::LMW(u32 rd, u32 ra, s32 d) {
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

    //InterpreterCall("LMW", &PPUInterpreter::LMW, rd, ra, d);
}

void PPULLVMRecompiler::STMW(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        addr_i64 = m_ir_builder->CreateAdd(addr_i64, GetGpr(ra));
    }

    for (u32 i = rs; i < 32; i++) {
        auto val_i32 = GetGpr(i, 32);
        WriteMemory(addr_i64, val_i32);
        addr_i64 = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64(4));
    }

    //InterpreterCall("STMW", &PPUInterpreter::STMW, rs, ra, d);
}

void PPULLVMRecompiler::LFS(u32 frd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    SetFpr(frd, mem_i32);
    //InterpreterCall("LFS", &PPUInterpreter::LFS, frd, ra, d);
}

void PPULLVMRecompiler::LFSU(u32 frd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    auto mem_i32  = ReadMemory(addr_i64, 32);
    SetFpr(frd, mem_i32);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LFSU", &PPUInterpreter::LFSU, frd, ra, ds);
}

void PPULLVMRecompiler::LFD(u32 frd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetFpr(frd, mem_i64);
    //InterpreterCall("LFD", &PPUInterpreter::LFD, frd, ra, d);
}

void PPULLVMRecompiler::LFDU(u32 frd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetFpr(frd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LFDU", &PPUInterpreter::LFDU, frd, ra, ds);
}

void PPULLVMRecompiler::STFS(u32 frs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i32 = m_ir_builder->CreateBitCast(GetFpr(frs, 32), m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
    //InterpreterCall("STFS", &PPUInterpreter::STFS, frs, ra, d);
}

void PPULLVMRecompiler::STFSU(u32 frs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto frs_i32 = m_ir_builder->CreateBitCast(GetFpr(frs, 32), m_ir_builder->getInt32Ty());
    WriteMemory(addr_i64, frs_i32);
    SetGpr(ra, addr_i64);
    //InterpreterCall("STFSU", &PPUInterpreter::STFSU, frs, ra, d);
}

void PPULLVMRecompiler::STFD(u32 frs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    WriteMemory(addr_i64, frs_i64);
    //InterpreterCall("STFD", &PPUInterpreter::STFD, frs, ra, d);
}

void PPULLVMRecompiler::STFDU(u32 frs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto frs_i64 = m_ir_builder->CreateBitCast(GetFpr(frs), m_ir_builder->getInt64Ty());
    WriteMemory(addr_i64, frs_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("STFDU", &PPUInterpreter::STFDU, frs, ra, d);
}

void PPULLVMRecompiler::LD(u32 rd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    //InterpreterCall("LD", &PPUInterpreter::LD, rd, ra, ds);
}

void PPULLVMRecompiler::LDU(u32 rd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LDU", &PPUInterpreter::LDU, rd, ra, ds);
}

void PPULLVMRecompiler::LWA(u32 rd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LWA", &PPUInterpreter::LWA, rd, ra, ds);
}

void PPULLVMRecompiler::FDIVS(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFDiv(ra_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    res_f64      = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FDIVS", &PPUInterpreter::FDIVS, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSUBS(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFSub(ra_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    res_f64      = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FSUBS", &PPUInterpreter::FSUBS, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FADDS(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFAdd(ra_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    res_f64      = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FADDS", &PPUInterpreter::FADDS, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSQRTS(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FSQRTS", &PPUInterpreter::FSQRTS, frd, frb, rc);
}

void PPULLVMRecompiler::FRES(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FRES", &PPUInterpreter::FRES, frd, frb, rc);
}

void PPULLVMRecompiler::FMULS(u32 frd, u32 fra, u32 frc, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = m_ir_builder->CreateFMul(ra_f64, rc_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    res_f64      = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FMULS", &PPUInterpreter::FMULS, frd, fra, frc, rc);
}

void PPULLVMRecompiler::FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, {m_ir_builder->getDoubleTy()}), ra_f64, rc_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    res_f64      = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FMADDS", &PPUInterpreter::FMADDS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, {m_ir_builder->getDoubleTy()}), ra_f64, rc_f64, rb_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    res_f64      = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FMSUBS", &PPUInterpreter::FMSUBS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, {m_ir_builder->getDoubleTy()}), ra_f64, rc_f64, rb_f64);
    res_f64      = m_ir_builder->CreateFNeg(res_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    res_f64      = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FNMSUBS", &PPUInterpreter::FNMSUBS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, {m_ir_builder->getDoubleTy()}), ra_f64, rc_f64, rb_f64);
    res_f64      = m_ir_builder->CreateFNeg(res_f64);
    auto res_f32 = m_ir_builder->CreateFPTrunc(res_f64, m_ir_builder->getFloatTy());
    res_f64      = m_ir_builder->CreateFPExt(res_f32, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FNMADDS", &PPUInterpreter::FNMADDS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::STD(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 64));
    //InterpreterCall("STD", &PPUInterpreter::STD, rs, ra, d);
}

void PPULLVMRecompiler::STDU(u32 rs, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 64));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STDU", &PPUInterpreter::STDU, rs, ra, ds);
}

void PPULLVMRecompiler::MTFSB1(u32 crbd, bool rc) {
    InterpreterCall("MTFSB1", &PPUInterpreter::MTFSB1, crbd, rc);
}

void PPULLVMRecompiler::MCRFS(u32 crbd, u32 crbs) {
    InterpreterCall("MCRFS", &PPUInterpreter::MCRFS, crbd, crbs);
}

void PPULLVMRecompiler::MTFSB0(u32 crbd, bool rc) {
    InterpreterCall("MTFSB0", &PPUInterpreter::MTFSB0, crbd, rc);
}

void PPULLVMRecompiler::MTFSFI(u32 crfd, u32 i, bool rc) {
    InterpreterCall("MTFSFI", &PPUInterpreter::MTFSFI, crfd, i, rc);
}

void PPULLVMRecompiler::MFFS(u32 frd, bool rc) {
    InterpreterCall("MFFS", &PPUInterpreter::MFFS, frd, rc);
}

void PPULLVMRecompiler::MTFSF(u32 flm, u32 frb, bool rc) {
    InterpreterCall("MTFSF", &PPUInterpreter::MTFSF, flm, frb, rc);
}

void PPULLVMRecompiler::FCMPU(u32 crfd, u32 fra, u32 frb) {
    InterpreterCall("FCMPU", &PPUInterpreter::FCMPU, crfd, fra, frb);
}

void PPULLVMRecompiler::FRSP(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FRSP", &PPUInterpreter::FRSP, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIW(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTIW", &PPUInterpreter::FCTIW, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIWZ(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTIWZ", &PPUInterpreter::FCTIWZ, frd, frb, rc);
}

void PPULLVMRecompiler::FDIV(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFDiv(ra_f64, rb_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FDIV", &PPUInterpreter::FDIV, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSUB(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFSub(ra_f64, rb_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FSUB", &PPUInterpreter::FSUB, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FADD(u32 frd, u32 fra, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = m_ir_builder->CreateFAdd(ra_f64, rb_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FADD", &PPUInterpreter::FADD, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSQRT(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FSQRT", &PPUInterpreter::FSQRT, frd, frb, rc);
}

void PPULLVMRecompiler::FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FSEL", &PPUInterpreter::FSEL, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMUL(u32 frd, u32 fra, u32 frc, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = m_ir_builder->CreateFMul(ra_f64, rc_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FMUL", &PPUInterpreter::FMUL, frd, fra, frc, rc);
}

void PPULLVMRecompiler::FRSQRTE(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FRSQRTE", &PPUInterpreter::FRSQRTE, frd, frb, rc);
}

void PPULLVMRecompiler::FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    auto res_f64 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, {m_ir_builder->getDoubleTy()}), ra_f64, rc_f64, rb_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FMSUB", &PPUInterpreter::FMSUB, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, {m_ir_builder->getDoubleTy()}), ra_f64, rc_f64, rb_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FMADD", &PPUInterpreter::FMADD, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, {m_ir_builder->getDoubleTy()}), ra_f64, rc_f64, rb_f64);
    res_f64      = m_ir_builder->CreateFNeg(res_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FNMSUB", &PPUInterpreter::FNMSUB, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    auto ra_f64  = GetFpr(fra);
    auto rb_f64  = GetFpr(frb);
    auto rc_f64  = GetFpr(frc);
    auto res_f64 = (Value *)m_ir_builder->CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::fmuladd, {m_ir_builder->getDoubleTy()}), ra_f64, rc_f64, rb_f64);
    res_f64      = m_ir_builder->CreateFNeg(res_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FNMADD", &PPUInterpreter::FNMADD, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FCMPO(u32 crfd, u32 fra, u32 frb) {
    InterpreterCall("FCMPO", &PPUInterpreter::FCMPO, crfd, fra, frb);
}

void PPULLVMRecompiler::FNEG(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    rb_f64       = m_ir_builder->CreateFNeg(rb_f64);
    SetFpr(frd, rb_f64);

    // TODO: Set flags
    //InterpreterCall("FNEG", &PPUInterpreter::FNEG, frd, frb, rc);
}

void PPULLVMRecompiler::FMR(u32 frd, u32 frb, bool rc) {
    SetFpr(frd, GetFpr(frb));
    // TODO: Set flags
    //InterpreterCall("FMR", &PPUInterpreter::FMR, frd, frb, rc);
}

void PPULLVMRecompiler::FNABS(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::fabs, {m_ir_builder->getDoubleTy()}), rb_f64);
    res_f64      = m_ir_builder->CreateFNeg(res_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FNABS", &PPUInterpreter::FNABS, frd, frb, rc);
}

void PPULLVMRecompiler::FABS(u32 frd, u32 frb, bool rc) {
    auto rb_f64  = GetFpr(frb);
    auto res_f64 = (Value *)m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::fabs, {m_ir_builder->getDoubleTy()}), rb_f64);
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FABS", &PPUInterpreter::FABS, frd, frb, rc);
}

void PPULLVMRecompiler::FCTID(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTID", &PPUInterpreter::FCTID, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIDZ(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTIDZ", &PPUInterpreter::FCTIDZ, frd, frb, rc);
}

void PPULLVMRecompiler::FCFID(u32 frd, u32 frb, bool rc) {
    auto rb_i64  = GetFpr(frb, 64, true);
    auto res_f64 = m_ir_builder->CreateSIToFP(rb_i64, m_ir_builder->getDoubleTy());
    SetFpr(frd, res_f64);

    // TODO: Set flags
    //InterpreterCall("FCFID", &PPUInterpreter::FCFID, frd, frb, rc);
}

void PPULLVMRecompiler::UNK(const u32 code, const u32 opcode, const u32 gcode) {
    //InterpreterCall("UNK", &PPUInterpreter::UNK, code, opcode, gcode);
}

BasicBlock * PPULLVMRecompiler::GetBlockInFunction(u32 address, Function * function, bool create_if_not_exist) {
    auto         block_name = fmt::Format("instr_0x%X", address);
    BasicBlock * block      = nullptr;
    for (auto i = function->getBasicBlockList().begin(); i != function->getBasicBlockList().end(); i++) {
        if (i->getName() == block_name) {
            block = &(*i);
            break;
        }
    }

    if (!block && create_if_not_exist) {
        block = BasicBlock::Create(m_ir_builder->getContext(), block_name, function);
    }

    return block;
}

void PPULLVMRecompiler::Compile(u32 address) {
    auto compilation_start = std::chrono::high_resolution_clock::now();

    // Get the revision number for this section
    u32  revision = 0;
    auto compiled = m_compiled.lower_bound(std::make_pair(address, 0));
    if (compiled != m_compiled.end() && compiled->first.first == address) {
        revision = ~(compiled->first.second);
        revision++;
    }

    auto ir_build_start = std::chrono::high_resolution_clock::now();

    // Create a function for this section
    auto function_name = fmt::Format("fn_0x%X_%u", address, revision);
    m_current_function = (Function *)m_module->getOrInsertFunction(function_name, m_ir_builder->getVoidTy(),
                                                                   m_ir_builder->getInt8PtrTy() /*ppu_state*/,
                                                                   m_ir_builder->getInt8PtrTy() /*interpreter*/, nullptr);
    m_current_function->setCallingConv(CallingConv::X86_64_Win64);
    auto arg_i = m_current_function->arg_begin();
    arg_i->setName("ppu_state");
    (++arg_i)->setName("interpreter");

    // Add an entry block that branches to the first instruction
    m_ir_builder->SetInsertPoint(BasicBlock::Create(m_ir_builder->getContext(), "entry", m_current_function));
    m_ir_builder->CreateBr(GetBlockInFunction(address, m_current_function, true));

    // Convert each block in this section to LLVM IR
    m_num_instructions = 0;
    m_current_function_uncompiled_blocks_list.clear();
    m_current_function_unhit_blocks_list.clear();
    m_current_function_uncompiled_blocks_list.push_back(address);
    while (!m_current_function_uncompiled_blocks_list.empty()) {
        m_current_instruction_address = m_current_function_uncompiled_blocks_list.front();
        auto block                    = GetBlockInFunction(m_current_instruction_address, m_current_function, true);
        m_hit_branch_instruction      = false;
        m_ir_builder->SetInsertPoint(block);
        m_current_function_uncompiled_blocks_list.pop_front();

        while (!m_hit_branch_instruction) {
            if (!block->getInstList().empty()) {
                break;
            }

            u32 instr = vm::read32(m_current_instruction_address);
            Decode(instr);
            m_num_instructions++;

            m_current_instruction_address += 4;
            if (!m_hit_branch_instruction) {
                block = GetBlockInFunction(m_current_instruction_address, m_current_function, true);
                m_ir_builder->CreateBr(block);
                m_ir_builder->SetInsertPoint(block);
            }
        }
    }

    auto ir_build_end  = std::chrono::high_resolution_clock::now();
    m_ir_build_time   += std::chrono::duration_cast<std::chrono::nanoseconds>(ir_build_end - ir_build_start);

    // Optimize this function
    auto optimize_start = std::chrono::high_resolution_clock::now();
    m_fpm->run(*m_current_function);
    auto optimize_end  = std::chrono::high_resolution_clock::now();
    m_optimizing_time += std::chrono::duration_cast<std::chrono::nanoseconds>(optimize_end - optimize_start);

    // Translate to machine code
    auto translate_start = std::chrono::high_resolution_clock::now();
    MachineCodeInfo mci;
    m_execution_engine->runJITOnFunction(m_current_function, &mci);
    auto translate_end  = std::chrono::high_resolution_clock::now();
    m_translation_time += std::chrono::duration_cast<std::chrono::nanoseconds>(translate_end - translate_start);

    // Add the executable to private and shared data stores
    ExecutableInfo executable_info;
    executable_info.executable                     = (Executable)mci.address();
    executable_info.size                           = mci.size();
    executable_info.num_instructions               = m_num_instructions;
    executable_info.unhit_blocks_list              = std::move(m_current_function_unhit_blocks_list);
    executable_info.llvm_function                  = m_current_function;
    m_compiled[std::make_pair(address, ~revision)] = executable_info;

    {
        std::lock_guard<std::mutex> lock(m_compiled_shared_lock);
        m_compiled_shared[std::make_pair(address, ~revision)] = std::make_pair(executable_info.executable, 0);
    }

    if (revision) {
        m_revision.fetch_add(1, std::memory_order_relaxed);
    }

    auto compilation_end  = std::chrono::high_resolution_clock::now();
    m_compilation_time   += std::chrono::duration_cast<std::chrono::nanoseconds>(compilation_end - compilation_start);
}

void PPULLVMRecompiler::RemoveUnusedOldVersions() {
    u32 num_removed  = 0;
    u32 prev_address = 0;
    for (auto i = m_compiled.begin(); i != m_compiled.end(); i++) {
        u32 current_address = i->first.first;
        if (prev_address == current_address) {
            bool erase_this_entry = false;

            {
                std::lock_guard<std::mutex> lock(m_compiled_shared_lock);
                auto j = m_compiled_shared.find(i->first);
                if (j->second.second == 0) {
                    m_compiled_shared.erase(j);
                    erase_this_entry = true;
                }
            }

            if (erase_this_entry) {
                auto tmp = i;
                i--;
                m_execution_engine->freeMachineCodeForFunction(tmp->second.llvm_function);
                tmp->second.llvm_function->eraseFromParent();
                m_compiled.erase(tmp);
                num_removed++;
            }
        }

        prev_address = current_address;
    }

    if (num_removed > 0) {
        LOG_NOTICE(PPU, "Removed %u old versions", num_removed);
    }
}

bool PPULLVMRecompiler::NeedsCompiling(u32 address) {
    auto i = m_compiled.lower_bound(std::make_pair(address, 0));
    if (i != m_compiled.end() && i->first.first == address) {
        if (i->second.num_instructions >= 300) {
            // This section has reached its limit. Don't allow further expansion.
            return false;
        }

        // If any of the unhit blocks in this function have been hit, then recompile this section
        for (auto j = i->second.unhit_blocks_list.begin(); j != i->second.unhit_blocks_list.end(); j++) {
            if (m_hit_blocks.find(*j) != m_hit_blocks.end()) {
                return true;
            }
        }

        return false;
    } else {
        // This section has not been encountered before
        return true;
    }
}

Value * PPULLVMRecompiler::GetPPUState() {
    return m_current_function->arg_begin();
}

Value * PPULLVMRecompiler::GetInterpreter() {
    auto i = m_current_function->arg_begin();
    i++;
    return i;
}

Value * PPULLVMRecompiler::GetBit(Value * val, u32 n) {
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

Value * PPULLVMRecompiler::ClrBit(Value * val, u32 n) {
    return m_ir_builder->CreateAnd(val, ~((u64)1 << (val->getType()->getIntegerBitWidth() - n - 1)));
}

Value * PPULLVMRecompiler::SetBit(Value * val, u32 n, Value * bit, bool doClear) {
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

Value * PPULLVMRecompiler::GetNibble(Value * val, u32 n) {
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

Value * PPULLVMRecompiler::ClrNibble(Value * val, u32 n) {
    return m_ir_builder->CreateAnd(val, ~((u64)0xF << ((((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4)));
}

Value * PPULLVMRecompiler::SetNibble(Value * val, u32 n, Value * nibble, bool doClear) {
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

Value * PPULLVMRecompiler::SetNibble(Value * val, u32 n, Value * b0, Value * b1, Value * b2, Value * b3, bool doClear) {
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

Value * PPULLVMRecompiler::GetPc() {
    auto pc_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, PC));
    auto pc_i32_ptr = m_ir_builder->CreateBitCast(pc_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(pc_i32_ptr, 4);
}

void PPULLVMRecompiler::SetPc(Value * val_ix) {
    auto pc_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, PC));
    auto pc_i32_ptr = m_ir_builder->CreateBitCast(pc_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    auto val_i32    = m_ir_builder->CreateZExtOrTrunc(val_ix, m_ir_builder->getInt32Ty());
    m_ir_builder->CreateAlignedStore(val_i32, pc_i32_ptr, 4);
}

Value * PPULLVMRecompiler::GetGpr(u32 r, u32 num_bits) {
    auto r_i8_ptr = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, GPR[r]));
    auto r_ix_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getIntNTy(num_bits)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(r_ix_ptr, 8);
}

void PPULLVMRecompiler::SetGpr(u32 r, Value * val_x64) {
    auto r_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, GPR[r]));
    auto r_i64_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    auto val_i64   = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    m_ir_builder->CreateAlignedStore(val_i64, r_i64_ptr, 8);
}

Value * PPULLVMRecompiler::GetCr() {
    auto cr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, CR));
    auto cr_i32_ptr = m_ir_builder->CreateBitCast(cr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(cr_i32_ptr, 4);
}

Value * PPULLVMRecompiler::GetCrField(u32 n) {
    return GetNibble(GetCr(), n);
}

void PPULLVMRecompiler::SetCr(Value * val_x32) {
    auto val_i32    = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto cr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, CR));
    auto cr_i32_ptr = m_ir_builder->CreateBitCast(cr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i32, cr_i32_ptr, 4);
}

void PPULLVMRecompiler::SetCrField(u32 n, Value * field) {
    SetCr(SetNibble(GetCr(), n, field));
}

void PPULLVMRecompiler::SetCrField(u32 n, Value * b0, Value * b1, Value * b2, Value * b3) {
    SetCr(SetNibble(GetCr(), n, b0, b1, b2, b3));
}

void PPULLVMRecompiler::SetCrFieldSignedCmp(u32 n, Value * a, Value * b) {
    auto lt_i1  = m_ir_builder->CreateICmpSLT(a, b);
    auto gt_i1  = m_ir_builder->CreateICmpSGT(a, b);
    auto eq_i1  = m_ir_builder->CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void PPULLVMRecompiler::SetCrFieldUnsignedCmp(u32 n, Value * a, Value * b) {
    auto lt_i1  = m_ir_builder->CreateICmpULT(a, b);
    auto gt_i1  = m_ir_builder->CreateICmpUGT(a, b);
    auto eq_i1  = m_ir_builder->CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void PPULLVMRecompiler::SetCr6AfterVectorCompare(u32 vr) {
    auto vr_v16i8    = GetVrAsIntVec(vr, 8);
    auto vr_mask_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmovmskb_128), vr_v16i8);
    auto cmp0_i1     = m_ir_builder->CreateICmpEQ(vr_mask_i32, m_ir_builder->getInt32(0));
    auto cmp1_i1     = m_ir_builder->CreateICmpEQ(vr_mask_i32, m_ir_builder->getInt32(0xFFFF));
    auto cr_i32      = GetCr();
    cr_i32           = SetNibble(cr_i32, 6, cmp1_i1, nullptr, cmp0_i1, nullptr);
    SetCr(cr_i32);
}

Value * PPULLVMRecompiler::GetLr() {
    auto lr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, LR));
    auto lr_i64_ptr = m_ir_builder->CreateBitCast(lr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(lr_i64_ptr, 8);
}

void PPULLVMRecompiler::SetLr(Value * val_x64) {
    auto val_i64     = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto lr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, LR));
    auto lr_i64_ptr = m_ir_builder->CreateBitCast(lr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i64, lr_i64_ptr, 8);
}

Value * PPULLVMRecompiler::GetCtr() {
    auto ctr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, CTR));
    auto ctr_i64_ptr = m_ir_builder->CreateBitCast(ctr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(ctr_i64_ptr, 8);
}

void PPULLVMRecompiler::SetCtr(Value * val_x64) {
    auto val_i64     = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto ctr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, CTR));
    auto ctr_i64_ptr = m_ir_builder->CreateBitCast(ctr_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i64, ctr_i64_ptr, 8);
}

Value * PPULLVMRecompiler::GetXer() {
    auto xer_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, XER));
    auto xer_i64_ptr = m_ir_builder->CreateBitCast(xer_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(xer_i64_ptr, 8);
}

Value * PPULLVMRecompiler::GetXerCa() {
    return GetBit(GetXer(), 34);
}

Value * PPULLVMRecompiler::GetXerSo() {
    return GetBit(GetXer(), 32);
}

void PPULLVMRecompiler::SetXer(Value * val_x64) {
    auto val_i64     = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto xer_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, XER));
    auto xer_i64_ptr = m_ir_builder->CreateBitCast(xer_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i64, xer_i64_ptr, 8);
}

void PPULLVMRecompiler::SetXerCa(Value * ca) {
    auto xer_i64 = GetXer();
    xer_i64      = SetBit(xer_i64, 34, ca);
    SetXer(xer_i64);
}

void PPULLVMRecompiler::SetXerSo(Value * so) {
    auto xer_i64 = GetXer();
    xer_i64      = SetBit(xer_i64, 32, so);
    SetXer(xer_i64);
}

Value * PPULLVMRecompiler::GetUsprg0() {
    auto usrpg0_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, USPRG0));
    auto usprg0_i64_ptr = m_ir_builder->CreateBitCast(usrpg0_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(usprg0_i64_ptr, 8);
}

void PPULLVMRecompiler::SetUsprg0(Value * val_x64) {
    auto val_i64        = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto usprg0_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, USPRG0));
    auto usprg0_i64_ptr = m_ir_builder->CreateBitCast(usprg0_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i64, usprg0_i64_ptr, 8);
}

Value * PPULLVMRecompiler::GetFpr(u32 r, u32 bits, bool as_int) {
    auto r_i8_ptr = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, FPR[r]));
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

void PPULLVMRecompiler::SetFpr(u32 r, Value * val) {
    auto r_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, FPR[r]));
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

Value * PPULLVMRecompiler::GetVscr() {
    auto vscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = m_ir_builder->CreateBitCast(vscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vscr_i32_ptr, 4);
}

void PPULLVMRecompiler::SetVscr(Value * val_x32) {
    auto val_i32      = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto vscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = m_ir_builder->CreateBitCast(vscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateAlignedStore(val_i32, vscr_i32_ptr, 4);
}

Value * PPULLVMRecompiler::GetVr(u32 vr) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vr_i128_ptr, 16);
}

Value * PPULLVMRecompiler::GetVrAsIntVec(u32 vr, u32 vec_elt_num_bits) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_vec_ptr  = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getIntNTy(vec_elt_num_bits), 128 / vec_elt_num_bits)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vr_vec_ptr, 16);
}

Value * PPULLVMRecompiler::GetVrAsFloatVec(u32 vr) {
    auto vr_i8_ptr    = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v4f32_ptr = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getFloatTy(), 4)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vr_v4f32_ptr, 16);
}

Value * PPULLVMRecompiler::GetVrAsDoubleVec(u32 vr) {
    auto vr_i8_ptr    = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v2f64_ptr = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getDoubleTy(), 2)->getPointerTo());
    return m_ir_builder->CreateAlignedLoad(vr_v2f64_ptr, 16);
}

void PPULLVMRecompiler::SetVr(u32 vr, Value * val_x128) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto val_i128    = m_ir_builder->CreateBitCast(val_x128, m_ir_builder->getIntNTy(128));
    m_ir_builder->CreateAlignedStore(val_i128, vr_i128_ptr, 16);
}

Value * PPULLVMRecompiler::CheckBranchCondition(u32 bo, u32 bi) {
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

void PPULLVMRecompiler::CreateBranch(llvm::Value * cmp_i1, llvm::Value * target_i64, bool lk) {
    if (lk) {
        SetLr(m_ir_builder->getInt64(m_current_instruction_address + 4));
    }

    auto         current_block = m_ir_builder->GetInsertBlock();
    BasicBlock * target_block  = nullptr;
    if (dyn_cast<ConstantInt>(target_i64)) {
        // Target address is an immediate value.
        u32 target_address = (u32)(dyn_cast<ConstantInt>(target_i64)->getLimitedValue());
        target_block       = GetBlockInFunction(target_address, m_current_function);
        if (!target_block) {
            target_block = GetBlockInFunction(target_address, m_current_function, true);
            if ((m_hit_blocks.find(target_address) != m_hit_blocks.end() || !cmp_i1) && m_num_instructions < 300) {
                // Target block has either been hit or this is an unconditional branch.
                m_current_function_uncompiled_blocks_list.push_back(target_address);
                m_hit_blocks.insert(target_address);
            } else {
                // Target block has not been encountered yet and this is not an unconditional branch
                m_ir_builder->SetInsertPoint(target_block);
                SetPc(target_i64);
                m_ir_builder->CreateRetVoid();
                m_current_function_unhit_blocks_list.push_back(target_address);
            }
        }
    } else {
        // Target addres is in a register
        target_block = BasicBlock::Create(m_ir_builder->getContext(), "", m_current_function);
        m_ir_builder->SetInsertPoint(target_block);
        SetPc(target_i64);
        m_ir_builder->CreateRetVoid();
    }

    if (cmp_i1) {
        // Conditional branch
        auto next_block = GetBlockInFunction(m_current_instruction_address + 4, m_current_function);
        if (!next_block) {
            next_block = GetBlockInFunction(m_current_instruction_address + 4, m_current_function, true);
            if (m_hit_blocks.find(m_current_instruction_address + 4) != m_hit_blocks.end() && m_num_instructions < 300) {
                // Next block has already been hit.
                m_current_function_uncompiled_blocks_list.push_back(m_current_instruction_address + 4);
            } else {
                // Next block has not been encountered yet
                m_ir_builder->SetInsertPoint(next_block);
                SetPc(m_ir_builder->getInt32(m_current_instruction_address + 4));
                m_ir_builder->CreateRetVoid();
                m_current_function_unhit_blocks_list.push_back(m_current_instruction_address + 4);
            }
        }

        m_ir_builder->SetInsertPoint(current_block);
        m_ir_builder->CreateCondBr(cmp_i1, target_block, next_block);
    } else {
        // Unconditional branch
        m_ir_builder->SetInsertPoint(current_block);
        m_ir_builder->CreateBr(target_block);
    }

    m_hit_branch_instruction = true;
}

Value * PPULLVMRecompiler::ReadMemory(Value * addr_i64, u32 bits, u32 alignment, bool bswap, bool could_be_mmio) {
    if (bits != 32 || could_be_mmio == false) {
        auto eaddr_i64    = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
        auto eaddr_ix_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getIntNTy(bits)->getPointerTo());
        auto val_ix       = (Value *)m_ir_builder->CreateLoad(eaddr_ix_ptr, alignment);
        if (bits > 8 && bswap) {
            val_ix = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getIntNTy(bits)}), val_ix);
        }

        return val_ix;
    } else {
        BasicBlock * next_block = nullptr;
        for (auto i = m_current_function->begin(); i != m_current_function->end(); i++) {
            if (&(*i) == m_ir_builder->GetInsertBlock()) {
                i++;
                if (i != m_current_function->end()) {
                    next_block = &(*i);
                }

                break;
            }
        }

        auto cmp_i1   = m_ir_builder->CreateICmpULT(addr_i64, m_ir_builder->getInt64(RAW_SPU_BASE_ADDR));
        auto then_bb  = BasicBlock::Create(m_ir_builder->getContext(), "", m_current_function, next_block);
        auto else_bb  = BasicBlock::Create(m_ir_builder->getContext(), "", m_current_function, next_block);
        auto merge_bb = BasicBlock::Create(m_ir_builder->getContext(), "", m_current_function, next_block);
        m_ir_builder->CreateCondBr(cmp_i1, then_bb, else_bb);

        m_ir_builder->SetInsertPoint(then_bb);
        auto eaddr_i64     = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
        auto eaddr_i32_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getInt32Ty()->getPointerTo());
        auto val_then_i32  = (Value *)m_ir_builder->CreateAlignedLoad(eaddr_i32_ptr, alignment);
        if (bswap) {
            val_then_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), val_then_i32);
        }

        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->SetInsertPoint(else_bb);
        auto val_else_i32 = Call<u32>("vm_read32", (u32(*)(u64))vm::read32, addr_i64);
        if (!bswap) {
            val_else_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), val_else_i32);
        }
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->SetInsertPoint(merge_bb);
        auto phi = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 2);
        phi->addIncoming(val_then_i32, then_bb);
        phi->addIncoming(val_else_i32, else_bb);
        return phi;
    }
}

void PPULLVMRecompiler::WriteMemory(Value * addr_i64, Value * val_ix, u32 alignment, bool bswap, bool could_be_mmio) {
    addr_i64 = m_ir_builder->CreateAnd(addr_i64, 0xFFFFFFFF);
    if (val_ix->getType()->getIntegerBitWidth() != 32 || could_be_mmio == false) {
        if (val_ix->getType()->getIntegerBitWidth() > 8 && bswap) {
            val_ix = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {val_ix->getType()}), val_ix);
        }

        auto eaddr_i64    = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
        auto eaddr_ix_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, val_ix->getType()->getPointerTo());
        m_ir_builder->CreateAlignedStore(val_ix, eaddr_ix_ptr, alignment);
    } else {
        BasicBlock * next_block = nullptr;
        for (auto i = m_current_function->begin(); i != m_current_function->end(); i++) {
            if (&(*i) == m_ir_builder->GetInsertBlock()) {
                i++;
                if (i != m_current_function->end()) {
                    next_block = &(*i);
                }

                break;
            }
        }

        auto cmp_i1   = m_ir_builder->CreateICmpULT(addr_i64, m_ir_builder->getInt64(RAW_SPU_BASE_ADDR));
        auto then_bb  = BasicBlock::Create(m_ir_builder->getContext(), "", m_current_function, next_block);
        auto else_bb  = BasicBlock::Create(m_ir_builder->getContext(), "", m_current_function, next_block);
        auto merge_bb = BasicBlock::Create(m_ir_builder->getContext(), "", m_current_function, next_block);
        m_ir_builder->CreateCondBr(cmp_i1, then_bb, else_bb);

        m_ir_builder->SetInsertPoint(then_bb);
        Value * val_then_i32 = val_ix;
        if (bswap) {
            val_then_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), val_then_i32);
        }

        auto eaddr_i64     = m_ir_builder->CreateAdd(addr_i64, m_ir_builder->getInt64((u64)vm::get_ptr<u8>(0)));
        auto eaddr_i32_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getInt32Ty()->getPointerTo());
        m_ir_builder->CreateAlignedStore(val_then_i32, eaddr_i32_ptr, alignment);
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->SetInsertPoint(else_bb);
        Value * val_else_i32 = val_ix;
        if (!bswap) {
            val_else_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), val_else_i32);
        }

        Call<void>("vm_write32", (void(*)(u64, u32))vm::write32, addr_i64, val_else_i32);
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->SetInsertPoint(merge_bb);
    }
}

template<class Func, class... Args>
Value * PPULLVMRecompiler::InterpreterCall(const char * name, Func function, Args... args) {
    auto i = m_interpreter_fallback_stats.find(name);
    if (i == m_interpreter_fallback_stats.end()) {
        i = m_interpreter_fallback_stats.insert(m_interpreter_fallback_stats.end(), std::make_pair<std::string, u64>(name, 0));
    }

    i->second++;

    return Call<void>(name, function, GetInterpreter(), m_ir_builder->getInt32(args)...);
}

template<class T>
Type * PPULLVMRecompiler::CppToLlvmType() {
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
Value * PPULLVMRecompiler::Call(const char * name, Func function, Args... args) {
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

void PPULLVMRecompiler::InitRotateMask() {
    for (u32 mb = 0; mb < 64; mb++) {
        for (u32 me = 0; me < 64; me++) {
            u64 mask = ((u64)-1 >> mb) ^ ((me >= 63) ? 0 : (u64)-1 >> (me + 1));
            s_rotate_mask[mb][me] = mb > me ? ~mask : mask;
        }
    }
}

u32                 PPULLVMEmulator::s_num_instances    = 0;
std::mutex          PPULLVMEmulator::s_recompiler_mutex;
PPULLVMRecompiler * PPULLVMEmulator::s_recompiler       = nullptr;

PPULLVMEmulator::PPULLVMEmulator(PPUThread & ppu)
    : m_ppu(ppu)
    , m_interpreter(new PPUInterpreter(ppu))
    , m_decoder(m_interpreter)
    , m_last_instr_was_branch(true)
    , m_last_cache_clear_time(std::chrono::high_resolution_clock::now())
    , m_recompiler_revision(0) {
    std::lock_guard<std::mutex> lock(s_recompiler_mutex);

    s_num_instances++;
    if (!s_recompiler) {
        s_recompiler = new PPULLVMRecompiler();
        //s_recompiler->RunAllTests(&m_ppu, m_interpreter);
    }
}

PPULLVMEmulator::~PPULLVMEmulator() {
    for (auto iter = m_address_to_executable.begin(); iter != m_address_to_executable.end(); iter++) {
        s_recompiler->ReleaseExecutable(iter->first, iter->second.revision);
    }

    std::lock_guard<std::mutex> lock(s_recompiler_mutex);

    s_num_instances--;
    if (s_recompiler && s_num_instances == 0) {
        delete s_recompiler;
        s_recompiler = nullptr;
    }
}

u8 PPULLVMEmulator::DecodeMemory(const u32 address) {
    auto now = std::chrono::high_resolution_clock::now();

    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_cache_clear_time).count() > 1000) {
        bool clear_all  = false;

        u32 revision = s_recompiler->GetCurrentRevision();
        if (m_recompiler_revision != revision) {
            m_recompiler_revision = revision;
            clear_all = true;
        }

        for (auto iter = m_address_to_executable.begin(); iter != m_address_to_executable.end();) {
            auto tmp = iter;
            iter++;
            if (tmp->second.num_hits == 0 || clear_all) {
                m_address_to_executable.erase(tmp);
                s_recompiler->ReleaseExecutable(tmp->first, tmp->second.revision);
            } else {
                tmp->second.num_hits = 0;
            }
        }

        m_last_cache_clear_time = now;
    }

    auto address_to_executable_iter = m_address_to_executable.find(address);
    if (address_to_executable_iter == m_address_to_executable.end()) {
        auto executable_and_revision = s_recompiler->GetExecutable(address);
        if (executable_and_revision.first) {
            ExecutableInfo executable_info;
            executable_info.executable = executable_and_revision.first;
            executable_info.revision   = executable_and_revision.second;
            executable_info.num_hits   = 0;

            address_to_executable_iter = m_address_to_executable.insert(m_address_to_executable.end(), std::make_pair(address, executable_info));
            m_uncompiled.erase(address);
        } else {
            if (m_last_instr_was_branch) {
                auto uncompiled_iter = m_uncompiled.find(address);
                if (uncompiled_iter != m_uncompiled.end()) {
                    uncompiled_iter->second++;
                    if ((uncompiled_iter->second % 1000) == 0) {
                        s_recompiler->RequestCompilation(address);
                    }
                } else {
                    m_uncompiled[address] = 0;
                }
            }
        }
    }

    u8 ret = 0;
    if (address_to_executable_iter != m_address_to_executable.end()) {
        address_to_executable_iter->second.executable(&m_ppu, m_interpreter);
        address_to_executable_iter->second.num_hits++;
        m_last_instr_was_branch = true;
    } else {
        ret                     = m_decoder.DecodeMemory(address);
        m_last_instr_was_branch = m_ppu.m_is_branch;
    }

    return ret;
}
