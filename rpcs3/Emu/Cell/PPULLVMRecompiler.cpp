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
    , m_ir_builder(m_llvm_context) {
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetDisassembler();

    m_module = new Module("Module", m_llvm_context);
    m_gpr    = new GlobalVariable(*m_module, ArrayType::get(Type::getInt64Ty(m_llvm_context), 32), false, GlobalValue::ExternalLinkage, nullptr, "gpr");
    m_vpr    = new GlobalVariable(*m_module, ArrayType::get(Type::getIntNTy(m_llvm_context, 128), 32), false, GlobalValue::ExternalLinkage, nullptr, "vpr");
    m_vscr   = new GlobalVariable(*m_module, Type::getInt32Ty(m_llvm_context), false, GlobalValue::ExternalLinkage, nullptr, "vscr");

    m_execution_engine = EngineBuilder(m_module).create();
    m_execution_engine->addGlobalMapping(m_gpr, m_ppu.GPR);
    m_execution_engine->addGlobalMapping(m_vpr, m_ppu.VPR);
    m_execution_engine->addGlobalMapping(m_vscr, &m_ppu.VSCR);

    m_disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);

    RunAllTests();
}

PPULLVMRecompiler::~PPULLVMRecompiler() {
    LLVMDisasmDispose(m_disassembler);
    delete m_execution_engine;
    llvm_shutdown();
}

u8 PPULLVMRecompiler::DecodeMemory(const u64 address) {
    auto function_name = fmt::Format("fn_0x%llx", address);
    auto function      = m_module->getFunction(function_name);

    if (!function) {
        function   = cast<Function>(m_module->getOrInsertFunction(function_name, Type::getVoidTy(m_llvm_context), (Type *)nullptr));
        auto block = BasicBlock::Create(m_llvm_context, "start", function);
        m_ir_builder.SetInsertPoint(block);

        //m_hit_branch_instruction = false;
        //while (!m_hit_branch_instruction)
        //{
        //	u32 instr = Memory.Read32(address);
        //	m_decoder.Decode(instr);
        //}

        VADDSWS(14, 23, 25);

        // TODO: Add code to set PC
        m_ir_builder.CreateRetVoid();
        std::string        s;
        raw_string_ostream s2(s);

        s2 << *m_module;
    }

    std::vector<GenericValue> args;
    m_execution_engine->runFunction(function, args);
    return 0;
}

void PPULLVMRecompiler::NULL_OP() {
    // UNK("null");
}

void PPULLVMRecompiler::NOP() {

}

void PPULLVMRecompiler::TDI(u32 to, u32 ra, s32 simm16) {
    s64 a = m_ppu.GPR[ra];

    if ((a < (s64)simm16 && (to & 0x10)) ||
        (a >(s64)simm16 && (to & 0x8)) ||
        (a == (s64)simm16 && (to & 0x4)) ||
        ((u64)a < (u64)simm16 && (to & 0x2)) ||
        ((u64)a > (u64)simm16 && (to & 0x1))) {
        // TODO: Log this 
        // UNK(fmt::Format("Trap! (tdi %x, r%d, %x)", to, ra, simm16));
    }
}

void PPULLVMRecompiler::TWI(u32 to, u32 ra, s32 simm16) {
    s32 a = (s32)m_ppu.GPR[ra];

    if ((a < simm16 && (to & 0x10)) ||
        (a > simm16 && (to & 0x8)) ||
        (a == simm16 && (to & 0x4)) ||
        ((u32)a < (u32)simm16 && (to & 0x2)) ||
        ((u32)a > (u32)simm16 && (to & 0x1))) {
        // TODO: Log this
        // UNK(fmt::Format("Trap! (twi %x, r%d, %x)", to, ra, simm16));
    }
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
    auto tmp3_v4i32    = m_ir_builder.CreateAdd(tmp2_v4i32, ConstantDataVector::get(m_llvm_context, tmp1_v4i32));

    // Next, we find if the addition can actually result in an overflow. Since an overflow can only happen if the operands
    // have the same sign, we bitwise AND both the operands. If the sign bit of the result is 1 then the operands have the
    // same sign and so may cause an overflow.
    auto tmp4_v4i32 = m_ir_builder.CreateAnd(va_v4i32, vb_v4i32);

    // Perform the sum.
    auto sum_v4i32 = m_ir_builder.CreateAdd(va_v4i32, vb_v4i32);

    // If an overflow occurs, then the sign of the sum will be different from the sign of the operands. So, we xor the
    // result with one of the operands. The sign bit of the result will be 1 if the sign bit of the sum and the sign bit of the
    // result is different. This result is again ANDed with tmp4 (the sign bit of tmp4 is 1 only if the operands have the same
    // sign and so can cause an overflow).
    auto tmp5_v4i32 = m_ir_builder.CreateXor(va_v4i32, sum_v4i32);
    auto tmp6_v4i32 = m_ir_builder.CreateAnd(tmp4_v4i32, tmp5_v4i32);
    auto tmp7_v4i32 = m_ir_builder.CreateAShr(tmp6_v4i32, 31);

    // tmp7 is equal to 0xFFFFFFFF if an overflow occured and 0x00000000 otherwise. tmp9 is bitwise inverse of tmp7.
    u32  tmp8_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    auto tmp9_v4i32    = m_ir_builder.CreateXor(tmp7_v4i32, ConstantDataVector::get(m_llvm_context, tmp8_v4i32));
    auto tmp10_v4i32   = m_ir_builder.CreateAnd(tmp3_v4i32, tmp7_v4i32);
    auto tmp11_v4i32   = m_ir_builder.CreateAnd(sum_v4i32, tmp9_v4i32);
    auto tmp12_v4i32   = m_ir_builder.CreateOr(tmp10_v4i32, tmp11_v4i32);
    SetVr(vd, tmp12_v4i32);

    // TODO: Set SAT
    // TODO: Optimize with pblend
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
    auto va_v1i128  = GetVrAsIntVec(va, 128);
    auto vb_v1i128  = GetVrAsIntVec(vb, 128);
    auto res_v1i128 = m_ir_builder.CreateAnd(va_v1i128, vb_v1i128);
    SetVr(vd, res_v1i128);
}

void PPULLVMRecompiler::VANDC(u32 vd, u32 va, u32 vb) {
    auto va_v1i128  = GetVrAsIntVec(va, 128);
    auto vb_v1i128  = GetVrAsIntVec(vb, 128);
    vb_v1i128       = m_ir_builder.CreateXor(vb_v1i128, m_ir_builder.getInt(APInt(128, "-1", 10)));
    auto res_v1i128 = m_ir_builder.CreateAnd(va_v1i128, vb_v1i128);
    SetVr(vd, res_v1i128);

    // TODO: Check if this generates ANDC
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
    auto avg_v16i8      = m_ir_builder.CreateBitCast(avg_v16i16, VectorType::get(Type::getInt8Ty(m_llvm_context), 16));
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
    auto avg_v8i16    = m_ir_builder.CreateBitCast(avg_v8i32, VectorType::get(Type::getInt16Ty(m_llvm_context), 8));
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
    auto avg_v4i32    = m_ir_builder.CreateBitCast(avg_v4i64, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
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
    auto avg_v4i32    = m_ir_builder.CreateBitCast(avg_v4i64, VectorType::get(Type::getInt32Ty(m_llvm_context), 4));
    SetVr(vd, avg_v4i32);
}

void PPULLVMRecompiler::VCFSX(u32 vd, u32 uimm5, u32 vb) {
    u32 scale = 1 << uimm5;

    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = ((float)m_ppu.VPR[vb]._s32[w]) / scale;
    }
}

void PPULLVMRecompiler::VCFUX(u32 vd, u32 uimm5, u32 vb) {
    u32 scale = 1 << uimm5;

    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = ((float)m_ppu.VPR[vb]._u32[w]) / scale;
    }
}

void PPULLVMRecompiler::VCMPBFP(u32 vd, u32 va, u32 vb) {
    //for (uint w = 0; w < 4; w++)
    //{
    //	u32 mask = 0;

    //	const float A = CheckVSCR_NJ(m_ppu.VPR[va]._f[w]);
    //	const float B = CheckVSCR_NJ(m_ppu.VPR[vb]._f[w]);

    //	if (A >  B) mask |= 1 << 31;
    //	if (A < -B) mask |= 1 << 30;

    //	m_ppu.VPR[vd]._u32[w] = mask;
    //}
}

void PPULLVMRecompiler::VCMPBFP_(u32 vd, u32 va, u32 vb) {
    //bool allInBounds = true;

    //for (uint w = 0; w < 4; w++)
    //{
    //	u32 mask = 0;

    //	const float A = CheckVSCR_NJ(m_ppu.VPR[va]._f[w]);
    //	const float B = CheckVSCR_NJ(m_ppu.VPR[vb]._f[w]);

    //	if (A >  B) mask |= 1 << 31;
    //	if (A < -B) mask |= 1 << 30;

    //	m_ppu.VPR[vd]._u32[w] = mask;

    //	if (mask)
    //		allInBounds = false;
    //}

    //// Bit n°2 of CR6
    //m_ppu.SetCRBit(6, 0x2, allInBounds);
}

void PPULLVMRecompiler::VCMPEQFP(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._f[w] == m_ppu.VPR[vb]._f[w] ? 0xffffffff : 0;
    }
}

void PPULLVMRecompiler::VCMPEQFP_(u32 vd, u32 va, u32 vb) {
    int all_equal = 0x8;
    int none_equal = 0x2;

    for (uint w = 0; w < 4; w++) {
        if (m_ppu.VPR[va]._f[w] == m_ppu.VPR[vb]._f[w]) {
            m_ppu.VPR[vd]._u32[w] = 0xffffffff;
            none_equal = 0;
        } else {
            m_ppu.VPR[vd]._u32[w] = 0;
            all_equal = 0;
        }
    }

    m_ppu.CR.cr6 = all_equal | none_equal;
}

void PPULLVMRecompiler::VCMPEQUB(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = m_ppu.VPR[va]._u8[b] == m_ppu.VPR[vb]._u8[b] ? 0xff : 0;
    }
}

void PPULLVMRecompiler::VCMPEQUB_(u32 vd, u32 va, u32 vb) {
    int all_equal = 0x8;
    int none_equal = 0x2;

    for (uint b = 0; b < 16; b++) {
        if (m_ppu.VPR[va]._u8[b] == m_ppu.VPR[vb]._u8[b]) {
            m_ppu.VPR[vd]._u8[b] = 0xff;
            none_equal = 0;
        } else {
            m_ppu.VPR[vd]._u8[b] = 0;
            all_equal = 0;
        }
    }

    m_ppu.CR.cr6 = all_equal | none_equal;
}

void PPULLVMRecompiler::VCMPEQUH(u32 vd, u32 va, u32 vb) //nf
{
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = m_ppu.VPR[va]._u16[h] == m_ppu.VPR[vb]._u16[h] ? 0xffff : 0;
    }
}
void PPULLVMRecompiler::VCMPEQUH_(u32 vd, u32 va, u32 vb) //nf
{
    int all_equal = 0x8;
    int none_equal = 0x2;

    for (uint h = 0; h < 8; h++) {
        if (m_ppu.VPR[va]._u16[h] == m_ppu.VPR[vb]._u16[h]) {
            m_ppu.VPR[vd]._u16[h] = 0xffff;
            none_equal = 0;
        } else {
            m_ppu.VPR[vd]._u16[h] = 0;
            all_equal = 0;
        }
    }

    m_ppu.CR.cr6 = all_equal | none_equal;
}
void PPULLVMRecompiler::VCMPEQUW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._u32[w] == m_ppu.VPR[vb]._u32[w] ? 0xffffffff : 0;
    }
}
void PPULLVMRecompiler::VCMPEQUW_(u32 vd, u32 va, u32 vb) {
    int all_equal = 0x8;
    int none_equal = 0x2;

    for (uint w = 0; w < 4; w++) {
        if (m_ppu.VPR[va]._u32[w] == m_ppu.VPR[vb]._u32[w]) {
            m_ppu.VPR[vd]._u32[w] = 0xffffffff;
            none_equal = 0;
        } else {
            m_ppu.VPR[vd]._u32[w] = 0;
            all_equal = 0;
        }
    }

    m_ppu.CR.cr6 = all_equal | none_equal;
}
void PPULLVMRecompiler::VCMPGEFP(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._f[w] >= m_ppu.VPR[vb]._f[w] ? 0xffffffff : 0;
    }
}
void PPULLVMRecompiler::VCMPGEFP_(u32 vd, u32 va, u32 vb) {
    int all_ge = 0x8;
    int none_ge = 0x2;

    for (uint w = 0; w < 4; w++) {
        if (m_ppu.VPR[va]._f[w] >= m_ppu.VPR[vb]._f[w]) {
            m_ppu.VPR[vd]._u32[w] = 0xffffffff;
            none_ge = 0;
        } else {
            m_ppu.VPR[vd]._u32[w] = 0;
            all_ge = 0;
        }
    }

    m_ppu.CR.cr6 = all_ge | none_ge;
}
void PPULLVMRecompiler::VCMPGTFP(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._f[w] > m_ppu.VPR[vb]._f[w] ? 0xffffffff : 0;
    }
}
void PPULLVMRecompiler::VCMPGTFP_(u32 vd, u32 va, u32 vb) {
    int all_ge = 0x8;
    int none_ge = 0x2;

    for (uint w = 0; w < 4; w++) {
        if (m_ppu.VPR[va]._f[w] > m_ppu.VPR[vb]._f[w]) {
            m_ppu.VPR[vd]._u32[w] = 0xffffffff;
            none_ge = 0;
        } else {
            m_ppu.VPR[vd]._u32[w] = 0;
            all_ge = 0;
        }
    }

    m_ppu.CR.cr6 = all_ge | none_ge;
}
void PPULLVMRecompiler::VCMPGTSB(u32 vd, u32 va, u32 vb) //nf
{
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = m_ppu.VPR[va]._s8[b] > m_ppu.VPR[vb]._s8[b] ? 0xff : 0;
    }
}
void PPULLVMRecompiler::VCMPGTSB_(u32 vd, u32 va, u32 vb) {
    int all_gt = 0x8;
    int none_gt = 0x2;

    for (uint b = 0; b < 16; b++) {
        if (m_ppu.VPR[va]._s8[b] > m_ppu.VPR[vb]._s8[b]) {
            m_ppu.VPR[vd]._u8[b] = 0xff;
            none_gt = 0;
        } else {
            m_ppu.VPR[vd]._u8[b] = 0;
            all_gt = 0;
        }
    }

    m_ppu.CR.cr6 = all_gt | none_gt;
}
void PPULLVMRecompiler::VCMPGTSH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = m_ppu.VPR[va]._s16[h] > m_ppu.VPR[vb]._s16[h] ? 0xffff : 0;
    }
}
void PPULLVMRecompiler::VCMPGTSH_(u32 vd, u32 va, u32 vb) {
    int all_gt = 0x8;
    int none_gt = 0x2;

    for (uint h = 0; h < 8; h++) {
        if (m_ppu.VPR[va]._s16[h] > m_ppu.VPR[vb]._s16[h]) {
            m_ppu.VPR[vd]._u16[h] = 0xffff;
            none_gt = 0;
        } else {
            m_ppu.VPR[vd]._u16[h] = 0;
            all_gt = 0;
        }
    }

    m_ppu.CR.cr6 = all_gt | none_gt;
}
void PPULLVMRecompiler::VCMPGTSW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._s32[w] > m_ppu.VPR[vb]._s32[w] ? 0xffffffff : 0;
    }
}
void PPULLVMRecompiler::VCMPGTSW_(u32 vd, u32 va, u32 vb) {
    int all_gt = 0x8;
    int none_gt = 0x2;

    for (uint w = 0; w < 4; w++) {
        if (m_ppu.VPR[va]._s32[w] > m_ppu.VPR[vb]._s32[w]) {
            m_ppu.VPR[vd]._u32[w] = 0xffffffff;
            none_gt = 0;
        } else {
            m_ppu.VPR[vd]._u32[w] = 0;
            all_gt = 0;
        }
    }

    m_ppu.CR.cr6 = all_gt | none_gt;
}
void PPULLVMRecompiler::VCMPGTUB(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = m_ppu.VPR[va]._u8[b] > m_ppu.VPR[vb]._u8[b] ? 0xff : 0;
    }
}
void PPULLVMRecompiler::VCMPGTUB_(u32 vd, u32 va, u32 vb) {
    int all_gt = 0x8;
    int none_gt = 0x2;

    for (uint b = 0; b < 16; b++) {
        if (m_ppu.VPR[va]._u8[b] > m_ppu.VPR[vb]._u8[b]) {
            m_ppu.VPR[vd]._u8[b] = 0xff;
            none_gt = 0;
        } else {
            m_ppu.VPR[vd]._u8[b] = 0;
            all_gt = 0;
        }
    }

    m_ppu.CR.cr6 = all_gt | none_gt;
}
void PPULLVMRecompiler::VCMPGTUH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = m_ppu.VPR[va]._u16[h] > m_ppu.VPR[vb]._u16[h] ? 0xffff : 0;
    }
}
void PPULLVMRecompiler::VCMPGTUH_(u32 vd, u32 va, u32 vb) {
    int all_gt = 0x8;
    int none_gt = 0x2;

    for (uint h = 0; h < 8; h++) {
        if (m_ppu.VPR[va]._u16[h] > m_ppu.VPR[vb]._u16[h]) {
            m_ppu.VPR[vd]._u16[h] = 0xffff;
            none_gt = 0;
        } else {
            m_ppu.VPR[vd]._u16[h] = 0;
            all_gt = 0;
        }
    }

    m_ppu.CR.cr6 = all_gt | none_gt;
}
void PPULLVMRecompiler::VCMPGTUW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._u32[w] > m_ppu.VPR[vb]._u32[w] ? 0xffffffff : 0;
    }
}
void PPULLVMRecompiler::VCMPGTUW_(u32 vd, u32 va, u32 vb) {
    int all_gt = 0x8;
    int none_gt = 0x2;

    for (uint w = 0; w < 4; w++) {
        if (m_ppu.VPR[va]._u32[w] > m_ppu.VPR[vb]._u32[w]) {
            m_ppu.VPR[vd]._u32[w] = 0xffffffff;
            none_gt = 0;
        } else {
            m_ppu.VPR[vd]._u32[w] = 0;
            all_gt = 0;
        }
    }

    m_ppu.CR.cr6 = all_gt | none_gt;
}
void PPULLVMRecompiler::VCTSXS(u32 vd, u32 uimm5, u32 vb) {
    int nScale = 1 << uimm5;

    for (uint w = 0; w < 4; w++) {
        float result = m_ppu.VPR[vb]._f[w] * nScale;

        if (result > INT_MAX)
            m_ppu.VPR[vd]._s32[w] = (int)INT_MAX;
        else if (result < INT_MIN)
            m_ppu.VPR[vd]._s32[w] = (int)INT_MIN;
        else // C rounding = Round towards 0
            m_ppu.VPR[vd]._s32[w] = (int)result;
    }
}
void PPULLVMRecompiler::VCTUXS(u32 vd, u32 uimm5, u32 vb) {
    int nScale = 1 << uimm5;

    for (uint w = 0; w < 4; w++) {
        // C rounding = Round towards 0
        s64 result = (s64)(m_ppu.VPR[vb]._f[w] * nScale);

        if (result > UINT_MAX)
            m_ppu.VPR[vd]._u32[w] = (u32)UINT_MAX;
        else if (result < 0)
            m_ppu.VPR[vd]._u32[w] = 0;
        else
            m_ppu.VPR[vd]._u32[w] = (u32)result;
    }
}
void PPULLVMRecompiler::VEXPTEFP(u32 vd, u32 vb) {
    // vd = exp(vb * log(2))
    // ISA : Note that the value placed into the element of vD may vary between implementations
    // and between different executions on the same implementation.
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = exp(m_ppu.VPR[vb]._f[w] * log(2.0f));
    }
}
void PPULLVMRecompiler::VLOGEFP(u32 vd, u32 vb) {
    // ISA : Note that the value placed into the element of vD may vary between implementations
    // and between different executions on the same implementation.
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = log(m_ppu.VPR[vb]._f[w]) / log(2.0f);
    }
}
void PPULLVMRecompiler::VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = m_ppu.VPR[va]._f[w] * m_ppu.VPR[vc]._f[w] + m_ppu.VPR[vb]._f[w];
    }
}
void PPULLVMRecompiler::VMAXFP(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = std::max(m_ppu.VPR[va]._f[w], m_ppu.VPR[vb]._f[w]);
    }
}
void PPULLVMRecompiler::VMAXSB(u32 vd, u32 va, u32 vb) //nf
{
    for (uint b = 0; b < 16; b++)
        m_ppu.VPR[vd]._s8[b] = std::max(m_ppu.VPR[va]._s8[b], m_ppu.VPR[vb]._s8[b]);
}
void PPULLVMRecompiler::VMAXSH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._s16[h] = std::max(m_ppu.VPR[va]._s16[h], m_ppu.VPR[vb]._s16[h]);
    }
}
void PPULLVMRecompiler::VMAXSW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s32[w] = std::max(m_ppu.VPR[va]._s32[w], m_ppu.VPR[vb]._s32[w]);
    }
}
void PPULLVMRecompiler::VMAXUB(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 16; b++)
        m_ppu.VPR[vd]._u8[b] = std::max(m_ppu.VPR[va]._u8[b], m_ppu.VPR[vb]._u8[b]);
}
void PPULLVMRecompiler::VMAXUH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = std::max(m_ppu.VPR[va]._u16[h], m_ppu.VPR[vb]._u16[h]);
    }
}
void PPULLVMRecompiler::VMAXUW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = std::max(m_ppu.VPR[va]._u32[w], m_ppu.VPR[vb]._u32[w]);
    }
}
void PPULLVMRecompiler::VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    for (uint h = 0; h < 8; h++) {
        s32 result = (s32)m_ppu.VPR[va]._s16[h] * (s32)m_ppu.VPR[vb]._s16[h] + (s32)m_ppu.VPR[vc]._s16[h];

        if (result > INT16_MAX) {
            m_ppu.VPR[vd]._s16[h] = (s16)INT16_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < INT16_MIN) {
            m_ppu.VPR[vd]._s16[h] = (s16)INT16_MIN;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._s16[h] = (s16)result;
    }
}
void PPULLVMRecompiler::VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    for (uint h = 0; h < 8; h++) {
        s32 result = (s32)m_ppu.VPR[va]._s16[h] * (s32)m_ppu.VPR[vb]._s16[h] + (s32)m_ppu.VPR[vc]._s16[h] + 0x4000;

        if (result > INT16_MAX) {
            m_ppu.VPR[vd]._s16[h] = (s16)INT16_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < INT16_MIN) {
            m_ppu.VPR[vd]._s16[h] = (s16)INT16_MIN;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._s16[h] = (s16)result;
    }
}
void PPULLVMRecompiler::VMINFP(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = std::min(m_ppu.VPR[va]._f[w], m_ppu.VPR[vb]._f[w]);
    }
}
void PPULLVMRecompiler::VMINSB(u32 vd, u32 va, u32 vb) //nf
{
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._s8[b] = std::min(m_ppu.VPR[va]._s8[b], m_ppu.VPR[vb]._s8[b]);
    }
}
void PPULLVMRecompiler::VMINSH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._s16[h] = std::min(m_ppu.VPR[va]._s16[h], m_ppu.VPR[vb]._s16[h]);
    }
}
void PPULLVMRecompiler::VMINSW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s32[w] = std::min(m_ppu.VPR[va]._s32[w], m_ppu.VPR[vb]._s32[w]);
    }
}
void PPULLVMRecompiler::VMINUB(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = std::min(m_ppu.VPR[va]._u8[b], m_ppu.VPR[vb]._u8[b]);
    }
}
void PPULLVMRecompiler::VMINUH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = std::min(m_ppu.VPR[va]._u16[h], m_ppu.VPR[vb]._u16[h]);
    }
}
void PPULLVMRecompiler::VMINUW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = std::min(m_ppu.VPR[va]._u32[w], m_ppu.VPR[vb]._u32[w]);
    }
}
void PPULLVMRecompiler::VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = m_ppu.VPR[va]._u16[h] * m_ppu.VPR[vb]._u16[h] + m_ppu.VPR[vc]._u16[h];
    }
}
void PPULLVMRecompiler::VMRGHB(u32 vd, u32 va, u32 vb) {
    VPR_reg VA = m_ppu.VPR[va];
    VPR_reg VB = m_ppu.VPR[vb];
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u8[15 - h * 2] = VA._u8[15 - h];
        m_ppu.VPR[vd]._u8[15 - h * 2 - 1] = VB._u8[15 - h];
    }
}
void PPULLVMRecompiler::VMRGHH(u32 vd, u32 va, u32 vb) {
    VPR_reg VA = m_ppu.VPR[va];
    VPR_reg VB = m_ppu.VPR[vb];
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u16[7 - w * 2] = VA._u16[7 - w];
        m_ppu.VPR[vd]._u16[7 - w * 2 - 1] = VB._u16[7 - w];
    }
}
void PPULLVMRecompiler::VMRGHW(u32 vd, u32 va, u32 vb) {
    VPR_reg VA = m_ppu.VPR[va];
    VPR_reg VB = m_ppu.VPR[vb];
    for (uint d = 0; d < 2; d++) {
        m_ppu.VPR[vd]._u32[3 - d * 2] = VA._u32[3 - d];
        m_ppu.VPR[vd]._u32[3 - d * 2 - 1] = VB._u32[3 - d];
    }
}
void PPULLVMRecompiler::VMRGLB(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u8[15 - h * 2] = m_ppu.VPR[va]._u8[7 - h];
        m_ppu.VPR[vd]._u8[15 - h * 2 - 1] = m_ppu.VPR[vb]._u8[7 - h];
    }
}
void PPULLVMRecompiler::VMRGLH(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u16[7 - w * 2] = m_ppu.VPR[va]._u16[3 - w];
        m_ppu.VPR[vd]._u16[7 - w * 2 - 1] = m_ppu.VPR[vb]._u16[3 - w];
    }
}
void PPULLVMRecompiler::VMRGLW(u32 vd, u32 va, u32 vb) {
    for (uint d = 0; d < 2; d++) {
        m_ppu.VPR[vd]._u32[3 - d * 2] = m_ppu.VPR[va]._u32[1 - d];
        m_ppu.VPR[vd]._u32[3 - d * 2 - 1] = m_ppu.VPR[vb]._u32[1 - d];
    }
}
void PPULLVMRecompiler::VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) //nf
{
    for (uint w = 0; w < 4; w++) {
        s32 result = 0;

        for (uint b = 0; b < 4; b++) {
            result += m_ppu.VPR[va]._s8[w * 4 + b] * m_ppu.VPR[vb]._u8[w * 4 + b];
        }

        result += m_ppu.VPR[vc]._s32[w];
        m_ppu.VPR[vd]._s32[w] = result;
    }
}
void PPULLVMRecompiler::VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) //nf
{
    for (uint w = 0; w < 4; w++) {
        s32 result = 0;

        for (uint h = 0; h < 2; h++) {
            result += m_ppu.VPR[va]._s16[w * 2 + h] * m_ppu.VPR[vb]._s16[w * 2 + h];
        }

        result += m_ppu.VPR[vc]._s32[w];
        m_ppu.VPR[vd]._s32[w] = result;
    }
}
void PPULLVMRecompiler::VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) //nf
{
    for (uint w = 0; w < 4; w++) {
        s64 result = 0;
        s32 saturated = 0;

        for (uint h = 0; h < 2; h++) {
            result += m_ppu.VPR[va]._s16[w * 2 + h] * m_ppu.VPR[vb]._s16[w * 2 + h];
        }

        result += m_ppu.VPR[vc]._s32[w];

        if (result > INT_MAX) {
            saturated = INT_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < INT_MIN) {
            saturated = INT_MIN;
            m_ppu.VSCR.SAT = 1;
        } else
            saturated = (s32)result;

        m_ppu.VPR[vd]._s32[w] = saturated;
    }
}
void PPULLVMRecompiler::VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) {
    for (uint w = 0; w < 4; w++) {
        u32 result = 0;

        for (uint b = 0; b < 4; b++) {
            result += m_ppu.VPR[va]._u8[w * 4 + b] * m_ppu.VPR[vb]._u8[w * 4 + b];
        }

        result += m_ppu.VPR[vc]._u32[w];
        m_ppu.VPR[vd]._u32[w] = result;
    }
}
void PPULLVMRecompiler::VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) //nf
{
    for (uint w = 0; w < 4; w++) {
        u32 result = 0;

        for (uint h = 0; h < 2; h++) {
            result += m_ppu.VPR[va]._u16[w * 2 + h] * m_ppu.VPR[vb]._u16[w * 2 + h];
        }

        result += m_ppu.VPR[vc]._u32[w];
        m_ppu.VPR[vd]._u32[w] = result;
    }
}
void PPULLVMRecompiler::VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) //nf
{
    for (uint w = 0; w < 4; w++) {
        u64 result = 0;
        u32 saturated = 0;

        for (uint h = 0; h < 2; h++) {
            result += m_ppu.VPR[va]._u16[w * 2 + h] * m_ppu.VPR[vb]._u16[w * 2 + h];
        }

        result += m_ppu.VPR[vc]._u32[w];

        if (result > UINT_MAX) {
            saturated = UINT_MAX;
            m_ppu.VSCR.SAT = 1;
        } else
            saturated = (u32)result;

        m_ppu.VPR[vd]._u32[w] = saturated;
    }
}
void PPULLVMRecompiler::VMULESB(u32 vd, u32 va, u32 vb) //nf
{
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._s16[h] = (s16)m_ppu.VPR[va]._s8[h * 2 + 1] * (s16)m_ppu.VPR[vb]._s8[h * 2 + 1];
    }
}
void PPULLVMRecompiler::VMULESH(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s32[w] = (s32)m_ppu.VPR[va]._s16[w * 2 + 1] * (s32)m_ppu.VPR[vb]._s16[w * 2 + 1];
    }
}
void PPULLVMRecompiler::VMULEUB(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = (u16)m_ppu.VPR[va]._u8[h * 2 + 1] * (u16)m_ppu.VPR[vb]._u8[h * 2 + 1];
    }
}
void PPULLVMRecompiler::VMULEUH(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = (u32)m_ppu.VPR[va]._u16[w * 2 + 1] * (u32)m_ppu.VPR[vb]._u16[w * 2 + 1];
    }
}
void PPULLVMRecompiler::VMULOSB(u32 vd, u32 va, u32 vb) //nf
{
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._s16[h] = (s16)m_ppu.VPR[va]._s8[h * 2] * (s16)m_ppu.VPR[vb]._s8[h * 2];
    }
}
void PPULLVMRecompiler::VMULOSH(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s32[w] = (s32)m_ppu.VPR[va]._s16[w * 2] * (s32)m_ppu.VPR[vb]._s16[w * 2];
    }
}
void PPULLVMRecompiler::VMULOUB(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = (u16)m_ppu.VPR[va]._u8[h * 2] * (u16)m_ppu.VPR[vb]._u8[h * 2];
    }
}
void PPULLVMRecompiler::VMULOUH(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = (u32)m_ppu.VPR[va]._u16[w * 2] * (u32)m_ppu.VPR[vb]._u16[w * 2];
    }
}
void PPULLVMRecompiler::VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = -(m_ppu.VPR[va]._f[w] * m_ppu.VPR[vc]._f[w] - m_ppu.VPR[vb]._f[w]);
    }
}
void PPULLVMRecompiler::VNOR(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = ~(m_ppu.VPR[va]._u32[w] | m_ppu.VPR[vb]._u32[w]);
    }
}
void PPULLVMRecompiler::VOR(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._u32[w] | m_ppu.VPR[vb]._u32[w];
    }
}
void PPULLVMRecompiler::VPERM(u32 vd, u32 va, u32 vb, u32 vc) {
    u8 tmpSRC[32];
    memcpy(tmpSRC, m_ppu.VPR[vb]._u8, 16);
    memcpy(tmpSRC + 16, m_ppu.VPR[va]._u8, 16);

    for (uint b = 0; b < 16; b++) {
        u8 index = m_ppu.VPR[vc]._u8[b] & 0x1f;

        m_ppu.VPR[vd]._u8[b] = tmpSRC[0x1f - index];
    }
}
void PPULLVMRecompiler::VPKPX(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 4; h++) {
        u16 bb7 = m_ppu.VPR[vb]._u8[15 - (h * 4 + 0)] & 0x1;
        u16 bb8 = m_ppu.VPR[vb]._u8[15 - (h * 4 + 1)] >> 3;
        u16 bb16 = m_ppu.VPR[vb]._u8[15 - (h * 4 + 2)] >> 3;
        u16 bb24 = m_ppu.VPR[vb]._u8[15 - (h * 4 + 3)] >> 3;
        u16 ab7 = m_ppu.VPR[va]._u8[15 - (h * 4 + 0)] & 0x1;
        u16 ab8 = m_ppu.VPR[va]._u8[15 - (h * 4 + 1)] >> 3;
        u16 ab16 = m_ppu.VPR[va]._u8[15 - (h * 4 + 2)] >> 3;
        u16 ab24 = m_ppu.VPR[va]._u8[15 - (h * 4 + 3)] >> 3;

        m_ppu.VPR[vd]._u16[3 - h] = (bb7 << 15) | (bb8 << 10) | (bb16 << 5) | bb24;
        m_ppu.VPR[vd]._u16[4 + (3 - h)] = (ab7 << 15) | (ab8 << 10) | (ab16 << 5) | ab24;
    }
}
void PPULLVMRecompiler::VPKSHSS(u32 vd, u32 va, u32 vb) //nf
{
    for (uint b = 0; b < 8; b++) {
        s16 result = m_ppu.VPR[va]._s16[b];

        if (result > INT8_MAX) {
            result = INT8_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < INT8_MIN) {
            result = INT8_MIN;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._s8[b + 8] = result;

        result = m_ppu.VPR[vb]._s16[b];

        if (result > INT8_MAX) {
            result = INT8_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < INT8_MIN) {
            result = INT8_MIN;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._s8[b] = result;
    }
}
void PPULLVMRecompiler::VPKSHUS(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 8; b++) {
        s16 result = m_ppu.VPR[va]._s16[b];

        if (result > UINT8_MAX) {
            result = UINT8_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < 0) {
            result = 0;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._u8[b + 8] = result;

        result = m_ppu.VPR[vb]._s16[b];

        if (result > UINT8_MAX) {
            result = UINT8_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < 0) {
            result = 0;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._u8[b] = result;
    }
}
void PPULLVMRecompiler::VPKSWSS(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 4; h++) {
        s32 result = m_ppu.VPR[va]._s32[h];

        if (result > INT16_MAX) {
            result = INT16_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < INT16_MIN) {
            result = INT16_MIN;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._s16[h + 4] = result;

        result = m_ppu.VPR[vb]._s32[h];

        if (result > INT16_MAX) {
            result = INT16_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < INT16_MIN) {
            result = INT16_MIN;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._s16[h] = result;
    }
}
void PPULLVMRecompiler::VPKSWUS(u32 vd, u32 va, u32 vb) //nf
{
    for (uint h = 0; h < 4; h++) {
        s32 result = m_ppu.VPR[va]._s32[h];

        if (result > UINT16_MAX) {
            result = UINT16_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < 0) {
            result = 0;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._u16[h + 4] = result;

        result = m_ppu.VPR[vb]._s32[h];

        if (result > UINT16_MAX) {
            result = UINT16_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (result < 0) {
            result = 0;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._u16[h] = result;
    }
}
void PPULLVMRecompiler::VPKUHUM(u32 vd, u32 va, u32 vb) //nf
{
    for (uint b = 0; b < 8; b++) {
        m_ppu.VPR[vd]._u8[b + 8] = m_ppu.VPR[va]._u8[b * 2];
        m_ppu.VPR[vd]._u8[b] = m_ppu.VPR[vb]._u8[b * 2];
    }
}
void PPULLVMRecompiler::VPKUHUS(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 8; b++) {
        u16 result = m_ppu.VPR[va]._u16[b];

        if (result > UINT8_MAX) {
            result = UINT8_MAX;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._u8[b + 8] = result;

        result = m_ppu.VPR[vb]._u16[b];

        if (result > UINT8_MAX) {
            result = UINT8_MAX;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._u8[b] = result;
    }
}
void PPULLVMRecompiler::VPKUWUM(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 4; h++) {
        m_ppu.VPR[vd]._u16[h + 4] = m_ppu.VPR[va]._u16[h * 2];
        m_ppu.VPR[vd]._u16[h] = m_ppu.VPR[vb]._u16[h * 2];
    }
}
void PPULLVMRecompiler::VPKUWUS(u32 vd, u32 va, u32 vb) //nf
{
    for (uint h = 0; h < 4; h++) {
        u32 result = m_ppu.VPR[va]._u32[h];

        if (result > UINT16_MAX) {
            result = UINT16_MAX;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._u16[h + 4] = result;

        result = m_ppu.VPR[vb]._u32[h];

        if (result > UINT16_MAX) {
            result = UINT16_MAX;
            m_ppu.VSCR.SAT = 1;
        }

        m_ppu.VPR[vd]._u16[h] = result;
    }
}
void PPULLVMRecompiler::VREFP(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = 1.0f / m_ppu.VPR[vb]._f[w];
    }
}
void PPULLVMRecompiler::VRFIM(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = floor(m_ppu.VPR[vb]._f[w]);
    }
}
void PPULLVMRecompiler::VRFIN(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = floor(m_ppu.VPR[vb]._f[w] + 0.5f);
    }
}
void PPULLVMRecompiler::VRFIP(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = ceil(m_ppu.VPR[vb]._f[w]);
    }
}
void PPULLVMRecompiler::VRFIZ(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        float f;
        modff(m_ppu.VPR[vb]._f[w], &f);
        m_ppu.VPR[vd]._f[w] = f;
    }
}
void PPULLVMRecompiler::VRLB(u32 vd, u32 va, u32 vb) //nf
{
    for (uint b = 0; b < 16; b++) {
        int nRot = m_ppu.VPR[vb]._u8[b] & 0x7;

        m_ppu.VPR[vd]._u8[b] = (m_ppu.VPR[va]._u8[b] << nRot) | (m_ppu.VPR[va]._u8[b] >> (8 - nRot));
    }
}
void PPULLVMRecompiler::VRLH(u32 vd, u32 va, u32 vb) //nf
{
    //for (uint h = 0; h < 8; h++)
    //{
    //	m_ppu.VPR[vd]._u16[h] = rotl16(m_ppu.VPR[va]._u16[h], m_ppu.VPR[vb]._u8[h * 2] & 0xf);
    //}
}
void PPULLVMRecompiler::VRLW(u32 vd, u32 va, u32 vb) {
    //for (uint w = 0; w < 4; w++)
    //{
    //	m_ppu.VPR[vd]._u32[w] = rotl32(m_ppu.VPR[va]._u32[w], m_ppu.VPR[vb]._u8[w * 4] & 0x1f);
    //}
}
void PPULLVMRecompiler::VRSQRTEFP(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        //TODO: accurate div
        m_ppu.VPR[vd]._f[w] = 1.0f / sqrtf(m_ppu.VPR[vb]._f[w]);
    }
}
void PPULLVMRecompiler::VSEL(u32 vd, u32 va, u32 vb, u32 vc) {
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = (m_ppu.VPR[vb]._u8[b] & m_ppu.VPR[vc]._u8[b]) | (m_ppu.VPR[va]._u8[b] & (~m_ppu.VPR[vc]._u8[b]));
    }
}
void PPULLVMRecompiler::VSL(u32 vd, u32 va, u32 vb) //nf
{
    u8 sh = m_ppu.VPR[vb]._u8[0] & 0x7;

    u32 t = 1;

    for (uint b = 0; b < 16; b++) {
        t &= (m_ppu.VPR[vb]._u8[b] & 0x7) == sh;
    }

    if (t) {
        m_ppu.VPR[vd]._u8[0] = m_ppu.VPR[va]._u8[0] << sh;

        for (uint b = 1; b < 16; b++) {
            m_ppu.VPR[vd]._u8[b] = (m_ppu.VPR[va]._u8[b] << sh) | (m_ppu.VPR[va]._u8[b - 1] >> (8 - sh));
        }
    } else {
        //undefined
        m_ppu.VPR[vd]._u32[0] = 0xCDCDCDCD;
        m_ppu.VPR[vd]._u32[1] = 0xCDCDCDCD;
        m_ppu.VPR[vd]._u32[2] = 0xCDCDCDCD;
        m_ppu.VPR[vd]._u32[3] = 0xCDCDCDCD;
    }
}
void PPULLVMRecompiler::VSLB(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = m_ppu.VPR[va]._u8[b] << (m_ppu.VPR[vb]._u8[b] & 0x7);
    }
}
void PPULLVMRecompiler::VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) {
    u8 tmpSRC[32];
    memcpy(tmpSRC, m_ppu.VPR[vb]._u8, 16);
    memcpy(tmpSRC + 16, m_ppu.VPR[va]._u8, 16);

    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[15 - b] = tmpSRC[31 - (b + sh)];
    }
}
void PPULLVMRecompiler::VSLH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = m_ppu.VPR[va]._u16[h] << (m_ppu.VPR[vb]._u8[h * 2] & 0xf);
    }
}
void PPULLVMRecompiler::VSLO(u32 vd, u32 va, u32 vb) {
    u8 nShift = (m_ppu.VPR[vb]._u8[0] >> 3) & 0xf;

    m_ppu.VPR[vd].Clear();

    for (u8 b = 0; b < 16 - nShift; b++) {
        m_ppu.VPR[vd]._u8[15 - b] = m_ppu.VPR[va]._u8[15 - (b + nShift)];
    }
}
void PPULLVMRecompiler::VSLW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._u32[w] << (m_ppu.VPR[vb]._u32[w] & 0x1f);
    }
}
void PPULLVMRecompiler::VSPLTB(u32 vd, u32 uimm5, u32 vb) {
    u8 byte = m_ppu.VPR[vb]._u8[15 - uimm5];

    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = byte;
    }
}
void PPULLVMRecompiler::VSPLTH(u32 vd, u32 uimm5, u32 vb) {
    assert(uimm5 < 8);

    u16 hword = m_ppu.VPR[vb]._u16[7 - uimm5];

    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = hword;
    }
}
void PPULLVMRecompiler::VSPLTISB(u32 vd, s32 simm5) {
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = simm5;
    }
}
void PPULLVMRecompiler::VSPLTISH(u32 vd, s32 simm5) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = (s16)simm5;
    }
}
void PPULLVMRecompiler::VSPLTISW(u32 vd, s32 simm5) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = (s32)simm5;
    }
}
void PPULLVMRecompiler::VSPLTW(u32 vd, u32 uimm5, u32 vb) {
    assert(uimm5 < 4);

    u32 word = m_ppu.VPR[vb]._u32[3 - uimm5];

    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = word;
    }
}
void PPULLVMRecompiler::VSR(u32 vd, u32 va, u32 vb) //nf
{
    u8 sh = m_ppu.VPR[vb]._u8[0] & 0x7;
    u32 t = 1;

    for (uint b = 0; b < 16; b++) {
        t &= (m_ppu.VPR[vb]._u8[b] & 0x7) == sh;
    }

    if (t) {
        m_ppu.VPR[vd]._u8[15] = m_ppu.VPR[va]._u8[15] >> sh;

        for (uint b = 14; ~b; b--) {
            m_ppu.VPR[vd]._u8[b] = (m_ppu.VPR[va]._u8[b] >> sh) | (m_ppu.VPR[va]._u8[b + 1] << (8 - sh));
        }
    } else {
        //undefined
        m_ppu.VPR[vd]._u32[0] = 0xCDCDCDCD;
        m_ppu.VPR[vd]._u32[1] = 0xCDCDCDCD;
        m_ppu.VPR[vd]._u32[2] = 0xCDCDCDCD;
        m_ppu.VPR[vd]._u32[3] = 0xCDCDCDCD;
    }
}
void PPULLVMRecompiler::VSRAB(u32 vd, u32 va, u32 vb) //nf
{
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._s8[b] = m_ppu.VPR[va]._s8[b] >> (m_ppu.VPR[vb]._u8[b] & 0x7);
    }
}
void PPULLVMRecompiler::VSRAH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._s16[h] = m_ppu.VPR[va]._s16[h] >> (m_ppu.VPR[vb]._u8[h * 2] & 0xf);
    }
}
void PPULLVMRecompiler::VSRAW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s32[w] = m_ppu.VPR[va]._s32[w] >> (m_ppu.VPR[vb]._u8[w * 4] & 0x1f);
    }
}
void PPULLVMRecompiler::VSRB(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = m_ppu.VPR[va]._u8[b] >> (m_ppu.VPR[vb]._u8[b] & 0x7);
    }
}
void PPULLVMRecompiler::VSRH(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = m_ppu.VPR[va]._u16[h] >> (m_ppu.VPR[vb]._u8[h * 2] & 0xf);
    }
}
void PPULLVMRecompiler::VSRO(u32 vd, u32 va, u32 vb) {
    u8 nShift = (m_ppu.VPR[vb]._u8[0] >> 3) & 0xf;

    m_ppu.VPR[vd].Clear();

    for (u8 b = 0; b < 16 - nShift; b++) {
        m_ppu.VPR[vd]._u8[b] = m_ppu.VPR[va]._u8[b + nShift];
    }
}
void PPULLVMRecompiler::VSRW(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._u32[w] >> (m_ppu.VPR[vb]._u8[w * 4] & 0x1f);
    }
}
void PPULLVMRecompiler::VSUBCUW(u32 vd, u32 va, u32 vb) //nf
{
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._u32[w] < m_ppu.VPR[vb]._u32[w] ? 0 : 1;
    }
}
void PPULLVMRecompiler::VSUBFP(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._f[w] = m_ppu.VPR[va]._f[w] - m_ppu.VPR[vb]._f[w];
    }
}
void PPULLVMRecompiler::VSUBSBS(u32 vd, u32 va, u32 vb) //nf
{
    for (uint b = 0; b < 16; b++) {
        s16 result = (s16)m_ppu.VPR[va]._s8[b] - (s16)m_ppu.VPR[vb]._s8[b];

        if (result < INT8_MIN) {
            m_ppu.VPR[vd]._s8[b] = INT8_MIN;
            m_ppu.VSCR.SAT = 1;
        } else if (result > INT8_MAX) {
            m_ppu.VPR[vd]._s8[b] = INT8_MAX;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._s8[b] = (s8)result;
    }
}
void PPULLVMRecompiler::VSUBSHS(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        s32 result = (s32)m_ppu.VPR[va]._s16[h] - (s32)m_ppu.VPR[vb]._s16[h];

        if (result < INT16_MIN) {
            m_ppu.VPR[vd]._s16[h] = (s16)INT16_MIN;
            m_ppu.VSCR.SAT = 1;
        } else if (result > INT16_MAX) {
            m_ppu.VPR[vd]._s16[h] = (s16)INT16_MAX;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._s16[h] = (s16)result;
    }
}
void PPULLVMRecompiler::VSUBSWS(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        s64 result = (s64)m_ppu.VPR[va]._s32[w] - (s64)m_ppu.VPR[vb]._s32[w];

        if (result < INT32_MIN) {
            m_ppu.VPR[vd]._s32[w] = (s32)INT32_MIN;
            m_ppu.VSCR.SAT = 1;
        } else if (result > INT32_MAX) {
            m_ppu.VPR[vd]._s32[w] = (s32)INT32_MAX;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._s32[w] = (s32)result;
    }
}
void PPULLVMRecompiler::VSUBUBM(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 16; b++) {
        m_ppu.VPR[vd]._u8[b] = (u8)((m_ppu.VPR[va]._u8[b] - m_ppu.VPR[vb]._u8[b]) & 0xff);
    }
}
void PPULLVMRecompiler::VSUBUBS(u32 vd, u32 va, u32 vb) {
    for (uint b = 0; b < 16; b++) {
        s16 result = (s16)m_ppu.VPR[va]._u8[b] - (s16)m_ppu.VPR[vb]._u8[b];

        if (result < 0) {
            m_ppu.VPR[vd]._u8[b] = 0;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._u8[b] = (u8)result;
    }
}
void PPULLVMRecompiler::VSUBUHM(u32 vd, u32 va, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._u16[h] = m_ppu.VPR[va]._u16[h] - m_ppu.VPR[vb]._u16[h];
    }
}
void PPULLVMRecompiler::VSUBUHS(u32 vd, u32 va, u32 vb) //nf
{
    for (uint h = 0; h < 8; h++) {
        s32 result = (s32)m_ppu.VPR[va]._u16[h] - (s32)m_ppu.VPR[vb]._u16[h];

        if (result < 0) {
            m_ppu.VPR[vd]._u16[h] = 0;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._u16[h] = (u16)result;
    }
}
void PPULLVMRecompiler::VSUBUWM(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._u32[w] = m_ppu.VPR[va]._u32[w] - m_ppu.VPR[vb]._u32[w];
    }
}
void PPULLVMRecompiler::VSUBUWS(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        s64 result = (s64)m_ppu.VPR[va]._u32[w] - (s64)m_ppu.VPR[vb]._u32[w];

        if (result < 0) {
            m_ppu.VPR[vd]._u32[w] = 0;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._u32[w] = (u32)result;
    }
}
void PPULLVMRecompiler::VSUMSWS(u32 vd, u32 va, u32 vb) {
    m_ppu.VPR[vd].Clear();

    s64 sum = m_ppu.VPR[vb]._s32[3];

    for (uint w = 0; w < 4; w++) {
        sum += m_ppu.VPR[va]._s32[w];
    }

    if (sum > INT32_MAX) {
        m_ppu.VPR[vd]._s32[0] = (s32)INT32_MAX;
        m_ppu.VSCR.SAT = 1;
    } else if (sum < INT32_MIN) {
        m_ppu.VPR[vd]._s32[0] = (s32)INT32_MIN;
        m_ppu.VSCR.SAT = 1;
    } else
        m_ppu.VPR[vd]._s32[0] = (s32)sum;
}
void PPULLVMRecompiler::VSUM2SWS(u32 vd, u32 va, u32 vb) {
    for (uint n = 0; n < 2; n++) {
        s64 sum = (s64)m_ppu.VPR[va]._s32[n * 2] + m_ppu.VPR[va]._s32[n * 2 + 1] + m_ppu.VPR[vb]._s32[n * 2];

        if (sum > INT32_MAX) {
            m_ppu.VPR[vd]._s32[n * 2] = (s32)INT32_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (sum < INT32_MIN) {
            m_ppu.VPR[vd]._s32[n * 2] = (s32)INT32_MIN;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._s32[n * 2] = (s32)sum;
    }
    m_ppu.VPR[vd]._s32[1] = 0;
    m_ppu.VPR[vd]._s32[3] = 0;
}
void PPULLVMRecompiler::VSUM4SBS(u32 vd, u32 va, u32 vb) //nf
{
    for (uint w = 0; w < 4; w++) {
        s64 sum = m_ppu.VPR[vb]._s32[w];

        for (uint b = 0; b < 4; b++) {
            sum += m_ppu.VPR[va]._s8[w * 4 + b];
        }

        if (sum > INT32_MAX) {
            m_ppu.VPR[vd]._s32[w] = (s32)INT32_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (sum < INT32_MIN) {
            m_ppu.VPR[vd]._s32[w] = (s32)INT32_MIN;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._s32[w] = (s32)sum;
    }
}
void PPULLVMRecompiler::VSUM4SHS(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        s64 sum = m_ppu.VPR[vb]._s32[w];

        for (uint h = 0; h < 2; h++) {
            sum += m_ppu.VPR[va]._s16[w * 2 + h];
        }

        if (sum > INT32_MAX) {
            m_ppu.VPR[vd]._s32[w] = (s32)INT32_MAX;
            m_ppu.VSCR.SAT = 1;
        } else if (sum < INT32_MIN) {
            m_ppu.VPR[vd]._s32[w] = (s32)INT32_MIN;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._s32[w] = (s32)sum;
    }
}
void PPULLVMRecompiler::VSUM4UBS(u32 vd, u32 va, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        u64 sum = m_ppu.VPR[vb]._u32[w];

        for (uint b = 0; b < 4; b++) {
            sum += m_ppu.VPR[va]._u8[w * 4 + b];
        }

        if (sum > UINT32_MAX) {
            m_ppu.VPR[vd]._u32[w] = (u32)UINT32_MAX;
            m_ppu.VSCR.SAT = 1;
        } else
            m_ppu.VPR[vd]._u32[w] = (u32)sum;
    }
}
void PPULLVMRecompiler::VUPKHPX(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s8[(3 - w) * 4 + 3] = m_ppu.VPR[vb]._s8[w * 2 + 0] >> 7;  // signed shift sign extends
        m_ppu.VPR[vd]._u8[(3 - w) * 4 + 2] = (m_ppu.VPR[vb]._u8[w * 2 + 0] >> 2) & 0x1f;
        m_ppu.VPR[vd]._u8[(3 - w) * 4 + 1] = ((m_ppu.VPR[vb]._u8[w * 2 + 0] & 0x3) << 3) | ((m_ppu.VPR[vb]._u8[w * 2 + 1] >> 5) & 0x7);
        m_ppu.VPR[vd]._u8[(3 - w) * 4 + 0] = m_ppu.VPR[vb]._u8[w * 2 + 1] & 0x1f;
    }
}
void PPULLVMRecompiler::VUPKHSB(u32 vd, u32 vb) {
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._s16[h] = m_ppu.VPR[vb]._s8[h];
    }
}
void PPULLVMRecompiler::VUPKHSH(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s32[w] = m_ppu.VPR[vb]._s16[w];
    }
}
void PPULLVMRecompiler::VUPKLPX(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s8[(3 - w) * 4 + 3] = m_ppu.VPR[vb]._s8[8 + w * 2 + 0] >> 7;  // signed shift sign extends
        m_ppu.VPR[vd]._u8[(3 - w) * 4 + 2] = (m_ppu.VPR[vb]._u8[8 + w * 2 + 0] >> 2) & 0x1f;
        m_ppu.VPR[vd]._u8[(3 - w) * 4 + 1] = ((m_ppu.VPR[vb]._u8[8 + w * 2 + 0] & 0x3) << 3) | ((m_ppu.VPR[vb]._u8[8 + w * 2 + 1] >> 5) & 0x7);
        m_ppu.VPR[vd]._u8[(3 - w) * 4 + 0] = m_ppu.VPR[vb]._u8[8 + w * 2 + 1] & 0x1f;
    }
}
void PPULLVMRecompiler::VUPKLSB(u32 vd, u32 vb) //nf
{
    for (uint h = 0; h < 8; h++) {
        m_ppu.VPR[vd]._s16[h] = m_ppu.VPR[vb]._s8[8 + h];
    }
}
void PPULLVMRecompiler::VUPKLSH(u32 vd, u32 vb) {
    for (uint w = 0; w < 4; w++) {
        m_ppu.VPR[vd]._s32[w] = m_ppu.VPR[vb]._s16[4 + w];
    }
}
void PPULLVMRecompiler::VXOR(u32 vd, u32 va, u32 vb) {
    m_ppu.VPR[vd]._u32[0] = m_ppu.VPR[va]._u32[0] ^ m_ppu.VPR[vb]._u32[0];
    m_ppu.VPR[vd]._u32[1] = m_ppu.VPR[va]._u32[1] ^ m_ppu.VPR[vb]._u32[1];
    m_ppu.VPR[vd]._u32[2] = m_ppu.VPR[va]._u32[2] ^ m_ppu.VPR[vb]._u32[2];
    m_ppu.VPR[vd]._u32[3] = m_ppu.VPR[va]._u32[3] ^ m_ppu.VPR[vb]._u32[3];
}
void PPULLVMRecompiler::MULLI(u32 rd, u32 ra, s32 simm16) {
    m_ppu.GPR[rd] = (s64)m_ppu.GPR[ra] * simm16;
}
void PPULLVMRecompiler::SUBFIC(u32 rd, u32 ra, s32 simm16) {
    const u64 RA = m_ppu.GPR[ra];
    const u64 IMM = (s64)simm16;
    m_ppu.GPR[rd] = ~RA + IMM + 1;

    m_ppu.XER.CA = m_ppu.IsCarry(~RA, IMM, 1);
}
void PPULLVMRecompiler::CMPLI(u32 crfd, u32 l, u32 ra, u32 uimm16) {
    m_ppu.UpdateCRnU(l, crfd, m_ppu.GPR[ra], uimm16);
}
void PPULLVMRecompiler::CMPI(u32 crfd, u32 l, u32 ra, s32 simm16) {
    m_ppu.UpdateCRnS(l, crfd, m_ppu.GPR[ra], simm16);
}
void PPULLVMRecompiler::ADDIC(u32 rd, u32 ra, s32 simm16) {
    const u64 RA = m_ppu.GPR[ra];
    m_ppu.GPR[rd] = RA + simm16;
    m_ppu.XER.CA = m_ppu.IsCarry(RA, simm16);
}
void PPULLVMRecompiler::ADDIC_(u32 rd, u32 ra, s32 simm16) {
    const u64 RA = m_ppu.GPR[ra];
    m_ppu.GPR[rd] = RA + simm16;
    m_ppu.XER.CA = m_ppu.IsCarry(RA, simm16);
    m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::ADDI(u32 rd, u32 ra, s32 simm16) {
    auto rd_ptr  = m_ir_builder.CreateConstGEP2_32(m_gpr, 0, rd);
    auto imm_val = m_ir_builder.getInt64((int64_t)simm16);

    if (ra == 0) {
        m_ir_builder.CreateStore(imm_val, rd_ptr);
    } else {
        auto ra_ptr = m_ir_builder.CreateConstGEP2_32(m_gpr, 0, ra);
        auto ra_val = m_ir_builder.CreateLoad(ra_ptr);
        auto sum    = m_ir_builder.CreateAdd(ra_val, imm_val);
        m_ir_builder.CreateStore(sum, rd_ptr);
    }
}
void PPULLVMRecompiler::ADDIS(u32 rd, u32 ra, s32 simm16) {
    m_ppu.GPR[rd] = ra ? ((s64)m_ppu.GPR[ra] + (simm16 << 16)) : (simm16 << 16);
}
void PPULLVMRecompiler::BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) {
    //if (CheckCondition(bo, bi))
    //{
    //	const u64 nextLR = m_ppu.PC + 4;
    //	m_ppu.SetBranch(branchTarget((aa ? 0 : m_ppu.PC), bd), lk);
    //	if (lk) m_ppu.LR = nextLR;
    //}
}
void PPULLVMRecompiler::SC(u32 sc_code) {
    //switch (sc_code)
    //{
    //case 0x1: UNK(fmt::Format("HyperCall %d", m_ppu.GPR[0])); break;
    //case 0x2: SysCall(); break;
    //case 0x3:
    //	Emu.GetSFuncManager().StaticExecute(m_ppu.GPR[11]);
    //	if (Ini.HLELogging.GetValue())
    //	{
    //		LOG_NOTICE(PPU, "'%s' done with code[0x%llx]! #pc: 0x%llx",
    //			Emu.GetSFuncManager()[m_ppu.GPR[11]]->name, m_ppu.GPR[3], m_ppu.PC);
    //	}
    //	break;
    //case 0x4: m_ppu.FastStop(); break;
    //case 0x22: UNK("HyperCall LV1"); break;
    //default: UNK(fmt::Format("Unknown sc: %x", sc_code));
    //}
}
void PPULLVMRecompiler::B(s32 ll, u32 aa, u32 lk) {
    const u64 nextLR = m_ppu.PC + 4;
    m_ppu.SetBranch(branchTarget(aa ? 0 : m_ppu.PC, ll), lk);
    if (lk) m_ppu.LR = nextLR;
}
void PPULLVMRecompiler::MCRF(u32 crfd, u32 crfs) {
    m_ppu.SetCR(crfd, m_ppu.GetCR(crfs));
}
void PPULLVMRecompiler::BCLR(u32 bo, u32 bi, u32 bh, u32 lk) {
    //if (CheckCondition(bo, bi))
    //{
    //	const u64 nextLR = m_ppu.PC + 4;
    //	m_ppu.SetBranch(branchTarget(0, m_ppu.LR), true);
    //	if (lk) m_ppu.LR = nextLR;
    //}
}
void PPULLVMRecompiler::CRNOR(u32 crbd, u32 crba, u32 crbb) {
    const u8 v = 1 ^ (m_ppu.IsCR(crba) | m_ppu.IsCR(crbb));
    m_ppu.SetCRBit2(crbd, v & 0x1);
}
void PPULLVMRecompiler::CRANDC(u32 crbd, u32 crba, u32 crbb) {
    const u8 v = m_ppu.IsCR(crba) & (1 ^ m_ppu.IsCR(crbb));
    m_ppu.SetCRBit2(crbd, v & 0x1);
}
void PPULLVMRecompiler::ISYNC() {
    _mm_mfence();
}
void PPULLVMRecompiler::CRXOR(u32 crbd, u32 crba, u32 crbb) {
    const u8 v = m_ppu.IsCR(crba) ^ m_ppu.IsCR(crbb);
    m_ppu.SetCRBit2(crbd, v & 0x1);
}
void PPULLVMRecompiler::CRNAND(u32 crbd, u32 crba, u32 crbb) {
    const u8 v = 1 ^ (m_ppu.IsCR(crba) & m_ppu.IsCR(crbb));
    m_ppu.SetCRBit2(crbd, v & 0x1);
}
void PPULLVMRecompiler::CRAND(u32 crbd, u32 crba, u32 crbb) {
    const u8 v = m_ppu.IsCR(crba) & m_ppu.IsCR(crbb);
    m_ppu.SetCRBit2(crbd, v & 0x1);
}
void PPULLVMRecompiler::CREQV(u32 crbd, u32 crba, u32 crbb) {
    const u8 v = 1 ^ (m_ppu.IsCR(crba) ^ m_ppu.IsCR(crbb));
    m_ppu.SetCRBit2(crbd, v & 0x1);
}
void PPULLVMRecompiler::CRORC(u32 crbd, u32 crba, u32 crbb) {
    const u8 v = m_ppu.IsCR(crba) | (1 ^ m_ppu.IsCR(crbb));
    m_ppu.SetCRBit2(crbd, v & 0x1);
}
void PPULLVMRecompiler::CROR(u32 crbd, u32 crba, u32 crbb) {
    const u8 v = m_ppu.IsCR(crba) | m_ppu.IsCR(crbb);
    m_ppu.SetCRBit2(crbd, v & 0x1);
}
void PPULLVMRecompiler::BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) {
    if (bo & 0x10 || m_ppu.IsCR(bi) == (bo & 0x8)) {
        const u64 nextLR = m_ppu.PC + 4;
        m_ppu.SetBranch(branchTarget(0, m_ppu.CTR), true);
        if (lk) m_ppu.LR = nextLR;
    }
}
void PPULLVMRecompiler::RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    //const u64 mask = rotate_mask[32 + mb][32 + me];
    //m_ppu.GPR[ra] = (m_ppu.GPR[ra] & ~mask) | (rotl32(m_ppu.GPR[rs], sh) & mask);
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    //m_ppu.GPR[ra] = rotl32(m_ppu.GPR[rs], sh) & rotate_mask[32 + mb][32 + me];
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc) {
    //m_ppu.GPR[ra] = rotl32(m_ppu.GPR[rs], m_ppu.GPR[rb] & 0x1f) & rotate_mask[32 + mb][32 + me];
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::ORI(u32 ra, u32 rs, u32 uimm16) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] | uimm16;
}
void PPULLVMRecompiler::ORIS(u32 ra, u32 rs, u32 uimm16) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] | (uimm16 << 16);
}
void PPULLVMRecompiler::XORI(u32 ra, u32 rs, u32 uimm16) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] ^ uimm16;
}
void PPULLVMRecompiler::XORIS(u32 ra, u32 rs, u32 uimm16) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] ^ (uimm16 << 16);
}
void PPULLVMRecompiler::ANDI_(u32 ra, u32 rs, u32 uimm16) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] & uimm16;
    m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::ANDIS_(u32 ra, u32 rs, u32 uimm16) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] & (uimm16 << 16);
    m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    //m_ppu.GPR[ra] = rotl64(m_ppu.GPR[rs], sh) & rotate_mask[mb][63];
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) {
    //m_ppu.GPR[ra] = rotl64(m_ppu.GPR[rs], sh) & rotate_mask[0][me];
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    //m_ppu.GPR[ra] = rotl64(m_ppu.GPR[rs], sh) & rotate_mask[mb][63 - sh];
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    //const u64 mask = rotate_mask[mb][63 - sh];
    //m_ppu.GPR[ra] = (m_ppu.GPR[ra] & ~mask) | (rotl64(m_ppu.GPR[rs], sh) & mask);
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) {
    if (is_r) // rldcr
    {
        RLDICR(ra, rs, m_ppu.GPR[rb], m_eb, rc);
    } else // rldcl
    {
        RLDICL(ra, rs, m_ppu.GPR[rb], m_eb, rc);
    }
}
void PPULLVMRecompiler::CMP(u32 crfd, u32 l, u32 ra, u32 rb) {
    m_ppu.UpdateCRnS(l, crfd, m_ppu.GPR[ra], m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::TW(u32 to, u32 ra, u32 rb) {
    //s32 a = m_ppu.GPR[ra];
    //s32 b = m_ppu.GPR[rb];

    //if ((a < b && (to & 0x10)) ||
    //	(a > b && (to & 0x8)) ||
    //	(a == b && (to & 0x4)) ||
    //	((u32)a < (u32)b && (to & 0x2)) ||
    //	((u32)a >(u32)b && (to & 0x1)))
    //{
    //	UNK(fmt::Format("Trap! (tw %x, r%d, r%d)", to, ra, rb));
    //}
}
void PPULLVMRecompiler::LVSL(u32 vd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];

    static const u64 lvsl_values[0x10][2] =
    {
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

    m_ppu.VPR[vd]._u64[0] = lvsl_values[addr & 0xf][0];
    m_ppu.VPR[vd]._u64[1] = lvsl_values[addr & 0xf][1];
}
void PPULLVMRecompiler::LVEBX(u32 vd, u32 ra, u32 rb) {
    //const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    //m_ppu.VPR[vd].Clear();
    //m_ppu.VPR[vd]._u8[addr & 0xf] = Memory.Read8(addr);
    m_ppu.VPR[vd]._u128 = Memory.Read128((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~0xfULL);
}
void PPULLVMRecompiler::SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const u64 RA = m_ppu.GPR[ra];
    //const u64 RB = m_ppu.GPR[rb];
    //m_ppu.GPR[rd] = ~RA + RB + 1;
    //m_ppu.XER.CA = m_ppu.IsCarry(~RA, RB, 1);
    //if (oe) UNK("subfco");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const s64 RA = m_ppu.GPR[ra];
    //const s64 RB = m_ppu.GPR[rb];
    //m_ppu.GPR[rd] = RA + RB;
    //m_ppu.XER.CA = m_ppu.IsCarry(RA, RB);
    //if (oe) UNK("addco");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::MULHDU(u32 rd, u32 ra, u32 rb, bool rc) {
    m_ppu.GPR[rd] = __umulh(m_ppu.GPR[ra], m_ppu.GPR[rb]);
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::MULHWU(u32 rd, u32 ra, u32 rb, bool rc) {
    u32 a = m_ppu.GPR[ra];
    u32 b = m_ppu.GPR[rb];
    m_ppu.GPR[rd] = ((u64)a * (u64)b) >> 32;
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::MFOCRF(u32 a, u32 rd, u32 crm) {
    /*
    if(a)
    {
    u32 n = 0, count = 0;
    for(u32 i = 0; i < 8; ++i)
    {
    if(crm & (1 << i))
    {
    n = i;
    count++;
    }
    }

    if(count == 1)
    {
    //RD[32+4*n : 32+4*n+3] = CR[4*n : 4*n+3];
    u8 offset = n * 4;
    m_ppu.GPR[rd] = (m_ppu.GPR[rd] & ~(0xf << offset)) | ((u32)m_ppu.GetCR(7 - n) << offset);
    }
    else
    m_ppu.GPR[rd] = 0;
    }
    else
    {
    */
    m_ppu.GPR[rd] = m_ppu.CR.CR;
    //}
}
void PPULLVMRecompiler::LWARX(u32 rd, u32 ra, u32 rb) {
    m_ppu.R_ADDR = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    m_ppu.R_VALUE = (u32&)Memory[m_ppu.R_ADDR];
    m_ppu.GPR[rd] = re32((u32)m_ppu.R_VALUE);
}
void PPULLVMRecompiler::LDX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = Memory.Read64(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::LWZX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = Memory.Read32(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::SLW(u32 ra, u32 rs, u32 rb, bool rc) {
    //u32 n = m_ppu.GPR[rb] & 0x1f;
    //u32 r = rotl32((u32)m_ppu.GPR[rs], n);
    //u32 m = (m_ppu.GPR[rb] & 0x20) ? 0 : rotate_mask[32][63 - n];

    //m_ppu.GPR[ra] = r & m;

    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::CNTLZW(u32 ra, u32 rs, bool rc) {
    u32 i;
    for (i = 0; i < 32; i++) {
        if (m_ppu.GPR[rs] & (1ULL << (31 - i))) break;
    }

    m_ppu.GPR[ra] = i;

    if (rc) m_ppu.SetCRBit(CR_LT, false);
}
void PPULLVMRecompiler::SLD(u32 ra, u32 rs, u32 rb, bool rc) {
    //u32 n = m_ppu.GPR[rb] & 0x3f;
    //u64 r = rotl64(m_ppu.GPR[rs], n);
    //u64 m = (m_ppu.GPR[rb] & 0x40) ? 0 : rotate_mask[0][63 - n];

    //m_ppu.GPR[ra] = r & m;

    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::AND(u32 ra, u32 rs, u32 rb, bool rc) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] & m_ppu.GPR[rb];
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::CMPL(u32 crfd, u32 l, u32 ra, u32 rb) {
    m_ppu.UpdateCRnU(l, crfd, m_ppu.GPR[ra], m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::LVSR(u32 vd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];

    static const u64 lvsr_values[0x10][2] =
    {
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

    m_ppu.VPR[vd]._u64[0] = lvsr_values[addr & 0xf][0];
    m_ppu.VPR[vd]._u64[1] = lvsr_values[addr & 0xf][1];
}
void PPULLVMRecompiler::LVEHX(u32 vd, u32 ra, u32 rb) {
    //const u64 addr = (ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~1ULL;
    //m_ppu.VPR[vd].Clear();
    //(u16&)m_ppu.VPR[vd]._u8[addr & 0xf] = Memory.Read16(addr);
    m_ppu.VPR[vd]._u128 = Memory.Read128((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~0xfULL);
}
void PPULLVMRecompiler::SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //m_ppu.GPR[rd] = m_ppu.GPR[rb] - m_ppu.GPR[ra];
    //if (oe) UNK("subfo");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::LDUX(u32 rd, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    m_ppu.GPR[rd] = Memory.Read64(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::DCBST(u32 ra, u32 rb) {
    //UNK("dcbst", false);
    _mm_mfence();
}
void PPULLVMRecompiler::LWZUX(u32 rd, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    m_ppu.GPR[rd] = Memory.Read32(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::CNTLZD(u32 ra, u32 rs, bool rc) {
    u32 i;
    for (i = 0; i < 64; i++) {
        if (m_ppu.GPR[rs] & (1ULL << (63 - i))) break;
    }

    m_ppu.GPR[ra] = i;
    if (rc) m_ppu.SetCRBit(CR_LT, false);
}
void PPULLVMRecompiler::ANDC(u32 ra, u32 rs, u32 rb, bool rc) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] & ~m_ppu.GPR[rb];
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::TD(u32 to, u32 ra, u32 rb) {
    //UNK("td");
}
void PPULLVMRecompiler::LVEWX(u32 vd, u32 ra, u32 rb) {
    //const u64 addr = (ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~3ULL;
    //m_ppu.VPR[vd].Clear();
    //(u32&)m_ppu.VPR[vd]._u8[addr & 0xf] = Memory.Read32(addr);
    m_ppu.VPR[vd]._u128 = Memory.Read128((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~0xfULL);
}
void PPULLVMRecompiler::MULHD(u32 rd, u32 ra, u32 rb, bool rc) {
    m_ppu.GPR[rd] = __mulh(m_ppu.GPR[ra], m_ppu.GPR[rb]);
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::MULHW(u32 rd, u32 ra, u32 rb, bool rc) {
    s32 a = m_ppu.GPR[ra];
    s32 b = m_ppu.GPR[rb];
    m_ppu.GPR[rd] = ((s64)a * (s64)b) >> 32;
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::LDARX(u32 rd, u32 ra, u32 rb) {
    m_ppu.R_ADDR = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    m_ppu.R_VALUE = (u64&)Memory[m_ppu.R_ADDR];
    m_ppu.GPR[rd] = re64(m_ppu.R_VALUE);
}
void PPULLVMRecompiler::DCBF(u32 ra, u32 rb) {
    //UNK("dcbf", false);
    _mm_mfence();
}
void PPULLVMRecompiler::LBZX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = Memory.Read8(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::LVX(u32 vd, u32 ra, u32 rb) {
    m_ppu.VPR[vd]._u128 = Memory.Read128((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~0xfULL);
}
void PPULLVMRecompiler::NEG(u32 rd, u32 ra, u32 oe, bool rc) {
    //m_ppu.GPR[rd] = 0 - m_ppu.GPR[ra];
    //if (oe) UNK("nego");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::LBZUX(u32 rd, u32 ra, u32 rb) {
    //if(ra == 0 || ra == rd) throw "Bad instruction [LBZUX]";

    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    m_ppu.GPR[rd] = Memory.Read8(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::NOR(u32 ra, u32 rs, u32 rb, bool rc) {
    m_ppu.GPR[ra] = ~(m_ppu.GPR[rs] | m_ppu.GPR[rb]);
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::STVEBX(u32 vs, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;
    Memory.Write8(addr, m_ppu.VPR[vs]._u8[15 - eb]);
}
void PPULLVMRecompiler::SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const u64 RA = m_ppu.GPR[ra];
    //const u64 RB = m_ppu.GPR[rb];
    //m_ppu.GPR[rd] = ~RA + RB + m_ppu.XER.CA;
    //m_ppu.XER.CA = m_ppu.IsCarry(~RA, RB, m_ppu.XER.CA);
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
    //if (oe) UNK("subfeo");
}
void PPULLVMRecompiler::ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const u64 RA = m_ppu.GPR[ra];
    //const u64 RB = m_ppu.GPR[rb];
    //if (m_ppu.XER.CA)
    //{
    //	if (RA == ~0ULL) //-1
    //	{
    //		m_ppu.GPR[rd] = RB;
    //		m_ppu.XER.CA = 1;
    //	}
    //	else
    //	{
    //		m_ppu.GPR[rd] = RA + 1 + RB;
    //		m_ppu.XER.CA = m_ppu.IsCarry(RA + 1, RB);
    //	}
    //}
    //else
    //{
    //	m_ppu.GPR[rd] = RA + RB;
    //	m_ppu.XER.CA = m_ppu.IsCarry(RA, RB);
    //}
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
    //if (oe) UNK("addeo");
}
void PPULLVMRecompiler::MTOCRF(u32 l, u32 crm, u32 rs) {
    if (l) {
        u32 n = 0, count = 0;
        for (u32 i = 0; i < 8; ++i) {
            if (crm & (1 << i)) {
                n = i;
                count++;
            }
        }

        if (count == 1) {
            //CR[4*n : 4*n+3] = RS[32+4*n : 32+4*n+3];
            m_ppu.SetCR(7 - n, (m_ppu.GPR[rs] >> (4 * n)) & 0xf);
        } else
            m_ppu.CR.CR = 0;
    } else {
        for (u32 i = 0; i < 8; ++i) {
            if (crm & (1 << i)) {
                m_ppu.SetCR(7 - i, m_ppu.GPR[rs] & (0xf << (i * 4)));
            }
        }
    }
}
void PPULLVMRecompiler::STDX(u32 rs, u32 ra, u32 rb) {
    Memory.Write64((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]), m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::STWCX_(u32 rs, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];

    if (m_ppu.R_ADDR == addr) {
        m_ppu.SetCR_EQ(0, InterlockedCompareExchange((volatile u32*)(Memory + addr), re32((u32)m_ppu.GPR[rs]), (u32)m_ppu.R_VALUE) == (u32)m_ppu.R_VALUE);
    } else {
        m_ppu.SetCR_EQ(0, false);
    }
}
void PPULLVMRecompiler::STWX(u32 rs, u32 ra, u32 rb) {
    Memory.Write32((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]), m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::STVEHX(u32 vs, u32 ra, u32 rb) {
    const u64 addr = (ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~1ULL;
    const u8 eb = (addr & 0xf) >> 1;
    Memory.Write16(addr, m_ppu.VPR[vs]._u16[7 - eb]);
}
void PPULLVMRecompiler::STDUX(u32 rs, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    Memory.Write64(addr, m_ppu.GPR[rs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STWUX(u32 rs, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    Memory.Write32(addr, m_ppu.GPR[rs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STVEWX(u32 vs, u32 ra, u32 rb) {
    const u64 addr = (ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~3ULL;
    const u8 eb = (addr & 0xf) >> 2;
    Memory.Write32(addr, m_ppu.VPR[vs]._u32[3 - eb]);
}
void PPULLVMRecompiler::ADDZE(u32 rd, u32 ra, u32 oe, bool rc) {
    //const u64 RA = m_ppu.GPR[ra];
    //m_ppu.GPR[rd] = RA + m_ppu.XER.CA;
    //m_ppu.XER.CA = m_ppu.IsCarry(RA, m_ppu.XER.CA);
    //if (oe) LOG_WARNING(PPU, "addzeo");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) {
    //const u64 RA = m_ppu.GPR[ra];
    //m_ppu.GPR[rd] = ~RA + m_ppu.XER.CA;
    //m_ppu.XER.CA = m_ppu.IsCarry(~RA, m_ppu.XER.CA);
    //if (oe) LOG_WARNING(PPU, "subfzeo");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::STDCX_(u32 rs, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];

    if (m_ppu.R_ADDR == addr) {
        m_ppu.SetCR_EQ(0, InterlockedCompareExchange((volatile u64*)(Memory + addr), re64(m_ppu.GPR[rs]), m_ppu.R_VALUE) == m_ppu.R_VALUE);
    } else {
        m_ppu.SetCR_EQ(0, false);
    }
}
void PPULLVMRecompiler::STBX(u32 rs, u32 ra, u32 rb) {
    Memory.Write8((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]), m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::STVX(u32 vs, u32 ra, u32 rb) {
    Memory.Write128((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~0xfULL, m_ppu.VPR[vs]._u128);
}
void PPULLVMRecompiler::SUBFME(u32 rd, u32 ra, u32 oe, bool rc) {
    //const u64 RA = m_ppu.GPR[ra];
    //m_ppu.GPR[rd] = ~RA + m_ppu.XER.CA + ~0ULL;
    //m_ppu.XER.CA = m_ppu.IsCarry(~RA, m_ppu.XER.CA, ~0ULL);
    //if (oe) LOG_WARNING(PPU, "subfmeo");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //m_ppu.GPR[rd] = (s64)((s64)m_ppu.GPR[ra] * (s64)m_ppu.GPR[rb]);
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
    //if (oe) UNK("mulldo");
}
void PPULLVMRecompiler::ADDME(u32 rd, u32 ra, u32 oe, bool rc) {
    //const s64 RA = m_ppu.GPR[ra];
    //m_ppu.GPR[rd] = RA + m_ppu.XER.CA - 1;
    //m_ppu.XER.CA |= RA != 0;

    //if (oe) UNK("addmeo");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //m_ppu.GPR[rd] = (s64)((s64)(s32)m_ppu.GPR[ra] * (s64)(s32)m_ppu.GPR[rb]);
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
    //if (oe) UNK("mullwo");
}
void PPULLVMRecompiler::DCBTST(u32 ra, u32 rb, u32 th) {
    //UNK("dcbtst", false);
    _mm_mfence();
}
void PPULLVMRecompiler::STBUX(u32 rs, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    Memory.Write8(addr, m_ppu.GPR[rs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const u64 RA = m_ppu.GPR[ra];
    //const u64 RB = m_ppu.GPR[rb];
    //m_ppu.GPR[rd] = RA + RB;
    //if (oe) UNK("addo");
    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::DCBT(u32 ra, u32 rb, u32 th) {
    //UNK("dcbt", false);
    _mm_mfence();
}
void PPULLVMRecompiler::LHZX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = Memory.Read16(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::EQV(u32 ra, u32 rs, u32 rb, bool rc) {
    m_ppu.GPR[ra] = ~(m_ppu.GPR[rs] ^ m_ppu.GPR[rb]);
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::ECIWX(u32 rd, u32 ra, u32 rb) {
    //HACK!
    m_ppu.GPR[rd] = Memory.Read32(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::LHZUX(u32 rd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    m_ppu.GPR[rd] = Memory.Read16(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::XOR(u32 ra, u32 rs, u32 rb, bool rc) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] ^ m_ppu.GPR[rb];
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::MFSPR(u32 rd, u32 spr) {
    //m_ppu.GPR[rd] = GetRegBySPR(spr);
}
void PPULLVMRecompiler::LWAX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = (s64)(s32)Memory.Read32(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::DST(u32 ra, u32 rb, u32 strm, u32 t) {
    _mm_mfence();
}
void PPULLVMRecompiler::LHAX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = (s64)(s16)Memory.Read16(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::LVXL(u32 vd, u32 ra, u32 rb) {
    m_ppu.VPR[vd]._u128 = Memory.Read128((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~0xfULL);
}
void PPULLVMRecompiler::MFTB(u32 rd, u32 spr) {
    //const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);

    //switch (n)
    //{
    //case 0x10C: m_ppu.GPR[rd] = m_ppu.TB; break;
    //case 0x10D: m_ppu.GPR[rd] = m_ppu.TBH; break;
    //default: UNK(fmt::Format("mftb r%d, %d", rd, spr)); break;
    //}
}

void PPULLVMRecompiler::LWAUX(u32 rd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    m_ppu.GPR[rd] = (s64)(s32)Memory.Read32(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::DSTST(u32 ra, u32 rb, u32 strm, u32 t) {
    _mm_mfence();
}
void PPULLVMRecompiler::LHAUX(u32 rd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    m_ppu.GPR[rd] = (s64)(s16)Memory.Read16(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STHX(u32 rs, u32 ra, u32 rb) {
    Memory.Write16(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb], m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::ORC(u32 ra, u32 rs, u32 rb, bool rc) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] | ~m_ppu.GPR[rb];
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::ECOWX(u32 rs, u32 ra, u32 rb) {
    //HACK!
    Memory.Write32((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]), m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::STHUX(u32 rs, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    Memory.Write16(addr, m_ppu.GPR[rs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::OR(u32 ra, u32 rs, u32 rb, bool rc) {
    m_ppu.GPR[ra] = m_ppu.GPR[rs] | m_ppu.GPR[rb];
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const u64 RA = m_ppu.GPR[ra];
    //const u64 RB = m_ppu.GPR[rb];

    //if (RB == 0)
    //{
    //	if (oe) UNK("divduo");
    //	m_ppu.GPR[rd] = 0;
    //}
    //else
    //{
    //	m_ppu.GPR[rd] = RA / RB;
    //}

    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const u32 RA = m_ppu.GPR[ra];
    //const u32 RB = m_ppu.GPR[rb];

    //if (RB == 0)
    //{
    //	if (oe) UNK("divwuo");
    //	m_ppu.GPR[rd] = 0;
    //}
    //else
    //{
    //	m_ppu.GPR[rd] = RA / RB;
    //}

    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::MTSPR(u32 spr, u32 rs) {
    //GetRegBySPR(spr) = m_ppu.GPR[rs];
}
/*0x1d6*///DCBI
void PPULLVMRecompiler::NAND(u32 ra, u32 rs, u32 rb, bool rc) {
    m_ppu.GPR[ra] = ~(m_ppu.GPR[rs] & m_ppu.GPR[rb]);

    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::STVXL(u32 vs, u32 ra, u32 rb) {
    Memory.Write128((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]) & ~0xfULL, m_ppu.VPR[vs]._u128);
}
void PPULLVMRecompiler::DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const s64 RA = m_ppu.GPR[ra];
    //const s64 RB = m_ppu.GPR[rb];

    //if (RB == 0 || ((u64)RA == (1ULL << 63) && RB == -1))
    //{
    //	if (oe) UNK("divdo");
    //	m_ppu.GPR[rd] = /*(((u64)RA & (1ULL << 63)) && RB == 0) ? -1 :*/ 0;
    //}
    //else
    //{
    //	m_ppu.GPR[rd] = RA / RB;
    //}

    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    //const s32 RA = m_ppu.GPR[ra];
    //const s32 RB = m_ppu.GPR[rb];

    //if (RB == 0 || ((u32)RA == (1 << 31) && RB == -1))
    //{
    //	if (oe) UNK("divwo");
    //	m_ppu.GPR[rd] = /*(((u32)RA & (1 << 31)) && RB == 0) ? -1 :*/ 0;
    //}
    //else
    //{
    //	m_ppu.GPR[rd] = (u32)(RA / RB);
    //}

    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[rd]);
}
void PPULLVMRecompiler::LVLX(u32 vd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;

    Memory.ReadLeft(m_ppu.VPR[vd]._u8 + eb, addr, 16 - eb);
}
void PPULLVMRecompiler::LDBRX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = (u64&)Memory[ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]];
}
void PPULLVMRecompiler::LSWX(u32 rd, u32 ra, u32 rb) {
    //UNK("lswx");
}
void PPULLVMRecompiler::LWBRX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = (u32&)Memory[ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]];
}
void PPULLVMRecompiler::LFSX(u32 frd, u32 ra, u32 rb) {
    (u32&)m_ppu.FPR[frd] = Memory.Read32(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
    m_ppu.FPR[frd] = (float&)m_ppu.FPR[frd];
}
void PPULLVMRecompiler::SRW(u32 ra, u32 rs, u32 rb, bool rc) {
    //u32 n = m_ppu.GPR[rb] & 0x1f;
    //u32 r = rotl32((u32)m_ppu.GPR[rs], 64 - n);
    //u32 m = (m_ppu.GPR[rb] & 0x20) ? 0 : rotate_mask[32 + n][63];
    //m_ppu.GPR[ra] = r & m;

    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::SRD(u32 ra, u32 rs, u32 rb, bool rc) {
    //u32 n = m_ppu.GPR[rb] & 0x3f;
    //u64 r = rotl64(m_ppu.GPR[rs], 64 - n);
    //u64 m = (m_ppu.GPR[rb] & 0x40) ? 0 : rotate_mask[n][63];
    //m_ppu.GPR[ra] = r & m;

    //if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::LVRX(u32 vd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;

    Memory.ReadRight(m_ppu.VPR[vd]._u8, addr & ~0xf, eb);
}
void PPULLVMRecompiler::LSWI(u32 rd, u32 ra, u32 nb) {
    u64 EA = ra ? m_ppu.GPR[ra] : 0;
    u64 N = nb ? nb : 32;
    u8 reg = m_ppu.GPR[rd];

    while (N > 0) {
        if (N > 3) {
            m_ppu.GPR[reg] = Memory.Read32(EA);
            EA += 4;
            N -= 4;
        } else {
            u32 buf = 0;
            while (N > 0) {
                N = N - 1;
                buf |= Memory.Read8(EA) << (N * 8);
                EA = EA + 1;
            }
            m_ppu.GPR[reg] = buf;
        }
        reg = (reg + 1) % 32;
    }
}
void PPULLVMRecompiler::LFSUX(u32 frd, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    (u64&)m_ppu.FPR[frd] = Memory.Read32(addr);
    m_ppu.FPR[frd] = (float&)m_ppu.FPR[frd];
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::SYNC(u32 l) {
    _mm_mfence();
}
void PPULLVMRecompiler::LFDX(u32 frd, u32 ra, u32 rb) {
    (u64&)m_ppu.FPR[frd] = Memory.Read64(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]);
}
void PPULLVMRecompiler::LFDUX(u32 frd, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    (u64&)m_ppu.FPR[frd] = Memory.Read64(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STVLX(u32 vs, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;

    Memory.WriteLeft(addr, 16 - eb, m_ppu.VPR[vs]._u8 + eb);
}
void PPULLVMRecompiler::STSWX(u32 rs, u32 ra, u32 rb) {
    //UNK("stwsx");
}
void PPULLVMRecompiler::STWBRX(u32 rs, u32 ra, u32 rb) {
    (u32&)Memory[ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]] = m_ppu.GPR[rs];
}
void PPULLVMRecompiler::STFSX(u32 frs, u32 ra, u32 rb) {
    Memory.Write32((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]), m_ppu.FPR[frs].To32());
}
void PPULLVMRecompiler::STVRX(u32 vs, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;

    Memory.WriteRight(addr - eb, eb, m_ppu.VPR[vs]._u8);
}
void PPULLVMRecompiler::STFSUX(u32 frs, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    Memory.Write32(addr, m_ppu.FPR[frs].To32());
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STSWI(u32 rd, u32 ra, u32 nb) {
    u64 EA = ra ? m_ppu.GPR[ra] : 0;
    u64 N = nb ? nb : 32;
    u8 reg = m_ppu.GPR[rd];

    while (N > 0) {
        if (N > 3) {
            Memory.Write32(EA, m_ppu.GPR[reg]);
            EA += 4;
            N -= 4;
        } else {
            u32 buf = m_ppu.GPR[reg];
            while (N > 0) {
                N = N - 1;
                Memory.Write8(EA, (0xFF000000 & buf) >> 24);
                buf <<= 8;
                EA = EA + 1;
            }
        }
        reg = (reg + 1) % 32;
    }
}
void PPULLVMRecompiler::STFDX(u32 frs, u32 ra, u32 rb) {
    Memory.Write64((ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]), (u64&)m_ppu.FPR[frs]);
}
void PPULLVMRecompiler::STFDUX(u32 frs, u32 ra, u32 rb) {
    const u64 addr = m_ppu.GPR[ra] + m_ppu.GPR[rb];
    Memory.Write64(addr, (u64&)m_ppu.FPR[frs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::LVLXL(u32 vd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;

    Memory.ReadLeft(m_ppu.VPR[vd]._u8 + eb, addr, 16 - eb);
}
void PPULLVMRecompiler::LHBRX(u32 rd, u32 ra, u32 rb) {
    m_ppu.GPR[rd] = (u16&)Memory[ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]];
}
void PPULLVMRecompiler::SRAW(u32 ra, u32 rs, u32 rb, bool rc) {
    s32 RS = m_ppu.GPR[rs];
    u8 shift = m_ppu.GPR[rb] & 63;
    if (shift > 31) {
        m_ppu.GPR[ra] = 0 - (RS < 0);
        m_ppu.XER.CA = (RS < 0);
    } else {
        m_ppu.GPR[ra] = RS >> shift;
        m_ppu.XER.CA = (RS < 0) & ((m_ppu.GPR[ra] << shift) != RS);
    }

    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::SRAD(u32 ra, u32 rs, u32 rb, bool rc) {
    s64 RS = m_ppu.GPR[rs];
    u8 shift = m_ppu.GPR[rb] & 127;
    if (shift > 63) {
        m_ppu.GPR[ra] = 0 - (RS < 0);
        m_ppu.XER.CA = (RS < 0);
    } else {
        m_ppu.GPR[ra] = RS >> shift;
        m_ppu.XER.CA = (RS < 0) & ((m_ppu.GPR[ra] << shift) != RS);
    }

    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::LVRXL(u32 vd, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;

    Memory.ReadRight(m_ppu.VPR[vd]._u8, addr & ~0xf, eb);
}
void PPULLVMRecompiler::DSS(u32 strm, u32 a) {
    _mm_mfence();
}
void PPULLVMRecompiler::SRAWI(u32 ra, u32 rs, u32 sh, bool rc) {
    s32 RS = (u32)m_ppu.GPR[rs];
    m_ppu.GPR[ra] = RS >> sh;
    m_ppu.XER.CA = (RS < 0) & ((u32)(m_ppu.GPR[ra] << sh) != RS);

    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::SRADI1(u32 ra, u32 rs, u32 sh, bool rc) {
    s64 RS = m_ppu.GPR[rs];
    m_ppu.GPR[ra] = RS >> sh;
    m_ppu.XER.CA = (RS < 0) & ((m_ppu.GPR[ra] << sh) != RS);

    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::SRADI2(u32 ra, u32 rs, u32 sh, bool rc) {
    SRADI1(ra, rs, sh, rc);
}
void PPULLVMRecompiler::EIEIO() {
    _mm_mfence();
}
void PPULLVMRecompiler::STVLXL(u32 vs, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;

    Memory.WriteLeft(addr, 16 - eb, m_ppu.VPR[vs]._u8 + eb);
}
void PPULLVMRecompiler::STHBRX(u32 rs, u32 ra, u32 rb) {
    (u16&)Memory[ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb]] = m_ppu.GPR[rs];
}
void PPULLVMRecompiler::EXTSH(u32 ra, u32 rs, bool rc) {
    m_ppu.GPR[ra] = (s64)(s16)m_ppu.GPR[rs];
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::STVRXL(u32 vs, u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    const u8 eb = addr & 0xf;

    Memory.WriteRight(addr - eb, eb, m_ppu.VPR[vs]._u8);
}
void PPULLVMRecompiler::EXTSB(u32 ra, u32 rs, bool rc) {
    m_ppu.GPR[ra] = (s64)(s8)m_ppu.GPR[rs];
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::STFIWX(u32 frs, u32 ra, u32 rb) {
    Memory.Write32(ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb], (u32&)m_ppu.FPR[frs]);
}
void PPULLVMRecompiler::EXTSW(u32 ra, u32 rs, bool rc) {
    m_ppu.GPR[ra] = (s64)(s32)m_ppu.GPR[rs];
    if (rc) m_ppu.UpdateCR0<s64>(m_ppu.GPR[ra]);
}
void PPULLVMRecompiler::ICBI(u32 ra, u32 rs) {
    // Clear jit for the specified block?  Nothing to do in the interpreter.
}
void PPULLVMRecompiler::DCBZ(u32 ra, u32 rb) {
    const u64 addr = ra ? m_ppu.GPR[ra] + m_ppu.GPR[rb] : m_ppu.GPR[rb];
    u8 *const cache_line = Memory.GetMemFromAddr(addr & ~127);
    if (cache_line)
        memset(cache_line, 0, 128);
    _mm_mfence();
}
void PPULLVMRecompiler::LWZ(u32 rd, u32 ra, s32 d) {
    m_ppu.GPR[rd] = Memory.Read32(ra ? m_ppu.GPR[ra] + d : d);
}
void PPULLVMRecompiler::LWZU(u32 rd, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    m_ppu.GPR[rd] = Memory.Read32(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::LBZ(u32 rd, u32 ra, s32 d) {
    m_ppu.GPR[rd] = Memory.Read8(ra ? m_ppu.GPR[ra] + d : d);
}
void PPULLVMRecompiler::LBZU(u32 rd, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    m_ppu.GPR[rd] = Memory.Read8(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STW(u32 rs, u32 ra, s32 d) {
    Memory.Write32(ra ? m_ppu.GPR[ra] + d : d, m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::STWU(u32 rs, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    Memory.Write32(addr, m_ppu.GPR[rs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STB(u32 rs, u32 ra, s32 d) {
    Memory.Write8(ra ? m_ppu.GPR[ra] + d : d, m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::STBU(u32 rs, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    Memory.Write8(addr, m_ppu.GPR[rs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::LHZ(u32 rd, u32 ra, s32 d) {
    m_ppu.GPR[rd] = Memory.Read16(ra ? m_ppu.GPR[ra] + d : d);
}
void PPULLVMRecompiler::LHZU(u32 rd, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    m_ppu.GPR[rd] = Memory.Read16(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::LHA(u32 rd, u32 ra, s32 d) {
    m_ppu.GPR[rd] = (s64)(s16)Memory.Read16(ra ? m_ppu.GPR[ra] + d : d);
}
void PPULLVMRecompiler::LHAU(u32 rd, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    m_ppu.GPR[rd] = (s64)(s16)Memory.Read16(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STH(u32 rs, u32 ra, s32 d) {
    Memory.Write16(ra ? m_ppu.GPR[ra] + d : d, m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::STHU(u32 rs, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    Memory.Write16(addr, m_ppu.GPR[rs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::LMW(u32 rd, u32 ra, s32 d) {
    u64 addr = ra ? m_ppu.GPR[ra] + d : d;
    for (u32 i = rd; i < 32; ++i, addr += 4) {
        m_ppu.GPR[i] = Memory.Read32(addr);
    }
}
void PPULLVMRecompiler::STMW(u32 rs, u32 ra, s32 d) {
    u64 addr = ra ? m_ppu.GPR[ra] + d : d;
    for (u32 i = rs; i < 32; ++i, addr += 4) {
        Memory.Write32(addr, m_ppu.GPR[i]);
    }
}
void PPULLVMRecompiler::LFS(u32 frd, u32 ra, s32 d) {
    const u32 v = Memory.Read32(ra ? m_ppu.GPR[ra] + d : d);
    m_ppu.FPR[frd] = (float&)v;
}
void PPULLVMRecompiler::LFSU(u32 frd, u32 ra, s32 ds) {
    const u64 addr = m_ppu.GPR[ra] + ds;
    const u32 v = Memory.Read32(addr);
    m_ppu.FPR[frd] = (float&)v;
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::LFD(u32 frd, u32 ra, s32 d) {
    (u64&)m_ppu.FPR[frd] = Memory.Read64(ra ? m_ppu.GPR[ra] + d : d);
}
void PPULLVMRecompiler::LFDU(u32 frd, u32 ra, s32 ds) {
    const u64 addr = m_ppu.GPR[ra] + ds;
    (u64&)m_ppu.FPR[frd] = Memory.Read64(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STFS(u32 frs, u32 ra, s32 d) {
    Memory.Write32(ra ? m_ppu.GPR[ra] + d : d, m_ppu.FPR[frs].To32());
}
void PPULLVMRecompiler::STFSU(u32 frs, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    Memory.Write32(addr, m_ppu.FPR[frs].To32());
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::STFD(u32 frs, u32 ra, s32 d) {
    Memory.Write64(ra ? m_ppu.GPR[ra] + d : d, (u64&)m_ppu.FPR[frs]);
}
void PPULLVMRecompiler::STFDU(u32 frs, u32 ra, s32 d) {
    const u64 addr = m_ppu.GPR[ra] + d;
    Memory.Write64(addr, (u64&)m_ppu.FPR[frs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::LD(u32 rd, u32 ra, s32 ds) {
    m_ppu.GPR[rd] = Memory.Read64(ra ? m_ppu.GPR[ra] + ds : ds);
}
void PPULLVMRecompiler::LDU(u32 rd, u32 ra, s32 ds) {
    //if(ra == 0 || rt == ra) return;
    const u64 addr = m_ppu.GPR[ra] + ds;
    m_ppu.GPR[rd] = Memory.Read64(addr);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::LWA(u32 rd, u32 ra, s32 ds) {
    m_ppu.GPR[rd] = (s64)(s32)Memory.Read32(ra ? m_ppu.GPR[ra] + ds : ds);
}
void PPULLVMRecompiler::FDIVS(u32 frd, u32 fra, u32 frb, bool rc) {
    //if (FPRdouble::IsNaN(m_ppu.FPR[fra]))
    //{
    //	m_ppu.FPR[frd] = m_ppu.FPR[fra];
    //}
    //else if (FPRdouble::IsNaN(m_ppu.FPR[frb]))
    //{
    //	m_ppu.FPR[frd] = m_ppu.FPR[frb];
    //}
    //else
    //{
    //	if (m_ppu.FPR[frb] == 0.0)
    //	{
    //		if (m_ppu.FPR[fra] == 0.0)
    //		{
    //			m_ppu.FPSCR.VXZDZ = true;
    //			m_ppu.FPR[frd] = FPR_NAN;
    //		}
    //		else
    //		{
    //			m_ppu.FPR[frd] = (float)(m_ppu.FPR[fra] / m_ppu.FPR[frb]);
    //		}

    //		m_ppu.FPSCR.ZX = true;
    //	}
    //	else if (FPRdouble::IsINF(m_ppu.FPR[fra]) && FPRdouble::IsINF(m_ppu.FPR[frb]))
    //	{
    //		m_ppu.FPSCR.VXIDI = true;
    //		m_ppu.FPR[frd] = FPR_NAN;
    //	}
    //	else
    //	{
    //		m_ppu.FPR[frd] = (float)(m_ppu.FPR[fra] / m_ppu.FPR[frb]);
    //	}
    //}

    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fdivs.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FSUBS(u32 frd, u32 fra, u32 frb, bool rc) {
    //m_ppu.FPR[frd] = static_cast<float>(m_ppu.FPR[fra] - m_ppu.FPR[frb]);
    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fsubs.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FADDS(u32 frd, u32 fra, u32 frb, bool rc) {
    //m_ppu.FPR[frd] = static_cast<float>(m_ppu.FPR[fra] + m_ppu.FPR[frb]);
    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fadds.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FSQRTS(u32 frd, u32 frb, bool rc) {
    //m_ppu.FPR[frd] = static_cast<float>(sqrt(m_ppu.FPR[frb]));
    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fsqrts.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FRES(u32 frd, u32 frb, bool rc) {
    //if (m_ppu.FPR[frb] == 0.0)
    //{
    //	m_ppu.SetFPSCRException(FPSCR_ZX);
    //}
    //m_ppu.FPR[frd] = static_cast<float>(1.0 / m_ppu.FPR[frb]);
    //if (rc) UNK("fres.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FMULS(u32 frd, u32 fra, u32 frc, bool rc) {
    //m_ppu.FPR[frd] = static_cast<float>(m_ppu.FPR[fra] * m_ppu.FPR[frc]);
    //m_ppu.FPSCR.FI = 0;
    //m_ppu.FPSCR.FR = 0;
    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fmuls.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    //m_ppu.FPR[frd] = static_cast<float>(m_ppu.FPR[fra] * m_ppu.FPR[frc] + m_ppu.FPR[frb]);
    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fmadds.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    //m_ppu.FPR[frd] = static_cast<float>(m_ppu.FPR[fra] * m_ppu.FPR[frc] - m_ppu.FPR[frb]);
    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fmsubs.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    //m_ppu.FPR[frd] = static_cast<float>(-(m_ppu.FPR[fra] * m_ppu.FPR[frc] - m_ppu.FPR[frb]));
    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fnmsubs.");////m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    //m_ppu.FPR[frd] = static_cast<float>(-(m_ppu.FPR[fra] * m_ppu.FPR[frc] + m_ppu.FPR[frb]));
    //m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fnmadds.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::STD(u32 rs, u32 ra, s32 d) {
    Memory.Write64(ra ? m_ppu.GPR[ra] + d : d, m_ppu.GPR[rs]);
}
void PPULLVMRecompiler::STDU(u32 rs, u32 ra, s32 ds) {
    //if(ra == 0 || rs == ra) return;
    const u64 addr = m_ppu.GPR[ra] + ds;
    Memory.Write64(addr, m_ppu.GPR[rs]);
    m_ppu.GPR[ra] = addr;
}
void PPULLVMRecompiler::MTFSB1(u32 crbd, bool rc) {
    //u64 mask = (1ULL << crbd);
    //if ((crbd == 29) && !m_ppu.FPSCR.NI) LOG_WARNING(PPU, "Non-IEEE mode enabled");
    //m_ppu.FPSCR.FPSCR |= mask;

    //if (rc) UNIMPLEMENTED();
}
void PPULLVMRecompiler::MCRFS(u32 crbd, u32 crbs) {
    u64 mask = (1ULL << crbd);
    m_ppu.CR.CR &= ~mask;
    m_ppu.CR.CR |= m_ppu.FPSCR.FPSCR & mask;
}
void PPULLVMRecompiler::MTFSB0(u32 crbd, bool rc) {
    //u64 mask = (1ULL << crbd);
    //if ((crbd == 29) && !m_ppu.FPSCR.NI) LOG_WARNING(PPU, "Non-IEEE mode disabled");
    //m_ppu.FPSCR.FPSCR &= ~mask;

    //if (rc) UNIMPLEMENTED();
}
void PPULLVMRecompiler::MTFSFI(u32 crfd, u32 i, bool rc) {
    //u64 mask = (0x1ULL << crfd);

    //if (i)
    //{
    //	if ((crfd == 29) && !m_ppu.FPSCR.NI) LOG_WARNING(PPU, "Non-IEEE mode enabled");
    //	m_ppu.FPSCR.FPSCR |= mask;
    //}
    //else
    //{
    //	if ((crfd == 29) && m_ppu.FPSCR.NI) LOG_WARNING(PPU, "Non-IEEE mode disabled");
    //	m_ppu.FPSCR.FPSCR &= ~mask;
    //}

    //if (rc) UNIMPLEMENTED();
}
void PPULLVMRecompiler::MFFS(u32 frd, bool rc) {
    //(u64&)m_ppu.FPR[frd] = m_ppu.FPSCR.FPSCR;
    //if (rc) UNIMPLEMENTED();
}
void PPULLVMRecompiler::MTFSF(u32 flm, u32 frb, bool rc) {
    //u32 mask = 0;
    //for (u32 i = 0; i<8; ++i)
    //{
    //	if (flm & (1 << i)) mask |= 0xf << (i * 4);
    //}

    //const u32 oldNI = m_ppu.FPSCR.NI;
    //m_ppu.FPSCR.FPSCR = (m_ppu.FPSCR.FPSCR & ~mask) | ((u32&)m_ppu.FPR[frb] & mask);
    //if (m_ppu.FPSCR.NI != oldNI)
    //{
    //	if (oldNI)
    //		LOG_WARNING(PPU, "Non-IEEE mode disabled");
    //	else
    //		LOG_WARNING(PPU, "Non-IEEE mode enabled");
    //}
    //if (rc) UNK("mtfsf.");
}
void PPULLVMRecompiler::FCMPU(u32 crfd, u32 fra, u32 frb) {
    int cmp_res = FPRdouble::Cmp(m_ppu.FPR[fra], m_ppu.FPR[frb]);

    if (cmp_res == CR_SO) {
        if (FPRdouble::IsSNaN(m_ppu.FPR[fra]) || FPRdouble::IsSNaN(m_ppu.FPR[frb])) {
            m_ppu.SetFPSCRException(FPSCR_VXSNAN);
        }
    }

    m_ppu.FPSCR.FPRF = cmp_res;
    m_ppu.SetCR(crfd, cmp_res);
}
void PPULLVMRecompiler::FRSP(u32 frd, u32 frb, bool rc) {
    const double b = m_ppu.FPR[frb];
    double b0 = b;
    if (m_ppu.FPSCR.NI) {
        if (((u64&)b0 & DOUBLE_EXP) < 0x3800000000000000ULL) (u64&)b0 &= DOUBLE_SIGN;
    }
    const double r = static_cast<float>(b0);
    m_ppu.FPSCR.FR = fabs(r) > fabs(b);
    m_ppu.SetFPSCR_FI(b != r);
    m_ppu.FPSCR.FPRF = PPCdouble(r).GetType();
    m_ppu.FPR[frd] = r;
}
void PPULLVMRecompiler::FCTIW(u32 frd, u32 frb, bool rc) {
    //const double b = m_ppu.FPR[frb];
    //u32 r;
    //if (b > (double)0x7fffffff)
    //{
    //	r = 0x7fffffff;
    //	m_ppu.SetFPSCRException(FPSCR_VXCVI);
    //	m_ppu.FPSCR.FI = 0;
    //	m_ppu.FPSCR.FR = 0;
    //}
    //else if (b < -(double)0x80000000)
    //{
    //	r = 0x80000000;
    //	m_ppu.SetFPSCRException(FPSCR_VXCVI);
    //	m_ppu.FPSCR.FI = 0;
    //	m_ppu.FPSCR.FR = 0;
    //}
    //else
    //{
    //	s32 i = 0;
    //	switch (m_ppu.FPSCR.RN)
    //	{
    //	case FPSCR_RN_NEAR:
    //	{
    //		double t = b + 0.5;
    //		i = (s32)t;
    //		if (t - i < 0 || (t - i == 0 && b > 0)) i--;
    //		break;
    //	}
    //	case FPSCR_RN_ZERO:
    //		i = (s32)b;
    //		break;
    //	case FPSCR_RN_PINF:
    //		i = (s32)b;
    //		if (b - i > 0) i++;
    //		break;
    //	case FPSCR_RN_MINF:
    //		i = (s32)b;
    //		if (b - i < 0) i--;
    //		break;
    //	}
    //	r = (u32)i;
    //	double di = i;
    //	if (di == b)
    //	{
    //		m_ppu.SetFPSCR_FI(0);
    //		m_ppu.FPSCR.FR = 0;
    //	}
    //	else
    //	{
    //		m_ppu.SetFPSCR_FI(1);
    //		m_ppu.FPSCR.FR = fabs(di) > fabs(b);
    //	}
    //}

    //(u64&)m_ppu.FPR[frd] = 0xfff8000000000000ull | r;
    //if (r == 0 && ((u64&)b & DOUBLE_SIGN)) (u64&)m_ppu.FPR[frd] |= 0x100000000ull;

    //if (rc) UNK("fctiw.");
}
void PPULLVMRecompiler::FCTIWZ(u32 frd, u32 frb, bool rc) {
    const double b = m_ppu.FPR[frb];
    u32 value;
    if (b > (double)0x7fffffff) {
        value = 0x7fffffff;
        m_ppu.SetFPSCRException(FPSCR_VXCVI);
        m_ppu.FPSCR.FI = 0;
        m_ppu.FPSCR.FR = 0;
    } else if (b < -(double)0x80000000) {
        value = 0x80000000;
        m_ppu.SetFPSCRException(FPSCR_VXCVI);
        m_ppu.FPSCR.FI = 0;
        m_ppu.FPSCR.FR = 0;
    } else {
        s32 i = (s32)b;
        double di = i;
        if (di == b) {
            m_ppu.SetFPSCR_FI(0);
            m_ppu.FPSCR.FR = 0;
        } else {
            m_ppu.SetFPSCR_FI(1);
            m_ppu.FPSCR.FR = fabs(di) > fabs(b);
        }
        value = (u32)i;
    }

    (u64&)m_ppu.FPR[frd] = 0xfff8000000000000ull | value;
    if (value == 0 && ((u64&)b & DOUBLE_SIGN))
        (u64&)m_ppu.FPR[frd] |= 0x100000000ull;

    //if (rc) UNK("fctiwz.");
}
void PPULLVMRecompiler::FDIV(u32 frd, u32 fra, u32 frb, bool rc) {
    double res;

    if (FPRdouble::IsNaN(m_ppu.FPR[fra])) {
        res = m_ppu.FPR[fra];
    } else if (FPRdouble::IsNaN(m_ppu.FPR[frb])) {
        res = m_ppu.FPR[frb];
    } else {
        if (m_ppu.FPR[frb] == 0.0) {
            if (m_ppu.FPR[fra] == 0.0) {
                m_ppu.FPSCR.VXZDZ = 1;
                res = FPR_NAN;
            } else {
                res = m_ppu.FPR[fra] / m_ppu.FPR[frb];
            }

            m_ppu.SetFPSCRException(FPSCR_ZX);
        } else if (FPRdouble::IsINF(m_ppu.FPR[fra]) && FPRdouble::IsINF(m_ppu.FPR[frb])) {
            m_ppu.FPSCR.VXIDI = 1;
            res = FPR_NAN;
        } else {
            res = m_ppu.FPR[fra] / m_ppu.FPR[frb];
        }
    }

    m_ppu.FPR[frd] = res;
    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fdiv.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FSUB(u32 frd, u32 fra, u32 frb, bool rc) {
    m_ppu.FPR[frd] = m_ppu.FPR[fra] - m_ppu.FPR[frb];
    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fsub.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FADD(u32 frd, u32 fra, u32 frb, bool rc) {
    m_ppu.FPR[frd] = m_ppu.FPR[fra] + m_ppu.FPR[frb];
    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fadd.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FSQRT(u32 frd, u32 frb, bool rc) {
    m_ppu.FPR[frd] = sqrt(m_ppu.FPR[frb]);
    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fsqrt.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    m_ppu.FPR[frd] = m_ppu.FPR[fra] >= 0.0 ? m_ppu.FPR[frc] : m_ppu.FPR[frb];
    //if (rc) UNK("fsel.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FMUL(u32 frd, u32 fra, u32 frc, bool rc) {
    if ((FPRdouble::IsINF(m_ppu.FPR[fra]) && m_ppu.FPR[frc] == 0.0) || (FPRdouble::IsINF(m_ppu.FPR[frc]) && m_ppu.FPR[fra] == 0.0)) {
        m_ppu.SetFPSCRException(FPSCR_VXIMZ);
        m_ppu.FPR[frd] = FPR_NAN;
        m_ppu.FPSCR.FI = 0;
        m_ppu.FPSCR.FR = 0;
        m_ppu.FPSCR.FPRF = FPR_QNAN;
    } else {
        if (FPRdouble::IsSNaN(m_ppu.FPR[fra]) || FPRdouble::IsSNaN(m_ppu.FPR[frc])) {
            m_ppu.SetFPSCRException(FPSCR_VXSNAN);
        }

        m_ppu.FPR[frd] = m_ppu.FPR[fra] * m_ppu.FPR[frc];
        m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    }

    //if (rc) UNK("fmul.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FRSQRTE(u32 frd, u32 frb, bool rc) {
    if (m_ppu.FPR[frb] == 0.0) {
        m_ppu.SetFPSCRException(FPSCR_ZX);
    }
    m_ppu.FPR[frd] = static_cast<float>(1.0 / sqrt(m_ppu.FPR[frb]));
    //if (rc) UNK("frsqrte.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    m_ppu.FPR[frd] = m_ppu.FPR[fra] * m_ppu.FPR[frc] - m_ppu.FPR[frb];
    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fmsub.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    m_ppu.FPR[frd] = m_ppu.FPR[fra] * m_ppu.FPR[frc] + m_ppu.FPR[frb];
    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fmadd.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    m_ppu.FPR[frd] = -(m_ppu.FPR[fra] * m_ppu.FPR[frc] - m_ppu.FPR[frb]);
    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fnmsub.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    m_ppu.FPR[frd] = -(m_ppu.FPR[fra] * m_ppu.FPR[frc] + m_ppu.FPR[frb]);
    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fnmadd.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FCMPO(u32 crfd, u32 fra, u32 frb) {
    int cmp_res = FPRdouble::Cmp(m_ppu.FPR[fra], m_ppu.FPR[frb]);

    if (cmp_res == CR_SO) {
        if (FPRdouble::IsSNaN(m_ppu.FPR[fra]) || FPRdouble::IsSNaN(m_ppu.FPR[frb])) {
            m_ppu.SetFPSCRException(FPSCR_VXSNAN);
            if (!m_ppu.FPSCR.VE) m_ppu.SetFPSCRException(FPSCR_VXVC);
        } else {
            m_ppu.SetFPSCRException(FPSCR_VXVC);
        }

        m_ppu.FPSCR.FX = 1;
    }

    m_ppu.FPSCR.FPRF = cmp_res;
    m_ppu.SetCR(crfd, cmp_res);
}
void PPULLVMRecompiler::FNEG(u32 frd, u32 frb, bool rc) {
    m_ppu.FPR[frd] = -m_ppu.FPR[frb];
    //if (rc) UNK("fneg.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FMR(u32 frd, u32 frb, bool rc) {
    m_ppu.FPR[frd] = m_ppu.FPR[frb];
    //if (rc) UNK("fmr.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FNABS(u32 frd, u32 frb, bool rc) {
    m_ppu.FPR[frd] = -fabs(m_ppu.FPR[frb]);
    //if (rc) UNK("fnabs.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FABS(u32 frd, u32 frb, bool rc) {
    m_ppu.FPR[frd] = fabs(m_ppu.FPR[frb]);
    //if (rc) UNK("fabs.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}
void PPULLVMRecompiler::FCTID(u32 frd, u32 frb, bool rc) {
    const double b = m_ppu.FPR[frb];
    u64 r;
    if (b > (double)0x7fffffffffffffff) {
        r = 0x7fffffffffffffff;
        m_ppu.SetFPSCRException(FPSCR_VXCVI);
        m_ppu.FPSCR.FI = 0;
        m_ppu.FPSCR.FR = 0;
    } else if (b < -(double)0x8000000000000000) {
        r = 0x8000000000000000;
        m_ppu.SetFPSCRException(FPSCR_VXCVI);
        m_ppu.FPSCR.FI = 0;
        m_ppu.FPSCR.FR = 0;
    } else {
        s64 i = 0;
        switch (m_ppu.FPSCR.RN) {
        case FPSCR_RN_NEAR:
        {
            double t = b + 0.5;
            i = (s64)t;
            if (t - i < 0 || (t - i == 0 && b > 0)) i--;
            break;
        }
        case FPSCR_RN_ZERO:
            i = (s64)b;
            break;
        case FPSCR_RN_PINF:
            i = (s64)b;
            if (b - i > 0) i++;
            break;
        case FPSCR_RN_MINF:
            i = (s64)b;
            if (b - i < 0) i--;
            break;
        }
        r = (u64)i;
        double di = i;
        if (di == b) {
            m_ppu.SetFPSCR_FI(0);
            m_ppu.FPSCR.FR = 0;
        } else {
            m_ppu.SetFPSCR_FI(1);
            m_ppu.FPSCR.FR = fabs(di) > fabs(b);
        }
    }

    (u64&)m_ppu.FPR[frd] = 0xfff8000000000000ull | r;
    if (r == 0 && ((u64&)b & DOUBLE_SIGN)) (u64&)m_ppu.FPR[frd] |= 0x100000000ull;

    //if (rc) UNK("fctid.");
}
void PPULLVMRecompiler::FCTIDZ(u32 frd, u32 frb, bool rc) {
    const double b = m_ppu.FPR[frb];
    u64 r;
    if (b > (double)0x7fffffffffffffff) {
        r = 0x7fffffffffffffff;
        m_ppu.SetFPSCRException(FPSCR_VXCVI);
        m_ppu.FPSCR.FI = 0;
        m_ppu.FPSCR.FR = 0;
    } else if (b < -(double)0x8000000000000000) {
        r = 0x8000000000000000;
        m_ppu.SetFPSCRException(FPSCR_VXCVI);
        m_ppu.FPSCR.FI = 0;
        m_ppu.FPSCR.FR = 0;
    } else {
        s64 i = (s64)b;
        double di = i;
        if (di == b) {
            m_ppu.SetFPSCR_FI(0);
            m_ppu.FPSCR.FR = 0;
        } else {
            m_ppu.SetFPSCR_FI(1);
            m_ppu.FPSCR.FR = fabs(di) > fabs(b);
        }
        r = (u64)i;
    }

    (u64&)m_ppu.FPR[frd] = 0xfff8000000000000ull | r;
    if (r == 0 && ((u64&)b & DOUBLE_SIGN)) (u64&)m_ppu.FPR[frd] |= 0x100000000ull;

    //if (rc) UNK("fctidz.");
}
void PPULLVMRecompiler::FCFID(u32 frd, u32 frb, bool rc) {
    s64 bi = (s64&)m_ppu.FPR[frb];
    double bf = (double)bi;
    s64 bfi = (s64)bf;

    if (bi == bfi) {
        m_ppu.SetFPSCR_FI(0);
        m_ppu.FPSCR.FR = 0;
    } else {
        m_ppu.SetFPSCR_FI(1);
        m_ppu.FPSCR.FR = abs(bfi) > abs(bi);
    }

    m_ppu.FPR[frd] = bf;

    m_ppu.FPSCR.FPRF = m_ppu.FPR[frd].GetType();
    //if (rc) UNK("fcfid.");//m_ppu.UpdateCR1(m_ppu.FPR[frd]);
}

void PPULLVMRecompiler::UNK(const u32 code, const u32 opcode, const u32 gcode) {
    //UNK(fmt::Format("Unknown/Illegal opcode! (0x%08x : 0x%x : 0x%x)", code, opcode, gcode));
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

void PPULLVMRecompiler::SetVr(u32 vr, Value * val) {
    auto vr_i128_ptr = m_ir_builder.CreateConstGEP2_32(m_vpr, 0, vr);
    auto val_i128    = m_ir_builder.CreateBitCast(val, Type::getIntNTy(m_llvm_context, 128));
    m_ir_builder.CreateStore(val_i128, vr_i128_ptr);
}
