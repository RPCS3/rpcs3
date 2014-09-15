#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/CodeGen/MachineCodeInfo.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

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
        LOG_NOTICE(PPU, "[UT %s] Test passed. %s.", name, msg.c_str());
    } else {
        LOG_ERROR(PPU, "[UT %s] Test failed. %s.", name, msg.c_str());
    }

    m_execution_engine->freeMachineCodeForFunction(function);
}

void PPULLVMRecompiler::RunAllTests() {
    std::function<void()>                  test_case;
    std::function<void()>                  input;
    std::function<bool(std::string & msg)> check_result;

    LOG_NOTICE(PPU, "Running Unit Tests");

    ///////////////////////////////////////////////////////////////////////////
    test_case = [this]() {
        MFVSCR(1);
    };
    input = [this]() {
        m_ppu.VPR[1]._u32[0] = m_ppu.VPR[1]._u32[1] = m_ppu.VPR[1]._u32[2] = m_ppu.VPR[1]._u32[3] = 0x9ABCDEF0;
        m_ppu.VSCR.VSCR      = 0x12345678;
    };
    check_result = [this](std::string & msg) {
        msg = fmt::Format("VPR[1]=%s", m_ppu.VPR[1].ToString(true).c_str());
        return m_ppu.VPR[1].Equals((u32)0x12345678, (u32)0, (u32)0, (u32)0);
    };
    RunTest("MFVSCR.1", test_case, input, check_result);

    ///////////////////////////////////////////////////////////////////////////
    test_case = [this]() {
        MTVSCR(1);
    };
    input = [this]() {
        m_ppu.VPR[1]._u32[0] = 0x9ABCDEF0;
        m_ppu.VPR[1]._u32[1] = m_ppu.VPR[1]._u32[2] = m_ppu.VPR[1]._u32[3] = 0x11112222;
        m_ppu.VSCR.VSCR = 0x12345678;
    };
    check_result = [this](std::string & msg) {
        msg = fmt::Format("VPR[1]=0x%s, VSCR=0x%lX", m_ppu.VPR[1].ToString(true).c_str(), m_ppu.VSCR.VSCR);
        return m_ppu.VSCR.VSCR == 0x9ABCDEF0;
    };
    RunTest("MTVSCR.1", test_case, input, check_result);

    ///////////////////////////////////////////////////////////////////////////
    test_case = [this]() {
        VADDCUW(0, 1, 2);
    };
    input = [this]() {
        m_ppu.VPR[0]._u32[0] = m_ppu.VPR[0]._u32[1] = m_ppu.VPR[0]._u32[2] = m_ppu.VPR[0]._u32[3] = 0x9ABCDEF0;
        m_ppu.VPR[1]._u32[0] = m_ppu.VPR[1]._u32[1] = m_ppu.VPR[1]._u32[2] = m_ppu.VPR[1]._u32[3] = 0x99999999;
        m_ppu.VPR[2]._u32[0] = m_ppu.VPR[2]._u32[1] = 0x77777777;
        m_ppu.VPR[2]._u32[2] = m_ppu.VPR[2]._u32[3] = 1;
    };
    check_result = [this](std::string & msg) {
        msg = fmt::Format("VPR[0]=0x%s, VPR[1]=0x%s, VPR[2]=0x%s",
                          m_ppu.VPR[0].ToString(true).c_str(),
                          m_ppu.VPR[1].ToString(true).c_str(),
                          m_ppu.VPR[2].ToString(true).c_str());
        return m_ppu.VPR[0].Equals((u32)1, (u32)1, (u32)0, (u32)0);
    };
    RunTest("VADDCUW.1", test_case, input, check_result);

    ///////////////////////////////////////////////////////////////////////////
    test_case = [this]() {
        VADDFP(0, 1, 2);
    };
    input = [this]() {
        m_ppu.VPR[0]._f[0] = m_ppu.VPR[0]._f[1] = m_ppu.VPR[0]._f[2] = m_ppu.VPR[0]._f[3] = 100.0f;
        m_ppu.VPR[1]._f[0] = m_ppu.VPR[1]._f[1] = m_ppu.VPR[1]._f[2] = m_ppu.VPR[1]._f[3] = 500.0f;
        m_ppu.VPR[2]._f[0] = m_ppu.VPR[2]._f[1] = m_ppu.VPR[2]._f[2] = m_ppu.VPR[2]._f[3] = 900.0f;
    };
    check_result = [this](std::string & msg) {
        msg = fmt::Format("VPR[0]=%s, VPR[1]=%s, VPR[2]=%s",
                          m_ppu.VPR[0].ToString().c_str(),
                          m_ppu.VPR[1].ToString().c_str(),
                          m_ppu.VPR[2].ToString().c_str());
        return m_ppu.VPR[0].Equals(1400.0f, 1400.0f, 1400.0f, 1400.0f);
    };
    RunTest("VADDFP.1", test_case, input, check_result);
}
