#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
#include "Emu/Memory/Memory.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Filesystem.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize.h"

using namespace llvm;

PPULLVMRecompiler::PPULLVMRecompiler()
    : ThreadBase("PPULLVMRecompiler")
    , m_compilation_time(0.0)
    , m_idling_time(0.0) {
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

    m_disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);
}

PPULLVMRecompiler::~PPULLVMRecompiler() {
    Stop();

    std::string error;
    raw_fd_ostream log_file("PPULLVMRecompiler.log", error, sys::fs::F_Text);
    log_file << "Time spent compiling = " << m_compilation_time.count() << "s\n";
    log_file << "Time spent idling    = " << m_idling_time.count() << "s\n\n";
    log_file << "Interpreter fallback stats:\n";
    for (auto i = m_interpreter_fallback_stats.begin(); i != m_interpreter_fallback_stats.end(); i++) {
        log_file << i->first << " = " << i->second << "\n";
    }

    log_file << "\nLLVM IR:\n";
    log_file << *m_module;

    LLVMDisasmDispose(m_disassembler);
    delete m_execution_engine;
    delete m_fpm;
    delete m_ir_builder;
    delete m_llvm_context;
}

PPULLVMRecompiler::CompiledBlock PPULLVMRecompiler::GetCompiledBlock(u64 address) {
    m_address_to_compiled_block_map_mutex.lock();
    auto i = m_address_to_compiled_block_map.find(address);
    m_address_to_compiled_block_map_mutex.unlock();

    if (i != m_address_to_compiled_block_map.end()) {
        return i->second;
    }

    m_pending_blocks_set_mutex.lock();
    m_pending_blocks_set.insert(address);
    m_pending_blocks_set_mutex.unlock();

    if (!IsAlive()) {
        Start();
    }

    Notify();
    return nullptr;
}

void PPULLVMRecompiler::Task() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    while (!TestDestroy() && !Emu.IsStopped()) {
        WaitForAnySignal();

        while (!TestDestroy() && !Emu.IsStopped()) {
            u64 address;

            m_pending_blocks_set_mutex.lock();
            auto i = m_pending_blocks_set.begin();
            m_pending_blocks_set_mutex.unlock();

            if (i != m_pending_blocks_set.end()) {
                address = *i;
                m_pending_blocks_set.erase(i);
            } else {
                break;
            }

            Compile(address);
        }
    }

    std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
    m_idling_time = std::chrono::duration_cast<std::chrono::duration<double>>(end - start - m_compilation_time);
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

    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    va_v4i32               = m_ir_builder->CreateXor(va_v4i32, ConstantDataVector::get(*m_llvm_context, not_mask_v4i32));
    auto cmpv4i1           = m_ir_builder->CreateICmpULT(va_v4i32, vb_v4i32);
    auto cmpv4i32          = m_ir_builder->CreateZExt(cmpv4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmpv4i32);

    // TODO: Implement with overflow intrinsics and check if the generated code is better
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

    // It looks like x86 does not have an instruction to add 32 bit intergers with singed/unsiged saturation.
    // To implement add with saturation, we first determine what the result would be if the operation were to cause
    // an overflow. If two -ve numbers are being added and cause an overflow, the result would be 0x80000000.
    // If two +ve numbers are being added and cause an overflow, the result would be 0x7FFFFFFF. Addition of a -ve
    // number and a +ve number cannot cause overflow. So the result in case of an overflow is 0x7FFFFFFF + sign bit
    // of any one of the operands.
    u32  tmp1_v4i32[4] = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    auto tmp2_v4i32    = m_ir_builder->CreateLShr(va_v4i32, 31);
    tmp2_v4i32         = m_ir_builder->CreateAdd(tmp2_v4i32, ConstantDataVector::get(*m_llvm_context, tmp1_v4i32));
    auto tmp2_v16i8    = m_ir_builder->CreateBitCast(tmp2_v4i32, VectorType::get(m_ir_builder->getInt8Ty(), 16));

    // Next, we find if the addition can actually result in an overflow. Since an overflow can only happen if the operands
    // have the same sign, we bitwise XOR both the operands. If the sign bit of the result is 0 then the operands have the
    // same sign and so may cause an overflow. We invert the result so that the sign bit is 1 when the operands have the
    // same sign.
    auto tmp3_v4i32        = m_ir_builder->CreateXor(va_v4i32, vb_v4i32);
    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    tmp3_v4i32             = m_ir_builder->CreateXor(tmp3_v4i32, ConstantDataVector::get(*m_llvm_context, not_mask_v4i32));

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
    vb_v4i32               = m_ir_builder->CreateXor(vb_v4i32, ConstantDataVector::get(*m_llvm_context, not_mask_v4i32));
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
    sum_v16i16          = m_ir_builder->CreateAdd(sum_v16i16, ConstantDataVector::get(*m_llvm_context, one_v16i16));
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
    sum_v8i32         = m_ir_builder->CreateAdd(sum_v8i32, ConstantDataVector::get(*m_llvm_context, one_v8i32));
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
    sum_v4i64         = m_ir_builder->CreateAdd(sum_v4i64, ConstantDataVector::get(*m_llvm_context, one_v4i64));
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
    sum_v4i64         = m_ir_builder->CreateAdd(sum_v4i64, ConstantDataVector::get(*m_llvm_context, one_v4i64));
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
        res_v4f32            = m_ir_builder->CreateFDiv(res_v4f32, ConstantDataVector::get(*m_llvm_context, scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VCFUX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder->CreateUIToFP(vb_v4i32, VectorType::get(m_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale          = (float)((u64)1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = m_ir_builder->CreateFDiv(res_v4f32, ConstantDataVector::get(*m_llvm_context, scale_v4f32));
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
    vd_v16i8          = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_ssse3_pshuf_b_128), vd_v16i8, ConstantDataVector::get(*m_llvm_context, mask_v16i8));
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
    InterpreterCall("VMADDFP", &PPUInterpreter::VMADDFP, vd, va, vc, vb);
}

void PPULLVMRecompiler::VMAXFP(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXFP", &PPUInterpreter::VMAXFP, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXSB", &PPUInterpreter::VMAXSB, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXSH", &PPUInterpreter::VMAXSH, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXSW", &PPUInterpreter::VMAXSW, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXUB", &PPUInterpreter::VMAXUB, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXUH", &PPUInterpreter::VMAXUH, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXUW", &PPUInterpreter::VMAXUW, vd, va, vb);
}

void PPULLVMRecompiler::VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMHADDSHS", &PPUInterpreter::VMHADDSHS, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMHRADDSHS", &PPUInterpreter::VMHRADDSHS, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMINFP(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINFP", &PPUInterpreter::VMINFP, vd, va, vb);
}

void PPULLVMRecompiler::VMINSB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINSB", &PPUInterpreter::VMINSB, vd, va, vb);
}

void PPULLVMRecompiler::VMINSH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINSH", &PPUInterpreter::VMINSH, vd, va, vb);
}

void PPULLVMRecompiler::VMINSW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINSW", &PPUInterpreter::VMINSW, vd, va, vb);
}

void PPULLVMRecompiler::VMINUB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINUB", &PPUInterpreter::VMINUB, vd, va, vb);
}

void PPULLVMRecompiler::VMINUH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINUH", &PPUInterpreter::VMINUH, vd, va, vb);
}

void PPULLVMRecompiler::VMINUW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINUW", &PPUInterpreter::VMINUW, vd, va, vb);
}

void PPULLVMRecompiler::VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMLADDUHM", &PPUInterpreter::VMLADDUHM, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMRGHB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGHB", &PPUInterpreter::VMRGHB, vd, va, vb);
}

void PPULLVMRecompiler::VMRGHH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGHH", &PPUInterpreter::VMRGHH, vd, va, vb);
}

void PPULLVMRecompiler::VMRGHW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGHW", &PPUInterpreter::VMRGHW, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGLB", &PPUInterpreter::VMRGLB, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGLH", &PPUInterpreter::VMRGLH, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGLW", &PPUInterpreter::VMRGLW, vd, va, vb);
}

void PPULLVMRecompiler::VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMMBM", &PPUInterpreter::VMSUMMBM, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMSHM", &PPUInterpreter::VMSUMSHM, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMSHS", &PPUInterpreter::VMSUMSHS, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMUBM", &PPUInterpreter::VMSUMUBM, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMUHM", &PPUInterpreter::VMSUMUHM, vd, va, vb, vc);
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
    InterpreterCall("VNMSUBFP", &PPUInterpreter::VNMSUBFP, vd, va, vc, vb);
}

void PPULLVMRecompiler::VNOR(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VNOR", &PPUInterpreter::VNOR, vd, va, vb);
}

void PPULLVMRecompiler::VOR(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VOR", &PPUInterpreter::VOR, vd, va, vb);
}

void PPULLVMRecompiler::VPERM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VPERM", &PPUInterpreter::VPERM, vd, va, vb, vc);
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
    InterpreterCall("VREFP", &PPUInterpreter::VREFP, vd, vb);
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
    InterpreterCall("VSEL", &PPUInterpreter::VSEL, vd, va, vb, vc);
}

void PPULLVMRecompiler::VSL(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSL", &PPUInterpreter::VSL, vd, va, vb);
}

void PPULLVMRecompiler::VSLB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSLB", &PPUInterpreter::VSLB, vd, va, vb);
}

void PPULLVMRecompiler::VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) {
    InterpreterCall("VSLDOI", &PPUInterpreter::VSLDOI, vd, va, vb, sh);
}

void PPULLVMRecompiler::VSLH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSLH", &PPUInterpreter::VSLH, vd, va, vb);
}

void PPULLVMRecompiler::VSLO(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSLO", &PPUInterpreter::VSLO, vd, va, vb);
}

void PPULLVMRecompiler::VSLW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSLW", &PPUInterpreter::VSLW, vd, va, vb);
}

void PPULLVMRecompiler::VSPLTB(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VSPLTB", &PPUInterpreter::VSPLTB, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSPLTH(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VSPLTH", &PPUInterpreter::VSPLTH, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSPLTISB(u32 vd, s32 simm5) {
    InterpreterCall("VSPLTISB", &PPUInterpreter::VSPLTISB, vd, simm5);
}

void PPULLVMRecompiler::VSPLTISH(u32 vd, s32 simm5) {
    InterpreterCall("VSPLTISH", &PPUInterpreter::VSPLTISH, vd, simm5);
}

void PPULLVMRecompiler::VSPLTISW(u32 vd, s32 simm5) {
    InterpreterCall("VSPLTISW", &PPUInterpreter::VSPLTISW, vd, simm5);
}

void PPULLVMRecompiler::VSPLTW(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VSPLTW", &PPUInterpreter::VSPLTW, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSR(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSR", &PPUInterpreter::VSR, vd, va, vb);
}

void PPULLVMRecompiler::VSRAB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRAB", &PPUInterpreter::VSRAB, vd, va, vb);
}

void PPULLVMRecompiler::VSRAH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRAH", &PPUInterpreter::VSRAH, vd, va, vb);
}

void PPULLVMRecompiler::VSRAW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRAW", &PPUInterpreter::VSRAW, vd, va, vb);
}

void PPULLVMRecompiler::VSRB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRB", &PPUInterpreter::VSRB, vd, va, vb);
}

void PPULLVMRecompiler::VSRH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRH", &PPUInterpreter::VSRH, vd, va, vb);
}

void PPULLVMRecompiler::VSRO(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRO", &PPUInterpreter::VSRO, vd, va, vb);
}

void PPULLVMRecompiler::VSRW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRW", &PPUInterpreter::VSRW, vd, va, vb);
}

void PPULLVMRecompiler::VSUBCUW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBCUW", &PPUInterpreter::VSUBCUW, vd, va, vb);
}

void PPULLVMRecompiler::VSUBFP(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBFP", &PPUInterpreter::VSUBFP, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSBS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBSBS", &PPUInterpreter::VSUBSBS, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSHS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBSHS", &PPUInterpreter::VSUBSHS, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSWS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBSWS", &PPUInterpreter::VSUBSWS, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUBM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUBM", &PPUInterpreter::VSUBUBM, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUBS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUBS", &PPUInterpreter::VSUBUBS, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUHM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUHM", &PPUInterpreter::VSUBUHM, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUHS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUHS", &PPUInterpreter::VSUBUHS, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUWM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUWM", &PPUInterpreter::VSUBUWM, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUWS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUWS", &PPUInterpreter::VSUBUWS, vd, va, vb);
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
    InterpreterCall("VXOR", &PPUInterpreter::VXOR, vd, va, vb);
}

void PPULLVMRecompiler::MULLI(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64  = GetGpr(ra);
    auto res_i64 = m_ir_builder->CreateMul(ra_i64, m_ir_builder->getInt64((s64)simm16));
    SetGpr(rd, res_i64);
    //InterpreterCall("MULLI", &PPUInterpreter::MULLI, rd, ra, simm16);
}

void PPULLVMRecompiler::SUBFIC(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64   = GetGpr(ra);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::ssub_with_overflow, {m_ir_builder->getInt64Ty()}), m_ir_builder->getInt64((s64)simm16), ra_i64);
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
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::sadd_with_overflow, {m_ir_builder->getInt64Ty()}), m_ir_builder->getInt64((s64)simm16), ra_i64);
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
    InterpreterCall("BC", &PPUInterpreter::BC, bo, bi, bd, aa, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::SC(u32 sc_code) {
    InterpreterCall("SC", &PPUInterpreter::SC, sc_code);
}

void PPULLVMRecompiler::B(s32 ll, u32 aa, u32 lk) {
    InterpreterCall("B", &PPUInterpreter::B, ll, aa, lk);
    m_hit_branch_instruction = true;
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
    InterpreterCall("BCLR", &PPUInterpreter::BCLR, bo, bi, bh, lk);
    m_hit_branch_instruction = true;
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
    InterpreterCall("BCCTR", &PPUInterpreter::BCCTR, bo, bi, bh, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    InterpreterCall("RLWIMI", &PPUInterpreter::RLWIMI, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompiler::RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    InterpreterCall("RLWINM", &PPUInterpreter::RLWINM, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompiler::RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc) {
    InterpreterCall("RLWNM", &PPUInterpreter::RLWNM, ra, rs, rb, mb, me, rc);
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
    InterpreterCall("RLDICL", &PPUInterpreter::RLDICL, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) {
    InterpreterCall("RLDICR", &PPUInterpreter::RLDICR, ra, rs, sh, me, rc);
}

void PPULLVMRecompiler::RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    InterpreterCall("RLDIC", &PPUInterpreter::RLDIC, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    InterpreterCall("RLDIMI", &PPUInterpreter::RLDIMI, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) {
    InterpreterCall("RLDC_LR", &PPUInterpreter::RLDC_LR, ra, rs, rb, m_eb, is_r, rc);
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
    InterpreterCall("LVSL", &PPUInterpreter::LVSL, vd, ra, rb);
}

void PPULLVMRecompiler::LVEBX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVEBX", &PPUInterpreter::LVEBX, vd, ra, rb);
}

void PPULLVMRecompiler::SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("SUBFC", &PPUInterpreter::SUBFC, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("ADDC", &PPUInterpreter::ADDC, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MULHDU(u32 rd, u32 ra, u32 rb, bool rc) {
    auto ra_i64     = GetGpr(ra);
    auto rb_i64     = GetGpr(rb);
    auto ra_i128    = m_ir_builder->CreateZExt(ra_i64, m_ir_builder->getIntNTy(128));
    auto rb_i128    = m_ir_builder->CreateZExt(rb_i64, m_ir_builder->getIntNTy(128));
    auto prod_i128  = m_ir_builder->CreateMul(ra_i128, rb_i128);
    auto prod_v2i64 = m_ir_builder->CreateBitCast(prod_i128, VectorType::get(m_ir_builder->getInt64Ty(), 2));
    auto prod_i64   = m_ir_builder->CreateExtractElement(prod_v2i64, m_ir_builder->getInt32(1));
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }

    // TODO: Check what code this generates
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
    InterpreterCall("MFOCRF", &PPUInterpreter::MFOCRF, a, rd, crm);
}

void PPULLVMRecompiler::LWARX(u32 rd, u32 ra, u32 rb) {
    InterpreterCall("LWARX", &PPUInterpreter::LWARX, rd, ra, rb);
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
    InterpreterCall("SLW", &PPUInterpreter::SLW, ra, rs, rb, rc);
}

void PPULLVMRecompiler::CNTLZW(u32 ra, u32 rs, bool rc) {
    InterpreterCall("CNTLZW", &PPUInterpreter::CNTLZW, ra, rs, rc);
}

void PPULLVMRecompiler::SLD(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SLD", &PPUInterpreter::SLD, ra, rs, rb, rc);
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
    InterpreterCall("LVSR", &PPUInterpreter::LVSR, vd, ra, rb);
}

void PPULLVMRecompiler::LVEHX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVEHX", &PPUInterpreter::LVEHX, vd, ra, rb);
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
    InterpreterCall("DCBST", &PPUInterpreter::DCBST, ra, rb);
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
    InterpreterCall("CNTLZD", &PPUInterpreter::CNTLZD, ra, rs, rc);
}

void PPULLVMRecompiler::ANDC(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("ANDC", &PPUInterpreter::ANDC, ra, rs, rb, rc);
}

void PPULLVMRecompiler::TD(u32 to, u32 ra, u32 rb) {
    InterpreterCall("TD", &PPUInterpreter::TD, to, ra, rb);
}

void PPULLVMRecompiler::LVEWX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVEWX", &PPUInterpreter::LVEWX, vd, ra, rb);
}

void PPULLVMRecompiler::MULHD(u32 rd, u32 ra, u32 rb, bool rc) {
    auto ra_i64     = GetGpr(ra);
    auto rb_i64     = GetGpr(rb);
    auto ra_i128    = m_ir_builder->CreateSExt(ra_i64, m_ir_builder->getIntNTy(128));
    auto rb_i128    = m_ir_builder->CreateSExt(rb_i64, m_ir_builder->getIntNTy(128));
    auto prod_i128  = m_ir_builder->CreateMul(ra_i128, rb_i128);
    auto prod_v2i64 = m_ir_builder->CreateBitCast(prod_i128, VectorType::get(m_ir_builder->getInt64Ty(), 2));
    auto prod_i64   = m_ir_builder->CreateExtractElement(prod_v2i64, m_ir_builder->getInt32(1));
    SetGpr(rd, prod_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, prod_i64, m_ir_builder->getInt64(0));
    }

    // TODO: Check what code this generates
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
    InterpreterCall("LDARX", &PPUInterpreter::LDARX, rd, ra, rb);
}

void PPULLVMRecompiler::DCBF(u32 ra, u32 rb) {
    InterpreterCall("DCBF", &PPUInterpreter::DCBF, ra, rb);
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
    InterpreterCall("LVX", &PPUInterpreter::LVX, vd, ra, rb);
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
    InterpreterCall("STVEBX", &PPUInterpreter::STVEBX, vs, ra, rb);
}

void PPULLVMRecompiler::SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("SUBFE", &PPUInterpreter::SUBFE, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("ADDE", &PPUInterpreter::ADDE, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MTOCRF(u32 l, u32 crm, u32 rs) {
    InterpreterCall("MTOCRF", &PPUInterpreter::MTOCRF, l, crm, rs);
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
    InterpreterCall("STVEHX", &PPUInterpreter::STVEHX, vs, ra, rb);
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
    InterpreterCall("STVEWX", &PPUInterpreter::STVEWX, vs, ra, rb);
}

void PPULLVMRecompiler::ADDZE(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("ADDZE", &PPUInterpreter::ADDZE, rd, ra, oe, rc);
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
    InterpreterCall("STVX", &PPUInterpreter::STVX, vs, ra, rb);
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
    InterpreterCall("DCBTST", &PPUInterpreter::DCBTST, ra, rb, th);
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
    InterpreterCall("DCBT", &PPUInterpreter::DCBT, ra, rb, th);
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
    InterpreterCall("MFSPR", &PPUInterpreter::MFSPR, rd, spr);
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
    InterpreterCall("LVXL", &PPUInterpreter::LVXL, vd, ra, rb);
}

void PPULLVMRecompiler::MFTB(u32 rd, u32 spr) {
    InterpreterCall("MFTB", &PPUInterpreter::MFTB, rd, spr);
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
    InterpreterCall("DIVDU", &PPUInterpreter::DIVDU, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("DIVWU", &PPUInterpreter::DIVWU, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MTSPR(u32 spr, u32 rs) {
    InterpreterCall("MTSPR", &PPUInterpreter::MTSPR, spr, rs);
}

void PPULLVMRecompiler::NAND(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("NAND", &PPUInterpreter::NAND, ra, rs, rb, rc);
}

void PPULLVMRecompiler::STVXL(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVXL", &PPUInterpreter::STVXL, vs, ra, rb);
}

void PPULLVMRecompiler::DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("DIVD", &PPUInterpreter::DIVD, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("DIVW", &PPUInterpreter::DIVW, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::LVLX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVLX", &PPUInterpreter::LVLX, vd, ra, rb);
}

void PPULLVMRecompiler::LDBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64, false);
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

    auto mem_i32 = ReadMemory(addr_i64, 32, false);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LWBRX", &PPUInterpreter::LWBRX, rd, ra, rb);
}

void PPULLVMRecompiler::LFSX(u32 frd, u32 ra, u32 rb) {
    InterpreterCall("LFSX", &PPUInterpreter::LFSX, frd, ra, rb);
}

void PPULLVMRecompiler::SRW(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SRW", &PPUInterpreter::SRW, ra, rs, rb, rc);
}

void PPULLVMRecompiler::SRD(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SRD", &PPUInterpreter::SRD, ra, rs, rb, rc);
}

void PPULLVMRecompiler::LVRX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVRX", &PPUInterpreter::LVRX, vd, ra, rb);
}

void PPULLVMRecompiler::LSWI(u32 rd, u32 ra, u32 nb) {
    InterpreterCall("LSWI", &PPUInterpreter::LSWI, rd, ra, nb);
}

void PPULLVMRecompiler::LFSUX(u32 frd, u32 ra, u32 rb) {
    InterpreterCall("LFSUX", &PPUInterpreter::LFSUX, frd, ra, rb);
}

void PPULLVMRecompiler::SYNC(u32 l) {
    InterpreterCall("SYNC", &PPUInterpreter::SYNC, l);
}

void PPULLVMRecompiler::LFDX(u32 frd, u32 ra, u32 rb) {
    InterpreterCall("LFDX", &PPUInterpreter::LFDX, frd, ra, rb);
}

void PPULLVMRecompiler::LFDUX(u32 frd, u32 ra, u32 rb) {
    InterpreterCall("LFDUX", &PPUInterpreter::LFDUX, frd, ra, rb);
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

    WriteMemory(addr_i64, GetGpr(rs, 32), false);
    //InterpreterCall("STWBRX", &PPUInterpreter::STWBRX, rs, ra, rb);
}

void PPULLVMRecompiler::STFSX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFSX", &PPUInterpreter::STFSX, frs, ra, rb);
}

void PPULLVMRecompiler::STVRX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVRX", &PPUInterpreter::STVRX, vs, ra, rb);
}

void PPULLVMRecompiler::STFSUX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFSUX", &PPUInterpreter::STFSUX, frs, ra, rb);
}

void PPULLVMRecompiler::STSWI(u32 rd, u32 ra, u32 nb) {
    InterpreterCall("STSWI", &PPUInterpreter::STSWI, rd, ra, nb);
}

void PPULLVMRecompiler::STFDX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFDX", &PPUInterpreter::STFDX, frs, ra, rb);
}

void PPULLVMRecompiler::STFDUX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFDUX", &PPUInterpreter::STFDUX, frs, ra, rb);
}

void PPULLVMRecompiler::LVLXL(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVLXL", &PPUInterpreter::LVLXL, vd, ra, rb);
}

void PPULLVMRecompiler::LHBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i16 = ReadMemory(addr_i64, 16, false);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    //InterpreterCall("LHBRX", &PPUInterpreter::LHBRX, rd, ra, rb);
}

void PPULLVMRecompiler::SRAW(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SRAW", &PPUInterpreter::SRAW, ra, rs, rb, rc);
}

void PPULLVMRecompiler::SRAD(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SRAD", &PPUInterpreter::SRAD, ra, rs, rb, rc);
}

void PPULLVMRecompiler::LVRXL(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVRXL", &PPUInterpreter::LVRXL, vd, ra, rb);
}

void PPULLVMRecompiler::DSS(u32 strm, u32 a) {
    InterpreterCall("DSS", &PPUInterpreter::DSS, strm, a);
}

void PPULLVMRecompiler::SRAWI(u32 ra, u32 rs, u32 sh, bool rc) {
    InterpreterCall("SRAWI", &PPUInterpreter::SRAWI, ra, rs, sh, rc);
}

void PPULLVMRecompiler::SRADI1(u32 ra, u32 rs, u32 sh, bool rc) {
    InterpreterCall("SRADI1", &PPUInterpreter::SRADI1, ra, rs, sh, rc);
}

void PPULLVMRecompiler::SRADI2(u32 ra, u32 rs, u32 sh, bool rc) {
    InterpreterCall("SRADI2", &PPUInterpreter::SRADI2, ra, rs, sh, rc);
}

void PPULLVMRecompiler::EIEIO() {
    InterpreterCall("EIEIO", &PPUInterpreter::EIEIO);
}

void PPULLVMRecompiler::STVLXL(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVLXL", &PPUInterpreter::STVLXL, vs, ra, rb);
}

void PPULLVMRecompiler::STHBRX(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("STHBRX", &PPUInterpreter::STHBRX, rs, ra, rb);
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
    InterpreterCall("STVRXL", &PPUInterpreter::STVRXL, vs, ra, rb);
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
    InterpreterCall("STFIWX", &PPUInterpreter::STFIWX, frs, ra, rb);
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
    InterpreterCall("DCBZ", &PPUInterpreter::DCBZ, ra, rb);
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
    InterpreterCall("LMW", &PPUInterpreter::LMW, rd, ra, d);
}

void PPULLVMRecompiler::STMW(u32 rs, u32 ra, s32 d) {
    InterpreterCall("STMW", &PPUInterpreter::STMW, rs, ra, d);
}

void PPULLVMRecompiler::LFS(u32 frd, u32 ra, s32 d) {
    InterpreterCall("LFS", &PPUInterpreter::LFS, frd, ra, d);
}

void PPULLVMRecompiler::LFSU(u32 frd, u32 ra, s32 ds) {
    InterpreterCall("LFSU", &PPUInterpreter::LFSU, frd, ra, ds);
}

void PPULLVMRecompiler::LFD(u32 frd, u32 ra, s32 d) {
    InterpreterCall("LFD", &PPUInterpreter::LFD, frd, ra, d);
}

void PPULLVMRecompiler::LFDU(u32 frd, u32 ra, s32 ds) {
    InterpreterCall("LFDU", &PPUInterpreter::LFDU, frd, ra, ds);
}

void PPULLVMRecompiler::STFS(u32 frs, u32 ra, s32 d) {
    InterpreterCall("STFS", &PPUInterpreter::STFS, frs, ra, d);
}

void PPULLVMRecompiler::STFSU(u32 frs, u32 ra, s32 d) {
    InterpreterCall("STFSU", &PPUInterpreter::STFSU, frs, ra, d);
}

void PPULLVMRecompiler::STFD(u32 frs, u32 ra, s32 d) {
    InterpreterCall("STFD", &PPUInterpreter::STFD, frs, ra, d);
}

void PPULLVMRecompiler::STFDU(u32 frs, u32 ra, s32 d) {
    InterpreterCall("STFDU", &PPUInterpreter::STFDU, frs, ra, d);
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
    InterpreterCall("FDIVS", &PPUInterpreter::FDIVS, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSUBS(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FSUBS", &PPUInterpreter::FSUBS, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FADDS(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FADDS", &PPUInterpreter::FADDS, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSQRTS(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FSQRTS", &PPUInterpreter::FSQRTS, frd, frb, rc);
}

void PPULLVMRecompiler::FRES(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FRES", &PPUInterpreter::FRES, frd, frb, rc);
}

void PPULLVMRecompiler::FMULS(u32 frd, u32 fra, u32 frc, bool rc) {
    InterpreterCall("FMULS", &PPUInterpreter::FMULS, frd, fra, frc, rc);
}

void PPULLVMRecompiler::FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FMADDS", &PPUInterpreter::FMADDS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FMSUBS", &PPUInterpreter::FMSUBS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FNMSUBS", &PPUInterpreter::FNMSUBS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FNMADDS", &PPUInterpreter::FNMADDS, frd, fra, frc, frb, rc);
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
    InterpreterCall("FDIV", &PPUInterpreter::FDIV, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSUB(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FSUB", &PPUInterpreter::FSUB, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FADD(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FADD", &PPUInterpreter::FADD, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSQRT(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FSQRT", &PPUInterpreter::FSQRT, frd, frb, rc);
}

void PPULLVMRecompiler::FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FSEL", &PPUInterpreter::FSEL, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMUL(u32 frd, u32 fra, u32 frc, bool rc) {
    InterpreterCall("FMUL", &PPUInterpreter::FMUL, frd, fra, frc, rc);
}

void PPULLVMRecompiler::FRSQRTE(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FRSQRTE", &PPUInterpreter::FRSQRTE, frd, frb, rc);
}

void PPULLVMRecompiler::FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FMSUB", &PPUInterpreter::FMSUB, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FMADD", &PPUInterpreter::FMADD, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FNMSUB", &PPUInterpreter::FNMSUB, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FNMADD", &PPUInterpreter::FNMADD, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FCMPO(u32 crfd, u32 fra, u32 frb) {
    InterpreterCall("FCMPO", &PPUInterpreter::FCMPO, crfd, fra, frb);
}

void PPULLVMRecompiler::FNEG(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FNEG", &PPUInterpreter::FNEG, frd, frb, rc);
}

void PPULLVMRecompiler::FMR(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FMR", &PPUInterpreter::FMR, frd, frb, rc);
}

void PPULLVMRecompiler::FNABS(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FNABS", &PPUInterpreter::FNABS, frd, frb, rc);
}

void PPULLVMRecompiler::FABS(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FABS", &PPUInterpreter::FABS, frd, frb, rc);
}

void PPULLVMRecompiler::FCTID(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTID", &PPUInterpreter::FCTID, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIDZ(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTIDZ", &PPUInterpreter::FCTIDZ, frd, frb, rc);
}

void PPULLVMRecompiler::FCFID(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCFID", &PPUInterpreter::FCFID, frd, frb, rc);
}

void PPULLVMRecompiler::UNK(const u32 code, const u32 opcode, const u32 gcode) {
    //InterpreterCall("UNK", &PPUInterpreter::UNK, code, opcode, gcode);
}

void PPULLVMRecompiler::Compile(const u64 address) {
    m_address_to_compiled_block_map_mutex.lock();
    auto i = m_address_to_compiled_block_map.find(address);
    m_address_to_compiled_block_map_mutex.unlock();

    if (i != m_address_to_compiled_block_map.end()) {
        return;
    }

    std::chrono::high_resolution_clock::time_point compilation_start = std::chrono::high_resolution_clock::now();

    auto function_name = fmt::Format("fn_0x%llX", address);
    m_function         = m_module->getFunction(function_name);
    if (!m_function) {

        m_function = (Function *)m_module->getOrInsertFunction(function_name, m_ir_builder->getVoidTy(),
                                                               m_ir_builder->getInt8PtrTy() /*ppu_state*/,
                                                               m_ir_builder->getInt64Ty() /*base_addres*/,
                                                               m_ir_builder->getInt8PtrTy() /*interpreter*/, nullptr);
        m_function->setCallingConv(CallingConv::X86_64_Win64);
        auto arg_i = m_function->arg_begin();
        arg_i->setName("ppu_state");
        (++arg_i)->setName("base_address");
        (++arg_i)->setName("interpreter");

        auto block = BasicBlock::Create(*m_llvm_context, "start", m_function);
        m_ir_builder->SetInsertPoint(block);

        u64 offset = 0;
        m_hit_branch_instruction = false;
        while (!m_hit_branch_instruction) {
            u32 instr = Memory.Read32(address + offset);
            Decode(instr);
            offset += 4;

            SetPc(m_ir_builder->getInt64(address + offset));
        }

        m_ir_builder->CreateRetVoid();
        m_fpm->run(*m_function);
    }

    CompiledBlock block = (CompiledBlock)m_execution_engine->getPointerToFunction(m_function);
    m_address_to_compiled_block_map_mutex.lock();
    m_address_to_compiled_block_map[address] = block;
    m_address_to_compiled_block_map_mutex.unlock();

    std::chrono::high_resolution_clock::time_point compilation_end = std::chrono::high_resolution_clock::now();
    m_compilation_time += std::chrono::duration_cast<std::chrono::duration<double>>(compilation_end - compilation_start);
}

Value * PPULLVMRecompiler::GetPPUState() {
    return m_function->arg_begin();
}

Value * PPULLVMRecompiler::GetBaseAddress() {
    auto i = m_function->arg_begin();
    i++;
    return i;
}

Value * PPULLVMRecompiler::GetInterpreter() {
    auto i = m_function->arg_begin();
    i++;
    i++;
    return i;
}

Value * PPULLVMRecompiler::GetBit(Value * val, u32 n) {
    Value * bit;

    if (val->getType()->isIntegerTy(32)) {
        bit = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_32), val, m_ir_builder->getInt32(1 << (31- n)));
    } else if (val->getType()->isIntegerTy(64)) {
        bit = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_64), val, m_ir_builder->getInt64((u64)1 << (63 - n)));
    } else {
        if (val->getType()->getIntegerBitWidth() != (n + 1)) {
            bit = m_ir_builder->CreateLShr(val, val->getType()->getIntegerBitWidth() - n - 1);
        }

        bit = m_ir_builder->CreateAnd(val, 1);
    }

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

    if (val->getType()->isIntegerTy(32)) {
        nibble = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_32), val, m_ir_builder->getInt32((u64)0xF << ((7 - n) * 4)));
    } else if (val->getType()->isIntegerTy(64)) {
        nibble = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_64), val, m_ir_builder->getInt64((u64)0xF << ((15 - n) * 4)));
    } else {
        if ((val->getType()->getIntegerBitWidth() >> 2) != (n + 1)) {
            nibble = m_ir_builder->CreateLShr(val, (((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4);
        }

        nibble = m_ir_builder->CreateAnd(val, 0xF);
    }

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
    auto pc_i64_ptr = m_ir_builder->CreateBitCast(pc_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateLoad(pc_i64_ptr);
}

void PPULLVMRecompiler::SetPc(Value * val_i64) {
    auto pc_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, PC));
    auto pc_i64_ptr = m_ir_builder->CreateBitCast(pc_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateStore(val_i64, pc_i64_ptr);
}

Value * PPULLVMRecompiler::GetGpr(u32 r, u32 num_bits) {
    auto r_i8_ptr = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, GPR[r]));
    auto r_ix_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getIntNTy(num_bits)->getPointerTo());
    return m_ir_builder->CreateLoad(r_ix_ptr);
}

void PPULLVMRecompiler::SetGpr(u32 r, Value * val_x64) {
    auto r_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, GPR[r]));
    auto r_i64_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    auto val_i64   = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    m_ir_builder->CreateStore(val_i64, r_i64_ptr);
}

Value * PPULLVMRecompiler::GetCr() {
    auto cr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, CR));
    auto cr_i32_ptr = m_ir_builder->CreateBitCast(cr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateLoad(cr_i32_ptr);
}

Value * PPULLVMRecompiler::GetCrField(u32 n) {
    return GetNibble(GetCr(), n);
}

void PPULLVMRecompiler::SetCr(Value * val_x32) {
    auto val_i32    = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto cr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, CR));
    auto cr_i32_ptr = m_ir_builder->CreateBitCast(cr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateStore(val_i32, cr_i32_ptr);
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

Value * PPULLVMRecompiler::GetXer() {
    auto xer_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, XER));
    auto xer_i64_ptr = m_ir_builder->CreateBitCast(xer_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateLoad(xer_i64_ptr);
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
    m_ir_builder->CreateStore(val_i64, xer_i64_ptr);
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

Value * PPULLVMRecompiler::GetVscr() {
    auto vscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = m_ir_builder->CreateBitCast(vscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateLoad(vscr_i32_ptr);
}

void PPULLVMRecompiler::SetVscr(Value * val_x32) {
    auto val_i32      = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto vscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = m_ir_builder->CreateBitCast(vscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateStore(val_i32, vscr_i32_ptr);
}

Value * PPULLVMRecompiler::GetVrAsIntVec(u32 vr, u32 vec_elt_num_bits) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_vec_ptr  = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getIntNTy(vec_elt_num_bits), 128 / vec_elt_num_bits)->getPointerTo());
    return m_ir_builder->CreateLoad(vr_vec_ptr);
}

Value * PPULLVMRecompiler::GetVrAsFloatVec(u32 vr) {
    auto vr_i8_ptr    = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v4f32_ptr = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getFloatTy(), 4)->getPointerTo());
    return m_ir_builder->CreateLoad(vr_v4f32_ptr);
}

Value * PPULLVMRecompiler::GetVrAsDoubleVec(u32 vr) {
    auto vr_i8_ptr    = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v2f64_ptr = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getDoubleTy(), 2)->getPointerTo());
    return m_ir_builder->CreateLoad(vr_v2f64_ptr);
}

void PPULLVMRecompiler::SetVr(u32 vr, Value * val_x128) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(GetPPUState(), (unsigned int)offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto val_i128    = m_ir_builder->CreateBitCast(val_x128, m_ir_builder->getIntNTy(128));
    m_ir_builder->CreateStore(val_i128, vr_i128_ptr);
}

Value * PPULLVMRecompiler::ReadMemory(Value * addr_i64, u32 bits, bool bswap) {
    if (bits != 32) {
        auto eaddr_i64    = m_ir_builder->CreateAdd(addr_i64, GetBaseAddress());
        auto eaddr_ix_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getIntNTy(bits)->getPointerTo());
        auto val_ix       = (Value *)m_ir_builder->CreateLoad(eaddr_ix_ptr);
        if (bits > 8 && bswap) {
            val_ix = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getIntNTy(bits)}), val_ix);
        }

        return val_ix;
    } else {
        auto cmp_i1   = m_ir_builder->CreateICmpULT(addr_i64, m_ir_builder->getInt64(RAW_SPU_BASE_ADDR));
        auto then_bb  = BasicBlock::Create(m_ir_builder->getContext(), "", m_ir_builder->GetInsertBlock()->getParent());
        auto else_bb  = BasicBlock::Create(m_ir_builder->getContext());
        auto merge_bb = BasicBlock::Create(m_ir_builder->getContext());
        m_ir_builder->CreateCondBr(cmp_i1, then_bb, else_bb);

        m_ir_builder->SetInsertPoint(then_bb);
        auto eaddr_i64     = m_ir_builder->CreateAdd(addr_i64, GetBaseAddress());
        auto eaddr_i32_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getInt32Ty()->getPointerTo());
        auto val_then_i32  = (Value *)m_ir_builder->CreateLoad(eaddr_i32_ptr);
        if (bswap) {
            val_then_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), val_then_i32);
        }

        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->GetInsertBlock()->getParent()->getBasicBlockList().push_back(else_bb);
        m_ir_builder->SetInsertPoint(else_bb);
        auto this_ptr  = (Value *)m_ir_builder->getInt64((u64)&Memory);
        this_ptr       = m_ir_builder->CreateIntToPtr(this_ptr, this_ptr->getType()->getPointerTo());
        auto val_else_i32 = Call("Read32", &MemoryBase::Read32<u64>, this_ptr, addr_i64);
        if (!bswap) {
            val_else_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), val_else_i32);
        }
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->GetInsertBlock()->getParent()->getBasicBlockList().push_back(merge_bb);
        m_ir_builder->SetInsertPoint(merge_bb);
        auto phi = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 2);
        phi->addIncoming(val_then_i32, then_bb);
        phi->addIncoming(val_else_i32, else_bb);
        return phi;
    }
}

void PPULLVMRecompiler::WriteMemory(Value * addr_i64, Value * val_ix, bool bswap) {
    if (val_ix->getType()->getIntegerBitWidth() != 32) {
        if (val_ix->getType()->getIntegerBitWidth() > 8 && bswap) {
            val_ix = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {val_ix->getType()}), val_ix);
        }

        auto eaddr_i64    = m_ir_builder->CreateAdd(addr_i64, GetBaseAddress());
        auto eaddr_ix_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, val_ix->getType()->getPointerTo());
        m_ir_builder->CreateStore(val_ix, eaddr_ix_ptr);
    } else {
        auto cmp_i1   = m_ir_builder->CreateICmpULT(addr_i64, m_ir_builder->getInt64(RAW_SPU_BASE_ADDR));
        auto then_bb  = BasicBlock::Create(m_ir_builder->getContext(), "", m_ir_builder->GetInsertBlock()->getParent());
        auto else_bb  = BasicBlock::Create(m_ir_builder->getContext());
        auto merge_bb = BasicBlock::Create(m_ir_builder->getContext());
        m_ir_builder->CreateCondBr(cmp_i1, then_bb, else_bb);

        m_ir_builder->SetInsertPoint(then_bb);
        Value * val_then_ix = val_ix;
        if (bswap) {
            val_then_ix = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), val_then_ix);
        }

        auto eaddr_i64     = m_ir_builder->CreateAdd(addr_i64, GetBaseAddress());
        auto eaddr_i32_ptr = m_ir_builder->CreateIntToPtr(eaddr_i64, m_ir_builder->getInt32Ty()->getPointerTo());
        m_ir_builder->CreateStore(val_then_ix, eaddr_i32_ptr);
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->GetInsertBlock()->getParent()->getBasicBlockList().push_back(else_bb);
        m_ir_builder->SetInsertPoint(else_bb);
        Value * val_else_ix = val_ix;
        if (!bswap) {
            val_else_ix = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::bswap, {m_ir_builder->getInt32Ty()}), val_else_ix);
        }

        auto this_ptr = (Value *)m_ir_builder->getInt64((u64)&Memory);
        this_ptr      = m_ir_builder->CreateIntToPtr(this_ptr, this_ptr->getType()->getPointerTo());
        Call("Write32", &MemoryBase::Write32<u64>, this_ptr, addr_i64, val_else_ix);
        m_ir_builder->CreateBr(merge_bb);

        m_ir_builder->GetInsertBlock()->getParent()->getBasicBlockList().push_back(merge_bb);
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

    return Call(name, function, GetInterpreter(), m_ir_builder->getInt32(args)...);
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

template<class Func, class... Args>
Value * PPULLVMRecompiler::Call(const char * name, Func function, Args... args) {
    typedef std::result_of<Func(Args...)>::type ReturnType;

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

u32                 PPULLVMEmulator::s_num_instances    = 0;
std::mutex          PPULLVMEmulator::s_recompiler_mutex;
PPULLVMRecompiler * PPULLVMEmulator::s_recompiler       = nullptr;

PPULLVMEmulator::PPULLVMEmulator(PPUThread & ppu)
    : m_ppu(ppu)
    , m_interpreter(new PPUInterpreter(ppu))
    , m_decoder(m_interpreter) {
    std::lock_guard<std::mutex> lock(s_recompiler_mutex);

    s_num_instances++;
    if (!s_recompiler) {
        s_recompiler = new PPULLVMRecompiler();
        //s_recompiler->RunAllTests(&m_ppu, (u64)Memory.GetBaseAddr(), m_interpreter);
    }
}

PPULLVMEmulator::~PPULLVMEmulator() {
    std::lock_guard<std::mutex> lock(s_recompiler_mutex);

    s_num_instances--;
    if (s_recompiler && s_num_instances == 0) {
        delete s_recompiler;
        s_recompiler = nullptr;
    }
}

u8 PPULLVMEmulator::DecodeMemory(const u64 address) {
    static u64 last_instr_address = 0;

    PPULLVMRecompiler::CompiledBlock compiled_block = nullptr;
    if (address != (last_instr_address + 4)) {
        compiled_block = s_recompiler->GetCompiledBlock(address);
    }

    last_instr_address = address;

    if (compiled_block) {
        compiled_block(&m_ppu, (u64)Memory.GetBaseAddr(), m_interpreter);
        return 0;
    } else {
        return m_decoder.DecodeMemory(address);
    }
}
