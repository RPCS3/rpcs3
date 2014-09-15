#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
#include "llvm/IR/Verifier.h"
#include "llvm/CodeGen/MachineCodeInfo.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#define VERIFY_INSTRUCTION_AGAINST_INTERPRETER(fn, tc, input, ...) \
VerifyInstructionAgainstInterpreter(fmt::Format("%s.%d", #fn, tc).c_str(), &PPULLVMRecompiler::fn, &PPUInterpreter::fn, input, __VA_ARGS__)

#define VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(fn, s, n, ...) {  \
    PPURegState input;                                                              \
    for (int i = s; i < (n + s); i++) {                                             \
        input.SetRandom();                                                          \
        VERIFY_INSTRUCTION_AGAINST_INTERPRETER(fn, i, input, __VA_ARGS__);          \
    }                                                                               \
}

/// Register state of a PPU
struct PPURegState {
    /// Floating point registers
    PPCdouble FPR[32];

    ///Floating point status and control register
    FPSCRhdr FPSCR;

    /// General purpose reggisters
    u64 GPR[32];

    /// Vector purpose registers
    VPR_reg VPR[32];

    /// Condition register
    CRhdr CR;

    /// Fixed point exception register
    XERhdr XER;

    /// Vector status and control register
    VSCRhdr VSCR;

    /// Link register
    u64 LR;

    /// Count register
    u64 CTR;

    /// SPR general purpose registers
    u64 SPRG[8];

    /// Time base register
    u64 TB;

    void Load(PPUThread & ppu) {
        for (int i = 0; i < 32; i++) {
            FPR[i] = ppu.FPR[i];
            GPR[i] = ppu.GPR[i];
            VPR[i] = ppu.VPR[i];

            if (i < 8) {
                SPRG[i] = ppu.SPRG[i];
            }
        }

        FPSCR = ppu.FPSCR;
        CR    = ppu.CR;
        XER   = ppu.XER;
        VSCR  = ppu.VSCR;
        LR    = ppu.LR;
        CTR   = ppu.CTR;
        TB    = ppu.TB;
    }

    void Store(PPUThread & ppu) {
        for (int i = 0; i < 32; i++) {
            ppu.FPR[i] = FPR[i];
            ppu.GPR[i] = GPR[i];
            ppu.VPR[i] = VPR[i];

            if (i < 8) {
                ppu.SPRG[i] = SPRG[i];
            }
        }

        ppu.FPSCR = FPSCR;
        ppu.CR    = CR;
        ppu.XER   = XER;
        ppu.VSCR  = VSCR;
        ppu.LR    = LR;
        ppu.CTR   = CTR;
        ppu.TB    = TB;
    }

    void SetRandom() {
        std::mt19937_64 rng;

        rng.seed(std::mt19937_64::default_seed);
        for (int i = 0; i < 32; i++) {
            FPR[i]         = (double)rng();
            GPR[i]         = rng();
            VPR[i]._u64[0] = rng();
            VPR[i]._u64[1] = rng();

            if (i < 8) {
                SPRG[i] = rng();
            }
        }

        FPSCR.FPSCR = (u32)rng();
        CR.CR       = (u32)rng();
        XER.XER     = 0;
        XER.CA      = (u32)rng();
        XER.SO      = (u32)rng();
        XER.OV      = (u32)rng();
        VSCR.VSCR   = (u32)rng();
        VSCR.X      = 0;
        VSCR.Y      = 0;
        LR          = rng();
        CTR         = rng();
        TB          = rng();
    }

    std::string ToString() const {
        std::string ret;

        for (int i = 0; i < 32; i++) {
            ret += fmt::Format("GPR[%02d] = 0x%016llx FPR[%02d] = %16g VPR[%02d] = 0x%s [%s]\n", i, GPR[i], i, FPR[i]._double, i, VPR[i].ToString(true).c_str(), VPR[i].ToString().c_str());
        }

        for (int i = 0; i < 8; i++) {
            ret += fmt::Format("SPRG[%d] = 0x%016llx\n", i, SPRG[i]);
        }

        ret += fmt::Format("CR      = 0x%08x LR = 0x%016llx CTR = 0x%016llx TB=0x%016llx\n", CR.CR, LR, CTR, TB);
        ret += fmt::Format("XER     = 0x%016llx [CA=%d | OV=%d | SO=%d]\n", XER.XER, fmt::by_value(XER.CA), fmt::by_value(XER.OV), fmt::by_value(XER.SO));
        ret += fmt::Format("FPSCR   = 0x%08x "
                           "[RN=%d | NI=%d | XE=%d | ZE=%d | UE=%d | OE=%d | VE=%d | "
                           "VXCVI=%d | VXSQRT=%d | VXSOFT=%d | FPRF=%d | "
                           "FI=%d | FR=%d | VXVC=%d | VXIMZ=%d | "
                           "VXZDZ=%d | VXIDI=%d | VXISI=%d | VXSNAN=%d | "
                           "XX=%d | ZX=%d | UX=%d | OX=%d | VX=%d | FEX=%d | FX=%d]\n",
                           FPSCR.FPSCR,
                           fmt::by_value(FPSCR.RN),
                           fmt::by_value(FPSCR.NI), fmt::by_value(FPSCR.XE), fmt::by_value(FPSCR.ZE), fmt::by_value(FPSCR.UE), fmt::by_value(FPSCR.OE), fmt::by_value(FPSCR.VE),
                           fmt::by_value(FPSCR.VXCVI), fmt::by_value(FPSCR.VXSQRT), fmt::by_value(FPSCR.VXSOFT), fmt::by_value(FPSCR.FPRF),
                           fmt::by_value(FPSCR.FI), fmt::by_value(FPSCR.FR), fmt::by_value(FPSCR.VXVC), fmt::by_value(FPSCR.VXIMZ),
                           fmt::by_value(FPSCR.VXZDZ), fmt::by_value(FPSCR.VXIDI), fmt::by_value(FPSCR.VXISI), fmt::by_value(FPSCR.VXSNAN),
                           fmt::by_value(FPSCR.XX), fmt::by_value(FPSCR.ZX), fmt::by_value(FPSCR.UX), fmt::by_value(FPSCR.OX), fmt::by_value(FPSCR.VX), fmt::by_value(FPSCR.FEX), fmt::by_value(FPSCR.FX));
        ret += fmt::Format("VSCR    = 0x%08x [NJ=%d | SAT=%d]\n", VSCR.VSCR, fmt::by_value(VSCR.NJ), fmt::by_value(VSCR.SAT));

        return ret;
    }
};

template <class PPULLVMRecompilerFn, class PPUInterpreterFn, class... Args>
void PPULLVMRecompiler::VerifyInstructionAgainstInterpreter(const char * name, PPULLVMRecompilerFn recomp_fn, PPUInterpreterFn interp_fn, PPURegState & input_reg_state, Args... args) {
    auto test_case = [&]() {
        (this->*recomp_fn)(args...);
    };
    auto input = [&]() {
        input_reg_state.Store(m_ppu);
    };
    auto check_result = [&](std::string & msg) {
        PPURegState recomp_output_reg_state;
        PPURegState interp_output_reg_state;

        recomp_output_reg_state.Load(m_ppu);
        input_reg_state.Store(m_ppu);
        (&m_interpreter->*interp_fn)(args...);
        interp_output_reg_state.Load(m_ppu);

        if (interp_output_reg_state.ToString() != recomp_output_reg_state.ToString()) {
            msg = std::string("Input register states:\n") + input_reg_state.ToString() +
                  std::string("\nOutput register states:\n") + recomp_output_reg_state.ToString() +
                  std::string("\nInterpreter output register states:\n") + interp_output_reg_state.ToString();
            return false;
        }

        return true;
    };
    RunTest(name, test_case, input, check_result);
}

void PPULLVMRecompiler::RunTest(const char * name, std::function<void()> test_case, std::function<void()> input, std::function<bool(std::string & msg)> check_result) {
    // Create the unit test function
    auto function = cast<Function>(m_module->getOrInsertFunction(name, Type::getVoidTy(m_llvm_context), (Type *)nullptr));
    auto block    = BasicBlock::Create(m_llvm_context, "start", function);
    m_ir_builder.SetInsertPoint(block);
    test_case();
    m_ir_builder.CreateRetVoid();
    verifyFunction(*function);

    // Print the IR
    std::string        ir;
    raw_string_ostream ir_ostream(ir);
    function->print(ir_ostream);
    LOG_NOTICE(PPU, "[UT %s] LLVM IR:%s", name, ir.c_str());

    // Generate the function
    MachineCodeInfo mci;
    m_execution_engine->runJITOnFunction(function, &mci);

    // Disassember the generated function
    LOG_NOTICE(PPU, "[UT %s] Disassembly:", name);
    for (uint64_t pc = 0; pc < mci.size();) {
        char str[1024];

        auto size = LLVMDisasmInstruction(m_disassembler, (uint8_t *)mci.address() + pc, mci.size() - pc, (uint64_t)((uint8_t *)mci.address() + pc), str, sizeof(str));
        LOG_NOTICE(PPU, "[UT %s] %p: %s.", name, (uint8_t *)mci.address() + pc, str);
        pc += size;
    }

    // Run the test
    input();
    std::vector<GenericValue> args;
    m_execution_engine->runFunction(function, args);

    // Verify results
    std::string msg;
    bool        pass = check_result(msg);
    if (pass) {
        LOG_NOTICE(PPU, "[UT %s] Test passed. %s", name, msg.c_str());
    } else {
        LOG_ERROR(PPU, "[UT %s] Test failed. %s", name, msg.c_str());
    }

    m_execution_engine->freeMachineCodeForFunction(function);
}

void PPULLVMRecompiler::RunAllTests() {
    std::function<void()>                  test_case;
    std::function<void()>                  input;
    std::function<bool(std::string & msg)> check_result;

    LOG_NOTICE(PPU, "Running Unit Tests");

    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(MFVSCR, 0, 5, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(MTVSCR, 0, 5, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDCUW, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDFP, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDSBS, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDSHS, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDSWS, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDUBM, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDUBS, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDUHM, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDUHS, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDUWM, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VADDUWS, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VAND, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VANDC, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VAVGSB, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VAVGSH, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VAVGSW, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VAVGUB, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VAVGUH, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VAVGUW, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCFSX, 0, 5, 0, 0, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCFSX, 5, 5, 0, 3, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCFUX, 0, 5, 0, 0, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCFUX, 5, 5, 0, 2, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPBFP, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPBFP, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPBFP_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPBFP_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQFP, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQFP, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQFP_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQFP_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUB, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUB, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUB_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUB_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUH, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUH, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUH_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUH_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUW, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUW, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUW_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPEQUW_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGEFP, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGEFP, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGEFP_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGEFP_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTFP, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTFP, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTFP_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTFP_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSB, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSB, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSB_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSB_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSH, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSH, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSH_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSH_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSW, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSW, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSW_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTSW_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUB, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUB, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUB_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUB_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUH, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUH, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUH_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUH_, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUW, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUW, 5, 5, 0, 1, 1);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUW_, 0, 5, 0, 1, 2);
    VERIFY_INSTRUCTION_AGAINST_INTERPRETER_USING_RANDOM_INPUT(VCMPGTUW_, 5, 5, 0, 1, 1);
}
