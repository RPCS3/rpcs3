#include "stdafx.h"
#include "AArch64ASM.h"

#include "Utilities/StrFmt.h"

namespace aarch64
{
    using fmt_replacement_list_t = std::vector<std::pair<std::string, std::string>>;

    void UASM::embed_args(compiled_instruction_t& instruction, const std::vector<Arg>& args, const std::vector<gpr>& clobbered)
    {
        for (const auto& arg : args)
        {
            switch (arg.type)
            {
            case ArgType::Immediate:
            case ArgType::Register:
            case ArgType::SRegister:
                // Embedded directly
                break;
            case ArgType::LLVMInt:
                instruction.constraints.push_back("i");
                instruction.args.push_back(arg.value);
                break;
            case ArgType::LLVMReg:
                instruction.constraints.push_back("r");
                instruction.args.push_back(arg.value);
                break;
            case ArgType::LLVMPtr:
                instruction.constraints.push_back("m");
                instruction.args.push_back(arg.value);
                break;
            default:
                break;
            }
        }

        for (const auto& reg : clobbered)
        {
            const auto clobber = fmt::format("~{%s}", gpr_names[static_cast<int>(reg)]);
            instruction.constraints.push_back(clobber);
        }
    }

    void UASM::emit0(const char* inst)
    {
        compiled_instruction_t i{};
        i.asm_ = inst;
        m_instructions.push_back(i);
    }

    void UASM::emit1(const char* inst, const Arg& arg0, const std::vector<gpr>& clobbered)
    {
        int arg_id = 0;
        fmt_replacement_list_t repl = {
            { "{0}", arg0.to_string(&arg_id) }
        };

        compiled_instruction_t i{};
        i.asm_ = fmt::replace_all(inst, repl);
        embed_args(i, { arg0 }, clobbered);
        m_instructions.push_back(i);
    }

    void UASM::emit2(const char* inst, const Arg& arg0, const Arg& arg1, const std::vector<gpr>& clobbered)
    {
        int arg_id = 0;
        fmt_replacement_list_t repl = {
            { "{0}", arg0.to_string(&arg_id) },
            { "{1}", arg1.to_string(&arg_id) },
        };

        compiled_instruction_t i{};
        i.asm_ = fmt::replace_all(inst, repl);
        embed_args(i, { arg0, arg1 }, clobbered);
        m_instructions.push_back(i);
    }

    void UASM::emit3(const char* inst, const Arg& arg0, const Arg& arg1, const Arg& arg2, const std::vector<gpr>& clobbered)
    {
        int arg_id = 0;
        fmt_replacement_list_t repl = {
            { "{0}", arg0.to_string(&arg_id) },
            { "{1}", arg1.to_string(&arg_id) },
            { "{2}", arg2.to_string(&arg_id) },
        };

        compiled_instruction_t i{};
        i.asm_ = fmt::replace_all(inst, repl);
        embed_args(i, { arg0, arg1, arg2 }, clobbered);
        m_instructions.push_back(i);
    }

    void UASM::emit4(const char* inst, const Arg& arg0, const Arg& arg1, const Arg& arg2, const Arg& arg3, const std::vector<gpr>& clobbered)
    {
        int arg_id = 0;
        fmt_replacement_list_t repl = {
            { "{0}", arg0.to_string(&arg_id) },
            { "{1}", arg1.to_string(&arg_id) },
            { "{2}", arg2.to_string(&arg_id) },
            { "{3}", arg3.to_string(&arg_id) },
        };

        compiled_instruction_t i{};
        i.asm_ = fmt::replace_all(inst, repl);
        embed_args(i, { arg0, arg1, arg2, arg3 }, clobbered);
        m_instructions.push_back(i);
    }

    void UASM::insert(llvm::IRBuilder<>* irb, llvm::LLVMContext& ctx) const
    {
        for (const auto& inst : m_instructions)
        {
            auto constraints = fmt::merge(inst.constraints, ",");
            llvm_asm(irb, inst.asm_, inst.args, constraints, ctx);
        }
    }

    void UASM::append(const UASM& that)
    {
        m_instructions.reserve(m_instructions.size() + that.m_instructions.size());
        std::copy(that.m_instructions.begin(), that.m_instructions.end(), std::back_inserter(this->m_instructions));
    }

    void UASM::prepend(const UASM& that)
    {
        auto new_instructions = that.m_instructions;
        new_instructions.reserve(m_instructions.size() + that.m_instructions.size());
        std::copy(m_instructions.begin(), m_instructions.end(), std::back_inserter(new_instructions));
        m_instructions = std::move(new_instructions);
    }

    // Convenience arg wrappers
    UASM::Arg UASM::Int(llvm::Value* v)
    {
        return Arg {
            .type = ArgType::LLVMInt,
            .value = v
        };
    }

    UASM::Arg UASM::Imm(s64 v)
    {
        return Arg {
            .type = ArgType::Immediate,
            .imm = v
        };
    }

    UASM::Arg UASM::Reg(gpr reg)
    {
        return Arg {
            .type = ArgType::Register,
            .reg = reg
        };
    }

    UASM::Arg UASM::Reg(spr reg)
    {
        return Arg {
            .type = ArgType::SRegister,
            .sreg = reg
        };
    }

    UASM::Arg UASM::Ptr(llvm::Value* v)
    {
        return Arg {
            .type = ArgType::LLVMPtr,
            .value = v
        };
    }

    UASM::Arg UASM::Var(llvm::Value* v)
    {
        return Arg {
            .type = ArgType::LLVMReg,
            .value = v
        };
    }

    void UASM::mov(gpr dst, gpr src)
    {
        emit2("mov {0}, {1}", Reg(dst), Reg(src), { dst });
    }

    void UASM::mov(gpr dst, const Arg& src)
    {
        emit2("mov {0}, {1}", Reg(dst), src, { dst });
    }

    void UASM::movnt(gpr dst, const Arg& src)
    {
        emit2("mov {0}, {1}", Reg(dst), src, {});
    }

    void UASM::str(gpr src, gpr base, const Arg& offset)
    {
        emit3("str {0}, [{1}, {2}]", Reg(src), Reg(base), offset, {});
    }

    void UASM::str(const Arg& src, spr base, const Arg& offset)
    {
        emit3("str {0}, [{1}, {2}]", src, Reg(base), offset, {});
    }

    void UASM::ldr(gpr dst, gpr base, const Arg& offset)
    {
        emit3("ldr {0}, [{1}, {2}]", Reg(dst), Reg(base), offset, { dst });
    }

    void UASM::ldr(gpr dst, spr base, const Arg& offset)
    {
        emit3("ldr {0}, [{1}, {2}]", Reg(dst), Reg(base), offset, { dst });
    }

    void UASM::stp(gpr src0, gpr src1, gpr base, const Arg& offset)
    {
        emit4("stp {0}, {1}, [{2}, {3}]", Reg(src0), Reg(src1), Reg(base), offset, {});
    }

    void UASM::ldp(gpr dst0, gpr dst1, gpr base, const Arg& offset)
    {
        emit4("ldp {0}, {1}, [{2}, {3}]", Reg(dst0), Reg(dst1), Reg(base), offset, { dst0, dst1 });
    }

    void UASM::b(const Arg& target)
    {
        emit1("b {0}", target, {});
    }

    void UASM::br(gpr target)
    {
        emit1("br {0}", Reg(target), {});
    }

    void UASM::br(const Arg& target)
    {
        emit1("br {0}", target, {});
    }

    void UASM::ret()
    {
        emit0("ret");
    }

    void UASM::adr(gpr dst, const Arg& src)
    {
        emit2("adr {0}, {1}", Reg(dst), src, { dst });
    }

    void UASM::add(spr dst, spr src0, const Arg& src1)
    {
        emit3("add {0}, {1}, {2}", Reg(dst), Reg(src0), src1, {});
    }

    void UASM::sub(spr dst, spr src0, const Arg& src1)
    {
        emit3("sub {0}, {1}, {2}", Reg(dst), Reg(src0), src1, {});
    }

    void UASM::nop(const std::vector<Arg>& refs)
    {
        emit0("nop");
        embed_args(m_instructions.back(), refs, {});
    }

    void UASM::brk(int mark)
    {
        emit1("brk {0}", Imm(mark), {});
    }

    std::string UASM::Arg::to_string(int* id) const
    {
        // Safety checks around the optional arg incrementer
        int dummy = 0;
        if (!id)
        {
            id = &dummy;
        }

        switch (type)
        {
        case ArgType::Immediate:
            return std::string("#") + std::to_string(imm);
        case ArgType::Register:
            return gpr_names[static_cast<int>(reg)];
        case ArgType::SRegister:
            return spr_asm_names[static_cast<int>(sreg)];
        case ArgType::LLVMInt:
        case ArgType::LLVMReg:
        case ArgType::LLVMPtr:
        default:
            // Return placeholder identifier
            return std::string("$") + std::to_string(*id++);
        }
    }
}
