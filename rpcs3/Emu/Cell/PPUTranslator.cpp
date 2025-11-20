#ifdef LLVM_AVAILABLE

#include "Emu/system_config.h"
#include "Emu/Cell/Common.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "PPUTranslator.h"
#include "PPUThread.h"
#include "SPUThread.h"

#include "util/types.hpp"
#include "util/endian.hpp"
#include "util/logs.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"
#include <algorithm>
#include <unordered_set>
#include <span>

#ifdef ARCH_ARM64
#include "Emu/CPU/Backends/AArch64/AArch64JIT.h"
#include "Emu/IdManager.h"
#include "Utilities/ppu_patch.h"
#endif

using namespace llvm;

const ppu_decoder<PPUTranslator> s_ppu_decoder;
extern const ppu_decoder<ppu_itype> g_ppu_itype;
extern const ppu_decoder<ppu_iname> g_ppu_iname;

PPUTranslator::PPUTranslator(LLVMContext& context, Module* _module, const ppu_module<lv2_obj>& info, ExecutionEngine& engine)
	: cpu_translator(_module, false)
	, m_info(info)
	, m_pure_attr()
{
	// Bind context
	cpu_translator::initialize(context, engine);

	// Initialize transform passes
	clear_transforms();

#ifdef ARCH_ARM64
	{
		// Base reg table definition
		// Assume all functions named __0x... are PPU functions and take the m_exec as the first arg
		std::vector<std::pair<std::string, aarch64::gpr>> base_reg_lookup = {
			{ "__0x", aarch64::x20 }, // PPU blocks
			{ "__indirect", aarch64::x20 }, // Indirect jumps
			{ "ppu_", aarch64::x19 }, // Fixed JIT helpers (e.g ppu_gateway)
			{ "__", aarch64::x19 }    // Probably link table entries
		};

		// Build list of imposter functions built by the patch manager.
		g_fxo->need<ppu_patch_block_registry_t>();
		std::vector<std::string> faux_functions_list;
		for (const auto& a : g_fxo->get<ppu_patch_block_registry_t>().block_addresses)
		{
			faux_functions_list.push_back(fmt::format("__0x%x", a));
		}

		aarch64::GHC_frame_preservation_pass::config_t config =
		{
			.debug_info = false,         // Set to "true" to insert debug frames on x27
			.use_stack_frames = false,   // We don't need this since the PPU GW allocates global scratch on the stack
			.hypervisor_context_offset = ::offset32(&ppu_thread::hv_ctx),
			.exclusion_callback = {},    // Unused, we don't have special exclusion functions on PPU
			.base_register_lookup = base_reg_lookup,
			.faux_function_list = std::move(faux_functions_list)
		};

		// Create transform pass
		std::unique_ptr<translator_pass> ghc_fixup_pass = std::make_unique<aarch64::GHC_frame_preservation_pass>(config);

		// Register it
		register_transform_pass(ghc_fixup_pass);
	}
#endif

	reset_transforms();

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
	thread_struct.insert(thread_struct.end(), 1, GetType<bool>()); // nj
	thread_struct.emplace_back(ArrayType::get(GetType<char>(), 3)); // Padding
	thread_struct.insert(thread_struct.end(), 1, GetType<u32[4]>()); // sat
	thread_struct.insert(thread_struct.end(), 1, GetType<u32>()); // jm_mask

	m_thread_type = StructType::create(m_context, thread_struct, "context_t");

	const auto md_name = MDString::get(m_context, "branch_weights");
	const auto md_low = ValueAsMetadata::get(ConstantInt::get(GetType<u32>(), 1));
	const auto md_high = ValueAsMetadata::get(ConstantInt::get(GetType<u32>(), 666));

	// Metadata for branch weights
	m_md_likely = MDTuple::get(m_context, {md_name, md_high, md_low});
	m_md_unlikely = MDTuple::get(m_context, {md_name, md_low, md_high});

	// Sort relevant relocations (TODO)
	const auto caddr = m_info.segs[0].addr;
	const auto cend = caddr + m_info.segs[0].size;

	for (const auto& rel : m_info.get_relocs())
	{
		if (rel.addr >= caddr && rel.addr < cend)
		{
			// Check relocation type
			switch (rel.type)
			{

			// Ignore relative relocations, they are handled in emitted code
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
			default: break;
			}

			// Align relocation address (TODO)
			if (!m_relocs.emplace(rel.addr & ~3, &rel).second)
			{
				ppu_log.error("Relocation repeated at 0x%x (%u)", rel.addr, rel.type);
			}
		}
	}

	if (m_info.is_relocatable)
	{
		m_reloc = &m_info.segs[0];
	}

	const auto nan_v = v128::from32p(0x7FC00000u);
	nan_vec4 = make_const_vector(nan_v, get_type<f32[4]>());
}

PPUTranslator::~PPUTranslator()
{
}

Type* PPUTranslator::GetContextType()
{
	return m_thread_type;
}

u32 ppu_get_far_jump(u32 pc);
bool ppu_test_address_may_be_mmio(std::span<const be_t<u32>> insts);

Function* PPUTranslator::Translate(const ppu_function& info)
{
	// Instruction address is (m_addr + base)
	const u64 base = m_reloc ? m_reloc->addr : 0;
	m_addr = info.addr - base;
	m_attr = m_info.attr;

	m_function = m_module->getFunction(fmt::format("__0x%x", m_addr));

	std::fill(std::begin(m_globals), std::end(m_globals), nullptr);
	std::fill(std::begin(m_locals), std::end(m_locals), nullptr);

	IRBuilder<> irb(BasicBlock::Create(m_context, "__entry", m_function));
	m_ir = &irb;

	// Don't emit check in small blocks without terminator
	bool need_check = info.size >= 16;

	for (u64 addr = m_addr; addr < m_addr + info.size; addr += 4)
	{
		const u32 op = *ensure(m_info.get_ptr<u32>(::narrow<u32>(addr + base)));

		switch (g_ppu_itype.decode(op))
		{
		case ppu_itype::UNK:
		case ppu_itype::ECIWX:
		case ppu_itype::ECOWX:
		case ppu_itype::TD:
		case ppu_itype::TDI:
		case ppu_itype::TW:
		case ppu_itype::TWI:
		case ppu_itype::B:
		case ppu_itype::BC:
		case ppu_itype::BCCTR:
		case ppu_itype::BCLR:
		case ppu_itype::SC:
		{
			need_check = true;
			break;
		}
		default:
		{
			break;
		}
		}
	}

	m_thread = m_function->getArg(1);
	m_base = m_function->getArg(3);
	m_exec = m_function->getArg(0);
	m_seg0 = m_function->getArg(2);

	m_gpr[0] = m_function->getArg(4);
	m_gpr[1] = m_function->getArg(5);
	m_gpr[2] = m_function->getArg(6);

	const auto body = BasicBlock::Create(m_context, "__body", m_function);

	//Call(GetType<void>(), "__trace", GetAddr());
	if (need_check)
	{
		// Check status register in the entry block
		auto ptr = llvm::dyn_cast<GetElementPtrInst>(m_ir->CreateStructGEP(m_thread_type, m_thread, 1));
		assert(ptr->getResultElementType() == GetType<u32>());
		const auto vstate = m_ir->CreateLoad(ptr->getResultElementType(), ptr, true);
		const auto vcheck = BasicBlock::Create(m_context, "__test", m_function);
		m_ir->CreateCondBr(m_ir->CreateIsNull(vstate), body, vcheck, m_md_likely);

		m_ir->SetInsertPoint(vcheck);

		// Raise wait flag as soon as possible
		m_ir->CreateAtomicRMW(llvm::AtomicRMWInst::Or, ptr, m_ir->getInt32((+cpu_flag::wait).operator u32()), llvm::MaybeAlign{4}, llvm::AtomicOrdering::AcquireRelease);

		// Create tail call to the check function
		Call(GetType<void>(), "__check", m_thread, GetAddr())->setTailCall();
		m_ir->CreateRetVoid();
	}
	else
	{
		m_ir->CreateBr(body);
	}

	m_ir->SetInsertPoint(body);

	// Process blocks
	const auto block = std::make_pair(info.addr, info.size);
	{
		// Optimize BLR (prefetch LR)
		if (*ensure(m_info.get_ptr<u32>(block.first + block.second - 4)) == ppu_instructions::BLR())
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

			// Reset MMIO hint
			m_may_be_mmio = true;

			const u32 op = *ensure(m_info.get_ptr<u32>(::narrow<u32>(m_addr + base)));

			(this->*(s_ppu_decoder.decode(op)))({op});

			if (m_rel)
			{
				// This is very bad. m_rel is normally set to nullptr after a relocation is handled (so it wasn't)
				ppu_log.error("LLVM: [0x%x] Unsupported relocation(%u) in '%s' (opcode=0x%x '%s'). Please report.", rel_found->first, m_rel->type, m_info.name, op, g_ppu_iname.decode(op));
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

	run_transforms(*m_function);
	return m_function;
}

Function* PPUTranslator::GetSymbolResolver(const ppu_module<lv2_obj>& info)
{
	ensure(m_module->getFunction("__resolve_symbols") == nullptr);
	ensure(info.jit_bounds);

	m_function = cast<Function>(m_module->getOrInsertFunction("__resolve_symbols", FunctionType::get(get_type<void>(), { get_type<u8*>(), get_type<u64>() }, false)).getCallee());

	IRBuilder<> irb(BasicBlock::Create(m_context, "__entry", m_function));
	m_ir = &irb;

	// Instruction address is (m_addr + base)
	const u64 base = m_reloc ? m_reloc->addr : 0;

	m_exec = m_function->getArg(0);
	m_seg0 = m_function->getArg(1);

	const auto ftype = FunctionType::get(get_type<void>(), {
		get_type<u8*>(), // Exec base
		m_ir->getPtrTy(), // PPU context
		get_type<u64>(), // Segment address (for PRX)
		get_type<u8*>(), // Memory base
		get_type<u64>(), // r0
		get_type<u64>(), // r1
		get_type<u64>(), // r2
		}, false);

	// Store function addresses in PPU jumptable using internal resolving instead of patching it externally.
	// Because, LLVM processed it extremely slow. (regression)
	// This is made in loop instead of inlined because it took tremendous amount of time to compile.

	std::vector<u32> vec_addrs;

	// Create an array of function pointers
	std::vector<llvm::Constant*> functions;

	for (const auto& f : info.get_funcs(false, true))
	{
		if (!f.size)
		{
			continue;
		}

		if (std::count(info.excluded_funcs.begin(), info.excluded_funcs.end(), f.addr))
		{
			// Excluded function (possibly patched)
			continue;
		}

		vec_addrs.push_back(static_cast<u32>(f.addr - base));
		functions.push_back(cast<Function>(m_module->getOrInsertFunction(fmt::format("__0x%x", f.addr - base), ftype).getCallee()));
	}

	if (vec_addrs.empty())
	{
		// Possible special case for no functions (allowing the do-while optimization)
		m_ir->CreateRetVoid();
		run_transforms(*m_function);
		return m_function;
	}

	const auto addr_array_type = ArrayType::get(get_type<u32>(), vec_addrs.size());
	const auto addr_array = new GlobalVariable(*m_module, addr_array_type, false, GlobalValue::PrivateLinkage, ConstantDataArray::get(m_context, vec_addrs));

	// Create an array of function pointers
	const auto func_table_type = ArrayType::get(m_ir->getPtrTy(), functions.size());
	const auto init_func_table = ConstantArray::get(func_table_type, functions);
	const auto func_table = new GlobalVariable(*m_module, func_table_type, false, GlobalVariable::PrivateLinkage, init_func_table);

	const auto loop_block = BasicBlock::Create(m_context, "__loop", m_function);
	const auto after_loop = BasicBlock::Create(m_context, "__after_loop", m_function);

	m_ir->CreateBr(loop_block);
	m_ir->SetInsertPoint(loop_block);

	const auto init_index_value = m_ir->getInt64(0);

	// Loop body
	const auto body_block = BasicBlock::Create(m_context, "__body", m_function);

	m_ir->CreateBr(body_block); // As do-while because vec_addrs is known to be more than 0
	m_ir->SetInsertPoint(body_block);

	const auto index_value = m_ir->CreatePHI(get_type<u64>(), 2);
	index_value->addIncoming(init_index_value, loop_block);

	auto ptr_inst = dyn_cast<GetElementPtrInst>(m_ir->CreateGEP(addr_array->getValueType(), addr_array, {m_ir->getInt64(0), index_value}));
	assert(ptr_inst->getResultElementType() == get_type<u32>());

	const auto func_pc = ZExt(m_ir->CreateLoad(ptr_inst->getResultElementType(), ptr_inst), get_type<u64>());

	ptr_inst = dyn_cast<GetElementPtrInst>(m_ir->CreateGEP(func_table->getValueType(), func_table, {m_ir->getInt64(0), index_value}));
	assert(ptr_inst->getResultElementType() == m_ir->getPtrTy());

	const auto faddr = m_ir->CreateLoad(ptr_inst->getResultElementType(), ptr_inst);
	const auto faddr_int = m_ir->CreatePtrToInt(faddr, get_type<uptr>());
	const auto pos_32 = m_reloc ? m_ir->CreateAdd(func_pc, m_seg0) : func_pc;
	const auto pos = m_ir->CreateShl(pos_32, 1);
	const auto ptr = m_ir->CreatePtrAdd(m_exec, pos);

	const auto seg_base_ptr = m_ir->CreatePtrAdd(m_exec, m_ir->getInt64(vm::g_exec_addr_seg_offset));
	const auto seg_pos = m_ir->CreateLShr(pos_32, 1);
	const auto seg_ptr = m_ir->CreatePtrAdd(seg_base_ptr, seg_pos);
	const auto seg_val = m_ir->CreateTrunc(m_ir->CreateLShr(m_seg0, 13), get_type<u16>());

	// Store to jumptable
	m_ir->CreateStore(faddr_int, ptr);
	m_ir->CreateStore(seg_val, seg_ptr);

	// Increment index and branch back to loop
	const auto post_add = m_ir->CreateAdd(index_value, m_ir->getInt64(1));
	index_value->addIncoming(post_add, body_block);

	Value* index_check = m_ir->CreateICmpULT(post_add, m_ir->getInt64(vec_addrs.size()));
	m_ir->CreateCondBr(index_check, body_block, after_loop);

	// Set insertion point to afterloop_block
	m_ir->SetInsertPoint(after_loop);

	m_ir->CreateRetVoid();

	run_transforms(*m_function);
	return m_function;
}

Value* PPUTranslator::VecHandleNan(Value* val)
{
	const auto is_nan = m_ir->CreateFCmpUNO(val, val);

	val = m_ir->CreateSelect(is_nan, nan_vec4, val);

	return val;
}

Value* PPUTranslator::VecHandleDenormal(Value* val)
{
	const auto type = val->getType();
	const auto value = bitcast(val, GetType<u32[4]>());

	const auto mask = SExt(m_ir->CreateICmpEQ(m_ir->CreateAnd(value, Broadcast(RegLoad(m_jm_mask), 4)), ConstantAggregateZero::get(value->getType())), GetType<s32[4]>());
	const auto nz = m_ir->CreateLShr(mask, 1);
	const auto result = m_ir->CreateAnd(m_ir->CreateNot(nz), value);

	return bitcast(result, type);
}

Value* PPUTranslator::VecHandleResult(Value* val)
{
	val = g_cfg.core.ppu_fix_vnan ? VecHandleNan(val) : val;
	val = g_cfg.core.ppu_llvm_nj_fixup ? VecHandleDenormal(val) : val;
	return val;
}

Value* PPUTranslator::GetAddr(u64 _add)
{
	if (m_reloc)
	{
		// Load segment address from global variable, compute actual instruction address
		return m_ir->CreateAdd(m_ir->getInt64(m_addr + _add), m_seg0);
	}

	return m_ir->getInt64(m_addr + _add);
}

Type* PPUTranslator::ScaleType(Type* type, s32 pow2)
{
	ensure(type->getScalarType()->isIntegerTy());
	ensure(pow2 > -32 && pow2 < 32);

	uint scaled = type->getScalarSizeInBits();
	ensure((scaled & (scaled - 1)) == 0);

	if (pow2 > 0)
	{
		scaled <<= pow2;
	}
	else if (pow2 < 0)
	{
		scaled >>= -pow2;
	}

	ensure(scaled);
	const auto new_type = m_ir->getIntNTy(scaled);
	const auto vec_type = dyn_cast<FixedVectorType>(type);
	return vec_type ? VectorType::get(new_type, vec_type->getNumElements(), false) : cast<Type>(new_type);
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
	const auto type = m_function->getFunctionType();
	const auto block = m_ir->GetInsertBlock();

	FunctionCallee callee;

	auto seg0 = m_seg0;

	if (!indirect)
	{
		const u64 base = m_reloc ? m_reloc->addr : 0;
		const u32 caddr = m_info.segs[0].addr;
		const u32 cend = caddr + m_info.segs[0].size - 1;
		const u64 _target = target + base;

		if (_target >= u32{umax})
		{
			Call(GetType<void>(), "__error", m_thread, GetAddr(), m_ir->getInt32(*ensure(m_info.get_ptr<u32>(::narrow<u32>(m_addr + base)))));
			m_ir->CreateRetVoid();
			return;
		}
		else if (_target >= caddr && _target <= cend)
		{
			u32 target_last = static_cast<u32>(_target);

			std::unordered_set<u32> passed_targets{target_last};

			// Try to follow unconditional branches as long as there is no infinite loop
			while (target_last != _target)
			{
				const ppu_opcode_t op{*ensure(m_info.get_ptr<u32>(target_last))};
				const ppu_itype::type itype = g_ppu_itype.decode(op.opcode);

				if (((itype == ppu_itype::BC && (op.bo & 0x14) == 0x14) || itype == ppu_itype::B) && !op.lk)
				{
					const u32 new_target = (op.aa ? 0 : target_last) + (itype == ppu_itype::B ? +op.bt24 : +op.bt14);

					if (target_last >= caddr && target_last <= cend)
					{
						if (passed_targets.emplace(new_target).second)
						{
							// Ok
							target_last = new_target;
							continue;
						}

						// Infinite loop detected
						target_last = static_cast<u32>(_target);
					}

					// Odd destination
				}
				else if (itype == ppu_itype::BCLR && (op.bo & 0x14) == 0x14 && !op.lk)
				{
					// Special case: empty function
					// In this case the branch can be treated as BCLR because previous CIA does not matter
					indirect = RegLoad(m_lr);
				}

				break;
			}

			if (!indirect)
			{
				callee = m_module->getOrInsertFunction(fmt::format("__0x%x", target_last - base), type);
				cast<Function>(callee.getCallee())->setCallingConv(CallingConv::GHC);

				if (g_cfg.core.ppu_prof)
				{
					m_ir->CreateStore(m_ir->getInt32(target_last), m_ir->CreateStructGEP(m_thread_type, m_thread, static_cast<uint>(&m_cia - m_locals)));
				}
			}
		}
		else
		{
			indirect = m_reloc ? m_ir->CreateAdd(m_ir->getInt64(target), seg0) : m_ir->getInt64(target);
		}
	}

	if (indirect)
	{
		m_ir->CreateStore(Trunc(indirect, GetType<u32>()), m_ir->CreateStructGEP(m_thread_type, m_thread, static_cast<uint>(&m_cia - m_locals)));

		// Try to optimize
		if (auto inst = dyn_cast_or_null<Instruction>(indirect))
		{
			if (auto next = inst->getNextNode())
			{
				m_ir->SetInsertPoint(next);
			}
		}

		const auto pos = m_ir->CreateShl(indirect, 1);
		const auto ptr = m_ir->CreatePtrAdd(m_exec, pos);
		const auto val = m_ir->CreateLoad(get_type<u64>(), ptr);
		callee = FunctionCallee(type, m_ir->CreateIntToPtr(val, m_ir->getPtrTy()));

		// Load new segment address
		const auto seg_base_ptr = m_ir->CreatePtrAdd(m_exec, m_ir->getInt64(vm::g_exec_addr_seg_offset));
		const auto seg_pos = m_ir->CreateLShr(indirect, 1);
		const auto seg_ptr = m_ir->CreatePtrAdd(seg_base_ptr, seg_pos);
		const auto seg_val = m_ir->CreateZExt(m_ir->CreateLoad(get_type<u16>(), seg_ptr), get_type<u64>());
		seg0 = m_ir->CreateShl(seg_val, 13);
	}

	m_ir->SetInsertPoint(block);
	const auto c = m_ir->CreateCall(callee, {m_exec, m_thread, seg0, m_base, GetGpr(0), GetGpr(1), GetGpr(2)});
	c->setTailCallKind(llvm::CallInst::TCK_Tail);
	c->setCallingConv(CallingConv::GHC);
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
	m_globals[index] = m_ir->CreateStructGEP(m_thread_type, m_thread, index);

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
	auto ptr = llvm::dyn_cast<llvm::GetElementPtrInst>(m_ir->CreateStructGEP(m_thread_type, m_thread, index));
	local = m_ir->CreateLoad(ptr->getResultElementType(), ptr);
	return local;
}

void PPUTranslator::RegStore(llvm::Value* value, llvm::Value*& local)
{
	RegInit(local);
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

			m_ir->CreateStore(local, m_globals[index]);
			m_globals[index] = nullptr;
		}
	}

	m_ir->SetInsertPoint(block);
}

Value* PPUTranslator::Solid(Value* value)
{
	const u32 size = ::narrow<u32>(+value->getType()->getPrimitiveSizeInBits());

	/* Workarounds (casting bool vectors directly may produce invalid code) */

	if (value->getType() == GetType<bool[4]>())
	{
		return bitcast(SExt(value, GetType<u32[4]>()), m_ir->getIntNTy(128));
	}

	if (value->getType() == GetType<bool[8]>())
	{
		return bitcast(SExt(value, GetType<u16[8]>()), m_ir->getIntNTy(128));
	}

	if (value->getType() == GetType<bool[16]>())
	{
		return bitcast(SExt(value, GetType<u8[16]>()), m_ir->getIntNTy(128));
	}

	return bitcast(value, m_ir->getIntNTy(size));
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
		return ConstantVector::getSplat(llvm::ElementCount::get(count, false), cv);
	}

	return m_ir->CreateVectorSplat(count, value);
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

		const u32 mask = cast<FixedVectorType>(type)->getNumElements() - 1;

		// Transform indices (works for vectors with size 2^N)
		for (usz i = 0; i < indices.size(); i++)
		{
			data.push_back(*(indices.begin() + indices.size() - 1 - i) ^ mask);
		}

		return m_ir->CreateShuffleVector(left, right, ConstantDataVector::get(m_context, data));
	}

	return m_ir->CreateShuffleVector(left, right, ConstantDataVector::get(m_context, { indices.begin(), indices.end() }));
}

Value* PPUTranslator::SExt(Value* value, Type* type)
{
	type = type ? type : ScaleType(value->getType(), 1);
	return value->getType() != type ? m_ir->CreateSExt(value, type) : value;
}

Value* PPUTranslator::ZExt(Value* value, Type* type)
{
	type = type ? type : ScaleType(value->getType(), 1);
	return value->getType() != type ? m_ir->CreateZExt(value, type) : value;
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
	type = type ? type : ScaleType(value->getType(), -1);
	return type != value->getType() ? m_ir->CreateTrunc(value, type) : value;
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

llvm::Value* PPUTranslator::GetMemory(llvm::Value* addr)
{
	return m_ir->CreatePtrAdd(m_base, addr);
}

void PPUTranslator::TestAborted()
{
	const auto body = BasicBlock::Create(m_context, fmt::format("__body_0x%x_%s", m_cia, m_ir->GetInsertBlock()->getName().str()), m_function);

	// Check status register in the entry block
	auto ptr = llvm::dyn_cast<GetElementPtrInst>(m_ir->CreateStructGEP(m_thread_type, m_thread, 1));
	assert(ptr->getResultElementType() == GetType<u32>());
	const auto vstate = m_ir->CreateLoad(ptr->getResultElementType(), ptr, true);
	const auto vcheck = BasicBlock::Create(m_context, fmt::format("__test_0x%x_%s", m_cia, m_ir->GetInsertBlock()->getName().str()), m_function);
	m_ir->CreateCondBr(m_ir->CreateIsNull(m_ir->CreateAnd(vstate, static_cast<u32>(cpu_flag::again + cpu_flag::exit))), body, vcheck, m_md_likely);

	m_ir->SetInsertPoint(vcheck);

	// Create tail call to the check function
	Call(GetType<void>(), "__check", m_thread, GetAddr())->setTailCall();
	m_ir->CreateRetVoid();
	m_ir->SetInsertPoint(body);
}

Value* PPUTranslator::ReadMemory(Value* addr, Type* type, bool is_be, u32 align)
{
	const u32 size = ::narrow<u32>(+type->getPrimitiveSizeInBits());

	if (m_may_be_mmio && size == 32)
	{
		// Test for MMIO patterns
		struct instructions_to_test
		{
			be_t<u32> insts[128];
		};

		m_may_be_mmio = false;

		if (auto ptr = m_info.get_ptr<instructions_to_test>(std::max<u32>(m_info.segs[0].addr, (m_reloc ? m_reloc->addr : 0) + utils::sub_saturate<u32>(::narrow<u32>(m_addr), sizeof(instructions_to_test) / 2))))
		{
			if (ppu_test_address_may_be_mmio(std::span(ptr->insts)))
			{
				m_may_be_mmio = true;
			}
		}
	}

	if (is_be ^ m_is_be && size > 8)
	{
		llvm::Value* value{};

		// Read, byteswap, bitcast
		const auto int_type = m_ir->getIntNTy(size);

		if (m_may_be_mmio && size == 32)
		{
			FlushRegisters();
			RegStore(Trunc(GetAddr()), m_cia);

			ppu_log.notice("LLVM: Detected potential MMIO32 read at [0x%08x]", m_addr + (m_reloc ? m_reloc->addr : 0));
			value = Call(GetType<u32>(), "__read_maybe_mmio32", m_base, addr);

			TestAborted();
		}
		else
		{
			const auto inst = m_ir->CreateAlignedLoad(int_type, GetMemory(addr), llvm::MaybeAlign{align});
			inst->setVolatile(true);
			value = inst;
		}

		return bitcast(Call(int_type, fmt::format("llvm.bswap.i%u", size), value), type);
	}

	if (m_may_be_mmio && size == 32)
	{
		FlushRegisters();
		RegStore(Trunc(GetAddr()), m_cia);

		ppu_log.notice("LLVM: Detected potential MMIO32 read at [0x%08x]", m_addr + (m_reloc ? m_reloc->addr : 0));
		Value* r = Call(GetType<u32>(), "__read_maybe_mmio32", m_base, addr);
		TestAborted();
		return r;
	}

	// Read normally
	const auto r = m_ir->CreateAlignedLoad(type, GetMemory(addr), llvm::MaybeAlign{align});
	r->setVolatile(true);
	return r;
}

void PPUTranslator::WriteMemory(Value* addr, Value* value, bool is_be, u32 align)
{
	const auto type = value->getType();
	const u32 size = ::narrow<u32>(+type->getPrimitiveSizeInBits());

	if (is_be ^ m_is_be && size > 8)
	{
		// Bitcast, byteswap
		const auto int_type = m_ir->getIntNTy(size);
		value = Call(int_type, fmt::format("llvm.bswap.i%u", size), bitcast(value, int_type));
	}

	if (m_may_be_mmio && size == 32)
	{
		// Test for MMIO patterns
		struct instructions_to_test
		{
			be_t<u32> insts[128];
		};

		if (auto ptr = m_info.get_ptr<instructions_to_test>(std::max<u32>(m_info.segs[0].addr, (m_reloc ? m_reloc->addr : 0) + utils::sub_saturate<u32>(::narrow<u32>(m_addr), sizeof(instructions_to_test) / 2))))
		{
			if (ppu_test_address_may_be_mmio(std::span(ptr->insts)))
			{
				FlushRegisters();
				RegStore(Trunc(GetAddr()), m_cia);

				ppu_log.notice("LLVM: Detected potential MMIO32 write at [0x%08x]", m_addr + (m_reloc ? m_reloc->addr : 0));
				Call(GetType<void>(), "__write_maybe_mmio32", m_base, addr, value);
				TestAborted();
				return;
			}
		}
	}

	// Write
	m_ir->CreateAlignedStore(value, GetMemory(addr), llvm::MaybeAlign{align})->setVolatile(true);
}

void PPUTranslator::CompilationError(const std::string& error)
{
	ppu_log.error("LLVM: [0x%08x] Error: %s", m_addr + (m_reloc ? m_reloc->addr : 0), error);
}


void PPUTranslator::MFVSCR(ppu_opcode_t op)
{
	const auto vsat = g_cfg.core.ppu_set_sat_bit ? ZExt(IsNotZero(RegLoad(m_sat)), GetType<u32>()) : m_ir->getInt32(0);
	const auto vscr = m_ir->CreateOr(vsat, m_ir->CreateShl(ZExt(RegLoad(m_nj), GetType<u32>()), 16));
	SetVr(op.vd, m_ir->CreateInsertElement(ConstantAggregateZero::get(GetType<u32[4]>()), vscr, m_ir->getInt32(m_is_be ? 3 : 0)));
}

void PPUTranslator::MTVSCR(ppu_opcode_t op)
{
	const auto vscr = m_ir->CreateExtractElement(GetVr(op.vb, VrType::vi32), m_ir->getInt32(m_is_be ? 3 : 0));
	const auto nj = Trunc(m_ir->CreateLShr(vscr, 16), GetType<bool>());
	RegStore(nj, m_nj);
	if (g_cfg.core.ppu_llvm_nj_fixup)
		RegStore(m_ir->CreateSelect(nj, m_ir->getInt32(0x7f80'0000), m_ir->getInt32(0x7fff'ffff)), m_jm_mask);
	if (g_cfg.core.ppu_set_sat_bit)
		RegStore(m_ir->CreateInsertElement(ConstantAggregateZero::get(GetType<u32[4]>()), m_ir->CreateAnd(vscr, 1), m_ir->getInt32(0)), m_sat);
}

void PPUTranslator::VADDCUW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, zext<u32[4]>(a + b < a));
}

void PPUTranslator::VADDFP(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<f32[4]>(op.va, op.vb);
	set_vr(op.vd, vec_handle_result(a + b));
}

void PPUTranslator::VADDSBS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s8[16]>(op.va, op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a + b));
}

void PPUTranslator::VADDSHS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a + b));
}

void PPUTranslator::VADDSWS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a + b));
}

void PPUTranslator::VADDUBM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, a + b);
}

void PPUTranslator::VADDUBS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a + b));
}

void PPUTranslator::VADDUHM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, a + b);
}

void PPUTranslator::VADDUHS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a + b));
}

void PPUTranslator::VADDUWM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, a + b);
}

void PPUTranslator::VADDUWS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	const auto r = add_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a + b));
}

void PPUTranslator::VAND(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, a & b);
}

void PPUTranslator::VANDC(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, a & ~b);
}

void PPUTranslator::VAVGSB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s8[16]>(op.va, op.vb);
	set_vr(op.vd, avg(a, b));
}

void PPUTranslator::VAVGSH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	set_vr(op.vd, avg(a, b));
}

void PPUTranslator::VAVGSW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	set_vr(op.vd, avg(a, b));
}

void PPUTranslator::VAVGUB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, avg(a, b));
}

void PPUTranslator::VAVGUH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, avg(a, b));
}

void PPUTranslator::VAVGUW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, avg(a, b));
}

void PPUTranslator::VCFSX(ppu_opcode_t op)
{
	const auto b = get_vr<s32[4]>(op.vb);
	set_vr(op.vd, fpcast<f32[4]>(b) * fsplat<f32[4]>(std::pow(2, -static_cast<int>(op.vuimm))));
}

void PPUTranslator::VCFUX(ppu_opcode_t op)
{
	const auto b = get_vr<u32[4]>(op.vb);

#ifdef ARCH_ARM64
	return set_vr(op.vd, fpcast<f32[4]>(b) * fsplat<f32[4]>(std::pow(2, -static_cast<int>(op.vuimm))));
#else
	if (m_use_avx512)
	{
		return set_vr(op.vd, fpcast<f32[4]>(b) * fsplat<f32[4]>(std::pow(2, -static_cast<int>(op.vuimm))));
	}

	constexpr int bit_shift = 9;
	const auto shifted = (b >> bit_shift);
	const auto cleared = shifted << bit_shift;
	const auto low_bits = b - cleared;
	const auto high_part = fpcast<f32[4]>(noncast<s32[4]>(shifted)) * fsplat<f32[4]>(static_cast<f32>(1u << bit_shift));
	const auto low_part = fpcast<f32[4]>(noncast<s32[4]>(low_bits));
	const auto temp = high_part + low_part;
	set_vr(op.vd, temp * fsplat<f32[4]>(std::pow(2, -static_cast<int>(op.vuimm))));
#endif
}

void PPUTranslator::VCMPBFP(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<f32[4]>(op.va, op.vb);
	const auto nle = sext<s32[4]>(fcmp_uno(a > b)) & 0x8000'0000;
	const auto nge = sext<s32[4]>(fcmp_uno(a < -b)) & 0x4000'0000;
	const auto r = eval(nle | nge);
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, m_ir->getFalse(), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPEQFP(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<f32[4]>(op.va, op.vb);
	const auto r = eval(sext<s32[4]>(fcmp_ord(a == b)));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPEQUB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	const auto r = eval(sext<s8[16]>(a == b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPEQUH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	const auto r = eval(sext<s16[8]>(a == b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPEQUW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	const auto r = eval(sext<s32[4]>(a == b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPGEFP(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<f32[4]>(op.va, op.vb);
	const auto r = eval(sext<s32[4]>(fcmp_ord(a >= b)));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPGTFP(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<f32[4]>(op.va, op.vb);
	const auto r = eval(sext<s32[4]>(fcmp_ord(a > b)));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPGTSB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s8[16]>(op.va, op.vb);
	const auto r = eval(sext<s8[16]>(a > b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPGTSH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	const auto r = eval(sext<s16[8]>(a > b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPGTSW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	const auto r = eval(sext<s32[4]>(a > b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPGTUB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	const auto r = eval(sext<s8[16]>(a > b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPGTUH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	const auto r = eval(sext<s16[8]>(a > b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCMPGTUW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	const auto r = eval(sext<s32[4]>(a > b));
	set_vr(op.vd, r);
	if (op.oe) SetCrField(6, IsOnes(r.value), m_ir->getFalse(), IsZero(r.value), m_ir->getFalse());
}

void PPUTranslator::VCTSXS(ppu_opcode_t op)
{
	const auto b = get_vr<f32[4]>(op.vb);
	const auto scaled = b * fsplat<f32[4]>(std::pow(2, 0 + op.vuimm));
	const auto const1 = fsplat<f32[4]>(-std::pow(2, 31));
	const auto is_nan = fcmp_uno(b != b);
	const auto sat_l = fcmp_ord(scaled < const1);
	const auto sat_h = fcmp_ord(scaled >= fsplat<f32[4]>(std::pow(2, 31)));
	value_t<s32[4]> converted = eval(fpcast<s32[4]>(select(sat_l, const1, scaled)));
	if (g_cfg.core.ppu_fix_vnan)
		converted = eval(select(is_nan, splat<s32[4]>(0), converted));  // NaN -> 0
	set_vr(op.vd, select(sat_h, splat<s32[4]>(0x7fff'ffff), converted));
	set_sat(sext<s32[4]>(sat_l) | sext<s32[4]>(sat_h));
}

void PPUTranslator::VCTUXS(ppu_opcode_t op)
{
	const auto b = get_vr<f32[4]>(op.vb);
	const auto scaled = b * fsplat<f32[4]>(std::pow(2, 0 + op.vuimm));
	const auto const0 = fsplat<f32[4]>(0.);
	const auto is_nan = fcmp_uno(b != b);
	const auto sat_l = fcmp_ord(scaled < const0);
	const auto sat_h = fcmp_ord(scaled >= fsplat<f32[4]>(std::pow(2, 32)));
	value_t<u32[4]> converted = eval(fpcast<u32[4]>(select(sat_l, const0, scaled)));
	if (g_cfg.core.ppu_fix_vnan)
		converted = eval(select(is_nan, splat<u32[4]>(0), converted)); // NaN -> 0
	set_vr(op.vd, select(sat_h, splat<u32[4]>(0xffff'ffff), converted));
	set_sat(sext<s32[4]>(sat_l) | sext<s32[4]>(sat_h));
}

void PPUTranslator::VEXPTEFP(ppu_opcode_t op)
{
	const auto b = get_vr<f32[4]>(op.vb);
	set_vr(op.vd, vec_handle_result(llvm_calli<f32[4], decltype(b)>{"llvm.exp2.v4f32", {b}}));
}

void PPUTranslator::VLOGEFP(ppu_opcode_t op)
{
	const auto b = get_vr<f32[4]>(op.vb);
	set_vr(op.vd, vec_handle_result(llvm_calli<f32[4], decltype(b)>{"llvm.log2.v4f32", {b}}));
}

void PPUTranslator::VMADDFP(ppu_opcode_t op)
{
	auto [a, b, c] = get_vrs<f32[4]>(op.va, op.vb, op.vc);

	// Optimization: Emit only a floating multiply if the addend is zero
	if (auto [ok, data] = get_const_vector(b.value, ::narrow<u32>(m_addr)); ok)
	{
		if (data == v128::from32p(1u << 31))
		{
			set_vr(op.vd, vec_handle_result(a * c));
			ppu_log.notice("LLVM: VMADDFP with -0 addend at [0x%08x]", m_addr + (m_reloc ? m_reloc->addr : 0));
			return;
		}

		if (!m_use_fma && data == v128{})
		{
			set_vr(op.vd, vec_handle_result(a * c + fsplat<f32[4]>(0.f)));
			ppu_log.notice("LLVM: VMADDFP with -0 addend at [0x%08x]", m_addr + (m_reloc ? m_reloc->addr : 0));
			return;
		}
	}

	if (m_use_fma)
	{
		set_vr(op.vd, vec_handle_result(fmuladd(a, c, b)));
		return;
	}

	// Emulated FMA via double precision (caution: out-of-lane algorithm)
	const auto xa = fpcast<f64[4]>(a);
	const auto xb = fpcast<f64[4]>(b);
	const auto xc = fpcast<f64[4]>(c);

	const auto xr = fmuladd(xa, xc, xb);
	set_vr(op.vd, vec_handle_result(fpcast<f32[4]>(xr)));
}

void PPUTranslator::VMAXFP(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<f32[4]>(op.va, op.vb);
	set_vr(op.vd, vec_handle_result(select(fcmp_ord(a < b) | fcmp_uno(b != b), b, a)));
}

void PPUTranslator::VMAXSB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s8[16]>(op.va, op.vb);
	set_vr(op.vd, max(a, b));
}

void PPUTranslator::VMAXSH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	set_vr(op.vd, max(a, b));
}

void PPUTranslator::VMAXSW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	set_vr(op.vd, max(a, b));
}

void PPUTranslator::VMAXUB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, max(a, b));
}

void PPUTranslator::VMAXUH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, max(a, b));
}

void PPUTranslator::VMAXUW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, max(a, b));
}

void PPUTranslator::VMHADDSHS(ppu_opcode_t op)
{
	// Caution: out-of-lane algorithm
	const auto [a, b, c] = get_vrs<s16[8]>(op.va, op.vb, op.vc);
	const auto m = ((sext<s32[8]>(a) * sext<s32[8]>(b)) >> 15) + sext<s32[8]>(c);
	const auto r = trunc<u16[8]>(min(max(m, splat<s32[8]>(-0x8000)), splat<s32[8]>(0x7fff)));
	set_vr(op.vd, r);
	set_sat(trunc<u16[8]>((m + 0x8000) >> 16));
}

void PPUTranslator::VMHRADDSHS(ppu_opcode_t op)
{
	// Caution: out-of-lane algorithm
	const auto [a, b, c] = get_vrs<s16[8]>(op.va, op.vb, op.vc);
	const auto m = ((sext<s32[8]>(a) * sext<s32[8]>(b) + splat<s32[8]>(0x4000)) >> 15) + sext<s32[8]>(c);
	const auto r = trunc<u16[8]>(min(max(m, splat<s32[8]>(-0x8000)), splat<s32[8]>(0x7fff)));
	set_vr(op.vd, r);
	set_sat(trunc<u16[8]>((m + 0x8000) >> 16));
}

void PPUTranslator::VMINFP(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<f32[4]>(op.va, op.vb);
	set_vr(op.vd, vec_handle_result(select(fcmp_ord(a > b) | fcmp_uno(b != b), b, a)));
}

void PPUTranslator::VMINSB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s8[16]>(op.va, op.vb);
	set_vr(op.vd, min(a, b));
}

void PPUTranslator::VMINSH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	set_vr(op.vd, min(a, b));
}

void PPUTranslator::VMINSW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	set_vr(op.vd, min(a, b));
}

void PPUTranslator::VMINUB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, min(a, b));
}

void PPUTranslator::VMINUH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, min(a, b));
}

void PPUTranslator::VMINUW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, min(a, b));
}

void PPUTranslator::VMLADDUHM(ppu_opcode_t op)
{
	const auto [a, b, c] = get_vrs<u16[8]>(op.va, op.vb, op.vc);
	set_vr(op.vd, a * b + c);
}

void PPUTranslator::VMRGHB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, shuffle2(a, b, 24, 8, 25, 9, 26, 10, 27, 11, 28, 12, 29, 13, 30, 14, 31, 15));
}

void PPUTranslator::VMRGHH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, shuffle2(a, b, 12, 4, 13, 5, 14, 6, 15, 7));
}

void PPUTranslator::VMRGHW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, shuffle2(a, b, 6, 2, 7, 3));
}

void PPUTranslator::VMRGLB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, shuffle2(a, b, 16, 0, 17, 1, 18, 2, 19, 3, 20, 4, 21, 5, 22, 6, 23, 7));
}

void PPUTranslator::VMRGLH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, shuffle2(a, b, 8, 0, 9, 1, 10, 2, 11, 3));
}

void PPUTranslator::VMRGLW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, shuffle2(a, b, 4, 0, 5, 1));
}

void PPUTranslator::VMSUMMBM(ppu_opcode_t op)
{
	const auto a = get_vr<s16[8]>(op.va);
	const auto b = get_vr<u16[8]>(op.vb);
	const auto c = get_vr<s32[4]>(op.vc);
	const auto ml = bitcast<s32[4]>((a << 8 >> 8) * noncast<s16[8]>(b << 8 >> 8));
	const auto mh = bitcast<s32[4]>((a >> 8) * noncast<s16[8]>(b >> 8));
	set_vr(op.vd, ((ml << 16 >> 16) + (ml >> 16)) + ((mh << 16 >> 16) + (mh >> 16)) + c);
}

void PPUTranslator::VMSUMSHM(ppu_opcode_t op)
{
	const auto [a, b, c] = get_vrs<s32[4]>(op.va, op.vb, op.vc);
	const auto ml = (a << 16 >> 16) * (b << 16 >> 16);
	const auto mh = (a >> 16) * (b >> 16);
	set_vr(op.vd, ml + mh + c);
}

void PPUTranslator::VMSUMSHS(ppu_opcode_t op)
{
	const auto [a, b, c] = get_vrs<s32[4]>(op.va, op.vb, op.vc);
	const auto ml = (a << 16 >> 16) * (b << 16 >> 16);
	const auto mh = (a >> 16) * (b >> 16);
	const auto m = eval(ml + mh);
	const auto s = eval(m + c);
	const auto z = eval((c >> 31) ^ 0x7fffffff);
	const auto mx = eval(m ^ sext<s32[4]>(m == 0x80000000u));
	const auto x = eval(((mx ^ s) & ~(c ^ mx)) >> 31);
	set_vr(op.vd, eval((z & x) | (s & ~x)));
	set_sat(x);
}

void PPUTranslator::VMSUMUBM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	const auto c = get_vr<u32[4]>(op.vc);
	const auto ml = bitcast<u32[4]>((a << 8 >> 8) * (b << 8 >> 8));
	const auto mh = bitcast<u32[4]>((a >> 8) * (b >> 8));
	set_vr(op.vd, eval(((ml << 16 >> 16) + (ml >> 16)) + ((mh << 16 >> 16) + (mh >> 16)) + c));
}

void PPUTranslator::VMSUMUHM(ppu_opcode_t op)
{
	const auto [a, b, c] = get_vrs<u32[4]>(op.va, op.vb, op.vc);
	const auto ml = (a << 16 >> 16) * (b << 16 >> 16);
	const auto mh = (a >> 16) * (b >> 16);
	set_vr(op.vd, ml + mh + c);
}

void PPUTranslator::VMSUMUHS(ppu_opcode_t op)
{
	const auto [a, b, c] = get_vrs<u32[4]>(op.va, op.vb, op.vc);
	const auto ml = (a << 16 >> 16) * (b << 16 >> 16);
	const auto mh = (a >> 16) * (b >> 16);
	const auto s = eval(ml + mh);
	const auto s2 = eval(s + c);
	const auto x = eval((s < ml) | (s2 < s));
	set_vr(op.vd, select(x, splat<u32[4]>(-1), s2));
	set_sat(x);
}

void PPUTranslator::VMULESB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	set_vr(op.vd, (a >> 8) * (b >> 8));
}

void PPUTranslator::VMULESH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	set_vr(op.vd, (a >> 16) * (b >> 16));
}

void PPUTranslator::VMULEUB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, (a >> 8) * (b >> 8));
}

void PPUTranslator::VMULEUH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, (a >> 16) * (b >> 16));
}

void PPUTranslator::VMULOSB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	set_vr(op.vd, (a << 8 >> 8) * (b << 8 >> 8));
}

void PPUTranslator::VMULOSH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	set_vr(op.vd, (a << 16 >> 16) * (b << 16 >> 16));
}

void PPUTranslator::VMULOUB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, (a << 8 >> 8) * (b << 8 >> 8));
}

void PPUTranslator::VMULOUH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, (a << 16 >> 16) * (b << 16 >> 16));
}

void PPUTranslator::VNMSUBFP(ppu_opcode_t op)
{
	auto [a, b, c] = get_vrs<f32[4]>(op.va, op.vb, op.vc);

	// Optimization: Emit only a floating multiply if the addend is zero
	if (const auto [ok, data] = get_const_vector(b.value, ::narrow<u32>(m_addr)); ok)
	{
		if (data == v128{})
		{
			set_vr(op.vd, vec_handle_result(-(a * c)));
			ppu_log.notice("LLVM: VNMSUBFP with 0 addend at [0x%08x]", m_addr + (m_reloc ? m_reloc->addr : 0));
			return;
		}

		if (!m_use_fma && data == v128::from32p(1u << 31))
		{
			set_vr(op.vd, vec_handle_result(-(a * c - fsplat<f32[4]>(0.f))));
			ppu_log.notice("LLVM: VNMSUBFP with -0 addend at [0x%08x]", m_addr + (m_reloc ? m_reloc->addr : 0));
			return;
		}
	}

	// Differs from the emulated path with regards to negative zero
	if (m_use_fma)
	{
		set_vr(op.vd, vec_handle_result(-fmuladd(a, c, -b)));
		return;
	}

	// Emulated FMA via double precision (caution: out-of-lane algorithm)
	const auto xa = fpcast<f64[4]>(a);
	const auto xb = fpcast<f64[4]>(b);
	const auto xc = fpcast<f64[4]>(c);

	const auto nr = xa * xc - xb;
	set_vr(op.vd, vec_handle_result(fpcast<f32[4]>(-nr)));
}

void PPUTranslator::VNOR(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, ~(a | b));
}

void PPUTranslator::VOR(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, a | b);
}

void PPUTranslator::VPERM(ppu_opcode_t op)
{
	const auto [a, b, c] = get_vrs<u8[16]>(op.va, op.vb, op.vc);

	if (op.ra == op.rb)
	{
		set_vr(op.vd, pshufb(a, ~c & 0xf));
		return;
	}

	if (m_use_avx512_icl)
	{
		const auto i = eval(~c);
		set_vr(op.vd, vperm2b(b, a, i));
		return;
	}

	const auto i = eval(~c & 0x1f);
	set_vr(op.vd, select(noncast<s8[16]>(c << 3) >= 0, pshufb(a, i), pshufb(b, i)));
}

void PPUTranslator::VPKPX(ppu_opcode_t op)
{
	// Caution: out-of-lane algorithm
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	const auto ab = shuffle2(b, a, 0, 1, 2, 3, 4, 5, 6, 7);
	const auto e1 = (ab & 0x01f80000) >> 9;
	const auto e2 = (ab & 0xf800) >> 6;
	const auto e3 = (ab & 0xf8) >> 3;
	set_vr(op.vd, trunc<u16[8]>(e1 | e2 | e3));
}

void PPUTranslator::VPKSHSS(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	const auto ab = shuffle2(b, a, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
	const auto r = trunc<u8[16]>(min(max(ab, splat<s16[16]>(-0x80)), splat<s16[16]>(0x7f)));
	set_vr(op.vd, r);
	set_sat(bitcast<u16[8]>((a + 0x80) | (b + 0x80)) >> 8);
}

void PPUTranslator::VPKSHUS(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	const auto ab = shuffle2(b, a, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
	const auto r = trunc<u8[16]>(min(max(ab, splat<s16[16]>(0)), splat<s16[16]>(0xff)));
	set_vr(op.vd, r);
	set_sat(bitcast<u16[8]>(a | b) >> 8);
}

void PPUTranslator::VPKSWSS(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	const auto ab = shuffle2(b, a, 0, 1, 2, 3, 4, 5, 6, 7);
	const auto r = trunc<u16[8]>(min(max(ab, splat<s32[8]>(-0x8000)), splat<s32[8]>(0x7fff)));
	set_vr(op.vd, r);
	set_sat(bitcast<u32[4]>((a + 0x8000) | (b + 0x8000)) >> 16);
}

void PPUTranslator::VPKSWUS(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	const auto ab = shuffle2(b, a, 0, 1, 2, 3, 4, 5, 6, 7);
	const auto r = trunc<u16[8]>(min(max(ab, splat<s32[8]>(0)), splat<s32[8]>(0xffff)));
	set_vr(op.vd, r);
	set_sat(bitcast<u32[4]>(a | b) >> 16);
}

void PPUTranslator::VPKUHUM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	const auto r = shuffle2(b, a, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
	set_vr(op.vd, r);
}

void PPUTranslator::VPKUHUS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	const auto ta = bitcast<u8[16]>(min(a, splat<u16[8]>(0xff)));
	const auto tb = bitcast<u8[16]>(min(b, splat<u16[8]>(0xff)));
	const auto r = shuffle2(tb, ta, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30);
	set_vr(op.vd, r);
	set_sat((a | b) >> 8);
}

void PPUTranslator::VPKUWUM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	const auto r = shuffle2(b, a, 0, 2, 4, 6, 8, 10, 12, 14);
	set_vr(op.vd, r);
}

void PPUTranslator::VPKUWUS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	const auto ta = bitcast<u16[8]>(min(a, splat<u32[4]>(0xffff)));
	const auto tb = bitcast<u16[8]>(min(b, splat<u32[4]>(0xffff)));
	const auto r = shuffle2(tb, ta, 0, 2, 4, 6, 8, 10, 12, 14);
	set_vr(op.vd, r);
	set_sat((a | b) >> 16);
}

void PPUTranslator::VREFP(ppu_opcode_t op)
{
	set_vr(op.vd, vec_handle_result(fsplat<f32[4]>(1.0) / get_vr<f32[4]>(op.vb)));
}

void PPUTranslator::VRFIM(ppu_opcode_t op)
{
	set_vr(op.vd, vec_handle_result(callf<f32[4]>(get_intrinsic<f32[4]>(Intrinsic::floor), get_vr<f32[4]>(op.vb))));
}

void PPUTranslator::VRFIN(ppu_opcode_t op)
{
	set_vr(op.vd, vec_handle_result(callf<f32[4]>(get_intrinsic<f32[4]>(Intrinsic::roundeven), get_vr<f32[4]>(op.vb))));
}

void PPUTranslator::VRFIP(ppu_opcode_t op)
{
	set_vr(op.vd, vec_handle_result(callf<f32[4]>(get_intrinsic<f32[4]>(Intrinsic::ceil), get_vr<f32[4]>(op.vb))));
}

void PPUTranslator::VRFIZ(ppu_opcode_t op)
{
	set_vr(op.vd, vec_handle_result(callf<f32[4]>(get_intrinsic<f32[4]>(Intrinsic::trunc), get_vr<f32[4]>(op.vb))));
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
	set_vr(op.vd, vec_handle_result(fsplat<f32[4]>(1.0) / callf<f32[4]>(get_intrinsic<f32[4]>(Intrinsic::sqrt), get_vr<f32[4]>(op.vb))));
}

void PPUTranslator::VSEL(ppu_opcode_t op)
{
	const auto c = get_vr<u32[4]>(op.vc);

	// Check if the constant mask doesn't require bit granularity
	if (auto [ok, mask] = get_const_vector(c.value, ::narrow<u32>(m_addr)); ok)
	{
		bool sel_32 = true;
		for (u32 i = 0; i < 4; i++)
		{
			if (mask._u32[i] && mask._u32[i] != 0xFFFFFFFF)
			{
				sel_32 = false;
				break;
			}
		}

		if (sel_32)
		{
			set_vr(op.vd, select(noncast<s32[4]>(c) != 0, get_vr<u32[4]>(op.vb), get_vr<u32[4]>(op.va)));
			return;
		}

		bool sel_16 = true;
		for (u32 i = 0; i < 8; i++)
		{
			if (mask._u16[i] && mask._u16[i] != 0xFFFF)
			{
				sel_16 = false;
				break;
			}
		}

		if (sel_16)
		{
			set_vr(op.vd, select(bitcast<s16[8]>(c) != 0, get_vr<u16[8]>(op.vb), get_vr<u16[8]>(op.va)));
			return;
		}


		bool sel_8 = true;
		for (u32 i = 0; i < 16; i++)
		{
			if (mask._u8[i] && mask._u8[i] != 0xFF)
			{
				sel_8 = false;
				break;
			}
		}

		if (sel_8)
		{
			set_vr(op.vd, select(bitcast<s8[16]>(c) != 0,get_vr<u8[16]>(op.vb), get_vr<u8[16]>(op.va)));
			return;
		}
	}

	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, eval((b & c) | (a & ~c)));
}

void PPUTranslator::VSL(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, fshl(a, zshuffle(a, 16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14), b));
}

void PPUTranslator::VSLB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, a << (b & 7));
}

void PPUTranslator::VSLDOI(ppu_opcode_t op)
{
	if (op.vsh == 0)
	{
		set_vr(op.vd, get_vr<u32[4]>(op.va));
	}
	else if ((op.vsh % 4) == 0)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
		const auto s = op.vsh / 4;
		const auto x = 7;
		set_vr(op.vd, shuffle2(b, a, (s + 3) ^ x, (s + 2) ^ x, (s + 1) ^ x, (s) ^ x));
	}
	else if ((op.vsh % 2) == 0)
	{
		const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
		const auto s = op.vsh / 2;
		const auto x = 15;
		set_vr(op.vd, shuffle2(b, a, (s + 7) ^ x, (s + 6) ^ x, (s + 5) ^ x, (s + 4) ^ x, (s + 3) ^ x, (s + 2) ^ x, (s + 1) ^ x, (s) ^ x));
	}
	else
	{
		const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
		const auto s = op.vsh;
		const auto x = 31;
		set_vr(op.vd, shuffle2(b, a, (s + 15) ^ x, (s + 14) ^ x, (s + 13) ^ x, (s + 12) ^ x, (s + 11) ^ x, (s + 10) ^ x, (s + 9) ^ x, (s + 8) ^ x, (s + 7) ^ x, (s + 6) ^ x, (s + 5) ^ x, (s + 4) ^ x, (s + 3) ^ x, (s + 2) ^ x, (s + 1) ^ x, (s) ^ x));
	}
}

void PPUTranslator::VSLH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, a << (b & 15));
}

void PPUTranslator::VSLO(ppu_opcode_t op)
{
	// TODO (rare)
	const auto [a, b] = get_vrs<u128>(op.va, op.vb);
	set_vr(op.vd, a << (b & 0x78));
}

void PPUTranslator::VSLW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, a << (b & 31));
}

void PPUTranslator::VSPLTB(ppu_opcode_t op)
{
	const u32 ui = ~op.vuimm & 0xf;
	set_vr(op.vd, zshuffle(get_vr<u8[16]>(op.vb), ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui, ui));
}

void PPUTranslator::VSPLTH(ppu_opcode_t op)
{
	const u32 ui = ~op.vuimm & 0x7;
	set_vr(op.vd, zshuffle(get_vr<u16[8]>(op.vb), ui, ui, ui, ui, ui, ui, ui, ui));
}

void PPUTranslator::VSPLTISB(ppu_opcode_t op)
{
	set_vr(op.vd, splat<u8[16]>(op.vsimm));
}

void PPUTranslator::VSPLTISH(ppu_opcode_t op)
{
	set_vr(op.vd, splat<u16[8]>(op.vsimm));
}

void PPUTranslator::VSPLTISW(ppu_opcode_t op)
{
	set_vr(op.vd, splat<u32[4]>(op.vsimm));
}

void PPUTranslator::VSPLTW(ppu_opcode_t op)
{
	const u32 ui = ~op.vuimm & 0x3;
	set_vr(op.vd, zshuffle(get_vr<u32[4]>(op.vb), ui, ui, ui, ui));
}

void PPUTranslator::VSR(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, fshr(zshuffle(a, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16), a, b));
}

void PPUTranslator::VSRAB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s8[16]>(op.va, op.vb);
	set_vr(op.vd, a >> (b & 7));
}

void PPUTranslator::VSRAH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	set_vr(op.vd, a >> (b & 15));
}

void PPUTranslator::VSRAW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	set_vr(op.vd, a >> (b & 31));
}

void PPUTranslator::VSRB(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, a >> (b & 7));
}

void PPUTranslator::VSRH(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, a >> (b & 15));
}

void PPUTranslator::VSRO(ppu_opcode_t op)
{
	// TODO (very rare)
	const auto [a, b] = get_vrs<u128>(op.va, op.vb);
	set_vr(op.vd, a >> (b & 0x78));
}

void PPUTranslator::VSRW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, a >> (b & 31));
}

void PPUTranslator::VSUBCUW(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, zext<u32[4]>(a >= b));
}

void PPUTranslator::VSUBFP(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<f32[4]>(op.va, op.vb);
	set_vr(op.vd, vec_handle_result(a - b));
}

void PPUTranslator::VSUBSBS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s8[16]>(op.va, op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a - b));
}

void PPUTranslator::VSUBSHS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s16[8]>(op.va, op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a - b));
}

void PPUTranslator::VSUBSWS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a - b));
}

void PPUTranslator::VSUBUBM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	set_vr(op.vd, eval(a - b));
}

void PPUTranslator::VSUBUBS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u8[16]>(op.va, op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a - b));
}

void PPUTranslator::VSUBUHM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	set_vr(op.vd, eval(a - b));
}

void PPUTranslator::VSUBUHS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u16[8]>(op.va, op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a - b));
}

void PPUTranslator::VSUBUWM(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, eval(a - b));
}

void PPUTranslator::VSUBUWS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	const auto r = sub_sat(a, b);
	set_vr(op.vd, r);
	set_sat(r ^ (a - b));
}

void PPUTranslator::VSUMSWS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s32[4]>(op.va, op.vb);
	const auto x = sext<s64[2]>(zshuffle(a, 0, 1));
	const auto y = sext<s64[2]>(zshuffle(a, 2, 3));
	const auto z = sext<s64[2]>(zshuffle(b, 0, 4));
	const auto s = eval(x + y + z);
	const auto r = min(max(zshuffle(s, 0, 2) + zshuffle(s, 1, 2), splat<s64[2]>(-0x8000'0000ll)), splat<s64[2]>(0x7fff'ffff));
	set_vr(op.vd, zshuffle(bitcast<u32[4]>(r), 0, 4, 4, 4));
	set_sat(bitcast<u64[2]>(r + 0x8000'0000) >> 32);
}

void PPUTranslator::VSUM2SWS(ppu_opcode_t op)
{
	const auto [a, b] = get_vrs<s64[2]>(op.va, op.vb);
	const auto x = a << 32 >> 32;
	const auto y = a >> 32;
	const auto z = b >> 32;
	const auto r = min(max(x + y + z, splat<s64[2]>(-0x8000'0000ll)), splat<s64[2]>(0x7fff'ffff));
	set_vr(op.vd, zshuffle(bitcast<u32[4]>(r), 0, 4, 2, 4));
	set_sat(bitcast<u64[2]>(r + 0x8000'0000) >> 32);
}

void PPUTranslator::VSUM4SBS(ppu_opcode_t op)
{
	const auto a = get_vr<s16[8]>(op.va);
	const auto b = get_vr<s32[4]>(op.vb);
	const auto x = eval(bitcast<s32[4]>((a << 8 >> 8) + (a >> 8)));
	const auto s = eval((x << 16 >> 16) + (x >> 16));
	const auto r = add_sat(s, b);
	set_vr(op.vd, r);
	set_sat(r ^ (s + b));
}

void PPUTranslator::VSUM4SHS(ppu_opcode_t op)
{
	const auto a = get_vr<s32[4]>(op.va);
	const auto b = get_vr<s32[4]>(op.vb);
	const auto s = eval((a << 16 >> 16) + (a >> 16));
	const auto r = add_sat(s, b);
	set_vr(op.vd, r);
	set_sat(r ^ (s + b));
}

void PPUTranslator::VSUM4UBS(ppu_opcode_t op)
{
	const auto a = get_vr<u16[8]>(op.va);
	const auto b = get_vr<u32[4]>(op.vb);
	const auto x = eval(bitcast<u32[4]>((a & 0xff) + (a >> 8)));
	const auto s = eval((x & 0xffff) + (x >> 16));
	const auto r = add_sat(s, b);
	set_vr(op.vd, r);
	set_sat(r ^ (s + b));
}

#define UNPACK_PIXEL_OP(px) (px & 0xff00001f) | ((px << 6) & 0x1f0000) | ((px << 3) & 0x1f00)

void PPUTranslator::VUPKHPX(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto px = sext<s32[4]>(zshuffle(get_vr<s16[8]>(op.vb), 4, 5, 6, 7));
	set_vr(op.vd, UNPACK_PIXEL_OP(px));
}

void PPUTranslator::VUPKHSB(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto r = sext<s16[8]>(zshuffle(get_vr<s8[16]>(op.vb), 8, 9, 10, 11, 12, 13, 14, 15));
	set_vr(op.vd, r);
}

void PPUTranslator::VUPKHSH(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto r = sext<s32[4]>(zshuffle(get_vr<s16[8]>(op.vb), 4, 5, 6, 7));
	set_vr(op.vd, r);
}

void PPUTranslator::VUPKLPX(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto px = sext<s32[4]>(zshuffle(get_vr<s16[8]>(op.vb), 0, 1, 2, 3));
	set_vr(op.vd, UNPACK_PIXEL_OP(px));
}

void PPUTranslator::VUPKLSB(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto r = sext<s16[8]>(zshuffle(get_vr<s8[16]>(op.vb), 0, 1, 2, 3, 4, 5, 6, 7));
	set_vr(op.vd, r);
}

void PPUTranslator::VUPKLSH(ppu_opcode_t op)
{
	// Caution: potentially out-of-lane algorithm
	const auto r = sext<s32[4]>(zshuffle(get_vr<s16[8]>(op.vb), 0, 1, 2, 3));
	set_vr(op.vd, r);
}

void PPUTranslator::VXOR(ppu_opcode_t op)
{
	if (op.va == op.vb)
	{
		// Assign zero, break dependencies
		set_vr(op.vd, splat<u32[4]>(0));
		return;
	}

	const auto [a, b] = get_vrs<u32[4]>(op.va, op.vb);
	set_vr(op.vd, a ^ b);
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm);
}

void PPUTranslator::ADDIS(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16 << 16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = m_ir->CreateShl(SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>()), 16);
		m_rel = nullptr;
	}

	SetGpr(op.rd, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm);
}

void PPUTranslator::BC(ppu_opcode_t op)
{
	const s32 bt14 = op.bt14; // Workaround for VS 16.5
	const u64 target = (op.aa ? 0 : m_addr) + bt14;

	if (op.aa && m_reloc)
	{
		CompilationError("Branch with absolute address");
	}

	if (op.lk)
	{
		m_ir->CreateStore(GetAddr(+4), m_ir->CreateStructGEP(m_thread_type, m_thread, static_cast<uint>(&m_lr - m_locals)));
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
			Call(GetType<void>(), fmt::format("%s", ppu_syscall_code(index)), m_thread);
			//Call(GetType<void>(), "__escape", m_thread)->setTailCall();
			m_ir->CreateRetVoid();
			return;
		}
	}

	Call(GetType<void>(), op.lev ? "__lv1call" : "__syscall", m_thread, num);
	//Call(GetType<void>(), "__escape", m_thread)->setTailCall();
	m_ir->CreateRetVoid();
}

void PPUTranslator::B(ppu_opcode_t op)
{
	const s32 bt24 = op.bt24; // Workaround for VS 16.5
	const u64 target = (op.aa ? 0 : m_addr) + bt24;

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
		m_ir->CreateStore(GetAddr(+4), m_ir->CreateStructGEP(m_thread_type, m_thread, static_cast<uint>(&m_lr - m_locals)));
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

void PPUTranslator::ISYNC(ppu_opcode_t)
{
	m_ir->CreateFence(AtomicOrdering::Acquire);
}

void PPUTranslator::CRXOR(ppu_opcode_t op)
{
	SetCrb(op.crbd, m_ir->CreateXor(GetCrb(op.crba), GetCrb(op.crbb)));
}

void PPUTranslator::DCBI(ppu_opcode_t)
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
		m_ir->CreateStore(GetAddr(+4), m_ir->CreateStructGEP(m_thread_type, m_thread, static_cast<uint>(&m_lr - m_locals)));
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

	if (mask != umax)
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = ZExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.ra, m_ir->CreateOr(GetGpr(op.rs), imm));
}

void PPUTranslator::ORIS(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.uimm16 << 16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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

	if (mask != umax)
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
	const auto addr = value<u64>(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb));
	set_vr(op.vd, build<u8[16]>(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0) + vsplat<u8[16]>(trunc<u8>(addr & 0xf)));
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
	if (op.oe) UNK(op);
}

void PPUTranslator::ADDC(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto result = m_ir->CreateAdd(a, b);
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULT(result, b));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));

	if (op.oe)
	{
		//const auto s = m_ir->CreateCall(get_intrinsic<u64>(llvm::Intrinsic::sadd_with_overflow), {a, b});
		//SetOverflow(m_ir->CreateExtractValue(s, {1}));
		SetOverflow(m_ir->CreateICmpSLT(m_ir->CreateAnd(m_ir->CreateXor(a, m_ir->CreateNot(b)), m_ir->CreateXor(a, result)), m_ir->getInt64(0)));
	}
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
	if (op.rc) SetCrFieldSignedCmp(0, GetGpr(op.rd), m_ir->getInt64(0));
}

void PPUTranslator::MFOCRF(ppu_opcode_t op)
{
	if (op.l11)
	{
		// MFOCRF
		const u64 pos = countl_zero<u32>(op.crm) - 24;

		if (pos >= 8 || 0x80u >> pos != op.crm)
		{
			SetGpr(op.rd, UndefValue::get(GetType<u64>()));
			return;
		}
	}
	else if (std::none_of(m_cr + 0, m_cr + 32, [](auto* p) { return p; }))
	{
		// MFCR (optimized)
		Value* ln0 = m_ir->CreateStructGEP(m_thread_type, m_thread, 99);
		Value* ln1 = m_ir->CreateStructGEP(m_thread_type, m_thread, 115);

		ln0 = m_ir->CreateLoad(GetType<u8[16]>(), ln0);
		ln1 = m_ir->CreateLoad(GetType<u8[16]>(), ln1);
		if (!m_is_be)
		{
			ln0 = Shuffle(ln0, nullptr, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
			ln1 = Shuffle(ln1, nullptr, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		}

		const auto m0 = ZExt(bitcast<u16>(Trunc(ln0, GetType<bool[16]>())));
		const auto m1 = ZExt(bitcast<u16>(Trunc(ln1, GetType<bool[16]>())));
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
	if (g_cfg.core.ppu_128_reservations_loop_max_length)
	{
		RegStore(Trunc(GetAddr()), m_cia);
		FlushRegisters();
		Call(GetType<void>(), "__resinterp", m_thread);
		//Call(GetType<void>(), "__escape", m_thread)->setTailCall();
		m_ir->CreateRetVoid();
		return;
	}

	SetGpr(op.rd, Call(GetType<u32>(), "__lwarx", m_thread, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::LDX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u64>()));
}

void PPUTranslator::LWZX(ppu_opcode_t op)
{
	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u && op.rb != 1u && op.rb != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation
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
	const auto addr = value<u64>(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb));
	set_vr(op.vd, build<u8[16]>(31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16) - vsplat<u8[16]>(trunc<u8>(addr & 0xf)));
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

	if (op.oe)
	{
		//const auto s = m_ir->CreateCall(get_intrinsic<u64>(llvm::Intrinsic::ssub_with_overflow), {b, m_ir->CreateNot(a)});
		//SetOverflow(m_ir->CreateExtractValue(s, {1}));
		SetOverflow(m_ir->CreateICmpSLT(m_ir->CreateAnd(m_ir->CreateXor(a, b), m_ir->CreateXor(m_ir->CreateNot(a), result)), m_ir->getInt64(0)));
	}
}

void PPUTranslator::LDUX(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb));
	SetGpr(op.rd, ReadMemory(addr, GetType<u64>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::DCBST(ppu_opcode_t)
{
}

void PPUTranslator::LWZUX(ppu_opcode_t op)
{
	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u && op.rb != 1u && op.rb != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation

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
	if (op.rc) SetCrFieldSignedCmp(0, GetGpr(op.rd), m_ir->getInt64(0));
}

void PPUTranslator::LDARX(ppu_opcode_t op)
{
	if (g_cfg.core.ppu_128_reservations_loop_max_length)
	{
		RegStore(Trunc(GetAddr()), m_cia);
		FlushRegisters();
		Call(GetType<void>(), "__resinterp", m_thread);
		//Call(GetType<void>(), "__escape", m_thread)->setTailCall();
		m_ir->CreateRetVoid();
		return;
	}

	SetGpr(op.rd, Call(GetType<u64>(), "__ldarx", m_thread, op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb)));
}

void PPUTranslator::DCBF(ppu_opcode_t)
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
	if (op.oe) SetOverflow(m_ir->CreateICmpEQ(result, m_ir->getInt64(1ull << 63)));
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
	if (op.oe) UNK(op);
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
	if (op.oe) UNK(op);
}

void PPUTranslator::MTOCRF(ppu_opcode_t op)
{
	if (op.l11)
	{
		// MTOCRF
		const u64 pos = countl_zero<u32>(op.crm) - 24;

		if (pos >= 8 || 0x80u >> pos != op.crm)
		{
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
			const auto src = m_ir->CreateGEP(m_mtocr_table->getValueType(), m_mtocr_table, {m_ir->getInt32(0), m_ir->CreateShl(index, 2)});
			const auto dst = m_ir->CreateStructGEP(m_thread_type, m_thread, static_cast<uint>(m_cr - m_locals) + i * 4);
			Call(GetType<void>(), "llvm.memcpy.p0.p0.i32", dst, src, m_ir->getInt32(4), m_ir->getFalse());
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
	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u && op.rb != 1u && op.rb != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation
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
	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u && op.rb != 1u && op.rb != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation
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
	if (op.oe) UNK(op);
}

void PPUTranslator::SUBFZE(ppu_opcode_t op)
{
	const auto a = m_ir->CreateNot(GetGpr(op.ra));
	const auto c = GetCarry();
	const auto result = m_ir->CreateAdd(a, ZExt(c, GetType<u64>()));
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateICmpULT(result, a));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) UNK(op);
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
	if (op.oe) UNK(op);
}

void PPUTranslator::MULLD(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto b = GetGpr(op.rb);
	const auto result = m_ir->CreateMul(a, b);
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) UNK(op);
}

void PPUTranslator::ADDME(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra);
	const auto c = GetCarry();
	const auto result = m_ir->CreateSub(a, ZExt(m_ir->CreateNot(c), GetType<u64>()));
	SetGpr(op.rd, result);
	SetCarry(m_ir->CreateOr(c, IsNotZero(a)));
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) UNK(op);
}

void PPUTranslator::MULLW(ppu_opcode_t op)
{
	const auto a = SExt(GetGpr(op.ra, 32));
	const auto b = SExt(GetGpr(op.rb, 32));
	const auto result = m_ir->CreateMul(a, b);
	SetGpr(op.rd, result);
	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
	if (op.oe) UNK(op);
}

void PPUTranslator::DCBTST(ppu_opcode_t)
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

	if (op.oe)
	{
		//const auto s = m_ir->CreateCall(get_intrinsic<u64>(llvm::Intrinsic::sadd_with_overflow), {a, b});
		//SetOverflow(m_ir->CreateExtractValue(s, {1}));
		SetOverflow(m_ir->CreateICmpSLT(m_ir->CreateAnd(m_ir->CreateXor(a, m_ir->CreateNot(b)), m_ir->CreateXor(a, result)), m_ir->getInt64(0)));
	}

	if (op.rc) SetCrFieldSignedCmp(0, result, m_ir->getInt64(0));
}

void PPUTranslator::DCBT(ppu_opcode_t)
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
	UNK(op);
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

void PPUTranslator::DST(ppu_opcode_t)
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

void PPUTranslator::DSTST(ppu_opcode_t)
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
	UNK(op);
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
	if (op.rc) SetCrFieldSignedCmp(0, GetGpr(op.rd), m_ir->getInt64(0));
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::DIVWU(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra, 32);
	const auto b = GetGpr(op.rb, 32);
	const auto o = IsZero(b);
	const auto result = m_ir->CreateUDiv(a, m_ir->CreateSelect(o, m_ir->getInt32(0xffffffff), b));
	SetGpr(op.rd, m_ir->CreateSelect(o, m_ir->getInt32(0), result));
	if (op.rc) SetCrFieldSignedCmp(0, GetGpr(op.rd), m_ir->getInt64(0));
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
	if (op.rc) SetCrFieldSignedCmp(0, GetGpr(op.rd), m_ir->getInt64(0));
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::DIVW(ppu_opcode_t op)
{
	const auto a = GetGpr(op.ra, 32);
	const auto b = GetGpr(op.rb, 32);
	const auto o = m_ir->CreateOr(IsZero(b), m_ir->CreateAnd(m_ir->CreateICmpEQ(a, m_ir->getInt32(s32{smin})), IsOnes(b)));
	const auto result = m_ir->CreateSDiv(a, m_ir->CreateSelect(o, m_ir->getInt32(s32{smin}), b));
	SetGpr(op.rd, m_ir->CreateSelect(o, m_ir->getInt32(0), result));
	if (op.rc) SetCrFieldSignedCmp(0, GetGpr(op.rd), m_ir->getInt64(0));
	if (op.oe) SetOverflow(o);
}

void PPUTranslator::LVLX(ppu_opcode_t op)
{
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	const auto data = ReadMemory(m_ir->CreateAnd(addr, ~0xfull), GetType<u8[16]>(), m_is_be, 16);
	set_vr(op.vd, pshufb(value<u8[16]>(data), build<u8[16]>(127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112) + vsplat<u8[16]>(trunc<u8>(value<u64>(addr) & 0xf))));
}

void PPUTranslator::LDBRX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u64>(), false));
}

void PPUTranslator::LSWX(ppu_opcode_t op)
{
	Call(GetType<void>(), "__lswx_not_supported", m_ir->getInt32(op.rd), RegLoad(m_cnt), op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb));
}

void PPUTranslator::LWBRX(ppu_opcode_t op)
{
	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetType<u32>(), false));
}

void PPUTranslator::LFSX(ppu_opcode_t op)
{
	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u && op.rb != 1u && op.rb != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation
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
	const auto offset = eval(trunc<u8>(value<u64>(addr) & 0xf));

	// Read from instruction address if offset is 0, this prevents accessing potentially bad memory from addr (because no actual memory is dereferenced)
	const auto data = ReadMemory(m_ir->CreateAnd(m_ir->CreateSelect(m_ir->CreateIsNull(offset.value), m_reloc ? m_seg0 : GetAddr(0), addr), ~0xfull), GetType<u8[16]>(), m_is_be, 16);
	set_vr(op.vd, pshufb(value<u8[16]>(data), build<u8[16]>(255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240) + vsplat<u8[16]>(offset)));
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
	// lwsync: Acq/Release barrier (but not really it seems from observing libsre.sprx)
	m_ir->CreateFence(op.l10 && false ? AtomicOrdering::AcquireRelease : AtomicOrdering::SequentiallyConsistent);
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
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	const auto data = pshufb(get_vr<u8[16]>(op.vs), build<u8[16]>(127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112) + vsplat<u8[16]>(trunc<u8>(value<u64>(addr) & 0xf)));
	const auto mask = bitcast<bool[16]>(splat<u16>(0xffff) << trunc<u16>(value<u64>(addr) & 0xf));
	const auto ptr = value<u8(*)[16]>(GetMemory(m_ir->CreateAnd(addr, ~0xfull)));
	const auto align = splat<u32>(16);
	eval(llvm_calli<void, decltype(data), decltype(ptr), decltype(align), decltype(mask)>{"llvm.masked.store.v16i8.p0", {data, ptr, align, mask}});
}

void PPUTranslator::STDBRX(ppu_opcode_t op)
{
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), GetGpr(op.rs), false);
}

void PPUTranslator::STSWX(ppu_opcode_t op)
{
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
	const auto addr = op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb);
	const auto data = pshufb(get_vr<u8[16]>(op.vs), build<u8[16]>(255, 254, 253, 252, 251, 250, 249, 248, 247, 246, 245, 244, 243, 242, 241, 240) + vsplat<u8[16]>(trunc<u8>(value<u64>(addr) & 0xf)));
	const auto mask = bitcast<bool[16]>(trunc<u16>(splat<u64>(0xffff) << (value<u64>(addr) & 0xf) >> 16));
	const auto ptr = value<u8(*)[16]>(GetMemory(m_ir->CreateAnd(addr, ~0xfull)));
	const auto align = splat<u32>(16);
	eval(llvm_calli<void, decltype(data), decltype(ptr), decltype(align), decltype(mask)>{"llvm.masked.store.v16i8.p0", {data, ptr, align, mask}});
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

void PPUTranslator::DSS(ppu_opcode_t)
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

void PPUTranslator::EIEIO(ppu_opcode_t)
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

void PPUTranslator::ICBI(ppu_opcode_t)
{
}

void PPUTranslator::DCBZ(ppu_opcode_t op)
{
	const auto addr = m_ir->CreateAnd(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), GetGpr(op.rb)) : GetGpr(op.rb), -128);

	if (g_cfg.core.accurate_cache_line_stores)
	{
		Call(GetType<void>(), "__dcbz", addr);
	}
	else
	{
		Call(GetType<void>(), "llvm.memset.p0.i32", GetMemory(addr), m_ir->getInt8(0), m_ir->getInt32(128), m_ir->getFalse());
	}
}

void PPUTranslator::LWZ(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation
	m_may_be_mmio &= op.simm16 == 0 || spu_thread::test_is_problem_state_register_offset(op.uimm16, true, false); // Either exact MMIO address or MMIO base with completing s16 address offset

	if (m_may_be_mmio)
	{
		struct instructions_data
		{
			be_t<u32> insts[3];
		};

		// Quick invalidation: expect exact MMIO address, so if the register is being reused with different offset than it's likely not MMIO
		if (auto ptr = m_info.get_ptr<instructions_data>(::narrow<u32>(m_addr + 4 + (m_reloc ? m_reloc->addr : 0))))
		{
			for (u32 inst : ptr->insts)
			{
				ppu_opcode_t test_op{inst};

				if (test_op.simm16 == op.simm16 || test_op.ra != op.ra)
				{
					// Same offset (at least according to this test) or different register
					continue;
				}

				if (op.simm16 && spu_thread::test_is_problem_state_register_offset(test_op.uimm16, true, false))
				{
					// Found register reuse with different MMIO offset
					continue;
				}

				switch (g_ppu_itype.decode(inst))
				{
				case ppu_itype::LWZ:
				case ppu_itype::STW:
				{
					// Not MMIO
					m_may_be_mmio = false;
					break;
				}
				default: break;
				}
			}
		}
	}

	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<u32>()));
}

void PPUTranslator::LWZU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation
	m_may_be_mmio &= op.simm16 == 0 || spu_thread::test_is_problem_state_register_offset(op.uimm16, true, false); // Either exact MMIO address or MMIO base with completing s16 address offset

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetGpr(op.rd, ReadMemory(addr, GetType<u32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LBZ(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<u8>()));
}

void PPUTranslator::LBZU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation
	m_may_be_mmio &= op.simm16 == 0 || spu_thread::test_is_problem_state_register_offset(op.uimm16, false, true); // Either exact MMIO address or MMIO base with completing s16 address offset

	if (m_may_be_mmio)
	{
		struct instructions_data
		{
			be_t<u32> insts[3];
		};

		// Quick invalidation: expect exact MMIO address, so if the register is being reused with different offset than it's likely not MMIO
		if (auto ptr = m_info.get_ptr<instructions_data>(::narrow<u32>(m_addr + 4 + (m_reloc ? m_reloc->addr : 0))))
		{
			for (u32 inst : ptr->insts)
			{
				ppu_opcode_t test_op{inst};

				if (test_op.simm16 == op.simm16 || test_op.ra != op.ra)
				{
					// Same offset (at least according to this test) or different register
					continue;
				}

				if (op.simm16 && spu_thread::test_is_problem_state_register_offset(test_op.uimm16, false, true))
				{
					// Found register reuse with different MMIO offset
					continue;
				}

				switch (g_ppu_itype.decode(inst))
				{
				case ppu_itype::LWZ:
				case ppu_itype::STW:
				{
					// Not MMIO
					m_may_be_mmio = false;
					break;
				}
				default: break;
				}
			}
		}
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u);// Stack register and TLS address register are unlikely to be used in MMIO address calculatio
	m_may_be_mmio &= op.simm16 == 0 || spu_thread::test_is_problem_state_register_offset(op.uimm16, false, true); // Either exact MMIO address or MMIO base with completing s16 address offset

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	WriteMemory(addr, GetGpr(op.rs, 32));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STB(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetGpr(op.rs, 8));
}

void PPUTranslator::STBU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<u16>()));
}

void PPUTranslator::LHZU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetGpr(op.rd, SExt(ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<s16>()), GetType<s64>()));
}

void PPUTranslator::LHAU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetGpr(op.rs, 16));
}

void PPUTranslator::STHU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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
	m_may_be_mmio &= op.rd == 31u && (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculatio

	for (u32 i = 0; i < 32 - op.rd; i++)
	{
		SetGpr(i + op.rd, ReadMemory(op.ra ? m_ir->CreateAdd(m_ir->getInt64(op.simm16 + i * 4), GetGpr(op.ra)) : m_ir->getInt64(op.simm16 + i * 4), GetType<u32>()));
	}
}

void PPUTranslator::STMW(ppu_opcode_t op)
{
	m_may_be_mmio &= op.rs == 31u && (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculatio

	for (u32 i = 0; i < 32 - op.rs; i++)
	{
		WriteMemory(op.ra ? m_ir->CreateAdd(m_ir->getInt64(op.simm16 + i * 4), GetGpr(op.ra)) : m_ir->getInt64(op.simm16 + i * 4), GetGpr(i + op.rs, 32));
	}
}

void PPUTranslator::LFS(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculatio
	m_may_be_mmio &= op.simm16 == 0 || spu_thread::test_is_problem_state_register_offset(op.uimm16, true, false); // Either exact MMIO address or MMIO base with completing s16 address offset

	SetFpr(op.frd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<f32>()));
}

void PPUTranslator::LFSU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculatio
	m_may_be_mmio &= op.simm16 == 0 || spu_thread::test_is_problem_state_register_offset(op.uimm16, true, false); // Either exact MMIO address or MMIO base with completing s16 address offset

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	SetFpr(op.frd, ReadMemory(addr, GetType<f32>()));
	SetGpr(op.ra, addr);
}

void PPUTranslator::LFD(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	SetFpr(op.frd, ReadMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetType<f64>()));
}

void PPUTranslator::LFDU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}
	else
	{
		m_may_be_mmio &= op.simm16 == 0 || spu_thread::test_is_problem_state_register_offset(op.uimm16, false, true); // Either exact MMIO address or MMIO base with completing s16 address offset
	}

	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation
	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetFpr(op.frs, 32));
}

void PPUTranslator::STFSU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}
	else
	{
		m_may_be_mmio &= op.simm16 == 0 || spu_thread::test_is_problem_state_register_offset(op.uimm16, false, true); // Either exact MMIO address or MMIO base with completing s16 address offset
	}

	m_may_be_mmio &= (op.ra != 1u && op.ra != 13u); // Stack register and TLS address register are unlikely to be used in MMIO address calculation

	const auto addr = m_ir->CreateAdd(GetGpr(op.ra), imm);
	WriteMemory(addr, GetFpr(op.frs, 32));
	SetGpr(op.ra, addr);
}

void PPUTranslator::STFD(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
	{
		imm = SExt(ReadMemory(GetAddr(+2), GetType<u16>()), GetType<u64>());
		m_rel = nullptr;
	}

	WriteMemory(op.ra ? m_ir->CreateAdd(GetGpr(op.ra), imm) : imm, GetFpr(op.frs));
}

void PPUTranslator::STFDU(ppu_opcode_t op)
{
	Value* imm = m_ir->getInt64(op.simm16);

	if (m_rel && (m_rel->type >= 4u && m_rel->type <= 6u))
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
	if (!m_fres_table)
	{
		m_fres_table = new GlobalVariable(*m_module, ArrayType::get(GetType<u32>(), 128), true, GlobalValue::PrivateLinkage, ConstantDataArray::get(m_context, ppu_fres_mantissas));
	}

	const auto a = GetFpr(op.frb);
	const auto b = bitcast<u64>(a);
	const auto n = m_ir->CreateFCmpUNO(a, a); // test for NaN
	const auto e = m_ir->CreateAnd(m_ir->CreateLShr(b, 52), 0x7ff); // double exp
	const auto i = m_ir->CreateAnd(m_ir->CreateLShr(b, 45), 0x7f); // mantissa LUT index
	const auto ptr = dyn_cast<GetElementPtrInst>(m_ir->CreateGEP(m_fres_table->getValueType(), m_fres_table, {m_ir->getInt64(0), i}));
	assert(ptr->getResultElementType() == get_type<u32>());
	const auto m = m_ir->CreateShl(ZExt(m_ir->CreateLoad(ptr->getResultElementType(), ptr)), 29);
	const auto c = m_ir->CreateICmpUGE(e, m_ir->getInt64(0x3ff + 0x80)); // test for INF
	const auto x = m_ir->CreateShl(m_ir->CreateSub(m_ir->getInt64(0x7ff - 2), e), 52);
	const auto s = m_ir->CreateSelect(c, m_ir->getInt64(0), m_ir->CreateOr(x, m));
	const auto r = bitcast<f64>(m_ir->CreateSelect(n, m_ir->CreateOr(b, 0x8'0000'0000'0000), m_ir->CreateOr(s, m_ir->CreateAnd(b, 0x8000'0000'0000'0000))));
	SetFpr(op.frd, m_ir->CreateFPTrunc(r, GetType<f32>()));

	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_fr);
	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_fi);
	//m_ir->CreateStore(GetUndef<bool>(), m_fpscr_xx);
	//SetFPSCRException(m_fpscr_ox, Call(GetType<bool>(), m_pure_attr, "__fres_get_ox", b));
	//SetFPSCRException(m_fpscr_ux, Call(GetType<bool>(), m_pure_attr, "__fres_get_ux", b));
	//SetFPSCRException(m_fpscr_zx, Call(GetType<bool>(), m_pure_attr, "__fres_get_zx", b));
	//SetFPSCRException(m_fpscr_vxsnan, Call(GetType<bool>(), m_pure_attr, "__fres_get_vxsnan", b));
	SetFPRF(r, op.rc != 0);
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

	llvm::Value* result;
	if (g_cfg.core.use_accurate_dfma)
	{
		result = m_ir->CreateCall(get_intrinsic<f64>(llvm::Intrinsic::fma), {a, c, b});
	}
	else
	{
		result = m_ir->CreateFAdd(m_ir->CreateFMul(a, c), b);
	}

	SetFpr(op.frd, m_ir->CreateFPTrunc(result, GetType<f32>()));

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

	llvm::Value* result;
	if (g_cfg.core.use_accurate_dfma)
	{
		result = m_ir->CreateCall(get_intrinsic<f64>(llvm::Intrinsic::fma), {a, c, m_ir->CreateFNeg(b)});
	}
	else
	{
		result = m_ir->CreateFSub(m_ir->CreateFMul(a, c), b);
	}

	SetFpr(op.frd, m_ir->CreateFPTrunc(result, GetType<f32>()));

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

	llvm::Value* result;
	if (g_cfg.core.use_accurate_dfma)
	{
		result = m_ir->CreateCall(get_intrinsic<f64>(llvm::Intrinsic::fma), {a, c, m_ir->CreateFNeg(b)});
	}
	else
	{
		result = m_ir->CreateFSub(m_ir->CreateFMul(a, c), b);
	}

	SetFpr(op.frd, m_ir->CreateFPTrunc(m_ir->CreateFNeg(result), GetType<f32>()));

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

	llvm::Value* result;
	if (g_cfg.core.use_accurate_dfma)
	{
		result = m_ir->CreateCall(get_intrinsic<f64>(llvm::Intrinsic::fma), {a, c, b});
	}
	else
	{
		result = m_ir->CreateFAdd(m_ir->CreateFMul(a, c), b);
	}

	SetFpr(op.frd, m_ir->CreateFPTrunc(m_ir->CreateFNeg(result), GetType<f32>()));

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
	SetFPSCRBit(op.crbd, m_ir->getTrue(), true);

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::MCRFS(ppu_opcode_t op)
{
	const auto lt = GetFPSCRBit(op.crfs * 4 + 0);
	const auto gt = GetFPSCRBit(op.crfs * 4 + 1);
	const auto eq = GetFPSCRBit(op.crfs * 4 + 2);
	const auto un = GetFPSCRBit(op.crfs * 4 + 3);
	SetCrField(op.crfd, lt, gt, eq, un);
}

void PPUTranslator::MTFSB0(ppu_opcode_t op)
{
	SetFPSCRBit(op.crbd, m_ir->getFalse(), false);

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::MTFSFI(ppu_opcode_t op)
{
	SetFPSCRBit(op.crfd * 4 + 0, m_ir->getInt1((op.i & 8) != 0), false);

	if (op.crfd != 0)
	{
		SetFPSCRBit(op.crfd * 4 + 1, m_ir->getInt1((op.i & 4) != 0), false);
		SetFPSCRBit(op.crfd * 4 + 2, m_ir->getInt1((op.i & 2) != 0), false);
	}

	SetFPSCRBit(op.crfd * 4 + 3, m_ir->getInt1((op.i & 1) != 0), false);

	if (op.rc) SetCrFieldFPCC(1);
}

void PPUTranslator::MFFS(ppu_opcode_t op)
{
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
	const auto value = GetFpr(op.frb, 32, true);

	for (u32 i = 16; i < 20; i++)
	{
		if ((op.flm & (128 >> (i / 4))) != 0)
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
#if defined(ARCH_X64)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s32>(), "llvm.x86.sse2.cvtsd2si", m_ir->CreateInsertElement(GetUndef<f64[2]>(), b, u64{0}))));
#elif defined(ARCH_ARM64)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s32>(), "llvm.aarch64.neon.fcvtns.i32.f64", b)));
#endif

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
#if defined(ARCH_X64)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s32>(), "llvm.x86.sse2.cvttsd2si", m_ir->CreateInsertElement(GetUndef<f64[2]>(), b, u64{0}))));
#elif defined(ARCH_ARM64)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s32>(), "llvm.aarch64.neon.fcvtzs.i32.f64", b)));
#endif
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
	if (!m_frsqrte_table)
	{
		m_frsqrte_table = new GlobalVariable(*m_module, ArrayType::get(GetType<u32>(), 0x8000), true, GlobalValue::PrivateLinkage, ConstantDataArray::get(m_context, ppu_frqrte_lut.data));
	}

	const auto b = m_ir->CreateBitCast(GetFpr(op.frb), GetType<u64>());
	const auto ptr = dyn_cast<GetElementPtrInst>(m_ir->CreateGEP(m_frsqrte_table->getValueType(), m_frsqrte_table, {m_ir->getInt64(0), m_ir->CreateLShr(b, 49)}));
	assert(ptr->getResultElementType() == get_type<u32>());
	const auto v = m_ir->CreateLoad(ptr->getResultElementType(), ptr);
	const auto result = m_ir->CreateBitCast(m_ir->CreateShl(ZExt(v), 32), GetType<f64>());
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

	llvm::Value* result;
	if (g_cfg.core.use_accurate_dfma)
	{
		result = m_ir->CreateCall(get_intrinsic<f64>(llvm::Intrinsic::fma), {a, c, m_ir->CreateFNeg(b)});
	}
	else
	{
		result = m_ir->CreateFSub(m_ir->CreateFMul(a, c), b);
	}

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

	llvm::Value* result;
	if (g_cfg.core.use_accurate_dfma)
	{
		result = m_ir->CreateCall(get_intrinsic<f64>(llvm::Intrinsic::fma), { a, c, b });
	}
	else
	{
		result = m_ir->CreateFAdd(m_ir->CreateFMul(a, c), b);
	}

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

	llvm::Value* result;
	if (g_cfg.core.use_accurate_dfma)
	{
		result = m_ir->CreateCall(get_intrinsic<f64>(llvm::Intrinsic::fma), {a, c, m_ir->CreateFNeg(b)});
	}
	else
	{
		result = m_ir->CreateFSub(m_ir->CreateFMul(a, c), b);
	}

	SetFpr(op.frd, m_ir->CreateFNeg(result));

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

	llvm::Value* result;
	if (g_cfg.core.use_accurate_dfma)
	{
		result = m_ir->CreateCall(get_intrinsic<f64>(llvm::Intrinsic::fma), {a, c, b});
	}
	else
	{
		result = m_ir->CreateFAdd(m_ir->CreateFMul(a, c), b);
	}

	SetFpr(op.frd, m_ir->CreateFNeg(result));

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
#if defined(ARCH_X64)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s64>(), "llvm.x86.sse2.cvtsd2si64", m_ir->CreateInsertElement(GetUndef<f64[2]>(), b, u64{0}))));
#elif defined(ARCH_ARM64)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s64>(), "llvm.aarch64.neon.fcvtns.i64.f64", b)));
#endif


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
#if defined(ARCH_X64)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s64>(), "llvm.x86.sse2.cvttsd2si64", m_ir->CreateInsertElement(GetUndef<f64[2]>(), b, u64{0}))));
#elif defined(ARCH_ARM64)
	SetFpr(op.frd, m_ir->CreateXor(xormask, Call(GetType<s64>(), "llvm.aarch64.neon.fcvtzs.i64.f64", b)));
#endif
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
	Call(GetType<void>(), "__error", m_thread, GetAddr(), m_ir->getInt32(op.opcode));
	//Call(GetType<void>(), "__escape", m_thread)->setTailCall();
	m_ir->CreateRetVoid();
}


Value* PPUTranslator::GetGpr(u32 r, u32 num_bits)
{
	return Trunc(RegLoad(m_gpr[r]), m_ir->getIntNTy(num_bits));
}

void PPUTranslator::SetGpr(u32 r, Value* value)
{
	RegStore(ZExt(value, GetType<u64>()), m_gpr[r]);
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
		return Trunc(bitcast(value, GetType<u64>()), m_ir->getIntNTy(bits));
	}
}

void PPUTranslator::SetFpr(u32 r, Value* val)
{
	const auto f64_val =
		val->getType() == GetType<s32>() ? bitcast(SExt(val), GetType<f64>()) :
		val->getType() == GetType<s64>() ? bitcast(val, GetType<f64>()) :
		val->getType() == GetType<f32>() ? m_ir->CreateFPExt(val, GetType<f64>()) : val;

	RegStore(f64_val, m_fpr[r]);
}

Value* PPUTranslator::GetVr(u32 vr, VrType type)
{
	const auto value = RegLoad(m_vr[vr]);

	llvm::Type* _type{};

	switch (type)
	{
	case VrType::vi32: _type = GetType<u32[4]>(); break;
	case VrType::vi8 : _type = GetType<u8[16]>(); break;
	case VrType::vi16: _type = GetType<u16[8]>(); break;
	case VrType::vf  : _type = GetType<f32[4]>(); break;
	case VrType::i128: _type = GetType<u128>(); break;
	default: ensure(false);
	}

	return bitcast(value, _type);
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

	ensure(value->getType()->getPrimitiveSizeInBits() == 128);
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

void PPUTranslator::SetFPRF(Value* value, bool /*set_cr*/)
{
	//const bool is32 =
		value->getType()->isFloatTy() ? true :
		value->getType()->isDoubleTy() ? false : ensure(false);

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

void PPUTranslator::SetFPSCR_FR(Value* /*value*/)
{
	//m_ir->CreateStore(value, m_fpscr_fr);
}

void PPUTranslator::SetFPSCR_FI(Value* /*value*/)
{
	//m_ir->CreateStore(value, m_fpscr_fi);
	//SetFPSCRException(m_fpscr_xx, value);
}

void PPUTranslator::SetFPSCRException(Value* /*ptr*/, Value* /*value*/)
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

void PPUTranslator::SetFPSCRBit(u32 n, Value* value, bool /*update_fx*/)
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

Value* PPUTranslator::CheckTrapCondition(u32 to, Value* left, Value* right)
{
	if ((to & 0x3) == 0x3 || (to & 0x18) == 0x18)
	{
		// Not-equal check or always-true
		return to & 0x4 ? m_ir->getTrue() : m_ir->CreateICmpNE(left, right);
	}

	Value* trap_condition = nullptr;

	auto add_condition = [&](Value* cond)
	{
		if (!trap_condition)
		{
			trap_condition = cond;
			return;
		}

		trap_condition = m_ir->CreateOr(trap_condition, cond);
	};

	if (to & 0x10) add_condition(m_ir->CreateICmpSLT(left, right));
	if (to & 0x8) add_condition(m_ir->CreateICmpSGT(left, right));
	if (to & 0x4) add_condition(m_ir->CreateICmpEQ(left, right));
	if (to & 0x2) add_condition(m_ir->CreateICmpULT(left, right));
	if (to & 0x1) add_condition(m_ir->CreateICmpUGT(left, right));

	return trap_condition ? trap_condition : m_ir->getFalse();
}

void PPUTranslator::Trap()
{
	Call(GetType<void>(), "__trap", m_thread, GetAddr());
	//Call(GetType<void>(), "__escape", m_thread)->setTailCall();
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

void PPUTranslator::build_interpreter()
{
#define BUILD_VEC_INST(i) { \
		m_function = llvm::cast<llvm::Function>(m_module->getOrInsertFunction("op_" #i, get_type<void>(), m_ir->getPtrTy()).getCallee()); \
		std::fill(std::begin(m_globals), std::end(m_globals), nullptr); \
		std::fill(std::begin(m_locals), std::end(m_locals), nullptr); \
		IRBuilder<> irb(BasicBlock::Create(m_context, "__entry", m_function)); \
		m_ir = &irb; \
		m_thread = m_function->getArg(0); \
		ppu_opcode_t op{}; \
		op.vd = 0; \
		op.va = 1; \
		op.vb = 2; \
		op.vc = 3; \
		this->i(op); \
		FlushRegisters(); \
		m_ir->CreateRetVoid(); \
		run_transforms(*m_function); \
	}

	BUILD_VEC_INST(VADDCUW);
	BUILD_VEC_INST(VADDFP);
	BUILD_VEC_INST(VADDSBS);
	BUILD_VEC_INST(VADDSHS);
	BUILD_VEC_INST(VADDSWS);
	BUILD_VEC_INST(VADDUBM);
	BUILD_VEC_INST(VADDUBS);
	BUILD_VEC_INST(VADDUHM);
	BUILD_VEC_INST(VADDUHS);
	BUILD_VEC_INST(VADDUWM);
	BUILD_VEC_INST(VADDUWS);
	BUILD_VEC_INST(VAND);
	BUILD_VEC_INST(VANDC);
	BUILD_VEC_INST(VAVGSB);
	BUILD_VEC_INST(VAVGSH);
	BUILD_VEC_INST(VAVGSW);
	BUILD_VEC_INST(VAVGUB);
	BUILD_VEC_INST(VAVGUH);
	BUILD_VEC_INST(VAVGUW);
	BUILD_VEC_INST(VCFSX);
	BUILD_VEC_INST(VCFUX);
	BUILD_VEC_INST(VCMPBFP);
	BUILD_VEC_INST(VCMPBFP_);
	BUILD_VEC_INST(VCMPEQFP);
	BUILD_VEC_INST(VCMPEQFP_);
	BUILD_VEC_INST(VCMPEQUB);
	BUILD_VEC_INST(VCMPEQUB_);
	BUILD_VEC_INST(VCMPEQUH);
	BUILD_VEC_INST(VCMPEQUH_);
	BUILD_VEC_INST(VCMPEQUW);
	BUILD_VEC_INST(VCMPEQUW_);
	BUILD_VEC_INST(VCMPGEFP);
	BUILD_VEC_INST(VCMPGEFP_);
	BUILD_VEC_INST(VCMPGTFP);
	BUILD_VEC_INST(VCMPGTFP_);
	BUILD_VEC_INST(VCMPGTSB);
	BUILD_VEC_INST(VCMPGTSB_);
	BUILD_VEC_INST(VCMPGTSH);
	BUILD_VEC_INST(VCMPGTSH_);
	BUILD_VEC_INST(VCMPGTSW);
	BUILD_VEC_INST(VCMPGTSW_);
	BUILD_VEC_INST(VCMPGTUB);
	BUILD_VEC_INST(VCMPGTUB_);
	BUILD_VEC_INST(VCMPGTUH);
	BUILD_VEC_INST(VCMPGTUH_);
	BUILD_VEC_INST(VCMPGTUW);
	BUILD_VEC_INST(VCMPGTUW_);
	BUILD_VEC_INST(VCTSXS);
	BUILD_VEC_INST(VCTUXS);
	BUILD_VEC_INST(VEXPTEFP);
	BUILD_VEC_INST(VLOGEFP);
	BUILD_VEC_INST(VMADDFP);
	BUILD_VEC_INST(VMAXFP);
	BUILD_VEC_INST(VMAXSB);
	BUILD_VEC_INST(VMAXSH);
	BUILD_VEC_INST(VMAXSW);
	BUILD_VEC_INST(VMAXUB);
	BUILD_VEC_INST(VMAXUH);
	BUILD_VEC_INST(VMAXUW);
	BUILD_VEC_INST(VMHADDSHS);
	BUILD_VEC_INST(VMHRADDSHS);
	BUILD_VEC_INST(VMINFP);
	BUILD_VEC_INST(VMINSB);
	BUILD_VEC_INST(VMINSH);
	BUILD_VEC_INST(VMINSW);
	BUILD_VEC_INST(VMINUB);
	BUILD_VEC_INST(VMINUH);
	BUILD_VEC_INST(VMINUW);
	BUILD_VEC_INST(VMLADDUHM);
	BUILD_VEC_INST(VMRGHB);
	BUILD_VEC_INST(VMRGHH);
	BUILD_VEC_INST(VMRGHW);
	BUILD_VEC_INST(VMRGLB);
	BUILD_VEC_INST(VMRGLH);
	BUILD_VEC_INST(VMRGLW);
	BUILD_VEC_INST(VMSUMMBM);
	BUILD_VEC_INST(VMSUMSHM);
	BUILD_VEC_INST(VMSUMSHS);
	BUILD_VEC_INST(VMSUMUBM);
	BUILD_VEC_INST(VMSUMUHM);
	BUILD_VEC_INST(VMSUMUHS);
	BUILD_VEC_INST(VMULESB);
	BUILD_VEC_INST(VMULESH);
	BUILD_VEC_INST(VMULEUB);
	BUILD_VEC_INST(VMULEUH);
	BUILD_VEC_INST(VMULOSB);
	BUILD_VEC_INST(VMULOSH);
	BUILD_VEC_INST(VMULOUB);
	BUILD_VEC_INST(VMULOUH);
	BUILD_VEC_INST(VNMSUBFP);
	BUILD_VEC_INST(VNOR);
	BUILD_VEC_INST(VOR);
	BUILD_VEC_INST(VPERM);
	BUILD_VEC_INST(VPKPX);
	BUILD_VEC_INST(VPKSHSS);
	BUILD_VEC_INST(VPKSHUS);
	BUILD_VEC_INST(VPKSWSS);
	BUILD_VEC_INST(VPKSWUS);
	BUILD_VEC_INST(VPKUHUM);
	BUILD_VEC_INST(VPKUHUS);
	BUILD_VEC_INST(VPKUWUM);
	BUILD_VEC_INST(VPKUWUS);
	BUILD_VEC_INST(VREFP);
	BUILD_VEC_INST(VRFIM);
	BUILD_VEC_INST(VRFIN);
	BUILD_VEC_INST(VRFIP);
	BUILD_VEC_INST(VRFIZ);
	BUILD_VEC_INST(VRLB);
	BUILD_VEC_INST(VRLH);
	BUILD_VEC_INST(VRLW);
	BUILD_VEC_INST(VRSQRTEFP);
	BUILD_VEC_INST(VSEL);
	BUILD_VEC_INST(VSL);
	BUILD_VEC_INST(VSLB);
	BUILD_VEC_INST(VSLDOI);
	BUILD_VEC_INST(VSLH);
	BUILD_VEC_INST(VSLO);
	BUILD_VEC_INST(VSLW);
	BUILD_VEC_INST(VSPLTB);
	BUILD_VEC_INST(VSPLTH);
	BUILD_VEC_INST(VSPLTISB);
	BUILD_VEC_INST(VSPLTISH);
	BUILD_VEC_INST(VSPLTISW);
	BUILD_VEC_INST(VSPLTW);
	BUILD_VEC_INST(VSR);
	BUILD_VEC_INST(VSRAB);
	BUILD_VEC_INST(VSRAH);
	BUILD_VEC_INST(VSRAW);
	BUILD_VEC_INST(VSRB);
	BUILD_VEC_INST(VSRH);
	BUILD_VEC_INST(VSRO);
	BUILD_VEC_INST(VSRW);
	BUILD_VEC_INST(VSUBCUW);
	BUILD_VEC_INST(VSUBFP);
	BUILD_VEC_INST(VSUBSBS);
	BUILD_VEC_INST(VSUBSHS);
	BUILD_VEC_INST(VSUBSWS);
	BUILD_VEC_INST(VSUBUBM);
	BUILD_VEC_INST(VSUBUBS);
	BUILD_VEC_INST(VSUBUHM);
	BUILD_VEC_INST(VSUBUHS);
	BUILD_VEC_INST(VSUBUWM);
	BUILD_VEC_INST(VSUBUWS);
	BUILD_VEC_INST(VSUMSWS);
	BUILD_VEC_INST(VSUM2SWS);
	BUILD_VEC_INST(VSUM4SBS);
	BUILD_VEC_INST(VSUM4SHS);
	BUILD_VEC_INST(VSUM4UBS);
	BUILD_VEC_INST(VUPKHPX);
	BUILD_VEC_INST(VUPKHSB);
	BUILD_VEC_INST(VUPKHSH);
	BUILD_VEC_INST(VUPKLPX);
	BUILD_VEC_INST(VUPKLSB);
	BUILD_VEC_INST(VUPKLSH);
	BUILD_VEC_INST(VXOR);
#undef BUILD_VEC_INST


}

#endif
