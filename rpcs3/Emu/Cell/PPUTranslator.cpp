#ifdef LLVM_AVAILABLE

#include "PPUTranslator.h"
#include "PPUThread.h"
#include "PPUInterpreter.h"

#include "../Utilities/Log.h"
#include <algorithm>

using namespace llvm;

const ppu_decoder<PPUTranslator> s_ppu_decoder;

PPUTranslator::PPUTranslator(LLVMContext& context, Module* module, const ppu_module& info, ExecutionEngine& engine)
	: cpu_translator(module, false)
	, m_info(info)
	, m_pure_attr(AttributeList::get(m_context, AttributeList::FunctionIndex, {Attribute::NoUnwind, Attribute::ReadNone}))
{
	// Bind context
	cpu_translator::initialize(context, engine);

	// There is no weak linkage on JIT, so let's create variables with different names for each module part
	const u32 gsuffix = m_info.name.empty() ? info.funcs[0].addr : info.funcs[0].addr - m_info.segs[0].addr;

	// Memory base
	m_base = new GlobalVariable(*module, ArrayType::get(GetType<char>(), 0x100000000)->getPointerTo(), true, GlobalValue::ExternalLinkage, 0, fmt::format("__mptr%x", gsuffix));
	m_base->setInitializer(ConstantPointerNull::get(cast<PointerType>(m_base->getType()->getPointerElementType())));
	m_base->setExternallyInitialized(true);

	// Thread context struct (TODO: safer member access)
	const u32 off0 = offset32(&ppu_thread::state);
	const u32 off1 = offset32(&ppu_thread::gpr);
	std::vector<Type*> thread_struct;
	thread_struct.emplace_back(ArrayType::get(GetType<char>(), off0));
	thread_struct.emplace_back(GetType<u32>()); // state
	thread_struct.emplace_back(ArrayType::get(GetType<char>(), off1 - off0 - 4));
	thread_struct.insert(thread_struct.end(), 32, GetType<u64>()); // gpr[0..31]
	thread_struct.insert(thread_struct.end(), 32, GetType<f64>()); // fpr[0..31]
	thread_struct.insert(thread_struct.end(), 32, GetType<u32[4]>()); // vr[0..31]
	thread_struct.insert(thread_struct.end(), 32, GetType<bool>()); // cr[0..31]
	thread_struct.insert(thread_struct.end(), 32, GetType<bool>()); // fpscr
	thread_struct.insert(thread_struct.end(), 2, GetType<u64>()); // lr, ctr
	thread_struct.insert(thread_struct.end(), 2, GetType<u32>()); // vrsave, cia
	thread_struct.insert(thread_struct.end(), 3, GetType<bool>()); // so, ov, ca
	thread_struct.insert(thread_struct.end(), 1, GetType<u8>()); // cnt
	thread_struct.insert(thread_struct.end(), 2, GetType<bool>()); // sat, nj

	m_thread_type = StructType::create(m_context, thread_struct, "context_t");

	// Callable
	m_call = new GlobalVariable(*module, ArrayType::get(GetType<u32>(), 0x80000000)->getPointerTo(), true, GlobalValue::ExternalLinkage, 0, fmt::format("__cptr%x", gsuffix));
	m_call->setInitializer(ConstantPointerNull::get(cast<PointerType>(m_call->getType()->getPointerElementType())));
	m_call->setExternallyInitialized(true);

	const auto md_name = MDString::get(m_context, "branch_weights");
	const auto md_low = ValueAsMetadata::get(ConstantInt::get(GetType<u32>(), 1));
	const auto md_high = ValueAsMetadata::get(ConstantInt::get(GetType<u32>(), 666));

	// Metadata for branch weights
	m_md_likely = MDTuple::get(m_context, {md_name, md_high, md_low});
	m_md_unlikely = MDTuple::get(m_context, {md_name, md_low, md_high});

	// Create segment variables
	for (const auto& seg : m_info.segs)
	{
		auto gv = new GlobalVariable(*module, GetType<u64>(), true, GlobalValue::ExternalLinkage, 0, fmt::format("__seg%u_%x", m_segs.size(), gsuffix));
		gv->setInitializer(ConstantInt::get(GetType<u64>(), seg.addr));
		gv->setExternallyInitialized(true);
		m_segs.emplace_back(gv);
	}

	// Sort relevant relocations (TODO)
	const auto caddr = m_info.segs[0].addr;
	const auto cend = caddr + m_info.segs[0].size;

	for (const auto& rel : m_info.relocs)
	{
		if (rel.addr >= caddr && rel.addr < cend)
		{
			// Check relocation type
			switch (rel.type)
			{

			// Ignore relative relocations, they are handled in emmitted code
			// Comment out types we haven't confirmed as used and working
			case 10:
			case 11:
			// case 12:
			// case 13:
			// case 26:
			// case 28:
			{
				ppu_log.notice("Ignoring relative relocation at 0x%x (%u)", rel.addr, rel.type);
				continue;
			}

			// Ignore 64-bit relocations
			case 20:
			case 22:
			case 38:
			case 43:
			case 44:
			case 45:
			case 46:
			case 51:
			case 68:
			case 73:
			case 78:
			{
				ppu_log.error("Ignoring 64-bit relocation at 0x%x (%u)", rel.addr, rel.type);
				continue;
			}
			}

			// Align relocation address (TODO)
			if (!m_relocs.emplace(rel.addr & ~3, &rel).second)
			{
				ppu_log.error("Relocation repeated at 0x%x (%u)", rel.addr, rel.type);
			}
		}
	}

	if (!m_info.name.empty())
	{
		m_reloc = &m_info.segs[0];
	}
}

PPUTranslator::~PPUTranslator()
{
}

Type* PPUTranslator::GetContextType()
{
	return m_thread_type;
}

Function* PPUTranslator::Translate(const ppu_function& info)
{
	m_function = m_module->getFunction(info.name);

	std::fill(std::begin(m_globals), std::end(m_globals), nullptr);
	std::fill(std::begin(m_locals), std::end(m_locals), nullptr);

	IRBuilder<> irb(BasicBlock::Create(m_context, "__entry", m_function));
	m_ir = &irb;

	// Instruction address is (m_addr + base)
	const u64 base = m_reloc ? m_reloc->addr : 0;
	m_addr = info.addr - base;

	m_thread = &*m_function->arg_begin();
	m_base_loaded = m_ir->CreateLoad(m_base);

	const auto body = BasicBlock::Create(m_context, "__body", m_function);

	// Check status register in the entry block
	const auto vstate = m_ir->CreateLoad(m_ir->CreateStructGEP(nullptr, m_thread, 1), true);
	const auto vcheck = BasicBlock::Create(m_context, "__test", m_function);
	m_ir->CreateCondBr(m_ir->CreateIsNull(vstate), body, vcheck, m_md_likely);

	// Create tail call to the check function
	m_ir->SetInsertPoint(vcheck);
	Call(GetType<void>(), "__check", m_thread, GetAddr())->setTailCallKind(llvm::CallInst::TCK_Tail);
	m_ir->CreateRetVoid();
	m_ir->SetInsertPoint(body);

	// Process blocks
	const auto block = std::make_pair(info.addr, info.size);
	{
		// Optimize BLR (prefetch LR)
		if (vm::read32(vm::cast(block.first + block.second - 4)) == ppu_instructions::BLR())
		{
			RegLoad(m_lr);
		}

		// Process the instructions
		for (m_addr = block.first - base; m_addr < block.first + block.second - base; m_addr += 4)
		{
			if (m_ir->GetInsertBlock()->getTerminator())
			{
				break;
			}

			// Find the relocation at current address
			const auto rel_found = m_relocs.find(m_addr + base);

			if (rel_found != m_relocs.end())
			{
				m_rel = rel_found->second;
			}
			else
			{
				m_rel = nullptr;
			}

			const u32 op = vm::read32(vm::cast(m_addr + base));
			(this->*(s_ppu_decoder.decode(op)))({op});

			if (m_rel)
			{
				// This is very bad. m_rel is normally set to nullptr after a relocation is handled (so it wasn't)
				ppu_log.error("LLVM: [0x%x] Unsupported relocation(%u) in '%s'. Please report.", rel_found->first, m_rel->type, m_info.name);
				return nullptr;
			}
		}

		// Finalize current block if necessary (create branch to the next address)
		if (!m_ir->GetInsertBlock()->getTerminator())
		{
			FlushRegisters();
			CallFunction(m_addr);
		}
	}

	return m_function;
}

Value* PPUTranslator::GetAddr(u64 _add)
{
	if (m_reloc)
	{
		// Load segment address from global variable, compute actual instruction address
		return m_ir->CreateAdd(m_ir->getInt64(m_addr + _add), m_ir->CreateLoad(m_segs[m_reloc - m_info.segs.data()]));
	}

	return m_ir->getInt64(m_addr + _add);
}

Type* PPUTranslator::ScaleType(Type* type, s32 pow2)
{
	verify(HERE), (type->getScalarType()->isIntegerTy());

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

void PPUTranslator::CallFunction(u64 target, Value* indirect)
{
	const auto type = FunctionType::get(GetType<void>(), {m_thread_type->getPointerTo()}, false);
	const auto block = m_ir->GetInsertBlock();

	if (!indirect)
	{
		if ((!m_reloc && target < 0x10000) || target >= -0x10000)
		{
			Trap();
			return;
		}

		indirect = m_module->getOrInsertFunction(fmt::format("__0x%llx", target), type).getCallee();
	}
	else
	{
		m_ir->CreateStore(Trunc(indirect, GetType<u32>()), m_ir->CreateStructGEP(nullptr, m_thread, &m_cia - m_locals), true);

		// Try to optimize
		if (auto inst = dyn_cast_or_null<Instruction>(indirect))
		{
			if (auto next = inst->getNextNode())
			{
				m_ir->SetInsertPoint(next);
			}
		}

		const auto pos = m_ir->CreateShl(m_ir->CreateLShr(indirect, 2, "", true), 1, "", true);
		const auto ptr = m_ir->CreateGEP(m_ir->CreateLoad(m_call), {m_ir->getInt64(0), pos});
		indirect = m_ir->CreateIntToPtr(m_ir->CreateLoad(ptr), type->getPointerTo());
	}

	m_ir->SetInsertPoint(block);
	m_ir->CreateCall(indirect, {m_thread})->setTailCallKind(llvm::CallInst::TCK_Tail);
	m_ir->CreateRetVoid();
}

Value* PPUTranslator::RegInit(Value*& local)
{
	const auto index = ::narrow<uint>(&local - m_locals);

	if (auto old = cast_or_null<Instruction>(m_globals[index]))
	{
		old->eraseFromParent();
	}

	// (Re)Initialize global, will be written in FlushRegisters
	m_globals[index] = m_ir->CreateStructGEP(nullptr, m_thread, index);

	return m_globals[index];
}

Value* PPUTranslator::RegLoad(Value*& local)
{
	const auto index = ::narrow<uint>(&local - m_locals);

	if (local)
	{
		// Simple load
		return local;
	}

	// Load from the global value
	local = m_ir->CreateLoad(m_ir->CreateStructGEP(nullptr, m_thread, index));
	return local;
}

void PPUTranslator::RegStore(llvm::Value* value, llvm::Value*& local)
{
	const auto glb = RegInit(local);
	local = value;
}

void PPUTranslator::FlushRegisters()
{
	const auto block = m_ir->GetInsertBlock();

	for (auto& local : m_locals)
	{
		const auto index = ::narrow<uint>(&local - m_locals);

		// Store value if necessary
		if (local && m_globals[index])
		{
			if (auto next = cast<Instruction>(m_globals[index])->getNextNode())
			{
				m_ir->SetInsertPoint(next);
			}
			else
			{
				m_ir->SetInsertPoint(block);
			}

			m_ir->CreateStore(local, m_ir->CreateBitCast(m_globals[index], local->getType()->getPointerTo()));
			m_globals[index] = nullptr;
		}
	}

	m_ir->SetInsertPoint(block);
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

void PPUTranslator::UseCondition(MDNode* hint, Value* cond)
{
	FlushRegisters();

	if (cond)
	{
		const auto local = BasicBlock::Create(m_context, "__cond", m_function);
		const auto next = BasicBlock::Create(m_context, "__next", m_function);
		m_ir->CreateCondBr(cond, local, next, hint);
		m_ir->SetInsertPoint(next);
		CallFunction(m_addr + 4);
		m_ir->SetInsertPoint(local);
	}
}

llvm::Value* PPUTranslator::GetMemory(llvm::Value* addr, llvm::Type* type)
{
	return m_ir->CreateBitCast(m_ir->CreateGEP(m_base_loaded, {m_ir->getInt64(0), addr}), type->getPointerTo());
}

Value* PPUTranslator::ReadMemory(Value* addr, Type* type, bool is_be, u32 align)
{
	const auto size = type->getPrimitiveSizeInBits();

	if (is_be ^ m_is_be && size > 8)
	{
		// Read, byteswap, bitcast
		const auto int_type = m_ir->getIntNTy(size);
		const auto value = m_ir->CreateAlignedLoad(GetMemory(addr, int_type), align, true);
		return m_ir->CreateBitCast(Call(int_type, fmt::format("llvm.bswap.i%u", size), value), type);
	}

	// Read normally
	return m_ir->CreateAlignedLoad(GetMemory(addr, type), align, true);
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
	m_ir->CreateAlignedStore(value, GetMemory(addr, value->getType()), align, true);
}

void PPUTranslator::CompilationError(const std::string& error)
{
	ppu_log.error("LLVM: [0x%08x] Error: %s", m_addr + (m_reloc ? m_reloc->addr : 0), error);
}


void PPUTranslator::MFVSCR(ppu_opcode_t op)
{
	const auto vscr = m_ir->CreateOr(ZExt(RegLoad(m_sat), GetType<u32>()), m_ir->CreateShl(ZExt(RegLoad(m_nj), GetType<u32>()), 16));
	SetVr(op.vd, m_ir->CreateInsertElement(ConstantVector::getSplat(4, m_ir->getInt32(0)), vscr, m_ir->getInt32(m_is_be ? 3 : 0)));
}

void PPUTranslator::MTVSCR(ppu_opcode_t op)
{
	const auto vscr = m_ir->CreateExtractElement(GetVr(op.vb, VrType::vi32), m_ir->getInt32(m_is_be ? 3 : 0));
	RegStore(Trunc(m_ir->CreateLShr(vscr, 16), GetType<bool>()), m_nj);
	RegStore(Trunc(vscr, GetType<bool>()), m_sat);
}

void PPUTranslator::VADDCUW(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, zext<u32[4]>(a + b < a));
}

void PPUTranslator::VADDFP(ppu_opcode_t op)
{
	const auto a = get_vr<f32[4]>(op.va);
	const auto b = get_vr<f32[4]>(op.vb);
	set_vr(op.vd, eval(a + b));
}

void PPUTranslator::VADDSBS(ppu_opcode_t op)
{
	const auto a = get_vr<s8[16]>(op.va);
	const auto b = get_vr<s8[16]>(op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a + b)).value));
}

void PPUTranslator::VADDSHS(ppu_opcode_t op)
{
	const auto a = get_vr<s16[8]>(op.va);
	const auto b = get_vr<s16[8]>(op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a + b)).value));
}

void PPUTranslator::VADDSWS(ppu_opcode_t op)
{
	const auto a = get_vr<s32[4]>(op.va);
	const auto b = get_vr<s32[4]>(op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a + b)).value));
}

void PPUTranslator::VADDUBM(ppu_opcode_t op)
{
	const auto a = get_vr<u8[16]>(op.va);
	const auto b = get_vr<u8[16]>(op.vb);
	set_vr(op.vd, eval(a + b));
}

void PPUTranslator::VADDUBS(ppu_opcode_t op)
{
	const auto a = get_vr<u8[16]>(op.va);
	const auto b = get_vr<u8[16]>(op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a + b)).value));
}

void PPUTranslator::VADDUHM(ppu_opcode_t op)
{
	const auto a = get_vr<u16[8]>(op.va);
	const auto b = get_vr<u16[8]>(op.vb);
	set_vr(op.vd, eval(a + b));
}

void PPUTranslator::VADDUHS(ppu_opcode_t op)
{
	const auto a = get_vr<u16[8]>(op.va);
	const auto b = get_vr<u16[8]>(op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a + b)).value));
}

void PPUTranslator::VADDUWM(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, eval(a + b));
}

void PPUTranslator::VADDUWS(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a + b)).value));
}

void PPUTranslator::VAND(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, eval(a & b));
}

void PPUTranslator::VANDC(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, eval(a & ~b));
}

void PPUTranslator::VAVGSB(ppu_opcode_t op)
{
	const auto a = get_vr<s8[16]>(op.va);
	const auto b = get_vr<s8[16]>(op.vb);
	set_vr(op.vd, eval(avg(a, b)));
}

void PPUTranslator::VAVGSH(ppu_opcode_t op)
{
	const auto a = get_vr<s16[8]>(op.va);
	const auto b = get_vr<s16[8]>(op.vb);
	set_vr(op.vd, eval(avg(a, b)));
}

void PPUTranslator::VAVGSW(ppu_opcode_t op)
{
	const auto a = get_vr<s32[4]>(op.va);
	const auto b = get_vr<s32[4]>(op.vb);
	set_vr(op.vd, eval(avg(a, b)));
}

void PPUTranslator::VAVGUB(ppu_opcode_t op)
{
	const auto a = get_vr<u8[16]>(op.va);
	const auto b = get_vr<u8[16]>(op.vb);
	set_vr(op.vd, eval(avg(a, b)));
}

void PPUTranslator::VAVGUH(ppu_opcode_t op)
{
	const auto a = get_vr<u16[8]>(op.va);
	const auto b = get_vr<u16[8]>(op.vb);
	set_vr(op.vd, eval(avg(a, b)));
}

void PPUTranslator::VAVGUW(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, eval(avg(a, b)));
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

// TODO: remove this (wrong casts)
#define FP_SAT_OP(fcmp, value) m_ir->CreateSelect(fcmp, cast<Constant>(cast<FCmpInst>(fcmp)->getOperand(1)), value)

void PPUTranslator::VCTSXS(ppu_opcode_t op)
{
	const auto b = GetVr(op.vb, VrType::vf);
	const auto scaled = Scale(b, op.vuimm);
	//const auto const0 = ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), 0.0));
	const auto const1 = ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), -std::pow(2, 31)));
	//const auto is_nan = m_ir->CreateFCmpUNO(b, const0); // NaN -> 0.0
	const auto sat_l = m_ir->CreateFCmpOLT(scaled, const1); // TODO ???
	const auto sat_h = m_ir->CreateFCmpOGE(scaled, ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), std::pow(2, 31))));
	const auto converted = m_ir->CreateFPToSI(m_ir->CreateSelect(sat_l, const1, scaled), GetType<s32[4]>());
	SetVr(op.vd, m_ir->CreateSelect(sat_h, ConstantVector::getSplat(4, m_ir->getInt32(0x7fffffff)), converted));
	SetSat(IsNotZero(m_ir->CreateOr(sat_l, sat_h)));
}

void PPUTranslator::VCTUXS(ppu_opcode_t op)
{
	const auto b = GetVr(op.vb, VrType::vf);
	const auto scaled = Scale(b, op.vuimm);
	const auto const0 = ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), 0.0));
	//const auto is_nan = m_ir->CreateFCmpUNO(b, const0); // NaN -> 0.0
	const auto sat_l = m_ir->CreateFCmpOLT(scaled, const0);
	const auto sat_h = m_ir->CreateFCmpOGE(scaled, ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), std::pow(2, 32)))); // TODO ???
	const auto converted = m_ir->CreateFPToUI(m_ir->CreateSelect(sat_l, const0, scaled), GetType<u32[4]>());
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

	// const auto a = get_vr<s16[8]>(op.va);
	// const auto b = get_vr<s16[8]>(op.vb);
	// const auto c = get_vr<s16[8]>(op.vc);
	// value_t<s16[8]> m;
	// m.value = m_ir->CreateShl(Trunc(m_ir->CreateAShr(m_ir->CreateMul(SExt(a.value), SExt(b.value)), 16)), 1);
	// m.value = m_ir->CreateOr(m.value, m_ir->CreateLShr(m_ir->CreateMul(a.value, b.value), 15));
	// const auto s = eval(c + m);
	// const auto z = eval((c >> 15) ^ 0x7fff);
	// const auto x = eval(((m ^ s) & ~(c ^ m)) >> 15);
	// set_vr(op.vd, eval((z & x) | (s & ~x)));
	//SetSat(IsNotZero(saturated.second));
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
	const auto a = get_vr<s16[8]>(op.va);
	const auto b = get_vr<u16[8]>(op.vb);
	const auto c = get_vr<s32[4]>(op.vc);
	const auto ml = bitcast<s32[4]>((a << 8 >> 8) * noncast<s16[8]>(b << 8 >> 8));
	const auto mh = bitcast<s32[4]>((a >> 8) * noncast<s16[8]>(b >> 8));
	set_vr(op.vd, eval(((ml << 16 >> 16) + (ml >> 16)) + ((mh << 16 >> 16) + (mh >> 16)) + c));
}

void PPUTranslator::VMSUMSHM(ppu_opcode_t op)
{
	const auto a = get_vr<s32[4]>(op.va);
	const auto b = get_vr<s32[4]>(op.vb);
	const auto c = get_vr<s32[4]>(op.vc);
	const auto ml = eval((a << 16 >> 16) * (b << 16 >> 16));
	const auto mh = eval((a >> 16) * (b >> 16));
	set_vr(op.vd, eval(ml + mh + c));
}

void PPUTranslator::VMSUMSHS(ppu_opcode_t op)
{
	const auto a = get_vr<s32[4]>(op.va);
	const auto b = get_vr<s32[4]>(op.vb);
	const auto c = get_vr<s32[4]>(op.vc);
	const auto ml = eval((a << 16 >> 16) * (b << 16 >> 16));
	const auto mh = eval((a >> 16) * (b >> 16));
	const auto m = eval(ml + mh);
	const auto s = eval(m + c);
	const auto z = eval((c >> 31) ^ 0x7fffffff);
	const auto mx = eval(m ^ sext<s32[4]>(m == 0x80000000u));
	const auto x = eval(((mx ^ s) & ~(c ^ mx)) >> 31);
	set_vr(op.vd, eval((z & x) | (s & ~x)));
	SetSat(IsNotZero(x.value));
}

void PPUTranslator::VMSUMUBM(ppu_opcode_t op)
{
	const auto a = get_vr<u16[8]>(op.va);
	const auto b = get_vr<u16[8]>(op.vb);
	const auto c = get_vr<u32[4]>(op.vc);
	const auto ml = bitcast<u32[4]>((a << 8 >> 8) * (b << 8 >> 8));
	const auto mh = bitcast<u32[4]>((a >> 8) * (b >> 8));
	set_vr(op.vd, eval(((ml << 16 >> 16) + (ml >> 16)) + ((mh << 16 >> 16) + (mh >> 16)) + c));
}

void PPUTranslator::VMSUMUHM(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	const auto c = get_vr<u32[4]>(op.vc);
	const auto ml = eval((a << 16 >> 16) * (b << 16 >> 16));
	const auto mh = eval((a >> 16) * (b >> 16));
	set_vr(op.vd, eval(ml + mh + c));
}

void PPUTranslator::VMSUMUHS(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	const auto c = get_vr<u32[4]>(op.vc);
	const auto ml = noncast<u32[4]>((a << 16 >> 16) * (b << 16 >> 16));
	const auto mh = noncast<u32[4]>((a >> 16) * (b >> 16));
	const auto s = eval(ml + mh);
	const auto s2 = eval(s + c);
	const auto x = eval((s < ml) | (s2 < s));
	set_vr(op.vd, select(x, splat<u32[4]>(-1), s2));
	SetSat(IsNotZero(x.value));
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
	SetVr(op.vd, m_ir->CreateFNeg(m_ir->CreateFSub(m_ir->CreateFMul(acb[0], acb[1]), acb[2])));
}

void PPUTranslator::VNOR(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, eval(~(a | b)));
}

void PPUTranslator::VOR(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, eval(a | b));
}

void PPUTranslator::VPERM(ppu_opcode_t op)
{
	const auto a = get_vr<u8[16]>(op.va);
	const auto b = get_vr<u8[16]>(op.vb);
	const auto c = get_vr<u8[16]>(op.vc);
	const auto i = eval(~c & 0x1f);
	set_vr(op.vd, select(noncast<s8[16]>(c << 3) >= 0, pshufb(a, i), pshufb(b, i)));
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
	const auto result = m_ir->CreateFDiv(ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), 1.0)), GetVr(op.vb, VrType::vf));
	SetVr(op.vd, result);
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
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, rol(a, b));
}

void PPUTranslator::VRLH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, rol(a, b));
}

void PPUTranslator::VRLW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, rol(a, b));
}

void PPUTranslator::VRSQRTEFP(ppu_opcode_t op)
{
	const auto result = m_ir->CreateFDiv(ConstantVector::getSplat(4, ConstantFP::get(GetType<f32>(), 1.0)), Call(GetType<f32[4]>(), "llvm.sqrt.v4f32", GetVr(op.vb, VrType::vf)));
	SetVr(op.vd, result);
}

void PPUTranslator::VSEL(ppu_opcode_t op)
{
	const auto [a, b, c] = get_vrs<u32[4]>(op.va, op.vb, op.vc);
	set_vr(op.vd, eval((b & c) | (a & ~c)));
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
	SetVr(op.vd, m_ir->CreateShl(GetVr(op.va, VrType::i128), m_ir->CreateAnd(GetVr(op.vb, VrType::i128), 0x78)));
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
	SetVr(op.vd, m_ir->CreateLShr(GetVr(op.va, VrType::i128), m_ir->CreateAnd(GetVr(op.vb, VrType::i128), 0x78)));
}

void PPUTranslator::VSRW(ppu_opcode_t op)
{
	const auto ab = GetVrs(VrType::vi32, op.va, op.vb);
	SetVr(op.vd, m_ir->CreateLShr(ab[0], m_ir->CreateAnd(ab[1], 31)));
}

void PPUTranslator::VSUBCUW(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, zext<u32[4]>(a >= b));
}

void PPUTranslator::VSUBFP(ppu_opcode_t op)
{
	const auto a = get_vr<f32[4]>(op.va);
	const auto b = get_vr<f32[4]>(op.vb);
	set_vr(op.vd, eval(a - b));
}

void PPUTranslator::VSUBSBS(ppu_opcode_t op)
{
	const auto a = get_vr<s8[16]>(op.va);
	const auto b = get_vr<s8[16]>(op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a - b)).value));
}

void PPUTranslator::VSUBSHS(ppu_opcode_t op)
{
	const auto a = get_vr<s16[8]>(op.va);
	const auto b = get_vr<s16[8]>(op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a - b)).value));
}

void PPUTranslator::VSUBSWS(ppu_opcode_t op)
{
	const auto a = get_vr<s32[4]>(op.va);
	const auto b = get_vr<s32[4]>(op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a - b)).value));
}

void PPUTranslator::VSUBUBM(ppu_opcode_t op)
{
	const auto a = get_vr<u8[16]>(op.va);
	const auto b = get_vr<u8[16]>(op.vb);
	set_vr(op.vd, eval(a - b));
}

void PPUTranslator::VSUBUBS(ppu_opcode_t op)
{
	const auto a = get_vr<u8[16]>(op.va);
	const auto b = get_vr<u8[16]>(op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a - b)).value));
}

void PPUTranslator::VSUBUHM(ppu_opcode_t op)
{
	const auto a = get_vr<u16[8]>(op.va);
	const auto b = get_vr<u16[8]>(op.vb);
	set_vr(op.vd, eval(a - b));
}

void PPUTranslator::VSUBUHS(ppu_opcode_t op)
{
	const auto a = get_vr<u16[8]>(op.va);
	const auto b = get_vr<u16[8]>(op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a - b)).value));
}

void PPUTranslator::VSUBUWM(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	set_vr(op.vd, eval(a - b));
}

void PPUTranslator::VSUBUWS(ppu_opcode_t op)
{
	const auto a = get_vr<u32[4]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	SetSat(IsNotZero(eval(r != (a - b)).value));
}

void PPUTranslator::VSUMSWS(ppu_opcode_t op)
{
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
	const auto a = SExt(GetVr(op.va, VrType::vi8), GetType<s32[16]>());
	const auto b = GetVr(op.vb, VrType::vi32);
	const auto e0 = Shuffle(a, nullptr, { 0, 4, 8, 12 });
	const auto e1 = Shuffle(a, nullptr, { 1, 5, 9, 13 });
	const auto e2 = Shuffle(a, nullptr, { 2, 6, 10, 14 });
	const auto e3 = Shuffle(a, nullptr, { 3, 7, 11, 15 });
	const auto result = m_ir->CreateAdd(SExt(b), SExt(Add({ e0, e1, e2, e3 }))); // Summ, (e0+e1+e2+e3) is small
	const auto saturated = SaturateSigned(result, -0x80000000ll, 0x7fffffff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUM4SHS(ppu_opcode_t op)
{
	const auto a = SExt(GetVr(op.va, VrType::vi16));
	const auto b = GetVr(op.vb, VrType::vi32);
	const auto e0 = Shuffle(a, nullptr, { 0, 2, 4, 6 });
	const auto e1 = Shuffle(a, nullptr, { 1, 3, 5, 7 });
	const auto result = m_ir->CreateAdd(SExt(b), SExt(Add({ e0, e1 }))); // Summ, (e0+e1) is small
	const auto saturated = SaturateSigned(result, -0x80000000ll, 0x7fffffff);
	SetVr(op.vd, saturated.first);
	SetSat(IsNotZero(saturated.second));
}

void PPUTranslator::VSUM4UBS(ppu_opcode_t op)
{
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
	UseCondition(m_md_unlikely, CheckTrapCondition(op.bo, GetGpr(op.ra), m_ir->getInt64(op.simm16)));
	Trap();
}

void PPUTranslator::TWI(ppu_opcode_t op)
{
	UseCondition(m_md_unlikely, CheckTrapCondition(op.bo, GetGpr(op.ra, 32), m_ir->getInt32(op.simm16)));
	Trap();
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
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto a = GetGpr(op.ra);
	const auto result = m_ir->CreateAdd(a, imm);
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULT(result, imm));
	if (op.main & 1) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::ADDI(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm);
}

void PPUTranslator::ADDIS(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16 << 16);

	if (m_rel && m_rel->type == 6)
	{
		imm = m_ir->CreateShl(SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), 16);
		m_rel = nullptr;
	}

	SetGpr(op.rd, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm);
}

void PPUTranslator::BC(ppu_opcode_t op)
{
	const u64 target = (op.aa ? 0 : m_addr) + op.bt14;

	if (op.aa && m_reloc)
	{
		CompilationError("Branch with absolute address");
	}

	if (op.lk)
	{
		m_ir->CreateStore(GetAddr(+4), m_ir->CreateStructGEP(nullptr, m_thread, &m_lr - m_locals));
	}

	UseCondition(CheckBranchProbability(op.bo), CheckBranchCondition(op.bo, op.bi));

	CallFunction(target);
}

void PPUTranslator::SC(ppu_opcode_t op)
{
	if (op.opcode != ppu_instructions::SC(0) && op.opcode != ppu_instructions::SC(1))
	{
		return UNK(op);
	}

	const auto num = GetGpr(11);
	RegStore(Trunc(GetAddr()), m_cia);
	FlushRegisters();

	if (!op.lev && isa<ConstantInt>(num))
	{
		// Try to determine syscall using the constant value from r11
		const u64 index = cast<ConstantInt>(num)->getZExtValue();

		if (index < 1024)
		{
			// Call the syscall directly
			Call(GetType<void>(), fmt::format("%s", ppu_syscall_code(index)), m_thread)->setTailCallKind(llvm::CallInst::TCK_Tail);
			m_ir->CreateRetVoid();
			return;
		}
	}

	Call(GetType<void>(), op.lev ? "__lv1call" : "__syscall", m_thread, num)->setTailCallKind(llvm::CallInst::TCK_Tail);
	m_ir->CreateRetVoid();
}

void PPUTranslator::B(ppu_opcode_t op)
{
	const u64 target = (op.aa ? 0 : m_addr) + op.bt24;

	if (op.aa && m_reloc)
	{
		CompilationError("Branch with absolute address");
	}

	if (op.lk)
	{
		RegStore(GetAddr(+4), m_lr);
	}

	FlushRegisters();
	CallFunction(target);
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
	const auto target = RegLoad(m_lr);

	if (op.lk)
	{
		m_ir->CreateStore(GetAddr(+4), m_ir->CreateStructGEP(nullptr, m_thread, &m_lr - m_locals));
	}

	UseCondition(CheckBranchProbability(op.bo), CheckBranchCondition(op.bo, op.bi));

	CallFunction(0, target);
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
	m_ir->CreateFence(AtomicOrdering::Acquire);
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
	const auto target = RegLoad(m_ctr);

	if (op.lk)
	{
		m_ir->CreateStore(GetAddr(+4), m_ir->CreateStructGEP(nullptr, m_thread, &m_lr - m_locals));
	}

	UseCondition(CheckBranchProbability(op.bo | 0x4), CheckBranchCondition(op.bo | 0x4, op.bi));

	CallFunction(0, target);
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
	Value* imm = m_ir->getInt64(op.uimm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = ZExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.ra, m_ir->CreateOr(GetGpr(op.rs), imm));
}

void PPUTranslator::ORIS(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.uimm16 << 16);

	if (m_rel && m_rel->type == 5)
	{
		imm = m_ir->CreateShl(ZExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), 16);
		m_rel = nullptr;
	}

	if (m_rel && m_rel->type == 6)
	{
		imm = m_ir->CreateShl(ZExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), 16);
		m_rel = nullptr;
	}

	SetGpr(op.ra, m_ir->CreateOr(GetGpr(op.rs), imm));
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
	if (op.opcode != ppu_instructions::TRAP())
	{
		UseCondition(m_md_unlikely, CheckTrapCondition(op.bo, GetGpr(op.ra, 32), GetGpr(op.rb, 32)));
	}
	else
	{
		FlushRegisters();
	}

	Trap();
}

void PPUTranslator::LVSL(ppu_opcode_t op)
{
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	//const auto _add = m_ir->CreateInsertElement(ConstantVector::getSplat(16, m_ir->getInt8(0)), Trunc(m_ir->CreateAnd(addr, 0xf), GetType<u8>()), m_ir->getInt32(0));
	//const auto base = ConstantDataVector::get(m_context, std::vector<u8>{15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
	//SetVr(op.vd, m_ir->CreateAdd(base, Shuffle(_add, nullptr, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})));
	SetVr(op.vd, Call(GetType<u8[16]>(), m_pure_attr, "__lvsl", addr));
}

void PPUTranslator::LVEBX(ppu_opcode_t op)
{
	return LVX(op);
}

void PPUTranslator::SUBFC(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto result = m_ir->CreateSub(b, a);
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULE(result, b));
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
	else if (std::none_of(m_cr + 0, m_cr + 32, [](auto* p) { return p; }))
	{
		// MFCR (optimized)
		Value* ln0 = m_ir->CreateIntToPtr(m_ir->CreatePtrToInt(m_ir->CreateStructGEP(nullptr, m_thread, 99), GetType<uptr>()), GetType<u8[16]>()->getPointerTo());
		Value* ln1 = m_ir->CreateIntToPtr(m_ir->CreatePtrToInt(m_ir->CreateStructGEP(nullptr, m_thread, 115), GetType<uptr>()), GetType<u8[16]>()->getPointerTo());

		ln0 = m_ir->CreateLoad(ln0);
		ln1 = m_ir->CreateLoad(ln1);
		if (!m_is_be)
		{
			ln0 = Shuffle(ln0, nullptr, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
			ln1 = Shuffle(ln1, nullptr, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		}

		const auto m0 = Call(GetType<u32>(), m_pure_attr, "llvm.x86.sse2.pmovmskb.128", m_ir->CreateShl(ln0, 7));
		const auto m1 = Call(GetType<u32>(), m_pure_attr, "llvm.x86.sse2.pmovmskb.128", m_ir->CreateShl(ln1, 7));
		SetGpr(op.rd, m_ir->CreateOr(m_ir->CreateShl(m0, 16), m1));
		return;
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
	SetGpr(op.rd, Call(GetType<u32>(), "__lwarx", m_thread, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
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
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	//const auto _add = m_ir->CreateInsertElement(ConstantVector::getSplat(16, m_ir->getInt8(0)), Trunc(m_ir->CreateAnd(addr, 0xf), GetType<u8>()), m_ir->getInt32(0));
	//const auto base = ConstantDataVector::get(m_context, std::vector<u8>{31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16});
	//SetVr(op.vd, m_ir->CreateSub(base, Shuffle(_add, nullptr, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})));
	SetVr(op.vd, Call(GetType<u8[16]>(), m_pure_attr, "__lvsr", addr));
}

void PPUTranslator::LVEHX(ppu_opcode_t op)
{
	return LVX(op);
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
	UseCondition(m_md_unlikely, CheckTrapCondition(op.bo, GetGpr(op.ra), GetGpr(op.rb)));
	Trap();
}

void PPUTranslator::LVEWX(ppu_opcode_t op)
{
	return LVX(op);
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
	SetGpr(op.rd, m_ir->CreateAShr(m_ir->CreateMul(a, b), 32));
	if (op.rc) SetCrField(0, GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>());
}

void PPUTranslator::LDARX(ppu_opcode_t op)
{
	SetGpr(op.rd, Call(GetType<u64>(), "__ldarx", m_thread, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
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
	const auto addr = m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), ~0xfull);
	const auto data = ReadMemory(addr, GetType<u8[16]>(), m_is_be, 16);
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
	const auto r1 = m_ir->CreateAdd(a, b);
	const auto r2 = m_ir->CreateAdd(r1, ZExt(c, GetType<u64>()));
	SetGpr(op.rd, r2);
	SetCarry(m_ir->CreateOr(m_ir->CreateICmpULT(r1, a), m_ir->CreateICmpULT(r2, r1)));
	if (op.rc) SetCrFieldSignedCmp(0, r2, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__subfe_get_ov", a, b, c));
}

void PPUTranslator::ADDE(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto c = GetCarry();
	const auto r1 = m_ir->CreateAdd(a, b);
	const auto r2 = m_ir->CreateAdd(r1, ZExt(c, GetType<u64>()));
	SetGpr(op.rd, r2);
	SetCarry(m_ir->CreateOr(m_ir->CreateICmpULT(r1, a), m_ir->CreateICmpULT(r2, r1)));
	if (op.rc) SetCrFieldSignedCmp(0, r2, m_ir->getInt64(0));
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

	static u8 s_table[64]
	{
		0, 0, 0, 0,
		0, 0, 0, 1,
		0, 0, 1, 0,
		0, 0, 1, 1,
		0, 1, 0, 0,
		0, 1, 0, 1,
		0, 1, 1, 0,
		0, 1, 1, 1,
		1, 0, 0, 0,
		1, 0, 0, 1,
		1, 0, 1, 0,
		1, 0, 1, 1,
		1, 1, 0, 0,
		1, 1, 0, 1,
		1, 1, 1, 0,
		1, 1, 1, 1,
	};

	if (!m_mtocr_table)
	{
		m_mtocr_table = new GlobalVariable(*m_module, ArrayType::get(GetType<u8>(), 64), true, GlobalValue::PrivateLinkage, ConstantDataArray::get(m_context, s_table));
	}

	const auto value = GetGpr(op.rs, 32);

	for (u32 i = 0; i < 8; i++)
	{
		if (op.crm & (128 >> i))
		{
			// Discard pending values
			std::fill_n(m_cr + i * 4, 4, nullptr);
			std::fill_n(m_g_cr + i * 4, 4, nullptr);

			const auto index = m_ir->CreateAnd(m_ir->CreateLShr(value, 28 - i * 4), 15);
			const auto src = m_ir->CreateGEP(m_mtocr_table, {m_ir->getInt32(0), m_ir->CreateShl(index, 2)});
			const auto dst = m_ir->CreateBitCast(m_ir->CreateStructGEP(nullptr, m_thread, m_cr - m_locals + i * 4), GetType<u8*>());
			Call(GetType<void>(), "llvm.memcpy.p0i8.p0i8.i32", dst, src, m_ir->getInt32(4), m_ir->getFalse());
		}
	}
}

void PPUTranslator::STDX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs));
}

void PPUTranslator::STWCX(ppu_opcode_t op)
{
	const auto bit = Call(GetType<bool>(), "__stwcx", m_thread, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs, 32));
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
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(Call(GetType<bool>(), m_pure_attr, "__subfze_get_ov", a, c));
}

void PPUTranslator::STDCX(ppu_opcode_t op)
{
	const auto bit = Call(GetType<bool>(), "__stdcx", m_thread, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs));
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
	const auto result = op.rs == op.rb ? static_cast<Value*>(m_ir->getInt64(0)) : m_ir->CreateXor(GetGpr(op.rs), GetGpr(op.rb));
	SetGpr(op.ra, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::MFSPR(ppu_opcode_t op)
{
	Value* result;
	switch (const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5))
	{
	case 0x001: // MFXER
		result = ZExt(RegLoad(m_cnt), GetType<u64>());
		result = m_ir->CreateOr(result, m_ir->CreateShl(ZExt(RegLoad(m_so), GetType<u64>()), 29));
		result = m_ir->CreateOr(result, m_ir->CreateShl(ZExt(RegLoad(m_ov), GetType<u64>()), 30));
		result = m_ir->CreateOr(result, m_ir->CreateShl(ZExt(RegLoad(m_ca), GetType<u64>()), 31));
		break;
	case 0x008: // MFLR
		result = RegLoad(m_lr);
		break;
	case 0x009: // MFCTR
		result = RegLoad(m_ctr);
		break;
	case 0x100:
		result = ZExt(RegLoad(m_vrsave));
		break;
	case 0x10C: // MFTB
		result = Call(GetType<u64>(), m_pure_attr, "__get_tb");
		break;
	case 0x10D: // MFTBU
		result = m_ir->CreateLShr(Call(GetType<u64>(), m_pure_attr, "__get_tb"), 32);
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
	Value* result;
	switch (const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5))
	{
	case 0x10C: // MFTB
		result = Call(GetType<u64>(), m_pure_attr, "__get_tb");
		break;
	case 0x10D: // MFTBU
		result = m_ir->CreateLShr(Call(GetType<u64>(), m_pure_attr, "__get_tb"), 32);
		break;
	default:
		result = Call(GetType<u64>(), fmt::format("__mftb_%u", n));
		break;
	}

	SetGpr(op.rd, result);
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
	const auto result = op.rs == op.rb ? static_cast<Value*>(m_ir->getInt64(-1)) : m_ir->CreateOr(GetGpr(op.rs), m_ir->CreateNot(GetGpr(op.rb)));
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
	SetGpr(op.rd, m_ir->CreateSelect(o, m_ir->getInt64(0), result));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::DIVWU(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra, 32);
	const auto b = GetGpr(op.rb, 32);
	const auto o = IsZero(b);
	const auto result = m_ir->CreateUDiv(a, m_ir->CreateSelect(o, m_ir->getInt32(0xffffffff), b));
	SetGpr(op.rd, m_ir->CreateSelect(o, m_ir->getInt32(0), result));
	if (op.rc) SetCrField(0, GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>());
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::MTSPR(ppu_opcode_t op)
{
	const auto value = GetGpr(op.rs);

	switch (const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5))
	{
	case 0x001: // MTXER
		RegStore(Trunc(m_ir->CreateLShr(value, 31), GetType<bool>()), m_ca);
		RegStore(Trunc(m_ir->CreateLShr(value, 30), GetType<bool>()), m_ov);
		RegStore(Trunc(m_ir->CreateLShr(value, 29), GetType<bool>()), m_so);
		RegStore(Trunc(value, GetType<u8>()), m_cnt);
		break;
	case 0x008: // MTLR
		RegStore(value, m_lr);
		break;
	case 0x009: // MTCTR
		RegStore(value, m_ctr);
		break;
	case 0x100:
		RegStore(Trunc(value), m_vrsave);
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
	SetGpr(op.rd, m_ir->CreateSelect(o, m_ir->getInt64(0), result));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::DIVW(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra, 32);
	const auto b = GetGpr(op.rb, 32);
	const auto o = m_ir->CreateOr(IsZero(b), m_ir->CreateAnd(m_ir->CreateICmpEQ(a, m_ir->getInt32(1 << 31)), IsOnes(b)));
	const auto result = m_ir->CreateSDiv(a, m_ir->CreateSelect(o, m_ir->getInt32(1 << 31), b));
	SetGpr(op.rd, m_ir->CreateSelect(o, m_ir->getInt32(0), result));
	if (op.rc) SetCrField(0, GetUndef<bool>(), GetUndef<bool>(), GetUndef<bool>());
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::LVLX(ppu_opcode_t op)
{
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	SetVr(op.vd, Call(GetType<u8[16]>(), "__lvlx", addr));
}

void PPUTranslator::LDBRX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u64>(), false));
}

void PPUTranslator::LSWX(ppu_opcode_t op)
{
	CompilationError("Unsupported instruction LSWX. Please report.");
	Call(GetType<void>(), "__lswx_not_supported", m_ir->getInt32(op.rd), RegLoad(m_cnt), op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb));
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
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	SetVr(op.vd, Call(GetType<u8[16]>(), "__lvrx", addr));
}

void PPUTranslator::LSWI(ppu_opcode_t op)
{
	Value* addr = op.ra ? GetGpr(op.ra) : m_ir->getInt64(0);
	u32 index = op.rb ? op.rb : 32;
	u32 reg = op.rd;

	while (index)
	{
		if (index > 3)
		{
			SetGpr(reg, ReadMemory(addr, GetType<u32>()));
			index -= 4;

			if (index)
			{
				addr = m_ir->CreateAdd(addr, m_ir->getInt64(4));
			}
		}
		else
		{
			Value* buf = nullptr;
			u32 i = 3;

			while (index)
			{
				const auto byte = m_ir->CreateShl(ZExt(ReadMemory(addr, GetType<u8>()), GetType<u32>()), i * 8);

				buf = buf ? m_ir->CreateOr(buf, byte) : byte;

				if (--index)
				{
					addr = m_ir->CreateAdd(addr, m_ir->getInt64(1));
					i--;
				}
			}

			SetGpr(reg, buf);
		}
		reg = (reg + 1) % 32;
	}
}

void PPUTranslator::LFSUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetFpr(op.frd, ReadMemory(addr, GetType<f32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::SYNC(ppu_opcode_t op)
{
	// sync: Full seq cst barrier
	// lwsync: Acq/Release barrier
	m_ir->CreateFence(op.l10 ? AtomicOrdering::AcquireRelease : AtomicOrdering::SequentiallyConsistent);
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
	CompilationError("Unsupported instruction STSWX. Please report.");
	Call(GetType<void>(), "__stswx_not_supported", m_ir->getInt32(op.rs), RegLoad(m_cnt), op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb));
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
	Value* addr = op.ra ? GetGpr(op.ra) : m_ir->getInt64(0);
	u32 index = op.rb ? op.rb : 32;
	u32 reg = op.rd;

	while (index)
	{
		if (index > 3)
		{
			WriteMemory(addr, GetGpr(reg, 32));
			index -= 4;

			if (index)
			{
				addr = m_ir->CreateAdd(addr, m_ir->getInt64(4));
			}
		}
		else
		{
			Value* buf = GetGpr(reg, 32);

			while (index)
			{
				WriteMemory(addr, Trunc(m_ir->CreateLShr(buf, 24), GetType<u8>()));

				if (--index)
				{
					buf = m_ir->CreateShl(buf, 8);
					addr = m_ir->CreateAdd(addr, m_ir->getInt64(1));
				}
			}
		}
		reg = (reg + 1) % 32;
	}
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
	SetCarry(m_ir->CreateAnd(m_ir->CreateICmpSLT(shift_arg, m_ir->getInt64(0)), m_ir->CreateICmpNE(arg_ext, m_ir->CreateShl(res_128, shift_num))));
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
	Call(GetType<void>(), "llvm.memset.p0i8.i32", ptr, m_ir->getInt8(0), m_ir->getInt32(128), m_ir->getTrue());
}

void PPUTranslator::LWZ(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<u32>()));
}

void PPUTranslator::LWZU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetGpr(op.rd, ReadMemory(addr, GetType<u32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LBZ(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<u8>()));
}

void PPUTranslator::LBZU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetGpr(op.rd, ReadMemory(addr, GetType<u8>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STW(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto value = GetGpr(op.rs, 32);
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm;
	WriteMemory(addr, value);

	//Insomniac engine v3 & v4 (newer R&C, Fuse, Resitance 3)
	if (auto ci = llvm::dyn_cast<ConstantInt>(value))
	{
		if (ci->getZExtValue() == 0xAAAAAAAA)
		{
			Call(GetType<void>(), "__resupdate", addr, m_ir->getInt32(128));
		}
	}
}

void PPUTranslator::STWU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	WriteMemory(addr, GetGpr(op.rs, 32));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STB(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetGpr(op.rs, 8));
}

void PPUTranslator::STBU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	WriteMemory(addr, GetGpr(op.rs, 8));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LHZ(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<u16>()));
}

void PPUTranslator::LHZU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetGpr(op.rd, ReadMemory(addr, GetType<u16>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LHA(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, SExt(ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<s16>()), GetType<s64>()));
}

void PPUTranslator::LHAU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetGpr(op.rd, SExt(ReadMemory(addr, GetType<s16>()), GetType<s64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STH(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetGpr(op.rs, 16));
}

void PPUTranslator::STHU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	WriteMemory(addr, GetGpr(op.rs, 16));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LMW(ppu_opcode_t op)
{
	for (u32 i = 0; i < 32 - op.rd; i++)
	{
		SetGpr(i + op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(m_ir->getInt64(op.simm16 + i * 4), GetGpr(op.ra)) : m_ir->getInt64(op.simm16 + i * 4), GetType<u32>()));
	}
}

void PPUTranslator::STMW(ppu_opcode_t op)
{
	for (u32 i = 0; i < 32 - op.rs; i++)
	{
		WriteMemory(op.ra ? m_ir->CreateAdd(m_ir->getInt64(op.simm16 + i * 4), GetGpr(op.ra)) : m_ir->getInt64(op.simm16 + i * 4), GetGpr(i + op.rs, 32));
	}
}

void PPUTranslator::LFS(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetFpr(op.frd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<f32>()));
}

void PPUTranslator::LFSU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetFpr(op.frd, ReadMemory(addr, GetType<f32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LFD(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetFpr(op.frd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<f64>()));
}

void PPUTranslator::LFDU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetFpr(op.frd, ReadMemory(addr, GetType<f64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STFS(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetFpr(op.frs, 32));
}

void PPUTranslator::STFSU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	WriteMemory(addr, GetFpr(op.frs, 32));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STFD(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetFpr(op.frs));
}

void PPUTranslator::STFDU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && m_rel->type == 4)
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	WriteMemory(addr, GetFpr(op.frs));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LD(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.ds << 2);

	if (m_rel && m_rel->type == 57)
	{
		imm = m_ir->CreateAnd(SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), ~3);
		m_rel = nullptr;
	}

	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<u64>()));
}

void PPUTranslator::LDU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.ds << 2);

	if (m_rel && m_rel->type == 57)
	{
		imm = m_ir->CreateAnd(SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), ~3);
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetGpr(op.rd, ReadMemory(addr, GetType<u64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LWA(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.ds << 2);

	if (m_rel && m_rel->type == 57)
	{
		imm = m_ir->CreateAnd(SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), ~3);
		m_rel = nullptr;
	}

	SetGpr(op.rd, SExt(ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<s32>())));
}

void PPUTranslator::STD(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.ds << 2);

	if (m_rel && m_rel->type == 57)
	{
		imm = m_ir->CreateAnd(SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), ~3);
		m_rel = nullptr;
	}

	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetGpr(op.rs));
}

void PPUTranslator::STDU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.ds << 2);

	if (m_rel && m_rel->type == 57)
	{
		imm = m_ir->CreateAnd(SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), ~3);
		m_rel = nullptr;
	}

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
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
	const auto result = m_ir->CreateFDiv(ConstantFP::get(GetType<f32>(), 1.0), b);
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
	const auto result = m_ir->CreateFPTrunc(m_ir->CreateFNeg(m_ir->CreateFSub(m_ir->CreateFMul(a, c), b)), GetType<f32>());
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

	if (op.rc) SetCrFieldFPCC(1);
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

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::MTFSFI(ppu_opcode_t op)
{
	CompilationError("MTFSFI");

	SetFPSCRBit(op.crfd * 4 + 0, m_ir->getInt1((op.i & 8) != 0), false);
	if (op.crfd != 0) SetFPSCRBit(op.crfd * 4 + 1, m_ir->getInt1((op.i & 4) != 0), false);
	if (op.crfd != 0) SetFPSCRBit(op.crfd * 4 + 2, m_ir->getInt1((op.i & 2) != 0), false);
	SetFPSCRBit(op.crfd * 4 + 3, m_ir->getInt1((op.i & 1) != 0), false);

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::MFFS(ppu_opcode_t op)
{
	ppu_log.warning("LLVM: [0x%08x] Warning: MFFS", m_addr + (m_reloc ? m_reloc->addr : 0));

	Value* result = m_ir->getInt64(0);

	for (u32 i = 16; i < 20; i++)
	{
		result = m_ir->CreateOr(result, m_ir->CreateShl(ZExt(RegLoad(m_fc[i]), GetType<u64>()), i ^ 31));
	}

	SetFpr(op.frd, result);

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::MTFSF(ppu_opcode_t op)
{
	ppu_log.warning("LLVM: [0x%08x] Warning: MTFSF", m_addr + (m_reloc ? m_reloc->addr : 0));

	const auto value = GetFpr(op.frb, 32, true);

	for (u32 i = 16; i < 20; i++)
	{
		if (i != 1 && i != 2 && (op.flm & (128 >> (i / 4))) != 0)
		{
			SetFPSCRBit(i, Trunc(m_ir->CreateLShr(value, i ^ 31), GetType<bool>()), false);
		}
	}

	if (op.rc) SetCrFieldFPCC(1);
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
	const auto xormask = m_ir->CreateSExt(m_ir->CreateFCmpOGE(b, ConstantFP::get(GetType<f64>(), std::exp2l(31.))), GetType<s32>());

	// fix result saturation (0x80000000 -> 0x7fffffff)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s32>(), "llvm.x86.sse2.cvtsd2si", m_ir->CreateInsertElement(GetUndef<f64[2]>(), b, u64{0}))));

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
	const auto xormask = m_ir->CreateSExt(m_ir->CreateFCmpOGE(b, ConstantFP::get(GetType<f64>(), std::exp2l(31.))), GetType<s32>());

	// fix result saturation (0x80000000 -> 0x7fffffff)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s32>(), "llvm.x86.sse2.cvttsd2si", m_ir->CreateInsertElement(GetUndef<f64[2]>(), b, u64{0}))));
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

	if (op.rc) SetCrFieldFPCC(1);
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
	const auto result = m_ir->CreateFDiv(ConstantFP::get(GetType<f32>(), 1.0), Call(GetType<f32>(), "llvm.sqrt.f32", b));
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
	const auto result = m_ir->CreateFNeg(m_ir->CreateFSub(m_ir->CreateFMul(a, c), b));
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

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::FMR(ppu_opcode_t op)
{
	SetFpr(op.frd, GetFpr(op.frb));

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::FNABS(ppu_opcode_t op)
{
	SetFpr(op.frd, m_ir->CreateFNeg(Call(GetType<f64>(), "llvm.fabs.f64", GetFpr(op.frb))));

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::FABS(ppu_opcode_t op)
{
	SetFpr(op.frd, Call(GetType<f64>(), "llvm.fabs.f64", GetFpr(op.frb)));

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::FCTID(ppu_opcode_t op)
{
	const auto b = GetFpr(op.frb);
	const auto xormask = m_ir->CreateSExt(m_ir->CreateFCmpOGE(b, ConstantFP::get(GetType<f64>(), std::exp2l(63.))), GetType<s64>());

	// fix result saturation (0x8000000000000000 -> 0x7fffffffffffffff)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s64>(), "llvm.x86.sse2.cvtsd2si64", m_ir->CreateInsertElement(GetUndef<f64[2]>(), b, u64{0}))));

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
	const auto xormask = m_ir->CreateSExt(m_ir->CreateFCmpOGE(b, ConstantFP::get(GetType<f64>(), std::exp2l(63.))), GetType<s64>());

	// fix result saturation (0x8000000000000000 -> 0x7fffffffffffffff)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s64>(), "llvm.x86.sse2.cvttsd2si64", m_ir->CreateInsertElement(GetUndef<f64[2]>(), b, u64{0}))));
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
	FlushRegisters();
	Call(GetType<void>(), "__error", m_thread, GetAddr(), m_ir->getInt32(op.opcode))->setTailCallKind(llvm::CallInst::TCK_Tail);
	m_ir->CreateRetVoid();
}


Value* PPUTranslator::GetGpr(u32 r, u32 num_bits)
{
	return m_ir->CreateTrunc(RegLoad(m_gpr[r]), m_ir->getIntNTy(num_bits));
}

void PPUTranslator::SetGpr(u32 r, Value* value)
{
	RegStore(m_ir->CreateZExt(value, GetType<u64>()), m_gpr[r]);
}

Value* PPUTranslator::GetFpr(u32 r, u32 bits, bool as_int)
{
	const auto value = RegLoad(m_fpr[r]);

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
		val->getType() == GetType<s32>() ? m_ir->CreateBitCast(SExt(val), GetType<f64>()) :
		val->getType() == GetType<s64>() ? m_ir->CreateBitCast(val, GetType<f64>()) :
		val->getType() == GetType<f32>() ? m_ir->CreateFPExt(val, GetType<f64>()) : val;

	RegStore(f64_val, m_fpr[r]);
}

Value* PPUTranslator::GetVr(u32 vr, VrType type)
{
	const auto value = RegLoad(m_vr[vr]);

	switch (type)
	{
	case VrType::vi32: return m_ir->CreateBitCast(value, GetType<u32[4]>());
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
			value = SExt(value, ScaleType(type, 7 - s32(std::log2(+size))));
		}
		else if (size == 256 || size == 512)
		{
			// Truncate big vectors
			value = Trunc(value, ScaleType(type, 7 - s32(std::log2(+size))));
		}
	}

	RegStore(value, m_vr[vr]);
}

Value* PPUTranslator::GetCrb(u32 crb)
{
	return RegLoad(m_cr[crb]);
}

void PPUTranslator::SetCrb(u32 crb, Value* value)
{
	RegStore(value, m_cr[crb]);
}

void PPUTranslator::SetCrField(u32 group, Value* lt, Value* gt, Value* eq, Value* so)
{
	SetCrb(group * 4 + 0, lt ? lt : GetUndef<bool>());
	SetCrb(group * 4 + 1, gt ? gt : GetUndef<bool>());
	SetCrb(group * 4 + 2, eq ? eq : GetUndef<bool>());
	SetCrb(group * 4 + 3, so ? so : RegLoad(m_so));
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

void PPUTranslator::SetCrFieldFPCC(u32 n)
{
	SetCrField(n, GetFPSCRBit(16), GetFPSCRBit(17), GetFPSCRBit(18), GetFPSCRBit(19));
}

void PPUTranslator::SetFPCC(Value* lt, Value* gt, Value* eq, Value* un, bool set_cr)
{
	SetFPSCRBit(16, lt, false);
	SetFPSCRBit(17, gt, false);
	SetFPSCRBit(18, eq, false);
	SetFPSCRBit(19, un, false);
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
	//m_ir->CreateStore(value, m_fpscr_fr);
}

void PPUTranslator::SetFPSCR_FI(Value* value)
{
	//m_ir->CreateStore(value, m_fpscr_fi);
	//SetFPSCRException(m_fpscr_xx, value);
}

void PPUTranslator::SetFPSCRException(Value* ptr, Value* value)
{
	//m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(ptr), value), ptr);
	//m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(m_fpscr_fx), value), m_fpscr_fx);
}

Value* PPUTranslator::GetFPSCRBit(u32 n)
{
	//if (n == 1 && m_fpscr[24])
	//{
	//	// Floating-Point Enabled Exception Summary (FEX) 24-29
	//	Value* value = m_ir->CreateLoad(m_fpscr[24]);
	//	for (u32 i = 25; i <= 29; i++) value = m_ir->CreateOr(value, m_ir->CreateLoad(m_fpscr[i]));
	//	return value;
	//}

	//if (n == 2 && m_fpscr[7])
	//{
	//	// Floating-Point Invalid Operation Exception Summary (VX) 7-12, 21-23
	//	Value* value = m_ir->CreateLoad(m_fpscr[7]);
	//	for (u32 i = 8; i <= 12; i++) value = m_ir->CreateOr(value, m_ir->CreateLoad(m_fpscr[i]));
	//	for (u32 i = 21; i <= 23; i++) value = m_ir->CreateOr(value, m_ir->CreateLoad(m_fpscr[i]));
	//	return value;
	//}

	if (n < 16 || n > 19)
	{
		return nullptr; // ???
	}

	// Get bit
	const auto value = RegLoad(m_fc[n]);

	//if (n == 0 || (n >= 3 && n <= 12) || (n >= 21 && n <= 23))
	//{
	//	// Clear FX or exception bits
	//	m_ir->CreateStore(m_ir->getFalse(), m_fpscr[n]);
	//}

	return value;
}

void PPUTranslator::SetFPSCRBit(u32 n, Value* value, bool update_fx)
{
	if (n < 16 || n > 19)
	{
		//CompilationError("SetFPSCRBit(): inaccessible bit " + std::to_string(n));
		return; // ???
	}

	//if (update_fx)
	//{
	//	if ((n >= 3 && n <= 12) || (n >= 21 && n <= 23))
	//	{
	//		// Update FX bit if necessary
	//		m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(m_fpscr_fx), value), m_fpscr_fx);
	//	}
	//}

	//if (n >= 24 && n <= 28) CompilationError("SetFPSCRBit: exception enable bit " + std::to_string(n));
	//if (n == 29) CompilationError("SetFPSCRBit: NI bit");
	//if (n >= 30) CompilationError("SetFPSCRBit: RN bit");

	// Store the bit
	RegStore(value, m_fc[n]);
}

Value* PPUTranslator::GetCarry()
{
	return RegLoad(m_ca);
}

void PPUTranslator::SetCarry(Value* bit)
{
	RegStore(bit, m_ca);
}

void PPUTranslator::SetOverflow(Value* bit)
{
	RegStore(bit, m_ov);
	RegStore(m_ir->CreateOr(RegLoad(m_so), bit), m_so);
}

void PPUTranslator::SetSat(Value* bit)
{
	RegStore(m_ir->CreateOr(RegLoad(m_sat), bit), m_sat);
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

void PPUTranslator::Trap()
{
	Call(GetType<void>(), "__trap", m_thread, GetAddr())->setTailCallKind(llvm::CallInst::TCK_Tail);
	m_ir->CreateRetVoid();
}

Value* PPUTranslator::CheckBranchCondition(u32 bo, u32 bi)
{
	const bool bo0 = (bo & 0x10) != 0;
	const bool bo1 = (bo & 0x08) != 0;
	const bool bo2 = (bo & 0x04) != 0;
	const bool bo3 = (bo & 0x02) != 0;

	// Decrement counter if necessary
	const auto ctr = bo2 ? nullptr : m_ir->CreateSub(RegLoad(m_ctr), m_ir->getInt64(1));

	// Store counter if necessary
	if (ctr) RegStore(ctr, m_ctr);

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

MDNode* PPUTranslator::CheckBranchProbability(u32 bo)
{
	const bool bo0 = (bo & 0x10) != 0;
	const bool bo1 = (bo & 0x08) != 0;
	const bool bo2 = (bo & 0x04) != 0;
	const bool bo3 = (bo & 0x02) != 0;
	const bool bo4 = (bo & 0x01) != 0;

	if ((bo0 && bo1) || (bo2 && bo3))
	{
		return bo4 ? m_md_likely : m_md_unlikely;
	}

	return nullptr;
}

#endif
