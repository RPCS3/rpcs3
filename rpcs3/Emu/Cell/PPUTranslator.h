#pragma once

#ifdef LLVM_AVAILABLE

#include "Emu/CPU/CPUTranslator.h"
#include "PPUOpcodes.h"
#include "PPUAnalyser.h"

class PPUTranslator final : public cpu_translator
{
	// PPU Module
	const ppu_module& m_info;

	// Relevant relocations
	std::map<u64, const ppu_reloc*> m_relocs;

	// Attributes for function calls which are "pure" and may be optimized away if their results are unused
	const llvm::AttributeList m_pure_attr;

	// LLVM function
	llvm::Function* m_function;

	llvm::MDNode* m_md_unlikely;
	llvm::MDNode* m_md_likely;

	// Current position-independent address
	u64 m_addr = 0;

	// Relocation info
	const ppu_segment* m_reloc = nullptr;

	// Set by instruction code after processing the relocation
	const ppu_reloc* m_rel = nullptr;

	/* Variables */

	// Segments
	std::vector<llvm::GlobalVariable*> m_segs;

	// Memory base
	llvm::GlobalVariable* m_base;
	llvm::Value* m_base_loaded;

	// Thread context
	llvm::Value* m_thread;

	// Callable functions
	llvm::GlobalVariable* m_call;

	// Thread context struct
	llvm::StructType* m_thread_type;

	llvm::Value* m_mtocr_table{};

	llvm::Value* m_globals[175];
	llvm::Value** const m_g_cr = m_globals + 99;
	llvm::Value* m_locals[175];
	llvm::Value** const m_gpr = m_locals + 3;
	llvm::Value** const m_fpr = m_locals + 35;
	llvm::Value** const m_vr = m_locals + 67;
	llvm::Value** const m_cr = m_locals + 99;
	llvm::Value** const m_fc = m_locals + 131; // FPSCR bits (used partially)

	llvm::Value* nan_vec4;

#define DEF_VALUE(loc, glb, pos)\
	llvm::Value*& loc = m_locals[pos];\
	llvm::Value*& glb = m_globals[pos];

	DEF_VALUE(m_lr, m_g_lr, 163) // LR, Link Register
	DEF_VALUE(m_ctr, m_g_ctr, 164) // CTR, Counter Register
	DEF_VALUE(m_vrsave, m_g_vrsave, 165)
	DEF_VALUE(m_cia, m_g_cia, 166)
	DEF_VALUE(m_so, m_g_so, 167) // XER.SO bit, summary overflow
	DEF_VALUE(m_ov, m_g_ov, 168) // XER.OV bit, overflow flag
	DEF_VALUE(m_ca, m_g_ca, 169) // XER.CA bit, carry flag
	DEF_VALUE(m_cnt, m_g_cnt, 170) // XER.CNT
	DEF_VALUE(m_sat, m_g_sat, 171) // VSCR.SAT bit, sticky saturation flag
	DEF_VALUE(m_nj, m_g_nj, 172) // VSCR.NJ bit, non-Java mode
	DEF_VALUE(m_jm_mask, m_g_jm_mask, 174) // Java-Mode helper mask

#undef DEF_VALUE
public:

	template <typename T>
	value_t<T> get_vr(u32 vr)
	{
		value_t<T> result;
		result.value = m_ir->CreateBitCast(GetVr(vr, VrType::vi32), value_t<T>::get_type(m_context));
		return result;
	}

	template <typename T, typename... Args>
	std::tuple<std::conditional_t<false, Args, value_t<T>>...> get_vrs(const Args&... args)
	{
		return {get_vr<T>(args)...};
	}

	template <typename T>
	void set_vr(u32 vr, T&& expr)
	{
		SetVr(vr, expr.eval(m_ir));
	}

	llvm::Value* VecHandleNan(llvm::Value* val);
	llvm::Value* VecHandleDenormal(llvm::Value* val);
	llvm::Value* VecHandleResult(llvm::Value* val);

	template <typename T>
	auto vec_handle_result(T&& expr)
	{
		value_t<typename T::type> result;
		result.value = VecHandleResult(expr.eval(m_ir));
		return result;
	}

	// Get current instruction address
	llvm::Value* GetAddr(u64 _add = 0);

	// Change integer size for integer or integer vector type (by 2^degree)
	llvm::Type* ScaleType(llvm::Type*, s32 pow2 = 0);

	// Extend arg to double width with its copy
	llvm::Value* DuplicateExt(llvm::Value* arg);

	// Rotate arg left by n (n must be < bitwidth)
	llvm::Value* RotateLeft(llvm::Value* arg, u64 n);

	// Rotate arg left by n (n will be masked)
	llvm::Value* RotateLeft(llvm::Value* arg, llvm::Value* n);

	// Emit function call
	void CallFunction(u64 target, llvm::Value* indirect = nullptr);

	// Initialize global for writing
	llvm::Value* RegInit(llvm::Value*& local);

	// Load last register value
	llvm::Value* RegLoad(llvm::Value*& local);

	// Store register value locally
	void RegStore(llvm::Value* value, llvm::Value*& local);

	// Write global registers
	void FlushRegisters();

	// Load gpr
	llvm::Value* GetGpr(u32 r, u32 num_bits = 64);

	// Set gpr
	void SetGpr(u32 r, llvm::Value* value);

	// Get fpr
	llvm::Value* GetFpr(u32 r, u32 bits = 64, bool as_int = false);

	// Set fpr
	void SetFpr(u32 r, llvm::Value* val);

	// Vector register type
	enum class VrType
	{
		vi8, // i8 vector
		vi16, // i16 vector
		vi32, // i32 vector
		vf, // f32 vector
		i128, // Solid 128-bit integer
	};

	// Load vr
	llvm::Value* GetVr(u32 vr, VrType);

	// Load VRs
	template<typename... Vrs>
	std::array<llvm::Value*, sizeof...(Vrs)> GetVrs(VrType type, Vrs... regs)
	{
		static_assert(sizeof...(Vrs), "Empty VR list");
		return{ GetVr(regs, type)... };
	}

	// Set vr to the specified value
	void SetVr(u32 vr, llvm::Value*);

	// Bitcast to scalar integer value
	llvm::Value* Solid(llvm::Value*);

	// Compare value with zero constant of appropriate size
	llvm::Value* IsZero(llvm::Value*); llvm::Value* IsNotZero(llvm::Value*);

	// Compare value with all-ones constant of appropriate size
	llvm::Value* IsOnes(llvm::Value*); llvm::Value* IsNotOnes(llvm::Value*);

	// Broadcast specified value
	llvm::Value* Broadcast(llvm::Value* value, u32 count);

	// Saturate scalar or vector given the comparison operand and the extreme value to compare with (second result is the comparison result)
	std::pair<llvm::Value*, llvm::Value*> Saturate(llvm::Value* value, llvm::CmpInst::Predicate inst, llvm::Value* extreme);

	// Saturate signed value (second result is the disjunction of comparison results)
	std::pair<llvm::Value*, llvm::Value*> SaturateSigned(llvm::Value* value, u64 min, u64 max);

	// Multiply FP value or vector by the pow(2, scale)
	llvm::Value* Scale(llvm::Value* value, s32 scale);

	// Create shuffle instruction with constant args
	llvm::Value* Shuffle(llvm::Value* left, llvm::Value* right, std::initializer_list<u32> indices);

	// Create sign extension (with double size if type is nullptr)
	llvm::Value* SExt(llvm::Value* value, llvm::Type* = nullptr);

	template<std::size_t N>
	std::array<llvm::Value*, N> SExt(std::array<llvm::Value*, N> values, llvm::Type* type = nullptr)
	{
		for (std::size_t i = 0; i < N; i++) values[i] = SExt(values[i], type);
		return values;
	}

	// Create zero extension (with double size if type is nullptr)
	llvm::Value* ZExt(llvm::Value*, llvm::Type* = nullptr);

	template<std::size_t N>
	std::array<llvm::Value*, N> ZExt(std::array<llvm::Value*, N> values, llvm::Type* type = nullptr)
	{
		for (std::size_t i = 0; i < N; i++) values[i] = ZExt(values[i], type);
		return values;
	}

	// Add multiple elements
	llvm::Value* Add(std::initializer_list<llvm::Value*>);

	// Create tuncation (with half size if type is nullptr)
	llvm::Value* Trunc(llvm::Value*, llvm::Type* = nullptr);

	// Get specified CR bit
	llvm::Value* GetCrb(u32 crb);

	// Set specified CR bit
	void SetCrb(u32 crb, llvm::Value* value);

	// Set CR field, if `so` value (5th arg) is nullptr, loaded from XER.SO
	void SetCrField(u32 group, llvm::Value* lt, llvm::Value* gt, llvm::Value* eq, llvm::Value* so = nullptr);

	// Set CR field based on signed comparison
	void SetCrFieldSignedCmp(u32 n, llvm::Value* a, llvm::Value* b);

	// Set CR field based on unsigned comparison
	void SetCrFieldUnsignedCmp(u32 n, llvm::Value* a, llvm::Value* b);

	// Set CR field from FPSCR CC fieds
	void SetCrFieldFPCC(u32 n);

	// Set FPSCR CC fields provided, optionally updating CR1
	void SetFPCC(llvm::Value* lt, llvm::Value* gt, llvm::Value* eq, llvm::Value* un, bool set_cr = false);

	// Update FPRF fields for the value, optionally updating CR1
	void SetFPRF(llvm::Value* value, bool set_cr);

	// Update FR bit
	void SetFPSCR_FR(llvm::Value* value);

	// Update FI bit (and set XX exception)
	void SetFPSCR_FI(llvm::Value* value);

	// Update sticky FPSCR exception bit, update FPSCR.FX
	void SetFPSCRException(llvm::Value* ptr, llvm::Value* value);

	// Get FPSCR bit (exception bits are cleared)
	llvm::Value* GetFPSCRBit(u32 n);

	// Set FPSCR bit
	void SetFPSCRBit(u32 n, llvm::Value*, bool update_fx);

	// Get XER.CA bit
	llvm::Value* GetCarry();

	// Set XER.CA bit
	void SetCarry(llvm::Value*);

	// Set XER.OV bit, and update XER.SO bit (|=)
	void SetOverflow(llvm::Value*);

	// Update sticky VSCR.SAT bit (|=)
	void SetSat(llvm::Value*);

	// Check condition for trap instructions
	llvm::Value* CheckTrapCondition(u32 to, llvm::Value* left, llvm::Value* right);

	// Emit trap for current address
	void Trap();

	// Get condition for branch instructions
	llvm::Value* CheckBranchCondition(u32 bo, u32 bi);

	// Get hint for branch instructions
	llvm::MDNode* CheckBranchProbability(u32 bo);

	// Branch to next instruction if condition failed, never branch on nullptr
	void UseCondition(llvm::MDNode* hint, llvm::Value* = nullptr);

	// Get memory pointer
	llvm::Value* GetMemory(llvm::Value* addr, llvm::Type* type);

	// Read from memory
	llvm::Value* ReadMemory(llvm::Value* addr, llvm::Type* type, bool is_be = true, u32 align = 1);

	// Write to memory
	void WriteMemory(llvm::Value* addr, llvm::Value* value, bool is_be = true, u32 align = 1);

	// Get an undefined value with specified type
	template<typename T>
	llvm::Value* GetUndef()
	{
		return llvm::UndefValue::get(GetType<T>());
	}

	// Call a function with attribute list
	template<typename... Args>
	llvm::CallInst* Call(llvm::Type* ret, llvm::AttributeList attr, llvm::StringRef name, Args... args)
	{
		// Call the function
		return m_ir->CreateCall(m_module->getOrInsertFunction(name, attr, ret, args->getType()...), {args...});
	}

	// Call a function
	template<typename... Args>
	llvm::CallInst* Call(llvm::Type* ret, llvm::StringRef name, Args... args)
	{
		return Call(ret, llvm::AttributeList{}, name, args...);
	}

	// Handle compilation errors
	void CompilationError(const std::string& error);

	PPUTranslator(llvm::LLVMContext& context, llvm::Module* _module, const ppu_module& info, llvm::ExecutionEngine& engine);
	~PPUTranslator();

	// Get thread context struct type
	llvm::Type* GetContextType();

	// Parses PPU opcodes and translate them into LLVM IR
	llvm::Function* Translate(const ppu_function& info);

	void MFVSCR(ppu_opcode_t op);
	void MTVSCR(ppu_opcode_t op);
	void VADDCUW(ppu_opcode_t op);
	void VADDFP(ppu_opcode_t op);
	void VADDSBS(ppu_opcode_t op);
	void VADDSHS(ppu_opcode_t op);
	void VADDSWS(ppu_opcode_t op);
	void VADDUBM(ppu_opcode_t op);
	void VADDUBS(ppu_opcode_t op);
	void VADDUHM(ppu_opcode_t op);
	void VADDUHS(ppu_opcode_t op);
	void VADDUWM(ppu_opcode_t op);
	void VADDUWS(ppu_opcode_t op);
	void VAND(ppu_opcode_t op);
	void VANDC(ppu_opcode_t op);
	void VAVGSB(ppu_opcode_t op);
	void VAVGSH(ppu_opcode_t op);
	void VAVGSW(ppu_opcode_t op);
	void VAVGUB(ppu_opcode_t op);
	void VAVGUH(ppu_opcode_t op);
	void VAVGUW(ppu_opcode_t op);
	void VCFSX(ppu_opcode_t op);
	void VCFUX(ppu_opcode_t op);
	void VCMPBFP(ppu_opcode_t op);
	void VCMPEQFP(ppu_opcode_t op);
	void VCMPEQUB(ppu_opcode_t op);
	void VCMPEQUH(ppu_opcode_t op);
	void VCMPEQUW(ppu_opcode_t op);
	void VCMPGEFP(ppu_opcode_t op);
	void VCMPGTFP(ppu_opcode_t op);
	void VCMPGTSB(ppu_opcode_t op);
	void VCMPGTSH(ppu_opcode_t op);
	void VCMPGTSW(ppu_opcode_t op);
	void VCMPGTUB(ppu_opcode_t op);
	void VCMPGTUH(ppu_opcode_t op);
	void VCMPGTUW(ppu_opcode_t op);
	void VCTSXS(ppu_opcode_t op);
	void VCTUXS(ppu_opcode_t op);
	void VEXPTEFP(ppu_opcode_t op);
	void VLOGEFP(ppu_opcode_t op);
	void VMADDFP(ppu_opcode_t op);
	void VMAXFP(ppu_opcode_t op);
	void VMAXSB(ppu_opcode_t op);
	void VMAXSH(ppu_opcode_t op);
	void VMAXSW(ppu_opcode_t op);
	void VMAXUB(ppu_opcode_t op);
	void VMAXUH(ppu_opcode_t op);
	void VMAXUW(ppu_opcode_t op);
	void VMHADDSHS(ppu_opcode_t op);
	void VMHRADDSHS(ppu_opcode_t op);
	void VMINFP(ppu_opcode_t op);
	void VMINSB(ppu_opcode_t op);
	void VMINSH(ppu_opcode_t op);
	void VMINSW(ppu_opcode_t op);
	void VMINUB(ppu_opcode_t op);
	void VMINUH(ppu_opcode_t op);
	void VMINUW(ppu_opcode_t op);
	void VMLADDUHM(ppu_opcode_t op);
	void VMRGHB(ppu_opcode_t op);
	void VMRGHH(ppu_opcode_t op);
	void VMRGHW(ppu_opcode_t op);
	void VMRGLB(ppu_opcode_t op);
	void VMRGLH(ppu_opcode_t op);
	void VMRGLW(ppu_opcode_t op);
	void VMSUMMBM(ppu_opcode_t op);
	void VMSUMSHM(ppu_opcode_t op);
	void VMSUMSHS(ppu_opcode_t op);
	void VMSUMUBM(ppu_opcode_t op);
	void VMSUMUHM(ppu_opcode_t op);
	void VMSUMUHS(ppu_opcode_t op);
	void VMULESB(ppu_opcode_t op);
	void VMULESH(ppu_opcode_t op);
	void VMULEUB(ppu_opcode_t op);
	void VMULEUH(ppu_opcode_t op);
	void VMULOSB(ppu_opcode_t op);
	void VMULOSH(ppu_opcode_t op);
	void VMULOUB(ppu_opcode_t op);
	void VMULOUH(ppu_opcode_t op);
	void VNMSUBFP(ppu_opcode_t op);
	void VNOR(ppu_opcode_t op);
	void VOR(ppu_opcode_t op);
	void VPERM(ppu_opcode_t op);
	void VPKPX(ppu_opcode_t op);
	void VPKSHSS(ppu_opcode_t op);
	void VPKSHUS(ppu_opcode_t op);
	void VPKSWSS(ppu_opcode_t op);
	void VPKSWUS(ppu_opcode_t op);
	void VPKUHUM(ppu_opcode_t op);
	void VPKUHUS(ppu_opcode_t op);
	void VPKUWUM(ppu_opcode_t op);
	void VPKUWUS(ppu_opcode_t op);
	void VREFP(ppu_opcode_t op);
	void VRFIM(ppu_opcode_t op);
	void VRFIN(ppu_opcode_t op);
	void VRFIP(ppu_opcode_t op);
	void VRFIZ(ppu_opcode_t op);
	void VRLB(ppu_opcode_t op);
	void VRLH(ppu_opcode_t op);
	void VRLW(ppu_opcode_t op);
	void VRSQRTEFP(ppu_opcode_t op);
	void VSEL(ppu_opcode_t op);
	void VSL(ppu_opcode_t op);
	void VSLB(ppu_opcode_t op);
	void VSLDOI(ppu_opcode_t op);
	void VSLH(ppu_opcode_t op);
	void VSLO(ppu_opcode_t op);
	void VSLW(ppu_opcode_t op);
	void VSPLTB(ppu_opcode_t op);
	void VSPLTH(ppu_opcode_t op);
	void VSPLTISB(ppu_opcode_t op);
	void VSPLTISH(ppu_opcode_t op);
	void VSPLTISW(ppu_opcode_t op);
	void VSPLTW(ppu_opcode_t op);
	void VSR(ppu_opcode_t op);
	void VSRAB(ppu_opcode_t op);
	void VSRAH(ppu_opcode_t op);
	void VSRAW(ppu_opcode_t op);
	void VSRB(ppu_opcode_t op);
	void VSRH(ppu_opcode_t op);
	void VSRO(ppu_opcode_t op);
	void VSRW(ppu_opcode_t op);
	void VSUBCUW(ppu_opcode_t op);
	void VSUBFP(ppu_opcode_t op);
	void VSUBSBS(ppu_opcode_t op);
	void VSUBSHS(ppu_opcode_t op);
	void VSUBSWS(ppu_opcode_t op);
	void VSUBUBM(ppu_opcode_t op);
	void VSUBUBS(ppu_opcode_t op);
	void VSUBUHM(ppu_opcode_t op);
	void VSUBUHS(ppu_opcode_t op);
	void VSUBUWM(ppu_opcode_t op);
	void VSUBUWS(ppu_opcode_t op);
	void VSUMSWS(ppu_opcode_t op);
	void VSUM2SWS(ppu_opcode_t op);
	void VSUM4SBS(ppu_opcode_t op);
	void VSUM4SHS(ppu_opcode_t op);
	void VSUM4UBS(ppu_opcode_t op);
	void VUPKHPX(ppu_opcode_t op);
	void VUPKHSB(ppu_opcode_t op);
	void VUPKHSH(ppu_opcode_t op);
	void VUPKLPX(ppu_opcode_t op);
	void VUPKLSB(ppu_opcode_t op);
	void VUPKLSH(ppu_opcode_t op);
	void VXOR(ppu_opcode_t op);

	void TDI(ppu_opcode_t op);
	void TWI(ppu_opcode_t op);
	void MULLI(ppu_opcode_t op);
	void SUBFIC(ppu_opcode_t op);
	void CMPLI(ppu_opcode_t op);
	void CMPI(ppu_opcode_t op);
	void ADDIC(ppu_opcode_t op);
	void ADDI(ppu_opcode_t op);
	void ADDIS(ppu_opcode_t op);
	void BC(ppu_opcode_t op);
	void SC(ppu_opcode_t op);
	void B(ppu_opcode_t op);
	void MCRF(ppu_opcode_t op);
	void BCLR(ppu_opcode_t op);
	void CRNOR(ppu_opcode_t op);
	void CRANDC(ppu_opcode_t op);
	void ISYNC(ppu_opcode_t op);
	void CRXOR(ppu_opcode_t op);
	void CRNAND(ppu_opcode_t op);
	void CRAND(ppu_opcode_t op);
	void CREQV(ppu_opcode_t op);
	void CRORC(ppu_opcode_t op);
	void CROR(ppu_opcode_t op);
	void BCCTR(ppu_opcode_t op);
	void RLWIMI(ppu_opcode_t op);
	void RLWINM(ppu_opcode_t op);
	void RLWNM(ppu_opcode_t op);
	void ORI(ppu_opcode_t op);
	void ORIS(ppu_opcode_t op);
	void XORI(ppu_opcode_t op);
	void XORIS(ppu_opcode_t op);
	void ANDI(ppu_opcode_t op);
	void ANDIS(ppu_opcode_t op);
	void RLDICL(ppu_opcode_t op);
	void RLDICR(ppu_opcode_t op);
	void RLDIC(ppu_opcode_t op);
	void RLDIMI(ppu_opcode_t op);
	void RLDCL(ppu_opcode_t op);
	void RLDCR(ppu_opcode_t op);
	void CMP(ppu_opcode_t op);
	void TW(ppu_opcode_t op);
	void LVSL(ppu_opcode_t op);
	void LVEBX(ppu_opcode_t op);
	void SUBFC(ppu_opcode_t op);
	void MULHDU(ppu_opcode_t op);
	void ADDC(ppu_opcode_t op);
	void MULHWU(ppu_opcode_t op);
	void MFOCRF(ppu_opcode_t op);
	void LWARX(ppu_opcode_t op);
	void LDX(ppu_opcode_t op);
	void LWZX(ppu_opcode_t op);
	void SLW(ppu_opcode_t op);
	void CNTLZW(ppu_opcode_t op);
	void SLD(ppu_opcode_t op);
	void AND(ppu_opcode_t op);
	void CMPL(ppu_opcode_t op);
	void LVSR(ppu_opcode_t op);
	void LVEHX(ppu_opcode_t op);
	void SUBF(ppu_opcode_t op);
	void LDUX(ppu_opcode_t op);
	void DCBST(ppu_opcode_t op);
	void LWZUX(ppu_opcode_t op);
	void CNTLZD(ppu_opcode_t op);
	void ANDC(ppu_opcode_t op);
	void TD(ppu_opcode_t op);
	void LVEWX(ppu_opcode_t op);
	void MULHD(ppu_opcode_t op);
	void MULHW(ppu_opcode_t op);
	void LDARX(ppu_opcode_t op);
	void DCBF(ppu_opcode_t op);
	void LBZX(ppu_opcode_t op);
	void LVX(ppu_opcode_t op);
	void NEG(ppu_opcode_t op);
	void LBZUX(ppu_opcode_t op);
	void NOR(ppu_opcode_t op);
	void STVEBX(ppu_opcode_t op);
	void SUBFE(ppu_opcode_t op);
	void ADDE(ppu_opcode_t op);
	void MTOCRF(ppu_opcode_t op);
	void STDX(ppu_opcode_t op);
	void STWCX(ppu_opcode_t op);
	void STWX(ppu_opcode_t op);
	void STVEHX(ppu_opcode_t op);
	void STDUX(ppu_opcode_t op);
	void STWUX(ppu_opcode_t op);
	void STVEWX(ppu_opcode_t op);
	void SUBFZE(ppu_opcode_t op);
	void ADDZE(ppu_opcode_t op);
	void STDCX(ppu_opcode_t op);
	void STBX(ppu_opcode_t op);
	void STVX(ppu_opcode_t op);
	void MULLD(ppu_opcode_t op);
	void SUBFME(ppu_opcode_t op);
	void ADDME(ppu_opcode_t op);
	void MULLW(ppu_opcode_t op);
	void DCBTST(ppu_opcode_t op);
	void STBUX(ppu_opcode_t op);
	void ADD(ppu_opcode_t op);
	void DCBT(ppu_opcode_t op);
	void LHZX(ppu_opcode_t op);
	void EQV(ppu_opcode_t op);
	void ECIWX(ppu_opcode_t op);
	void LHZUX(ppu_opcode_t op);
	void XOR(ppu_opcode_t op);
	void MFSPR(ppu_opcode_t op);
	void LWAX(ppu_opcode_t op);
	void DST(ppu_opcode_t op);
	void LHAX(ppu_opcode_t op);
	void LVXL(ppu_opcode_t op);
	void MFTB(ppu_opcode_t op);
	void LWAUX(ppu_opcode_t op);
	void DSTST(ppu_opcode_t op);
	void LHAUX(ppu_opcode_t op);
	void STHX(ppu_opcode_t op);
	void ORC(ppu_opcode_t op);
	void ECOWX(ppu_opcode_t op);
	void STHUX(ppu_opcode_t op);
	void OR(ppu_opcode_t op);
	void DIVDU(ppu_opcode_t op);
	void DIVWU(ppu_opcode_t op);
	void MTSPR(ppu_opcode_t op);
	void DCBI(ppu_opcode_t op);
	void NAND(ppu_opcode_t op);
	void STVXL(ppu_opcode_t op);
	void DIVD(ppu_opcode_t op);
	void DIVW(ppu_opcode_t op);
	void LVLX(ppu_opcode_t op);
	void LDBRX(ppu_opcode_t op);
	void LSWX(ppu_opcode_t op);
	void LWBRX(ppu_opcode_t op);
	void LFSX(ppu_opcode_t op);
	void SRW(ppu_opcode_t op);
	void SRD(ppu_opcode_t op);
	void LVRX(ppu_opcode_t op);
	void LSWI(ppu_opcode_t op);
	void LFSUX(ppu_opcode_t op);
	void SYNC(ppu_opcode_t op);
	void LFDX(ppu_opcode_t op);
	void LFDUX(ppu_opcode_t op);
	void STVLX(ppu_opcode_t op);
	void STDBRX(ppu_opcode_t op);
	void STSWX(ppu_opcode_t op);
	void STWBRX(ppu_opcode_t op);
	void STFSX(ppu_opcode_t op);
	void STVRX(ppu_opcode_t op);
	void STFSUX(ppu_opcode_t op);
	void STSWI(ppu_opcode_t op);
	void STFDX(ppu_opcode_t op);
	void STFDUX(ppu_opcode_t op);
	void LVLXL(ppu_opcode_t op);
	void LHBRX(ppu_opcode_t op);
	void SRAW(ppu_opcode_t op);
	void SRAD(ppu_opcode_t op);
	void LVRXL(ppu_opcode_t op);
	void DSS(ppu_opcode_t op);
	void SRAWI(ppu_opcode_t op);
	void SRADI(ppu_opcode_t op);
	void EIEIO(ppu_opcode_t op);
	void STVLXL(ppu_opcode_t op);
	void STHBRX(ppu_opcode_t op);
	void EXTSH(ppu_opcode_t op);
	void STVRXL(ppu_opcode_t op);
	void EXTSB(ppu_opcode_t op);
	void STFIWX(ppu_opcode_t op);
	void EXTSW(ppu_opcode_t op);
	void ICBI(ppu_opcode_t op);
	void DCBZ(ppu_opcode_t op);
	void LWZ(ppu_opcode_t op);
	void LWZU(ppu_opcode_t op);
	void LBZ(ppu_opcode_t op);
	void LBZU(ppu_opcode_t op);
	void STW(ppu_opcode_t op);
	void STWU(ppu_opcode_t op);
	void STB(ppu_opcode_t op);
	void STBU(ppu_opcode_t op);
	void LHZ(ppu_opcode_t op);
	void LHZU(ppu_opcode_t op);
	void LHA(ppu_opcode_t op);
	void LHAU(ppu_opcode_t op);
	void STH(ppu_opcode_t op);
	void STHU(ppu_opcode_t op);
	void LMW(ppu_opcode_t op);
	void STMW(ppu_opcode_t op);
	void LFS(ppu_opcode_t op);
	void LFSU(ppu_opcode_t op);
	void LFD(ppu_opcode_t op);
	void LFDU(ppu_opcode_t op);
	void STFS(ppu_opcode_t op);
	void STFSU(ppu_opcode_t op);
	void STFD(ppu_opcode_t op);
	void STFDU(ppu_opcode_t op);
	void LD(ppu_opcode_t op);
	void LDU(ppu_opcode_t op);
	void LWA(ppu_opcode_t op);
	void STD(ppu_opcode_t op);
	void STDU(ppu_opcode_t op);

	void FDIVS(ppu_opcode_t op);
	void FSUBS(ppu_opcode_t op);
	void FADDS(ppu_opcode_t op);
	void FSQRTS(ppu_opcode_t op);
	void FRES(ppu_opcode_t op);
	void FMULS(ppu_opcode_t op);
	void FMADDS(ppu_opcode_t op);
	void FMSUBS(ppu_opcode_t op);
	void FNMSUBS(ppu_opcode_t op);
	void FNMADDS(ppu_opcode_t op);
	void MTFSB1(ppu_opcode_t op);
	void MCRFS(ppu_opcode_t op);
	void MTFSB0(ppu_opcode_t op);
	void MTFSFI(ppu_opcode_t op);
	void MFFS(ppu_opcode_t op);
	void MTFSF(ppu_opcode_t op);
	void FCMPU(ppu_opcode_t op);
	void FRSP(ppu_opcode_t op);
	void FCTIW(ppu_opcode_t op);
	void FCTIWZ(ppu_opcode_t op);
	void FDIV(ppu_opcode_t op);
	void FSUB(ppu_opcode_t op);
	void FADD(ppu_opcode_t op);
	void FSQRT(ppu_opcode_t op);
	void FSEL(ppu_opcode_t op);
	void FMUL(ppu_opcode_t op);
	void FRSQRTE(ppu_opcode_t op);
	void FMSUB(ppu_opcode_t op);
	void FMADD(ppu_opcode_t op);
	void FNMSUB(ppu_opcode_t op);
	void FNMADD(ppu_opcode_t op);
	void FCMPO(ppu_opcode_t op);
	void FNEG(ppu_opcode_t op);
	void FMR(ppu_opcode_t op);
	void FNABS(ppu_opcode_t op);
	void FABS(ppu_opcode_t op);
	void FCTID(ppu_opcode_t op);
	void FCTIDZ(ppu_opcode_t op);
	void FCFID(ppu_opcode_t op);

	void UNK(ppu_opcode_t op);
};

#endif
