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

using namespace llvm;

PPULLVMRecompilerWorker::PPULLVMRecompilerWorker()
    : m_decoder(this)
    , m_compilation_time(0.0)
    , m_idling_time(0.0) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetDisassembler();

    m_llvm_context = new LLVMContext();
    m_ir_builder   = new IRBuilder<>(*m_llvm_context);
    m_module       = new llvm::Module("Module", *m_llvm_context);

    EngineBuilder engine_builder(m_module);
    engine_builder.setMCPU(sys::getHostCPUName());
    engine_builder.setEngineKind(EngineKind::JIT);
    engine_builder.setOptLevel(CodeGenOpt::Default);
    m_execution_engine = engine_builder.create();

    m_disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);
}

PPULLVMRecompilerWorker::~PPULLVMRecompilerWorker() {
    std::string error;
    raw_fd_ostream log_file("PPULLVMRecompilerWorker.log", error, sys::fs::F_Text);
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
    delete m_ir_builder;
    delete m_llvm_context;
}

void PPULLVMRecompilerWorker::Compile(const u64 address) {
    auto i = m_address_to_compiled_block_map.find(address);
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
            m_decoder.Decode(instr);
            offset += 4;

            SetPc(m_ir_builder->getInt64(address + offset));
        }

        m_ir_builder->CreateRetVoid();
        m_execution_engine->runJITOnFunction(m_function);
    }

    m_address_to_compiled_block_map[address] = (CompiledBlock)m_execution_engine->getPointerToFunction(m_function);

    std::chrono::high_resolution_clock::time_point compilation_end = std::chrono::high_resolution_clock::now();
    m_compilation_time += std::chrono::duration_cast<std::chrono::duration<double>>(compilation_end - compilation_start);
}

PPULLVMRecompilerWorker::CompiledBlock PPULLVMRecompilerWorker::GetCompiledBlock(u64 address) {
    auto i = m_address_to_compiled_block_map.find(address);
    if (i != m_address_to_compiled_block_map.end()) {
        return i->second;
    }

    return nullptr;
}

void PPULLVMRecompilerWorker::NULL_OP() {
    InterpreterCall("NULL_OP", &PPUInterpreter::NULL_OP);
}

void PPULLVMRecompilerWorker::NOP() {
    InterpreterCall("NOP", &PPUInterpreter::NOP);
}

void PPULLVMRecompilerWorker::TDI(u32 to, u32 ra, s32 simm16) {
    InterpreterCall("TDI", &PPUInterpreter::TDI, to, ra, simm16);
}

void PPULLVMRecompilerWorker::TWI(u32 to, u32 ra, s32 simm16) {
    InterpreterCall("TWI", &PPUInterpreter::TWI, to, ra, simm16);
}

void PPULLVMRecompilerWorker::MFVSCR(u32 vd) {
    auto vscr_i32  = GetVscr();
    auto vscr_i128 = m_ir_builder->CreateZExt(vscr_i32, m_ir_builder->getIntNTy(128));
    SetVr(vd, vscr_i128);
}

void PPULLVMRecompilerWorker::MTVSCR(u32 vb) {
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);
    auto vscr_i32 = m_ir_builder->CreateExtractElement(vb_v4i32, m_ir_builder->getInt32(0));
    vscr_i32      = m_ir_builder->CreateAnd(vscr_i32, 0x00010001);
    SetVscr(vscr_i32);
}

void PPULLVMRecompilerWorker::VADDCUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    va_v4i32               = m_ir_builder->CreateXor(va_v4i32, ConstantDataVector::get(*m_llvm_context, not_mask_v4i32));
    auto cmpv4i1           = m_ir_builder->CreateICmpULT(va_v4i32, vb_v4i32);
    auto cmpv4i32          = m_ir_builder->CreateZExt(cmpv4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmpv4i32);

    // TODO: Implement with overflow intrinsics and check if the generated code is better
}

void PPULLVMRecompilerWorker::VADDFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto sum_v4f32 = m_ir_builder->CreateFAdd(va_v4f32, vb_v4f32);
    SetVr(vd, sum_v4f32);
}

void PPULLVMRecompilerWorker::VADDSBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_padds_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set VSCR.SAT
}

void PPULLVMRecompilerWorker::VADDSHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_padds_w), va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);

    // TODO: Set VSCR.SAT
}

void PPULLVMRecompilerWorker::VADDSWS(u32 vd, u32 va, u32 vb) {
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

void PPULLVMRecompilerWorker::VADDUBM(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateAdd(va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);
}

void PPULLVMRecompilerWorker::VADDUBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_paddus_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompilerWorker::VADDUHM(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateAdd(va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);
}

void PPULLVMRecompilerWorker::VADDUHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_paddus_w), va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);

    // TODO: Set SAT
}

void PPULLVMRecompilerWorker::VADDUWM(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = m_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    SetVr(vd, sum_v4i32);
}

void PPULLVMRecompilerWorker::VADDUWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = m_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpULT(sum_v4i32, va_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto res_v4i32 = m_ir_builder->CreateOr(sum_v4i32, cmp_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set SAT
}

void PPULLVMRecompilerWorker::VAND(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder->CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompilerWorker::VANDC(u32 vd, u32 va, u32 vb) {
    auto va_v4i32          = GetVrAsIntVec(va, 32);
    auto vb_v4i32          = GetVrAsIntVec(vb, 32);
    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    vb_v4i32               = m_ir_builder->CreateXor(vb_v4i32, ConstantDataVector::get(*m_llvm_context, not_mask_v4i32));
    auto res_v4i32         = m_ir_builder->CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompilerWorker::VAVGSB(u32 vd, u32 va, u32 vb) {
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

void PPULLVMRecompilerWorker::VAVGSH(u32 vd, u32 va, u32 vb) {
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

void PPULLVMRecompilerWorker::VAVGSW(u32 vd, u32 va, u32 vb) {
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

void PPULLVMRecompilerWorker::VAVGUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto avg_v16i8 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pavg_b), va_v16i8, vb_v16i8);
    SetVr(vd, avg_v16i8);
}

void PPULLVMRecompilerWorker::VAVGUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto avg_v8i16 = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pavg_w), va_v8i16, vb_v8i16);
    SetVr(vd, avg_v8i16);
}

void PPULLVMRecompilerWorker::VAVGUW(u32 vd, u32 va, u32 vb) {
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

void PPULLVMRecompilerWorker::VCFSX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder->CreateSIToFP(vb_v4i32, VectorType::get(m_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale          = (float)((u64)1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = m_ir_builder->CreateFDiv(res_v4f32, ConstantDataVector::get(*m_llvm_context, scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompilerWorker::VCFUX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder->CreateUIToFP(vb_v4i32, VectorType::get(m_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale          = (float)((u64)1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = m_ir_builder->CreateFDiv(res_v4f32, ConstantDataVector::get(*m_llvm_context, scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompilerWorker::VCMPBFP(u32 vd, u32 va, u32 vb) {
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

void PPULLVMRecompilerWorker::VCMPBFP_(u32 vd, u32 va, u32 vb) {
    VCMPBFP(vd, va, vb);

    auto vd_v16i8     = GetVrAsIntVec(vd, 8);
    u8 mask_v16i8[16] = {3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vd_v16i8          = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_ssse3_pshuf_b_128), vd_v16i8, ConstantDataVector::get(*m_llvm_context, mask_v16i8));
    auto vd_v4i32     = m_ir_builder->CreateBitCast(vd_v16i8, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    auto vd_mask_i32  = m_ir_builder->CreateExtractElement(vd_v4i32, m_ir_builder->getInt32(0));
    auto cmp_i1       = m_ir_builder->CreateICmpEQ(vd_mask_i32, m_ir_builder->getInt32(0));
    SetCrField(6, nullptr, nullptr, cmp_i1, nullptr);
}

void PPULLVMRecompilerWorker::VCMPEQFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOEQ(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompilerWorker::VCMPEQFP_(u32 vd, u32 va, u32 vb) {
    VCMPEQFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPEQUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpEQ(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompilerWorker::VCMPEQUB_(u32 vd, u32 va, u32 vb) {
    VCMPEQUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPEQUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpEQ(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompilerWorker::VCMPEQUH_(u32 vd, u32 va, u32 vb) {
    VCMPEQUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPEQUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpEQ(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompilerWorker::VCMPEQUW_(u32 vd, u32 va, u32 vb) {
    VCMPEQUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPGEFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOGE(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompilerWorker::VCMPGEFP_(u32 vd, u32 va, u32 vb) {
    VCMPGEFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPGTFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder->CreateFCmpOGT(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompilerWorker::VCMPGTFP_(u32 vd, u32 va, u32 vb) {
    VCMPGTFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPGTSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpSGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompilerWorker::VCMPGTSB_(u32 vd, u32 va, u32 vb) {
    VCMPGTSB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPGTSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpSGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompilerWorker::VCMPGTSH_(u32 vd, u32 va, u32 vb) {
    VCMPGTSH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPGTSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpSGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompilerWorker::VCMPGTSW_(u32 vd, u32 va, u32 vb) {
    VCMPGTSW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPGTUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder->CreateICmpUGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(m_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompilerWorker::VCMPGTUB_(u32 vd, u32 va, u32 vb) {
    VCMPGTUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPGTUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder->CreateICmpUGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(m_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompilerWorker::VCMPGTUH_(u32 vd, u32 va, u32 vb) {
    VCMPGTUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCMPGTUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder->CreateICmpUGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(m_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompilerWorker::VCMPGTUW_(u32 vd, u32 va, u32 vb) {
    VCMPGTUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompilerWorker::VCTSXS(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VCTSXS", &PPUInterpreter::VCTSXS, vd, uimm5, vb);
}

void PPULLVMRecompilerWorker::VCTUXS(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VCTUXS", &PPUInterpreter::VCTUXS, vd, uimm5, vb);
}

void PPULLVMRecompilerWorker::VEXPTEFP(u32 vd, u32 vb) {
    InterpreterCall("VEXPTEFP", &PPUInterpreter::VEXPTEFP, vd, vb);
}

void PPULLVMRecompilerWorker::VLOGEFP(u32 vd, u32 vb) {
    InterpreterCall("VLOGEFP", &PPUInterpreter::VLOGEFP, vd, vb);
}

void PPULLVMRecompilerWorker::VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) {
    InterpreterCall("VMADDFP", &PPUInterpreter::VMADDFP, vd, va, vc, vb);
}

void PPULLVMRecompilerWorker::VMAXFP(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXFP", &PPUInterpreter::VMAXFP, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMAXSB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXSB", &PPUInterpreter::VMAXSB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMAXSH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXSH", &PPUInterpreter::VMAXSH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMAXSW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXSW", &PPUInterpreter::VMAXSW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMAXUB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXUB", &PPUInterpreter::VMAXUB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMAXUH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXUH", &PPUInterpreter::VMAXUH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMAXUW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMAXUW", &PPUInterpreter::VMAXUW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMHADDSHS", &PPUInterpreter::VMHADDSHS, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMHRADDSHS", &PPUInterpreter::VMHRADDSHS, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMINFP(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINFP", &PPUInterpreter::VMINFP, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMINSB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINSB", &PPUInterpreter::VMINSB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMINSH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINSH", &PPUInterpreter::VMINSH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMINSW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINSW", &PPUInterpreter::VMINSW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMINUB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINUB", &PPUInterpreter::VMINUB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMINUH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINUH", &PPUInterpreter::VMINUH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMINUW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMINUW", &PPUInterpreter::VMINUW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMLADDUHM", &PPUInterpreter::VMLADDUHM, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMRGHB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGHB", &PPUInterpreter::VMRGHB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMRGHH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGHH", &PPUInterpreter::VMRGHH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMRGHW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGHW", &PPUInterpreter::VMRGHW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMRGLB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGLB", &PPUInterpreter::VMRGLB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMRGLH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGLH", &PPUInterpreter::VMRGLH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMRGLW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMRGLW", &PPUInterpreter::VMRGLW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMMBM", &PPUInterpreter::VMSUMMBM, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMSHM", &PPUInterpreter::VMSUMSHM, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMSHS", &PPUInterpreter::VMSUMSHS, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMUBM", &PPUInterpreter::VMSUMUBM, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMUHM", &PPUInterpreter::VMSUMUHM, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VMSUMUHS", &PPUInterpreter::VMSUMUHS, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VMULESB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULESB", &PPUInterpreter::VMULESB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMULESH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULESH", &PPUInterpreter::VMULESH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMULEUB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULEUB", &PPUInterpreter::VMULEUB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMULEUH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULEUH", &PPUInterpreter::VMULEUH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMULOSB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULOSB", &PPUInterpreter::VMULOSB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMULOSH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULOSH", &PPUInterpreter::VMULOSH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMULOUB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULOUB", &PPUInterpreter::VMULOUB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VMULOUH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VMULOUH", &PPUInterpreter::VMULOUH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) {
    InterpreterCall("VNMSUBFP", &PPUInterpreter::VNMSUBFP, vd, va, vc, vb);
}

void PPULLVMRecompilerWorker::VNOR(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VNOR", &PPUInterpreter::VNOR, vd, va, vb);
}

void PPULLVMRecompilerWorker::VOR(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VOR", &PPUInterpreter::VOR, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPERM(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VPERM", &PPUInterpreter::VPERM, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VPKPX(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKPX", &PPUInterpreter::VPKPX, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPKSHSS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKSHSS", &PPUInterpreter::VPKSHSS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPKSHUS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKSHUS", &PPUInterpreter::VPKSHUS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPKSWSS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKSWSS", &PPUInterpreter::VPKSWSS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPKSWUS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKSWUS", &PPUInterpreter::VPKSWUS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPKUHUM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKUHUM", &PPUInterpreter::VPKUHUM, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPKUHUS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKUHUS", &PPUInterpreter::VPKUHUS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPKUWUM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKUWUM", &PPUInterpreter::VPKUWUM, vd, va, vb);
}

void PPULLVMRecompilerWorker::VPKUWUS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VPKUWUS", &PPUInterpreter::VPKUWUS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VREFP(u32 vd, u32 vb) {
    InterpreterCall("VREFP", &PPUInterpreter::VREFP, vd, vb);
}

void PPULLVMRecompilerWorker::VRFIM(u32 vd, u32 vb) {
    InterpreterCall("VRFIM", &PPUInterpreter::VRFIM, vd, vb);
}

void PPULLVMRecompilerWorker::VRFIN(u32 vd, u32 vb) {
    InterpreterCall("VRFIN", &PPUInterpreter::VRFIN, vd, vb);
}

void PPULLVMRecompilerWorker::VRFIP(u32 vd, u32 vb) {
    InterpreterCall("VRFIP", &PPUInterpreter::VRFIP, vd, vb);
}

void PPULLVMRecompilerWorker::VRFIZ(u32 vd, u32 vb) {
    InterpreterCall("VRFIZ", &PPUInterpreter::VRFIZ, vd, vb);
}

void PPULLVMRecompilerWorker::VRLB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VRLB", &PPUInterpreter::VRLB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VRLH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VRLH", &PPUInterpreter::VRLH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VRLW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VRLW", &PPUInterpreter::VRLW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VRSQRTEFP(u32 vd, u32 vb) {
    InterpreterCall("VRSQRTEFP", &PPUInterpreter::VRSQRTEFP, vd, vb);
}

void PPULLVMRecompilerWorker::VSEL(u32 vd, u32 va, u32 vb, u32 vc) {
    InterpreterCall("VSEL", &PPUInterpreter::VSEL, vd, va, vb, vc);
}

void PPULLVMRecompilerWorker::VSL(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSL", &PPUInterpreter::VSL, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSLB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSLB", &PPUInterpreter::VSLB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) {
    InterpreterCall("VSLDOI", &PPUInterpreter::VSLDOI, vd, va, vb, sh);
}

void PPULLVMRecompilerWorker::VSLH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSLH", &PPUInterpreter::VSLH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSLO(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSLO", &PPUInterpreter::VSLO, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSLW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSLW", &PPUInterpreter::VSLW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSPLTB(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VSPLTB", &PPUInterpreter::VSPLTB, vd, uimm5, vb);
}

void PPULLVMRecompilerWorker::VSPLTH(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VSPLTH", &PPUInterpreter::VSPLTH, vd, uimm5, vb);
}

void PPULLVMRecompilerWorker::VSPLTISB(u32 vd, s32 simm5) {
    InterpreterCall("VSPLTISB", &PPUInterpreter::VSPLTISB, vd, simm5);
}

void PPULLVMRecompilerWorker::VSPLTISH(u32 vd, s32 simm5) {
    InterpreterCall("VSPLTISH", &PPUInterpreter::VSPLTISH, vd, simm5);
}

void PPULLVMRecompilerWorker::VSPLTISW(u32 vd, s32 simm5) {
    InterpreterCall("VSPLTISW", &PPUInterpreter::VSPLTISW, vd, simm5);
}

void PPULLVMRecompilerWorker::VSPLTW(u32 vd, u32 uimm5, u32 vb) {
    InterpreterCall("VSPLTW", &PPUInterpreter::VSPLTW, vd, uimm5, vb);
}

void PPULLVMRecompilerWorker::VSR(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSR", &PPUInterpreter::VSR, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSRAB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRAB", &PPUInterpreter::VSRAB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSRAH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRAH", &PPUInterpreter::VSRAH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSRAW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRAW", &PPUInterpreter::VSRAW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSRB(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRB", &PPUInterpreter::VSRB, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSRH(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRH", &PPUInterpreter::VSRH, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSRO(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRO", &PPUInterpreter::VSRO, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSRW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSRW", &PPUInterpreter::VSRW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBCUW(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBCUW", &PPUInterpreter::VSUBCUW, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBFP(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBFP", &PPUInterpreter::VSUBFP, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBSBS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBSBS", &PPUInterpreter::VSUBSBS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBSHS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBSHS", &PPUInterpreter::VSUBSHS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBSWS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBSWS", &PPUInterpreter::VSUBSWS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBUBM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUBM", &PPUInterpreter::VSUBUBM, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBUBS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUBS", &PPUInterpreter::VSUBUBS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBUHM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUHM", &PPUInterpreter::VSUBUHM, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBUHS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUHS", &PPUInterpreter::VSUBUHS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBUWM(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUWM", &PPUInterpreter::VSUBUWM, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUBUWS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUBUWS", &PPUInterpreter::VSUBUWS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUMSWS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUMSWS", &PPUInterpreter::VSUMSWS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUM2SWS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUM2SWS", &PPUInterpreter::VSUM2SWS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUM4SBS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUM4SBS", &PPUInterpreter::VSUM4SBS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUM4SHS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUM4SHS", &PPUInterpreter::VSUM4SHS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VSUM4UBS(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VSUM4UBS", &PPUInterpreter::VSUM4UBS, vd, va, vb);
}

void PPULLVMRecompilerWorker::VUPKHPX(u32 vd, u32 vb) {
    InterpreterCall("VUPKHPX", &PPUInterpreter::VUPKHPX, vd, vb);
}

void PPULLVMRecompilerWorker::VUPKHSB(u32 vd, u32 vb) {
    InterpreterCall("VUPKHSB", &PPUInterpreter::VUPKHSB, vd, vb);
}

void PPULLVMRecompilerWorker::VUPKHSH(u32 vd, u32 vb) {
    InterpreterCall("VUPKHSH", &PPUInterpreter::VUPKHSH, vd, vb);
}

void PPULLVMRecompilerWorker::VUPKLPX(u32 vd, u32 vb) {
    InterpreterCall("VUPKLPX", &PPUInterpreter::VUPKLPX, vd, vb);
}

void PPULLVMRecompilerWorker::VUPKLSB(u32 vd, u32 vb) {
    InterpreterCall("VUPKLSB", &PPUInterpreter::VUPKLSB, vd, vb);
}

void PPULLVMRecompilerWorker::VUPKLSH(u32 vd, u32 vb) {
    InterpreterCall("VUPKLSH", &PPUInterpreter::VUPKLSH, vd, vb);
}

void PPULLVMRecompilerWorker::VXOR(u32 vd, u32 va, u32 vb) {
    InterpreterCall("VXOR", &PPUInterpreter::VXOR, vd, va, vb);
}

void PPULLVMRecompilerWorker::MULLI(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64  = GetGpr(ra);
    auto res_i64 = m_ir_builder->CreateMul(ra_i64, m_ir_builder->getInt64((s64)simm16));
    SetGpr(rd, res_i64);
    //InterpreterCall("MULLI", &PPUInterpreter::MULLI, rd, ra, simm16);
}

void PPULLVMRecompilerWorker::SUBFIC(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64   = GetGpr(ra);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::ssub_with_overflow, {m_ir_builder->getInt64Ty()}), m_ir_builder->getInt64((s64)simm16), ra_i64);
    auto diff_i64 = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, diff_i64);
    SetXerCa(carry_i1);
    //InterpreterCall("SUBFIC", &PPUInterpreter::SUBFIC, rd, ra, simm16);
}

void PPULLVMRecompilerWorker::CMPLI(u32 crfd, u32 l, u32 ra, u32 uimm16) {
    Value * ra_i64;
    if (l == 0) {
        ra_i64 = m_ir_builder->CreateZExt(GetGpr(ra, 32), m_ir_builder->getInt64Ty());
    } else {
        ra_i64 = GetGpr(ra);
    }

    SetCrFieldUnsignedCmp(crfd, ra_i64, m_ir_builder->getInt64(uimm16));
    //InterpreterCall("CMPLI", &PPUInterpreter::CMPLI, crfd, l, ra, uimm16);
}

void PPULLVMRecompilerWorker::CMPI(u32 crfd, u32 l, u32 ra, s32 simm16) {
    Value * ra_i64;
    if (l == 0) {
        ra_i64 = m_ir_builder->CreateSExt(GetGpr(ra, 32), m_ir_builder->getInt64Ty());
    } else {
        ra_i64 = GetGpr(ra);
    }

    SetCrFieldSignedCmp(crfd, ra_i64, m_ir_builder->getInt64((s64)simm16));
    //InterpreterCall("CMPI", &PPUInterpreter::CMPI, crfd, l, ra, simm16);
}

void PPULLVMRecompilerWorker::ADDIC(u32 rd, u32 ra, s32 simm16) {
    auto ra_i64   = GetGpr(ra);
    auto res_s    = m_ir_builder->CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::sadd_with_overflow, {m_ir_builder->getInt64Ty()}), m_ir_builder->getInt64((s64)simm16), ra_i64);
    auto sum_i64  = m_ir_builder->CreateExtractValue(res_s, {0});
    auto carry_i1 = m_ir_builder->CreateExtractValue(res_s, {1});
    SetGpr(rd, sum_i64);
    SetXerCa(carry_i1);
    //InterpreterCall("ADDIC", &PPUInterpreter::ADDIC, rd, ra, simm16);
}

void PPULLVMRecompilerWorker::ADDIC_(u32 rd, u32 ra, s32 simm16) {
    ADDIC(rd, ra, simm16);
    SetCrFieldSignedCmp(0, GetGpr(rd), m_ir_builder->getInt64(0));
    //InterpreterCall("ADDIC_", &PPUInterpreter::ADDIC_, rd, ra, simm16);
}

void PPULLVMRecompilerWorker::ADDI(u32 rd, u32 ra, s32 simm16) {
    if (ra == 0) {
        SetGpr(rd, m_ir_builder->getInt64((s64)simm16));
    } else {
        auto ra_i64  = GetGpr(ra);
        auto sum_i64 = m_ir_builder->CreateAdd(ra_i64, m_ir_builder->getInt64((s64)simm16));
        SetGpr(rd, sum_i64);
    }
    //InterpreterCall("ADDI", &PPUInterpreter::ADDI, rd, ra, simm16);
}

void PPULLVMRecompilerWorker::ADDIS(u32 rd, u32 ra, s32 simm16) {
    if (ra == 0) {
        SetGpr(rd, m_ir_builder->getInt64((s64)simm16 << 16));
    } else {
        auto ra_i64  = GetGpr(ra);
        auto sum_i64 = m_ir_builder->CreateAdd(ra_i64, m_ir_builder->getInt64((s64)simm16 << 16));
        SetGpr(rd, sum_i64);
    }
    //InterpreterCall("ADDIS", &PPUInterpreter::ADDIS, rd, ra, simm16);
}

void PPULLVMRecompilerWorker::BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) {
    InterpreterCall("BC", &PPUInterpreter::BC, bo, bi, bd, aa, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompilerWorker::SC(u32 sc_code) {
    InterpreterCall("SC", &PPUInterpreter::SC, sc_code);
}

void PPULLVMRecompilerWorker::B(s32 ll, u32 aa, u32 lk) {
    InterpreterCall("B", &PPUInterpreter::B, ll, aa, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompilerWorker::MCRF(u32 crfd, u32 crfs) {
    if (crfd != crfs) {
        auto cr_i32  = GetCr();
        auto crf_i32 = GetNibble(cr_i32, crfs);
        cr_i32       = SetNibble(cr_i32, crfd, crf_i32);
        SetCr(cr_i32);
    }
    //InterpreterCall("MCRF", &PPUInterpreter::MCRF, crfd, crfs);
}

void PPULLVMRecompilerWorker::BCLR(u32 bo, u32 bi, u32 bh, u32 lk) {
    InterpreterCall("BCLR", &PPUInterpreter::BCLR, bo, bi, bh, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompilerWorker::CRNOR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateOr(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRNOR", &PPUInterpreter::CRNOR, crbd, crba, crbb);
}

void PPULLVMRecompilerWorker::CRANDC(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(bb_i32, 1);
    res_i32      = m_ir_builder->CreateAnd(ba_i32, res_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRANDC", &PPUInterpreter::CRANDC, crbd, crba, crbb);
}

void PPULLVMRecompilerWorker::ISYNC() {
    m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_mfence));
}

void PPULLVMRecompilerWorker::CRXOR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRXOR", &PPUInterpreter::CRXOR, crbd, crba, crbb);
}

void PPULLVMRecompilerWorker::CRNAND(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateAnd(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRNAND", &PPUInterpreter::CRNAND, crbd, crba, crbb);
}

void PPULLVMRecompilerWorker::CRAND(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateAnd(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRAND", &PPUInterpreter::CRAND, crbd, crba, crbb);
}

void PPULLVMRecompilerWorker::CREQV(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(ba_i32, bb_i32);
    res_i32      = m_ir_builder->CreateXor(res_i32, 1);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CREQV", &PPUInterpreter::CREQV, crbd, crba, crbb);
}

void PPULLVMRecompilerWorker::CRORC(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateXor(bb_i32, 1);
    res_i32      = m_ir_builder->CreateOr(ba_i32, res_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CRORC", &PPUInterpreter::CRORC, crbd, crba, crbb);
}

void PPULLVMRecompilerWorker::CROR(u32 crbd, u32 crba, u32 crbb) {
    auto cr_i32  = GetCr();
    auto ba_i32  = GetBit(cr_i32, crba);
    auto bb_i32  = GetBit(cr_i32, crbb);
    auto res_i32 = m_ir_builder->CreateOr(ba_i32, bb_i32);
    cr_i32       = SetBit(cr_i32, crbd, res_i32);
    SetCr(cr_i32);
    //InterpreterCall("CROR", &PPUInterpreter::CROR, crbd, crba, crbb);
}

void PPULLVMRecompilerWorker::BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) {
    InterpreterCall("BCCTR", &PPUInterpreter::BCCTR, bo, bi, bh, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompilerWorker::RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    InterpreterCall("RLWIMI", &PPUInterpreter::RLWIMI, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompilerWorker::RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    InterpreterCall("RLWINM", &PPUInterpreter::RLWINM, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompilerWorker::RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc) {
    InterpreterCall("RLWNM", &PPUInterpreter::RLWNM, ra, rs, rb, mb, me, rc);
}

void PPULLVMRecompilerWorker::ORI(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, uimm16);
    SetGpr(ra, res_i64);
    //InterpreterCall("ORI", &PPUInterpreter::ORI, ra, rs, uimm16);
}

void PPULLVMRecompilerWorker::ORIS(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
    //InterpreterCall("ORIS", &PPUInterpreter::ORIS, ra, rs, uimm16);
}

void PPULLVMRecompilerWorker::XORI(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, uimm16);
    SetGpr(ra, res_i64);
    //InterpreterCall("XORI", &PPUInterpreter::XORI, ra, rs, uimm16);
}

void PPULLVMRecompilerWorker::XORIS(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
    //InterpreterCall("XORIS", &PPUInterpreter::XORIS, ra, rs, uimm16);
}

void PPULLVMRecompilerWorker::ANDI_(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, uimm16);
    SetGpr(ra, res_i64);
    SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    //InterpreterCall("ANDI_", &PPUInterpreter::ANDI_, ra, rs, uimm16);
}

void PPULLVMRecompilerWorker::ANDIS_(u32 ra, u32 rs, u32 uimm16) {
    auto rs_i64  = GetGpr(rs);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, (u64)uimm16 << 16);
    SetGpr(ra, res_i64);
    SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    //InterpreterCall("ANDIS_", &PPUInterpreter::ANDIS_, ra, rs, uimm16);
}

void PPULLVMRecompilerWorker::RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    InterpreterCall("RLDICL", &PPUInterpreter::RLDICL, ra, rs, sh, mb, rc);
}

void PPULLVMRecompilerWorker::RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) {
    InterpreterCall("RLDICR", &PPUInterpreter::RLDICR, ra, rs, sh, me, rc);
}

void PPULLVMRecompilerWorker::RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    InterpreterCall("RLDIC", &PPUInterpreter::RLDIC, ra, rs, sh, mb, rc);
}

void PPULLVMRecompilerWorker::RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    InterpreterCall("RLDIMI", &PPUInterpreter::RLDIMI, ra, rs, sh, mb, rc);
}

void PPULLVMRecompilerWorker::RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) {
    InterpreterCall("RLDC_LR", &PPUInterpreter::RLDC_LR, ra, rs, rb, m_eb, is_r, rc);
}

void PPULLVMRecompilerWorker::CMP(u32 crfd, u32 l, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::TW(u32 to, u32 ra, u32 rb) {
    InterpreterCall("TW", &PPUInterpreter::TW, to, ra, rb);
}

void PPULLVMRecompilerWorker::LVSL(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVSL", &PPUInterpreter::LVSL, vd, ra, rb);
}

void PPULLVMRecompilerWorker::LVEBX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVEBX", &PPUInterpreter::LVEBX, vd, ra, rb);
}

void PPULLVMRecompilerWorker::SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("SUBFC", &PPUInterpreter::SUBFC, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("ADDC", &PPUInterpreter::ADDC, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::MULHDU(u32 rd, u32 ra, u32 rb, bool rc) {
    InterpreterCall("MULHDU", &PPUInterpreter::MULHDU, rd, ra, rb, rc);
}

void PPULLVMRecompilerWorker::MULHWU(u32 rd, u32 ra, u32 rb, bool rc) {
    InterpreterCall("MULHWU", &PPUInterpreter::MULHWU, rd, ra, rb, rc);
}

void PPULLVMRecompilerWorker::MFOCRF(u32 a, u32 rd, u32 crm) {
    InterpreterCall("MFOCRF", &PPUInterpreter::MFOCRF, a, rd, crm);
}

void PPULLVMRecompilerWorker::LWARX(u32 rd, u32 ra, u32 rb) {
    InterpreterCall("LWARX", &PPUInterpreter::LWARX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::LDX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    //InterpreterCall("LDX", &PPUInterpreter::LDX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::LWZX(u32 rd, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::SLW(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SLW", &PPUInterpreter::SLW, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::CNTLZW(u32 ra, u32 rs, bool rc) {
    InterpreterCall("CNTLZW", &PPUInterpreter::CNTLZW, ra, rs, rc);
}

void PPULLVMRecompilerWorker::SLD(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SLD", &PPUInterpreter::SLD, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::AND(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateAnd(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("AND", &PPUInterpreter::AND, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::CMPL(u32 crfd, u32 l, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::LVSR(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVSR", &PPUInterpreter::LVSR, vd, ra, rb);
}

void PPULLVMRecompilerWorker::LVEHX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVEHX", &PPUInterpreter::LVEHX, vd, ra, rb);
}

void PPULLVMRecompilerWorker::SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("SUBF", &PPUInterpreter::SUBF, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::LDUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LDUX", &PPUInterpreter::LDUX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::DCBST(u32 ra, u32 rb) {
    InterpreterCall("DCBST", &PPUInterpreter::DCBST, ra, rb);
}

void PPULLVMRecompilerWorker::LWZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LWZUX", &PPUInterpreter::LWZUX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::CNTLZD(u32 ra, u32 rs, bool rc) {
    InterpreterCall("CNTLZD", &PPUInterpreter::CNTLZD, ra, rs, rc);
}

void PPULLVMRecompilerWorker::ANDC(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("ANDC", &PPUInterpreter::ANDC, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::TD(u32 to, u32 ra, u32 rb) {
    InterpreterCall("TD", &PPUInterpreter::TD, to, ra, rb);
}

void PPULLVMRecompilerWorker::LVEWX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVEWX", &PPUInterpreter::LVEWX, vd, ra, rb);
}

void PPULLVMRecompilerWorker::MULHD(u32 rd, u32 ra, u32 rb, bool rc) {
    InterpreterCall("MULHD", &PPUInterpreter::MULHD, rd, ra, rb, rc);
}

void PPULLVMRecompilerWorker::MULHW(u32 rd, u32 ra, u32 rb, bool rc) {
    InterpreterCall("MULHW", &PPUInterpreter::MULHW, rd, ra, rb, rc);
}

void PPULLVMRecompilerWorker::LDARX(u32 rd, u32 ra, u32 rb) {
    InterpreterCall("LDARX", &PPUInterpreter::LDARX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::DCBF(u32 ra, u32 rb) {
    InterpreterCall("DCBF", &PPUInterpreter::DCBF, ra, rb);
}

void PPULLVMRecompilerWorker::LBZX(u32 rd, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::LVX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVX", &PPUInterpreter::LVX, vd, ra, rb);
}

void PPULLVMRecompilerWorker::NEG(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("NEG", &PPUInterpreter::NEG, rd, ra, oe, rc);
}

void PPULLVMRecompilerWorker::LBZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LBZUX", &PPUInterpreter::LBZUX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::NOR(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("NOR", &PPUInterpreter::NOR, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::STVEBX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVEBX", &PPUInterpreter::STVEBX, vs, ra, rb);
}

void PPULLVMRecompilerWorker::SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("SUBFE", &PPUInterpreter::SUBFE, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("ADDE", &PPUInterpreter::ADDE, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::MTOCRF(u32 l, u32 crm, u32 rs) {
    InterpreterCall("MTOCRF", &PPUInterpreter::MTOCRF, l, crm, rs);
}

void PPULLVMRecompilerWorker::STDX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 64));
    //InterpreterCall("STDX", &PPUInterpreter::STDX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STWCX_(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("STWCX_", &PPUInterpreter::STWCX_, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STWX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32));
    //InterpreterCall("STWX", &PPUInterpreter::STWX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STVEHX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVEHX", &PPUInterpreter::STVEHX, vs, ra, rb);
}

void PPULLVMRecompilerWorker::STDUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 64));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STDUX", &PPUInterpreter::STDUX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STWUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 32));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STWUX", &PPUInterpreter::STWUX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STVEWX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVEWX", &PPUInterpreter::STVEWX, vs, ra, rb);
}

void PPULLVMRecompilerWorker::ADDZE(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("ADDZE", &PPUInterpreter::ADDZE, rd, ra, oe, rc);
}

void PPULLVMRecompilerWorker::SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("SUBFZE", &PPUInterpreter::SUBFZE, rd, ra, oe, rc);
}

void PPULLVMRecompilerWorker::STDCX_(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("STDCX_", &PPUInterpreter::STDCX_, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STBX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 8));
    //InterpreterCall("STBX", &PPUInterpreter::STBX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STVX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVX", &PPUInterpreter::STVX, vs, ra, rb);
}

void PPULLVMRecompilerWorker::SUBFME(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("SUBFME", &PPUInterpreter::SUBFME, rd, ra, oe, rc);
}

void PPULLVMRecompilerWorker::MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("MULLD", &PPUInterpreter::MULLD, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::ADDME(u32 rd, u32 ra, u32 oe, bool rc) {
    InterpreterCall("ADDME", &PPUInterpreter::ADDME, rd, ra, oe, rc);
}

void PPULLVMRecompilerWorker::MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("MULLW", &PPUInterpreter::MULLW, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::DCBTST(u32 ra, u32 rb, u32 th) {
    InterpreterCall("DCBTST", &PPUInterpreter::DCBTST, ra, rb, th);
}

void PPULLVMRecompilerWorker::STBUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 8));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STBUX", &PPUInterpreter::STBUX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
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

void PPULLVMRecompilerWorker::DCBT(u32 ra, u32 rb, u32 th) {
    InterpreterCall("DCBT", &PPUInterpreter::DCBT, ra, rb, th);
}

void PPULLVMRecompilerWorker::LHZX(u32 rd, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::EQV(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("EQV", &PPUInterpreter::EQV, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::ECIWX(u32 rd, u32 ra, u32 rb) {
    InterpreterCall("ECIWX", &PPUInterpreter::ECIWX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::LHZUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LHZUX", &PPUInterpreter::LHZUX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::XOR(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateXor(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("XOR", &PPUInterpreter::XOR, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::MFSPR(u32 rd, u32 spr) {
    InterpreterCall("MFSPR", &PPUInterpreter::MFSPR, rd, spr);
}

void PPULLVMRecompilerWorker::LWAX(u32 rd, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::DST(u32 ra, u32 rb, u32 strm, u32 t) {
    InterpreterCall("DST", &PPUInterpreter::DST, ra, rb, strm, t);
}

void PPULLVMRecompilerWorker::LHAX(u32 rd, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::LVXL(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVXL", &PPUInterpreter::LVXL, vd, ra, rb);
}

void PPULLVMRecompilerWorker::MFTB(u32 rd, u32 spr) {
    InterpreterCall("MFTB", &PPUInterpreter::MFTB, rd, spr);
}

void PPULLVMRecompilerWorker::LWAUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LWAUX", &PPUInterpreter::LWAUX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::DSTST(u32 ra, u32 rb, u32 strm, u32 t) {
    InterpreterCall("DSTST", &PPUInterpreter::DSTST, ra, rb, strm, t);
}

void PPULLVMRecompilerWorker::LHAUX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LHAUX", &PPUInterpreter::LHAUX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::STHX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 16));
    //InterpreterCall("STHX", &PPUInterpreter::STHX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::ORC(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("ORC", &PPUInterpreter::ORC, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::ECOWX(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("ECOWX", &PPUInterpreter::ECOWX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STHUX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 16));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STHUX", &PPUInterpreter::STHUX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::OR(u32 ra, u32 rs, u32 rb, bool rc) {
    auto rs_i64  = GetGpr(rs);
    auto rb_i64  = GetGpr(rb);
    auto res_i64 = m_ir_builder->CreateOr(rs_i64, rb_i64);
    SetGpr(ra, res_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, res_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("OR", &PPUInterpreter::OR, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("DIVDU", &PPUInterpreter::DIVDU, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("DIVWU", &PPUInterpreter::DIVWU, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::MTSPR(u32 spr, u32 rs) {
    InterpreterCall("MTSPR", &PPUInterpreter::MTSPR, spr, rs);
}

void PPULLVMRecompilerWorker::NAND(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("NAND", &PPUInterpreter::NAND, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::STVXL(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVXL", &PPUInterpreter::STVXL, vs, ra, rb);
}

void PPULLVMRecompilerWorker::DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("DIVD", &PPUInterpreter::DIVD, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    InterpreterCall("DIVW", &PPUInterpreter::DIVW, rd, ra, rb, oe, rc);
}

void PPULLVMRecompilerWorker::LVLX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVLX", &PPUInterpreter::LVLX, vd, ra, rb);
}

void PPULLVMRecompilerWorker::LDBRX(u32 rd, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64, false);
    SetGpr(rd, mem_i64);
    //InterpreterCall("LDBRX", &PPUInterpreter::LDBRX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::LSWX(u32 rd, u32 ra, u32 rb) {
    InterpreterCall("LSWX", &PPUInterpreter::LSWX, rd, ra, rb);
}

void PPULLVMRecompilerWorker::LWBRX(u32 rd, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::LFSX(u32 frd, u32 ra, u32 rb) {
    InterpreterCall("LFSX", &PPUInterpreter::LFSX, frd, ra, rb);
}

void PPULLVMRecompilerWorker::SRW(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SRW", &PPUInterpreter::SRW, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::SRD(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SRD", &PPUInterpreter::SRD, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::LVRX(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVRX", &PPUInterpreter::LVRX, vd, ra, rb);
}

void PPULLVMRecompilerWorker::LSWI(u32 rd, u32 ra, u32 nb) {
    InterpreterCall("LSWI", &PPUInterpreter::LSWI, rd, ra, nb);
}

void PPULLVMRecompilerWorker::LFSUX(u32 frd, u32 ra, u32 rb) {
    InterpreterCall("LFSUX", &PPUInterpreter::LFSUX, frd, ra, rb);
}

void PPULLVMRecompilerWorker::SYNC(u32 l) {
    InterpreterCall("SYNC", &PPUInterpreter::SYNC, l);
}

void PPULLVMRecompilerWorker::LFDX(u32 frd, u32 ra, u32 rb) {
    InterpreterCall("LFDX", &PPUInterpreter::LFDX, frd, ra, rb);
}

void PPULLVMRecompilerWorker::LFDUX(u32 frd, u32 ra, u32 rb) {
    InterpreterCall("LFDUX", &PPUInterpreter::LFDUX, frd, ra, rb);
}

void PPULLVMRecompilerWorker::STVLX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVLX", &PPUInterpreter::STVLX, vs, ra, rb);
}

void PPULLVMRecompilerWorker::STSWX(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("STSWX", &PPUInterpreter::STSWX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STWBRX(u32 rs, u32 ra, u32 rb) {
    auto addr_i64 = GetGpr(rb);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32), false);
    //InterpreterCall("STWBRX", &PPUInterpreter::STWBRX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::STFSX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFSX", &PPUInterpreter::STFSX, frs, ra, rb);
}

void PPULLVMRecompilerWorker::STVRX(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVRX", &PPUInterpreter::STVRX, vs, ra, rb);
}

void PPULLVMRecompilerWorker::STFSUX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFSUX", &PPUInterpreter::STFSUX, frs, ra, rb);
}

void PPULLVMRecompilerWorker::STSWI(u32 rd, u32 ra, u32 nb) {
    InterpreterCall("STSWI", &PPUInterpreter::STSWI, rd, ra, nb);
}

void PPULLVMRecompilerWorker::STFDX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFDX", &PPUInterpreter::STFDX, frs, ra, rb);
}

void PPULLVMRecompilerWorker::STFDUX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFDUX", &PPUInterpreter::STFDUX, frs, ra, rb);
}

void PPULLVMRecompilerWorker::LVLXL(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVLXL", &PPUInterpreter::LVLXL, vd, ra, rb);
}

void PPULLVMRecompilerWorker::LHBRX(u32 rd, u32 ra, u32 rb) {
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

void PPULLVMRecompilerWorker::SRAW(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SRAW", &PPUInterpreter::SRAW, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::SRAD(u32 ra, u32 rs, u32 rb, bool rc) {
    InterpreterCall("SRAD", &PPUInterpreter::SRAD, ra, rs, rb, rc);
}

void PPULLVMRecompilerWorker::LVRXL(u32 vd, u32 ra, u32 rb) {
    InterpreterCall("LVRXL", &PPUInterpreter::LVRXL, vd, ra, rb);
}

void PPULLVMRecompilerWorker::DSS(u32 strm, u32 a) {
    InterpreterCall("DSS", &PPUInterpreter::DSS, strm, a);
}

void PPULLVMRecompilerWorker::SRAWI(u32 ra, u32 rs, u32 sh, bool rc) {
    InterpreterCall("SRAWI", &PPUInterpreter::SRAWI, ra, rs, sh, rc);
}

void PPULLVMRecompilerWorker::SRADI1(u32 ra, u32 rs, u32 sh, bool rc) {
    InterpreterCall("SRADI1", &PPUInterpreter::SRADI1, ra, rs, sh, rc);
}

void PPULLVMRecompilerWorker::SRADI2(u32 ra, u32 rs, u32 sh, bool rc) {
    InterpreterCall("SRADI2", &PPUInterpreter::SRADI2, ra, rs, sh, rc);
}

void PPULLVMRecompilerWorker::EIEIO() {
    InterpreterCall("EIEIO", &PPUInterpreter::EIEIO);
}

void PPULLVMRecompilerWorker::STVLXL(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVLXL", &PPUInterpreter::STVLXL, vs, ra, rb);
}

void PPULLVMRecompilerWorker::STHBRX(u32 rs, u32 ra, u32 rb) {
    InterpreterCall("STHBRX", &PPUInterpreter::STHBRX, rs, ra, rb);
}

void PPULLVMRecompilerWorker::EXTSH(u32 ra, u32 rs, bool rc) {
    auto rs_i16 = GetGpr(rs, 16);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i16, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("EXTSH", &PPUInterpreter::EXTSH, ra, rs, rc);
}

void PPULLVMRecompilerWorker::STVRXL(u32 vs, u32 ra, u32 rb) {
    InterpreterCall("STVRXL", &PPUInterpreter::STVRXL, vs, ra, rb);
}

void PPULLVMRecompilerWorker::EXTSB(u32 ra, u32 rs, bool rc) {
    auto rs_i8  = GetGpr(rs, 8);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i8, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("EXTSB", &PPUInterpreter::EXTSB, ra, rs, rc);
}

void PPULLVMRecompilerWorker::STFIWX(u32 frs, u32 ra, u32 rb) {
    InterpreterCall("STFIWX", &PPUInterpreter::STFIWX, frs, ra, rb);
}

void PPULLVMRecompilerWorker::EXTSW(u32 ra, u32 rs, bool rc) {
    auto rs_i32 = GetGpr(rs, 32);
    auto rs_i64 = m_ir_builder->CreateSExt(rs_i32, m_ir_builder->getInt64Ty());
    SetGpr(ra, rs_i64);

    if (rc) {
        SetCrFieldSignedCmp(0, rs_i64, m_ir_builder->getInt64(0));
    }
    //InterpreterCall("EXTSW", &PPUInterpreter::EXTSW, ra, rs, rc);
}

void PPULLVMRecompilerWorker::ICBI(u32 ra, u32 rs) {
    InterpreterCall("ICBI", &PPUInterpreter::ICBI, ra, rs);
}

void PPULLVMRecompilerWorker::DCBZ(u32 ra, u32 rb) {
    InterpreterCall("DCBZ", &PPUInterpreter::DCBZ, ra, rb);
}

void PPULLVMRecompilerWorker::LWZ(u32 rd, u32 ra, s32 d) {
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

void PPULLVMRecompilerWorker::LWZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i32 = ReadMemory(addr_i64, 32);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i32, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LWZU", &PPUInterpreter::LWZU, rd, ra, d);
}

void PPULLVMRecompilerWorker::LBZ(u32 rd, u32 ra, s32 d) {
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

void PPULLVMRecompilerWorker::LBZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i8  = ReadMemory(addr_i64, 8);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i8, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LBZU", &PPUInterpreter::LBZU, rd, ra, d);
}

void PPULLVMRecompilerWorker::STW(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 32));
    //InterpreterCall("STW", &PPUInterpreter::STW, rs, ra, d);
}

void PPULLVMRecompilerWorker::STWU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 32));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STWU", &PPUInterpreter::STWU, rs, ra, d);
}

void PPULLVMRecompilerWorker::STB(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 8));
    //InterpreterCall("STB", &PPUInterpreter::STB, rs, ra, d);
}

void PPULLVMRecompilerWorker::STBU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 8));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STBU", &PPUInterpreter::STBU, rs, ra, d);
}

void PPULLVMRecompilerWorker::LHZ(u32 rd, u32 ra, s32 d) {
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

void PPULLVMRecompilerWorker::LHZU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateZExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LHZU", &PPUInterpreter::LHZU, rd, ra, d);
}

void PPULLVMRecompilerWorker::LHA(u32 rd, u32 ra, s32 d) {
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

void PPULLVMRecompilerWorker::LHAU(u32 rd, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i16 = ReadMemory(addr_i64, 16);
    auto mem_i64 = m_ir_builder->CreateSExt(mem_i16, m_ir_builder->getInt64Ty());
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LHAU", &PPUInterpreter::LHAU, rd, ra, d);
}

void PPULLVMRecompilerWorker::STH(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 16));
    //InterpreterCall("STH", &PPUInterpreter::STH, rs, ra, d);
}

void PPULLVMRecompilerWorker::STHU(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 16));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STHU", &PPUInterpreter::STHU, rs, ra, d);
}

void PPULLVMRecompilerWorker::LMW(u32 rd, u32 ra, s32 d) {
    InterpreterCall("LMW", &PPUInterpreter::LMW, rd, ra, d);
}

void PPULLVMRecompilerWorker::STMW(u32 rs, u32 ra, s32 d) {
    InterpreterCall("STMW", &PPUInterpreter::STMW, rs, ra, d);
}

void PPULLVMRecompilerWorker::LFS(u32 frd, u32 ra, s32 d) {
    InterpreterCall("LFS", &PPUInterpreter::LFS, frd, ra, d);
}

void PPULLVMRecompilerWorker::LFSU(u32 frd, u32 ra, s32 ds) {
    InterpreterCall("LFSU", &PPUInterpreter::LFSU, frd, ra, ds);
}

void PPULLVMRecompilerWorker::LFD(u32 frd, u32 ra, s32 d) {
    InterpreterCall("LFD", &PPUInterpreter::LFD, frd, ra, d);
}

void PPULLVMRecompilerWorker::LFDU(u32 frd, u32 ra, s32 ds) {
    InterpreterCall("LFDU", &PPUInterpreter::LFDU, frd, ra, ds);
}

void PPULLVMRecompilerWorker::STFS(u32 frs, u32 ra, s32 d) {
    InterpreterCall("STFS", &PPUInterpreter::STFS, frs, ra, d);
}

void PPULLVMRecompilerWorker::STFSU(u32 frs, u32 ra, s32 d) {
    InterpreterCall("STFSU", &PPUInterpreter::STFSU, frs, ra, d);
}

void PPULLVMRecompilerWorker::STFD(u32 frs, u32 ra, s32 d) {
    InterpreterCall("STFD", &PPUInterpreter::STFD, frs, ra, d);
}

void PPULLVMRecompilerWorker::STFDU(u32 frs, u32 ra, s32 d) {
    InterpreterCall("STFDU", &PPUInterpreter::STFDU, frs, ra, d);
}

void PPULLVMRecompilerWorker::LD(u32 rd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    //InterpreterCall("LD", &PPUInterpreter::LD, rd, ra, ds);
}

void PPULLVMRecompilerWorker::LDU(u32 rd, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    auto mem_i64 = ReadMemory(addr_i64, 64);
    SetGpr(rd, mem_i64);
    SetGpr(ra, addr_i64);
    //InterpreterCall("LDU", &PPUInterpreter::LDU, rd, ra, ds);
}

void PPULLVMRecompilerWorker::LWA(u32 rd, u32 ra, s32 ds) {
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

void PPULLVMRecompilerWorker::FDIVS(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FDIVS", &PPUInterpreter::FDIVS, frd, fra, frb, rc);
}

void PPULLVMRecompilerWorker::FSUBS(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FSUBS", &PPUInterpreter::FSUBS, frd, fra, frb, rc);
}

void PPULLVMRecompilerWorker::FADDS(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FADDS", &PPUInterpreter::FADDS, frd, fra, frb, rc);
}

void PPULLVMRecompilerWorker::FSQRTS(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FSQRTS", &PPUInterpreter::FSQRTS, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FRES(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FRES", &PPUInterpreter::FRES, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FMULS(u32 frd, u32 fra, u32 frc, bool rc) {
    InterpreterCall("FMULS", &PPUInterpreter::FMULS, frd, fra, frc, rc);
}

void PPULLVMRecompilerWorker::FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FMADDS", &PPUInterpreter::FMADDS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FMSUBS", &PPUInterpreter::FMSUBS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FNMSUBS", &PPUInterpreter::FNMSUBS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FNMADDS", &PPUInterpreter::FNMADDS, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::STD(u32 rs, u32 ra, s32 d) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)d);
    if (ra) {
        auto ra_i64 = GetGpr(ra);
        addr_i64    = m_ir_builder->CreateAdd(ra_i64, addr_i64);
    }

    WriteMemory(addr_i64, GetGpr(rs, 64));
    //InterpreterCall("STD", &PPUInterpreter::STD, rs, ra, d);
}

void PPULLVMRecompilerWorker::STDU(u32 rs, u32 ra, s32 ds) {
    auto addr_i64 = (Value *)m_ir_builder->getInt64((s64)ds);
    auto ra_i64   = GetGpr(ra);
    addr_i64      = m_ir_builder->CreateAdd(ra_i64, addr_i64);

    WriteMemory(addr_i64, GetGpr(rs, 64));
    SetGpr(ra, addr_i64);
    //InterpreterCall("STDU", &PPUInterpreter::STDU, rs, ra, ds);
}

void PPULLVMRecompilerWorker::MTFSB1(u32 crbd, bool rc) {
    InterpreterCall("MTFSB1", &PPUInterpreter::MTFSB1, crbd, rc);
}

void PPULLVMRecompilerWorker::MCRFS(u32 crbd, u32 crbs) {
    InterpreterCall("MCRFS", &PPUInterpreter::MCRFS, crbd, crbs);
}

void PPULLVMRecompilerWorker::MTFSB0(u32 crbd, bool rc) {
    InterpreterCall("MTFSB0", &PPUInterpreter::MTFSB0, crbd, rc);
}

void PPULLVMRecompilerWorker::MTFSFI(u32 crfd, u32 i, bool rc) {
    InterpreterCall("MTFSFI", &PPUInterpreter::MTFSFI, crfd, i, rc);
}

void PPULLVMRecompilerWorker::MFFS(u32 frd, bool rc) {
    InterpreterCall("MFFS", &PPUInterpreter::MFFS, frd, rc);
}

void PPULLVMRecompilerWorker::MTFSF(u32 flm, u32 frb, bool rc) {
    InterpreterCall("MTFSF", &PPUInterpreter::MTFSF, flm, frb, rc);
}

void PPULLVMRecompilerWorker::FCMPU(u32 crfd, u32 fra, u32 frb) {
    InterpreterCall("FCMPU", &PPUInterpreter::FCMPU, crfd, fra, frb);
}

void PPULLVMRecompilerWorker::FRSP(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FRSP", &PPUInterpreter::FRSP, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FCTIW(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTIW", &PPUInterpreter::FCTIW, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FCTIWZ(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTIWZ", &PPUInterpreter::FCTIWZ, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FDIV(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FDIV", &PPUInterpreter::FDIV, frd, fra, frb, rc);
}

void PPULLVMRecompilerWorker::FSUB(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FSUB", &PPUInterpreter::FSUB, frd, fra, frb, rc);
}

void PPULLVMRecompilerWorker::FADD(u32 frd, u32 fra, u32 frb, bool rc) {
    InterpreterCall("FADD", &PPUInterpreter::FADD, frd, fra, frb, rc);
}

void PPULLVMRecompilerWorker::FSQRT(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FSQRT", &PPUInterpreter::FSQRT, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FSEL", &PPUInterpreter::FSEL, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::FMUL(u32 frd, u32 fra, u32 frc, bool rc) {
    InterpreterCall("FMUL", &PPUInterpreter::FMUL, frd, fra, frc, rc);
}

void PPULLVMRecompilerWorker::FRSQRTE(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FRSQRTE", &PPUInterpreter::FRSQRTE, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FMSUB", &PPUInterpreter::FMSUB, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FMADD", &PPUInterpreter::FMADD, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FNMSUB", &PPUInterpreter::FNMSUB, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    InterpreterCall("FNMADD", &PPUInterpreter::FNMADD, frd, fra, frc, frb, rc);
}

void PPULLVMRecompilerWorker::FCMPO(u32 crfd, u32 fra, u32 frb) {
    InterpreterCall("FCMPO", &PPUInterpreter::FCMPO, crfd, fra, frb);
}

void PPULLVMRecompilerWorker::FNEG(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FNEG", &PPUInterpreter::FNEG, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FMR(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FMR", &PPUInterpreter::FMR, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FNABS(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FNABS", &PPUInterpreter::FNABS, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FABS(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FABS", &PPUInterpreter::FABS, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FCTID(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTID", &PPUInterpreter::FCTID, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FCTIDZ(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCTIDZ", &PPUInterpreter::FCTIDZ, frd, frb, rc);
}

void PPULLVMRecompilerWorker::FCFID(u32 frd, u32 frb, bool rc) {
    InterpreterCall("FCFID", &PPUInterpreter::FCFID, frd, frb, rc);
}

void PPULLVMRecompilerWorker::UNK(const u32 code, const u32 opcode, const u32 gcode) {
    //InterpreterCall("UNK", &PPUInterpreter::UNK, code, opcode, gcode);
}

Value * PPULLVMRecompilerWorker::GetPPUState() {
    return m_function->arg_begin();
}

Value * PPULLVMRecompilerWorker::GetBaseAddress() {
    auto i = m_function->arg_begin();
    i++;
    return i;
}

Value * PPULLVMRecompilerWorker::GetInterpreter() {
    auto i = m_function->arg_begin();
    i++;
    i++;
    return i;
}

Value * PPULLVMRecompilerWorker::GetBit(Value * val, u32 n) {
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

Value * PPULLVMRecompilerWorker::ClrBit(Value * val, u32 n) {
    return m_ir_builder->CreateAnd(val, ~((u64)1 << (val->getType()->getIntegerBitWidth() - n - 1)));
}

Value * PPULLVMRecompilerWorker::SetBit(Value * val, u32 n, Value * bit, bool doClear) {
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

Value * PPULLVMRecompilerWorker::GetNibble(Value * val, u32 n) {
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

Value * PPULLVMRecompilerWorker::ClrNibble(Value * val, u32 n) {
    return m_ir_builder->CreateAnd(val, ~((u64)0xF << ((((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4)));
}

Value * PPULLVMRecompilerWorker::SetNibble(Value * val, u32 n, Value * nibble, bool doClear) {
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

Value * PPULLVMRecompilerWorker::SetNibble(Value * val, u32 n, Value * b0, Value * b1, Value * b2, Value * b3, bool doClear) {
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

Value * PPULLVMRecompilerWorker::GetPc() {
    auto pc_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, PC));
    auto pc_i64_ptr = m_ir_builder->CreateBitCast(pc_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateLoad(pc_i64_ptr);
}

void PPULLVMRecompilerWorker::SetPc(Value * val_i64) {
    auto pc_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, PC));
    auto pc_i64_ptr = m_ir_builder->CreateBitCast(pc_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateStore(val_i64, pc_i64_ptr);
}

Value * PPULLVMRecompilerWorker::GetGpr(u32 r, u32 num_bits) {
    auto r_i8_ptr = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, GPR[r]));
    auto r_ix_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getIntNTy(num_bits)->getPointerTo());
    return m_ir_builder->CreateLoad(r_ix_ptr);
}

void PPULLVMRecompilerWorker::SetGpr(u32 r, Value * val_x64) {
    auto r_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, GPR[r]));
    auto r_i64_ptr = m_ir_builder->CreateBitCast(r_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    auto val_i64   = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    m_ir_builder->CreateStore(val_i64, r_i64_ptr);
}

Value * PPULLVMRecompilerWorker::GetCr() {
    auto cr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, CR));
    auto cr_i32_ptr = m_ir_builder->CreateBitCast(cr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateLoad(cr_i32_ptr);
}

Value * PPULLVMRecompilerWorker::GetCrField(u32 n) {
    return GetNibble(GetCr(), n);
}

void PPULLVMRecompilerWorker::SetCr(Value * val_x32) {
    auto val_i32    = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto cr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, CR));
    auto cr_i32_ptr = m_ir_builder->CreateBitCast(cr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateStore(val_i32, cr_i32_ptr);
}

void PPULLVMRecompilerWorker::SetCrField(u32 n, Value * field) {
    SetCr(SetNibble(GetCr(), n, field));
}

void PPULLVMRecompilerWorker::SetCrField(u32 n, Value * b0, Value * b1, Value * b2, Value * b3) {
    SetCr(SetNibble(GetCr(), n, b0, b1, b2, b3));
}

void PPULLVMRecompilerWorker::SetCrFieldSignedCmp(u32 n, Value * a, Value * b) {
    auto lt_i1  = m_ir_builder->CreateICmpSLT(a, b);
    auto gt_i1  = m_ir_builder->CreateICmpSGT(a, b);
    auto eq_i1  = m_ir_builder->CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void PPULLVMRecompilerWorker::SetCrFieldUnsignedCmp(u32 n, Value * a, Value * b) {
    auto lt_i1  = m_ir_builder->CreateICmpULT(a, b);
    auto gt_i1  = m_ir_builder->CreateICmpUGT(a, b);
    auto eq_i1  = m_ir_builder->CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void PPULLVMRecompilerWorker::SetCr6AfterVectorCompare(u32 vr) {
    auto vr_v16i8    = GetVrAsIntVec(vr, 8);
    auto vr_mask_i32 = m_ir_builder->CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmovmskb_128), vr_v16i8);
    auto cmp0_i1     = m_ir_builder->CreateICmpEQ(vr_mask_i32, m_ir_builder->getInt32(0));
    auto cmp1_i1     = m_ir_builder->CreateICmpEQ(vr_mask_i32, m_ir_builder->getInt32(0xFFFF));
    auto cr_i32      = GetCr();
    cr_i32           = SetNibble(cr_i32, 6, cmp1_i1, nullptr, cmp0_i1, nullptr);
    SetCr(cr_i32);
}

Value * PPULLVMRecompilerWorker::GetXer() {
    auto xer_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, XER));
    auto xer_i64_ptr = m_ir_builder->CreateBitCast(xer_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    return m_ir_builder->CreateLoad(xer_i64_ptr);
}

Value * PPULLVMRecompilerWorker::GetXerCa() {
    return GetBit(GetXer(), 34);
}

Value * PPULLVMRecompilerWorker::GetXerSo() {
    return GetBit(GetXer(), 32);
}

void PPULLVMRecompilerWorker::SetXer(Value * val_x64) {
    auto val_i64     = m_ir_builder->CreateBitCast(val_x64, m_ir_builder->getInt64Ty());
    auto xer_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, XER));
    auto xer_i64_ptr = m_ir_builder->CreateBitCast(xer_i8_ptr, m_ir_builder->getInt64Ty()->getPointerTo());
    m_ir_builder->CreateStore(val_i64, xer_i64_ptr);
}

void PPULLVMRecompilerWorker::SetXerCa(Value * ca) {
    auto xer_i64 = GetXer();
    xer_i64      = SetBit(xer_i64, 34, ca);
    SetXer(xer_i64);
}

void PPULLVMRecompilerWorker::SetXerSo(Value * so) {
    auto xer_i64 = GetXer();
    xer_i64      = SetBit(xer_i64, 32, so);
    SetXer(xer_i64);
}

Value * PPULLVMRecompilerWorker::GetVscr() {
    auto vscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = m_ir_builder->CreateBitCast(vscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    return m_ir_builder->CreateLoad(vscr_i32_ptr);
}

void PPULLVMRecompilerWorker::SetVscr(Value * val_x32) {
    auto val_i32      = m_ir_builder->CreateBitCast(val_x32, m_ir_builder->getInt32Ty());
    auto vscr_i8_ptr  = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = m_ir_builder->CreateBitCast(vscr_i8_ptr, m_ir_builder->getInt32Ty()->getPointerTo());
    m_ir_builder->CreateStore(val_i32, vscr_i32_ptr);
}

Value * PPULLVMRecompilerWorker::GetVrAsIntVec(u32 vr, u32 vec_elt_num_bits) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_vec_ptr  = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getIntNTy(vec_elt_num_bits), 128 / vec_elt_num_bits)->getPointerTo());
    return m_ir_builder->CreateLoad(vr_vec_ptr);
}

Value * PPULLVMRecompilerWorker::GetVrAsFloatVec(u32 vr) {
    auto vr_i8_ptr    = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v4f32_ptr = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getFloatTy(), 4)->getPointerTo());
    return m_ir_builder->CreateLoad(vr_v4f32_ptr);
}

Value * PPULLVMRecompilerWorker::GetVrAsDoubleVec(u32 vr) {
    auto vr_i8_ptr    = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v2f64_ptr = m_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(m_ir_builder->getDoubleTy(), 2)->getPointerTo());
    return m_ir_builder->CreateLoad(vr_v2f64_ptr);
}

void PPULLVMRecompilerWorker::SetVr(u32 vr, Value * val_x128) {
    auto vr_i8_ptr   = m_ir_builder->CreateConstGEP1_32(GetPPUState(), offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = m_ir_builder->CreateBitCast(vr_i8_ptr, m_ir_builder->getIntNTy(128)->getPointerTo());
    auto val_i128    = m_ir_builder->CreateBitCast(val_x128, m_ir_builder->getIntNTy(128));
    m_ir_builder->CreateStore(val_i128, vr_i128_ptr);
}

Value * PPULLVMRecompilerWorker::ReadMemory(Value * addr_i64, u32 bits, bool bswap) {
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

void PPULLVMRecompilerWorker::WriteMemory(Value * addr_i64, Value * val_ix, bool bswap) {
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
Value * PPULLVMRecompilerWorker::InterpreterCall(const char * name, Func function, Args... args) {
    auto i = m_interpreter_fallback_stats.find(name);
    if (i == m_interpreter_fallback_stats.end()) {
        i = m_interpreter_fallback_stats.insert(m_interpreter_fallback_stats.end(), std::make_pair<std::string, u64>(name, 0));
    }

    i->second++;

    return Call(name, function, GetInterpreter(), m_ir_builder->getInt32(args)...);
}

template<class T>
Type * PPULLVMRecompilerWorker::CppToLlvmType() {
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
Value * PPULLVMRecompilerWorker::Call(const char * name, Func function, Args... args) {
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

PPULLVMRecompiler::PPULLVMRecompiler(PPUThread & ppu)
    : m_ppu(ppu)
    , m_interpreter(ppu) {
    //m_worker.RunAllTests(&m_ppu, (u64)Memory.GetBaseAddr(), &m_interpreter);
}

PPULLVMRecompiler::~PPULLVMRecompiler() {

}

u8 PPULLVMRecompiler::DecodeMemory(const u64 address) {
    m_worker.Compile(address);
    auto fn = m_worker.GetCompiledBlock(address);
    fn(&m_ppu, (u64)Memory.GetBaseAddr(), &m_interpreter);
    return 0;
}
