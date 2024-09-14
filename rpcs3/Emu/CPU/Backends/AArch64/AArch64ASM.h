#pragma once

#include "AArch64Common.h"

namespace aarch64
{
    // Micro-assembler
    class UASM
    {
    public:
        enum ArgType
        {
            Register = 0,
            SRegister,
            Immediate,
            LLVMInt,
            LLVMPtr,
            LLVMReg
        };

        struct Arg
        {
            ArgType type;
            union
            {
                llvm::Value* value;
                gpr reg;
                spr sreg;
                s64 imm;
            };

            std::string to_string(int* id = nullptr) const;
        };

    protected:
        struct compiled_instruction_t
        {
            std::string asm_;
            std::vector<std::string> constraints;
            std::vector<llvm::Value*> args;
        };

        std::vector<compiled_instruction_t> m_instructions;

        void emit0(const char* inst);
        void emit1(const char* inst, const Arg& arg0, const std::vector<gpr>& clobbered);
        void emit2(const char* inst, const Arg& arg0, const Arg& arg1, const std::vector<gpr>& clobbered);
        void emit3(const char* inst, const Arg& arg0, const Arg& arg1, const Arg& arg2, const std::vector<gpr>& clobbered);
        void emit4(const char* inst, const Arg& arg0, const Arg& arg1, const Arg& arg2, const Arg& arg3, const std::vector<gpr>& clobbered);

        void embed_args(compiled_instruction_t& instruction, const std::vector<Arg>& args, const std::vector<gpr>& clobbered);

    public:
        UASM() = default;

        // Convenience wrappers
        static Arg Int(llvm::Value* v);
        static Arg Imm(s64 v);
        static Arg Reg(gpr reg);
        static Arg Reg(spr reg);
        static Arg Ptr(llvm::Value* v);
        static Arg Var(llvm::Value* v);

        void mov(gpr dst, gpr src);
        void mov(gpr dst, const Arg& src);
        void movnt(gpr dst, const Arg& src);

        void adr(gpr dst, const Arg& src);

        void str(gpr src, gpr base, const Arg& offset);
        void str(gpr src, spr base, const Arg& offset);
        void str(const Arg& src, gpr base, const Arg& offset);
        void str(const Arg& src, spr base, const Arg& offset);
        void ldr(gpr dst, gpr base, const Arg& offset);
        void ldr(gpr dst, spr base, const Arg& offset);

        void stp(gpr src0, gpr src1, gpr base, const Arg& offset);
        void stp(gpr src0, gpr src1, spr base, const Arg& offset);
        void ldp(gpr dst0, gpr dst1, gpr base, const Arg& offset);
        void ldp(gpr dst0, gpr dst1, spr base, const Arg& offset);

        void add(spr dst, spr src0, const Arg& src1);
        void add(gpr dst, gpr src0, const Arg& src1);
        void sub(spr dst, spr src0, const Arg& src1);
        void sub(gpr dst, gpr src0, const Arg& src1);

        void b(const Arg& target);
        void br(gpr target);
        void br(const Arg& target);
        void ret();

        void nop(const std::vector<Arg>& refs = {});
        void brk(int mark = 0);

        void append(const UASM& other);
        void prepend(const UASM& other);

        void insert(llvm::IRBuilder<>* irb, llvm::LLVMContext& ctx) const;
    };

    using ASMBlock = UASM;
}
