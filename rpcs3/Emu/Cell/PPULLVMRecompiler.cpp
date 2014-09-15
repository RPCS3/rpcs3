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

using namespace llvm;

PPULLVMRecompiler::PPULLVMRecompiler(PPUThread & ppu)
    : m_ppu(ppu)
    , m_decoder(this)
    , m_ir_builder(m_llvm_context)
    , m_interpreter(ppu) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetDisassembler();

    m_module = new llvm::Module("Module", m_llvm_context);
    m_pc     = new GlobalVariable(*m_module, Type::getInt64Ty(m_llvm_context), false, GlobalValue::ExternalLinkage, nullptr, "pc");
    m_gpr    = new GlobalVariable(*m_module, ArrayType::get(Type::getInt64Ty(m_llvm_context), 32), false, GlobalValue::ExternalLinkage, nullptr, "gpr");
    m_cr     = new GlobalVariable(*m_module, Type::getInt32Ty(m_llvm_context), false, GlobalValue::ExternalLinkage, nullptr, "cr");
    m_xer    = new GlobalVariable(*m_module, Type::getInt64Ty(m_llvm_context), false, GlobalValue::ExternalLinkage, nullptr, "xer");
    m_vpr    = new GlobalVariable(*m_module, ArrayType::get(Type::getIntNTy(m_llvm_context, 128), 32), false, GlobalValue::ExternalLinkage, nullptr, "vpr");
    m_vscr   = new GlobalVariable(*m_module, Type::getInt32Ty(m_llvm_context), false, GlobalValue::ExternalLinkage, nullptr, "vscr");

    m_execution_engine = EngineBuilder(m_module).create();
    m_execution_engine->addGlobalMapping(m_pc, &m_ppu.PC);
    m_execution_engine->addGlobalMapping(m_gpr, m_ppu.GPR);
    m_execution_engine->addGlobalMapping(m_cr, &m_ppu.CR);
    m_execution_engine->addGlobalMapping(m_xer, &m_ppu.XER);
    m_execution_engine->addGlobalMapping(m_vpr, m_ppu.VPR);
    m_execution_engine->addGlobalMapping(m_vscr, &m_ppu.VSCR);

    m_disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);

    // RunAllTests();
}

PPULLVMRecompiler::~PPULLVMRecompiler() {
    LLVMDisasmDispose(m_disassembler);
    delete m_execution_engine;
    llvm_shutdown();
}

static std::string module;
static std::string registers;
static u64 lastAddress;
static std::chrono::duration<double> compilation_time;
static std::chrono::duration<double> execution_time;

u8 PPULLVMRecompiler::DecodeMemory(const u64 address) {
    auto function_name = fmt::Format("fn_0x%llx", address);
    auto function      = m_module->getFunction(function_name);

    if (!function) {
        std::chrono::high_resolution_clock::time_point compilation_start = std::chrono::high_resolution_clock::now();

        u64 offset = 0;
        function   = cast<Function>(m_module->getOrInsertFunction(function_name, Type::getVoidTy(m_llvm_context), (Type *)nullptr));
        auto block = BasicBlock::Create(m_llvm_context, "start", function);
        m_ir_builder.SetInsertPoint(block);

        m_hit_branch_instruction = false;
        while (!m_hit_branch_instruction) {
            u32 instr = Memory.Read32(address + offset);
            m_decoder.Decode(instr);
            offset += 4;

            m_ir_builder.CreateStore(m_ir_builder.getInt64(m_ppu.PC + offset), m_pc);
        }

        m_ir_builder.CreateRetVoid();

        m_execution_engine->runJITOnFunction(function);
        //module = "";
        //raw_string_ostream stream(module);
        //stream << *m_module;

        std::chrono::high_resolution_clock::time_point compilation_end = std::chrono::high_resolution_clock::now();
        compilation_time += std::chrono::duration_cast<std::chrono::duration<double>>(compilation_end - compilation_start);
    }

    //lastAddress = address;
    //registers   = m_ppu.RegsToString();
    std::chrono::high_resolution_clock::time_point execution_start = std::chrono::high_resolution_clock::now();
    std::vector<GenericValue> args;
    m_execution_engine->runFunction(function, args);
    std::chrono::high_resolution_clock::time_point execution_end = std::chrono::high_resolution_clock::now();
    execution_time += std::chrono::duration_cast<std::chrono::duration<double>>(execution_end - execution_start);
    return 0;
}

void PPULLVMRecompiler::NULL_OP() {
    ThisCall0("NULL_OP", &PPUInterpreter::NULL_OP, &m_interpreter);
}

void PPULLVMRecompiler::NOP() {

}

void PPULLVMRecompiler::TDI(u32 to, u32 ra, s32 simm16) {
    ThisCall3("TDI", &PPUInterpreter::TDI, &m_interpreter, to, ra, simm16);
}

void PPULLVMRecompiler::TWI(u32 to, u32 ra, s32 simm16) {
    ThisCall3("TWI", &PPUInterpreter::TWI, &m_interpreter, to, ra, simm16);
}

void PPULLVMRecompiler::MFVSCR(u32 vd) {
    auto vscr_i32  = m_ir_builder.CreateLoad(m_vscr);
    auto vscr_i128 = m_ir_builder.CreateZExt(vscr_i32, Type::getIntNTy(m_llvm_context, 128));
    SetVr(vd, vscr_i128);
}

void PPULLVMRecompiler::MTVSCR(u32 vb) {
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);
    auto vscr_i32 = m_ir_builder.CreateExtractElement(vb_v4i32, m_ir_builder.getInt32(0));
    m_ir_builder.CreateStore(vscr_i32, m_vscr);
}

void PPULLVMRecompiler::VADDCUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    va_v4i32               = m_ir_builder.CreateXor(va_v4i32, ConstantDataVector::get(m_llvm_context, not_mask_v4i32));
    auto cmpv4i1           = m_ir_builder.CreateICmpULT(va_v4i32, vb_v4i32);
    auto cmpv4i32          = m_ir_builder.CreateZExt(cmpv4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, cmpv4i32);

    // TODO: Implement with overflow intrinsics and check if the generated code is better
}

void PPULLVMRecompiler::VADDFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto sum_v4f32 = m_ir_builder.CreateFAdd(va_v4f32, vb_v4f32);
    SetVr(vd, sum_v4f32);
}

void PPULLVMRecompiler::VADDSBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_padds_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set VSCR.SAT
}

void PPULLVMRecompiler::VADDSHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_padds_w), va_v8i16, vb_v8i16);
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
    auto tmp2_v4i32    = m_ir_builder.CreateLShr(va_v4i32, 31);
    tmp2_v4i32         = m_ir_builder.CreateAdd(tmp2_v4i32, ConstantDataVector::get(m_llvm_context, tmp1_v4i32));
    auto tmp2_v16i8    = m_ir_builder.CreateBitCast(tmp2_v4i32, VectorType::get(Type::getInt8Ty(m_llvm_context), 16));

    // Next, we find if the addition can actually result in an overflow. Since an overflow can only happen if the operands
    // have the same sign, we bitwise XOR both the operands. If the sign bit of the result is 0 then the operands have the
    // same sign and so may cause an overflow. We invert the result so that the sign bit is 1 when the operands have the
    // same sign.
    auto tmp3_v4i32        = m_ir_builder.CreateXor(va_v4i32, vb_v4i32);
    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    tmp3_v4i32             = m_ir_builder.CreateXor(tmp3_v4i32, ConstantDataVector::get(m_llvm_context, not_mask_v4i32));

    // Perform the sum.
    auto sum_v4i32 = m_ir_builder.CreateAdd(va_v4i32, vb_v4i32);
    auto sum_v16i8 = m_ir_builder.CreateBitCast(sum_v4i32, VectorType::get(Type::getInt8Ty(m_llvm_context), 16));

    // If an overflow occurs, then the sign of the sum will be different from the sign of the operands. So, we xor the
    // result with one of the operands. The sign bit of the result will be 1 if the sign bit of the sum and the sign bit of the
    // result is different. This result is again ANDed with tmp3 (the sign bit of tmp3 is 1 only if the operands have the same
    // sign and so can cause an overflow).
    auto tmp4_v4i32 = m_ir_builder.CreateXor(va_v4i32, sum_v4i32);
    tmp4_v4i32      = m_ir_builder.CreateAnd(tmp3_v4i32, tmp4_v4i32);
    tmp4_v4i32      = m_ir_builder.CreateAShr(tmp4_v4i32, 31);
    auto tmp4_v16i8 = m_ir_builder.CreateBitCast(tmp4_v4i32, VectorType::get(Type::getInt8Ty(m_llvm_context), 16));

    // tmp4 is equal to 0xFFFFFFFF if an overflow occured and 0x00000000 otherwise.
    auto res_v16i8 = m_ir_builder.CreateCall3(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse41_pblendvb), sum_v16i8, tmp2_v16i8, tmp4_v16i8);
    SetVr(vd, res_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUBM(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder.CreateAdd(va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);
}

void PPULLVMRecompiler::VADDUBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_paddus_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUHM(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder.CreateAdd(va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);
}

void PPULLVMRecompiler::VADDUHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_paddus_w), va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUWM(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = m_ir_builder.CreateAdd(va_v4i32, vb_v4i32);
    SetVr(vd, sum_v4i32);
}

void PPULLVMRecompiler::VADDUWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = m_ir_builder.CreateAdd(va_v4i32, vb_v4i32);
    auto cmp_v4i1  = m_ir_builder.CreateICmpULT(sum_v4i32, va_v4i32);
    auto cmp_v4i32 = m_ir_builder.CreateSExt(cmp_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    auto res_v4i32 = m_ir_builder.CreateOr(sum_v4i32, cmp_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VAND(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = m_ir_builder.CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VANDC(u32 vd, u32 va, u32 vb) {
    auto va_v4i32          = GetVrAsIntVec(va, 32);
    auto vb_v4i32          = GetVrAsIntVec(vb, 32);
    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    vb_v4i32               = m_ir_builder.CreateXor(vb_v4i32, ConstantDataVector::get(m_llvm_context, not_mask_v4i32));
    auto res_v4i32         = m_ir_builder.CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VAVGSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8       = GetVrAsIntVec(va, 8);
    auto vb_v16i8       = GetVrAsIntVec(vb, 8);
    auto va_v16i16      = m_ir_builder.CreateSExt(va_v16i8, VectorType::get(Type::getInt16Ty(m_llvm_context), 16));
    auto vb_v16i16      = m_ir_builder.CreateSExt(vb_v16i8, VectorType::get(Type::getInt16Ty(m_llvm_context), 16));
    auto sum_v16i16     = m_ir_builder.CreateAdd(va_v16i16, vb_v16i16);
    u16  one_v16i16[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    sum_v16i16          = m_ir_builder.CreateAdd(sum_v16i16, ConstantDataVector::get(m_llvm_context, one_v16i16));
    auto avg_v16i16     = m_ir_builder.CreateAShr(sum_v16i16, 1);
    auto avg_v16i8      = m_ir_builder.CreateTrunc(avg_v16i16, VectorType::get(Type::getInt8Ty(m_llvm_context), 16));
    SetVr(vd, avg_v16i8);
}

void PPULLVMRecompiler::VAVGSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16     = GetVrAsIntVec(va, 16);
    auto vb_v8i16     = GetVrAsIntVec(vb, 16);
    auto va_v8i32     = m_ir_builder.CreateSExt(va_v8i16, VectorType::get(Type::getInt32Ty(m_llvm_context), 8));
    auto vb_v8i32     = m_ir_builder.CreateSExt(vb_v8i16, VectorType::get(Type::getInt32Ty(m_llvm_context), 8));
    auto sum_v8i32    = m_ir_builder.CreateAdd(va_v8i32, vb_v8i32);
    u32  one_v8i32[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    sum_v8i32         = m_ir_builder.CreateAdd(sum_v8i32, ConstantDataVector::get(m_llvm_context, one_v8i32));
    auto avg_v8i32    = m_ir_builder.CreateAShr(sum_v8i32, 1);
    auto avg_v8i16    = m_ir_builder.CreateTrunc(avg_v8i32, VectorType::get(Type::getInt16Ty(m_llvm_context), 8));
    SetVr(vd, avg_v8i16);
}

void PPULLVMRecompiler::VAVGSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32     = GetVrAsIntVec(va, 32);
    auto vb_v4i32     = GetVrAsIntVec(vb, 32);
    auto va_v4i64     = m_ir_builder.CreateSExt(va_v4i32, VectorType::get(Type::getInt64Ty(m_llvm_context), 4));
    auto vb_v4i64     = m_ir_builder.CreateSExt(vb_v4i32, VectorType::get(Type::getInt64Ty(m_llvm_context), 4));
    auto sum_v4i64    = m_ir_builder.CreateAdd(va_v4i64, vb_v4i64);
    u64  one_v4i64[4] = {1, 1, 1, 1};
    sum_v4i64         = m_ir_builder.CreateAdd(sum_v4i64, ConstantDataVector::get(m_llvm_context, one_v4i64));
    auto avg_v4i64    = m_ir_builder.CreateAShr(sum_v4i64, 1);
    auto avg_v4i32    = m_ir_builder.CreateTrunc(avg_v4i64, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, avg_v4i32);
}

void PPULLVMRecompiler::VAVGUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto avg_v16i8 = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pavg_b), va_v16i8, vb_v16i8);
    SetVr(vd, avg_v16i8);
}

void PPULLVMRecompiler::VAVGUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto avg_v8i16 = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pavg_w), va_v8i16, vb_v8i16);
    SetVr(vd, avg_v8i16);
}

void PPULLVMRecompiler::VAVGUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32     = GetVrAsIntVec(va, 32);
    auto vb_v4i32     = GetVrAsIntVec(vb, 32);
    auto va_v4i64     = m_ir_builder.CreateZExt(va_v4i32, VectorType::get(Type::getInt64Ty(m_llvm_context), 4));
    auto vb_v4i64     = m_ir_builder.CreateZExt(vb_v4i32, VectorType::get(Type::getInt64Ty(m_llvm_context), 4));
    auto sum_v4i64    = m_ir_builder.CreateAdd(va_v4i64, vb_v4i64);
    u64  one_v4i64[4] = {1, 1, 1, 1};
    sum_v4i64         = m_ir_builder.CreateAdd(sum_v4i64, ConstantDataVector::get(m_llvm_context, one_v4i64));
    auto avg_v4i64    = m_ir_builder.CreateLShr(sum_v4i64, 1);
    auto avg_v4i32    = m_ir_builder.CreateTrunc(avg_v4i64, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, avg_v4i32);
}

void PPULLVMRecompiler::VCFSX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder.CreateSIToFP(vb_v4i32, VectorType::get(Type::getFloatTy(m_llvm_context), 4));

    if (uimm5) {
        float scale          = (float)(1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = m_ir_builder.CreateFDiv(res_v4f32, ConstantDataVector::get(m_llvm_context, scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VCFUX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = m_ir_builder.CreateUIToFP(vb_v4i32, VectorType::get(Type::getFloatTy(m_llvm_context), 4));

    if (uimm5) {
        float scale          = (float)(1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = m_ir_builder.CreateFDiv(res_v4f32, ConstantDataVector::get(m_llvm_context, scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VCMPBFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32     = GetVrAsFloatVec(va);
    auto vb_v4f32     = GetVrAsFloatVec(vb);
    auto cmp_gt_v4i1  = m_ir_builder.CreateFCmpOGT(va_v4f32, vb_v4f32);
    vb_v4f32          = m_ir_builder.CreateFNeg(vb_v4f32);
    auto cmp_lt_v4i1  = m_ir_builder.CreateFCmpOLT(va_v4f32, vb_v4f32);
    auto cmp_gt_v4i32 = m_ir_builder.CreateZExt(cmp_gt_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    auto cmp_lt_v4i32 = m_ir_builder.CreateZExt(cmp_lt_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    cmp_gt_v4i32      = m_ir_builder.CreateShl(cmp_gt_v4i32, 31);
    cmp_lt_v4i32      = m_ir_builder.CreateShl(cmp_lt_v4i32, 30);
    auto res_v4i32    = m_ir_builder.CreateOr(cmp_gt_v4i32, cmp_lt_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Implement NJ mode
}

void PPULLVMRecompiler::VCMPBFP_(u32 vd, u32 va, u32 vb) {
    VCMPBFP(vd, va, vb);

    auto vd_v16i8     = GetVrAsIntVec(vd, 8);
    u8 mask_v16i8[16] = {3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vd_v16i8          = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_ssse3_pshuf_b_128), vd_v16i8, ConstantDataVector::get(m_llvm_context, mask_v16i8));
    auto vd_v4i32     = m_ir_builder.CreateBitCast(vd_v16i8, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    auto vd_mask_i32  = m_ir_builder.CreateExtractElement(vd_v4i32, m_ir_builder.getInt32(0));
    auto cmp_i1       = m_ir_builder.CreateICmpEQ(vd_mask_i32, m_ir_builder.getInt32(0));
    SetCrField(6, nullptr, nullptr, cmp_i1, nullptr);
}

void PPULLVMRecompiler::VCMPEQFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb); 
    auto cmp_v4i1  = m_ir_builder.CreateFCmpOEQ(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder.CreateSExt(cmp_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPEQFP_(u32 vd, u32 va, u32 vb) {
    VCMPEQFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder.CreateICmpEQ(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder.CreateSExt(cmp_v16i1, VectorType::get(Type::getInt8Ty(m_llvm_context), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPEQUB_(u32 vd, u32 va, u32 vb) {
    VCMPEQUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder.CreateICmpEQ(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder.CreateSExt(cmp_v8i1, VectorType::get(Type::getInt16Ty(m_llvm_context), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPEQUH_(u32 vd, u32 va, u32 vb) {
    VCMPEQUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder.CreateICmpEQ(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder.CreateSExt(cmp_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPEQUW_(u32 vd, u32 va, u32 vb) {
    VCMPEQUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGEFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder.CreateFCmpOGE(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder.CreateSExt(cmp_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGEFP_(u32 vd, u32 va, u32 vb) {
    VCMPGEFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = m_ir_builder.CreateFCmpOGT(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = m_ir_builder.CreateSExt(cmp_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTFP_(u32 vd, u32 va, u32 vb) {
    VCMPGTFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder.CreateICmpSGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder.CreateSExt(cmp_v16i1, VectorType::get(Type::getInt8Ty(m_llvm_context), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPGTSB_(u32 vd, u32 va, u32 vb) {
    VCMPGTSB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder.CreateICmpSGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder.CreateSExt(cmp_v8i1, VectorType::get(Type::getInt16Ty(m_llvm_context), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPGTSH_(u32 vd, u32 va, u32 vb) {
    VCMPGTSH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder.CreateICmpSGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder.CreateSExt(cmp_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTSW_(u32 vd, u32 va, u32 vb) {
    VCMPGTSW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = m_ir_builder.CreateICmpUGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = m_ir_builder.CreateSExt(cmp_v16i1, VectorType::get(Type::getInt8Ty(m_llvm_context), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPGTUB_(u32 vd, u32 va, u32 vb) {
    VCMPGTUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = m_ir_builder.CreateICmpUGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = m_ir_builder.CreateSExt(cmp_v8i1, VectorType::get(Type::getInt16Ty(m_llvm_context), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPGTUH_(u32 vd, u32 va, u32 vb) {
    VCMPGTUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = m_ir_builder.CreateICmpUGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = m_ir_builder.CreateSExt(cmp_v4i1, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTUW_(u32 vd, u32 va, u32 vb) {
    VCMPGTUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCTSXS(u32 vd, u32 uimm5, u32 vb) {
    ThisCall3("VCTSXS", &PPUInterpreter::VCTSXS, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VCTUXS(u32 vd, u32 uimm5, u32 vb) {
    ThisCall3("VCTUXS", &PPUInterpreter::VCTUXS, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VEXPTEFP(u32 vd, u32 vb) {
    ThisCall2("VEXPTEFP", &PPUInterpreter::VEXPTEFP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VLOGEFP(u32 vd, u32 vb) {
    ThisCall2("VLOGEFP", &PPUInterpreter::VLOGEFP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) {
    ThisCall4("VMADDFP", &PPUInterpreter::VMADDFP, &m_interpreter, vd, va, vc, vb);
}

void PPULLVMRecompiler::VMAXFP(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMAXFP", &PPUInterpreter::VMAXFP, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMAXSB", &PPUInterpreter::VMAXSB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMAXSH", &PPUInterpreter::VMAXSH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMAXSW", &PPUInterpreter::VMAXSW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMAXUB", &PPUInterpreter::VMAXUB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMAXUH", &PPUInterpreter::VMAXUH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMAXUW", &PPUInterpreter::VMAXUW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMHADDSHS", &PPUInterpreter::VMHADDSHS, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMHRADDSHS", &PPUInterpreter::VMHRADDSHS, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMINFP(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMINFP", &PPUInterpreter::VMINFP, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINSB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMINSB", &PPUInterpreter::VMINSB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINSH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMINSH", &PPUInterpreter::VMINSH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINSW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMINSW", &PPUInterpreter::VMINSW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINUB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMINUB", &PPUInterpreter::VMINUB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINUH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMINUH", &PPUInterpreter::VMINUH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINUW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMINUW", &PPUInterpreter::VMINUW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMLADDUHM", &PPUInterpreter::VMLADDUHM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMRGHB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMRGHB", &PPUInterpreter::VMRGHB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGHH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMRGHH", &PPUInterpreter::VMRGHH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGHW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMRGHW", &PPUInterpreter::VMRGHW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMRGLB", &PPUInterpreter::VMRGLB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMRGLH", &PPUInterpreter::VMRGLH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMRGLW", &PPUInterpreter::VMRGLW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMSUMMBM", &PPUInterpreter::VMSUMMBM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMSUMSHM", &PPUInterpreter::VMSUMSHM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMSUMSHS", &PPUInterpreter::VMSUMSHS, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMSUMUBM", &PPUInterpreter::VMSUMUBM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMSUMUHM", &PPUInterpreter::VMSUMUHM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VMSUMUHS", &PPUInterpreter::VMSUMUHS, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMULESB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMULESB", &PPUInterpreter::VMULESB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULESH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMULESH", &PPUInterpreter::VMULESH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULEUB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMULEUB", &PPUInterpreter::VMULEUB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULEUH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMULEUH", &PPUInterpreter::VMULEUH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULOSB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMULOSB", &PPUInterpreter::VMULOSB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULOSH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMULOSH", &PPUInterpreter::VMULOSH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULOUB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMULOUB", &PPUInterpreter::VMULOUB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULOUH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VMULOUH", &PPUInterpreter::VMULOUH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) {
    ThisCall4("VNMSUBFP", &PPUInterpreter::VNMSUBFP, &m_interpreter, vd, va, vc, vb);
}

void PPULLVMRecompiler::VNOR(u32 vd, u32 va, u32 vb) {
    ThisCall3("VNOR", &PPUInterpreter::VNOR, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VOR(u32 vd, u32 va, u32 vb) {
    ThisCall3("VOR", &PPUInterpreter::VOR, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPERM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VPERM", &PPUInterpreter::VPERM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VPKPX(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKPX", &PPUInterpreter::VPKPX, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKSHSS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKSHSS", &PPUInterpreter::VPKSHSS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKSHUS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKSHUS", &PPUInterpreter::VPKSHUS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKSWSS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKSWSS", &PPUInterpreter::VPKSWSS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKSWUS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKSWUS", &PPUInterpreter::VPKSWUS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKUHUM(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKUHUM", &PPUInterpreter::VPKUHUM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKUHUS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKUHUS", &PPUInterpreter::VPKUHUS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKUWUM(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKUWUM", &PPUInterpreter::VPKUWUM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKUWUS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VPKUWUS", &PPUInterpreter::VPKUWUS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VREFP(u32 vd, u32 vb) {
    ThisCall2("VREFP", &PPUInterpreter::VREFP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRFIM(u32 vd, u32 vb) {
    ThisCall2("VRFIM", &PPUInterpreter::VRFIM, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRFIN(u32 vd, u32 vb) {
    ThisCall2("VRFIN", &PPUInterpreter::VRFIN, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRFIP(u32 vd, u32 vb) {
    ThisCall2("VRFIP", &PPUInterpreter::VRFIP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRFIZ(u32 vd, u32 vb) {
    ThisCall2("VRFIZ", &PPUInterpreter::VRFIZ, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRLB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VRLB", &PPUInterpreter::VRLB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VRLH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VRLH", &PPUInterpreter::VRLH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VRLW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VRLW", &PPUInterpreter::VRLW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VRSQRTEFP(u32 vd, u32 vb) {
    ThisCall2("VRSQRTEFP", &PPUInterpreter::VRSQRTEFP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VSEL(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall4("VSEL", &PPUInterpreter::VSEL, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VSL(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSL", &PPUInterpreter::VSL, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSLB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSLB", &PPUInterpreter::VSLB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) {
    ThisCall4("VSLDOI", &PPUInterpreter::VSLDOI, &m_interpreter, vd, va, vb, sh);
}

void PPULLVMRecompiler::VSLH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSLH", &PPUInterpreter::VSLH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSLO(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSLO", &PPUInterpreter::VSLO, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSLW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSLW", &PPUInterpreter::VSLW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSPLTB(u32 vd, u32 uimm5, u32 vb) {
    ThisCall3("VSPLTB", &PPUInterpreter::VSPLTB, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSPLTH(u32 vd, u32 uimm5, u32 vb) {
    ThisCall3("VSPLTH", &PPUInterpreter::VSPLTH, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSPLTISB(u32 vd, s32 simm5) {
    ThisCall2("VSPLTISB", &PPUInterpreter::VSPLTISB, &m_interpreter, vd, simm5);
}

void PPULLVMRecompiler::VSPLTISH(u32 vd, s32 simm5) {
    ThisCall2("VSPLTISH", &PPUInterpreter::VSPLTISH, &m_interpreter, vd, simm5);
}

void PPULLVMRecompiler::VSPLTISW(u32 vd, s32 simm5) {
    ThisCall2("VSPLTISW", &PPUInterpreter::VSPLTISW, &m_interpreter, vd, simm5);
}

void PPULLVMRecompiler::VSPLTW(u32 vd, u32 uimm5, u32 vb) {
    ThisCall3("VSPLTW", &PPUInterpreter::VSPLTW, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSR(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSR", &PPUInterpreter::VSR, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRAB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSRAB", &PPUInterpreter::VSRAB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRAH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSRAH", &PPUInterpreter::VSRAH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRAW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSRAW", &PPUInterpreter::VSRAW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRB(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSRB", &PPUInterpreter::VSRB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRH(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSRH", &PPUInterpreter::VSRH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRO(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSRO", &PPUInterpreter::VSRO, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSRW", &PPUInterpreter::VSRW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBCUW(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBCUW", &PPUInterpreter::VSUBCUW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBFP(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBFP", &PPUInterpreter::VSUBFP, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSBS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBSBS", &PPUInterpreter::VSUBSBS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSHS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBSHS", &PPUInterpreter::VSUBSHS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSWS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBSWS", &PPUInterpreter::VSUBSWS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUBM(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBUBM", &PPUInterpreter::VSUBUBM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUBS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBUBS", &PPUInterpreter::VSUBUBS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUHM(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBUHM", &PPUInterpreter::VSUBUHM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUHS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBUHS", &PPUInterpreter::VSUBUHS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUWM(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBUWM", &PPUInterpreter::VSUBUWM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUWS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUBUWS", &PPUInterpreter::VSUBUWS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUMSWS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUMSWS", &PPUInterpreter::VSUMSWS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUM2SWS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUM2SWS", &PPUInterpreter::VSUM2SWS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4SBS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUM4SBS", &PPUInterpreter::VSUM4SBS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4SHS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUM4SHS", &PPUInterpreter::VSUM4SHS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4UBS(u32 vd, u32 va, u32 vb) {
    ThisCall3("VSUM4UBS", &PPUInterpreter::VSUM4UBS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VUPKHPX(u32 vd, u32 vb) {
    ThisCall2("VUPKHPX", &PPUInterpreter::VUPKHPX, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKHSB(u32 vd, u32 vb) {
    ThisCall2("VUPKHSB", &PPUInterpreter::VUPKHSB, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKHSH(u32 vd, u32 vb) {
    ThisCall2("VUPKHSH", &PPUInterpreter::VUPKHSH, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKLPX(u32 vd, u32 vb) {
    ThisCall2("VUPKLPX", &PPUInterpreter::VUPKLPX, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKLSB(u32 vd, u32 vb) {
    ThisCall2("VUPKLSB", &PPUInterpreter::VUPKLSB, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKLSH(u32 vd, u32 vb) {
    ThisCall2("VUPKLSH", &PPUInterpreter::VUPKLSH, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VXOR(u32 vd, u32 va, u32 vb) {
    ThisCall3("VXOR", &PPUInterpreter::VXOR, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::MULLI(u32 rd, u32 ra, s32 simm16) {
    //auto ra_i64  = GetGpr(ra);
    //auto res_i64 = m_ir_builder.CreateMul(ra_i64, m_ir_builder.getInt64((s64)simm16));
    //SetGpr(rd, res_i64);
    ThisCall3("MULLI", &PPUInterpreter::MULLI, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::SUBFIC(u32 rd, u32 ra, s32 simm16) {
    //auto ra_i64   = GetGpr(ra);
    //auto res_s    = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::ssub_with_overflow), m_ir_builder.getInt64((s64)simm16), ra_i64);
    //auto diff_i64 = m_ir_builder.CreateExtractValue(res_s, {0});
    //auto carry_i1 = m_ir_builder.CreateExtractValue(res_s, {1});
    //SetGpr(rd, diff_i64);
    //SetXerCa(carry_i1);
    ThisCall3("SUBFIC", &PPUInterpreter::SUBFIC, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::CMPLI(u32 crfd, u32 l, u32 ra, u32 uimm16) {
    //auto ra_i64 = GetGpr(ra);
    //if (l == 0) {
    //    ra_i64 = m_ir_builder.CreateAnd(ra_i64, 0xFFFFFFFF);
    //}

    //SetCrFieldUnsignedCmp(crfd, ra_i64, m_ir_builder.getInt64(uimm16));
    ThisCall4("CMPLI", &PPUInterpreter::CMPLI, &m_interpreter, crfd, l, ra, uimm16);
}

void PPULLVMRecompiler::CMPI(u32 crfd, u32 l, u32 ra, s32 simm16) {
    //auto ra_i64 = GetGpr(ra);
    //if (l == 0) {
    //    auto ra_i32 = m_ir_builder.CreateTrunc(ra_i64, Type::getInt32Ty(m_llvm_context));
    //    ra_i64      = m_ir_builder.CreateSExt(ra_i64, Type::getInt64Ty(m_llvm_context));
    //}

    //SetCrFieldSignedCmp(crfd, ra_i64, m_ir_builder.getInt64((s64)simm16));
    ThisCall4("CMPI", &PPUInterpreter::CMPI, &m_interpreter, crfd, l, ra, simm16);
}

void PPULLVMRecompiler::ADDIC(u32 rd, u32 ra, s32 simm16) {
    //auto ra_i64   = GetGpr(ra);
    //auto res_s    = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::sadd_with_overflow), m_ir_builder.getInt64((s64)simm16), ra_i64);
    //auto sum_i64  = m_ir_builder.CreateExtractValue(res_s, {0});
    //auto carry_i1 = m_ir_builder.CreateExtractValue(res_s, {1});
    //SetGpr(rd, sum_i64);
    //SetXerCa(carry_i1);
    ThisCall3("ADDIC", &PPUInterpreter::ADDIC, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDIC_(u32 rd, u32 ra, s32 simm16) {
    //ADDIC(rd, ra, simm16);
    //SetCrFieldSignedCmp(0, GetGpr(rd), m_ir_builder.getInt64(0));
    ThisCall3("ADDIC_", &PPUInterpreter::ADDIC_, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDI(u32 rd, u32 ra, s32 simm16) {
    //if (ra == 0) {
    //    SetGpr(rd, m_ir_builder.getInt64((s64)simm16));
    //} else {
    //    auto ra_i64  = GetGpr(ra);
    //    auto sum_i64 = m_ir_builder.CreateAdd(ra_i64, m_ir_builder.getInt64((s64)simm16));
    //    SetGpr(rd, sum_i64);
    //}
    ThisCall3("ADDI", &PPUInterpreter::ADDI, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDIS(u32 rd, u32 ra, s32 simm16) {
    //if (ra == 0) {
    //    SetGpr(rd, m_ir_builder.getInt64((s64)(simm16 << 16)));
    //} else {
    //    auto ra_i64  = GetGpr(ra);
    //    auto sum_i64 = m_ir_builder.CreateAdd(ra_i64, m_ir_builder.getInt64((s64)(simm16 << 16)));
    //    SetGpr(rd, sum_i64);
    //}
    ThisCall3("ADDIS", &PPUInterpreter::ADDIS, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) {
    ThisCall5("BC", &PPUInterpreter::BC, &m_interpreter, bo, bi, bd, aa, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::SC(u32 sc_code) {
    ThisCall1("SC", &PPUInterpreter::SC, &m_interpreter, sc_code);
}

void PPULLVMRecompiler::B(s32 ll, u32 aa, u32 lk) {
    ThisCall3("B", &PPUInterpreter::B, &m_interpreter, ll, aa, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::MCRF(u32 crfd, u32 crfs) {
    //if (crfd != crfs) {
    //    auto cr_i32  = GetCr();
    //    auto crf_i32 = GetNibble(cr_i32, crfs);
    //    cr_i32       = SetNibble(cr_i32, crfd, crf_i32);
    //    SetCr(cr_i32);
    //}
    ThisCall2("MCRF", &PPUInterpreter::MCRF, &m_interpreter, crfd, crfs);
}

void PPULLVMRecompiler::BCLR(u32 bo, u32 bi, u32 bh, u32 lk) {
    ThisCall4("BCLR", &PPUInterpreter::BCLR, &m_interpreter, bo, bi, bh, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::CRNOR(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = m_ir_builder.CreateOr(ba_i32, bb_i32);
    //res_i32      = m_ir_builder.CreateXor(res_i32, 1);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall3("CRNOR", &PPUInterpreter::CRNOR, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRANDC(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = m_ir_builder.CreateXor(bb_i32, 1);
    //res_i32      = m_ir_builder.CreateAnd(ba_i32, res_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall3("CRANDC", &PPUInterpreter::CRANDC, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::ISYNC() {
    m_ir_builder.CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_mfence));
}

void PPULLVMRecompiler::CRXOR(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = m_ir_builder.CreateXor(ba_i32, bb_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall3("CRXOR", &PPUInterpreter::CRXOR, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRNAND(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = m_ir_builder.CreateAnd(ba_i32, bb_i32);
    //res_i32      = m_ir_builder.CreateXor(res_i32, 1);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall3("CRNAND", &PPUInterpreter::CRNAND, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRAND(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = m_ir_builder.CreateAnd(ba_i32, bb_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall3("CRAND", &PPUInterpreter::CRAND, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CREQV(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = m_ir_builder.CreateXor(ba_i32, bb_i32);
    //res_i32      = m_ir_builder.CreateXor(res_i32, 1);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall3("CREQV", &PPUInterpreter::CREQV, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRORC(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = m_ir_builder.CreateXor(bb_i32, 1);
    //res_i32      = m_ir_builder.CreateOr(ba_i32, res_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall3("CRORC", &PPUInterpreter::CRORC, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CROR(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = m_ir_builder.CreateOr(ba_i32, bb_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall3("CROR", &PPUInterpreter::CROR, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) {
    ThisCall4("BCCTR", &PPUInterpreter::BCCTR, &m_interpreter, bo, bi, bh, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    ThisCall6("RLWIMI", &PPUInterpreter::RLWIMI, &m_interpreter, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompiler::RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    ThisCall6("RLWINM", &PPUInterpreter::RLWINM, &m_interpreter, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompiler::RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc) {
    ThisCall6("RLWNM", &PPUInterpreter::RLWNM, &m_interpreter, ra, rs, rb, mb, me, rc);
}

void PPULLVMRecompiler::ORI(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = m_ir_builder.CreateOr(rs_i64, uimm16);
    //SetGpr(ra, res_i64);
    ThisCall3("ORI", &PPUInterpreter::ORI, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::ORIS(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = m_ir_builder.CreateOr(rs_i64, uimm16 << 16);
    //SetGpr(ra, res_i64);
    ThisCall3("ORIS", &PPUInterpreter::ORIS, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::XORI(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = m_ir_builder.CreateXor(rs_i64, uimm16);
    //SetGpr(ra, res_i64);
    ThisCall3("XORI", &PPUInterpreter::XORI, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::XORIS(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = m_ir_builder.CreateXor(rs_i64, uimm16 << 16);
    //SetGpr(ra, res_i64);
    ThisCall3("XORIS", &PPUInterpreter::XORIS, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::ANDI_(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = m_ir_builder.CreateAnd(rs_i64, uimm16);
    //SetGpr(ra, res_i64);
    //SetCrFieldSignedCmp(0, res_i64, m_ir_builder.getInt64(0));
    ThisCall3("ANDI_", &PPUInterpreter::ANDI_, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::ANDIS_(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = m_ir_builder.CreateAnd(rs_i64, uimm16 << 16);
    //SetGpr(ra, res_i64);
    //SetCrFieldSignedCmp(0, res_i64, m_ir_builder.getInt64(0));
    ThisCall3("ANDIS_", &PPUInterpreter::ANDIS_, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    ThisCall5("RLDICL", &PPUInterpreter::RLDICL, &m_interpreter, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) {
    ThisCall5("RLDICR", &PPUInterpreter::RLDICR, &m_interpreter, ra, rs, sh, me, rc);
}

void PPULLVMRecompiler::RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    ThisCall5("RLDIC", &PPUInterpreter::RLDIC, &m_interpreter, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    ThisCall5("RLDIMI", &PPUInterpreter::RLDIMI, &m_interpreter, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) {
    ThisCall6("RLDC_LR", &PPUInterpreter::RLDC_LR, &m_interpreter, ra, rs, rb, m_eb, is_r, rc);
}

void PPULLVMRecompiler::CMP(u32 crfd, u32 l, u32 ra, u32 rb) {
    //m_ppu.UpdateCRnS(l, crfd, m_ppu.GPR[ra], m_ppu.GPR[rb]);
	ThisCall4("CMP", &PPUInterpreter::CMP, &m_interpreter, crfd, l, ra, rb);
}

void PPULLVMRecompiler::TW(u32 to, u32 ra, u32 rb) {
    ThisCall3("TW", &PPUInterpreter::TW, &m_interpreter, to, ra, rb);
}

void PPULLVMRecompiler::LVSL(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVSL", &PPUInterpreter::LVSL, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LVEBX(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVEBX", &PPUInterpreter::LVEBX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("SUBFC", &PPUInterpreter::SUBFC, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("ADDC", &PPUInterpreter::ADDC, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MULHDU(u32 rd, u32 ra, u32 rb, bool rc) {
    ThisCall4("MULHDU", &PPUInterpreter::MULHDU, &m_interpreter, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MULHWU(u32 rd, u32 ra, u32 rb, bool rc) {
    ThisCall4("MULHWU", &PPUInterpreter::MULHWU, &m_interpreter, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MFOCRF(u32 a, u32 rd, u32 crm) {
    ThisCall3("MFOCRF", &PPUInterpreter::MFOCRF, &m_interpreter, a, rd, crm);
}

void PPULLVMRecompiler::LWARX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LWARX", &PPUInterpreter::LWARX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LDX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LDX", &PPUInterpreter::LDX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LWZX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LWZX", &PPUInterpreter::LWZX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::SLW(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("SLW", &PPUInterpreter::SLW, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::CNTLZW(u32 ra, u32 rs, bool rc) {
    ThisCall3("CNTLZW", &PPUInterpreter::CNTLZW, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::SLD(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("SLD", &PPUInterpreter::SLD, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::AND(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("AND", &PPUInterpreter::AND, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::CMPL(u32 crfd, u32 l, u32 ra, u32 rb) {
    ThisCall4("CMPL", &PPUInterpreter::CMPL, &m_interpreter, crfd, l, ra, rb);
}

void PPULLVMRecompiler::LVSR(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVSR", &PPUInterpreter::LVSR, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LVEHX(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVEHX", &PPUInterpreter::LVEHX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("SUBF", &PPUInterpreter::SUBF, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::LDUX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LDUX", &PPUInterpreter::LDUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::DCBST(u32 ra, u32 rb) {
    ThisCall2("DCBST", &PPUInterpreter::DCBST, &m_interpreter, ra, rb);
}

void PPULLVMRecompiler::LWZUX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LWZUX", &PPUInterpreter::LWZUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::CNTLZD(u32 ra, u32 rs, bool rc) {
    ThisCall3("CNTLZD", &PPUInterpreter::CNTLZD, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::ANDC(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("ANDC", &PPUInterpreter::ANDC, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::TD(u32 to, u32 ra, u32 rb) {
    ThisCall3("TD", &PPUInterpreter::TD, &m_interpreter, to, ra, rb);
}

void PPULLVMRecompiler::LVEWX(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVEWX", &PPUInterpreter::LVEWX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::MULHD(u32 rd, u32 ra, u32 rb, bool rc) {
    ThisCall4("MULHD", &PPUInterpreter::MULHD, &m_interpreter, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MULHW(u32 rd, u32 ra, u32 rb, bool rc) {
    ThisCall4("MULHW", &PPUInterpreter::MULHW, &m_interpreter, rd, ra, rb, rc);
}

void PPULLVMRecompiler::LDARX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LDARX", &PPUInterpreter::LDARX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::DCBF(u32 ra, u32 rb) {
    ThisCall2("DCBF", &PPUInterpreter::DCBF, &m_interpreter, ra, rb);
}

void PPULLVMRecompiler::LBZX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LBZX", &PPUInterpreter::LBZX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LVX(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVX", &PPUInterpreter::LVX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::NEG(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall4("NEG", &PPUInterpreter::NEG, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::LBZUX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LBZUX", &PPUInterpreter::LBZUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::NOR(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("NOR", &PPUInterpreter::NOR, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::STVEBX(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVEBX", &PPUInterpreter::STVEBX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("SUBFE", &PPUInterpreter::SUBFE, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("ADDE", &PPUInterpreter::ADDE, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MTOCRF(u32 l, u32 crm, u32 rs) {
    ThisCall3("MTOCRF", &PPUInterpreter::MTOCRF, &m_interpreter, l, crm, rs);
}

void PPULLVMRecompiler::STDX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STDX", &PPUInterpreter::STDX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STWCX_(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STWCX_", &PPUInterpreter::STWCX_, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STWX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STWX", &PPUInterpreter::STWX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STVEHX(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVEHX", &PPUInterpreter::STVEHX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::STDUX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STDUX", &PPUInterpreter::STDUX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STWUX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STWUX", &PPUInterpreter::STWUX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STVEWX(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVEWX", &PPUInterpreter::STVEWX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::ADDZE(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall4("ADDZE", &PPUInterpreter::ADDZE, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall4("SUBFZE", &PPUInterpreter::SUBFZE, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::STDCX_(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STDCX_", &PPUInterpreter::STDCX_, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STBX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STBX", &PPUInterpreter::STBX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STVX(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVX", &PPUInterpreter::STVX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::SUBFME(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall4("SUBFME", &PPUInterpreter::SUBFME, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("MULLD", &PPUInterpreter::MULLD, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDME(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall4("ADDME", &PPUInterpreter::ADDME, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("MULLW", &PPUInterpreter::MULLW, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DCBTST(u32 ra, u32 rb, u32 th) {
    ThisCall3("DCBTST", &PPUInterpreter::DCBTST, &m_interpreter, ra, rb, th);
}

void PPULLVMRecompiler::STBUX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STBUX", &PPUInterpreter::STBUX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("ADD", &PPUInterpreter::ADD, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DCBT(u32 ra, u32 rb, u32 th) {
    ThisCall3("DCBT", &PPUInterpreter::DCBT, &m_interpreter, ra, rb, th);
}

void PPULLVMRecompiler::LHZX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LHZX", &PPUInterpreter::LHZX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::EQV(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("EQV", &PPUInterpreter::EQV, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::ECIWX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("ECIWX", &PPUInterpreter::ECIWX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LHZUX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LHZUX", &PPUInterpreter::LHZUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::XOR(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("XOR", &PPUInterpreter::XOR, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::MFSPR(u32 rd, u32 spr) {
    ThisCall2("MFSPR", &PPUInterpreter::MFSPR, &m_interpreter, rd, spr);
}

void PPULLVMRecompiler::LWAX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LWAX", &PPUInterpreter::LWAX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::DST(u32 ra, u32 rb, u32 strm, u32 t) {
    ThisCall4("DST", &PPUInterpreter::DST, &m_interpreter, ra, rb, strm, t);
}

void PPULLVMRecompiler::LHAX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LHAX", &PPUInterpreter::LHAX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LVXL(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVXL", &PPUInterpreter::LVXL, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::MFTB(u32 rd, u32 spr) {
    ThisCall2("MFTB", &PPUInterpreter::MFTB, &m_interpreter, rd, spr);
}

void PPULLVMRecompiler::LWAUX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LWAUX", &PPUInterpreter::LWAUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::DSTST(u32 ra, u32 rb, u32 strm, u32 t) {
    ThisCall4("DSTST", &PPUInterpreter::DSTST, &m_interpreter, ra, rb, strm, t);
}

void PPULLVMRecompiler::LHAUX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LHAUX", &PPUInterpreter::LHAUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::STHX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STHX", &PPUInterpreter::STHX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::ORC(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("ORC", &PPUInterpreter::ORC, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::ECOWX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("ECOWX", &PPUInterpreter::ECOWX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STHUX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STHUX", &PPUInterpreter::STHUX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::OR(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("OR", &PPUInterpreter::OR, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("DIVDU", &PPUInterpreter::DIVDU, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("DIVWU", &PPUInterpreter::DIVWU, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MTSPR(u32 spr, u32 rs) {
    ThisCall2("MTSPR", &PPUInterpreter::MTSPR, &m_interpreter, spr, rs);
}

void PPULLVMRecompiler::NAND(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("NAND", &PPUInterpreter::NAND, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::STVXL(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVXL", &PPUInterpreter::STVXL, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("DIVD", &PPUInterpreter::DIVD, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall5("DIVW", &PPUInterpreter::DIVW, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::LVLX(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVLX", &PPUInterpreter::LVLX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LDBRX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LDBRX", &PPUInterpreter::LDBRX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LSWX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LSWX", &PPUInterpreter::LSWX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LWBRX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LWBRX", &PPUInterpreter::LWBRX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LFSX(u32 frd, u32 ra, u32 rb) {
    ThisCall3("LFSX", &PPUInterpreter::LFSX, &m_interpreter, frd, ra, rb);
}

void PPULLVMRecompiler::SRW(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("SRW", &PPUInterpreter::SRW, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::SRD(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("SRD", &PPUInterpreter::SRD, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::LVRX(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVRX", &PPUInterpreter::LVRX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LSWI(u32 rd, u32 ra, u32 nb) {
    ThisCall3("LSWI", &PPUInterpreter::LSWI, &m_interpreter, rd, ra, nb);
}

void PPULLVMRecompiler::LFSUX(u32 frd, u32 ra, u32 rb) {
    ThisCall3("LFSUX", &PPUInterpreter::LFSUX, &m_interpreter, frd, ra, rb);
}

void PPULLVMRecompiler::SYNC(u32 l) {
    ThisCall1("SYNC", &PPUInterpreter::SYNC, &m_interpreter, l);
}

void PPULLVMRecompiler::LFDX(u32 frd, u32 ra, u32 rb) {
    ThisCall3("LFDX", &PPUInterpreter::LFDX, &m_interpreter, frd, ra, rb);
}

void PPULLVMRecompiler::LFDUX(u32 frd, u32 ra, u32 rb) {
    ThisCall3("LFDUX", &PPUInterpreter::LFDUX, &m_interpreter, frd, ra, rb);
}

void PPULLVMRecompiler::STVLX(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVLX", &PPUInterpreter::STVLX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::STSWX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STSWX", &PPUInterpreter::STSWX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STWBRX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STWBRX", &PPUInterpreter::STWBRX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STFSX(u32 frs, u32 ra, u32 rb) {
    ThisCall3("STFSX", &PPUInterpreter::STFSX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::STVRX(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVRX", &PPUInterpreter::STVRX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::STFSUX(u32 frs, u32 ra, u32 rb) {
    ThisCall3("STFSUX", &PPUInterpreter::STFSUX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::STSWI(u32 rd, u32 ra, u32 nb) {
    ThisCall3("STSWI", &PPUInterpreter::STSWI, &m_interpreter, rd, ra, nb);
}

void PPULLVMRecompiler::STFDX(u32 frs, u32 ra, u32 rb) {
    ThisCall3("STFDX", &PPUInterpreter::STFDX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::STFDUX(u32 frs, u32 ra, u32 rb) {
    ThisCall3("STFDUX", &PPUInterpreter::STFDUX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::LVLXL(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVLXL", &PPUInterpreter::LVLXL, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LHBRX(u32 rd, u32 ra, u32 rb) {
    ThisCall3("LHBRX", &PPUInterpreter::LHBRX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::SRAW(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("SRAW", &PPUInterpreter::SRAW, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::SRAD(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall4("SRAD", &PPUInterpreter::SRAD, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::LVRXL(u32 vd, u32 ra, u32 rb) {
    ThisCall3("LVRXL", &PPUInterpreter::LVRXL, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::DSS(u32 strm, u32 a) {
    ThisCall2("DSS", &PPUInterpreter::DSS, &m_interpreter, strm, a);
}

void PPULLVMRecompiler::SRAWI(u32 ra, u32 rs, u32 sh, bool rc) {
    ThisCall4("SRAWI", &PPUInterpreter::SRAWI, &m_interpreter, ra, rs, sh, rc);
}

void PPULLVMRecompiler::SRADI1(u32 ra, u32 rs, u32 sh, bool rc) {
    ThisCall4("SRADI1", &PPUInterpreter::SRADI1, &m_interpreter, ra, rs, sh, rc);
}

void PPULLVMRecompiler::SRADI2(u32 ra, u32 rs, u32 sh, bool rc) {
    ThisCall4("SRADI2", &PPUInterpreter::SRADI2, &m_interpreter, ra, rs, sh, rc);
}

void PPULLVMRecompiler::EIEIO() {
    ThisCall0("EIEIO", &PPUInterpreter::EIEIO, &m_interpreter);
}

void PPULLVMRecompiler::STVLXL(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVLXL", &PPUInterpreter::STVLXL, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::STHBRX(u32 rs, u32 ra, u32 rb) {
    ThisCall3("STHBRX", &PPUInterpreter::STHBRX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::EXTSH(u32 ra, u32 rs, bool rc) {
    ThisCall3("EXTSH", &PPUInterpreter::EXTSH, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::STVRXL(u32 vs, u32 ra, u32 rb) {
    ThisCall3("STVRXL", &PPUInterpreter::STVRXL, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::EXTSB(u32 ra, u32 rs, bool rc) {
    ThisCall3("EXTSB", &PPUInterpreter::EXTSB, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::STFIWX(u32 frs, u32 ra, u32 rb) {
    ThisCall3("STFIWX", &PPUInterpreter::STFIWX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::EXTSW(u32 ra, u32 rs, bool rc) {
    ThisCall3("EXTSW", &PPUInterpreter::EXTSW, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::ICBI(u32 ra, u32 rs) {
    ThisCall2("ICBI", &PPUInterpreter::ICBI, &m_interpreter, ra, rs);
}

void PPULLVMRecompiler::DCBZ(u32 ra, u32 rb) {
    ThisCall2("DCBZ", &PPUInterpreter::DCBZ, &m_interpreter, ra, rb);
}

void PPULLVMRecompiler::LWZ(u32 rd, u32 ra, s32 d) {
    ThisCall3("LWZ", &PPUInterpreter::LWZ, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LWZU(u32 rd, u32 ra, s32 d) {
    ThisCall3("LWZU", &PPUInterpreter::LWZU, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LBZ(u32 rd, u32 ra, s32 d) {
    ThisCall3("LBZ", &PPUInterpreter::LBZ, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LBZU(u32 rd, u32 ra, s32 d) {
    ThisCall3("LBZU", &PPUInterpreter::LBZU, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::STW(u32 rs, u32 ra, s32 d) {
    ThisCall3("STW", &PPUInterpreter::STW, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STWU(u32 rs, u32 ra, s32 d) {
    ThisCall3("STWU", &PPUInterpreter::STWU, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STB(u32 rs, u32 ra, s32 d) {
    ThisCall3("STB", &PPUInterpreter::STB, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STBU(u32 rs, u32 ra, s32 d) {
    ThisCall3("STBU", &PPUInterpreter::STBU, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::LHZ(u32 rd, u32 ra, s32 d) {
    ThisCall3("LHZ", &PPUInterpreter::LHZ, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LHZU(u32 rd, u32 ra, s32 d) {
    ThisCall3("LHZU", &PPUInterpreter::LHZU, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LHA(u32 rd, u32 ra, s32 d) {
    ThisCall3("LHA", &PPUInterpreter::LHA, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LHAU(u32 rd, u32 ra, s32 d) {
    ThisCall3("LHAU", &PPUInterpreter::LHAU, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::STH(u32 rs, u32 ra, s32 d) {
    ThisCall3("STH", &PPUInterpreter::STH, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STHU(u32 rs, u32 ra, s32 d) {
    ThisCall3("STHU", &PPUInterpreter::STHU, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::LMW(u32 rd, u32 ra, s32 d) {
    ThisCall3("LMW", &PPUInterpreter::LMW, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::STMW(u32 rs, u32 ra, s32 d) {
    ThisCall3("STMW", &PPUInterpreter::STMW, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::LFS(u32 frd, u32 ra, s32 d) {
    ThisCall3("LFS", &PPUInterpreter::LFS, &m_interpreter, frd, ra, d);
}

void PPULLVMRecompiler::LFSU(u32 frd, u32 ra, s32 ds) {
    ThisCall3("LFSU", &PPUInterpreter::LFSU, &m_interpreter, frd, ra, ds);
}

void PPULLVMRecompiler::LFD(u32 frd, u32 ra, s32 d) {
    ThisCall3("LFD", &PPUInterpreter::LFD, &m_interpreter, frd, ra, d);
}

void PPULLVMRecompiler::LFDU(u32 frd, u32 ra, s32 ds) {
    ThisCall3("LFDU", &PPUInterpreter::LFDU, &m_interpreter, frd, ra, ds);
}

void PPULLVMRecompiler::STFS(u32 frs, u32 ra, s32 d) {
    ThisCall3("STFS", &PPUInterpreter::STFS, &m_interpreter, frs, ra, d);
}

void PPULLVMRecompiler::STFSU(u32 frs, u32 ra, s32 d) {
    ThisCall3("STFSU", &PPUInterpreter::STFSU, &m_interpreter, frs, ra, d);
}

void PPULLVMRecompiler::STFD(u32 frs, u32 ra, s32 d) {
    ThisCall3("STFD", &PPUInterpreter::STFD, &m_interpreter, frs, ra, d);
}

void PPULLVMRecompiler::STFDU(u32 frs, u32 ra, s32 d) {
    ThisCall3("STFDU", &PPUInterpreter::STFDU, &m_interpreter, frs, ra, d);
}

void PPULLVMRecompiler::LD(u32 rd, u32 ra, s32 ds) {
    ThisCall3("LD", &PPUInterpreter::LD, &m_interpreter, rd, ra, ds);
}

void PPULLVMRecompiler::LDU(u32 rd, u32 ra, s32 ds) {
    ThisCall3("LDU", &PPUInterpreter::LDU, &m_interpreter, rd, ra, ds);
}

void PPULLVMRecompiler::LWA(u32 rd, u32 ra, s32 ds) {
    ThisCall3("LWA", &PPUInterpreter::LWA, &m_interpreter, rd, ra, ds);
}

void PPULLVMRecompiler::FDIVS(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall4("FDIVS", &PPUInterpreter::FDIVS, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSUBS(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall4("FSUBS", &PPUInterpreter::FSUBS, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FADDS(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall4("FADDS", &PPUInterpreter::FADDS, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSQRTS(u32 frd, u32 frb, bool rc) {
    ThisCall3("FSQRTS", &PPUInterpreter::FSQRTS, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FRES(u32 frd, u32 frb, bool rc) {
    ThisCall3("FRES", &PPUInterpreter::FRES, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FMULS(u32 frd, u32 fra, u32 frc, bool rc) {
    ThisCall4("FMULS", &PPUInterpreter::FMULS, &m_interpreter, frd, fra, frc, rc);
}

void PPULLVMRecompiler::FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FMADDS", &PPUInterpreter::FMADDS, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FMSUBS", &PPUInterpreter::FMSUBS, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FNMSUBS", &PPUInterpreter::FNMSUBS, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FNMADDS", &PPUInterpreter::FNMADDS, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::STD(u32 rs, u32 ra, s32 d) {
    ThisCall3("STD", &PPUInterpreter::STD, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STDU(u32 rs, u32 ra, s32 ds) {
    ThisCall3("STDU", &PPUInterpreter::STDU, &m_interpreter, rs, ra, ds);
}

void PPULLVMRecompiler::MTFSB1(u32 crbd, bool rc) {
    ThisCall2("MTFSB1", &PPUInterpreter::MTFSB1, &m_interpreter, crbd, rc);
}

void PPULLVMRecompiler::MCRFS(u32 crbd, u32 crbs) {
    ThisCall2("MCRFS", &PPUInterpreter::MCRFS, &m_interpreter, crbd, crbs);
}

void PPULLVMRecompiler::MTFSB0(u32 crbd, bool rc) {
    ThisCall2("MTFSB0", &PPUInterpreter::MTFSB0, &m_interpreter, crbd, rc);
}

void PPULLVMRecompiler::MTFSFI(u32 crfd, u32 i, bool rc) {
    ThisCall3("MTFSFI", &PPUInterpreter::MTFSFI, &m_interpreter, crfd, i, rc);
}

void PPULLVMRecompiler::MFFS(u32 frd, bool rc) {
    ThisCall2("MFFS", &PPUInterpreter::MFFS, &m_interpreter, frd, rc);
}

void PPULLVMRecompiler::MTFSF(u32 flm, u32 frb, bool rc) {
    ThisCall3("MTFSF", &PPUInterpreter::MTFSF, &m_interpreter, flm, frb, rc);
}

void PPULLVMRecompiler::FCMPU(u32 crfd, u32 fra, u32 frb) {
    ThisCall3("FCMPU", &PPUInterpreter::FCMPU, &m_interpreter, crfd, fra, frb);
}

void PPULLVMRecompiler::FRSP(u32 frd, u32 frb, bool rc) {
    ThisCall3("FRSP", &PPUInterpreter::FRSP, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIW(u32 frd, u32 frb, bool rc) {
    ThisCall3("FCTIW", &PPUInterpreter::FCTIW, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIWZ(u32 frd, u32 frb, bool rc) {
    ThisCall3("FCTIWZ", &PPUInterpreter::FCTIWZ, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FDIV(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall4("FDIV", &PPUInterpreter::FDIV, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSUB(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall4("FSUB", &PPUInterpreter::FSUB, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FADD(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall4("FADD", &PPUInterpreter::FADD, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSQRT(u32 frd, u32 frb, bool rc) {
    ThisCall3("FSQRT", &PPUInterpreter::FSQRT, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FSEL", &PPUInterpreter::FSEL, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMUL(u32 frd, u32 fra, u32 frc, bool rc) {
    ThisCall4("FMUL", &PPUInterpreter::FMUL, &m_interpreter, frd, fra, frc, rc);
}

void PPULLVMRecompiler::FRSQRTE(u32 frd, u32 frb, bool rc) {
    ThisCall3("FRSQRTE", &PPUInterpreter::FRSQRTE, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FMSUB", &PPUInterpreter::FMSUB, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FMADD", &PPUInterpreter::FMADD, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FNMSUB", &PPUInterpreter::FNMSUB, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall5("FNMADD", &PPUInterpreter::FNMADD, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FCMPO(u32 crfd, u32 fra, u32 frb) {
    ThisCall3("FCMPO", &PPUInterpreter::FCMPO, &m_interpreter, crfd, fra, frb);
}

void PPULLVMRecompiler::FNEG(u32 frd, u32 frb, bool rc) {
    ThisCall3("FNEG", &PPUInterpreter::FNEG, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FMR(u32 frd, u32 frb, bool rc) {
    ThisCall3("FMR", &PPUInterpreter::FMR, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FNABS(u32 frd, u32 frb, bool rc) {
    ThisCall3("FNABS", &PPUInterpreter::FNABS, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FABS(u32 frd, u32 frb, bool rc) {
    ThisCall3("FABS", &PPUInterpreter::FABS, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCTID(u32 frd, u32 frb, bool rc) {
    ThisCall3("FCTID", &PPUInterpreter::FCTID, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIDZ(u32 frd, u32 frb, bool rc) {
    ThisCall3("FCTIDZ", &PPUInterpreter::FCTIDZ, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCFID(u32 frd, u32 frb, bool rc) {
    ThisCall3("FCFID", &PPUInterpreter::FCFID, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::UNK(const u32 code, const u32 opcode, const u32 gcode) {
    //ThisCall3("UNK", &PPUInterpreter::UNK, &m_interpreter, code, opcode, gcode);
}

Value * PPULLVMRecompiler::GetBit(Value * val, u32 n) {
    Value * bit;

    if (val->getType()->isIntegerTy(32)) {
        bit = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_32), val, m_ir_builder.getInt32(1 << (31- n)));
    } else if (val->getType()->isIntegerTy(64)) {
        bit = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_64), val, m_ir_builder.getInt64(1 << (63 - n)));
    } else {
        if (val->getType()->getIntegerBitWidth() != (n + 1)) {
            bit = m_ir_builder.CreateLShr(val, val->getType()->getIntegerBitWidth() - n - 1);
        }

        bit = m_ir_builder.CreateAnd(val, 1);
    }

    return bit;
}

Value * PPULLVMRecompiler::ClrBit(Value * val, u32 n) {
    return m_ir_builder.CreateAnd(val, ~(1 << (val->getType()->getIntegerBitWidth() - n - 1)));
}

Value * PPULLVMRecompiler::SetBit(Value * val, u32 n, Value * bit, bool doClear) {
    if (doClear) {
        val = ClrBit(val, n);
    }

    if (bit->getType()->getIntegerBitWidth() < val->getType()->getIntegerBitWidth()) {
        bit = m_ir_builder.CreateZExt(bit, val->getType());
    } else if (bit->getType()->getIntegerBitWidth() > val->getType()->getIntegerBitWidth()) {
        bit = m_ir_builder.CreateTrunc(bit, val->getType());
    }

    if (val->getType()->getIntegerBitWidth() != (n + 1)) {
        bit = m_ir_builder.CreateShl(bit, bit->getType()->getIntegerBitWidth() - n - 1);
    }

    return m_ir_builder.CreateOr(val, bit);
}

Value * PPULLVMRecompiler::GetNibble(Value * val, u32 n) {
    Value * nibble;

    if (val->getType()->isIntegerTy(32)) {
        nibble = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_32), val, m_ir_builder.getInt32(0xF << ((7 - n) * 4)));
    } else if (val->getType()->isIntegerTy(64)) {
        nibble = m_ir_builder.CreateCall2(Intrinsic::getDeclaration(m_module, Intrinsic::x86_bmi_pext_64), val, m_ir_builder.getInt64(0xF << ((15 - n) * 4)));
    } else {
        if ((val->getType()->getIntegerBitWidth() >> 2) != (n + 1)) {
            nibble = m_ir_builder.CreateLShr(val, (((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4);
        }

        nibble = m_ir_builder.CreateAnd(val, 0xF);
    }

    return nibble;
}

Value * PPULLVMRecompiler::ClrNibble(Value * val, u32 n) {
    return m_ir_builder.CreateAnd(val, ~(0xF << ((((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4)));
}

Value * PPULLVMRecompiler::SetNibble(Value * val, u32 n, Value * nibble, bool doClear) {
    if (doClear) {
        val = ClrNibble(val, n);
    }

    if (nibble->getType()->getIntegerBitWidth() < val->getType()->getIntegerBitWidth()) {
        nibble = m_ir_builder.CreateZExt(nibble, val->getType());
    } else if (nibble->getType()->getIntegerBitWidth() > val->getType()->getIntegerBitWidth()) {
        nibble = m_ir_builder.CreateTrunc(nibble, val->getType());
    }

    if ((val->getType()->getIntegerBitWidth() >> 2) != (n + 1)) {
        nibble = m_ir_builder.CreateShl(nibble, (((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4);
    }

    return m_ir_builder.CreateOr(val, nibble);
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

Value * PPULLVMRecompiler::GetGpr(u32 r) {
    auto r_i64_ptr = m_ir_builder.CreateConstGEP2_32(m_gpr, 0, r);
    return m_ir_builder.CreateLoad(r_i64_ptr);
}

void PPULLVMRecompiler::SetGpr(u32 r, Value * val_x64) {
    auto r_i64_ptr = m_ir_builder.CreateConstGEP2_32(m_gpr, 0, r);
    auto val_i64   = m_ir_builder.CreateBitCast(val_x64, Type::getInt64Ty(m_llvm_context));
    m_ir_builder.CreateStore(val_i64, r_i64_ptr);
}

Value * PPULLVMRecompiler::GetCr() {
    return m_ir_builder.CreateLoad(m_cr);
}

Value * PPULLVMRecompiler::GetCrField(u32 n) {
    return GetNibble(GetCr(), n);
}

void PPULLVMRecompiler::SetCr(Value * val_x32) {
    auto val_i32 = m_ir_builder.CreateBitCast(val_x32, Type::getInt32Ty(m_llvm_context));
    m_ir_builder.CreateStore(val_i32, m_cr);
}

void PPULLVMRecompiler::SetCrField(u32 n, Value * field) {
    SetCr(SetNibble(GetCr(), n, field));
}

void PPULLVMRecompiler::SetCrField(u32 n, Value * b0, Value * b1, Value * b2, Value * b3) {
    SetCr(SetNibble(GetCr(), n, b0, b1, b2, b3));
}

void PPULLVMRecompiler::SetCrFieldSignedCmp(u32 n, llvm::Value * a, llvm::Value * b) {
    auto lt_i1  = m_ir_builder.CreateICmpSLT(a, b);
    auto gt_i1  = m_ir_builder.CreateICmpSGT(a, b);
    auto eq_i1  = m_ir_builder.CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void PPULLVMRecompiler::SetCrFieldUnsignedCmp(u32 n, llvm::Value * a, llvm::Value * b) {
    auto lt_i1  = m_ir_builder.CreateICmpULT(a, b);
    auto gt_i1  = m_ir_builder.CreateICmpUGT(a, b);
    auto eq_i1  = m_ir_builder.CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void PPULLVMRecompiler::SetCr6AfterVectorCompare(u32 vr) {
    auto vr_v16i8    = GetVrAsIntVec(vr, 8);
    auto vr_mask_i32 = m_ir_builder.CreateCall(Intrinsic::getDeclaration(m_module, Intrinsic::x86_sse2_pmovmskb_128), vr_v16i8);
    auto cmp0_i1     = m_ir_builder.CreateICmpEQ(vr_mask_i32, m_ir_builder.getInt32(0));
    auto cmp1_i1     = m_ir_builder.CreateICmpEQ(vr_mask_i32, m_ir_builder.getInt32(0xFFFF));
    auto cr_i32      = GetCr();
    cr_i32           = SetNibble(cr_i32, 6, cmp1_i1, nullptr, cmp0_i1, nullptr);
    SetCr(cr_i32);
}

Value * PPULLVMRecompiler::GetXer() {
    return m_ir_builder.CreateLoad(m_xer);
}

Value * PPULLVMRecompiler::GetXerCa() {
    return GetBit(GetXer(), 34);
}

Value * PPULLVMRecompiler::GetXerSo() {
    return GetBit(GetXer(), 32);
}

void PPULLVMRecompiler::SetXer(Value * val_x64) {
    auto val_i64 = m_ir_builder.CreateBitCast(val_x64, Type::getInt64Ty(m_llvm_context));
    m_ir_builder.CreateStore(val_i64, m_xer);
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

Value * PPULLVMRecompiler::GetVrAsIntVec(u32 vr, u32 vec_elt_num_bits) {
    auto vr_i128_ptr = m_ir_builder.CreateConstGEP2_32(m_vpr, 0, vr);
    auto vr_vec_ptr  = m_ir_builder.CreateBitCast(vr_i128_ptr, VectorType::get(Type::getIntNTy(m_llvm_context, vec_elt_num_bits), 128 / vec_elt_num_bits)->getPointerTo());
    return m_ir_builder.CreateLoad(vr_vec_ptr);
}

Value * PPULLVMRecompiler::GetVrAsFloatVec(u32 vr) {
    auto vr_i128_ptr  = m_ir_builder.CreateConstGEP2_32(m_vpr, 0, vr);
    auto vr_v4f32_ptr = m_ir_builder.CreateBitCast(vr_i128_ptr, VectorType::get(Type::getFloatTy(m_llvm_context), 4)->getPointerTo());
    return m_ir_builder.CreateLoad(vr_v4f32_ptr);
}

Value * PPULLVMRecompiler::GetVrAsDoubleVec(u32 vr) {
    auto vr_i128_ptr  = m_ir_builder.CreateConstGEP2_32(m_vpr, 0, vr);
    auto vr_v2f64_ptr = m_ir_builder.CreateBitCast(vr_i128_ptr, VectorType::get(Type::getDoubleTy(m_llvm_context), 2)->getPointerTo());
    return m_ir_builder.CreateLoad(vr_v2f64_ptr);
}

void PPULLVMRecompiler::SetVr(u32 vr, Value * val_x128) {
    auto vr_i128_ptr = m_ir_builder.CreateConstGEP2_32(m_vpr, 0, vr);
    auto val_i128    = m_ir_builder.CreateBitCast(val_x128, Type::getIntNTy(m_llvm_context, 128));
    m_ir_builder.CreateStore(val_i128, vr_i128_ptr);
}

template<class F, class C>
void PPULLVMRecompiler::ThisCall0(const char * name, F function, C * this_p) {
    static FunctionType * fn_type = nullptr;

    if (fn_type == nullptr) {
        static std::vector<Type *> fn_args;
        fn_args.push_back(Type::getInt64PtrTy(m_llvm_context));
        fn_type = FunctionType::get(Type::getVoidTy(m_llvm_context), fn_args, false);
    }

    auto fn_ptr  = m_module->getFunction(name);
    if (!fn_ptr) {
        fn_ptr = Function::Create(fn_type, GlobalValue::ExternalLinkage, name, m_module);
        fn_ptr->setCallingConv(CallingConv::C);
        m_execution_engine->addGlobalMapping(fn_ptr, (void *&)function);
    }

    auto this_i64 = m_ir_builder.getInt64((uint64_t)this_p);
    auto this_ptr = m_ir_builder.CreateIntToPtr(this_i64, Type::getInt64PtrTy(m_llvm_context));
    m_ir_builder.CreateCall(fn_ptr, this_ptr);
}

template<class F, class C, class T1>
void PPULLVMRecompiler::ThisCall1(const char * name, F function, C * this_p, T1 arg1) {
    static FunctionType * fn_type = nullptr;

    if (fn_type == nullptr) {
        static std::vector<Type *> fn_args;
        fn_args.push_back(Type::getInt64PtrTy(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_type = FunctionType::get(Type::getVoidTy(m_llvm_context), fn_args, false);
    }

    auto fn_ptr  = m_module->getFunction(name);
    if (!fn_ptr) {
        fn_ptr = Function::Create(fn_type, GlobalValue::ExternalLinkage, name, m_module);
        fn_ptr->setCallingConv(CallingConv::C);
        m_execution_engine->addGlobalMapping(fn_ptr, (void *&)function);
    }

    auto this_i64 = m_ir_builder.getInt64((uint64_t)this_p);
    auto this_ptr = m_ir_builder.CreateIntToPtr(this_i64, Type::getInt64PtrTy(m_llvm_context));
    m_ir_builder.CreateCall2(fn_ptr, this_ptr, m_ir_builder.getInt32(arg1));
}

template<class F, class C, class T1, class T2>
void PPULLVMRecompiler::ThisCall2(const char * name, F function, C * this_p, T1 arg1, T2 arg2) {
    static FunctionType * fn_type = nullptr;

    if (fn_type == nullptr) {
        static std::vector<Type *> fn_args;
        fn_args.push_back(Type::getInt64PtrTy(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_type = FunctionType::get(Type::getVoidTy(m_llvm_context), fn_args, false);
    }

    auto fn_ptr  = m_module->getFunction(name);
    if (!fn_ptr) {
        fn_ptr = Function::Create(fn_type, GlobalValue::ExternalLinkage, name, m_module);
        fn_ptr->setCallingConv(CallingConv::C);
        m_execution_engine->addGlobalMapping(fn_ptr, (void *&)function);
    }

    auto this_i64 = m_ir_builder.getInt64((uint64_t)this_p);
    auto this_ptr = m_ir_builder.CreateIntToPtr(this_i64, Type::getInt64PtrTy(m_llvm_context));
    m_ir_builder.CreateCall3(fn_ptr, this_ptr, m_ir_builder.getInt32(arg1), m_ir_builder.getInt32(arg2));
}

template<class F, class C, class T1, class T2, class T3>
void PPULLVMRecompiler::ThisCall3(const char * name, F function, C * this_p, T1 arg1, T2 arg2, T3 arg3) {
    static FunctionType * fn_type = nullptr;

    if (fn_type == nullptr) {
        static std::vector<Type *> fn_args;
        fn_args.push_back(Type::getInt64PtrTy(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_type = FunctionType::get(Type::getVoidTy(m_llvm_context), fn_args, false);
    }

    auto fn_ptr  = m_module->getFunction(name);
    if (!fn_ptr) {
        fn_ptr = Function::Create(fn_type, GlobalValue::ExternalLinkage, name, m_module);
        fn_ptr->setCallingConv(CallingConv::C);
        m_execution_engine->addGlobalMapping(fn_ptr, (void *&)function);
    }

    auto this_i64 = m_ir_builder.getInt64((uint64_t)this_p);
    auto this_ptr = m_ir_builder.CreateIntToPtr(this_i64, Type::getInt64PtrTy(m_llvm_context));
    m_ir_builder.CreateCall4(fn_ptr, this_ptr, m_ir_builder.getInt32(arg1), m_ir_builder.getInt32(arg2), m_ir_builder.getInt32(arg3));
}

template<class F, class C, class T1, class T2, class T3, class T4>
void PPULLVMRecompiler::ThisCall4(const char * name, F function, C * this_p, T1 arg1, T2 arg2, T3 arg3, T4 arg4) {
    static FunctionType * fn_type = nullptr;

    if (fn_type == nullptr) {
        static std::vector<Type *> fn_args;
        fn_args.push_back(Type::getInt64PtrTy(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_type = FunctionType::get(Type::getVoidTy(m_llvm_context), fn_args, false);
    }

    auto fn_ptr  = m_module->getFunction(name);
    if (!fn_ptr) {
        fn_ptr = Function::Create(fn_type, GlobalValue::ExternalLinkage, name, m_module);
        fn_ptr->setCallingConv(CallingConv::C);
        m_execution_engine->addGlobalMapping(fn_ptr, (void *&)function);
    }

    auto this_i64 = m_ir_builder.getInt64((uint64_t)this_p);
    auto this_ptr = m_ir_builder.CreateIntToPtr(this_i64, Type::getInt64PtrTy(m_llvm_context));
    m_ir_builder.CreateCall5(fn_ptr, this_ptr, m_ir_builder.getInt32(arg1), m_ir_builder.getInt32(arg2), m_ir_builder.getInt32(arg3), m_ir_builder.getInt32(arg4));
}

template<class F, class C, class T1, class T2, class T3, class T4, class T5>
void PPULLVMRecompiler::ThisCall5(const char * name, F function, C * this_p, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5) {
    static FunctionType * fn_type = nullptr;

    if (fn_type == nullptr) {
        static std::vector<Type *> fn_args;
        fn_args.push_back(Type::getInt64PtrTy(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_type = FunctionType::get(Type::getVoidTy(m_llvm_context), fn_args, false);
    }

    auto fn_ptr  = m_module->getFunction(name);
    if (!fn_ptr) {
        fn_ptr = Function::Create(fn_type, GlobalValue::ExternalLinkage, name, m_module);
        fn_ptr->setCallingConv(CallingConv::C);
        m_execution_engine->addGlobalMapping(fn_ptr, (void *&)function);
    }

    auto this_i64 = m_ir_builder.getInt64((uint64_t)this_p);
    auto this_ptr = m_ir_builder.CreateIntToPtr(this_i64, Type::getInt64PtrTy(m_llvm_context));
    std::vector<Value *> args;
    args.push_back(this_ptr);
    args.push_back(m_ir_builder.getInt32(arg1));
    args.push_back(m_ir_builder.getInt32(arg2));
    args.push_back(m_ir_builder.getInt32(arg3));
    args.push_back(m_ir_builder.getInt32(arg4));
    args.push_back(m_ir_builder.getInt32(arg5));
    m_ir_builder.CreateCall(fn_ptr, args);
}

template<class F, class C, class T1, class T2, class T3, class T4, class T5, class T6>
void PPULLVMRecompiler::ThisCall6(const char * name, F function, C * this_p, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6) {
    static FunctionType * fn_type = nullptr;

    if (fn_type == nullptr) {
        static std::vector<Type *> fn_args;
        fn_args.push_back(Type::getInt64PtrTy(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_args.push_back(Type::getInt32Ty(m_llvm_context));
        fn_type = FunctionType::get(Type::getVoidTy(m_llvm_context), fn_args, false);
    }

    auto fn_ptr  = m_module->getFunction(name);
    if (!fn_ptr) {
        fn_ptr = Function::Create(fn_type, GlobalValue::ExternalLinkage, name, m_module);
        fn_ptr->setCallingConv(CallingConv::C);
        m_execution_engine->addGlobalMapping(fn_ptr, (void *&)function);
    }

    auto this_i64 = m_ir_builder.getInt64((uint64_t)this_p);
    auto this_ptr = m_ir_builder.CreateIntToPtr(this_i64, Type::getInt64PtrTy(m_llvm_context));
    std::vector<Value *> args;
    args.push_back(this_ptr);
    args.push_back(m_ir_builder.getInt32(arg1));
    args.push_back(m_ir_builder.getInt32(arg2));
    args.push_back(m_ir_builder.getInt32(arg3));
    args.push_back(m_ir_builder.getInt32(arg4));
    args.push_back(m_ir_builder.getInt32(arg5));
    args.push_back(m_ir_builder.getInt32(arg6));
    m_ir_builder.CreateCall(fn_ptr, args);
}
