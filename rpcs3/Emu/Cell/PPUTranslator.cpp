#ifdef LLVM_AVAILABLE

#include "PPUTranslator.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"

#include "../Utilities/Log.h"

using namespace llvm;

const ppu_decoder<PPUTranslator> s_ppu_decoder;

/* Interpreter Call Macro */

#define VEC3OP(name) SetVr(op.vd, Call(GetType<u32[4]>(), "__vec3op",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	GetVr(op.va, VrType::vi32),\
	GetVr(op.vb, VrType::vi32),\
	GetVr(op.vc, VrType::vi32)))

#define VEC2OP(name) SetVr(op.vd, Call(GetType<u32[4]>(), "__vec3op",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	GetVr(op.va, VrType::vi32),\
	GetVr(op.vb, VrType::vi32),\
	GetUndef<u32[4]>()))

#define VECIOP(name) SetVr(op.vd, Call(GetType<u32[4]>(), "__veciop",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	m_ir->getInt32(op.opcode),\
	GetVr(op.vb, VrType::vi32)))

#define FPOP(name) SetFpr(op.frd, Call(GetType<f64>(), "__fpop",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	GetFpr(op.fra),\
	GetFpr(op.frb),\
	GetFpr(op.frc)))

#define AIMMOP(name) SetGpr(op.ra, Call(GetType<u64>(), "__aimmop",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	m_ir->getInt32(op.opcode),\
	GetGpr(op.rs)))

#define AIMMBOP(name) SetGpr(op.ra, Call(GetType<u64>(), "__aimmbop",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	m_ir->getInt32(op.opcode),\
	GetGpr(op.rs),\
	GetGpr(op.rb)))

#define AAIMMOP(name) SetGpr(op.ra, Call(GetType<u64>(), "__aaimmop",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	m_ir->getInt32(op.opcode),\
	GetGpr(op.rs),\
	GetGpr(op.ra)))

#define IMMAOP(name) SetGpr(op.rd, Call(GetType<u64>(), "__immaop",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	m_ir->getInt32(op.opcode),\
	GetGpr(op.ra)))

#define IMMABOP(name) SetGpr(op.rd, Call(GetType<u64>(), "__immabop",\
	m_ir->getInt64((u64)&ppu_interpreter_fast::name),\
	m_ir->getInt32(op.opcode),\
	GetGpr(op.ra),\
	GetGpr(op.rb)))

PPUTranslator::PPUTranslator(LLVMContext& context, Module* module, u64 base, u64 entry)
	: m_context(context)
	, m_module(module)
	, m_base_addr(base)
	, m_is_be(false)
	, m_pure_attr(AttributeSet::get(m_context, AttributeSet::FunctionIndex, {Attribute::NoUnwind, Attribute::ReadNone}))
{
	// Memory base
	m_base = new GlobalVariable(*module, ArrayType::get(GetType<char>(), 0x100000000), false, GlobalValue::ExternalLinkage, 0, "__memory");

	// Thread context struct (TODO: safer member access)
	std::vector<Type*> thread_struct{ArrayType::get(GetType<char>(), OFFSET_32(PPUThread, GPR))};

	thread_struct.insert(thread_struct.end(), 32, GetType<u64>()); // GPR[0..31]
	thread_struct.insert(thread_struct.end(), 32, GetType<f64>()); // FPR[0..31]
	thread_struct.insert(thread_struct.end(), 32, GetType<u32[4]>()); // VR[0..31]
	thread_struct.insert(thread_struct.end(), 32, GetType<bool>()); // CR[0..31]

	m_thread_type = StructType::create(m_context, thread_struct, "context_t");

	// Callable
	m_call = new GlobalVariable(*module, ArrayType::get(FunctionType::get(GetType<void>(), {m_thread_type->getPointerTo()}, false)->getPointerTo(), 0x40000000), true, GlobalValue::ExternalLinkage, 0, "__call");
}

PPUTranslator::~PPUTranslator()
{
}

Type* PPUTranslator::GetContextType()
{
	return m_thread_type;
}

void PPUTranslator::AddFunction(u64 addr, Function* func, FunctionType* type)
{
	if (!m_func_types.emplace(addr, type).second || !m_func_list.emplace(addr, func).second)
	{
		throw fmt::exception("AddFunction(0x%08llx: %s) failed: function already exists", addr, func->getName().data());
	}
}

void PPUTranslator::AddBlockInfo(u64 addr)
{
	m_block_info.emplace(addr);
}

Function* PPUTranslator::TranslateToIR(u64 start_addr, u64 end_addr, be_t<u32>* bin, void(*custom)(PPUTranslator*))
{
	m_function = m_func_list[start_addr];
	m_function_type = m_func_types[start_addr];
	m_start_addr = start_addr;
	m_end_addr = end_addr;
	m_blocks.clear();
	m_value_usage.clear();

	IRBuilder<> builder(BasicBlock::Create(m_context, "__entry", m_function));
	m_ir = &builder;

	/* Create context variables */
	//m_thread = Call(m_thread_type->getPointerTo(), AttributeSet::get(m_context, AttributeSet::FunctionIndex, {Attribute::NoUnwind, Attribute::ReadOnly}), "__context", m_ir->getInt64(start_addr));
	m_thread = &*m_function->getArgumentList().begin();
	
	// Non-volatile registers with special meaning (TODO)
	m_g_gpr[1] = m_ir->CreateConstGEP2_32(nullptr, m_thread, 0, 1 + 1, ".sp");
	m_g_gpr[2] = m_ir->CreateConstGEP2_32(nullptr, m_thread, 0, 1 + 2, ".rtoc");
	m_g_gpr[13] = m_ir->CreateConstGEP2_32(nullptr, m_thread, 0, 1 + 13, ".tls");

	// Registers used for args or results (TODO)
	for (u32 i = 3; i <= 10; i++) m_g_gpr[i] = m_ir->CreateConstGEP2_32(nullptr, m_thread, 0, 1 + i, fmt::format(".r%u", i));
	for (u32 i = 1; i <= 13; i++) m_g_fpr[i] = m_ir->CreateConstGEP2_32(nullptr, m_thread, 0, 33 + i, fmt::format(".f%u", i));
	for (u32 i = 2; i <= 13; i++) m_g_vr[i] = m_ir->CreateConstGEP2_32(nullptr, m_thread, 0, 65 + i, fmt::format(".v%u", i));

	/* Create local variables */
	for (u32 i = 0; i < 32; i++) m_gpr[i] = m_g_gpr[i] ? m_g_gpr[i] : m_ir->CreateAlloca(GetType<u64>(), nullptr, fmt::format(".r%d", i));
	for (u32 i = 0; i < 32; i++) m_fpr[i] = m_g_fpr[i] ? m_g_fpr[i] : m_ir->CreateAlloca(GetType<f64>(), nullptr, fmt::format(".f%d", i));
	for (u32 i = 0; i < 32; i++) m_vr[i] = m_g_vr[i] ? m_g_vr[i] : m_ir->Insert(new AllocaInst(GetType<u32[4]>(), nullptr, 16, fmt::format(".v%d", i)));

	for (u32 i = 0; i < 32; i++)
	{
		static const char* const names[]
		{
			"lt",
			"gt",
			"eq",
			"so",
		};

		//m_cr[i] = m_ir->CreateConstGEP2_32(nullptr, m_thread, 0, 97 + i, fmt::format("cr%u.%s", i / 4, names[i % 4]));
		m_cr[i] = m_ir->CreateAlloca(GetType<bool>(), 0, fmt::format("cr%u.%s", i / 4, names[i % 4]));
	}

	m_reg_lr = m_ir->CreateAlloca(GetType<u64>(), nullptr, ".lr");
	m_reg_ctr = m_ir->CreateAlloca(GetType<u64>(), nullptr, ".ctr");
	m_reg_vrsave = m_ir->CreateAlloca(GetType<u32>(), nullptr, ".vrsave");

	m_xer_so = m_ir->CreateAlloca(GetType<bool>(), nullptr, "xer.so");
	m_xer_ov = m_ir->CreateAlloca(GetType<bool>(), nullptr, "xer.ov");
	m_xer_ca = m_ir->CreateAlloca(GetType<bool>(), nullptr, ".carry");
	m_xer_count = m_ir->CreateAlloca(GetType<u8>(), nullptr, "xer.count");

	m_vscr_nj = m_ir->CreateAlloca(GetType<bool>(), nullptr, "vscr.nj");
	m_vscr_sat = m_ir->CreateAlloca(GetType<bool>(), nullptr, "vscr.sat");

	//m_fpscr_fx = m_fpscr[0] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.fx");
	//m_fpscr_ox = m_fpscr[3] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.ox");
	//m_fpscr_ux = m_fpscr[4] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.ux");
	//m_fpscr_zx = m_fpscr[5] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.zx");
	//m_fpscr_xx = m_fpscr[6] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.xx");
	//m_fpscr_vxsnan = m_fpscr[7] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vxsnan");
	//m_fpscr_vxisi = m_fpscr[8] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vxisi");
	//m_fpscr_vxidi = m_fpscr[9] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vxidi");
	//m_fpscr_vxzdz = m_fpscr[10] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vxzdz");
	//m_fpscr_vximz = m_fpscr[11] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vximz");
	//m_fpscr_vxvc = m_fpscr[12] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vxvc");
	//m_fpscr_fr = m_fpscr[13] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.fr");
	//m_fpscr_fi = m_fpscr[14] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.fi");
	//m_fpscr_c = m_fpscr[15] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.c");
	m_fpscr_lt = m_fpscr[16] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.lt");
	m_fpscr_gt = m_fpscr[17] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.gt");
	m_fpscr_eq = m_fpscr[18] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.eq");
	m_fpscr_un = m_fpscr[19] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.un");
	//m_fpscr_reserved = m_fpscr[20] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.52");
	//m_fpscr_vxsoft = m_fpscr[21] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vxsoft");
	//m_fpscr_vxsqrt = m_fpscr[22] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vxsqrt");
	//m_fpscr_vxcvi = m_fpscr[23] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.vxcvi");
	//m_fpscr_ve = m_fpscr[24] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.ve");
	//m_fpscr_oe = m_fpscr[25] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.oe");
	//m_fpscr_ue = m_fpscr[26] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.ue");
	//m_fpscr_ze = m_fpscr[27] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.ze");
	//m_fpscr_xe = m_fpscr[28] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.xe");
	//m_fpscr_ni = m_fpscr[29] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.ni");
	//m_fpscr_rnh = m_fpscr[30] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.rn.msb");
	//m_fpscr_rnl = m_fpscr[31] = m_ir->CreateAlloca(GetType<bool>(), nullptr, "fpscr.rn.lsb");

	/* Initialize local variables */
	m_ir->CreateStore(m_ir->getFalse(), m_xer_so); // XER.SO
	m_ir->CreateStore(m_ir->getFalse(), m_vscr_sat); // VSCR.SAT
	m_ir->CreateStore(m_ir->getTrue(), m_vscr_nj);

	// TODO: only loaded r12 (extended argument for program initialization)
	m_ir->CreateStore(m_ir->CreateLoad(m_ir->CreateConstGEP2_32(nullptr, m_thread, 0, 1 + 12)), m_gpr[12]);

	m_jtr = BasicBlock::Create(m_context, "__jtr", m_function);

	/* Convert each instruction to LLVM IR */
	const auto start = GetBasicBlock(m_start_addr);
	m_ir->CreateBr(start);
	m_ir->SetInsertPoint(start);

	for (m_current_addr = start_addr; m_current_addr < end_addr;)
	{
		// Preserve current address (m_current_addr may be changed by the decoder)
		const u64 addr = m_current_addr;

		// Translate opcode
		const u32 op = *(m_bin = bin + (addr - start_addr) / sizeof(u32));
		(this->*(s_ppu_decoder.decode(op)))({op});

		// Calculate next address if necessary
		if (m_current_addr == addr) m_current_addr += sizeof(u32);

		// Get next block
		const auto next = GetBasicBlock(m_current_addr);

		// Finalize current block if necessary (create branch to next address)
		if (!m_ir->GetInsertBlock()->getTerminator())
		{
			m_ir->CreateBr(next);
		}

		// Start next block
		m_ir->SetInsertPoint(next);
	}

	// Run custom IR generation function
	if (custom) custom(this);

	// Finalize past-the-end block
	if (!m_ir->GetInsertBlock()->getTerminator())
	{
		Call(GetType<void>(), "__end", m_ir->getInt64(end_addr));
		m_ir->CreateUnreachable();
	}

	m_ir->SetInsertPoint(m_jtr);

	if (m_jtr->use_empty())
	{
		m_ir->CreateUnreachable();
	}
	else
	{
		// Get block entries
		const std::vector<u64> cases{m_block_info.upper_bound(start_addr), m_block_info.lower_bound(end_addr)};

		const auto _ctr = m_ir->CreateLoad(m_reg_ctr);
		const auto _default = BasicBlock::Create(m_context, "__jtr.def", m_function);
		const auto _switch = m_ir->CreateSwitch(_ctr, _default, ::size32(cases));

		for (const u64 addr : cases)
		{
			_switch->addCase(m_ir->getInt64(addr), GetBasicBlock(addr));
		}

		m_ir->SetInsertPoint(_default);
		CallFunction(0, true, _ctr);
	}

	//for (auto i = inst_begin(*m_function), end = inst_end(*m_function); i != end;)
	//{
	//	const auto inst = &*i++;

	//	// Remove unnecessary stores of global variables created by PrepareGlobalArguments() and similar functions
	//	if (const auto si = dyn_cast<StoreInst>(inst))
	//	{
	//		const auto g = dyn_cast<GlobalVariable>(si->getOperand(1));

	//		if (g && m_value_usage[g] == 0)
	//		{
	//			si->eraseFromParent();
	//			continue;
	//		}
	//	}
	//}

	return m_function;
}

Type* PPUTranslator::ScaleType(Type* type, s32 pow2)
{
	EXPECTS(type->getScalarType()->isIntegerTy());

	const auto new_type = m_ir->getIntNTy(type->getScalarSizeInBits() * std::pow(2, pow2));
	return type->isVectorTy() ? VectorType::get(new_type, type->getVectorNumElements()) : cast<Type>(new_type);
}

Value* PPUTranslator::DuplicateExt(Value* arg)
{
	const auto extended = ZExt(arg);
	return m_ir->CreateOr(extended, m_ir->CreateShl(extended, arg->getType()->getScalarSizeInBits()));
}

Value* PPUTranslator::RotateLeft(Value* arg, u64 n)
{
	return !n ? arg : m_ir->CreateOr(m_ir->CreateShl(arg, n), m_ir->CreateLShr(arg, arg->getType()->getScalarSizeInBits() - n));
}

Value* PPUTranslator::RotateLeft(Value* arg, Value* n)
{
	const u64 mask = arg->getType()->getScalarSizeInBits() - 1;

	return m_ir->CreateOr(m_ir->CreateShl(arg, m_ir->CreateAnd(n, mask)), m_ir->CreateLShr(arg, m_ir->CreateAnd(m_ir->CreateNeg(n), mask)));
}

void PPUTranslator::CallFunction(u64 target, bool tail, Value* indirect)
{
	const auto func = indirect ? nullptr : m_func_list[target];

	const auto callee_type = func ? m_func_types[target] : nullptr;

	if (func)
	{
		m_ir->CreateCall(func, {m_thread});
	}
	else
	{
		const auto addr = indirect ? indirect : (Value*)m_ir->getInt64(target);
		const auto pos = m_ir->CreateLShr(addr, 2, "", true);
		const auto ptr = m_ir->CreateGEP(m_call, {m_ir->getInt64(0), pos});
		m_ir->CreateCall(m_ir->CreateLoad(ptr), {m_thread});
	}

	if (!tail)
	{
		UndefineVolatileRegisters();
	}

	if (tail)
	{
		m_ir->CreateRetVoid();
	}
}

void PPUTranslator::UndefineVolatileRegisters()
{
	const auto undef_i64 = GetUndef<u64>();
	const auto undef_f64 = GetUndef<f64>();
	const auto undef_vec = GetUndef<u32[4]>();
	const auto undef_bool = GetUndef<bool>();

	// Undefine local volatile registers
	SetGpr(0, undef_i64); // r0
	SetFpr(0, undef_f64); // f0: volatile scratch register
	SetVr(0, undef_vec); // v0: volatile scratch register
	SetVr(1, undef_vec); // v1: volatile scratch register

	m_ir->CreateStore(undef_i64, m_reg_lr); // LR
	m_ir->CreateStore(undef_i64, m_reg_ctr); // CTR
	m_ir->CreateStore(undef_bool, m_xer_ca); // XER.CA

	m_ir->CreateStore(undef_bool, m_fpscr_lt); 
	m_ir->CreateStore(undef_bool, m_fpscr_gt);
	m_ir->CreateStore(undef_bool, m_fpscr_eq);
	m_ir->CreateStore(undef_bool, m_fpscr_un);

	SetCrField(0, undef_bool, undef_bool, undef_bool, undef_bool); // cr0
	SetCrField(1, undef_bool, undef_bool, undef_bool, undef_bool); // cr1
	SetCrField(5, undef_bool, undef_bool, undef_bool, undef_bool); // cr5
	SetCrField(6, undef_bool, undef_bool, undef_bool, undef_bool); // cr6
	SetCrField(7, undef_bool, undef_bool, undef_bool, undef_bool); // cr7

	// Cannot undef sticky flags because it makes |= op meaningless
	//m_ir->CreateStore(m_ir->getFalse(), m_xer_so); // XER.SO
	//m_ir->CreateStore(m_ir->getFalse(), m_vscr_sat); // VSCR.SAT 
}

BasicBlock* PPUTranslator::GetBasicBlock(u64 address)
{
	if (auto& block = m_blocks[address])
	{
		return block;
	}
	else
	{
		return block = BasicBlock::Create(m_context, fmt::format("loc_%llx", address/* - m_start_addr*/), m_function);
	}
}

Value* PPUTranslator::Solid(Value* value)
{
	const u32 size = value->getType()->getPrimitiveSizeInBits();

	/* Workarounds (casting bool vectors directly may produce invalid code) */
	
	if (value->getType() == GetType<bool[4]>())
	{
		return m_ir->CreateBitCast(SExt(value, GetType<u32[4]>()), m_ir->getIntNTy(128));
	}

	if (value->getType() == GetType<bool[8]>())
	{
		return m_ir->CreateBitCast(SExt(value, GetType<u16[8]>()), m_ir->getIntNTy(128));
	}

	if (value->getType() == GetType<bool[16]>())
	{
		return m_ir->CreateBitCast(SExt(value, GetType<u8[16]>()), m_ir->getIntNTy(128));
	}

	return m_ir->CreateBitCast(value, m_ir->getIntNTy(size));
}

Value* PPUTranslator::IsZero(Value* value)
{
	return m_ir->CreateIsNull(Solid(value));
}

Value* PPUTranslator::IsNotZero(Value* value)
{
	return m_ir->CreateIsNotNull(Solid(value));
}

Value* PPUTranslator::IsOnes(Value* value)
{
	value = Solid(value);
	return m_ir->CreateICmpEQ(value, ConstantInt::getSigned(value->getType(), -1));
}

Value* PPUTranslator::IsNotOnes(Value* value)
{
	value = Solid(value);
	return m_ir->CreateICmpNE(value, ConstantInt::getSigned(value->getType(), -1));
}

Value* PPUTranslator::Broadcast(Value* value, u32 count)
{
	if (const auto cv = dyn_cast<Constant>(value))
	{
		return ConstantVector::getSplat(count, cv);
	}

	return m_ir->CreateVectorSplat(count, value);
}

std::pair<Value*, Value*> PPUTranslator::Saturate(Value* value, CmpInst::Predicate inst, Value* extreme)
{
	// Modify args
	if (value->getType()->isVectorTy() && !extreme->getType()->isVectorTy())
		extreme = Broadcast(extreme, value->getType()->getVectorNumElements());
	if (extreme->getType()->isVectorTy() && !value->getType()->isVectorTy())
		value = Broadcast(value, extreme->getType()->getVectorNumElements());

	// Compare args
	const auto cmp = m_ir->CreateICmp(inst, value, extreme);

	// Return saturated result and saturation bitmask
	return{m_ir->CreateSelect(cmp, extreme, value), cmp};
}

std::pair<Value*, Value*> PPUTranslator::SaturateSigned(Value* value, u64 min, u64 max)
{
	const auto type = value->getType()->getScalarType();
	const auto sat_l = Saturate(value, ICmpInst::ICMP_SLT, ConstantInt::get(type, min, true));
	const auto sat_h = Saturate(sat_l.first, ICmpInst::ICMP_SGT, ConstantInt::get(type, max, true));

	// Return saturated result and saturation bitmask
	return{sat_h.first, m_ir->CreateOr(sat_l.second, sat_h.second)};
}

Value* PPUTranslator::Scale(Value* value, s32 scale)
{
	if (scale)
	{
		const auto type = value->getType();
		const auto power = std::pow(2, scale);

		if (type->isVectorTy())
		{
			return m_ir->CreateFMul(value, ConstantVector::getSplat(type->getVectorNumElements(), ConstantFP::get(type->getVectorElementType(), power)));
		}
		else
		{
			return m_ir->CreateFMul(value, ConstantFP::get(type, power));
		}
	}

	return value;
}

Value* PPUTranslator::Shuffle(Value* left, Value* right, std::initializer_list<u32> indices)
{
	const auto type = left->getType();

	if (!right)
	{
		right = UndefValue::get(type);
	}

	if (!m_is_be)
	{
		std::vector<u32> data; data.reserve(indices.size());

		const u32 mask = type->getVectorNumElements() - 1;

		// Transform indices (works for vectors with size 2^N)
		for (std::size_t i = 0; i < indices.size(); i++)
		{
			data.push_back(indices.end()[~i] ^ mask);
		}

		return m_ir->CreateShuffleVector(left, right, ConstantDataVector::get(m_context, data));
	}

	return m_ir->CreateShuffleVector(left, right, ConstantDataVector::get(m_context, { indices.begin(), indices.end() }));
}

Value* PPUTranslator::SExt(Value* value, Type* type)
{
	return m_ir->CreateSExt(value, type ? type : ScaleType(value->getType(), 1));
}

Value* PPUTranslator::ZExt(Value* value, Type* type)
{
	return m_ir->CreateZExt(value, type ? type : ScaleType(value->getType(), 1));
}

Value* PPUTranslator::Add(std::initializer_list<Value*> args)
{
	Value* result{};
	for (auto arg : args)
	{
		result = result ? m_ir->CreateAdd(result, arg) : arg;
	}

	return result;
}

Value* PPUTranslator::Trunc(Value* value, Type* type)
{
	return m_ir->CreateTrunc(value, type ? type : ScaleType(value->getType(), -1));
}

void PPUTranslator::UseCondition(Value* cond)
{
	if (cond)
	{
		const auto local = BasicBlock::Create(m_context, fmt::format("loc_%llx.cond", m_current_addr/* - m_start_addr*/), m_function);
		m_ir->CreateCondBr(cond, local, GetBasicBlock(m_current_addr + 4));
		m_ir->SetInsertPoint(local);
	}
}

llvm::Value* PPUTranslator::GetMemory(llvm::Value* addr, llvm::Type* type)
{
	return m_ir->CreateBitCast(m_ir->CreateGEP(m_base, {m_ir->getInt64(0), addr}), type->getPointerTo());
}

Value* PPUTranslator::ReadMemory(Value* addr, Type* type, bool is_be, u32 align)
{
	const auto size = type->getPrimitiveSizeInBits();

	if (is_be ^ m_is_be && size > 8)
	{
		// Read, byteswap, bitcast
		const auto int_type = m_ir->getIntNTy(size);
		const auto value = m_ir->CreateAlignedLoad(GetMemory(addr, int_type), align, !IsStackAddr(addr));
		return m_ir->CreateBitCast(Call(int_type, fmt::format("llvm.bswap.i%u", size), value), type);
	}

	// Read normally
	return m_ir->CreateAlignedLoad(GetMemory(addr, type), align, !IsStackAddr(addr));
}

void PPUTranslator::WriteMemory(Value* addr, Value* value, bool is_be, u32 align)
{
	const auto type = value->getType();
	const auto size = type->getPrimitiveSizeInBits();

	if (is_be ^ m_is_be && size > 8)
	{
		// Bitcast, byteswap
		const auto int_type = m_ir->getIntNTy(size);
		value = Call(int_type, fmt::format("llvm.bswap.i%u", size), m_ir->CreateBitCast(value, int_type));
	}

	// Write
	m_ir->CreateAlignedStore(value, GetMemory(addr, value->getType()), align, !IsStackAddr(addr));
}

void PPUTranslator::CompilationError(const std::string& error)
{
	LOG_ERROR(PPU, "0x%08llx: Error: %s", m_current_addr, error);
}


void PPUTranslator::MFVSCR(ppu_opcode_t op)
{
	const auto vscr = m_ir->CreateOr(ZExt(m_ir->CreateLoad(m_vscr_sat), GetType<u32>()), m_ir->CreateShl(ZExt(m_ir->CreateLoad(m_vscr_nj), GetType<u32>()), 16));
	SetVr(op.vd, m_ir->CreateInsertElement(ConstantVector::getSplat(4, m_ir->getInt32(0)), vscr, m_ir->getInt32(m_is_be ? 3 : 0)));
}

void PPUTranslator::MTVSCR(ppu_opcode_t op)
{
	const auto vscr = m_ir->CreateExtractElement(GetVr(op.vb, VrType::vi32), m_ir->getInt32(m_is_be ? 3 : 0));
	m_ir->CreateStore(Trunc(m_ir->CreateLShr(vscr, 16), GetType<bool>()), m_vscr_nj);
	m_ir->CreateStore(Trunc(vscr, GetType<bool>()), m_vscr_sat);
}

void PPUTranslator::VADDCUW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, ZExt(m_ir->CreateICmpULT(m_ir->CreateAdd(ab[0], ab[1]), ab[0]), GetType<u32[4]>()));
}

void PPUTranslator::VADDFP(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vf, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateFAdd(ab[0], ab[1]));
}

void PPUTranslator::VADDSBS(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi8, op.va, op.vb));
	const auto result = m_ir->CreateAdd(ab[0], ab[1]);
	const auto saturated = SaturateSigned(result, -0x80, 0x7f);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VADDSHS(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi16, op.va, op.vb));
	const auto result = m_ir->CreateAdd(ab[0], ab[1]);
	const auto saturated = SaturateSigned(result, -0x8000, 0x7fff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VADDSWS(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi32, op.va, op.vb));
	const auto result = m_ir->CreateAdd(ab[0], ab[1]);
	const auto saturated = SaturateSigned(result, -0x80000000ll, 0x7fffffff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VADDUBM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateAdd(ab[0], ab[1]));
}

void PPUTranslator::VADDUBS(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi8, op.va, op.vb));
	const auto result = m_ir->CreateAdd(ab[0], ab[1]);
	const auto saturated = Saturate(result, ICmpInst::ICMP_UGT, m_ir->getInt16(0xff));
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VADDUHM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateAdd(ab[0], ab[1]));
}

void PPUTranslator::VADDUHS(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi16, op.va, op.vb));
	const auto result = m_ir->CreateAdd(ab[0], ab[1]);
	const auto saturated = Saturate(result, ICmpInst::ICMP_UGT, m_ir->getInt32(0xffff));
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VADDUWM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateAdd(ab[0], ab[1]));
}

void PPUTranslator::VADDUWS(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi32, op.va, op.vb));
	const auto result = m_ir->CreateAdd(ab[0], ab[1]);
	const auto saturated = Saturate(result, ICmpInst::ICMP_UGT, m_ir->getInt64(0xffffffff));
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VAND(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateAnd(ab[0], ab[1]));
}

void PPUTranslator::VANDC(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateAnd(ab[0], m_ir->CreateNot(ab[1])));
}

#define AVG_OP(a, b) m_ir->CreateLShr(m_ir->CreateSub(a, m_ir->CreateNot(b)), 1) /* (a + b + 1) >> 1 */

void PPUTranslator::VAVGSB(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi8, op.va, op.vb));
	SetVr(op.vd, AVG_OP(ab[0], ab[1]));
}

void PPUTranslator::VAVGSH(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi16, op.va, op.vb));
	SetVr(op.vd, AVG_OP(ab[0], ab[1]));
}

void PPUTranslator::VAVGSW(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi32, op.va, op.vb));
	SetVr(op.vd, AVG_OP(ab[0], ab[1]));
}

void PPUTranslator::VAVGUB(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi8, op.va, op.vb));
	SetVr(op.vd, AVG_OP(ab[0], ab[1]));
}

void PPUTranslator::VAVGUH(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi16, op.va, op.vb));
	SetVr(op.vd, AVG_OP(ab[0], ab[1]));
}

void PPUTranslator::VAVGUW(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi32, op.va, op.vb));
	SetVr(op.vd, AVG_OP(ab[0], ab[1]));
}

void PPUTranslator::VCFSX(ppu_opcode_t op)
{
	const auto b = GetVr(op.vb, VrType::vi32);
	SetVr(op.vd, Scale(m_ir->CreateSIToFP(b, GetType<f32[4]>()), 0 - op.vuimm));
}

void PPUTranslator::VCFUX(ppu_opcode_t op)
{
	const auto b = GetVr(op.vb, VrType::vi32);
	SetVr(op.vd, Scale(m_ir->CreateUIToFP(b, GetType<f32[4]>()), 0 - op.vuimm));
}

void PPUTranslator::VCMPBFP(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vf, op.va, op.vb);
	const auto nle = m_ir->CreateFCmpUGT(ab[0], ab[1]);
	const auto nge = m_ir->CreateFCmpULT(ab[0], m_ir->CreateFNeg(ab[1]));
	const auto le_bit = m_ir->CreateShl(ZExt(nle, GetType<u32[4]>()), 31);
	const auto ge_bit = m_ir->CreateShl(ZExt(nge, GetType<u32[4]>()), 30);
	const auto result = m_ir->CreateOr(le_bit, ge_bit);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, m_ir->getFalse(), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPEQFP(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vf, op.va, op.vb);
	const auto result = m_ir->CreateFCmpOEQ(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPEQUB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	const auto result = m_ir->CreateICmpEQ(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPEQUH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	const auto result = m_ir->CreateICmpEQ(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPEQUW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto result = m_ir->CreateICmpEQ(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPGEFP(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vf, op.va, op.vb);
	const auto result = m_ir->CreateFCmpOGE(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPGTFP(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vf, op.va, op.vb);
	const auto result = m_ir->CreateFCmpOGT(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPGTSB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	const auto result = m_ir->CreateICmpSGT(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPGTSH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	const auto result = m_ir->CreateICmpSGT(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPGTSW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto result = m_ir->CreateICmpSGT(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPGTUB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	const auto result = m_ir->CreateICmpUGT(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPGTUH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	const auto result = m_ir->CreateICmpUGT(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

void PPUTranslator::VCMPGTUW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto result = m_ir->CreateICmpUGT(ab[0], ab[1]);
	SetVr(op.vd, result);
	if (op.oe) SetCrField(6, IsOnes(result), m_ir->getFalse(), IsZero(result), m_ir->getFalse());
}

#define FP_SAT_OP(fcmp, value) m_ir->CreateSelect(fcmp, cast<Constant>(cast<FCmpInst>(fcmp)->getOperand(1)), value)

void PPUTranslator::VCTSXS(ppu_opcode_t op)
{
	const auto b = GetVr(op.vb, VrType::vf);
	const auto scaled = Scale(b, op.vuimm);
	//const auto is_nan = m_ir->CreateFCmpUNO(b, ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), 0.0))); // NaN -> 0.0
	const auto sat_l = m_ir->CreateFCmpOLT(scaled, ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), -std::pow(2, 31)))); // TODO ???
	const auto sat_h = m_ir->CreateFCmpOGE(scaled, ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), std::pow(2, 31))));
	const auto converted = m_ir->CreateFPToSI(FP_SAT_OP(sat_l, scaled), GetType<s32[4]>());
	SetVr(op.vd, m_ir->CreateSelect(sat_h, ConstantVector::getSplat(4, m_ir->getInt32(0x7fffffff)), converted));
	SetSat(IsNotZero(m_ir->CreateOr(sat_l, sat_h)));
}

void PPUTranslator::VCTUXS(ppu_opcode_t op)
{
	const auto b = GetVr(op.vb, VrType::vf);
	const auto scaled = Scale(b, op.vuimm);
	//const auto is_nan = m_ir->CreateFCmpUNO(b, ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), 0.0))); // NaN -> 0.0
	const auto sat_l = m_ir->CreateFCmpOLT(scaled, ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), 0.0)));
	const auto sat_h = m_ir->CreateFCmpOGE(scaled, ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), std::pow(2, 32)))); // TODO ???
	const auto converted = m_ir->CreateFPToUI(FP_SAT_OP(sat_l, scaled), GetType<u32[4]>());
	SetVr(op.vd, m_ir->CreateSelect(sat_h, ConstantVector::getSplat(4, m_ir->getInt32(0xffffffff)), converted));
	SetSat(IsNotZero(m_ir->CreateOr(sat_l, sat_h)));
}

void PPUTranslator::VEXPTEFP(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<f32[4]>(), m_pure_attr, "__vexptefp", GetVr(op.vb, VrType::vf)));
}

void PPUTranslator::VLOGEFP(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<f32[4]>(), m_pure_attr, "__vlogefp", GetVr(op.vb, VrType::vf)));
}

void PPUTranslator::VMADDFP(ppu_opcode_t op)
{
	const auto acb = GetVrs(VrType::vf, op.va, op.vc, op.vb);
	SetVr(op.vd, m_ir->CreateFAdd(m_ir->CreateFMul(acb[0], acb[1]), acb[2]));
}

void PPUTranslator::VMAXFP(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vf, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateSelect(m_ir->CreateFCmpOGT(ab[0], ab[1]), ab[0], ab[1]));
}

void PPUTranslator::VMAXSB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_SLT, ab[1]).first);
}

void PPUTranslator::VMAXSH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_SLT, ab[1]).first);
}

void PPUTranslator::VMAXSW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_SLT, ab[1]).first);
}

void PPUTranslator::VMAXUB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_ULT, ab[1]).first);
}

void PPUTranslator::VMAXUH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_ULT, ab[1]).first);
}

void PPUTranslator::VMAXUW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_ULT, ab[1]).first);
}

void PPUTranslator::VMHADDSHS(ppu_opcode_t op)
{
	const auto abc = SExt(GetVrs(VrType::vi16, op.va, op.vb, op.vc));
	const auto result = m_ir->CreateAdd(m_ir->CreateAShr(m_ir->CreateMul(abc[0], abc[1]), 15), abc[2]);
	const auto saturated = SaturateSigned(result, -0x8000, 0x7fff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VMHRADDSHS(ppu_opcode_t op)
{
	const auto abc = SExt(GetVrs(VrType::vi16, op.va, op.vb, op.vc));
	const auto result = m_ir->CreateAdd(m_ir->CreateAShr(m_ir->CreateAdd(m_ir->CreateMul(abc[0], abc[1]), ConstantVector::getSplat(8, m_ir->getInt32(0x4000))), 15), abc[2]);
	const auto saturated = SaturateSigned(result, -0x8000, 0x7fff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VMINFP(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vf, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateSelect(m_ir->CreateFCmpOLT(ab[0], ab[1]), ab[0], ab[1]));
}

void PPUTranslator::VMINSB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_SGT, ab[1]).first);
}

void PPUTranslator::VMINSH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_SGT, ab[1]).first);
}

void PPUTranslator::VMINSW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_SGT, ab[1]).first);
}

void PPUTranslator::VMINUB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_UGT, ab[1]).first);
}

void PPUTranslator::VMINUH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_UGT, ab[1]).first);
}

void PPUTranslator::VMINUW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, Saturate(ab[0], CmpInst::ICMP_UGT, ab[1]).first);
}

void PPUTranslator::VMLADDUHM(ppu_opcode_t op)
{
	const auto abc = GetVrs(VrType::vi16, op.va, op.vb, op.vc);
	SetVr(op.vd, m_ir->CreateAdd(m_ir->CreateMul(abc[0], abc[1]), abc[2]));
}

void PPUTranslator::VMRGHB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, Shuffle(ab[0], ab[1], { 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23 }));
}

void PPUTranslator::VMRGHH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, Shuffle(ab[0], ab[1], { 0, 8, 1, 9, 2, 10, 3, 11 }));
}

void PPUTranslator::VMRGHW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, Shuffle(ab[0], ab[1], { 0, 4, 1, 5 }));
}

void PPUTranslator::VMRGLB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, Shuffle(ab[0], ab[1], { 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31 }));
}

void PPUTranslator::VMRGLH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, Shuffle(ab[0], ab[1], { 4, 12, 5, 13, 6, 14, 7, 15 }));
}

void PPUTranslator::VMRGLW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, Shuffle(ab[0], ab[1], { 2, 6, 3, 7 }));
}

void PPUTranslator::VMSUMMBM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	const auto a = SExt(ab[0], GetType<s32[16]>());
	const auto b = ZExt(ab[1], GetType<u32[16]>());
	const auto p = m_ir->CreateMul(a, b);
	const auto c = GetVr(op.vc, VrType::vi32);
	const auto e0 = Shuffle(p, nullptr, { 0, 4, 8, 12 });
	const auto e1 = Shuffle(p, nullptr, { 1, 5, 9, 13 });
	const auto e2 = Shuffle(p, nullptr, { 2, 6, 10, 14 });
	const auto e3 = Shuffle(p, nullptr, { 3, 7, 11, 15 });
	SetVr(op.vd, Add({ c, e0, e1, e2, e3 }));
}

void PPUTranslator::VMSUMSHM(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi16, op.va, op.vb));
	const auto p = m_ir->CreateMul(ab[0], ab[1]);
	const auto c = GetVr(op.vc, VrType::vi32);
	const auto e0 = Shuffle(p, nullptr, { 0, 2, 4, 6 });
	const auto e1 = Shuffle(p, nullptr, { 1, 3, 5, 7 });
	SetVr(op.vd, Add({ c, e0, e1 }));
}

void PPUTranslator::VMSUMSHS(ppu_opcode_t op)
{
	// TODO (very rare)
	/**/ return_ VEC3OP(VMSUMSHS);
	//const auto a = GetVr(op.va, VrType::vi16);
	//const auto b = GetVr(op.vb, VrType::vi16);
	//const auto c = GetVr(op.vc, VrType::vi32);
	//SetVr(op.vd, Call(GetType<u32[4]>(), m_pure_attr, "__vmsumshs", a, b, c));
	//SetSat(Call(GetType<bool>(), m_pure_attr, "__vmsumshs_get_sat", a, b, c));
}

void PPUTranslator::VMSUMUBM(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi8, op.va, op.vb), GetType<u32[16]>());
	const auto p = m_ir->CreateMul(ab[0], ab[1]);
	const auto c = GetVr(op.vc, VrType::vi32);
	const auto e0 = Shuffle(p, nullptr, { 0, 4, 8, 12 });
	const auto e1 = Shuffle(p, nullptr, { 1, 5, 9, 13 });
	const auto e2 = Shuffle(p, nullptr, { 2, 6, 10, 14 });
	const auto e3 = Shuffle(p, nullptr, { 3, 7, 11, 15 });
	SetVr(op.vd, Add({ c, e0, e1, e2, e3 }));
}

void PPUTranslator::VMSUMUHM(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi16, op.va, op.vb));
	const auto p = m_ir->CreateMul(ab[0], ab[1]);
	const auto c = GetVr(op.vc, VrType::vi32);
	const auto e0 = Shuffle(p, nullptr, { 0, 2, 4, 6 });
	const auto e1 = Shuffle(p, nullptr, { 1, 3, 5, 7 });
	SetVr(op.vd, Add({ c, e0, e1 }));
}

void PPUTranslator::VMSUMUHS(ppu_opcode_t op)
{
	// TODO (very rare)
	/**/ return_ VEC3OP(VMSUMUHS);
	//const auto a = GetVr(op.va, VrType::vi16);
	//const auto b = GetVr(op.vb, VrType::vi16);
	//const auto c = GetVr(op.vc, VrType::vi32);
	//SetVr(op.vd, Call(GetType<u32[4]>(), m_pure_attr, "__vmsumuhs", a, b, c));
	//SetSat(Call(GetType<bool>(), m_pure_attr, "__vmsumuhs_get_sat", a, b, c));
}

void PPUTranslator::VMULESB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateMul(m_ir->CreateAShr(ab[0], 8), m_ir->CreateAShr(ab[1], 8)));
}

void PPUTranslator::VMULESH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateMul(m_ir->CreateAShr(ab[0], 16), m_ir->CreateAShr(ab[1], 16)));
}

void PPUTranslator::VMULEUB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateMul(m_ir->CreateLShr(ab[0], 8), m_ir->CreateLShr(ab[1], 8)));
}

void PPUTranslator::VMULEUH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateMul(m_ir->CreateLShr(ab[0], 16), m_ir->CreateLShr(ab[1], 16)));
}

void PPUTranslator::VMULOSB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateMul(m_ir->CreateAShr(m_ir->CreateShl(ab[0], 8), 8), m_ir->CreateAShr(m_ir->CreateShl(ab[1], 8), 8)));
}

void PPUTranslator::VMULOSH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateMul(m_ir->CreateAShr(m_ir->CreateShl(ab[0], 16), 16), m_ir->CreateAShr(m_ir->CreateShl(ab[1], 16), 16)));
}

void PPUTranslator::VMULOUB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateMul(m_ir->CreateLShr(m_ir->CreateShl(ab[0], 8), 8), m_ir->CreateLShr(m_ir->CreateShl(ab[1], 8), 8)));
}

void PPUTranslator::VMULOUH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateMul(m_ir->CreateLShr(m_ir->CreateShl(ab[0], 16), 16), m_ir->CreateLShr(m_ir->CreateShl(ab[1], 16), 16)));
}

void PPUTranslator::VNMSUBFP(ppu_opcode_t op)
{
	const auto acb = GetVrs(VrType::vf, op.va, op.vc, op.vb);
	SetVr(op.vd, m_ir->CreateFSub(acb[2], m_ir->CreateFMul(acb[0], acb[1])));
}

void PPUTranslator::VNOR(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateNot(m_ir->CreateOr(ab[0], ab[1])));
}

void PPUTranslator::VOR(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateOr(ab[0], ab[1]));
}

void PPUTranslator::VPERM(ppu_opcode_t op)
{
	const auto abc = GetVrs(VrType::vi8, op.va, op.vb, op.vc);
	SetVr(op.vd, Call(GetType<u8[16]>(), m_pure_attr, "__vperm", abc[0], abc[1], abc[2]));
}

void PPUTranslator::VPKPX(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto px = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7 });
	const auto e1 = m_ir->CreateLShr(m_ir->CreateAnd(px, 0x01f80000), 9);
	const auto e2 = m_ir->CreateLShr(m_ir->CreateAnd(px, 0xf800), 6);
	const auto e3 = m_ir->CreateLShr(m_ir->CreateAnd(px, 0xf8), 3);
	SetVr(op.vd, m_ir->CreateOr(m_ir->CreateOr(e1, e2), e3));
}

void PPUTranslator::VPKSHSS(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	const auto src = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
	const auto saturated = SaturateSigned(src, -0x80, 0x7f);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VPKSHUS(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	const auto src = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
	const auto saturated = SaturateSigned(src, 0, 0xff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VPKSWSS(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto src = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7 });
	const auto saturated = SaturateSigned(src, -0x8000, 0x7fff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VPKSWUS(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto src = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7 });
	const auto saturated = SaturateSigned(src, 0, 0xffff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VPKUHUM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	const auto src = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
	SetVr(op.vd, src); // Truncate
}

void PPUTranslator::VPKUHUS(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	const auto src = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
	const auto saturated = Saturate(src, ICmpInst::ICMP_UGT, m_ir->getInt16(0xff));
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VPKUWUM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto src = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7 });
	SetVr(op.vd, src); // Truncate
}

void PPUTranslator::VPKUWUS(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto src = Shuffle(ab[0], ab[1], { 0, 1, 2, 3, 4, 5, 6, 7 });
	const auto saturated = Saturate(src, ICmpInst::ICMP_UGT, m_ir->getInt32(0xffff));
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VREFP(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<f32[4]>(), m_pure_attr, "__vrefp", GetVr(op.vb, VrType::vf)));
}

void PPUTranslator::VRFIM(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<f32[4]>(), "llvm.floor.v4f32", GetVr(op.vb, VrType::vf)));
}

void PPUTranslator::VRFIN(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<f32[4]>(), "llvm.nearbyint.v4f32", GetVr(op.vb, VrType::vf)));
}

void PPUTranslator::VRFIP(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<f32[4]>(), "llvm.ceil.v4f32", GetVr(op.vb, VrType::vf)));
}

void PPUTranslator::VRFIZ(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<f32[4]>(), "llvm.trunc.v4f32", GetVr(op.vb, VrType::vf)));
}

void PPUTranslator::VRLB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, RotateLeft(ab[0], ab[1]));
}

void PPUTranslator::VRLH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, RotateLeft(ab[0], ab[1]));
}

void PPUTranslator::VRLW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, RotateLeft(ab[0], ab[1]));
}

void PPUTranslator::VRSQRTEFP(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<f32[4]>(), m_pure_attr, "__vrsqrtefp", GetVr(op.vb, VrType::vf)));
}

void PPUTranslator::VSEL(ppu_opcode_t op)
{
	const auto abc = GetVrs(VrType::vi32, op.va, op.vb, op.vc);
	SetVr(op.vd, m_ir->CreateOr(m_ir->CreateAnd(abc[1], abc[2]), m_ir->CreateAnd(abc[0], m_ir->CreateNot(abc[2]))));
}

void PPUTranslator::VSL(ppu_opcode_t op)
{
	// TODO (very rare)
	SetVr(op.vd, m_ir->CreateShl(GetVr(op.va, VrType::i128), m_ir->CreateAnd(GetVr(op.vb, VrType::i128), 7)));
}

void PPUTranslator::VSLB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateShl(ab[0], m_ir->CreateAnd(ab[1], 7)));
}

void PPUTranslator::VSLDOI(ppu_opcode_t op)
{
	if (op.vsh == 0)
	{
		const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
		SetVr(op.vd, ab[0]);
	}
	else if ((op.vsh % 4) == 0)
	{
		const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
		const auto s = op.vsh / 4;
		SetVr(op.vd, Shuffle(ab[0], ab[1], { s, s + 1, s + 2, s + 3 }));
	}
	else if ((op.vsh % 2) == 0)
	{
		const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
		const auto s = op.vsh / 2;
		SetVr(op.vd, Shuffle(ab[0], ab[1], { s, s + 1, s + 2, s + 3, s + 4, s + 5, s + 6, s + 7 }));
	}
	else
	{
		const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
		const auto s = op.vsh;
		SetVr(op.vd, Shuffle(ab[0], ab[1], { s, s + 1, s + 2, s + 3, s + 4, s + 5, s + 6, s + 7, s + 8, s + 9, s + 10, s + 11, s + 12, s + 13, s + 14, s + 15 }));
	}
}

void PPUTranslator::VSLH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateShl(ab[0], m_ir->CreateAnd(ab[1], 15)));
}

void PPUTranslator::VSLO(ppu_opcode_t op)
{
	// TODO (rare)
	SetVr(op.vd, m_ir->CreateShl(GetVr(op.va, VrType::i128), m_ir->CreateShl(m_ir->CreateAnd(GetVr(op.vb, VrType::i128), 15), 3)));
}

void PPUTranslator::VSLW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateShl(ab[0], m_ir->CreateAnd(ab[1], 31)));
}

void PPUTranslator::VSPLTB(ppu_opcode_t op)
{
	const u32 ui = op.vuimm & 0xf;
	SetVr(op.vd, Shuffle(GetVr(op.vb, VrType::vi8), nullptr, { ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui }));
}

void PPUTranslator::VSPLTH(ppu_opcode_t op)
{
	const u32 ui = op.vuimm & 0x7;
	SetVr(op.vd, Shuffle(GetVr(op.vb, VrType::vi16), nullptr, { ui, ui, ui, ui, ui, ui, ui, ui }));
}

void PPUTranslator::VSPLTISB(ppu_opcode_t op)
{
	SetVr(op.vd, ConstantVector::getSplat(16, m_ir->getInt8(op.vsimm)));
}

void PPUTranslator::VSPLTISH(ppu_opcode_t op)
{
	SetVr(op.vd, ConstantVector::getSplat(8, m_ir->getInt16(op.vsimm)));
}

void PPUTranslator::VSPLTISW(ppu_opcode_t op)
{
	SetVr(op.vd, ConstantVector::getSplat(4, m_ir->getInt32(op.vsimm)));
}

void PPUTranslator::VSPLTW(ppu_opcode_t op)
{
	const u32 ui = op.vuimm & 0x3;
	SetVr(op.vd, Shuffle(GetVr(op.vb, VrType::vi32), nullptr, { ui, ui, ui, ui }));
}

void PPUTranslator::VSR(ppu_opcode_t op)
{
	// TODO (very rare)
	SetVr(op.vd, m_ir->CreateLShr(GetVr(op.va, VrType::i128), m_ir->CreateAnd(GetVr(op.vb, VrType::i128), 7)));
}

void PPUTranslator::VSRAB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateAShr(ab[0], m_ir->CreateAnd(ab[1], 7)));
}

void PPUTranslator::VSRAH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateAShr(ab[0], m_ir->CreateAnd(ab[1], 15)));
}

void PPUTranslator::VSRAW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateAShr(ab[0], m_ir->CreateAnd(ab[1], 31)));
}

void PPUTranslator::VSRB(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateLShr(ab[0], m_ir->CreateAnd(ab[1], 7)));
}

void PPUTranslator::VSRH(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateLShr(ab[0], m_ir->CreateAnd(ab[1], 15)));
}

void PPUTranslator::VSRO(ppu_opcode_t op)
{
	// TODO (very rare)
	SetVr(op.vd, m_ir->CreateLShr(GetVr(op.va, VrType::i128), m_ir->CreateShl(m_ir->CreateAnd(GetVr(op.vb, VrType::i128), 15), 3)));
}

void PPUTranslator::VSRW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateLShr(ab[0], m_ir->CreateAnd(ab[1], 31)));
}

void PPUTranslator::VSUBCUW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, ZExt(m_ir->CreateICmpUGE(ab[0], ab[1]), GetType<u32[4]>()));
}

void PPUTranslator::VSUBFP(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vf, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateFSub(ab[0], ab[1]));
}

void PPUTranslator::VSUBSBS(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi8, op.va, op.vb));
	const auto result = m_ir->CreateSub(ab[0], ab[1]);
	const auto saturated = SaturateSigned(result, -0x80, 0x7f);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUBSHS(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi16, op.va, op.vb));
	const auto result = m_ir->CreateSub(ab[0], ab[1]);
	const auto saturated = SaturateSigned(result, -0x8000, 0x7fff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUBSWS(ppu_opcode_t op)
{
	const auto ab = SExt(GetVrs(VrType::vi32, op.va, op.vb));
	const auto result = m_ir->CreateSub(ab[0], ab[1]);
	const auto saturated = SaturateSigned(result, -0x80000000ll, 0x7fffffff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUBUBM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi8, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateSub(ab[0], ab[1]));
}

void PPUTranslator::VSUBUBS(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi8, op.va, op.vb));
	const auto result = m_ir->CreateSub(ab[0], ab[1]);
	const auto saturated = Saturate(result, ICmpInst::ICMP_SLT, m_ir->getInt16(0));
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUBUHM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi16, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateSub(ab[0], ab[1]));
}

void PPUTranslator::VSUBUHS(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi16, op.va, op.vb));
	const auto result = m_ir->CreateSub(ab[0], ab[1]);
	const auto saturated = Saturate(result, ICmpInst::ICMP_SLT, m_ir->getInt32(0));
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUBUWM(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateSub(ab[0], ab[1]));
}

void PPUTranslator::VSUBUWS(ppu_opcode_t op)
{
	const auto ab = ZExt(GetVrs(VrType::vi32, op.va, op.vb));
	const auto result = m_ir->CreateSub(ab[0], ab[1]);
	const auto saturated = Saturate(result, ICmpInst::ICMP_SLT, m_ir->getInt64(0));
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUMSWS(ppu_opcode_t op)
{
	// TODO (rare)
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto a = SExt(ab[0]);
	const auto b = SExt(m_ir->CreateExtractElement(ab[1], m_ir->getInt32(m_is_be ? 3 : 0)));
	const auto e0 = m_ir->CreateExtractElement(a, m_ir->getInt32(0));
	const auto e1 = m_ir->CreateExtractElement(a, m_ir->getInt32(1));
	const auto e2 = m_ir->CreateExtractElement(a, m_ir->getInt32(2));
	const auto e3 = m_ir->CreateExtractElement(a, m_ir->getInt32(3));
	const auto saturated = SaturateSigned(Add({ b, e0, e1, e2, e3 }), -0x80000000ll, 0x7fffffff);
	SetVr(op.vd, ZExt(m_ir->CreateAnd(saturated.first, 0xffffffff)));
	SetSat(saturated.second);
}

void PPUTranslator::VSUM2SWS(ppu_opcode_t op)
{
	// TODO (rare)
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	const auto b = SExt(Shuffle(ab[1], nullptr, { 1, 3 }));
	const auto a = SExt(ab[0]);
	const auto e0 = Shuffle(a, nullptr, { 0, 2 });
	const auto e1 = Shuffle(a, nullptr, { 1, 3 });
	const auto saturated = SaturateSigned(Add({ b, e0, e1 }), -0x80000000ll, 0x7fffffff);
	SetVr(op.vd, m_ir->CreateAnd(saturated.first, 0xffffffff));
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUM4SBS(ppu_opcode_t op)
{
	// TODO (very rare)
	/**/ return_ VEC2OP(VSUM4SBS);
	//const auto a = GetVr(op.va, VrType::vi8);
	//const auto b = GetVr(op.vb, VrType::vi32);
	//SetVr(op.vd, Call(GetType<u32[4]>(), m_pure_attr, "__vsum4sbs", a, b));
	//SetSat(Call(GetType<bool>(), m_pure_attr, "__vsum4sbs_get_sat", a, b));
}

void PPUTranslator::VSUM4SHS(ppu_opcode_t op)
{
	// TODO (very rare)
	/**/ return_ VEC2OP(VSUM4SHS);
	//const auto a = GetVr(op.va, VrType::vi16);
	//const auto b = GetVr(op.vb, VrType::vi32);
	//SetVr(op.vd, Call(GetType<u32[4]>(), m_pure_attr, "__vsum4shs", a, b));
	//SetSat(Call(GetType<bool>(), m_pure_attr, "__vsum4shs_get_sat", a, b));
}

void PPUTranslator::VSUM4UBS(ppu_opcode_t op)
{
	// TODO
	const auto a = ZExt(GetVr(op.va, VrType::vi8), GetType<u32[16]>());
	const auto b = GetVr(op.vb, VrType::vi32);
	const auto e0 = Shuffle(a, nullptr, { 0, 4, 8, 12 });
	const auto e1 = Shuffle(a, nullptr, { 1, 5, 9, 13 });
	const auto e2 = Shuffle(a, nullptr, { 2, 6, 10, 14 });
	const auto e3 = Shuffle(a, nullptr, { 3, 7, 11, 15 });
	const auto r = Add({ b, e0, e1, e2, e3 }); // Summ, (e0+e1+e2+e3) is small
	const auto s = m_ir->CreateICmpULT(r, b); // Carry (saturation)
	SetVr(op.vd, m_ir->CreateSelect(s, ConstantVector::getSplat(4, m_ir->getInt32(0xffffffff)), r));
	SetSat(IsNotZero(s));
}

#define UNPACK_PIXEL_OP(px) m_ir->CreateOr(m_ir->CreateAnd(px, 0xff00001f), m_ir->CreateOr(m_ir->CreateAnd(m_ir->CreateShl(px, 6), 0x1f0000), m_ir->CreateAnd(m_ir->CreateShl(px, 3), 0x1f00)))

void PPUTranslator::VUPKHPX(ppu_opcode_t op)
{
	const auto px = SExt(Shuffle(GetVr(op.vb, VrType::vi16), nullptr, { 0, 1, 2, 3 }));
	SetVr(op.vd, UNPACK_PIXEL_OP(px));
}

void PPUTranslator::VUPKHSB(ppu_opcode_t op)
{
	SetVr(op.vd, SExt(Shuffle(GetVr(op.vb, VrType::vi8), nullptr, { 0, 1, 2, 3, 4, 5, 6, 7 })));
}

void PPUTranslator::VUPKHSH(ppu_opcode_t op)
{
	SetVr(op.vd, SExt(Shuffle(GetVr(op.vb, VrType::vi16), nullptr, { 0, 1, 2, 3 })));
}

void PPUTranslator::VUPKLPX(ppu_opcode_t op)
{
	const auto px = SExt(Shuffle(GetVr(op.vb, VrType::vi16), nullptr, { 4, 5, 6, 7 }));
	SetVr(op.vd, UNPACK_PIXEL_OP(px));
}

void PPUTranslator::VUPKLSB(ppu_opcode_t op)
{
	SetVr(op.vd, SExt(Shuffle(GetVr(op.vb, VrType::vi8), nullptr, { 8, 9, 10, 11, 12, 13, 14, 15 })));
}

void PPUTranslator::VUPKLSH(ppu_opcode_t op)
{
	SetVr(op.vd, SExt(Shuffle(GetVr(op.vb, VrType::vi16), nullptr, { 4, 5, 6, 7 })));
}

void PPUTranslator::VXOR(ppu_opcode_t op)
{
	if (op.va == op.vb)
	{
		// Assign zero, break dependencies
		SetVr(op.vd, ConstantVector::getSplat(4, m_ir->getInt32(0)));
		return;
	}

	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateXor(ab[0], ab[1]));
}

void PPUTranslator::TDI(ppu_opcode_t op)
{
	UseCondition(CheckTrapCondition(op.bo, GetGpr(op.ra), m_ir->getInt64(op.simm16)));
	Trap(m_current_addr);
}

void PPUTranslator::TWI(ppu_opcode_t op)
{
	UseCondition(CheckTrapCondition(op.bo, GetGpr(op.ra, 32), m_ir->getInt32(op.simm16)));
	Trap(m_current_addr);
}

void PPUTranslator::MULLI(ppu_opcode_t op)
{
	SetGpr(op.rd, m_ir->CreateMul(GetGpr(op.ra), m_ir->getInt64(op.simm16)));
}

void PPUTranslator::SUBFIC(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto imm = m_ir->getInt64(op.simm16);
	const auto result = m_ir->CreateSub(imm, a);
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULE(result, imm));
	//SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", m_ir->CreateNot(a), imm, m_ir->getTrue()));
}

void PPUTranslator::CMPLI(ppu_opcode_t op)
{
	SetCrFieldUnsignedCmp(op.crfd, GetGpr(op.ra, op.l10 ? 64 : 32), op.l10 ? m_ir->getInt64(op.uimm16) : m_ir->getInt32(op.uimm16));
}

void PPUTranslator::CMPI(ppu_opcode_t op)
{
	SetCrFieldSignedCmp(op.crfd, GetGpr(op.ra, op.l10 ? 64 : 32), op.l10 ? m_ir->getInt64(op.simm16) : m_ir->getInt32(op.simm16));
}

void PPUTranslator::ADDIC(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto imm = m_ir->getInt64(op.simm16);
	const auto result = m_ir->CreateAdd(a, imm);
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULT(result, imm));
	//SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", a, imm, m_ir->getFalse()));
	if (op.main & 1) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::ADDI(ppu_opcode_t op)
{
	const auto imm = m_ir->getInt64(op.simm16);
	SetGpr(op.rd, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm);
}

void PPUTranslator::ADDIS(ppu_opcode_t op)
{
	const auto imm = m_ir->getInt64(op.simm16 << 16);
	SetGpr(op.rd, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm);
}

void PPUTranslator::BC(ppu_opcode_t op)
{
	const u64 target = ppu_branch_target(op.aa ? 0 : m_current_addr, op.simm16);

	const auto cond = CheckBranchCondition(op.bo, op.bi);
	
	if ((target > m_start_addr && target < m_end_addr) || (target == m_start_addr && !op.lk))
	{
		// Local branch

		if (op.lk)
		{
			CompilationError("BCL: local branch");
			Call(GetType<void>(), "__trace", m_ir->getInt64(m_current_addr));
			m_ir->CreateStore(m_ir->getInt64(m_current_addr + 4), m_reg_lr);
		}
		else if (cond)
		{
			m_ir->CreateCondBr(cond, GetBasicBlock(target), GetBasicBlock(m_current_addr + 4));
			return;
		}
		else
		{
			m_ir->CreateBr(GetBasicBlock(target));
			return;
		}
	}

	// External branch
	UseCondition(cond);
	CallFunction(target, !op.lk);
}

void PPUTranslator::HACK(ppu_opcode_t op)
{
	Call(GetType<void>(), "__hlecall", m_thread, m_ir->getInt32(op.opcode & 0x3ffffff));
	UndefineVolatileRegisters();
}

void PPUTranslator::SC(ppu_opcode_t op)
{
	Call(GetType<void>(), fmt::format(op.lev == 0 ? "__syscall" : "__lv%ucall", +op.lev), m_thread, m_ir->CreateLoad(m_gpr[11]));
	UndefineVolatileRegisters();
}

void PPUTranslator::B(ppu_opcode_t op)
{
	const u64 target = ppu_branch_target(op.aa ? 0 : m_current_addr, op.ll);

	if ((target > m_start_addr && target < m_end_addr) || (target == m_start_addr && !op.lk))
	{
		// Local branch

		if (op.lk)
		{
			CompilationError("BL: local branch");
			Call(GetType<void>(), "__trace", m_ir->getInt64(m_current_addr));
			m_ir->CreateStore(m_ir->getInt64(m_current_addr + 4), m_reg_lr);		
		}
		else
		{
			m_ir->CreateBr(GetBasicBlock(target));
			return;
		}
	}

	// External branch or recursive call
	CallFunction(target, !op.lk);
}

void PPUTranslator::MCRF(ppu_opcode_t op)
{
	const auto le = GetCrb(op.crfs * 4 + 0);
	const auto ge = GetCrb(op.crfs * 4 + 1);
	const auto eq = GetCrb(op.crfs * 4 + 2);
	const auto so = GetCrb(op.crfs * 4 + 3);
	SetCrField(op.crfd, le, ge, eq, so);
}

void PPUTranslator::BCLR(ppu_opcode_t op)
{
	UseCondition(CheckBranchCondition(op.bo, op.bi));

	if (op.lk)
	{
		// Sort of indirect call
		CallFunction(0, false, m_ir->CreateLoad(m_reg_lr));
	}
	else
	{
		// Simple return
		m_ir->CreateRetVoid();
	}
}

void PPUTranslator::CRNOR(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateNot(m_ir->CreateOr(GetCrb(op.crba), GetCrb(op.crbb))));
}

void PPUTranslator::CRANDC(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateAnd(GetCrb(op.crba), m_ir->CreateNot(GetCrb(op.crbb))));
}

void PPUTranslator::ISYNC(ppu_opcode_t op)
{
	m_ir->CreateFence(AtomicOrdering::SequentiallyConsistent);
}

void PPUTranslator::CRXOR(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateXor(GetCrb(op.crba), GetCrb(op.crbb)));
}

void PPUTranslator::DCBI(ppu_opcode_t op)
{
}

void PPUTranslator::CRNAND(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateNot(m_ir->CreateAnd(GetCrb(op.crba), GetCrb(op.crbb))));
}

void PPUTranslator::CRAND(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateAnd(GetCrb(op.crba), GetCrb(op.crbb)));
}

void PPUTranslator::CREQV(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateNot(m_ir->CreateXor(GetCrb(op.crba), GetCrb(op.crbb))));
}

void PPUTranslator::CRORC(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateOr(GetCrb(op.crba), m_ir->CreateNot(GetCrb(op.crbb))));
}

void PPUTranslator::CROR(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateOr(GetCrb(op.crba), GetCrb(op.crbb)));
}

void PPUTranslator::BCCTR(ppu_opcode_t op)
{
	UseCondition(CheckBranchCondition(op.bo | 0x4, op.bi));

	const auto jt_addr = m_current_addr + 4;
	const auto jt_data = m_bin + 1;

	// Detect a possible jumptable
	for (u64 i = 0, addr = jt_addr; addr < m_end_addr; i++, addr += sizeof(u32))
	{
		const u64 target = jt_addr + static_cast<s32>(jt_data[i]);

		// Check jumptable entry conditions
		if (target % 4 || target < m_start_addr || target >= m_end_addr)
		{
			if (i >= 2)
			{
				// Fix next instruction address
				m_current_addr = addr;

				if (!op.lk)
				{
					// Get sorted set of possible targets
					const std::set<s32> cases(jt_data, jt_data + i);

					// Create switch with special default case
					const auto _default = BasicBlock::Create(m_context, fmt::format("loc_%llx.def", m_current_addr/* - m_start_addr*/), m_function);
					const auto _switch = m_ir->CreateSwitch(m_ir->CreateLoad(m_reg_ctr), _default, ::size32(cases));

					for (const s32 offset : cases)
					{
						const u64 target = jt_addr + offset;
						_switch->addCase(m_ir->getInt64(target), GetBasicBlock(target));
					}

					m_ir->SetInsertPoint(_default);
					Trap(m_current_addr);
					return;
				}
				else
				{
					CompilationError("BCCTRL with a jt");
				}
			}

			break;
		}
	}

	if (!op.lk)
	{
		// Indirect branch
		m_ir->CreateBr(m_jtr);
	}
	else
	{
		// Indirect call
		CallFunction(0, false, m_ir->CreateLoad(m_reg_ctr));
	}
}

void PPUTranslator::RLWIMI(ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	Value* result;

	if (op.mb32 <= op.me32)
	{
		if (op.mb32 == 0 && op.me32 == 31)
		{
			result = RotateLeft(GetGpr(op.rs, 32), op.sh32);
		}
		else if (op.mb32 == 0 && op.sh32 == 31 - op.me32)
		{
			result = m_ir->CreateShl(GetGpr(op.rs, 32), op.sh32);
		}
		else if (op.me32 == 31 && op.sh32 == 32 - op.mb32)
		{
			result = m_ir->CreateLShr(GetGpr(op.rs, 32), 32 - op.sh32);
		}
		else if (op.mb32 == 0 && op.sh32 < 31 - op.me32)
		{
			// INSLWI and other possible mnemonics
			result = m_ir->CreateAnd(m_ir->CreateShl(GetGpr(op.rs, 32), op.sh32), mask);
		}
		else if (op.me32 == 31 && 32 - op.sh32 < op.mb32)
		{
			// INSRWI and other possible mnemonics
			result = m_ir->CreateAnd(m_ir->CreateLShr(GetGpr(op.rs, 32), 32 - op.sh32), mask);
		}
		else
		{
			// Generic op
			result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs, 32), op.sh32), mask);
		}

		// Extend 32-bit op result
		result = ZExt(result);
	}
	else
	{
		// Full 64-bit op with duplication
		result = m_ir->CreateAnd(RotateLeft(DuplicateExt(GetGpr(op.rs, 32)), op.sh32), mask);
	}

	if (mask != -1)
	{
		// Insertion
		result = m_ir->CreateOr(result, m_ir->CreateAnd(GetGpr(op.ra), ~mask));
	}

	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::RLWINM(ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	Value* result;

	if (op.mb32 <= op.me32)
	{
		if (op.mb32 == 0 && op.me32 == 31)
		{
			// ROTLWI, ROTRWI mnemonics
			result = RotateLeft(GetGpr(op.rs, 32), op.sh32);
		}
		else if (op.mb32 == 0 && op.sh32 == 31 - op.me32)
		{
			// SLWI mnemonic
			result = m_ir->CreateShl(GetGpr(op.rs, 32), op.sh32);
		}
		else if (op.me32 == 31 && op.sh32 == 32 - op.mb32)
		{
			// SRWI mnemonic
			result = m_ir->CreateLShr(GetGpr(op.rs, 32), 32 - op.sh32);
		}
		else if (op.mb32 == 0 && op.sh32 < 31 - op.me32)
		{
			// EXTLWI and other possible mnemonics
			result = m_ir->CreateAnd(m_ir->CreateShl(GetGpr(op.rs, 32), op.sh32), mask);
		}
		else if (op.me32 == 31 && 32 - op.sh32 < op.mb32)
		{
			// EXTRWI and other possible mnemonics
			result = m_ir->CreateAnd(m_ir->CreateLShr(GetGpr(op.rs, 32), 32 - op.sh32), mask);
		}
		else
		{
			// Generic op, including CLRLWI, CLRRWI mnemonics
			result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs, 32), op.sh32), mask);
		}

		// Extend 32-bit op result
		result = ZExt(result);
	}
	else
	{
		// Full 64-bit op with duplication
		result = m_ir->CreateAnd(RotateLeft(DuplicateExt(GetGpr(op.rs, 32)), op.sh32), mask);
	}

	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::RLWNM(ppu_opcode_t op)
{
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	Value* result;

	if (op.mb32 <= op.me32)
	{
		if (op.mb32 == 0 && op.me32 == 31)
		{
			// ROTLW mnemonic
			result = RotateLeft(GetGpr(op.rs, 32), GetGpr(op.rb, 32));
		}
		else
		{
			// Generic op
			result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs, 32), GetGpr(op.rb, 32)), mask);
		}
		
		// Extend 32-bit op result
		result = ZExt(result);
	}
	else
	{
		// Full 64-bit op with duplication
		result = m_ir->CreateAnd(RotateLeft(DuplicateExt(GetGpr(op.rs, 32)), GetGpr(op.rb)), mask);
	}

	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::ORI(ppu_opcode_t op)
{
	SetGpr(op.ra, m_ir->CreateOr(GetGpr(op.rs), op.uimm16));
}

void PPUTranslator::ORIS(ppu_opcode_t op)
{
	SetGpr(op.ra, m_ir->CreateOr(GetGpr(op.rs), op.uimm16 << 16));
}

void PPUTranslator::XORI(ppu_opcode_t op)
{
	SetGpr(op.ra, m_ir->CreateXor(GetGpr(op.rs), op.uimm16));
}

void PPUTranslator::XORIS(ppu_opcode_t op)
{
	SetGpr(op.ra, m_ir->CreateXor(GetGpr(op.rs), op.uimm16 << 16));
}

void PPUTranslator::ANDI(ppu_opcode_t op)
{
	const auto result = m_ir->CreateAnd(GetGpr(op.rs), op.uimm16);
	SetGpr(op.ra, result);
	SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::ANDIS(ppu_opcode_t op)
{
	const auto result = m_ir->CreateAnd(GetGpr(op.rs), op.uimm16 << 16);
	SetGpr(op.ra, result);
	SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::RLDICL(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;
	const u64 mask = ~0ull >> mb;
	Value* result;

	if (64 - sh < mb)
	{
		// EXTRDI and other possible mnemonics
		result = m_ir->CreateAnd(m_ir->CreateLShr(GetGpr(op.rs), 64 - sh), mask);
	}
	else if (64 - sh == mb)
	{
		// SRDI mnemonic
		result = m_ir->CreateLShr(GetGpr(op.rs), 64 - sh);
	}
	else
	{
		// Generic op, including CLRLDI mnemonic
		result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs), sh), mask);
	}
	
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::RLDICR(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 me = op.mbe64;
	const u64 mask = ~0ull << (63 - me);
	Value* result;

	if (sh < 63 - me)
	{
		// EXTLDI and other possible mnemonics
		result = m_ir->CreateAnd(m_ir->CreateShl(GetGpr(op.rs), sh), mask);
	}
	else if (sh == 63 - me)
	{
		// SLDI mnemonic
		result = m_ir->CreateShl(GetGpr(op.rs), sh);
	}
	else
	{
		// Generic op, including CLRRDI mnemonic
		result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs), sh), mask);
	}

	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::RLDIC(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;
	const u64 mask = ppu_rotate_mask(mb, 63 - sh);
	Value* result;

	if (mb == 0 && sh == 0)
	{
		result = GetGpr(op.rs);
	}
	else if (mb <= 63 - sh)
	{
		// CLRLSLDI and other possible mnemonics
		result = m_ir->CreateAnd(m_ir->CreateShl(GetGpr(op.rs), sh), mask);
	}
	else
	{
		// Generic op
		result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs), sh), mask);
	}

	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::RLDIMI(ppu_opcode_t op)
{
	const u32 sh = op.sh64;
	const u32 mb = op.mbe64;
	const u64 mask = ppu_rotate_mask(mb, 63 - sh);
	Value* result;

	if (mb == 0 && sh == 0)
	{
		result = GetGpr(op.rs);
	}
	else if (mb <= 63 - sh)
	{
		// INSRDI and other possible mnemonics
		result = m_ir->CreateAnd(m_ir->CreateShl(GetGpr(op.rs), sh), mask);
	}
	else
	{
		// Generic op
		result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs), sh), mask);
	}

	if (mask != -1)
	{
		// Insertion
		result = m_ir->CreateOr(result, m_ir->CreateAnd(GetGpr(op.ra), ~mask));
	}

	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::RLDCL(ppu_opcode_t op)
{
	const u32 mb = op.mbe64;
	const u64 mask = ~0ull >> mb;

	const auto result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs), GetGpr(op.rb)), mask);
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::RLDCR(ppu_opcode_t op)
{
	const u32 me = op.mbe64;
	const u64 mask = ~0ull << (63 - me);

	const auto result = m_ir->CreateAnd(RotateLeft(GetGpr(op.rs), GetGpr(op.rb)), mask);
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::CMP(ppu_opcode_t op)
{
	SetCrFieldSignedCmp(op.crfd, GetGpr(op.ra, op.l10 ? 64 : 32), GetGpr(op.rb, op.l10 ? 64 : 32));
}

void PPUTranslator::TW(ppu_opcode_t op)
{
	UseCondition(CheckTrapCondition(op.bo, GetGpr(op.ra, 32), GetGpr(op.rb, 32)));
	Trap(m_current_addr);
}

void PPUTranslator::LVSL(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<u8[16]>(), m_pure_attr, "__lvsl", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::LVEBX(ppu_opcode_t op)
{
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	const auto pos = m_ir->CreateXor(m_ir->CreateAnd(addr, 15), m_is_be ? 0 : 15);
	SetVr(op.vd, m_ir->CreateInsertElement(ConstantVector::getSplat(16, m_ir->getInt8(0)), ReadMemory(addr, GetType<u8>()), pos));
}

void PPUTranslator::SUBFC(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto result = m_ir->CreateSub(b, a);
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULE(result, b));
	//SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", m_ir->CreateNot(a), b, m_ir->getTrue()));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__subfc_get_ov", a, b));
}

void PPUTranslator::ADDC(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto result = m_ir->CreateAdd(a, b);
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULT(result, b));
	//SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", a, b, m_ir->getFalse()));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__addc_get_ov", a, b));
}

void PPUTranslator::MULHDU(ppu_opcode_t op)
{
	const auto a = ZExt(GetGpr(op.ra));
	const auto b = ZExt(GetGpr(op.rb));
	const auto result = Trunc(m_ir->CreateLShr(m_ir->CreateMul(a, b), 64));
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::MULHWU(ppu_opcode_t op)
{
	const auto a = ZExt(GetGpr(op.ra, 32));
	const auto b = ZExt(GetGpr(op.rb, 32));
	SetGpr(op.rd, m_ir->CreateLShr(m_ir->CreateMul(a, b), 32));
	if (op.rc) SetCrField(0, GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>());
}

void PPUTranslator::MFOCRF(ppu_opcode_t op)
{
	if (op.l11)
	{
		// MFOCRF

		const u64 pos = countLeadingZeros<u32>(op.crm, ZB_Width) - 24;

		if (pos >= 8 || 0x80 >> pos != op.crm)
		{
			CompilationError("MFOCRF: Undefined behaviour");
			SetGpr(op.rd, UndefValue::get(GetType<u64>()));
			return;
		}
	}
	else
	{
		// MFCR
	}

	Value* result{};
	for (u32 i = 0; i < 8; i++)
	{
		if (!op.l11 || op.crm & (128 >> i))
		{
			for (u32 b = i * 4; b < i * 4 + 4; b++)
			{
				const auto value = m_ir->CreateShl(ZExt(GetCrb(b), GetType<u64>()), 31 - b);
				result = result ? m_ir->CreateOr(result, value) : value;
			}
		}
	}

	SetGpr(op.rd, result);
}

void PPUTranslator::LWARX(ppu_opcode_t op)
{
	SetGpr(op.rd, Call(GetType<u32>(), "__lwarx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::LDX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u64>()));
}

void PPUTranslator::LWZX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u32>()));
}

void PPUTranslator::SLW(ppu_opcode_t op)
{
	const auto shift_num = m_ir->CreateAnd(GetGpr(op.rb), 0x3f);
	const auto shift_res = m_ir->CreateShl(GetGpr(op.rs), shift_num);
	const auto result = m_ir->CreateAnd(shift_res, 0xffffffff);
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::CNTLZW(ppu_opcode_t op)
{
	const auto result = Call(GetType<u32>(), "llvm.ctlz.i32", GetGpr(op.rs, 32), m_ir->getFalse());
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt32(0));
}

void PPUTranslator::SLD(ppu_opcode_t op)
{
	const auto shift_num = m_ir->CreateAnd(GetGpr(op.rb), 0x7f);
	const auto shift_arg = GetGpr(op.rs);
	const auto result = Trunc(m_ir->CreateShl(ZExt(shift_arg), ZExt(shift_num)));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::AND(ppu_opcode_t op)
{
	const auto result = op.rs == op.rb ? GetGpr(op.rs) : m_ir->CreateAnd(GetGpr(op.rs), GetGpr(op.rb));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::CMPL(ppu_opcode_t op)
{
	SetCrFieldUnsignedCmp(op.crfd, GetGpr(op.ra, op.l10 ? 64 : 32), GetGpr(op.rb, op.l10 ? 64 : 32));
}

void PPUTranslator::LVSR(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<u8[16]>(), m_pure_attr, "__lvsr", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::LVEHX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), -2);
	const auto pos = m_ir->CreateLShr(m_ir->CreateXor(m_ir->CreateAnd(addr, 15), m_is_be ? 0 : 15), 1);
	SetVr(op.vd, m_ir->CreateInsertElement(ConstantVector::getSplat(8, m_ir->getInt16(0)), ReadMemory(addr, GetType<u16>(), true, 2), pos));
}

void PPUTranslator::SUBF(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto result = m_ir->CreateSub(b, a);
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__subf_get_ov", a, b));
}

void PPUTranslator::LDUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetGpr(op.rd, ReadMemory(addr, GetType<u64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::DCBST(ppu_opcode_t op)
{
}

void PPUTranslator::LWZUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetGpr(op.rd, ReadMemory(addr, GetType<u32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::CNTLZD(ppu_opcode_t op)
{
	const auto result = Call(GetType<u64>(), "llvm.ctlz.i64", GetGpr(op.rs), m_ir->getFalse());
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::ANDC(ppu_opcode_t op)
{
	const auto result = m_ir->CreateAnd(GetGpr(op.rs), m_ir->CreateNot(GetGpr(op.rb)));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::TD(ppu_opcode_t op)
{
	UseCondition(CheckTrapCondition(op.bo, GetGpr(op.ra), GetGpr(op.rb)));
	Trap(m_current_addr);
}

void PPUTranslator::LVEWX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), -4);
	const auto pos = m_ir->CreateLShr(m_ir->CreateXor(m_ir->CreateAnd(addr, 15), m_is_be ? 0 : 15), 2);
	SetVr(op.vd, m_ir->CreateInsertElement(ConstantVector::getSplat(4, m_ir->getInt32(0)), ReadMemory(addr, GetType<u32>(), true, 4), pos));
}

void PPUTranslator::MULHD(ppu_opcode_t op)
{
	const auto a = SExt(GetGpr(op.ra)); // i128
	const auto b = SExt(GetGpr(op.rb));
	const auto result = Trunc(m_ir->CreateLShr(m_ir->CreateMul(a, b), 64));
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::MULHW(ppu_opcode_t op)
{
	const auto a = SExt(GetGpr(op.ra, 32));
	const auto b = SExt(GetGpr(op.rb, 32));
	SetGpr(op.rd, m_ir->CreateLShr(m_ir->CreateMul(a, b), 32));
	if (op.rc) SetCrField(0, GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>());
}

void PPUTranslator::LDARX(ppu_opcode_t op)
{
	SetGpr(op.rd, Call(GetType<u64>(), "__ldarx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::DCBF(ppu_opcode_t op)
{
}

void PPUTranslator::LBZX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u8>()));
}

void PPUTranslator::LVX(ppu_opcode_t op)
{
	const auto data = ReadMemory(m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), -16), GetType<u8[16]>(), m_is_be, 16);
	SetVr(op.vd, m_is_be ? data : Shuffle(data, nullptr, { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 }));
}

void PPUTranslator::NEG(ppu_opcode_t op)
{
	const auto reg = GetGpr(op.ra);
	const auto result = m_ir->CreateNeg(reg);
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__neg_get_ov", reg));
}

void PPUTranslator::LBZUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetGpr(op.rd, ReadMemory(addr, GetType<u8>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::NOR(ppu_opcode_t op)
{
	const auto result = m_ir->CreateNot(op.rs == op.rb ? GetGpr(op.rs) : m_ir->CreateOr(GetGpr(op.rs), GetGpr(op.rb)));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::STVEBX(ppu_opcode_t op)
{
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	WriteMemory(addr, m_ir->CreateExtractElement(GetVr(op.vs, VrType::vi8), m_ir->CreateXor(m_ir->CreateAnd(addr, 15), m_is_be ? 0 : 15)));
}

void PPUTranslator::SUBFE(ppu_opcode_t op)
{
	const auto a = m_ir->CreateNot(GetGpr(op.ra));
	const auto b = GetGpr(op.rb);
	const auto c = GetCarry();
	const auto result = m_ir->CreateAdd(m_ir->CreateAdd(a, b), ZExt(c, GetType<u64>()));
	SetGpr(op.rd, result);
	SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", a, b, c)); // TODO
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__subfe_get_ov", a, b, c));
}

void PPUTranslator::ADDE(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto c = GetCarry();
	const auto result = m_ir->CreateAdd(m_ir->CreateAdd(a, b), ZExt(c, GetType<u64>()));
	SetGpr(op.rd, result);
	SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", a, b, c)); // TODO
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__adde_get_ov", a, b, c));
}

void PPUTranslator::MTOCRF(ppu_opcode_t op)
{
	if (op.l11)
	{
		// MTOCRF
		const u64 pos = countLeadingZeros<u32>(op.crm, ZB_Width) - 24;

		if (pos >= 8 || 128 >> pos != op.crm)
		{
			CompilationError("MTOCRF: Undefined behaviour");
			return;
		}
	}
	else
	{
		// MTCRF
	}

	const auto value = GetGpr(op.rs);

	for (u32 i = 0; i < 8; i++)
	{
		if (op.crm & (128 >> i))
		{
			for (u32 bit = i * 4; bit < i * 4 + 4; bit++)
			{
				SetCrb(bit, Trunc(m_ir->CreateLShr(value, 31 - bit), GetType<bool>()));
			}
		}
	}
}

void PPUTranslator::STDX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs));
}

void PPUTranslator::STWCX(ppu_opcode_t op)
{
	const auto bit = Call(GetType<bool>(), "__stwcx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs, 32));
	SetCrField(0, m_ir->getFalse(), m_ir->getFalse(), bit);
}

void PPUTranslator::STWX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs, 32));
}

void PPUTranslator::STVEHX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), -2);
	WriteMemory(addr, m_ir->CreateExtractElement(GetVr(op.vs, VrType::vi16), m_ir->CreateLShr(m_ir->CreateXor(m_ir->CreateAnd(addr, 15), m_is_be ? 0 : 15), 1)), true, 2);
}

void PPUTranslator::STDUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	WriteMemory(addr, GetGpr(op.rs));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STWUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	WriteMemory(addr, GetGpr(op.rs, 32));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STVEWX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), -4);
	WriteMemory(addr, m_ir->CreateExtractElement(GetVr(op.vs, VrType::vi32), m_ir->CreateLShr(m_ir->CreateXor(m_ir->CreateAnd(addr, 15), m_is_be ? 0 : 15), 2)), true, 4);
}

void PPUTranslator::ADDZE(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto c = GetCarry();
	const auto result = m_ir->CreateAdd(a, ZExt(c, GetType<u64>()));
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULT(result, a));
	//SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", a, m_ir->getInt64(0), c));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__addze_get_ov", a, c));
}

void PPUTranslator::SUBFZE(ppu_opcode_t op)
{
	const auto a = m_ir->CreateNot(GetGpr(op.ra));
	const auto c = GetCarry();
	const auto result = m_ir->CreateAdd(a, ZExt(c, GetType<u64>()));
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULT(result, a));
	//SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", a, m_ir->getInt64(0), c));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__subfze_get_ov", a, c));
}

void PPUTranslator::STDCX(ppu_opcode_t op)
{
	const auto bit = Call(GetType<bool>(), "__stdcx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs));
	SetCrField(0, m_ir->getFalse(), m_ir->getFalse(), bit);
}

void PPUTranslator::STBX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs, 8));
}

void PPUTranslator::STVX(ppu_opcode_t op)
{
	const auto value = GetVr(op.vs, VrType::vi8);
	const auto data = m_is_be ? value : Shuffle(value, nullptr, { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 });
	WriteMemory(m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), -16), data, m_is_be, 16);
}

void PPUTranslator::SUBFME(ppu_opcode_t op)
{
	const auto a = m_ir->CreateNot(GetGpr(op.ra));
	const auto c = GetCarry();
	const auto result = m_ir->CreateSub(a, ZExt(m_ir->CreateNot(c), GetType<u64>()));
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateOr(c, IsNotZero(a)));
	//SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", a, m_ir->getInt64(-1), c));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__subfme_get_ov", a, c));
}

void PPUTranslator::MULLD(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto result = m_ir->CreateMul(a, b);
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__mulld_get_ov", a, b));
}

void PPUTranslator::ADDME(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto c = GetCarry();
	const auto result = m_ir->CreateSub(a, ZExt(m_ir->CreateNot(c), GetType<u64>()));
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateOr(c, IsNotZero(a)));
	//SetCarry(Call(GetType<bool>(), m_pure_attr, "__adde_get_ca", a, m_ir->getInt64(-1), c));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__addme_get_ov", a, c));
}

void PPUTranslator::MULLW(ppu_opcode_t op)
{
	const auto a = SExt(GetGpr(op.ra, 32));
	const auto b = SExt(GetGpr(op.rb, 32));
	const auto result = m_ir->CreateMul(a, b);
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__mullw_get_ov", a, b));
}

void PPUTranslator::DCBTST(ppu_opcode_t op)
{
}

void PPUTranslator::STBUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	WriteMemory(addr, GetGpr(op.rs, 8));
	SetGpr(op.ra, addr);
}

void PPUTranslator::ADD(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto result = m_ir->CreateAdd(a, b);
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__add_get_ov", a, b));
}

void PPUTranslator::DCBT(ppu_opcode_t op)
{
}

void PPUTranslator::LHZX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u16>()));
}

void PPUTranslator::EQV(ppu_opcode_t op)
{
	const auto result = m_ir->CreateNot(m_ir->CreateXor(GetGpr(op.rs), GetGpr(op.rb)));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::ECIWX(ppu_opcode_t op)
{
	SetGpr(op.rd, Call(GetType<u64>(), "__eciwx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::LHZUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetGpr(op.rd, ReadMemory(addr, GetType<u16>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::XOR(ppu_opcode_t op)
{
	const auto result = op.rs == op.rb ? (Value*)m_ir->getInt64(0) : m_ir->CreateXor(GetGpr(op.rs), GetGpr(op.rb));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::MFSPR(ppu_opcode_t op)
{
	Value* result;
	switch (const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5))
	{
	case 0x001: // MFXER
		result = ZExt(m_ir->CreateLoad(m_xer_count), GetType<u64>());
		result = m_ir->CreateOr(result, m_ir->CreateShl(ZExt(m_ir->CreateLoad(m_xer_so), GetType<u64>()), 29));
		result = m_ir->CreateOr(result, m_ir->CreateShl(ZExt(m_ir->CreateLoad(m_xer_ov), GetType<u64>()), 30));
		result = m_ir->CreateOr(result, m_ir->CreateShl(ZExt(m_ir->CreateLoad(m_xer_ca), GetType<u64>()), 31));
		break;
	case 0x008: // MFLR
		result = m_ir->CreateLoad(m_reg_lr);
		break;
	case 0x009: // MFCTR
		result = m_ir->CreateLoad(m_reg_ctr);
		break;
	case 0x100:
		result = ZExt(m_ir->CreateLoad(m_reg_vrsave));
		break;
	case 0x10C: // MFTB
		result = ZExt(Call(GetType<u32>(), m_pure_attr, "__get_tbl"));
		break;
	case 0x10D: // MFTBU
		result = ZExt(Call(GetType<u32>(), m_pure_attr, "__get_tbh"));
		break;
	default:
		result = Call(GetType<u64>(), fmt::format("__mfspr_%u", n));
		break;
	}

	SetGpr(op.rd, result);
}

void PPUTranslator::LWAX(ppu_opcode_t op)
{
	SetGpr(op.rd, SExt(ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<s32>())));
}

void PPUTranslator::DST(ppu_opcode_t op)
{
}

void PPUTranslator::LHAX(ppu_opcode_t op)
{
	SetGpr(op.rd, SExt(ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<s16>()), GetType<s64>()));
}

void PPUTranslator::LVXL(ppu_opcode_t op)
{
	return LVX(op);
}

void PPUTranslator::MFTB(ppu_opcode_t op)
{
	return MFSPR(op);
}

void PPUTranslator::LWAUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetGpr(op.rd, SExt(ReadMemory(addr, GetType<s32>())));
	SetGpr(op.ra, addr);
}

void PPUTranslator::DSTST(ppu_opcode_t op)
{
}

void PPUTranslator::LHAUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetGpr(op.rd, SExt(ReadMemory(addr, GetType<s16>()), GetType<s64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STHX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs, 16));
}

void PPUTranslator::ORC(ppu_opcode_t op)
{
	const auto result = op.rs == op.rb ? (Value*)m_ir->getInt64(-1) : m_ir->CreateOr(GetGpr(op.rs), m_ir->CreateNot(GetGpr(op.rb)));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::ECOWX(ppu_opcode_t op)
{
	Call(GetType<void>(), "__ecowx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs, 32));
}

void PPUTranslator::STHUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	WriteMemory(addr, GetGpr(op.rs, 16));
	SetGpr(op.ra, addr);
}

void PPUTranslator::OR(ppu_opcode_t op)
{
	const auto result = op.rs == op.rb ? GetGpr(op.rs) : m_ir->CreateOr(GetGpr(op.rs), GetGpr(op.rb));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::DIVDU(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto o = IsZero(b);
	const auto result = m_ir->CreateUDiv(a, m_ir->CreateSelect(o, m_ir->getInt64(-1), b));
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::DIVWU(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra, 32);
	const auto b = GetGpr(op.rb, 32);
	const auto o = IsZero(b);
	SetGpr(op.rd, m_ir->CreateUDiv(a, m_ir->CreateSelect(o, m_ir->getInt32(0xffffffff), b)));
	if (op.rc) SetCrField(0, GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>());
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::MTSPR(ppu_opcode_t op)
{
	const auto value = GetGpr(op.rs);

	switch (const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5))
	{
	case 0x001: // MTXER
		m_ir->CreateStore(Trunc(m_ir->CreateLShr(value, 31), GetType<bool>()), m_xer_ca);
		m_ir->CreateStore(Trunc(m_ir->CreateLShr(value, 30), GetType<bool>()), m_xer_ov);
		m_ir->CreateStore(Trunc(m_ir->CreateLShr(value, 29), GetType<bool>()), m_xer_so);
		m_ir->CreateStore(Trunc(value, GetType<u8>()), m_xer_count);
		break;
	case 0x008: // MTLR
		m_ir->CreateStore(value, m_reg_lr);
		break;
	case 0x009: // MTCTR
		m_ir->CreateStore(value, m_reg_ctr);
		break;
	case 0x100:
		m_ir->CreateStore(Trunc(value), m_reg_vrsave);
		break;
	default:
		Call(GetType<void>(), fmt::format("__mtspr_%u", n), value);
		break;
	}
}

void PPUTranslator::NAND(ppu_opcode_t op)
{
	const auto result = m_ir->CreateNot(op.rs == op.rb ? GetGpr(op.rs) : m_ir->CreateAnd(GetGpr(op.rs), GetGpr(op.rb)));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::STVXL(ppu_opcode_t op)
{
	return STVX(op);
}

void PPUTranslator::DIVD(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto o = m_ir->CreateOr(IsZero(b), m_ir->CreateAnd(m_ir->CreateICmpEQ(a, m_ir->getInt64(1ull << 63)), IsOnes(b)));
	const auto result = m_ir->CreateSDiv(a, m_ir->CreateSelect(o, m_ir->getInt64(1ull << 63), b));
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::DIVW(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra, 32);
	const auto b = GetGpr(op.rb, 32);
	const auto o = m_ir->CreateOr(IsZero(b), m_ir->CreateAnd(m_ir->CreateICmpEQ(a, m_ir->getInt32(1 << 31)), IsOnes(b)));
	SetGpr(op.rd, m_ir->CreateSDiv(a, m_ir->CreateSelect(o, m_ir->getInt32(1 << 31), b)));
	if (op.rc) SetCrField(0, GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>());
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::LVLX(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<u8[16]>(), "__lvlx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::LDBRX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u64>(), false));
}

void PPUTranslator::LSWX(ppu_opcode_t op)
{
	Call(GetType<void>(), "__lswx", m_ir->getInt32(op.rd), m_ir->CreateLoad(m_xer_count), op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb));
}

void PPUTranslator::LWBRX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u32>(), false));
}

void PPUTranslator::LFSX(ppu_opcode_t op)
{
	SetFpr(op.frd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<f32>()));
}

void PPUTranslator::SRW(ppu_opcode_t op)
{
	const auto shift_num = m_ir->CreateAnd(GetGpr(op.rb), 0x3f);
	const auto shift_arg = m_ir->CreateAnd(GetGpr(op.rs), 0xffffffff);
	const auto result = m_ir->CreateLShr(shift_arg, shift_num);
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::SRD(ppu_opcode_t op)
{
	const auto shift_num = m_ir->CreateAnd(GetGpr(op.rb), 0x7f);
	const auto shift_arg = GetGpr(op.rs);
	const auto result = Trunc(m_ir->CreateLShr(ZExt(shift_arg), ZExt(shift_num)));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::LVRX(ppu_opcode_t op)
{
	SetVr(op.vd, Call(GetType<u8[16]>(), "__lvrx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::LSWI(ppu_opcode_t op)
{
	Call(GetType<void>(), "__lswi", m_ir->getInt32(op.rd), m_ir->getInt32(op.rb), op.ra ? GetGpr(op.ra) : m_ir->getInt64(0));
}

void PPUTranslator::LFSUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetFpr(op.frd, ReadMemory(addr, GetType<f32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::SYNC(ppu_opcode_t op)
{
	m_ir->CreateFence(AtomicOrdering::SequentiallyConsistent);
}

void PPUTranslator::LFDX(ppu_opcode_t op)
{
	SetFpr(op.frd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<f64>()));
}

void PPUTranslator::LFDUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetFpr(op.frd, ReadMemory(addr, GetType<f64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STVLX(ppu_opcode_t op)
{
	Call(GetType<void>(), "__stvlx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetVr(op.vs, VrType::vi8));
}

void PPUTranslator::STDBRX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs), false);
}

void PPUTranslator::STSWX(ppu_opcode_t op)
{
	Call(GetType<void>(), "__stswx", m_ir->getInt32(op.rs), m_ir->CreateLoad(m_xer_count), op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb));
}

void PPUTranslator::STWBRX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs, 32), false);
}

void PPUTranslator::STFSX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetFpr(op.frs, 32));
}

void PPUTranslator::STVRX(ppu_opcode_t op)
{
	Call(GetType<void>(), "__stvrx", op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetVr(op.vs, VrType::vi8));
}

void PPUTranslator::STFSUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	WriteMemory(addr, GetFpr(op.frs, 32));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STSWI(ppu_opcode_t op)
{
	Call(GetType<void>(), "__stswi", m_ir->getInt32(op.rd), m_ir->getInt32(op.rb), op.ra ? GetGpr(op.ra) : m_ir->getInt64(0));
}

void PPUTranslator::STFDX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetFpr(op.frs));
}

void PPUTranslator::STFDUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	WriteMemory(addr, GetFpr(op.frs));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LVLXL(ppu_opcode_t op)
{
	return LVLX(op);
}

void PPUTranslator::LHBRX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u16>(), false));
}

void PPUTranslator::SRAW(ppu_opcode_t op)
{
	const auto shift_num = m_ir->CreateAnd(GetGpr(op.rb), 0x3f);
	const auto shift_arg = GetGpr(op.rs, 32);
	const auto arg_ext = SExt(shift_arg);
	const auto result = m_ir->CreateAShr(arg_ext, shift_num);
	SetGpr(op.ra, result);
	SetCarry(m_ir->CreateAnd(m_ir->CreateICmpSLT(shift_arg, m_ir->getInt32(0)), m_ir->CreateICmpNE(arg_ext, m_ir->CreateShl(result, shift_num))));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::SRAD(ppu_opcode_t op)
{
	const auto shift_num = ZExt(m_ir->CreateAnd(GetGpr(op.rb), 0x7f)); // i128
	const auto shift_arg = GetGpr(op.rs);
	const auto arg_ext = SExt(shift_arg); // i128
	const auto res_128 = m_ir->CreateAShr(arg_ext, shift_num); // i128
	const auto result = Trunc(res_128);
	SetGpr(op.ra, result);
	SetCarry(m_ir->CreateAnd(m_ir->CreateICmpSLT(shift_arg, m_ir->getInt64(0)), m_ir->CreateICmpNE(arg_ext, m_ir->CreateShl(result, shift_num))));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::LVRXL(ppu_opcode_t op)
{
	return LVRX(op);
}

void PPUTranslator::DSS(ppu_opcode_t op)
{
}

void PPUTranslator::SRAWI(ppu_opcode_t op)
{
	const auto shift_arg = GetGpr(op.rs, 32);
	const auto res_32 = m_ir->CreateAShr(shift_arg, op.sh32);
	const auto result = SExt(res_32);
	SetGpr(op.ra, result);
	SetCarry(m_ir->CreateAnd(m_ir->CreateICmpSLT(shift_arg, m_ir->getInt32(0)), m_ir->CreateICmpNE(shift_arg, m_ir->CreateShl(res_32, op.sh32))));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::SRADI(ppu_opcode_t op)
{
	const auto shift_arg = GetGpr(op.rs);
	const auto result = m_ir->CreateAShr(shift_arg, op.sh64);
	SetGpr(op.ra, result);
	SetCarry(m_ir->CreateAnd(m_ir->CreateICmpSLT(shift_arg, m_ir->getInt64(0)), m_ir->CreateICmpNE(shift_arg, m_ir->CreateShl(result, op.sh64))));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::EIEIO(ppu_opcode_t op)
{
	// TODO
	m_ir->CreateFence(AtomicOrdering::SequentiallyConsistent);
}

void PPUTranslator::STVLXL(ppu_opcode_t op)
{
	return STVLX(op);
}

void PPUTranslator::STHBRX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs, 16), false);
}

void PPUTranslator::EXTSH(ppu_opcode_t op)
{
	const auto result = SExt(GetGpr(op.rs, 16), GetType<s64>());
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::STVRXL(ppu_opcode_t op)
{
	return STVRX(op);
}

void PPUTranslator::EXTSB(ppu_opcode_t op)
{
	const auto result = SExt(GetGpr(op.rs, 8), GetType<s64>());
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::STFIWX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetFpr(op.frs, 32, true));
}

void PPUTranslator::EXTSW(ppu_opcode_t op)
{
	const auto result = SExt(GetGpr(op.rs, 32));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::ICBI(ppu_opcode_t op)
{
}

void PPUTranslator::DCBZ(ppu_opcode_t op)
{
	const auto ptr = GetMemory(m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), -128), GetType<u8>());
	Call(GetType<void>(), "llvm.memset.p0i8.i32", ptr, m_ir->getInt8(0), m_ir->getInt32(128), m_ir->getInt32(16), m_ir->getTrue());
}

void PPUTranslator::LWZ(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetType<u32>()));
}

void PPUTranslator::LWZU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	SetGpr(op.rd, ReadMemory(addr, GetType<u32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LBZ(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetType<u8>()));
}

void PPUTranslator::LBZU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	SetGpr(op.rd, ReadMemory(addr, GetType<u8>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STW(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetGpr(op.rs, 32));
}

void PPUTranslator::STWU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	WriteMemory(addr, GetGpr(op.rs, 32));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STB(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetGpr(op.rs, 8));
}

void PPUTranslator::STBU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	WriteMemory(addr, GetGpr(op.rs, 8));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LHZ(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetType<u16>()));
}

void PPUTranslator::LHZU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	SetGpr(op.rd, ReadMemory(addr, GetType<u16>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LHA(ppu_opcode_t op)
{
	SetGpr(op.rd, SExt(ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetType<s16>()), GetType<s64>()));
}

void PPUTranslator::LHAU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	SetGpr(op.rd, SExt(ReadMemory(addr, GetType<s16>()), GetType<s64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STH(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetGpr(op.rs, 16));
}

void PPUTranslator::STHU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	WriteMemory(addr, GetGpr(op.rs, 16));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LMW(ppu_opcode_t op)
{
	Call(GetType<void>(), "__trace", m_ir->getInt64(m_current_addr));
	for (u32 i = 0; i < 32 - op.rd; i++)
	{
		SetGpr(i + op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(m_ir->getInt64(op.simm16 + i * 4), GetGpr(op.ra)) : m_ir->getInt64(op.simm16 + i * 4), GetType<u32>()));
	}
}

void PPUTranslator::STMW(ppu_opcode_t op)
{
	Call(GetType<void>(), "__trace", m_ir->getInt64(m_current_addr));
	for (u32 i = 0; i < 32 - op.rs; i++)
	{
		WriteMemory(op.ra ? m_ir->CreateAdd(m_ir->getInt64(op.simm16 + i * 4), GetGpr(op.ra)) : m_ir->getInt64(op.simm16 + i * 4), GetGpr(i + op.rs, 32));
	}
}

void PPUTranslator::LFS(ppu_opcode_t op)
{
	SetFpr(op.frd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetType<f32>()));
}

void PPUTranslator::LFSU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	SetFpr(op.frd, ReadMemory(addr, GetType<f32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LFD(ppu_opcode_t op)
{
	SetFpr(op.frd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetType<f64>()));
}

void PPUTranslator::LFDU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	SetFpr(op.frd, ReadMemory(addr, GetType<f64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STFS(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetFpr(op.frs, 32));
}

void PPUTranslator::STFSU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	WriteMemory(addr, GetFpr(op.frs, 32));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STFD(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16)) : m_ir->getInt64(op.simm16), GetFpr(op.frs));
}

void PPUTranslator::STFDU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.simm16));
	WriteMemory(addr, GetFpr(op.frs));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LD(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.ds << 2)) : m_ir->getInt64(op.ds << 2), GetType<u64>()));
}

void PPUTranslator::LDU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.ds << 2));
	SetGpr(op.rd, ReadMemory(addr, GetType<u64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LWA(ppu_opcode_t op)
{
	SetGpr(op.rd, SExt(ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.ds << 2)) : m_ir->getInt64(op.ds << 2), GetType<s32>())));
}

void PPUTranslator::STD(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.ds << 2)) : m_ir->getInt64(op.ds << 2), GetGpr(op.rs));
}

void PPUTranslator::STDU(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), m_ir->getInt64(op.ds << 2));
	WriteMemory(addr, GetGpr(op.rs));
	SetGpr(op.ra, addr);
}

void PPUTranslator::FDIVS(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFDiv(a, b), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fdivs_get_fr", a, b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fdivs_get_fi", a, b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fdivs_get_ox", a, b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fdivs_get_ux", a, b));
	//SetFPSCRException(m_fpscr_zx, Call(GetType<bool>(), m_pure_attr, "__fdivs_get_zx", a, b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fdivs_get_vxsnan", a, b));
	//SetFPSCRException(m_fpscr_vxidi, Call(GetType<bool>(), m_pure_attr, "__fdivs_get_vxidi", a, b));
	//SetFPSCRException(m_fpscr_vxzdz, Call(GetType<bool>(), m_pure_attr, "__fdivs_get_vxzdz", a, b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FSUBS(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFSub(a, b), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fsubs_get_fr", a, b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fsubs_get_fi", a, b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fsubs_get_ox", a, b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fsubs_get_ux", a, b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fsubs_get_vxsnan", a, b));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fsubs_get_vxisi", a, b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FADDS(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFAdd(a, b), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fadds_get_fr", a, b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fadds_get_fi", a, b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fadds_get_ox", a, b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fadds_get_ux", a, b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fadds_get_vxsnan", a, b));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fadds_get_vxisi", a, b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FSQRTS(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	const auto result = m_ir->CreateFPTrunc(Call(GetType<f64>(), "llvm.sqrt.f64", b), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fsqrts_get_fr", b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fsqrts_get_fi", b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fsqrts_get_ox", b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fsqrts_get_ux", b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fsqrts_get_vxsnan", b));
	//SetFPSCRException(m_fpscr_vxsqrt, Call(GetType<bool>(), m_pure_attr, "__fsqrts_get_vxsqrt", b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FRES(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb, 32);
	const auto result = Call(GetType<f32>(), m_pure_attr, "__fre", b);
	SetFpr(op.frd, result);

	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_fr);
	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_fi);
	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_xx);
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fres_get_ox", b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fres_get_ux", b));
	//SetFPSCRException(m_fpscr_zx, Call(GetType<bool>(), m_pure_attr, "__fres_get_zx", b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fres_get_vxsnan", b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FMULS(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFMul(a, c), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmuls_get_fr", a, c));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmuls_get_fi", a, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmuls_get_ox", a, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmuls_get_ux", a, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmuls_get_vxsnan", a, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmuls_get_vximz", a, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FMADDS(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFAdd(m_ir->CreateFMul(a, c), b), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmadds_get_fr", a, b, c));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmadds_get_fi", a, b, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_ox", a, b, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_ux", a, b, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vxsnan", a, b, c));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vxisi", a, b, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vximz", a, b, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FMSUBS(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFSub(m_ir->CreateFMul(a, c), b), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmadds_get_fr", a, b, c)); // TODO ???
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmadds_get_fi", a, b, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_ox", a, b, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_ux", a, b, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vxsnan", a, b, c));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vxisi", a, b, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vximz", a, b, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FNMSUBS(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFSub(b, m_ir->CreateFMul(a, c)), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmadds_get_fr", a, b, c)); // TODO ???
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmadds_get_fi", a, b, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_ox", a, b, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_ux", a, b, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vxsnan", a, b, c));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vxisi", a, b, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vximz", a, b, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FNMADDS(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFNeg(m_ir->CreateFAdd(m_ir->CreateFMul(a, c), b)), GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmadds_get_fr", a, b, c)); // TODO ???
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmadds_get_fi", a, b, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_ox", a, b, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_ux", a, b, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vxsnan", a, b, c));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vxisi", a, b, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmadds_get_vximz", a, b, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::MTFSB1(ppu_opcode_t op)
{
	CompilationError("MTFSB1");

	SetFPSCRBit(op.crbd, m_ir->getTrue(), true);

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::MCRFS(ppu_opcode_t op)
{
	CompilationError("MCRFS");

	const auto lt = GetFPSCRBit(op.crfs * 4 + 0);
	const auto gt = GetFPSCRBit(op.crfs * 4 + 1);
	const auto eq = GetFPSCRBit(op.crfs * 4 + 2);
	const auto un = GetFPSCRBit(op.crfs * 4 + 3);
	SetCrField(op.crfd, lt, gt, eq, un);
}

void PPUTranslator::MTFSB0(ppu_opcode_t op)
{
	CompilationError("MTFSB0");

	SetFPSCRBit(op.crbd, m_ir->getFalse(), false);

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::MTFSFI(ppu_opcode_t op)
{
	CompilationError("MTFSFI");

	SetFPSCRBit(op.crfd * 4 + 0, m_ir->getInt1((op.i & 8) != 0), false);
	if (op.crfd != 0) SetFPSCRBit(op.crfd * 4 + 1, m_ir->getInt1((op.i & 4) != 0), false);
	if (op.crfd != 0) SetFPSCRBit(op.crfd * 4 + 2, m_ir->getInt1((op.i & 2) != 0), false);
	SetFPSCRBit(op.crfd * 4 + 3, m_ir->getInt1((op.i & 1) != 0), false);

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::MFFS(ppu_opcode_t op)
{
	CompilationError("MFFS");

	Value* result = m_ir->getInt64(0);

	for (u32 i = 0; i < 32; i++)
	{
		if (const auto bit = m_fpscr[i] ? m_ir->CreateLoad(m_fpscr[i]) : GetFPSCRBit(i))
		{
			result = m_ir->CreateOr(result, m_ir->CreateShl(ZExt(bit, GetType<u64>()), i ^ 31));
		}
	}

	SetFpr(op.frd, result);

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::MTFSF(ppu_opcode_t op)
{
	CompilationError("MTFSF");

	const auto value = GetFpr(op.frb, 32, true);

	for (u32 i = 0; i < 32; i++)
	{
		if (i != 1 && i != 2 && (op.flm & (128 >> (i / 4))) != 0)
		{
			SetFPSCRBit(i, Trunc(m_ir->CreateLShr(value, i ^ 31), GetType<bool>()), false);
		}
	}

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::FCMPU(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto lt = m_ir->CreateFCmpOLT(a, b);
	const auto gt = m_ir->CreateFCmpOGT(a, b);
	const auto eq = m_ir->CreateFCmpOEQ(a, b);
	const auto un = m_ir->CreateFCmpUNO(a, b);
	SetCrField(op.crfd, lt, gt, eq, un);
	SetFPCC(lt, gt, eq, un);
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fcmpu_get_vxsnan", a, b));
}

void PPUTranslator::FRSP(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	const auto result = m_ir->CreateFPTrunc(b, GetType<f32>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__frsp_get_fr", b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__frsp_get_fi", b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__frsp_get_ox", b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__frsp_get_ux", b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__frsp_get_vxsnan", b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FCTIW(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	//const auto sat_l = m_ir->CreateFCmpULT(b, ConstantFP::get(GetType<f64>(), -std::pow(2, 31))); // TODO ???
	//const auto sat_h = m_ir->CreateFCmpOGE(b, ConstantFP::get(GetType<f64>(), std::pow(2, 31)));
	//const auto converted = m_ir->CreateFPToSI(FP_SAT_OP(sat_l, b), GetType<s64>());
	//SetFpr(op.frd, m_ir->CreateSelect(sat_h, m_ir->getInt64(0x7fffffff), converted));
	SetFpr(op.frd, m_ir->CreateFPToSI(b, GetType<s32>()));

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fctiw_get_fr", b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fctiw_get_fi", b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fctiw_get_vxsnan", b));
	//SetFPSCRException(m_fpscr_vxcvi, m_ir->CreateOr(sat_l, sat_h));
	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_c);
	//SetFPCC(GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>(), op.rc != 0);
}

void PPUTranslator::FCTIWZ(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	SetFpr(op.frd, m_ir->CreateFPToSI(b, GetType<s32>()));
}

void PPUTranslator::FDIV(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto result = m_ir->CreateFDiv(a, b);
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fdiv_get_fr", a, b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fdiv_get_fi", a, b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fdiv_get_ox", a, b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fdiv_get_ux", a, b));
	//SetFPSCRException(m_fpscr_zx, Call(GetType<bool>(), m_pure_attr, "__fdiv_get_zx", a, b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fdiv_get_vxsnan", a, b));
	//SetFPSCRException(m_fpscr_vxidi, Call(GetType<bool>(), m_pure_attr, "__fdiv_get_vxidi", a, b));
	//SetFPSCRException(m_fpscr_vxzdz, Call(GetType<bool>(), m_pure_attr, "__fdiv_get_vxzdz", a, b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FSUB(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto result = m_ir->CreateFSub(a, b);
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fsub_get_fr", a, b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fsub_get_fi", a, b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fsub_get_ox", a, b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fsub_get_ux", a, b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fsub_get_vxsnan", a, b));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fsub_get_vxisi", a, b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FADD(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto result = m_ir->CreateFAdd(a, b);
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fadd_get_fr", a, b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fadd_get_fi", a, b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fadd_get_ox", a, b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fadd_get_ux", a, b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fadd_get_vxsnan", a, b));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fadd_get_vxisi", a, b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FSQRT(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	const auto result = Call(GetType<f64>(), "llvm.sqrt.f64", b);
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fsqrt_get_fr", b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fsqrt_get_fi", b));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fsqrt_get_ox", b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fsqrt_get_ux", b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fsqrt_get_vxsnan", b));
	//SetFPSCRException(m_fpscr_vxsqrt, Call(GetType<bool>(), m_pure_attr, "__fsqrt_get_vxsqrt", b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FSEL(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	SetFpr(op.frd, m_ir->CreateSelect(m_ir->CreateFCmpOGE(a, ConstantFP::get(GetType<f64>(), 0.0)), c, b));

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::FMUL(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFMul(a, c);
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmul_get_fr", a, c));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmul_get_fi", a, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmul_get_ox", a, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmul_get_ux", a, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmul_get_vxsnan", a, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmul_get_vximz", a, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FRSQRTE(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb, 32);
	const auto result = Call(GetType<f32>(), m_pure_attr, "__frsqrte", b);
	SetFpr(op.frd, result);

	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_fr);
	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_fi);
	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_xx);
	//SetFPSCRException(m_fpscr_zx, Call(GetType<bool>(), m_pure_attr, "__frsqrte_get_zx", b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__frsqrte_get_vxsnan", b));
	//SetFPSCRException(m_fpscr_vxsqrt, Call(GetType<bool>(), m_pure_attr, "__frsqrte_get_vxsqrt", b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FMSUB(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFSub(m_ir->CreateFMul(a, c), b);
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmadd_get_fr", a, b, c)); // TODO ???
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmadd_get_fi", a, b, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_ox", a, b, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_ux", a, b, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vxsnan", a, b, c));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vxisi", a, b, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vximz", a, b, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FMADD(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFAdd(m_ir->CreateFMul(a, c), b);
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmadd_get_fr", a, b, c));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmadd_get_fi", a, b, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_ox", a, b, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_ux", a, b, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vxsnan", a, b, c));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vxisi", a, b, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vximz", a, b, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FNMSUB(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFSub(b, m_ir->CreateFMul(a, c));
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmadd_get_fr", a, b, c)); // TODO ???
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmadd_get_fi", a, b, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_ox", a, b, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_ux", a, b, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vxsnan", a, b, c));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vxisi", a, b, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vximz", a, b, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FNMADD(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto c = GetFpr(op.frc);
	const auto result = m_ir->CreateFNeg(m_ir->CreateFAdd(m_ir->CreateFMul(a, c), b));
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fmadd_get_fr", a, b, c)); // TODO ???
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fmadd_get_fi", a, b, c));
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_ox", a, b, c));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_ux", a, b, c));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vxsnan", a, b, c));
	//SetFPSCRException(m_fpscr_vxisi, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vxisi", a, b, c));
	//SetFPSCRException(m_fpscr_vximz, Call(GetType<bool>(), m_pure_attr, "__fmadd_get_vximz", a, b, c));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::FCMPO(ppu_opcode_t op)
{
	const auto a = GetFpr(op.fra);
	const auto b = GetFpr(op.frb);
	const auto lt = m_ir->CreateFCmpOLT(a, b);
	const auto gt = m_ir->CreateFCmpOGT(a, b);
	const auto eq = m_ir->CreateFCmpOEQ(a, b);
	const auto un = m_ir->CreateFCmpUNO(a, b);
	SetCrField(op.crfd, lt, gt, eq, un);
	SetFPCC(lt, gt, eq, un);
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fcmpo_get_vxsnan", a, b));
	//SetFPSCRException(m_fpscr_vxvc, Call(GetType<bool>(), m_pure_attr, "__fcmpo_get_vxvc", a, b));
}

void PPUTranslator::FNEG(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	SetFpr(op.frd, m_ir->CreateFNeg(b));

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::FMR(ppu_opcode_t op)
{
	SetFpr(op.frd, GetFpr(op.frb));

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::FNABS(ppu_opcode_t op)
{
	SetFpr(op.frd, m_ir->CreateFNeg(Call(GetType<f64>(), "llvm.fabs.f64", GetFpr(op.frb))));

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::FABS(ppu_opcode_t op)
{
	SetFpr(op.frd, Call(GetType<f64>(), "llvm.fabs.f64", GetFpr(op.frb)));

	if (op.rc) SetCrField(1, m_ir->CreateLoad(m_fpscr_lt), m_ir->CreateLoad(m_fpscr_gt), m_ir->CreateLoad(m_fpscr_eq), m_ir->CreateLoad(m_fpscr_un));
}

void PPUTranslator::FCTID(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	//const auto sat_l = m_ir->CreateFCmpULT(b, ConstantFP::get(GetType<f64>(), -std::pow(2, 63)));
	//const auto sat_h = m_ir->CreateFCmpOGE(b, ConstantFP::get(GetType<f64>(), std::pow(2, 63)));
	//const auto converted = m_ir->CreateFPToSI(FP_SAT_OP(sat_l, b), GetType<s64>());
	//SetFpr(op.frd, m_ir->CreateSelect(sat_h, m_ir->getInt64(0x7fffffffffffffff), converted));
	SetFpr(op.frd, m_ir->CreateFPToSI(b, GetType<s64>()));

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fctid_get_fr", b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fctid_get_fi", b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fctid_get_vxsnan", b));
	//SetFPSCRException(m_fpscr_vxcvi, m_ir->CreateOr(sat_l, sat_h));
	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_c);
	//SetFPCC(GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>(), op.rc != 0);
}

void PPUTranslator::FCTIDZ(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	SetFpr(op.frd, m_ir->CreateFPToSI(b, GetType<s64>()));
}

void PPUTranslator::FCFID(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb, 64, true);
	const auto result = m_ir->CreateSIToFP(b, GetType<f64>());
	SetFpr(op.frd, result);

	//SetFPSCR_FR(Call(GetType<bool>(), m_pure_attr, "__fcfid_get_fr", b));
	//SetFPSCR_FI(Call(GetType<bool>(), m_pure_attr, "__fcfid_get_fi", b));
	SetFPRF(result, op.rc != 0);
}

void PPUTranslator::UNK(ppu_opcode_t op)
{
	LOG_WARNING(PPU, "0x%08llx: Unknown/illegal opcode 0x%08x", m_current_addr, op.opcode);
	m_ir->CreateUnreachable();
}


Value* PPUTranslator::GetGpr(u32 r, u32 num_bits)
{
	m_value_usage[m_gpr[r]]++;
	return m_ir->CreateTrunc(m_ir->CreateLoad(m_gpr[r]), m_ir->getIntNTy(num_bits));
}

void PPUTranslator::SetGpr(u32 r, Value* value)
{
	m_ir->CreateStore(m_ir->CreateZExt(value, GetType<u64>()), m_gpr[r]);
	m_value_usage[m_gpr[r]]++;
}

Value* PPUTranslator::GetFpr(u32 r, u32 bits, bool as_int)
{
	const auto value = m_ir->CreateAlignedLoad(m_fpr[r], 8);
	m_value_usage[m_fpr[r]]++;

	if (!as_int && bits == 64)
	{
		return value;
	}
	else if (!as_int && bits == 32)
	{
		return m_ir->CreateFPTrunc(value, GetType<f32>());
	}
	else
	{
		return m_ir->CreateTrunc(m_ir->CreateBitCast(value, GetType<u64>()), m_ir->getIntNTy(bits));
	}
}

void PPUTranslator::SetFpr(u32 r, Value* val)
{
	const auto f64_val =
		val->getType() == GetType<u32>() ? m_ir->CreateBitCast(ZExt(val), GetType<f64>()) :
		val->getType() == GetType<u64>() ? m_ir->CreateBitCast(val, GetType<f64>()) :
		val->getType() == GetType<f32>() ? m_ir->CreateFPExt(val, GetType<f64>()) : val;

	m_ir->CreateAlignedStore(f64_val, m_fpr[r], 8);
	m_value_usage[m_fpr[r]]++;
}

Value* PPUTranslator::GetVr(u32 vr, VrType type)
{
	const auto value = m_ir->CreateAlignedLoad(m_vr[vr], 16);
	m_value_usage[m_vr[vr]]++;

	switch (type)
	{
	case VrType::vi32: return value;
	case VrType::vi8: return m_ir->CreateBitCast(value, GetType<u8[16]>());
	case VrType::vi16: return m_ir->CreateBitCast(value, GetType<u16[8]>());
	case VrType::vf: return m_ir->CreateBitCast(value, GetType<f32[4]>());
	case VrType::i128: return m_ir->CreateBitCast(value, GetType<u128>());
	}

	throw std::logic_error("GetVr(): invalid type");
}

void PPUTranslator::SetVr(u32 vr, Value* value)
{
	const auto type = value->getType();
	const auto size = type->getPrimitiveSizeInBits();

	if (type->isVectorTy() && size != 128)
	{
		if (type->getScalarType()->isIntegerTy(1))
		{
			// Sign-extend bool values
			value = SExt(value, ScaleType(type, 7 - s32(std::log2(size))));
		}
		else if (size == 256 || size == 512)
		{
			// Truncate big vectors
			value = Trunc(value, ScaleType(type, 7 - s32(std::log2(size))));
		}
	}

	m_ir->CreateAlignedStore(m_ir->CreateBitCast(value, GetType<u32[4]>()), m_vr[vr], 16);
	m_value_usage[m_vr[vr]]++;
}

Value* PPUTranslator::GetCrb(u32 crb)
{
	return m_ir->CreateLoad(m_cr[crb]);
}

void PPUTranslator::SetCrb(u32 crb, Value* value)
{
	m_ir->CreateStore(value, m_cr[crb]);
}

void PPUTranslator::SetCrField(u32 group, Value* lt, Value* gt, Value* eq, Value* so)
{
	SetCrb(group * 4 + 0, lt ? lt : GetUndef<bool>());
	SetCrb(group * 4 + 1, gt ? gt : GetUndef<bool>());
	SetCrb(group * 4 + 2, eq ? eq : GetUndef<bool>());
	SetCrb(group * 4 + 3, so ? so : m_ir->CreateLoad(m_xer_so));
}

void PPUTranslator::SetCrFieldSignedCmp(u32 n, Value* a, Value* b)
{
	const auto lt = m_ir->CreateICmpSLT(a, b);
	const auto gt = m_ir->CreateICmpSGT(a, b);
	const auto eq = m_ir->CreateICmpEQ(a, b);
	SetCrField(n, lt, gt, eq);
}

void PPUTranslator::SetCrFieldUnsignedCmp(u32 n, Value* a, Value* b)
{
	const auto lt = m_ir->CreateICmpULT(a, b);
	const auto gt = m_ir->CreateICmpUGT(a, b);
	const auto eq = m_ir->CreateICmpEQ(a, b);
	SetCrField(n, lt, gt, eq);
}

void PPUTranslator::SetFPCC(Value* lt, Value* gt, Value* eq, Value* un, bool set_cr)
{
	m_ir->CreateStore(lt, m_fpscr_lt);
	m_ir->CreateStore(gt, m_fpscr_gt);
	m_ir->CreateStore(eq, m_fpscr_eq);
	m_ir->CreateStore(un, m_fpscr_un);
	if (set_cr) SetCrField(1, lt, gt, eq, un);
}

void PPUTranslator::SetFPRF(Value* value, bool set_cr)
{
	const bool is32 =
		value->getType()->isFloatTy() ? true :
		value->getType()->isDoubleTy() ? false :
		throw std::logic_error("SetFPRF(): invalid value type");

	//const auto zero = ConstantFP::get(value->getType(), 0.0);
	//const auto is_nan = m_ir->CreateFCmpUNO(value, zero);
	//const auto is_inf = Call(GetType<bool>(), m_pure_attr, is32 ? "__is_inf32" : "__is_inf", value); // TODO
	//const auto is_denorm = Call(GetType<bool>(), m_pure_attr, is32 ? "__is_denorm32" : "__is_denorm", value); // TODO
	//const auto is_neg_zero = Call(GetType<bool>(), m_pure_attr, is32 ? "__is_neg_zero32" : "__is_neg_zero", value); // TODO

	//const auto cc = m_ir->CreateOr(is_nan, m_ir->CreateOr(is_denorm, is_neg_zero));
	//const auto lt = m_ir->CreateFCmpOLT(value, zero);
	//const auto gt = m_ir->CreateFCmpOGT(value, zero);
	//const auto eq = m_ir->CreateFCmpOEQ(value, zero);
	//const auto un = m_ir->CreateOr(is_nan, is_inf);
	//m_ir->CreateStore(cc, m_fpscr_c);
	//SetFPCC(lt, gt, eq, un, set_cr);
}

void PPUTranslator::SetFPSCR_FR(Value* value)
{
	m_ir->CreateStore(value, m_fpscr_fr);
}

void PPUTranslator::SetFPSCR_FI(Value* value)
{
	m_ir->CreateStore(value, m_fpscr_fi);
	SetFPSCRException(m_fpscr_xx, value);
}

void PPUTranslator::SetFPSCRException(Value* ptr, Value* value)
{
	m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(ptr), value), ptr);
	m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(m_fpscr_fx), value), m_fpscr_fx);
}

Value* PPUTranslator::GetFPSCRBit(u32 n)
{
	if (n == 1 && m_fpscr[24])
	{
		// Floating-Point Enabled Exception Summary (FEX) 24-29
		Value* value = m_ir->CreateLoad(m_fpscr[24]);
		for (u32 i = 25; i <= 29; i++) value = m_ir->CreateOr(value, m_ir->CreateLoad(m_fpscr[i]));
		return value;
	}

	if (n == 2 && m_fpscr[7])
	{
		// Floating-Point Invalid Operation Exception Summary (VX) 7-12, 21-23
		Value* value = m_ir->CreateLoad(m_fpscr[7]);
		for (u32 i = 8; i <= 12; i++) value = m_ir->CreateOr(value, m_ir->CreateLoad(m_fpscr[i]));
		for (u32 i = 21; i <= 23; i++) value = m_ir->CreateOr(value, m_ir->CreateLoad(m_fpscr[i]));
		return value;
	}

	if (n >= 32 || !m_fpscr[n])
	{
		return nullptr; // ???
	}

	// Get bit
	const auto value = m_ir->CreateLoad(m_fpscr[n]);

	if (n == 0 || (n >= 3 && n <= 12) || (n >= 21 && n <= 23))
	{
		// Clear FX or exception bits
		m_ir->CreateStore(m_ir->getFalse(), m_fpscr[n]);
	}

	return value;
}

void PPUTranslator::SetFPSCRBit(u32 n, Value* value, bool update_fx)
{
	if (n >= 32 || !m_fpscr[n])
	{
		//CompilationError("SetFPSCRBit(): inaccessible bit " + std::to_string(n));
		return; // ???
	}

	if (update_fx)
	{
		if ((n >= 3 && n <= 12) || (n >= 21 && n <= 23))
		{
			// Update FX bit if necessary
			m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(m_fpscr_fx), value), m_fpscr_fx);
		}
	}

	//if (n >= 24 && n <= 28) CompilationError("SetFPSCRBit: exception enable bit " + std::to_string(n));
	//if (n == 29) CompilationError("SetFPSCRBit: NI bit");
	//if (n >= 30) CompilationError("SetFPSCRBit: RN bit");

	// Store the bit
	m_ir->CreateStore(value, m_fpscr[n]);
}

Value* PPUTranslator::GetCarry()
{
	return m_ir->CreateLoad(m_xer_ca);
}

void PPUTranslator::SetCarry(Value* bit)
{
	m_ir->CreateStore(bit, m_xer_ca);
}

void PPUTranslator::SetOverflow(Value* bit)
{
	m_ir->CreateStore(bit, m_xer_ov);
	m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(m_xer_so), bit), m_xer_so);
}

void PPUTranslator::SetSat(Value* bit)
{
	m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(m_vscr_sat), bit), m_vscr_sat);
}

Value* PPUTranslator::CheckTrapCondition(u32 to, Value* left, Value* right)
{
	Value* trap_condition = m_ir->getFalse();
	if (to & 0x10) trap_condition = m_ir->CreateOr(trap_condition, m_ir->CreateICmpSLT(left, right));
	if (to & 0x8) trap_condition = m_ir->CreateOr(trap_condition, m_ir->CreateICmpSGT(left, right));
	if (to & 0x4) trap_condition = m_ir->CreateOr(trap_condition, m_ir->CreateICmpEQ(left, right));
	if (to & 0x2) trap_condition = m_ir->CreateOr(trap_condition, m_ir->CreateICmpULT(left, right));
	if (to & 0x1) trap_condition = m_ir->CreateOr(trap_condition, m_ir->CreateICmpUGT(left, right));
	return trap_condition;
}

Value* PPUTranslator::Trap(u64 addr)
{
	return Call(GetType<void>(), /*AttributeSet::get(m_context, AttributeSet::FunctionIndex, Attribute::NoReturn),*/ "__trap", m_ir->getInt64(m_current_addr));
}

Value* PPUTranslator::CheckBranchCondition(u32 bo, u32 bi)
{
	const bool bo0 = (bo & 0x10) != 0;
	const bool bo1 = (bo & 0x08) != 0;
	const bool bo2 = (bo & 0x04) != 0;
	const bool bo3 = (bo & 0x02) != 0;

	// Decrement counter if necessary
	const auto ctr = bo2 ? nullptr : m_ir->CreateSub(m_ir->CreateLoad(m_reg_ctr), m_ir->getInt64(1));

	// Store counter if necessary
	if (ctr) m_ir->CreateStore(ctr, m_reg_ctr);

	// Generate counter condition
	const auto use_ctr = bo2 ? nullptr : m_ir->CreateICmp(bo3 ? ICmpInst::ICMP_EQ : ICmpInst::ICMP_NE, ctr, m_ir->getInt64(0));

	// Generate condition bit access
	const auto use_cond = bo0 ? nullptr : bo1 ? GetCrb(bi) : m_ir->CreateNot(GetCrb(bi));

	if (use_ctr && use_cond)
	{
		// Combine conditions if necessary
		return m_ir->CreateAnd(use_ctr, use_cond);
	}

	return use_ctr ? use_ctr : use_cond;
}

bool PPUTranslator::IsStackAddr(Value* addr)
{
	// Analyse various binary ops
	if (const auto bin_op = dyn_cast<BinaryOperator>(addr))
	{
		if (bin_op->isBinaryOp(Instruction::Add) || bin_op->isBinaryOp(Instruction::And) || bin_op->isBinaryOp(Instruction::Or) || bin_op->isBinaryOp(Instruction::Xor))
		{
			return IsStackAddr(bin_op->getOperand(0)) || IsStackAddr(bin_op->getOperand(1));
		}

		if (bin_op->isBinaryOp(Instruction::Sub))
		{
			return IsStackAddr(bin_op->getOperand(0));
		}

		// TODO
	}

	// Detect load instruction
	if (const auto load_op = dyn_cast<LoadInst>(addr))
	{
		return load_op->getOperand(0) == m_gpr[1];
	}

	return false;
}

#endif
