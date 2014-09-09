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

u32                                  PPULLVMRecompiler::s_num_instances;
std::map<u64, void *>                PPULLVMRecompiler::s_address_to_code_map;
std::mutex                           PPULLVMRecompiler::s_address_to_code_map_mutex;
std::mutex                           PPULLVMRecompiler::s_llvm_mutex;
LLVMContext                        * PPULLVMRecompiler::s_llvm_context;
IRBuilder<>                        * PPULLVMRecompiler::s_ir_builder;
llvm::Module                       * PPULLVMRecompiler::s_module;
Function                           * PPULLVMRecompiler::s_execute_this_call_fn;
MDNode                             * PPULLVMRecompiler::s_execute_this_call_fn_name_and_args_md_node;
ExecutionEngine                    * PPULLVMRecompiler::s_execution_engine;
LLVMDisasmContextRef                 PPULLVMRecompiler::s_disassembler;
Value                              * PPULLVMRecompiler::s_state_ptr;
std::chrono::duration<double>        PPULLVMRecompiler::s_compilation_time;
std::chrono::duration<double>        PPULLVMRecompiler::s_execution_time;
std::map<std::string, u64>           PPULLVMRecompiler::s_interpreter_invocation_stats;
std::list<std::function<void()> *>   PPULLVMRecompiler::s_this_call_ptrs_list;

PPULLVMRecompiler::PPULLVMRecompiler(PPUThread & ppu)
    : m_ppu(ppu)
    , m_decoder(this)
    , m_interpreter(ppu) {
    std::lock_guard<std::mutex> lock(s_llvm_mutex);

    s_num_instances++;
    if (!s_llvm_context) {
        InitializeNativeTarget();
        InitializeNativeTargetAsmPrinter();
        InitializeNativeTargetDisassembler();

        s_llvm_context = new LLVMContext();
        s_ir_builder   = new IRBuilder<>(*s_llvm_context);
        s_module       = new llvm::Module("Module", *s_llvm_context);

        s_execute_this_call_fn = cast<Function>(s_module->getOrInsertFunction("ExecuteThisCall", s_ir_builder->getVoidTy(), s_ir_builder->getInt64Ty()->getPointerTo(), nullptr));
        s_execute_this_call_fn->setCallingConv(CallingConv::X86_64_Win64);
        s_execute_this_call_fn_name_and_args_md_node = MDNode::get(*s_llvm_context, MDString::get(*s_llvm_context, "Function name and arguments"));

        EngineBuilder engine_builder(s_module);
        engine_builder.setMCPU(sys::getHostCPUName());
        engine_builder.setEngineKind(EngineKind::JIT);
        engine_builder.setOptLevel(CodeGenOpt::Default);
        s_execution_engine = engine_builder.create();

        s_execution_engine->addGlobalMapping(s_execute_this_call_fn, (void *)&PPULLVMRecompiler::ExecuteThisCall);

        s_disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);

        //RunAllTests();
    }
}

PPULLVMRecompiler::~PPULLVMRecompiler() {
    std::lock_guard<std::mutex> lock(s_llvm_mutex);

    s_num_instances--;
    if (s_num_instances == 0) {
        for (auto i = s_this_call_ptrs_list.begin(); i != s_this_call_ptrs_list.end(); i++) {
            delete *i;
        }

        s_this_call_ptrs_list.clear();

        std::string error;
        raw_fd_ostream log_file("PPULLVMRecompiler.log", error, sys::fs::F_Text);
        log_file << "Time spent compiling = " << s_compilation_time.count() << "s\n";
        log_file << "Time spent executing = " << s_execution_time.count() << "s\n\n";
        log_file << "Interpreter invocation stats:\n";
        for (auto i = s_interpreter_invocation_stats.begin(); i != s_interpreter_invocation_stats.end(); i++) {
            log_file << i->first << " = " << i->second << "\n";
        }

        log_file << "\nLLVM IR:\n";
        log_file << *s_module;

        s_address_to_code_map.clear();
        LLVMDisasmDispose(s_disassembler);
        delete s_execution_engine;
        delete s_ir_builder;
        delete s_llvm_context;
    }
}

u8 PPULLVMRecompiler::DecodeMemory(const u64 address) {
    void * compiled_code = nullptr;

    s_address_to_code_map_mutex.lock();
    auto i = s_address_to_code_map.find(address);
    s_address_to_code_map_mutex.unlock();

    if (i != s_address_to_code_map.end()) {
        compiled_code = i->second;
    }

    if (!compiled_code) {
        std::lock_guard<std::mutex> lock(s_llvm_mutex);

        auto function_name = fmt::Format("fn_0x%llX", address);
        auto function      = s_module->getFunction(function_name);
        if (!function) {
            std::chrono::high_resolution_clock::time_point compilation_start = std::chrono::high_resolution_clock::now();

            function = cast<Function>(s_module->getOrInsertFunction(function_name, s_ir_builder->getVoidTy(), s_ir_builder->getInt8Ty()->getPointerTo(), nullptr));
            function->setCallingConv(CallingConv::X86_64_Win64);
            s_state_ptr = function->arg_begin();
            s_state_ptr->setName("state");

            auto block = BasicBlock::Create(*s_llvm_context, "start", function);
            s_ir_builder->SetInsertPoint(block);

            u64 offset = 0;
            m_hit_branch_instruction = false;
            while (!m_hit_branch_instruction) {
                u32 instr = Memory.Read32(address + offset);
                m_decoder.Decode(instr);
                offset += 4;

                SetPc(s_ir_builder->getInt64(m_ppu.PC + offset));
            }

            s_ir_builder->CreateRetVoid();
            s_execution_engine->runJITOnFunction(function);

            compiled_code = s_execution_engine->getPointerToFunction(function);
            s_address_to_code_map_mutex.lock();
            s_address_to_code_map[address] = compiled_code;
            s_address_to_code_map_mutex.unlock();

            std::chrono::high_resolution_clock::time_point compilation_end = std::chrono::high_resolution_clock::now();
            s_compilation_time += std::chrono::duration_cast<std::chrono::duration<double>>(compilation_end - compilation_start);
        }
    }

    std::chrono::high_resolution_clock::time_point execution_start = std::chrono::high_resolution_clock::now();
    ((void(*)(PPUThread *))(compiled_code))(&m_ppu);
    std::chrono::high_resolution_clock::time_point execution_end = std::chrono::high_resolution_clock::now();
    s_execution_time += std::chrono::duration_cast<std::chrono::duration<double>>(execution_end - execution_start);
    return 0;
}

void PPULLVMRecompiler::NULL_OP() {
    ThisCall("NULL_OP", &PPUInterpreter::NULL_OP, &m_interpreter);
}

void PPULLVMRecompiler::NOP() {
    ThisCall("NOP", &PPUInterpreter::NOP, &m_interpreter);
}

void PPULLVMRecompiler::TDI(u32 to, u32 ra, s32 simm16) {
    ThisCall("TDI", &PPUInterpreter::TDI, &m_interpreter, to, ra, simm16);
}

void PPULLVMRecompiler::TWI(u32 to, u32 ra, s32 simm16) {
    ThisCall("TWI", &PPUInterpreter::TWI, &m_interpreter, to, ra, simm16);
}

void PPULLVMRecompiler::MFVSCR(u32 vd) {
    auto vscr_i32  = GetVscr();
    auto vscr_i128 = s_ir_builder->CreateZExt(vscr_i32, s_ir_builder->getIntNTy(128));
    SetVr(vd, vscr_i128);
}

void PPULLVMRecompiler::MTVSCR(u32 vb) {
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);
    auto vscr_i32 = s_ir_builder->CreateExtractElement(vb_v4i32, s_ir_builder->getInt32(0));
    vscr_i32      = s_ir_builder->CreateAnd(vscr_i32, 0x00010001);
    SetVscr(vscr_i32);
}

void PPULLVMRecompiler::VADDCUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32 = GetVrAsIntVec(va, 32);
    auto vb_v4i32 = GetVrAsIntVec(vb, 32);

    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    va_v4i32               = s_ir_builder->CreateXor(va_v4i32, ConstantDataVector::get(*s_llvm_context, not_mask_v4i32));
    auto cmpv4i1           = s_ir_builder->CreateICmpULT(va_v4i32, vb_v4i32);
    auto cmpv4i32          = s_ir_builder->CreateZExt(cmpv4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmpv4i32);

    // TODO: Implement with overflow intrinsics and check if the generated code is better
}

void PPULLVMRecompiler::VADDFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto sum_v4f32 = s_ir_builder->CreateFAdd(va_v4f32, vb_v4f32);
    SetVr(vd, sum_v4f32);
}

void PPULLVMRecompiler::VADDSBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse2_padds_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set VSCR.SAT
}

void PPULLVMRecompiler::VADDSHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse2_padds_w), va_v8i16, vb_v8i16);
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
    auto tmp2_v4i32    = s_ir_builder->CreateLShr(va_v4i32, 31);
    tmp2_v4i32         = s_ir_builder->CreateAdd(tmp2_v4i32, ConstantDataVector::get(*s_llvm_context, tmp1_v4i32));
    auto tmp2_v16i8    = s_ir_builder->CreateBitCast(tmp2_v4i32, VectorType::get(s_ir_builder->getInt8Ty(), 16));

    // Next, we find if the addition can actually result in an overflow. Since an overflow can only happen if the operands
    // have the same sign, we bitwise XOR both the operands. If the sign bit of the result is 0 then the operands have the
    // same sign and so may cause an overflow. We invert the result so that the sign bit is 1 when the operands have the
    // same sign.
    auto tmp3_v4i32        = s_ir_builder->CreateXor(va_v4i32, vb_v4i32);
    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    tmp3_v4i32             = s_ir_builder->CreateXor(tmp3_v4i32, ConstantDataVector::get(*s_llvm_context, not_mask_v4i32));

    // Perform the sum.
    auto sum_v4i32 = s_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    auto sum_v16i8 = s_ir_builder->CreateBitCast(sum_v4i32, VectorType::get(s_ir_builder->getInt8Ty(), 16));

    // If an overflow occurs, then the sign of the sum will be different from the sign of the operands. So, we xor the
    // result with one of the operands. The sign bit of the result will be 1 if the sign bit of the sum and the sign bit of the
    // result is different. This result is again ANDed with tmp3 (the sign bit of tmp3 is 1 only if the operands have the same
    // sign and so can cause an overflow).
    auto tmp4_v4i32 = s_ir_builder->CreateXor(va_v4i32, sum_v4i32);
    tmp4_v4i32      = s_ir_builder->CreateAnd(tmp3_v4i32, tmp4_v4i32);
    tmp4_v4i32      = s_ir_builder->CreateAShr(tmp4_v4i32, 31);
    auto tmp4_v16i8 = s_ir_builder->CreateBitCast(tmp4_v4i32, VectorType::get(s_ir_builder->getInt8Ty(), 16));

    // tmp4 is equal to 0xFFFFFFFF if an overflow occured and 0x00000000 otherwise.
    auto res_v16i8 = s_ir_builder->CreateCall3(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse41_pblendvb), sum_v16i8, tmp2_v16i8, tmp4_v16i8);
    SetVr(vd, res_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUBM(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = s_ir_builder->CreateAdd(va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);
}

void PPULLVMRecompiler::VADDUBS(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto sum_v16i8 = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse2_paddus_b), va_v16i8, vb_v16i8);
    SetVr(vd, sum_v16i8);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUHM(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = s_ir_builder->CreateAdd(va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);
}

void PPULLVMRecompiler::VADDUHS(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto sum_v8i16 = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse2_paddus_w), va_v8i16, vb_v8i16);
    SetVr(vd, sum_v8i16);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VADDUWM(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = s_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    SetVr(vd, sum_v4i32);
}

void PPULLVMRecompiler::VADDUWS(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto sum_v4i32 = s_ir_builder->CreateAdd(va_v4i32, vb_v4i32);
    auto cmp_v4i1  = s_ir_builder->CreateICmpULT(sum_v4i32, va_v4i32);
    auto cmp_v4i32 = s_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    auto res_v4i32 = s_ir_builder->CreateOr(sum_v4i32, cmp_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Set SAT
}

void PPULLVMRecompiler::VAND(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4i32 = s_ir_builder->CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VANDC(u32 vd, u32 va, u32 vb) {
    auto va_v4i32          = GetVrAsIntVec(va, 32);
    auto vb_v4i32          = GetVrAsIntVec(vb, 32);
    u32  not_mask_v4i32[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
    vb_v4i32               = s_ir_builder->CreateXor(vb_v4i32, ConstantDataVector::get(*s_llvm_context, not_mask_v4i32));
    auto res_v4i32         = s_ir_builder->CreateAnd(va_v4i32, vb_v4i32);
    SetVr(vd, res_v4i32);
}

void PPULLVMRecompiler::VAVGSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8       = GetVrAsIntVec(va, 8);
    auto vb_v16i8       = GetVrAsIntVec(vb, 8);
    auto va_v16i16      = s_ir_builder->CreateSExt(va_v16i8, VectorType::get(s_ir_builder->getInt16Ty(), 16));
    auto vb_v16i16      = s_ir_builder->CreateSExt(vb_v16i8, VectorType::get(s_ir_builder->getInt16Ty(), 16));
    auto sum_v16i16     = s_ir_builder->CreateAdd(va_v16i16, vb_v16i16);
    u16  one_v16i16[16] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    sum_v16i16          = s_ir_builder->CreateAdd(sum_v16i16, ConstantDataVector::get(*s_llvm_context, one_v16i16));
    auto avg_v16i16     = s_ir_builder->CreateAShr(sum_v16i16, 1);
    auto avg_v16i8      = s_ir_builder->CreateTrunc(avg_v16i16, VectorType::get(s_ir_builder->getInt8Ty(), 16));
    SetVr(vd, avg_v16i8);
}

void PPULLVMRecompiler::VAVGSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16     = GetVrAsIntVec(va, 16);
    auto vb_v8i16     = GetVrAsIntVec(vb, 16);
    auto va_v8i32     = s_ir_builder->CreateSExt(va_v8i16, VectorType::get(s_ir_builder->getInt32Ty(), 8));
    auto vb_v8i32     = s_ir_builder->CreateSExt(vb_v8i16, VectorType::get(s_ir_builder->getInt32Ty(), 8));
    auto sum_v8i32    = s_ir_builder->CreateAdd(va_v8i32, vb_v8i32);
    u32  one_v8i32[8] = {1, 1, 1, 1, 1, 1, 1, 1};
    sum_v8i32         = s_ir_builder->CreateAdd(sum_v8i32, ConstantDataVector::get(*s_llvm_context, one_v8i32));
    auto avg_v8i32    = s_ir_builder->CreateAShr(sum_v8i32, 1);
    auto avg_v8i16    = s_ir_builder->CreateTrunc(avg_v8i32, VectorType::get(s_ir_builder->getInt16Ty(), 8));
    SetVr(vd, avg_v8i16);
}

void PPULLVMRecompiler::VAVGSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32     = GetVrAsIntVec(va, 32);
    auto vb_v4i32     = GetVrAsIntVec(vb, 32);
    auto va_v4i64     = s_ir_builder->CreateSExt(va_v4i32, VectorType::get(s_ir_builder->getInt64Ty(), 4));
    auto vb_v4i64     = s_ir_builder->CreateSExt(vb_v4i32, VectorType::get(s_ir_builder->getInt64Ty(), 4));
    auto sum_v4i64    = s_ir_builder->CreateAdd(va_v4i64, vb_v4i64);
    u64  one_v4i64[4] = {1, 1, 1, 1};
    sum_v4i64         = s_ir_builder->CreateAdd(sum_v4i64, ConstantDataVector::get(*s_llvm_context, one_v4i64));
    auto avg_v4i64    = s_ir_builder->CreateAShr(sum_v4i64, 1);
    auto avg_v4i32    = s_ir_builder->CreateTrunc(avg_v4i64, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, avg_v4i32);
}

void PPULLVMRecompiler::VAVGUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto avg_v16i8 = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse2_pavg_b), va_v16i8, vb_v16i8);
    SetVr(vd, avg_v16i8);
}

void PPULLVMRecompiler::VAVGUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto avg_v8i16 = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse2_pavg_w), va_v8i16, vb_v8i16);
    SetVr(vd, avg_v8i16);
}

void PPULLVMRecompiler::VAVGUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32     = GetVrAsIntVec(va, 32);
    auto vb_v4i32     = GetVrAsIntVec(vb, 32);
    auto va_v4i64     = s_ir_builder->CreateZExt(va_v4i32, VectorType::get(s_ir_builder->getInt64Ty(), 4));
    auto vb_v4i64     = s_ir_builder->CreateZExt(vb_v4i32, VectorType::get(s_ir_builder->getInt64Ty(), 4));
    auto sum_v4i64    = s_ir_builder->CreateAdd(va_v4i64, vb_v4i64);
    u64  one_v4i64[4] = {1, 1, 1, 1};
    sum_v4i64         = s_ir_builder->CreateAdd(sum_v4i64, ConstantDataVector::get(*s_llvm_context, one_v4i64));
    auto avg_v4i64    = s_ir_builder->CreateLShr(sum_v4i64, 1);
    auto avg_v4i32    = s_ir_builder->CreateTrunc(avg_v4i64, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, avg_v4i32);
}

void PPULLVMRecompiler::VCFSX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = s_ir_builder->CreateSIToFP(vb_v4i32, VectorType::get(s_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale          = (float)(1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = s_ir_builder->CreateFDiv(res_v4f32, ConstantDataVector::get(*s_llvm_context, scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VCFUX(u32 vd, u32 uimm5, u32 vb) {
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto res_v4f32 = s_ir_builder->CreateUIToFP(vb_v4i32, VectorType::get(s_ir_builder->getFloatTy(), 4));

    if (uimm5) {
        float scale          = (float)(1 << uimm5);
        float scale_v4f32[4] = {scale, scale, scale, scale};
        res_v4f32            = s_ir_builder->CreateFDiv(res_v4f32, ConstantDataVector::get(*s_llvm_context, scale_v4f32));
    }

    SetVr(vd, res_v4f32);
}

void PPULLVMRecompiler::VCMPBFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32     = GetVrAsFloatVec(va);
    auto vb_v4f32     = GetVrAsFloatVec(vb);
    auto cmp_gt_v4i1  = s_ir_builder->CreateFCmpOGT(va_v4f32, vb_v4f32);
    vb_v4f32          = s_ir_builder->CreateFNeg(vb_v4f32);
    auto cmp_lt_v4i1  = s_ir_builder->CreateFCmpOLT(va_v4f32, vb_v4f32);
    auto cmp_gt_v4i32 = s_ir_builder->CreateZExt(cmp_gt_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    auto cmp_lt_v4i32 = s_ir_builder->CreateZExt(cmp_lt_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    cmp_gt_v4i32      = s_ir_builder->CreateShl(cmp_gt_v4i32, 31);
    cmp_lt_v4i32      = s_ir_builder->CreateShl(cmp_lt_v4i32, 30);
    auto res_v4i32    = s_ir_builder->CreateOr(cmp_gt_v4i32, cmp_lt_v4i32);
    SetVr(vd, res_v4i32);

    // TODO: Implement NJ mode
}

void PPULLVMRecompiler::VCMPBFP_(u32 vd, u32 va, u32 vb) {
    VCMPBFP(vd, va, vb);

    auto vd_v16i8     = GetVrAsIntVec(vd, 8);
    u8 mask_v16i8[16] = {3, 7, 11, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    vd_v16i8          = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_ssse3_pshuf_b_128), vd_v16i8, ConstantDataVector::get(*s_llvm_context, mask_v16i8));
    auto vd_v4i32     = s_ir_builder->CreateBitCast(vd_v16i8, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    auto vd_mask_i32  = s_ir_builder->CreateExtractElement(vd_v4i32, s_ir_builder->getInt32(0));
    auto cmp_i1       = s_ir_builder->CreateICmpEQ(vd_mask_i32, s_ir_builder->getInt32(0));
    SetCrField(6, nullptr, nullptr, cmp_i1, nullptr);
}

void PPULLVMRecompiler::VCMPEQFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = s_ir_builder->CreateFCmpOEQ(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = s_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPEQFP_(u32 vd, u32 va, u32 vb) {
    VCMPEQFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = s_ir_builder->CreateICmpEQ(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = s_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(s_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPEQUB_(u32 vd, u32 va, u32 vb) {
    VCMPEQUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = s_ir_builder->CreateICmpEQ(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = s_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(s_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPEQUH_(u32 vd, u32 va, u32 vb) {
    VCMPEQUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPEQUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = s_ir_builder->CreateICmpEQ(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = s_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPEQUW_(u32 vd, u32 va, u32 vb) {
    VCMPEQUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGEFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = s_ir_builder->CreateFCmpOGE(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = s_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGEFP_(u32 vd, u32 va, u32 vb) {
    VCMPGEFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTFP(u32 vd, u32 va, u32 vb) {
    auto va_v4f32  = GetVrAsFloatVec(va);
    auto vb_v4f32  = GetVrAsFloatVec(vb);
    auto cmp_v4i1  = s_ir_builder->CreateFCmpOGT(va_v4f32, vb_v4f32);
    auto cmp_v4i32 = s_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTFP_(u32 vd, u32 va, u32 vb) {
    VCMPGTFP(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = s_ir_builder->CreateICmpSGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = s_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(s_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPGTSB_(u32 vd, u32 va, u32 vb) {
    VCMPGTSB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = s_ir_builder->CreateICmpSGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = s_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(s_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPGTSH_(u32 vd, u32 va, u32 vb) {
    VCMPGTSH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTSW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = s_ir_builder->CreateICmpSGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = s_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTSW_(u32 vd, u32 va, u32 vb) {
    VCMPGTSW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUB(u32 vd, u32 va, u32 vb) {
    auto va_v16i8  = GetVrAsIntVec(va, 8);
    auto vb_v16i8  = GetVrAsIntVec(vb, 8);
    auto cmp_v16i1 = s_ir_builder->CreateICmpUGT(va_v16i8, vb_v16i8);
    auto cmp_v16i8 = s_ir_builder->CreateSExt(cmp_v16i1, VectorType::get(s_ir_builder->getInt8Ty(), 16));
    SetVr(vd, cmp_v16i8);
}

void PPULLVMRecompiler::VCMPGTUB_(u32 vd, u32 va, u32 vb) {
    VCMPGTUB(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUH(u32 vd, u32 va, u32 vb) {
    auto va_v8i16  = GetVrAsIntVec(va, 16);
    auto vb_v8i16  = GetVrAsIntVec(vb, 16);
    auto cmp_v8i1  = s_ir_builder->CreateICmpUGT(va_v8i16, vb_v8i16);
    auto cmp_v8i16 = s_ir_builder->CreateSExt(cmp_v8i1, VectorType::get(s_ir_builder->getInt16Ty(), 8));
    SetVr(vd, cmp_v8i16);
}

void PPULLVMRecompiler::VCMPGTUH_(u32 vd, u32 va, u32 vb) {
    VCMPGTUH(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCMPGTUW(u32 vd, u32 va, u32 vb) {
    auto va_v4i32  = GetVrAsIntVec(va, 32);
    auto vb_v4i32  = GetVrAsIntVec(vb, 32);
    auto cmp_v4i1  = s_ir_builder->CreateICmpUGT(va_v4i32, vb_v4i32);
    auto cmp_v4i32 = s_ir_builder->CreateSExt(cmp_v4i1, VectorType::get(s_ir_builder->getInt32Ty(), 4));
    SetVr(vd, cmp_v4i32);
}

void PPULLVMRecompiler::VCMPGTUW_(u32 vd, u32 va, u32 vb) {
    VCMPGTUW(vd, va, vb);
    SetCr6AfterVectorCompare(vd);
}

void PPULLVMRecompiler::VCTSXS(u32 vd, u32 uimm5, u32 vb) {
    ThisCall("VCTSXS", &PPUInterpreter::VCTSXS, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VCTUXS(u32 vd, u32 uimm5, u32 vb) {
    ThisCall("VCTUXS", &PPUInterpreter::VCTUXS, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VEXPTEFP(u32 vd, u32 vb) {
    ThisCall("VEXPTEFP", &PPUInterpreter::VEXPTEFP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VLOGEFP(u32 vd, u32 vb) {
    ThisCall("VLOGEFP", &PPUInterpreter::VLOGEFP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) {
    ThisCall("VMADDFP", &PPUInterpreter::VMADDFP, &m_interpreter, vd, va, vc, vb);
}

void PPULLVMRecompiler::VMAXFP(u32 vd, u32 va, u32 vb) {
    ThisCall("VMAXFP", &PPUInterpreter::VMAXFP, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMAXSB", &PPUInterpreter::VMAXSB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMAXSH", &PPUInterpreter::VMAXSH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXSW(u32 vd, u32 va, u32 vb) {
    ThisCall("VMAXSW", &PPUInterpreter::VMAXSW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMAXUB", &PPUInterpreter::VMAXUB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMAXUH", &PPUInterpreter::VMAXUH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMAXUW(u32 vd, u32 va, u32 vb) {
    ThisCall("VMAXUW", &PPUInterpreter::VMAXUW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMHADDSHS", &PPUInterpreter::VMHADDSHS, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMHRADDSHS", &PPUInterpreter::VMHRADDSHS, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMINFP(u32 vd, u32 va, u32 vb) {
    ThisCall("VMINFP", &PPUInterpreter::VMINFP, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINSB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMINSB", &PPUInterpreter::VMINSB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINSH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMINSH", &PPUInterpreter::VMINSH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINSW(u32 vd, u32 va, u32 vb) {
    ThisCall("VMINSW", &PPUInterpreter::VMINSW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINUB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMINUB", &PPUInterpreter::VMINUB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINUH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMINUH", &PPUInterpreter::VMINUH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMINUW(u32 vd, u32 va, u32 vb) {
    ThisCall("VMINUW", &PPUInterpreter::VMINUW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMLADDUHM", &PPUInterpreter::VMLADDUHM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMRGHB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMRGHB", &PPUInterpreter::VMRGHB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGHH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMRGHH", &PPUInterpreter::VMRGHH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGHW(u32 vd, u32 va, u32 vb) {
    ThisCall("VMRGHW", &PPUInterpreter::VMRGHW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMRGLB", &PPUInterpreter::VMRGLB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMRGLH", &PPUInterpreter::VMRGLH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMRGLW(u32 vd, u32 va, u32 vb) {
    ThisCall("VMRGLW", &PPUInterpreter::VMRGLW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMSUMMBM", &PPUInterpreter::VMSUMMBM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMSUMSHM", &PPUInterpreter::VMSUMSHM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMSUMSHS", &PPUInterpreter::VMSUMSHS, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMSUMUBM", &PPUInterpreter::VMSUMUBM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMSUMUHM", &PPUInterpreter::VMSUMUHM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VMSUMUHS", &PPUInterpreter::VMSUMUHS, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VMULESB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMULESB", &PPUInterpreter::VMULESB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULESH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMULESH", &PPUInterpreter::VMULESH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULEUB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMULEUB", &PPUInterpreter::VMULEUB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULEUH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMULEUH", &PPUInterpreter::VMULEUH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULOSB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMULOSB", &PPUInterpreter::VMULOSB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULOSH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMULOSH", &PPUInterpreter::VMULOSH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULOUB(u32 vd, u32 va, u32 vb) {
    ThisCall("VMULOUB", &PPUInterpreter::VMULOUB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VMULOUH(u32 vd, u32 va, u32 vb) {
    ThisCall("VMULOUH", &PPUInterpreter::VMULOUH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) {
    ThisCall("VNMSUBFP", &PPUInterpreter::VNMSUBFP, &m_interpreter, vd, va, vc, vb);
}

void PPULLVMRecompiler::VNOR(u32 vd, u32 va, u32 vb) {
    ThisCall("VNOR", &PPUInterpreter::VNOR, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VOR(u32 vd, u32 va, u32 vb) {
    ThisCall("VOR", &PPUInterpreter::VOR, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPERM(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VPERM", &PPUInterpreter::VPERM, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VPKPX(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKPX", &PPUInterpreter::VPKPX, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKSHSS(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKSHSS", &PPUInterpreter::VPKSHSS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKSHUS(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKSHUS", &PPUInterpreter::VPKSHUS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKSWSS(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKSWSS", &PPUInterpreter::VPKSWSS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKSWUS(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKSWUS", &PPUInterpreter::VPKSWUS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKUHUM(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKUHUM", &PPUInterpreter::VPKUHUM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKUHUS(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKUHUS", &PPUInterpreter::VPKUHUS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKUWUM(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKUWUM", &PPUInterpreter::VPKUWUM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VPKUWUS(u32 vd, u32 va, u32 vb) {
    ThisCall("VPKUWUS", &PPUInterpreter::VPKUWUS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VREFP(u32 vd, u32 vb) {
    ThisCall("VREFP", &PPUInterpreter::VREFP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRFIM(u32 vd, u32 vb) {
    ThisCall("VRFIM", &PPUInterpreter::VRFIM, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRFIN(u32 vd, u32 vb) {
    ThisCall("VRFIN", &PPUInterpreter::VRFIN, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRFIP(u32 vd, u32 vb) {
    ThisCall("VRFIP", &PPUInterpreter::VRFIP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRFIZ(u32 vd, u32 vb) {
    ThisCall("VRFIZ", &PPUInterpreter::VRFIZ, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VRLB(u32 vd, u32 va, u32 vb) {
    ThisCall("VRLB", &PPUInterpreter::VRLB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VRLH(u32 vd, u32 va, u32 vb) {
    ThisCall("VRLH", &PPUInterpreter::VRLH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VRLW(u32 vd, u32 va, u32 vb) {
    ThisCall("VRLW", &PPUInterpreter::VRLW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VRSQRTEFP(u32 vd, u32 vb) {
    ThisCall("VRSQRTEFP", &PPUInterpreter::VRSQRTEFP, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VSEL(u32 vd, u32 va, u32 vb, u32 vc) {
    ThisCall("VSEL", &PPUInterpreter::VSEL, &m_interpreter, vd, va, vb, vc);
}

void PPULLVMRecompiler::VSL(u32 vd, u32 va, u32 vb) {
    ThisCall("VSL", &PPUInterpreter::VSL, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSLB(u32 vd, u32 va, u32 vb) {
    ThisCall("VSLB", &PPUInterpreter::VSLB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) {
    ThisCall("VSLDOI", &PPUInterpreter::VSLDOI, &m_interpreter, vd, va, vb, sh);
}

void PPULLVMRecompiler::VSLH(u32 vd, u32 va, u32 vb) {
    ThisCall("VSLH", &PPUInterpreter::VSLH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSLO(u32 vd, u32 va, u32 vb) {
    ThisCall("VSLO", &PPUInterpreter::VSLO, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSLW(u32 vd, u32 va, u32 vb) {
    ThisCall("VSLW", &PPUInterpreter::VSLW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSPLTB(u32 vd, u32 uimm5, u32 vb) {
    ThisCall("VSPLTB", &PPUInterpreter::VSPLTB, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSPLTH(u32 vd, u32 uimm5, u32 vb) {
    ThisCall("VSPLTH", &PPUInterpreter::VSPLTH, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSPLTISB(u32 vd, s32 simm5) {
    ThisCall("VSPLTISB", &PPUInterpreter::VSPLTISB, &m_interpreter, vd, simm5);
}

void PPULLVMRecompiler::VSPLTISH(u32 vd, s32 simm5) {
    ThisCall("VSPLTISH", &PPUInterpreter::VSPLTISH, &m_interpreter, vd, simm5);
}

void PPULLVMRecompiler::VSPLTISW(u32 vd, s32 simm5) {
    ThisCall("VSPLTISW", &PPUInterpreter::VSPLTISW, &m_interpreter, vd, simm5);
}

void PPULLVMRecompiler::VSPLTW(u32 vd, u32 uimm5, u32 vb) {
    ThisCall("VSPLTW", &PPUInterpreter::VSPLTW, &m_interpreter, vd, uimm5, vb);
}

void PPULLVMRecompiler::VSR(u32 vd, u32 va, u32 vb) {
    ThisCall("VSR", &PPUInterpreter::VSR, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRAB(u32 vd, u32 va, u32 vb) {
    ThisCall("VSRAB", &PPUInterpreter::VSRAB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRAH(u32 vd, u32 va, u32 vb) {
    ThisCall("VSRAH", &PPUInterpreter::VSRAH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRAW(u32 vd, u32 va, u32 vb) {
    ThisCall("VSRAW", &PPUInterpreter::VSRAW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRB(u32 vd, u32 va, u32 vb) {
    ThisCall("VSRB", &PPUInterpreter::VSRB, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRH(u32 vd, u32 va, u32 vb) {
    ThisCall("VSRH", &PPUInterpreter::VSRH, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRO(u32 vd, u32 va, u32 vb) {
    ThisCall("VSRO", &PPUInterpreter::VSRO, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSRW(u32 vd, u32 va, u32 vb) {
    ThisCall("VSRW", &PPUInterpreter::VSRW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBCUW(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBCUW", &PPUInterpreter::VSUBCUW, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBFP(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBFP", &PPUInterpreter::VSUBFP, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSBS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBSBS", &PPUInterpreter::VSUBSBS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSHS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBSHS", &PPUInterpreter::VSUBSHS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBSWS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBSWS", &PPUInterpreter::VSUBSWS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUBM(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBUBM", &PPUInterpreter::VSUBUBM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUBS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBUBS", &PPUInterpreter::VSUBUBS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUHM(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBUHM", &PPUInterpreter::VSUBUHM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUHS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBUHS", &PPUInterpreter::VSUBUHS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUWM(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBUWM", &PPUInterpreter::VSUBUWM, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUBUWS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUBUWS", &PPUInterpreter::VSUBUWS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUMSWS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUMSWS", &PPUInterpreter::VSUMSWS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUM2SWS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUM2SWS", &PPUInterpreter::VSUM2SWS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4SBS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUM4SBS", &PPUInterpreter::VSUM4SBS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4SHS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUM4SHS", &PPUInterpreter::VSUM4SHS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VSUM4UBS(u32 vd, u32 va, u32 vb) {
    ThisCall("VSUM4UBS", &PPUInterpreter::VSUM4UBS, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::VUPKHPX(u32 vd, u32 vb) {
    ThisCall("VUPKHPX", &PPUInterpreter::VUPKHPX, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKHSB(u32 vd, u32 vb) {
    ThisCall("VUPKHSB", &PPUInterpreter::VUPKHSB, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKHSH(u32 vd, u32 vb) {
    ThisCall("VUPKHSH", &PPUInterpreter::VUPKHSH, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKLPX(u32 vd, u32 vb) {
    ThisCall("VUPKLPX", &PPUInterpreter::VUPKLPX, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKLSB(u32 vd, u32 vb) {
    ThisCall("VUPKLSB", &PPUInterpreter::VUPKLSB, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VUPKLSH(u32 vd, u32 vb) {
    ThisCall("VUPKLSH", &PPUInterpreter::VUPKLSH, &m_interpreter, vd, vb);
}

void PPULLVMRecompiler::VXOR(u32 vd, u32 va, u32 vb) {
    ThisCall("VXOR", &PPUInterpreter::VXOR, &m_interpreter, vd, va, vb);
}

void PPULLVMRecompiler::MULLI(u32 rd, u32 ra, s32 simm16) {
    //auto ra_i64  = GetGpr(ra);
    //auto res_i64 = s_ir_builder->CreateMul(ra_i64, s_ir_builder->getInt64((s64)simm16));
    //SetGpr(rd, res_i64);
    ThisCall("MULLI", &PPUInterpreter::MULLI, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::SUBFIC(u32 rd, u32 ra, s32 simm16) {
    //auto ra_i64   = GetGpr(ra);
    //auto res_s    = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::ssub_with_overflow), s_ir_builder->getInt64((s64)simm16), ra_i64);
    //auto diff_i64 = s_ir_builder->CreateExtractValue(res_s, {0});
    //auto carry_i1 = s_ir_builder->CreateExtractValue(res_s, {1});
    //SetGpr(rd, diff_i64);
    //SetXerCa(carry_i1);
    ThisCall("SUBFIC", &PPUInterpreter::SUBFIC, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::CMPLI(u32 crfd, u32 l, u32 ra, u32 uimm16) {
    //auto ra_i64 = GetGpr(ra);
    //if (l == 0) {
    //    ra_i64 = s_ir_builder->CreateAnd(ra_i64, 0xFFFFFFFF);
    //}

    //SetCrFieldUnsignedCmp(crfd, ra_i64, s_ir_builder->getInt64(uimm16));
    ThisCall("CMPLI", &PPUInterpreter::CMPLI, &m_interpreter, crfd, l, ra, uimm16);
}

void PPULLVMRecompiler::CMPI(u32 crfd, u32 l, u32 ra, s32 simm16) {
    //auto ra_i64 = GetGpr(ra);
    //if (l == 0) {
    //    auto ra_i32 = s_ir_builder->CreateTrunc(ra_i64, s_ir_builder->getInt32Ty());
    //    ra_i64      = s_ir_builder->CreateSExt(ra_i64, s_ir_builder->getInt64Ty());
    //}

    //SetCrFieldSignedCmp(crfd, ra_i64, s_ir_builder->getInt64((s64)simm16));
    ThisCall("CMPI", &PPUInterpreter::CMPI, &m_interpreter, crfd, l, ra, simm16);
}

void PPULLVMRecompiler::ADDIC(u32 rd, u32 ra, s32 simm16) {
    //auto ra_i64   = GetGpr(ra);
    //auto res_s    = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::sadd_with_overflow), s_ir_builder->getInt64((s64)simm16), ra_i64);
    //auto sum_i64  = s_ir_builder->CreateExtractValue(res_s, {0});
    //auto carry_i1 = s_ir_builder->CreateExtractValue(res_s, {1});
    //SetGpr(rd, sum_i64);
    //SetXerCa(carry_i1);
    ThisCall("ADDIC", &PPUInterpreter::ADDIC, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDIC_(u32 rd, u32 ra, s32 simm16) {
    //ADDIC(rd, ra, simm16);
    //SetCrFieldSignedCmp(0, GetGpr(rd), s_ir_builder->getInt64(0));
    ThisCall("ADDIC_", &PPUInterpreter::ADDIC_, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDI(u32 rd, u32 ra, s32 simm16) {
    if (ra == 0) {
        SetGpr(rd, s_ir_builder->getInt64((s64)simm16));
    } else {
        auto ra_i64  = GetGpr(ra);
        auto sum_i64 = s_ir_builder->CreateAdd(ra_i64, s_ir_builder->getInt64((s64)simm16));
        SetGpr(rd, sum_i64);
    }
    //ThisCall("ADDI", &PPUInterpreter::ADDI, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::ADDIS(u32 rd, u32 ra, s32 simm16) {
    //if (ra == 0) {
    //    SetGpr(rd, s_ir_builder->getInt64((s64)(simm16 << 16)));
    //} else {
    //    auto ra_i64  = GetGpr(ra);
    //    auto sum_i64 = s_ir_builder->CreateAdd(ra_i64, s_ir_builder->getInt64((s64)(simm16 << 16)));
    //    SetGpr(rd, sum_i64);
    //}
    ThisCall("ADDIS", &PPUInterpreter::ADDIS, &m_interpreter, rd, ra, simm16);
}

void PPULLVMRecompiler::BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) {
    ThisCall("BC", &PPUInterpreter::BC, &m_interpreter, bo, bi, bd, aa, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::SC(u32 sc_code) {
    ThisCall("SC", &PPUInterpreter::SC, &m_interpreter, sc_code);
}

void PPULLVMRecompiler::B(s32 ll, u32 aa, u32 lk) {
    ThisCall("B", &PPUInterpreter::B, &m_interpreter, ll, aa, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::MCRF(u32 crfd, u32 crfs) {
    //if (crfd != crfs) {
    //    auto cr_i32  = GetCr();
    //    auto crf_i32 = GetNibble(cr_i32, crfs);
    //    cr_i32       = SetNibble(cr_i32, crfd, crf_i32);
    //    SetCr(cr_i32);
    //}
    ThisCall("MCRF", &PPUInterpreter::MCRF, &m_interpreter, crfd, crfs);
}

void PPULLVMRecompiler::BCLR(u32 bo, u32 bi, u32 bh, u32 lk) {
    ThisCall("BCLR", &PPUInterpreter::BCLR, &m_interpreter, bo, bi, bh, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::CRNOR(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = s_ir_builder->CreateOr(ba_i32, bb_i32);
    //res_i32      = s_ir_builder->CreateXor(res_i32, 1);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall("CRNOR", &PPUInterpreter::CRNOR, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRANDC(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = s_ir_builder->CreateXor(bb_i32, 1);
    //res_i32      = s_ir_builder->CreateAnd(ba_i32, res_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall("CRANDC", &PPUInterpreter::CRANDC, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::ISYNC() {
    s_ir_builder->CreateCall(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse2_mfence));
}

void PPULLVMRecompiler::CRXOR(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = s_ir_builder->CreateXor(ba_i32, bb_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall("CRXOR", &PPUInterpreter::CRXOR, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRNAND(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = s_ir_builder->CreateAnd(ba_i32, bb_i32);
    //res_i32      = s_ir_builder->CreateXor(res_i32, 1);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall("CRNAND", &PPUInterpreter::CRNAND, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRAND(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = s_ir_builder->CreateAnd(ba_i32, bb_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall("CRAND", &PPUInterpreter::CRAND, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CREQV(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = s_ir_builder->CreateXor(ba_i32, bb_i32);
    //res_i32      = s_ir_builder->CreateXor(res_i32, 1);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall("CREQV", &PPUInterpreter::CREQV, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CRORC(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = s_ir_builder->CreateXor(bb_i32, 1);
    //res_i32      = s_ir_builder->CreateOr(ba_i32, res_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall("CRORC", &PPUInterpreter::CRORC, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::CROR(u32 crbd, u32 crba, u32 crbb) {
    //auto cr_i32  = GetCr();
    //auto ba_i32  = GetBit(cr_i32, crba);
    //auto bb_i32  = GetBit(cr_i32, crbb);
    //auto res_i32 = s_ir_builder->CreateOr(ba_i32, bb_i32);
    //cr_i32       = SetBit(cr_i32, crbd, res_i32);
    //SetCr(cr_i32);
    ThisCall("CROR", &PPUInterpreter::CROR, &m_interpreter, crbd, crba, crbb);
}

void PPULLVMRecompiler::BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) {
    ThisCall("BCCTR", &PPUInterpreter::BCCTR, &m_interpreter, bo, bi, bh, lk);
    m_hit_branch_instruction = true;
}

void PPULLVMRecompiler::RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    ThisCall("RLWIMI", &PPUInterpreter::RLWIMI, &m_interpreter, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompiler::RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) {
    ThisCall("RLWINM", &PPUInterpreter::RLWINM, &m_interpreter, ra, rs, sh, mb, me, rc);
}

void PPULLVMRecompiler::RLWNM(u32 ra, u32 rs, u32 rb, u32 mb, u32 me, bool rc) {
    ThisCall("RLWNM", &PPUInterpreter::RLWNM, &m_interpreter, ra, rs, rb, mb, me, rc);
}

void PPULLVMRecompiler::ORI(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = s_ir_builder->CreateOr(rs_i64, uimm16);
    //SetGpr(ra, res_i64);
    ThisCall("ORI", &PPUInterpreter::ORI, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::ORIS(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = s_ir_builder->CreateOr(rs_i64, uimm16 << 16);
    //SetGpr(ra, res_i64);
    ThisCall("ORIS", &PPUInterpreter::ORIS, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::XORI(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = s_ir_builder->CreateXor(rs_i64, uimm16);
    //SetGpr(ra, res_i64);
    ThisCall("XORI", &PPUInterpreter::XORI, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::XORIS(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = s_ir_builder->CreateXor(rs_i64, uimm16 << 16);
    //SetGpr(ra, res_i64);
    ThisCall("XORIS", &PPUInterpreter::XORIS, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::ANDI_(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = s_ir_builder->CreateAnd(rs_i64, uimm16);
    //SetGpr(ra, res_i64);
    //SetCrFieldSignedCmp(0, res_i64, s_ir_builder->getInt64(0));
    ThisCall("ANDI_", &PPUInterpreter::ANDI_, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::ANDIS_(u32 ra, u32 rs, u32 uimm16) {
    //auto rs_i64  = GetGpr(rs);
    //auto res_i64 = s_ir_builder->CreateAnd(rs_i64, uimm16 << 16);
    //SetGpr(ra, res_i64);
    //SetCrFieldSignedCmp(0, res_i64, s_ir_builder->getInt64(0));
    ThisCall("ANDIS_", &PPUInterpreter::ANDIS_, &m_interpreter, ra, rs, uimm16);
}

void PPULLVMRecompiler::RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    ThisCall("RLDICL", &PPUInterpreter::RLDICL, &m_interpreter, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) {
    ThisCall("RLDICR", &PPUInterpreter::RLDICR, &m_interpreter, ra, rs, sh, me, rc);
}

void PPULLVMRecompiler::RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    ThisCall("RLDIC", &PPUInterpreter::RLDIC, &m_interpreter, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) {
    ThisCall("RLDIMI", &PPUInterpreter::RLDIMI, &m_interpreter, ra, rs, sh, mb, rc);
}

void PPULLVMRecompiler::RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) {
    ThisCall("RLDC_LR", &PPUInterpreter::RLDC_LR, &m_interpreter, ra, rs, rb, m_eb, is_r, rc);
}

void PPULLVMRecompiler::CMP(u32 crfd, u32 l, u32 ra, u32 rb) {
    ThisCall("CMP", &PPUInterpreter::CMP, &m_interpreter, crfd, l, ra, rb);
}

void PPULLVMRecompiler::TW(u32 to, u32 ra, u32 rb) {
    ThisCall("TW", &PPUInterpreter::TW, &m_interpreter, to, ra, rb);
}

void PPULLVMRecompiler::LVSL(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVSL", &PPUInterpreter::LVSL, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LVEBX(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVEBX", &PPUInterpreter::LVEBX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("SUBFC", &PPUInterpreter::SUBFC, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("ADDC", &PPUInterpreter::ADDC, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MULHDU(u32 rd, u32 ra, u32 rb, bool rc) {
    ThisCall("MULHDU", &PPUInterpreter::MULHDU, &m_interpreter, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MULHWU(u32 rd, u32 ra, u32 rb, bool rc) {
    ThisCall("MULHWU", &PPUInterpreter::MULHWU, &m_interpreter, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MFOCRF(u32 a, u32 rd, u32 crm) {
    ThisCall("MFOCRF", &PPUInterpreter::MFOCRF, &m_interpreter, a, rd, crm);
}

void PPULLVMRecompiler::LWARX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LWARX", &PPUInterpreter::LWARX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LDX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LDX", &PPUInterpreter::LDX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LWZX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LWZX", &PPUInterpreter::LWZX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::SLW(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("SLW", &PPUInterpreter::SLW, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::CNTLZW(u32 ra, u32 rs, bool rc) {
    ThisCall("CNTLZW", &PPUInterpreter::CNTLZW, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::SLD(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("SLD", &PPUInterpreter::SLD, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::AND(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("AND", &PPUInterpreter::AND, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::CMPL(u32 crfd, u32 l, u32 ra, u32 rb) {
    ThisCall("CMPL", &PPUInterpreter::CMPL, &m_interpreter, crfd, l, ra, rb);
}

void PPULLVMRecompiler::LVSR(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVSR", &PPUInterpreter::LVSR, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LVEHX(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVEHX", &PPUInterpreter::LVEHX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("SUBF", &PPUInterpreter::SUBF, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::LDUX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LDUX", &PPUInterpreter::LDUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::DCBST(u32 ra, u32 rb) {
    ThisCall("DCBST", &PPUInterpreter::DCBST, &m_interpreter, ra, rb);
}

void PPULLVMRecompiler::LWZUX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LWZUX", &PPUInterpreter::LWZUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::CNTLZD(u32 ra, u32 rs, bool rc) {
    ThisCall("CNTLZD", &PPUInterpreter::CNTLZD, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::ANDC(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("ANDC", &PPUInterpreter::ANDC, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::TD(u32 to, u32 ra, u32 rb) {
    ThisCall("TD", &PPUInterpreter::TD, &m_interpreter, to, ra, rb);
}

void PPULLVMRecompiler::LVEWX(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVEWX", &PPUInterpreter::LVEWX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::MULHD(u32 rd, u32 ra, u32 rb, bool rc) {
    ThisCall("MULHD", &PPUInterpreter::MULHD, &m_interpreter, rd, ra, rb, rc);
}

void PPULLVMRecompiler::MULHW(u32 rd, u32 ra, u32 rb, bool rc) {
    ThisCall("MULHW", &PPUInterpreter::MULHW, &m_interpreter, rd, ra, rb, rc);
}

void PPULLVMRecompiler::LDARX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LDARX", &PPUInterpreter::LDARX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::DCBF(u32 ra, u32 rb) {
    ThisCall("DCBF", &PPUInterpreter::DCBF, &m_interpreter, ra, rb);
}

void PPULLVMRecompiler::LBZX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LBZX", &PPUInterpreter::LBZX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LVX(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVX", &PPUInterpreter::LVX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::NEG(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall("NEG", &PPUInterpreter::NEG, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::LBZUX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LBZUX", &PPUInterpreter::LBZUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::NOR(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("NOR", &PPUInterpreter::NOR, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::STVEBX(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVEBX", &PPUInterpreter::STVEBX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("SUBFE", &PPUInterpreter::SUBFE, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("ADDE", &PPUInterpreter::ADDE, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MTOCRF(u32 l, u32 crm, u32 rs) {
    ThisCall("MTOCRF", &PPUInterpreter::MTOCRF, &m_interpreter, l, crm, rs);
}

void PPULLVMRecompiler::STDX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STDX", &PPUInterpreter::STDX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STWCX_(u32 rs, u32 ra, u32 rb) {
    ThisCall("STWCX_", &PPUInterpreter::STWCX_, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STWX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STWX", &PPUInterpreter::STWX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STVEHX(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVEHX", &PPUInterpreter::STVEHX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::STDUX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STDUX", &PPUInterpreter::STDUX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STWUX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STWUX", &PPUInterpreter::STWUX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STVEWX(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVEWX", &PPUInterpreter::STVEWX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::ADDZE(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall("ADDZE", &PPUInterpreter::ADDZE, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall("SUBFZE", &PPUInterpreter::SUBFZE, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::STDCX_(u32 rs, u32 ra, u32 rb) {
    ThisCall("STDCX_", &PPUInterpreter::STDCX_, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STBX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STBX", &PPUInterpreter::STBX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STVX(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVX", &PPUInterpreter::STVX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::SUBFME(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall("SUBFME", &PPUInterpreter::SUBFME, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("MULLD", &PPUInterpreter::MULLD, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::ADDME(u32 rd, u32 ra, u32 oe, bool rc) {
    ThisCall("ADDME", &PPUInterpreter::ADDME, &m_interpreter, rd, ra, oe, rc);
}

void PPULLVMRecompiler::MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("MULLW", &PPUInterpreter::MULLW, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DCBTST(u32 ra, u32 rb, u32 th) {
    ThisCall("DCBTST", &PPUInterpreter::DCBTST, &m_interpreter, ra, rb, th);
}

void PPULLVMRecompiler::STBUX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STBUX", &PPUInterpreter::STBUX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("ADD", &PPUInterpreter::ADD, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DCBT(u32 ra, u32 rb, u32 th) {
    ThisCall("DCBT", &PPUInterpreter::DCBT, &m_interpreter, ra, rb, th);
}

void PPULLVMRecompiler::LHZX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LHZX", &PPUInterpreter::LHZX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::EQV(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("EQV", &PPUInterpreter::EQV, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::ECIWX(u32 rd, u32 ra, u32 rb) {
    ThisCall("ECIWX", &PPUInterpreter::ECIWX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LHZUX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LHZUX", &PPUInterpreter::LHZUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::XOR(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("XOR", &PPUInterpreter::XOR, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::MFSPR(u32 rd, u32 spr) {
    ThisCall("MFSPR", &PPUInterpreter::MFSPR, &m_interpreter, rd, spr);
}

void PPULLVMRecompiler::LWAX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LWAX", &PPUInterpreter::LWAX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::DST(u32 ra, u32 rb, u32 strm, u32 t) {
    ThisCall("DST", &PPUInterpreter::DST, &m_interpreter, ra, rb, strm, t);
}

void PPULLVMRecompiler::LHAX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LHAX", &PPUInterpreter::LHAX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LVXL(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVXL", &PPUInterpreter::LVXL, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::MFTB(u32 rd, u32 spr) {
    ThisCall("MFTB", &PPUInterpreter::MFTB, &m_interpreter, rd, spr);
}

void PPULLVMRecompiler::LWAUX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LWAUX", &PPUInterpreter::LWAUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::DSTST(u32 ra, u32 rb, u32 strm, u32 t) {
    ThisCall("DSTST", &PPUInterpreter::DSTST, &m_interpreter, ra, rb, strm, t);
}

void PPULLVMRecompiler::LHAUX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LHAUX", &PPUInterpreter::LHAUX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::STHX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STHX", &PPUInterpreter::STHX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::ORC(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("ORC", &PPUInterpreter::ORC, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::ECOWX(u32 rs, u32 ra, u32 rb) {
    ThisCall("ECOWX", &PPUInterpreter::ECOWX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STHUX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STHUX", &PPUInterpreter::STHUX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::OR(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("OR", &PPUInterpreter::OR, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("DIVDU", &PPUInterpreter::DIVDU, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("DIVWU", &PPUInterpreter::DIVWU, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::MTSPR(u32 spr, u32 rs) {
    ThisCall("MTSPR", &PPUInterpreter::MTSPR, &m_interpreter, spr, rs);
}

void PPULLVMRecompiler::NAND(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("NAND", &PPUInterpreter::NAND, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::STVXL(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVXL", &PPUInterpreter::STVXL, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("DIVD", &PPUInterpreter::DIVD, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) {
    ThisCall("DIVW", &PPUInterpreter::DIVW, &m_interpreter, rd, ra, rb, oe, rc);
}

void PPULLVMRecompiler::LVLX(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVLX", &PPUInterpreter::LVLX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LDBRX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LDBRX", &PPUInterpreter::LDBRX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LSWX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LSWX", &PPUInterpreter::LSWX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LWBRX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LWBRX", &PPUInterpreter::LWBRX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::LFSX(u32 frd, u32 ra, u32 rb) {
    ThisCall("LFSX", &PPUInterpreter::LFSX, &m_interpreter, frd, ra, rb);
}

void PPULLVMRecompiler::SRW(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("SRW", &PPUInterpreter::SRW, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::SRD(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("SRD", &PPUInterpreter::SRD, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::LVRX(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVRX", &PPUInterpreter::LVRX, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LSWI(u32 rd, u32 ra, u32 nb) {
    ThisCall("LSWI", &PPUInterpreter::LSWI, &m_interpreter, rd, ra, nb);
}

void PPULLVMRecompiler::LFSUX(u32 frd, u32 ra, u32 rb) {
    ThisCall("LFSUX", &PPUInterpreter::LFSUX, &m_interpreter, frd, ra, rb);
}

void PPULLVMRecompiler::SYNC(u32 l) {
    ThisCall("SYNC", &PPUInterpreter::SYNC, &m_interpreter, l);
}

void PPULLVMRecompiler::LFDX(u32 frd, u32 ra, u32 rb) {
    ThisCall("LFDX", &PPUInterpreter::LFDX, &m_interpreter, frd, ra, rb);
}

void PPULLVMRecompiler::LFDUX(u32 frd, u32 ra, u32 rb) {
    ThisCall("LFDUX", &PPUInterpreter::LFDUX, &m_interpreter, frd, ra, rb);
}

void PPULLVMRecompiler::STVLX(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVLX", &PPUInterpreter::STVLX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::STSWX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STSWX", &PPUInterpreter::STSWX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STWBRX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STWBRX", &PPUInterpreter::STWBRX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::STFSX(u32 frs, u32 ra, u32 rb) {
    ThisCall("STFSX", &PPUInterpreter::STFSX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::STVRX(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVRX", &PPUInterpreter::STVRX, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::STFSUX(u32 frs, u32 ra, u32 rb) {
    ThisCall("STFSUX", &PPUInterpreter::STFSUX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::STSWI(u32 rd, u32 ra, u32 nb) {
    ThisCall("STSWI", &PPUInterpreter::STSWI, &m_interpreter, rd, ra, nb);
}

void PPULLVMRecompiler::STFDX(u32 frs, u32 ra, u32 rb) {
    ThisCall("STFDX", &PPUInterpreter::STFDX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::STFDUX(u32 frs, u32 ra, u32 rb) {
    ThisCall("STFDUX", &PPUInterpreter::STFDUX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::LVLXL(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVLXL", &PPUInterpreter::LVLXL, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::LHBRX(u32 rd, u32 ra, u32 rb) {
    ThisCall("LHBRX", &PPUInterpreter::LHBRX, &m_interpreter, rd, ra, rb);
}

void PPULLVMRecompiler::SRAW(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("SRAW", &PPUInterpreter::SRAW, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::SRAD(u32 ra, u32 rs, u32 rb, bool rc) {
    ThisCall("SRAD", &PPUInterpreter::SRAD, &m_interpreter, ra, rs, rb, rc);
}

void PPULLVMRecompiler::LVRXL(u32 vd, u32 ra, u32 rb) {
    ThisCall("LVRXL", &PPUInterpreter::LVRXL, &m_interpreter, vd, ra, rb);
}

void PPULLVMRecompiler::DSS(u32 strm, u32 a) {
    ThisCall("DSS", &PPUInterpreter::DSS, &m_interpreter, strm, a);
}

void PPULLVMRecompiler::SRAWI(u32 ra, u32 rs, u32 sh, bool rc) {
    ThisCall("SRAWI", &PPUInterpreter::SRAWI, &m_interpreter, ra, rs, sh, rc);
}

void PPULLVMRecompiler::SRADI1(u32 ra, u32 rs, u32 sh, bool rc) {
    ThisCall("SRADI1", &PPUInterpreter::SRADI1, &m_interpreter, ra, rs, sh, rc);
}

void PPULLVMRecompiler::SRADI2(u32 ra, u32 rs, u32 sh, bool rc) {
    ThisCall("SRADI2", &PPUInterpreter::SRADI2, &m_interpreter, ra, rs, sh, rc);
}

void PPULLVMRecompiler::EIEIO() {
    ThisCall("EIEIO", &PPUInterpreter::EIEIO, &m_interpreter);
}

void PPULLVMRecompiler::STVLXL(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVLXL", &PPUInterpreter::STVLXL, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::STHBRX(u32 rs, u32 ra, u32 rb) {
    ThisCall("STHBRX", &PPUInterpreter::STHBRX, &m_interpreter, rs, ra, rb);
}

void PPULLVMRecompiler::EXTSH(u32 ra, u32 rs, bool rc) {
    ThisCall("EXTSH", &PPUInterpreter::EXTSH, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::STVRXL(u32 vs, u32 ra, u32 rb) {
    ThisCall("STVRXL", &PPUInterpreter::STVRXL, &m_interpreter, vs, ra, rb);
}

void PPULLVMRecompiler::EXTSB(u32 ra, u32 rs, bool rc) {
    ThisCall("EXTSB", &PPUInterpreter::EXTSB, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::STFIWX(u32 frs, u32 ra, u32 rb) {
    ThisCall("STFIWX", &PPUInterpreter::STFIWX, &m_interpreter, frs, ra, rb);
}

void PPULLVMRecompiler::EXTSW(u32 ra, u32 rs, bool rc) {
    ThisCall("EXTSW", &PPUInterpreter::EXTSW, &m_interpreter, ra, rs, rc);
}

void PPULLVMRecompiler::ICBI(u32 ra, u32 rs) {
    ThisCall("ICBI", &PPUInterpreter::ICBI, &m_interpreter, ra, rs);
}

void PPULLVMRecompiler::DCBZ(u32 ra, u32 rb) {
    ThisCall("DCBZ", &PPUInterpreter::DCBZ, &m_interpreter, ra, rb);
}

void PPULLVMRecompiler::LWZ(u32 rd, u32 ra, s32 d) {
    ThisCall("LWZ", &PPUInterpreter::LWZ, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LWZU(u32 rd, u32 ra, s32 d) {
    ThisCall("LWZU", &PPUInterpreter::LWZU, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LBZ(u32 rd, u32 ra, s32 d) {
    ThisCall("LBZ", &PPUInterpreter::LBZ, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LBZU(u32 rd, u32 ra, s32 d) {
    ThisCall("LBZU", &PPUInterpreter::LBZU, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::STW(u32 rs, u32 ra, s32 d) {
    ThisCall("STW", &PPUInterpreter::STW, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STWU(u32 rs, u32 ra, s32 d) {
    ThisCall("STWU", &PPUInterpreter::STWU, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STB(u32 rs, u32 ra, s32 d) {
    ThisCall("STB", &PPUInterpreter::STB, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STBU(u32 rs, u32 ra, s32 d) {
    ThisCall("STBU", &PPUInterpreter::STBU, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::LHZ(u32 rd, u32 ra, s32 d) {
    ThisCall("LHZ", &PPUInterpreter::LHZ, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LHZU(u32 rd, u32 ra, s32 d) {
    ThisCall("LHZU", &PPUInterpreter::LHZU, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LHA(u32 rd, u32 ra, s32 d) {
    ThisCall("LHA", &PPUInterpreter::LHA, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::LHAU(u32 rd, u32 ra, s32 d) {
    ThisCall("LHAU", &PPUInterpreter::LHAU, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::STH(u32 rs, u32 ra, s32 d) {
    ThisCall("STH", &PPUInterpreter::STH, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STHU(u32 rs, u32 ra, s32 d) {
    ThisCall("STHU", &PPUInterpreter::STHU, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::LMW(u32 rd, u32 ra, s32 d) {
    ThisCall("LMW", &PPUInterpreter::LMW, &m_interpreter, rd, ra, d);
}

void PPULLVMRecompiler::STMW(u32 rs, u32 ra, s32 d) {
    ThisCall("STMW", &PPUInterpreter::STMW, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::LFS(u32 frd, u32 ra, s32 d) {
    ThisCall("LFS", &PPUInterpreter::LFS, &m_interpreter, frd, ra, d);
}

void PPULLVMRecompiler::LFSU(u32 frd, u32 ra, s32 ds) {
    ThisCall("LFSU", &PPUInterpreter::LFSU, &m_interpreter, frd, ra, ds);
}

void PPULLVMRecompiler::LFD(u32 frd, u32 ra, s32 d) {
    ThisCall("LFD", &PPUInterpreter::LFD, &m_interpreter, frd, ra, d);
}

void PPULLVMRecompiler::LFDU(u32 frd, u32 ra, s32 ds) {
    ThisCall("LFDU", &PPUInterpreter::LFDU, &m_interpreter, frd, ra, ds);
}

void PPULLVMRecompiler::STFS(u32 frs, u32 ra, s32 d) {
    ThisCall("STFS", &PPUInterpreter::STFS, &m_interpreter, frs, ra, d);
}

void PPULLVMRecompiler::STFSU(u32 frs, u32 ra, s32 d) {
    ThisCall("STFSU", &PPUInterpreter::STFSU, &m_interpreter, frs, ra, d);
}

void PPULLVMRecompiler::STFD(u32 frs, u32 ra, s32 d) {
    ThisCall("STFD", &PPUInterpreter::STFD, &m_interpreter, frs, ra, d);
}

void PPULLVMRecompiler::STFDU(u32 frs, u32 ra, s32 d) {
    ThisCall("STFDU", &PPUInterpreter::STFDU, &m_interpreter, frs, ra, d);
}

void PPULLVMRecompiler::LD(u32 rd, u32 ra, s32 ds) {
    ThisCall("LD", &PPUInterpreter::LD, &m_interpreter, rd, ra, ds);
}

void PPULLVMRecompiler::LDU(u32 rd, u32 ra, s32 ds) {
    ThisCall("LDU", &PPUInterpreter::LDU, &m_interpreter, rd, ra, ds);
}

void PPULLVMRecompiler::LWA(u32 rd, u32 ra, s32 ds) {
    ThisCall("LWA", &PPUInterpreter::LWA, &m_interpreter, rd, ra, ds);
}

void PPULLVMRecompiler::FDIVS(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall("FDIVS", &PPUInterpreter::FDIVS, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSUBS(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall("FSUBS", &PPUInterpreter::FSUBS, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FADDS(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall("FADDS", &PPUInterpreter::FADDS, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSQRTS(u32 frd, u32 frb, bool rc) {
    ThisCall("FSQRTS", &PPUInterpreter::FSQRTS, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FRES(u32 frd, u32 frb, bool rc) {
    ThisCall("FRES", &PPUInterpreter::FRES, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FMULS(u32 frd, u32 fra, u32 frc, bool rc) {
    ThisCall("FMULS", &PPUInterpreter::FMULS, &m_interpreter, frd, fra, frc, rc);
}

void PPULLVMRecompiler::FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FMADDS", &PPUInterpreter::FMADDS, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FMSUBS", &PPUInterpreter::FMSUBS, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FNMSUBS", &PPUInterpreter::FNMSUBS, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FNMADDS", &PPUInterpreter::FNMADDS, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::STD(u32 rs, u32 ra, s32 d) {
    ThisCall("STD", &PPUInterpreter::STD, &m_interpreter, rs, ra, d);
}

void PPULLVMRecompiler::STDU(u32 rs, u32 ra, s32 ds) {
    ThisCall("STDU", &PPUInterpreter::STDU, &m_interpreter, rs, ra, ds);
}

void PPULLVMRecompiler::MTFSB1(u32 crbd, bool rc) {
    ThisCall("MTFSB1", &PPUInterpreter::MTFSB1, &m_interpreter, crbd, rc);
}

void PPULLVMRecompiler::MCRFS(u32 crbd, u32 crbs) {
    ThisCall("MCRFS", &PPUInterpreter::MCRFS, &m_interpreter, crbd, crbs);
}

void PPULLVMRecompiler::MTFSB0(u32 crbd, bool rc) {
    ThisCall("MTFSB0", &PPUInterpreter::MTFSB0, &m_interpreter, crbd, rc);
}

void PPULLVMRecompiler::MTFSFI(u32 crfd, u32 i, bool rc) {
    ThisCall("MTFSFI", &PPUInterpreter::MTFSFI, &m_interpreter, crfd, i, rc);
}

void PPULLVMRecompiler::MFFS(u32 frd, bool rc) {
    ThisCall("MFFS", &PPUInterpreter::MFFS, &m_interpreter, frd, rc);
}

void PPULLVMRecompiler::MTFSF(u32 flm, u32 frb, bool rc) {
    ThisCall("MTFSF", &PPUInterpreter::MTFSF, &m_interpreter, flm, frb, rc);
}

void PPULLVMRecompiler::FCMPU(u32 crfd, u32 fra, u32 frb) {
    ThisCall("FCMPU", &PPUInterpreter::FCMPU, &m_interpreter, crfd, fra, frb);
}

void PPULLVMRecompiler::FRSP(u32 frd, u32 frb, bool rc) {
    ThisCall("FRSP", &PPUInterpreter::FRSP, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIW(u32 frd, u32 frb, bool rc) {
    ThisCall("FCTIW", &PPUInterpreter::FCTIW, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIWZ(u32 frd, u32 frb, bool rc) {
    ThisCall("FCTIWZ", &PPUInterpreter::FCTIWZ, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FDIV(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall("FDIV", &PPUInterpreter::FDIV, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSUB(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall("FSUB", &PPUInterpreter::FSUB, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FADD(u32 frd, u32 fra, u32 frb, bool rc) {
    ThisCall("FADD", &PPUInterpreter::FADD, &m_interpreter, frd, fra, frb, rc);
}

void PPULLVMRecompiler::FSQRT(u32 frd, u32 frb, bool rc) {
    ThisCall("FSQRT", &PPUInterpreter::FSQRT, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FSEL", &PPUInterpreter::FSEL, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMUL(u32 frd, u32 fra, u32 frc, bool rc) {
    ThisCall("FMUL", &PPUInterpreter::FMUL, &m_interpreter, frd, fra, frc, rc);
}

void PPULLVMRecompiler::FRSQRTE(u32 frd, u32 frb, bool rc) {
    ThisCall("FRSQRTE", &PPUInterpreter::FRSQRTE, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FMSUB", &PPUInterpreter::FMSUB, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FMADD", &PPUInterpreter::FMADD, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FNMSUB", &PPUInterpreter::FNMSUB, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) {
    ThisCall("FNMADD", &PPUInterpreter::FNMADD, &m_interpreter, frd, fra, frc, frb, rc);
}

void PPULLVMRecompiler::FCMPO(u32 crfd, u32 fra, u32 frb) {
    ThisCall("FCMPO", &PPUInterpreter::FCMPO, &m_interpreter, crfd, fra, frb);
}

void PPULLVMRecompiler::FNEG(u32 frd, u32 frb, bool rc) {
    ThisCall("FNEG", &PPUInterpreter::FNEG, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FMR(u32 frd, u32 frb, bool rc) {
    ThisCall("FMR", &PPUInterpreter::FMR, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FNABS(u32 frd, u32 frb, bool rc) {
    ThisCall("FNABS", &PPUInterpreter::FNABS, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FABS(u32 frd, u32 frb, bool rc) {
    ThisCall("FABS", &PPUInterpreter::FABS, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCTID(u32 frd, u32 frb, bool rc) {
    ThisCall("FCTID", &PPUInterpreter::FCTID, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCTIDZ(u32 frd, u32 frb, bool rc) {
    ThisCall("FCTIDZ", &PPUInterpreter::FCTIDZ, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::FCFID(u32 frd, u32 frb, bool rc) {
    ThisCall("FCFID", &PPUInterpreter::FCFID, &m_interpreter, frd, frb, rc);
}

void PPULLVMRecompiler::UNK(const u32 code, const u32 opcode, const u32 gcode) {
    //ThisCall("UNK", &PPUInterpreter::UNK, &m_interpreter, code, opcode, gcode);
}

Value * PPULLVMRecompiler::GetBit(Value * val, u32 n) {
    Value * bit;

    if (val->getType()->isIntegerTy(32)) {
        bit = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_bmi_pext_32), val, s_ir_builder->getInt32(1 << (31- n)));
    } else if (val->getType()->isIntegerTy(64)) {
        bit = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_bmi_pext_64), val, s_ir_builder->getInt64(1 << (63 - n)));
    } else {
        if (val->getType()->getIntegerBitWidth() != (n + 1)) {
            bit = s_ir_builder->CreateLShr(val, val->getType()->getIntegerBitWidth() - n - 1);
        }

        bit = s_ir_builder->CreateAnd(val, 1);
    }

    return bit;
}

Value * PPULLVMRecompiler::ClrBit(Value * val, u32 n) {
    return s_ir_builder->CreateAnd(val, ~(1 << (val->getType()->getIntegerBitWidth() - n - 1)));
}

Value * PPULLVMRecompiler::SetBit(Value * val, u32 n, Value * bit, bool doClear) {
    if (doClear) {
        val = ClrBit(val, n);
    }

    if (bit->getType()->getIntegerBitWidth() < val->getType()->getIntegerBitWidth()) {
        bit = s_ir_builder->CreateZExt(bit, val->getType());
    } else if (bit->getType()->getIntegerBitWidth() > val->getType()->getIntegerBitWidth()) {
        bit = s_ir_builder->CreateTrunc(bit, val->getType());
    }

    if (val->getType()->getIntegerBitWidth() != (n + 1)) {
        bit = s_ir_builder->CreateShl(bit, bit->getType()->getIntegerBitWidth() - n - 1);
    }

    return s_ir_builder->CreateOr(val, bit);
}

Value * PPULLVMRecompiler::GetNibble(Value * val, u32 n) {
    Value * nibble;

    if (val->getType()->isIntegerTy(32)) {
        nibble = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_bmi_pext_32), val, s_ir_builder->getInt32(0xF << ((7 - n) * 4)));
    } else if (val->getType()->isIntegerTy(64)) {
        nibble = s_ir_builder->CreateCall2(Intrinsic::getDeclaration(s_module, Intrinsic::x86_bmi_pext_64), val, s_ir_builder->getInt64(0xF << ((15 - n) * 4)));
    } else {
        if ((val->getType()->getIntegerBitWidth() >> 2) != (n + 1)) {
            nibble = s_ir_builder->CreateLShr(val, (((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4);
        }

        nibble = s_ir_builder->CreateAnd(val, 0xF);
    }

    return nibble;
}

Value * PPULLVMRecompiler::ClrNibble(Value * val, u32 n) {
    return s_ir_builder->CreateAnd(val, ~(0xF << ((((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4)));
}

Value * PPULLVMRecompiler::SetNibble(Value * val, u32 n, Value * nibble, bool doClear) {
    if (doClear) {
        val = ClrNibble(val, n);
    }

    if (nibble->getType()->getIntegerBitWidth() < val->getType()->getIntegerBitWidth()) {
        nibble = s_ir_builder->CreateZExt(nibble, val->getType());
    } else if (nibble->getType()->getIntegerBitWidth() > val->getType()->getIntegerBitWidth()) {
        nibble = s_ir_builder->CreateTrunc(nibble, val->getType());
    }

    if ((val->getType()->getIntegerBitWidth() >> 2) != (n + 1)) {
        nibble = s_ir_builder->CreateShl(nibble, (((val->getType()->getIntegerBitWidth() >> 2) - 1) - n) * 4);
    }

    return s_ir_builder->CreateOr(val, nibble);
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
    auto pc_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, PC));
    auto pc_i64_ptr = s_ir_builder->CreateBitCast(pc_i8_ptr, s_ir_builder->getInt64Ty()->getPointerTo());
    return s_ir_builder->CreateLoad(pc_i64_ptr);
}

void PPULLVMRecompiler::SetPc(Value * val_i64) {
    auto pc_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, PC));
    auto pc_i64_ptr = s_ir_builder->CreateBitCast(pc_i8_ptr, s_ir_builder->getInt64Ty()->getPointerTo());
    s_ir_builder->CreateStore(val_i64, pc_i64_ptr);
}

Value * PPULLVMRecompiler::GetGpr(u32 r) {
    auto r_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, GPR[r]));
    auto r_i64_ptr = s_ir_builder->CreateBitCast(r_i8_ptr, s_ir_builder->getInt64Ty()->getPointerTo());
    return s_ir_builder->CreateLoad(r_i64_ptr);
}

void PPULLVMRecompiler::SetGpr(u32 r, Value * val_x64) {
    auto r_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, GPR[r]));
    auto r_i64_ptr = s_ir_builder->CreateBitCast(r_i8_ptr, s_ir_builder->getInt64Ty()->getPointerTo());
    auto val_i64   = s_ir_builder->CreateBitCast(val_x64, s_ir_builder->getInt64Ty());
    s_ir_builder->CreateStore(val_i64, r_i64_ptr);
}

Value * PPULLVMRecompiler::GetCr() {
    auto cr_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, CR));
    auto cr_i32_ptr = s_ir_builder->CreateBitCast(cr_i8_ptr, s_ir_builder->getInt32Ty()->getPointerTo());
    return s_ir_builder->CreateLoad(cr_i32_ptr);
}

Value * PPULLVMRecompiler::GetCrField(u32 n) {
    return GetNibble(GetCr(), n);
}

void PPULLVMRecompiler::SetCr(Value * val_x32) {
    auto val_i32    = s_ir_builder->CreateBitCast(val_x32, s_ir_builder->getInt32Ty());
    auto cr_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, CR));
    auto cr_i32_ptr = s_ir_builder->CreateBitCast(cr_i8_ptr, s_ir_builder->getInt32Ty()->getPointerTo());
    s_ir_builder->CreateStore(val_i32, cr_i32_ptr);
}

void PPULLVMRecompiler::SetCrField(u32 n, Value * field) {
    SetCr(SetNibble(GetCr(), n, field));
}

void PPULLVMRecompiler::SetCrField(u32 n, Value * b0, Value * b1, Value * b2, Value * b3) {
    SetCr(SetNibble(GetCr(), n, b0, b1, b2, b3));
}

void PPULLVMRecompiler::SetCrFieldSignedCmp(u32 n, Value * a, Value * b) {
    auto lt_i1  = s_ir_builder->CreateICmpSLT(a, b);
    auto gt_i1  = s_ir_builder->CreateICmpSGT(a, b);
    auto eq_i1  = s_ir_builder->CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void PPULLVMRecompiler::SetCrFieldUnsignedCmp(u32 n, Value * a, Value * b) {
    auto lt_i1  = s_ir_builder->CreateICmpULT(a, b);
    auto gt_i1  = s_ir_builder->CreateICmpUGT(a, b);
    auto eq_i1  = s_ir_builder->CreateICmpEQ(a, b);
    auto cr_i32 = GetCr();
    cr_i32      = SetNibble(cr_i32, n, lt_i1, gt_i1, eq_i1, GetXerSo());
    SetCr(cr_i32);
}

void PPULLVMRecompiler::SetCr6AfterVectorCompare(u32 vr) {
    auto vr_v16i8    = GetVrAsIntVec(vr, 8);
    auto vr_mask_i32 = s_ir_builder->CreateCall(Intrinsic::getDeclaration(s_module, Intrinsic::x86_sse2_pmovmskb_128), vr_v16i8);
    auto cmp0_i1     = s_ir_builder->CreateICmpEQ(vr_mask_i32, s_ir_builder->getInt32(0));
    auto cmp1_i1     = s_ir_builder->CreateICmpEQ(vr_mask_i32, s_ir_builder->getInt32(0xFFFF));
    auto cr_i32      = GetCr();
    cr_i32           = SetNibble(cr_i32, 6, cmp1_i1, nullptr, cmp0_i1, nullptr);
    SetCr(cr_i32);
}

Value * PPULLVMRecompiler::GetXer() {
    auto xer_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, XER));
    auto xer_i64_ptr = s_ir_builder->CreateBitCast(xer_i8_ptr, s_ir_builder->getInt64Ty()->getPointerTo());
    return s_ir_builder->CreateLoad(xer_i64_ptr);
}

Value * PPULLVMRecompiler::GetXerCa() {
    return GetBit(GetXer(), 34);
}

Value * PPULLVMRecompiler::GetXerSo() {
    return GetBit(GetXer(), 32);
}

void PPULLVMRecompiler::SetXer(Value * val_x64) {
    auto val_i64     = s_ir_builder->CreateBitCast(val_x64, s_ir_builder->getInt64Ty());
    auto xer_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, XER));
    auto xer_i64_ptr = s_ir_builder->CreateBitCast(xer_i8_ptr, s_ir_builder->getInt64Ty()->getPointerTo());
    s_ir_builder->CreateStore(val_i64, xer_i64_ptr);
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
    auto vscr_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = s_ir_builder->CreateBitCast(vscr_i8_ptr, s_ir_builder->getInt32Ty()->getPointerTo());
    return s_ir_builder->CreateLoad(vscr_i32_ptr);
}

void PPULLVMRecompiler::SetVscr(Value * val_x32) {
    auto val_i32      = s_ir_builder->CreateBitCast(val_x32, s_ir_builder->getInt32Ty());
    auto vscr_i8_ptr  = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, VSCR));
    auto vscr_i32_ptr = s_ir_builder->CreateBitCast(vscr_i8_ptr, s_ir_builder->getInt32Ty()->getPointerTo());
    s_ir_builder->CreateStore(val_i32, vscr_i32_ptr);
}

Value * PPULLVMRecompiler::GetVrAsIntVec(u32 vr, u32 vec_elt_num_bits) {
    auto vr_i8_ptr   = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = s_ir_builder->CreateBitCast(vr_i8_ptr, s_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_vec_ptr  = s_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(s_ir_builder->getIntNTy(vec_elt_num_bits), 128 / vec_elt_num_bits)->getPointerTo());
    return s_ir_builder->CreateLoad(vr_vec_ptr);
}

Value * PPULLVMRecompiler::GetVrAsFloatVec(u32 vr) {
    auto vr_i8_ptr    = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = s_ir_builder->CreateBitCast(vr_i8_ptr, s_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v4f32_ptr = s_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(s_ir_builder->getFloatTy(), 4)->getPointerTo());
    return s_ir_builder->CreateLoad(vr_v4f32_ptr);
}

Value * PPULLVMRecompiler::GetVrAsDoubleVec(u32 vr) {
    auto vr_i8_ptr    = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr  = s_ir_builder->CreateBitCast(vr_i8_ptr, s_ir_builder->getIntNTy(128)->getPointerTo());
    auto vr_v2f64_ptr = s_ir_builder->CreateBitCast(vr_i128_ptr, VectorType::get(s_ir_builder->getDoubleTy(), 2)->getPointerTo());
    return s_ir_builder->CreateLoad(vr_v2f64_ptr);
}

void PPULLVMRecompiler::SetVr(u32 vr, Value * val_x128) {
    auto vr_i8_ptr   = s_ir_builder->CreateConstGEP1_32(s_state_ptr, offsetof(PPUThread, VPR[vr]));
    auto vr_i128_ptr = s_ir_builder->CreateBitCast(vr_i8_ptr, s_ir_builder->getIntNTy(128)->getPointerTo());
    auto val_i128    = s_ir_builder->CreateBitCast(val_x128, s_ir_builder->getIntNTy(128));
    s_ir_builder->CreateStore(val_i128, vr_i128_ptr);
}

template<class F, class C, class... Args>
void PPULLVMRecompiler::ThisCall(const char * name, F function, C * this_ptr, Args... args) {
    auto this_call_fn = new std::function<void()>(std::bind(function, this_ptr, args...));
    s_this_call_ptrs_list.push_back(this_call_fn);

    auto this_call_fn_i64 = s_ir_builder->getInt64((uint64_t)this_call_fn);
    auto this_call_fn_ptr = s_ir_builder->CreateIntToPtr(this_call_fn_i64, s_ir_builder->getInt64Ty()->getPointerTo());
    std::vector<Value *> execute_this_call_fn_args;
    execute_this_call_fn_args.push_back(this_call_fn_ptr);
    auto call_execute_this_call_fn_instr = s_ir_builder->CreateCall(s_execute_this_call_fn, execute_this_call_fn_args);

    std::string comment = fmt::Format("%s.%s", name, ArgsToString(args...).c_str());
    call_execute_this_call_fn_instr->setMetadata(comment, s_execute_this_call_fn_name_and_args_md_node);

    auto i = s_interpreter_invocation_stats.find(name);
    if (i == s_interpreter_invocation_stats.end()) {
        i = s_interpreter_invocation_stats.insert(s_interpreter_invocation_stats.end(), std::make_pair<std::string, u64>(name, 0));
    }

    i->second++;
}

void PPULLVMRecompiler::ExecuteThisCall(std::function<void()> * function) {
    (*function)();
}

template<class T, class... Args>
std::string PPULLVMRecompiler::ArgsToString(T arg1, Args... args) {
    return fmt::Format("%d", arg1) + "." + ArgsToString(args...);
}

std::string PPULLVMRecompiler::ArgsToString() {
    return "";
}
