#ifndef PPU_LLVM_RECOMPILER_H
#define PPU_LLVM_RECOMPILER_H

#ifdef LLVM_AVAILABLE
#define PPU_LLVM_RECOMPILER 1

#include <list>
#include "Emu/Cell/PPUDecoder.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUInterpreter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/PassManager.h"

namespace ppu_recompiler_llvm {
    class Compiler;
    class RecompilationEngine;
    class Tracer;
    class ExecutionEngine;
    struct PPUState;

    /// An entry in an execution trace
    struct ExecutionTraceEntry {
        /// Data associated with the entry. This is discriminated by type.
        union {
            struct Instruction {
                u32 address;
            } instruction;

            struct FunctionCall {
                u32 address;
            } function_call;

            struct CompiledBlock {
                u32 entry_address;
                u32 exit_address;
            } compiled_block;
        } data;

        /// The type of the entry
        enum class Type {
            FunctionCall,
            Instruction,
            CompiledBlock,
        } type;

        ExecutionTraceEntry(Type type, u32 arg1, u32 arg2 = 0)
            : type(type) {
            switch (type) {
            case Type::Instruction:
                data.instruction.address = arg1;
                break;
            case Type::FunctionCall:
                data.function_call.address = arg1;
                break;
            case Type::CompiledBlock:
                data.compiled_block.entry_address = arg1;
                data.compiled_block.exit_address  = arg2;
                break;
            default:
                assert(0);
                break;
            }
        }

        u32 GetPrimaryAddress() const {
            switch (type) {
            case Type::Instruction:
                return data.instruction.address;
            case Type::FunctionCall:
                return data.function_call.address;
            case Type::CompiledBlock:
                return data.compiled_block.entry_address;
            default:
                assert(0);
                return 0;
            }
        }

        std::string ToString() const {
            switch (type) {
            case Type::Instruction:
                return fmt::Format("I:0x%08X", data.instruction.address);
            case Type::FunctionCall:
                return fmt::Format("F:0x%08X", data.function_call.address);
            case Type::CompiledBlock:
                return fmt::Format("C:0x%08X-0x%08X", data.compiled_block.entry_address, data.compiled_block.exit_address);
            default:
                assert(0);
                return "";
            }
        }

        u64 hash() const {
            u64 hash = ((u64)type << 32);
            switch (type) {
            case Type::Instruction:
                hash |= data.instruction.address;
                break;
            case Type::FunctionCall:
                hash |= data.function_call.address;
                break;
            case Type::CompiledBlock:
                hash   = data.compiled_block.exit_address;
                hash <<= 32;
                hash  |= data.compiled_block.entry_address;
                break;
            default:
                assert(0);
                break;
            }

            return hash;
        }
    };

    /// An execution trace.
    struct ExecutionTrace {
        /// Unique id of an execution trace;
        typedef u64 Id;

        /// The function to which this trace belongs
        u32 function_address;

        /// Execution trace type
        enum class Type {
            Linear,
            Loop,
        } type;

        /// entries in the trace
        std::vector<ExecutionTraceEntry> entries;

        ExecutionTrace(u32 address)
            : function_address(address) {
        }

        std::string ToString() const {
            auto s = fmt::Format("0x%08X %s ->", function_address, type == ExecutionTrace::Type::Loop ? "Loop" : "Linear");
            for (auto i = 0; i < entries.size(); i++) {
                s += " " + entries[i].ToString();
            }

            return s;
        }

        Id GetId() const {
            Id id = 0;

            for (auto i = entries.begin(); i != entries.end(); i++) {
                id  ^= i->hash();
                id <<= 1;
            }

            return id;
        }
    };

    /// A control flow graph
    struct ControlFlowGraph {
        /// Address of the first instruction
        u32 start_address;

        /// Address of the function to which this CFG belongs to
        u32 function_address;

        /// Set of addresses of the instructions in the CFG
        std::set<u32> instruction_addresses;

        /// Branches in the CFG.
        /// Key is the address of an instruction
        /// Data is the set of all instructions to which this instruction branches to.
        std::map<u32, std::set<u32>> branches;

        /// Function calls in the CFG
        /// Key is the address of an instruction
        /// Data is the set of all functions which this instruction invokes.
        std::map<u32, std::set<u32>> calls;

        ControlFlowGraph(u32 start_address, u32 function_address)
            : start_address(start_address)
            , function_address(function_address) {
        }

        void operator += (const ControlFlowGraph & other) {
            for (auto i = other.instruction_addresses.begin(); i != other.instruction_addresses.end(); i++) {
                instruction_addresses.insert(*i);
            }

            for (auto i = other.branches.begin(); i != other.branches.end(); i++) {
                auto j = branches.find(i->first);
                if (j == branches.end()) {
                    j = branches.insert(branches.begin(), std::make_pair(i->first, std::set<u32>()));
                }

                for (auto k = i->second.begin(); k != i->second.end(); k++) {
                    j->second.insert(*k);
                }
            }

            for (auto i = other.calls.begin(); i != other.calls.end(); i++) {
                auto j = calls.find(i->first);
                if (j == calls.end()) {
                    j = calls.insert(calls.begin(), std::make_pair(i->first, std::set<u32>()));
                }

                for (auto k = i->second.begin(); k != i->second.end(); k++) {
                    j->second.insert(*k);
                }
            }
        }

        std::string ToString() const {
            auto s = fmt::Format("0x%08X (0x%08X): Size=%u ->", start_address, function_address, GetSize());
            for (auto i = instruction_addresses.begin(); i != instruction_addresses.end(); i++) {
                s += fmt::Format(" 0x%08X", *i);
            }

            s += "\nBranches:";
            for (auto i = branches.begin(); i != branches.end(); i++) {
                s += fmt::Format("\n0x%08X ->", i->first);
                for (auto j = i->second.begin(); j != i->second.end(); j++) {
                    s += fmt::Format(" 0x%08X", *j);
                }
            }

            s += "\nCalls:";
            for (auto i = calls.begin(); i != calls.end(); i++) {
                s += fmt::Format("\n0x%08X ->", i->first);
                for (auto j = i->second.begin(); j != i->second.end(); j++) {
                    s += fmt::Format(" 0x%08X", *j);
                }
            }

            return s;
        }

        /// Get the size of the CFG. The size is a score of how large the CFG is and increases everytime
        /// a node or an edge is added to the CFG.
        size_t GetSize() const {
            return instruction_addresses.size() + branches.size() + calls.size();
        }
    };

    enum class BranchType {
        NonBranch,
        LocalBranch,
        FunctionCall,
        Return,
    };

    /// Pointer to an executable
    typedef u32(*Executable)(PPUThread * ppu_state, u64 context);

    /// PPU compiler that uses LLVM for code generation and optimization
    class Compiler : protected PPUOpcodes, protected PPCDecoder {
    public:
        struct Stats {
            /// Time spent building the LLVM IR
            std::chrono::nanoseconds ir_build_time;

            /// Time spent optimizing
            std::chrono::nanoseconds optimization_time;

            /// Time spent translating LLVM IR to machine code
            std::chrono::nanoseconds translation_time;

            /// Total time
            std::chrono::nanoseconds total_time;
        };

        Compiler(RecompilationEngine & recompilation_engine, const Executable execute_unknown_function,
                 const Executable execute_unknown_block, bool (*poll_status_function)(PPUThread * ppu_state));

        Compiler(const Compiler & other) = delete;
        Compiler(Compiler && other) = delete;

        virtual ~Compiler();

        Compiler & operator = (const Compiler & other) = delete;
        Compiler & operator = (Compiler && other) = delete;

        /// Compile a code fragment described by a cfg and return an executable
        Executable Compile(const std::string & name, const ControlFlowGraph & cfg, bool inline_all_blocks, bool generate_linkable_exits);

        /// Free an executable earilier obtained via a call to Compile
        void FreeExecutable(const std::string & name);

        /// Retrieve compiler stats
        Stats GetStats();

        /// Execute all tests
        void RunAllTests();

    protected:
        void Decode(const u32 code) override;

        void NULL_OP() override;
        void NOP() override;

        void TDI(u32 to, u32 ra, s32 simm16) override;
        void TWI(u32 to, u32 ra, s32 simm16) override;

        void MFVSCR(u32 vd) override;
        void MTVSCR(u32 vb) override;
        void VADDCUW(u32 vd, u32 va, u32 vb) override;
        void VADDFP(u32 vd, u32 va, u32 vb) override;
        void VADDSBS(u32 vd, u32 va, u32 vb) override;
        void VADDSHS(u32 vd, u32 va, u32 vb) override;
        void VADDSWS(u32 vd, u32 va, u32 vb) override;
        void VADDUBM(u32 vd, u32 va, u32 vb) override;
        void VADDUBS(u32 vd, u32 va, u32 vb) override;
        void VADDUHM(u32 vd, u32 va, u32 vb) override;
        void VADDUHS(u32 vd, u32 va, u32 vb) override;
        void VADDUWM(u32 vd, u32 va, u32 vb) override;
        void VADDUWS(u32 vd, u32 va, u32 vb) override;
        void VAND(u32 vd, u32 va, u32 vb) override;
        void VANDC(u32 vd, u32 va, u32 vb) override;
        void VAVGSB(u32 vd, u32 va, u32 vb) override;
        void VAVGSH(u32 vd, u32 va, u32 vb) override;
        void VAVGSW(u32 vd, u32 va, u32 vb) override;
        void VAVGUB(u32 vd, u32 va, u32 vb) override;
        void VAVGUH(u32 vd, u32 va, u32 vb) override;
        void VAVGUW(u32 vd, u32 va, u32 vb) override;
        void VCFSX(u32 vd, u32 uimm5, u32 vb) override;
        void VCFUX(u32 vd, u32 uimm5, u32 vb) override;
        void VCMPBFP(u32 vd, u32 va, u32 vb) override;
        void VCMPBFP_(u32 vd, u32 va, u32 vb) override;
        void VCMPEQFP(u32 vd, u32 va, u32 vb) override;
        void VCMPEQFP_(u32 vd, u32 va, u32 vb) override;
        void VCMPEQUB(u32 vd, u32 va, u32 vb) override;
        void VCMPEQUB_(u32 vd, u32 va, u32 vb) override;
        void VCMPEQUH(u32 vd, u32 va, u32 vb) override;
        void VCMPEQUH_(u32 vd, u32 va, u32 vb) override;
        void VCMPEQUW(u32 vd, u32 va, u32 vb) override;
        void VCMPEQUW_(u32 vd, u32 va, u32 vb) override;
        void VCMPGEFP(u32 vd, u32 va, u32 vb) override;
        void VCMPGEFP_(u32 vd, u32 va, u32 vb) override;
        void VCMPGTFP(u32 vd, u32 va, u32 vb) override;
        void VCMPGTFP_(u32 vd, u32 va, u32 vb) override;
        void VCMPGTSB(u32 vd, u32 va, u32 vb) override;
        void VCMPGTSB_(u32 vd, u32 va, u32 vb) override;
        void VCMPGTSH(u32 vd, u32 va, u32 vb) override;
        void VCMPGTSH_(u32 vd, u32 va, u32 vb) override;
        void VCMPGTSW(u32 vd, u32 va, u32 vb) override;
        void VCMPGTSW_(u32 vd, u32 va, u32 vb) override;
        void VCMPGTUB(u32 vd, u32 va, u32 vb) override;
        void VCMPGTUB_(u32 vd, u32 va, u32 vb) override;
        void VCMPGTUH(u32 vd, u32 va, u32 vb) override;
        void VCMPGTUH_(u32 vd, u32 va, u32 vb) override;
        void VCMPGTUW(u32 vd, u32 va, u32 vb) override;
        void VCMPGTUW_(u32 vd, u32 va, u32 vb) override;
        void VCTSXS(u32 vd, u32 uimm5, u32 vb) override;
        void VCTUXS(u32 vd, u32 uimm5, u32 vb) override;
        void VEXPTEFP(u32 vd, u32 vb) override;
        void VLOGEFP(u32 vd, u32 vb) override;
        void VMADDFP(u32 vd, u32 va, u32 vc, u32 vb) override;
        void VMAXFP(u32 vd, u32 va, u32 vb) override;
        void VMAXSB(u32 vd, u32 va, u32 vb) override;
        void VMAXSH(u32 vd, u32 va, u32 vb) override;
        void VMAXSW(u32 vd, u32 va, u32 vb) override;
        void VMAXUB(u32 vd, u32 va, u32 vb) override;
        void VMAXUH(u32 vd, u32 va, u32 vb) override;
        void VMAXUW(u32 vd, u32 va, u32 vb) override;
        void VMHADDSHS(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMHRADDSHS(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMINFP(u32 vd, u32 va, u32 vb) override;
        void VMINSB(u32 vd, u32 va, u32 vb) override;
        void VMINSH(u32 vd, u32 va, u32 vb) override;
        void VMINSW(u32 vd, u32 va, u32 vb) override;
        void VMINUB(u32 vd, u32 va, u32 vb) override;
        void VMINUH(u32 vd, u32 va, u32 vb) override;
        void VMINUW(u32 vd, u32 va, u32 vb) override;
        void VMLADDUHM(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMRGHB(u32 vd, u32 va, u32 vb) override;
        void VMRGHH(u32 vd, u32 va, u32 vb) override;
        void VMRGHW(u32 vd, u32 va, u32 vb) override;
        void VMRGLB(u32 vd, u32 va, u32 vb) override;
        void VMRGLH(u32 vd, u32 va, u32 vb) override;
        void VMRGLW(u32 vd, u32 va, u32 vb) override;
        void VMSUMMBM(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMSUMSHM(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMSUMSHS(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMSUMUBM(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMSUMUHM(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMSUMUHS(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VMULESB(u32 vd, u32 va, u32 vb) override;
        void VMULESH(u32 vd, u32 va, u32 vb) override;
        void VMULEUB(u32 vd, u32 va, u32 vb) override;
        void VMULEUH(u32 vd, u32 va, u32 vb) override;
        void VMULOSB(u32 vd, u32 va, u32 vb) override;
        void VMULOSH(u32 vd, u32 va, u32 vb) override;
        void VMULOUB(u32 vd, u32 va, u32 vb) override;
        void VMULOUH(u32 vd, u32 va, u32 vb) override;
        void VNMSUBFP(u32 vd, u32 va, u32 vc, u32 vb) override;
        void VNOR(u32 vd, u32 va, u32 vb) override;
        void VOR(u32 vd, u32 va, u32 vb) override;
        void VPERM(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VPKPX(u32 vd, u32 va, u32 vb) override;
        void VPKSHSS(u32 vd, u32 va, u32 vb) override;
        void VPKSHUS(u32 vd, u32 va, u32 vb) override;
        void VPKSWSS(u32 vd, u32 va, u32 vb) override;
        void VPKSWUS(u32 vd, u32 va, u32 vb) override;
        void VPKUHUM(u32 vd, u32 va, u32 vb) override;
        void VPKUHUS(u32 vd, u32 va, u32 vb) override;
        void VPKUWUM(u32 vd, u32 va, u32 vb) override;
        void VPKUWUS(u32 vd, u32 va, u32 vb) override;
        void VREFP(u32 vd, u32 vb) override;
        void VRFIM(u32 vd, u32 vb) override;
        void VRFIN(u32 vd, u32 vb) override;
        void VRFIP(u32 vd, u32 vb) override;
        void VRFIZ(u32 vd, u32 vb) override;
        void VRLB(u32 vd, u32 va, u32 vb) override;
        void VRLH(u32 vd, u32 va, u32 vb) override;
        void VRLW(u32 vd, u32 va, u32 vb) override;
        void VRSQRTEFP(u32 vd, u32 vb) override;
        void VSEL(u32 vd, u32 va, u32 vb, u32 vc) override;
        void VSL(u32 vd, u32 va, u32 vb) override;
        void VSLB(u32 vd, u32 va, u32 vb) override;
        void VSLDOI(u32 vd, u32 va, u32 vb, u32 sh) override;
        void VSLH(u32 vd, u32 va, u32 vb) override;
        void VSLO(u32 vd, u32 va, u32 vb) override;
        void VSLW(u32 vd, u32 va, u32 vb) override;
        void VSPLTB(u32 vd, u32 uimm5, u32 vb) override;
        void VSPLTH(u32 vd, u32 uimm5, u32 vb) override;
        void VSPLTISB(u32 vd, s32 simm5) override;
        void VSPLTISH(u32 vd, s32 simm5) override;
        void VSPLTISW(u32 vd, s32 simm5) override;
        void VSPLTW(u32 vd, u32 uimm5, u32 vb) override;
        void VSR(u32 vd, u32 va, u32 vb) override;
        void VSRAB(u32 vd, u32 va, u32 vb) override;
        void VSRAH(u32 vd, u32 va, u32 vb) override;
        void VSRAW(u32 vd, u32 va, u32 vb) override;
        void VSRB(u32 vd, u32 va, u32 vb) override;
        void VSRH(u32 vd, u32 va, u32 vb) override;
        void VSRO(u32 vd, u32 va, u32 vb) override;
        void VSRW(u32 vd, u32 va, u32 vb) override;
        void VSUBCUW(u32 vd, u32 va, u32 vb) override;
        void VSUBFP(u32 vd, u32 va, u32 vb) override;
        void VSUBSBS(u32 vd, u32 va, u32 vb) override;
        void VSUBSHS(u32 vd, u32 va, u32 vb) override;
        void VSUBSWS(u32 vd, u32 va, u32 vb) override;
        void VSUBUBM(u32 vd, u32 va, u32 vb) override;
        void VSUBUBS(u32 vd, u32 va, u32 vb) override;
        void VSUBUHM(u32 vd, u32 va, u32 vb) override;
        void VSUBUHS(u32 vd, u32 va, u32 vb) override;
        void VSUBUWM(u32 vd, u32 va, u32 vb) override;
        void VSUBUWS(u32 vd, u32 va, u32 vb) override;
        void VSUMSWS(u32 vd, u32 va, u32 vb) override;
        void VSUM2SWS(u32 vd, u32 va, u32 vb) override;
        void VSUM4SBS(u32 vd, u32 va, u32 vb) override;
        void VSUM4SHS(u32 vd, u32 va, u32 vb) override;
        void VSUM4UBS(u32 vd, u32 va, u32 vb) override;
        void VUPKHPX(u32 vd, u32 vb) override;
        void VUPKHSB(u32 vd, u32 vb) override;
        void VUPKHSH(u32 vd, u32 vb) override;
        void VUPKLPX(u32 vd, u32 vb) override;
        void VUPKLSB(u32 vd, u32 vb) override;
        void VUPKLSH(u32 vd, u32 vb) override;
        void VXOR(u32 vd, u32 va, u32 vb) override;
        void MULLI(u32 rd, u32 ra, s32 simm16) override;
        void SUBFIC(u32 rd, u32 ra, s32 simm16) override;
        void CMPLI(u32 bf, u32 l, u32 ra, u32 uimm16) override;
        void CMPI(u32 bf, u32 l, u32 ra, s32 simm16) override;
        void ADDIC(u32 rd, u32 ra, s32 simm16) override;
        void ADDIC_(u32 rd, u32 ra, s32 simm16) override;
        void ADDI(u32 rd, u32 ra, s32 simm16) override;
        void ADDIS(u32 rd, u32 ra, s32 simm16) override;
        void BC(u32 bo, u32 bi, s32 bd, u32 aa, u32 lk) override;
        void HACK(u32 id) override;
        void SC(u32 sc_code) override;
        void B(s32 ll, u32 aa, u32 lk) override;
        void MCRF(u32 crfd, u32 crfs) override;
        void BCLR(u32 bo, u32 bi, u32 bh, u32 lk) override;
        void CRNOR(u32 bt, u32 ba, u32 bb) override;
        void CRANDC(u32 bt, u32 ba, u32 bb) override;
        void ISYNC() override;
        void CRXOR(u32 bt, u32 ba, u32 bb) override;
        void CRNAND(u32 bt, u32 ba, u32 bb) override;
        void CRAND(u32 bt, u32 ba, u32 bb) override;
        void CREQV(u32 bt, u32 ba, u32 bb) override;
        void CRORC(u32 bt, u32 ba, u32 bb) override;
        void CROR(u32 bt, u32 ba, u32 bb) override;
        void BCCTR(u32 bo, u32 bi, u32 bh, u32 lk) override;
        void RLWIMI(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) override;
        void RLWINM(u32 ra, u32 rs, u32 sh, u32 mb, u32 me, bool rc) override;
        void RLWNM(u32 ra, u32 rs, u32 rb, u32 MB, u32 ME, bool rc) override;
        void ORI(u32 rs, u32 ra, u32 uimm16) override;
        void ORIS(u32 rs, u32 ra, u32 uimm16) override;
        void XORI(u32 ra, u32 rs, u32 uimm16) override;
        void XORIS(u32 ra, u32 rs, u32 uimm16) override;
        void ANDI_(u32 ra, u32 rs, u32 uimm16) override;
        void ANDIS_(u32 ra, u32 rs, u32 uimm16) override;
        void RLDICL(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) override;
        void RLDICR(u32 ra, u32 rs, u32 sh, u32 me, bool rc) override;
        void RLDIC(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) override;
        void RLDIMI(u32 ra, u32 rs, u32 sh, u32 mb, bool rc) override;
        void RLDC_LR(u32 ra, u32 rs, u32 rb, u32 m_eb, bool is_r, bool rc) override;
        void CMP(u32 crfd, u32 l, u32 ra, u32 rb) override;
        void TW(u32 to, u32 ra, u32 rb) override;
        void LVSL(u32 vd, u32 ra, u32 rb) override;
        void LVEBX(u32 vd, u32 ra, u32 rb) override;
        void SUBFC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void MULHDU(u32 rd, u32 ra, u32 rb, bool rc) override;
        void ADDC(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void MULHWU(u32 rd, u32 ra, u32 rb, bool rc) override;
        void MFOCRF(u32 a, u32 rd, u32 crm) override;
        void LWARX(u32 rd, u32 ra, u32 rb) override;
        void LDX(u32 ra, u32 rs, u32 rb) override;
        void LWZX(u32 rd, u32 ra, u32 rb) override;
        void SLW(u32 ra, u32 rs, u32 rb, bool rc) override;
        void CNTLZW(u32 ra, u32 rs, bool rc) override;
        void SLD(u32 ra, u32 rs, u32 rb, bool rc) override;
        void AND(u32 ra, u32 rs, u32 rb, bool rc) override;
        void CMPL(u32 bf, u32 l, u32 ra, u32 rb) override;
        void LVSR(u32 vd, u32 ra, u32 rb) override;
        void LVEHX(u32 vd, u32 ra, u32 rb) override;
        void SUBF(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void LDUX(u32 rd, u32 ra, u32 rb) override;
        void DCBST(u32 ra, u32 rb) override;
        void LWZUX(u32 rd, u32 ra, u32 rb) override;
        void CNTLZD(u32 ra, u32 rs, bool rc) override;
        void ANDC(u32 ra, u32 rs, u32 rb, bool rc) override;
        void TD(u32 to, u32 ra, u32 rb) override;
        void LVEWX(u32 vd, u32 ra, u32 rb) override;
        void MULHD(u32 rd, u32 ra, u32 rb, bool rc) override;
        void MULHW(u32 rd, u32 ra, u32 rb, bool rc) override;
        void LDARX(u32 rd, u32 ra, u32 rb) override;
        void DCBF(u32 ra, u32 rb) override;
        void LBZX(u32 rd, u32 ra, u32 rb) override;
        void LVX(u32 vd, u32 ra, u32 rb) override;
        void NEG(u32 rd, u32 ra, u32 oe, bool rc) override;
        void LBZUX(u32 rd, u32 ra, u32 rb) override;
        void NOR(u32 ra, u32 rs, u32 rb, bool rc) override;
        void STVEBX(u32 vs, u32 ra, u32 rb) override;
        void SUBFE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void ADDE(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void MTOCRF(u32 l, u32 crm, u32 rs) override;
        void STDX(u32 rs, u32 ra, u32 rb) override;
        void STWCX_(u32 rs, u32 ra, u32 rb) override;
        void STWX(u32 rs, u32 ra, u32 rb) override;
        void STVEHX(u32 vs, u32 ra, u32 rb) override;
        void STDUX(u32 rs, u32 ra, u32 rb) override;
        void STWUX(u32 rs, u32 ra, u32 rb) override;
        void STVEWX(u32 vs, u32 ra, u32 rb) override;
        void SUBFZE(u32 rd, u32 ra, u32 oe, bool rc) override;
        void ADDZE(u32 rd, u32 ra, u32 oe, bool rc) override;
        void STDCX_(u32 rs, u32 ra, u32 rb) override;
        void STBX(u32 rs, u32 ra, u32 rb) override;
        void STVX(u32 vs, u32 ra, u32 rb) override;
        void MULLD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void SUBFME(u32 rd, u32 ra, u32 oe, bool rc) override;
        void ADDME(u32 rd, u32 ra, u32 oe, bool rc) override;
        void MULLW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void DCBTST(u32 ra, u32 rb, u32 th) override;
        void STBUX(u32 rs, u32 ra, u32 rb) override;
        void ADD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void DCBT(u32 ra, u32 rb, u32 th) override;
        void LHZX(u32 rd, u32 ra, u32 rb) override;
        void EQV(u32 ra, u32 rs, u32 rb, bool rc) override;
        void ECIWX(u32 rd, u32 ra, u32 rb) override;
        void LHZUX(u32 rd, u32 ra, u32 rb) override;
        void XOR(u32 rs, u32 ra, u32 rb, bool rc) override;
        void MFSPR(u32 rd, u32 spr) override;
        void LWAX(u32 rd, u32 ra, u32 rb) override;
        void DST(u32 ra, u32 rb, u32 strm, u32 t) override;
        void LHAX(u32 rd, u32 ra, u32 rb) override;
        void LVXL(u32 vd, u32 ra, u32 rb) override;
        void MFTB(u32 rd, u32 spr) override;
        void LWAUX(u32 rd, u32 ra, u32 rb) override;
        void DSTST(u32 ra, u32 rb, u32 strm, u32 t) override;
        void LHAUX(u32 rd, u32 ra, u32 rb) override;
        void STHX(u32 rs, u32 ra, u32 rb) override;
        void ORC(u32 rs, u32 ra, u32 rb, bool rc) override;
        void ECOWX(u32 rs, u32 ra, u32 rb) override;
        void STHUX(u32 rs, u32 ra, u32 rb) override;
        void OR(u32 ra, u32 rs, u32 rb, bool rc) override;
        void DIVDU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void DIVWU(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void MTSPR(u32 spr, u32 rs) override;
        void DCBI(u32 ra, u32 rb) override;
        void NAND(u32 ra, u32 rs, u32 rb, bool rc) override;
        void STVXL(u32 vs, u32 ra, u32 rb) override;
        void DIVD(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void DIVW(u32 rd, u32 ra, u32 rb, u32 oe, bool rc) override;
        void LVLX(u32 vd, u32 ra, u32 rb) override;
        void LDBRX(u32 rd, u32 ra, u32 rb) override;
        void LSWX(u32 rd, u32 ra, u32 rb) override;
        void LWBRX(u32 rd, u32 ra, u32 rb) override;
        void LFSX(u32 frd, u32 ra, u32 rb) override;
        void SRW(u32 ra, u32 rs, u32 rb, bool rc) override;
        void SRD(u32 ra, u32 rs, u32 rb, bool rc) override;
        void LVRX(u32 vd, u32 ra, u32 rb) override;
        void LSWI(u32 rd, u32 ra, u32 nb) override;
        void LFSUX(u32 frd, u32 ra, u32 rb) override;
        void SYNC(u32 l) override;
        void LFDX(u32 frd, u32 ra, u32 rb) override;
        void LFDUX(u32 frd, u32 ra, u32 rb) override;
        void STVLX(u32 vs, u32 ra, u32 rb) override;
        void STDBRX(u32 rd, u32 ra, u32 rb) override;
        void STSWX(u32 rs, u32 ra, u32 rb) override;
        void STWBRX(u32 rs, u32 ra, u32 rb) override;
        void STFSX(u32 frs, u32 ra, u32 rb) override;
        void STVRX(u32 vs, u32 ra, u32 rb) override;
        void STFSUX(u32 frs, u32 ra, u32 rb) override;
        void STSWI(u32 rd, u32 ra, u32 nb) override;
        void STFDX(u32 frs, u32 ra, u32 rb) override;
        void STFDUX(u32 frs, u32 ra, u32 rb) override;
        void LVLXL(u32 vd, u32 ra, u32 rb) override;
        void LHBRX(u32 rd, u32 ra, u32 rb) override;
        void SRAW(u32 ra, u32 rs, u32 rb, bool rc) override;
        void SRAD(u32 ra, u32 rs, u32 rb, bool rc) override;
        void LVRXL(u32 vd, u32 ra, u32 rb) override;
        void DSS(u32 strm, u32 a) override;
        void SRAWI(u32 ra, u32 rs, u32 sh, bool rc) override;
        void SRADI1(u32 ra, u32 rs, u32 sh, bool rc) override;
        void SRADI2(u32 ra, u32 rs, u32 sh, bool rc) override;
        void EIEIO() override;
        void STVLXL(u32 vs, u32 ra, u32 rb) override;
        void STHBRX(u32 rs, u32 ra, u32 rb) override;
        void EXTSH(u32 ra, u32 rs, bool rc) override;
        void STVRXL(u32 sd, u32 ra, u32 rb) override;
        void EXTSB(u32 ra, u32 rs, bool rc) override;
        void STFIWX(u32 frs, u32 ra, u32 rb) override;
        void EXTSW(u32 ra, u32 rs, bool rc) override;
        void ICBI(u32 ra, u32 rb) override;
        void DCBZ(u32 ra, u32 rb) override;
        void LWZ(u32 rd, u32 ra, s32 d) override;
        void LWZU(u32 rd, u32 ra, s32 d) override;
        void LBZ(u32 rd, u32 ra, s32 d) override;
        void LBZU(u32 rd, u32 ra, s32 d) override;
        void STW(u32 rs, u32 ra, s32 d) override;
        void STWU(u32 rs, u32 ra, s32 d) override;
        void STB(u32 rs, u32 ra, s32 d) override;
        void STBU(u32 rs, u32 ra, s32 d) override;
        void LHZ(u32 rd, u32 ra, s32 d) override;
        void LHZU(u32 rd, u32 ra, s32 d) override;
        void LHA(u32 rs, u32 ra, s32 d) override;
        void LHAU(u32 rs, u32 ra, s32 d) override;
        void STH(u32 rs, u32 ra, s32 d) override;
        void STHU(u32 rs, u32 ra, s32 d) override;
        void LMW(u32 rd, u32 ra, s32 d) override;
        void STMW(u32 rs, u32 ra, s32 d) override;
        void LFS(u32 frd, u32 ra, s32 d) override;
        void LFSU(u32 frd, u32 ra, s32 d) override;
        void LFD(u32 frd, u32 ra, s32 d) override;
        void LFDU(u32 frd, u32 ra, s32 d) override;
        void STFS(u32 frs, u32 ra, s32 d) override;
        void STFSU(u32 frs, u32 ra, s32 d) override;
        void STFD(u32 frs, u32 ra, s32 d) override;
        void STFDU(u32 frs, u32 ra, s32 d) override;
        void LD(u32 rd, u32 ra, s32 ds) override;
        void LDU(u32 rd, u32 ra, s32 ds) override;
        void LWA(u32 rd, u32 ra, s32 ds) override;
        void FDIVS(u32 frd, u32 fra, u32 frb, bool rc) override;
        void FSUBS(u32 frd, u32 fra, u32 frb, bool rc) override;
        void FADDS(u32 frd, u32 fra, u32 frb, bool rc) override;
        void FSQRTS(u32 frd, u32 frb, bool rc) override;
        void FRES(u32 frd, u32 frb, bool rc) override;
        void FMULS(u32 frd, u32 fra, u32 frc, bool rc) override;
        void FMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void FMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void FNMSUBS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void FNMADDS(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void STD(u32 rs, u32 ra, s32 ds) override;
        void STDU(u32 rs, u32 ra, s32 ds) override;
        void MTFSB1(u32 bt, bool rc) override;
        void MCRFS(u32 bf, u32 bfa) override;
        void MTFSB0(u32 bt, bool rc) override;
        void MTFSFI(u32 crfd, u32 i, bool rc) override;
        void MFFS(u32 frd, bool rc) override;
        void MTFSF(u32 flm, u32 frb, bool rc) override;

        void FCMPU(u32 bf, u32 fra, u32 frb) override;
        void FRSP(u32 frd, u32 frb, bool rc) override;
        void FCTIW(u32 frd, u32 frb, bool rc) override;
        void FCTIWZ(u32 frd, u32 frb, bool rc) override;
        void FDIV(u32 frd, u32 fra, u32 frb, bool rc) override;
        void FSUB(u32 frd, u32 fra, u32 frb, bool rc) override;
        void FADD(u32 frd, u32 fra, u32 frb, bool rc) override;
        void FSQRT(u32 frd, u32 frb, bool rc) override;
        void FSEL(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void FMUL(u32 frd, u32 fra, u32 frc, bool rc) override;
        void FRSQRTE(u32 frd, u32 frb, bool rc) override;
        void FMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void FMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void FNMSUB(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void FNMADD(u32 frd, u32 fra, u32 frc, u32 frb, bool rc) override;
        void FCMPO(u32 crfd, u32 fra, u32 frb) override;
        void FNEG(u32 frd, u32 frb, bool rc) override;
        void FMR(u32 frd, u32 frb, bool rc) override;
        void FNABS(u32 frd, u32 frb, bool rc) override;
        void FABS(u32 frd, u32 frb, bool rc) override;
        void FCTID(u32 frd, u32 frb, bool rc) override;
        void FCTIDZ(u32 frd, u32 frb, bool rc) override;
        void FCFID(u32 frd, u32 frb, bool rc) override;

        void UNK(const u32 code, const u32 opcode, const u32 gcode) override;

    private:
        /// State of a compilation task
        struct CompileTaskState {
            enum Args {
                State,
                Context,
                MaxArgs,
            };

            /// The LLVM function for the compilation task
            llvm::Function * function;

            /// Args of the LLVM function
            llvm::Value * args[MaxArgs];

            /// The CFG being compiled
            const ControlFlowGraph * cfg;

            /// Address of the current instruction being compiled
            u32 current_instruction_address;

            /// A flag used to detect branch instructions.
            /// This is set to false at the start of compilation of an instruction.
            /// If a branch instruction is encountered, this is set to true by the decode function.
            bool hit_branch_instruction;

            /// Indicates whether a block should be inlined even if an already compiled version of the block exists
            bool inline_all;

            /// Create code such that exit points can be linked to other blocks
            bool generate_linkable_exits;
        };

        /// Recompilation engine
        RecompilationEngine & m_recompilation_engine;

        /// The function that should be called to check the status of the thread
        bool (*m_poll_status_function)(PPUThread * ppu_state);

        /// The function that will be called to execute unknown functions
        llvm::Function * m_execute_unknown_function;

        /// The executable that will be called to execute unknown blocks
        llvm::Function *  m_execute_unknown_block;

        /// LLVM context
        llvm::LLVMContext * m_llvm_context;

        /// LLVM IR builder
        llvm::IRBuilder<> * m_ir_builder;

        /// Module to which all generated code is output to
        llvm::Module * m_module;

        /// JIT execution engine
        llvm::ExecutionEngine * m_execution_engine;

        /// Function pass manager
        llvm::FunctionPassManager * m_fpm;

        /// LLVM type of the functions genreated by the compiler
        llvm::FunctionType * m_compiled_function_type;

        /// State of the current compilation task
        CompileTaskState m_state;

        /// Compiler stats
        Stats m_stats;

        /// Get the name of the basic block for the specified address
        std::string GetBasicBlockNameFromAddress(u32 address, const std::string & suffix = "") const;

        /// Get the address of a basic block from its name
        u32 GetAddressFromBasicBlockName(const std::string & name) const;

        /// Get the basic block in for the specified address.
        llvm::BasicBlock * GetBasicBlockFromAddress(u32 address, const std::string & suffix = "", bool create_if_not_exist = true);

        /// Get a bit
        llvm::Value * GetBit(llvm::Value * val, u32 n);

        /// Clear a bit
        llvm::Value * ClrBit(llvm::Value * val, u32 n);

        /// Set a bit
        llvm::Value * SetBit(llvm::Value * val, u32 n, llvm::Value * bit, bool doClear = true);

        /// Get a nibble
        llvm::Value * GetNibble(llvm::Value * val, u32 n);

        /// Clear a nibble
        llvm::Value * ClrNibble(llvm::Value * val, u32 n);

        /// Set a nibble
        llvm::Value * SetNibble(llvm::Value * val, u32 n, llvm::Value * nibble, bool doClear = true);

        /// Set a nibble
        llvm::Value * SetNibble(llvm::Value * val, u32 n, llvm::Value * b0, llvm::Value * b1, llvm::Value * b2, llvm::Value * b3, bool doClear = true);

        /// Load PC
        llvm::Value * GetPc();

        /// Set PC
        void SetPc(llvm::Value * val_ix);

        /// Load GPR
        llvm::Value * GetGpr(u32 r, u32 num_bits = 64);

        /// Set GPR
        void SetGpr(u32 r, llvm::Value * val_x64);

        /// Load CR
        llvm::Value * GetCr();

        /// Load CR and get field CRn
        llvm::Value * GetCrField(u32 n);

        /// Set CR
        void SetCr(llvm::Value * val_x32);

        /// Set CR field
        void SetCrField(u32 n, llvm::Value * field);

        /// Set CR field
        void SetCrField(u32 n, llvm::Value * b0, llvm::Value * b1, llvm::Value * b2, llvm::Value * b3);

        /// Set CR field based on signed comparison
        void SetCrFieldSignedCmp(u32 n, llvm::Value * a, llvm::Value * b);

        /// Set CR field based on unsigned comparison
        void SetCrFieldUnsignedCmp(u32 n, llvm::Value * a, llvm::Value * b);

        /// Set CR6 based on the result of the vector compare instruction
        void SetCr6AfterVectorCompare(u32 vr);

        /// Get LR
        llvm::Value * GetLr();

        /// Set LR
        void SetLr(llvm::Value * val_x64);

        /// Get CTR
        llvm::Value * GetCtr();

        /// Set CTR
        void SetCtr(llvm::Value * val_x64);

        /// Load XER and convert it to an i64
        llvm::Value * GetXer();

        /// Load XER and return the CA bit
        llvm::Value * GetXerCa();

        /// Load XER and return the SO bit
        llvm::Value * GetXerSo();

        /// Set XER
        void SetXer(llvm::Value * val_x64);

        /// Set the CA bit of XER
        void SetXerCa(llvm::Value * ca);

        /// Set the SO bit of XER
        void SetXerSo(llvm::Value * so);

        /// Get VRSAVE
        llvm::Value * GetVrsave();

        /// Set VRSAVE
        void SetVrsave(llvm::Value * val_x64);

        /// Load FPSCR
        llvm::Value * GetFpscr();

        /// Set FPSCR
        void SetFpscr(llvm::Value * val_x32);

        /// Get FPR
        llvm::Value * GetFpr(u32 r, u32 bits = 64, bool as_int = false);

        /// Set FPR
        void SetFpr(u32 r, llvm::Value * val);

        /// Load VSCR
        llvm::Value * GetVscr();

        /// Set VSCR
        void SetVscr(llvm::Value * val_x32);

        /// Load VR
        llvm::Value * GetVr(u32 vr);

        /// Load VR and convert it to an integer vector
        llvm::Value * GetVrAsIntVec(u32 vr, u32 vec_elt_num_bits);

        /// Load VR and convert it to a float vector with 4 elements
        llvm::Value * GetVrAsFloatVec(u32 vr);

        /// Load VR and convert it to a double vector with 2 elements
        llvm::Value * GetVrAsDoubleVec(u32 vr);

        /// Set VR to the specified value
        void SetVr(u32 vr, llvm::Value * val_x128);

        /// Check condition for branch instructions
        llvm::Value * CheckBranchCondition(u32 bo, u32 bi);

        /// Create IR for a branch instruction
        void CreateBranch(llvm::Value * cmp_i1, llvm::Value * target_i32, bool lk, bool target_is_lr = false);

        /// Read from memory
        llvm::Value * ReadMemory(llvm::Value * addr_i64, u32 bits, u32 alignment = 0, bool bswap = true, bool could_be_mmio = true);

        /// Write to memory
        void WriteMemory(llvm::Value * addr_i64, llvm::Value * val_ix, u32 alignment = 0, bool bswap = true, bool could_be_mmio = true);

        /// Convert a C++ type to an LLVM type
        template<class T>
        llvm::Type * CppToLlvmType();

        /// Call a function
        template<class ReturnType, class Func, class... Args>
        llvm::Value * Call(const char * name, Func function, Args... args);

        /// Indirect call
        llvm::Value * IndirectCall(u32 address, llvm::Value * context_i64, bool is_function);

        /// Test an instruction against the interpreter
        template <class... Args>
        void VerifyInstructionAgainstInterpreter(const char * name, void (Compiler::*recomp_fn)(Args...), void (PPUInterpreter::*interp_fn)(Args...), PPUState & input_state, Args... args);

        /// Excute a test
        void RunTest(const char * name, std::function<void()> test_case, std::function<void()> input, std::function<bool(std::string & msg)> check_result);

        /// Handle compilation errors
        void CompilationError(const std::string & error);

        /// A mask used in rotate instructions
        static u64 s_rotate_mask[64][64];

        /// A flag indicating whether s_rotate_mask has been initialised or not
        static bool s_rotate_mask_inited;

        /// Initialse s_rotate_mask
        static void InitRotateMask();
    };

    class RecompilationEngine final : protected thread_t {
    public:
        virtual ~RecompilationEngine() override;

        /// Allocate an ordinal
        u32 AllocateOrdinal(u32 address, bool is_function);

        /// Get the ordinal for the specified address
        u32 GetOrdinal(u32 address) const;

        /// Get the executable specified by the ordinal
        const Executable GetExecutable(u32 ordinal) const;

        /// Get the address of the executable lookup
        u64 GetAddressOfExecutableLookup() const;

        /// Notify the recompilation engine about a newly detected trace. It takes ownership of the trace.
        void NotifyTrace(ExecutionTrace * execution_trace);

        /// Log
        llvm::raw_fd_ostream & Log();

        void Task();

        /// Get a pointer to the instance of this class
        static std::shared_ptr<RecompilationEngine> GetInstance();

    private:
        /// An entry in the block table
        struct BlockEntry {
            /// Number of times this block was hit
            u32 num_hits;

            /// The current revision number of this function
            u32 revision;

            /// Size of the CFG when it was last compiled
            size_t last_compiled_cfg_size;

            /// The CFG for this block
            ControlFlowGraph cfg;

            /// Indicates whether the block has been compiled or not
            bool is_compiled;

            BlockEntry(u32 start_address, u32 function_address)
                : num_hits(0)
                , revision(0)
                , last_compiled_cfg_size(0)
                , is_compiled(false)
                , cfg(start_address, function_address) {
            }

            std::string ToString() const {
                return fmt::Format("0x%08X (0x%08X): NumHits=%u, Revision=%u, LastCompiledCfgSize=%u, IsCompiled=%c",
                                   cfg.start_address, cfg.function_address, num_hits, revision, last_compiled_cfg_size, is_compiled ? 'Y' : 'N');
            }

            bool operator == (const BlockEntry & other) const {
                return cfg.start_address == other.cfg.start_address;
            }

            bool IsFunction() const {
                return cfg.function_address == cfg.start_address;
            }

            struct hash {
                size_t operator()(const BlockEntry * e) const {
                    return e->cfg.start_address;
                }
            };

            struct equal_to {
                bool operator()(const BlockEntry * lhs, const BlockEntry * rhs) const {
                    return *lhs == *rhs;
                }
            };
        };

        /// Log
        llvm::raw_fd_ostream * m_log;

        /// Lock for accessing m_pending_execution_traces. TODO: Eliminate this and use a lock-free queue.
        std::mutex m_pending_execution_traces_lock;

        /// Queue of execution traces pending processing
        std::list<ExecutionTrace *> m_pending_execution_traces;

        /// Block table
        std::unordered_set<BlockEntry *, BlockEntry::hash, BlockEntry::equal_to> m_block_table;

        /// Execution traces that have been already encountered. Data is the list of all blocks that this trace includes.
        std::unordered_map<ExecutionTrace::Id, std::vector<BlockEntry *>> m_processed_execution_traces;

        /// Lock for accessing m_address_to_ordinal.
        // TODO: Make this a RW lock
        mutable std::mutex m_address_to_ordinal_lock;

        /// Mapping from address to ordinal
        std::unordered_map<u32, u32> m_address_to_ordinal;

        /// Next ordinal to allocate
        u32 m_next_ordinal;

        /// PPU Compiler
        Compiler m_compiler;

        /// Executable lookup table
        Executable m_executable_lookup[10000]; // TODO: Adjust size

        RecompilationEngine();

        RecompilationEngine(const RecompilationEngine & other) = delete;
        RecompilationEngine(RecompilationEngine && other) = delete;

        RecompilationEngine & operator = (const RecompilationEngine & other) = delete;
        RecompilationEngine & operator = (RecompilationEngine && other) = delete;

        /// Process an execution trace.
        void ProcessExecutionTrace(const ExecutionTrace & execution_trace);

        /// Update a CFG
        void UpdateControlFlowGraph(ControlFlowGraph & cfg, const ExecutionTraceEntry & this_entry, const ExecutionTraceEntry * next_entry);

        /// Compile a block
        void CompileBlock(BlockEntry & block_entry);

        /// Mutex used to prevent multiple creation
        static std::mutex s_mutex;

        /// The instance
        static std::shared_ptr<RecompilationEngine> s_the_instance;
    };

    /// Finds interesting execution sequences
    class Tracer {
    public:
        /// Trace type
        enum class TraceType : u32 {
            CallFunction,
            EnterFunction,
            ExitFromCompiledFunction,
            Return,
            Instruction,
            ExitFromCompiledBlock,
        };

        Tracer();

        Tracer(const Tracer & other) = delete;
        Tracer(Tracer && other) = delete;

        virtual ~Tracer();

        Tracer & operator = (const Tracer & other) = delete;
        Tracer & operator = (Tracer && other) = delete;

        /// Notify the tracer
        void Trace(TraceType trace_type, u32 arg1, u32 arg2);

        /// Notify the tracer that the execution sequence is being terminated.
        void Terminate();

    private:
        /// Call stack
        std::vector<ExecutionTrace *> m_stack;

        /// Recompilation engine
        std::shared_ptr<RecompilationEngine> m_recompilation_engine;
    };

    /// PPU execution engine
    class ExecutionEngine : public CPUDecoder {
        friend class RecompilationEngine;
    public:
        ExecutionEngine(PPUThread & ppu);
        ExecutionEngine() = delete;

        ExecutionEngine(const ExecutionEngine & other) = delete;
        ExecutionEngine(ExecutionEngine && other) = delete;

        virtual ~ExecutionEngine();

        ExecutionEngine & operator = (const ExecutionEngine & other) = delete;
        ExecutionEngine & operator = (ExecutionEngine && other) = delete;

        u32 DecodeMemory(const u32 address) override;

    private:
        /// PPU processor context
        PPUThread & m_ppu;

        /// PPU Interpreter
        PPUInterpreter * m_interpreter;

        /// PPU instruction Decoder
        PPUDecoder m_decoder;

        /// Execution tracer
        Tracer m_tracer;

        /// The time at which the m_address_to_ordinal cache was last cleared
        mutable std::chrono::high_resolution_clock::time_point m_last_cache_clear_time;

        /// Address to ordinal cahce. Key is address. Data is the pair (ordinal, times hit).
        mutable std::unordered_map<u32, std::pair<u32, u32>> m_address_to_ordinal;

        /// Recompilation engine
        std::shared_ptr<RecompilationEngine> m_recompilation_engine;

        /// Remove unused entries from the m_address_to_ordinal cache
        void RemoveUnusedEntriesFromCache() const;

        /// Get the executable for the specified address
        Executable GetExecutable(u32 address, Executable default_executable) const;

        /// Execute a function
        static u32 ExecuteFunction(PPUThread * ppu_state, u64 context);

        /// Execute till the current function returns
        static u32 ExecuteTillReturn(PPUThread * ppu_state, u64 context);

        /// Check thread status. Returns true if the thread must exit.
        static bool PollStatus(PPUThread * ppu_state);
    };

    /// Get the branch type from a branch instruction
    BranchType GetBranchTypeFromInstruction(u32 instruction);
}

#endif // LLVM_AVAILABLE
#endif // PPU_LLVM_RECOMPILER_H
