#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/Memory/Memory.h"
#include "Crypto/sha1.h"

#include "SPUThread.h"
#include "SPUAnalyser.h"
#include "SPUInterpreter.h"
#include "SPUDisAsm.h"
#include "SPURecompiler.h"
#include <algorithm>
#include <mutex>
#include <thread>

extern u64 get_system_time();

const spu_decoder<spu_itype> s_spu_itype;

spu_recompiler_base::spu_recompiler_base(SPUThread& spu)
	: m_spu(spu)
{
	// Initialize lookup table
	spu.jit_dispatcher.fill(&dispatch);
}

spu_recompiler_base::~spu_recompiler_base()
{
}

void spu_recompiler_base::dispatch(SPUThread& spu, void*, u8* rip)
{
	// If check failed after direct branch, patch it with single NOP
	if (rip)
	{
#ifdef _MSC_VER
		*(volatile u64*)(rip) = 0x841f0f;
#else
		__atomic_store_n(reinterpret_cast<u64*>(rip), 0x841f0f, __ATOMIC_RELAXED);
#endif
	}

	const auto func = spu.jit->get(spu.pc);

	// First attempt (load new trampoline and retry)
	if (func != spu.jit_dispatcher[spu.pc / 4])
	{
		spu.jit_dispatcher[spu.pc / 4] = func;
		return;
	}

	// Second attempt (recover from the recursion after repeated unsuccessful trampoline call)
	if (spu.block_counter != spu.block_recover && func != &dispatch)
	{
		spu.block_recover = spu.block_counter;
		return;
	}

	// Compile
	verify(HERE), spu.jit->compile(block(spu, spu.pc, &spu.jit->m_block_info));
	spu.jit_dispatcher[spu.pc / 4] = spu.jit->get(spu.pc);
}

void spu_recompiler_base::branch(SPUThread& spu, void*, u8* rip)
{
	const auto pair = *reinterpret_cast<std::pair<const std::vector<u32>, spu_function_t>**>(rip + 24);

	spu.pc = pair->first[0];

	const auto func = pair->second ? pair->second : spu.jit->compile(pair->first);

	verify(HERE), func, pair->second == func;

	// Overwrite function address
	reinterpret_cast<atomic_t<spu_function_t>*>(rip + 32)->store(func);

	// Overwrite jump to this function with jump to the compiled function
	const s64 rel = reinterpret_cast<u64>(func) - reinterpret_cast<u64>(rip) - 5;

	alignas(8) u8 bytes[8];

	if (rel >= INT32_MIN && rel <= INT32_MAX)
	{
		const s64 rel8 = (rel + 5) - 2;

		if (rel8 >= INT8_MIN && rel8 <= INT8_MAX)
		{
			bytes[0] = 0xeb; // jmp rel8
			bytes[1] = static_cast<s8>(rel8);
			std::memset(bytes + 2, 0x90, 6);
		}
		else
		{
			bytes[0] = 0xe9; // jmp rel32
			std::memcpy(bytes + 1, &rel, 4);
			std::memset(bytes + 5, 0x90, 3);
		}
	}
	else
	{
		bytes[0] = 0xff; // jmp [rip+26]
		bytes[1] = 0x25;
		bytes[2] = 0x1a;
		bytes[3] = 0x00;
		bytes[4] = 0x00;
		bytes[5] = 0x00;
		bytes[6] = 0x90;
		bytes[7] = 0x90;
	}

#ifdef _MSC_VER
	*(volatile u64*)(rip) = *reinterpret_cast<u64*>(+bytes);
#else
	__atomic_store_n(reinterpret_cast<u64*>(rip), *reinterpret_cast<u64*>(+bytes), __ATOMIC_RELAXED);
#endif
}

std::vector<u32> spu_recompiler_base::block(SPUThread& spu, u32 lsa, std::bitset<0x10000>* out_info)
{
	// Block info (local)
	std::bitset<0x10000> block_info{};

	// Select one to use
	std::bitset<0x10000>& blocks = out_info ? *out_info : block_info;

	if (out_info)
	{
		out_info->reset();
	}

	// Result: addr + raw instruction data
	std::vector<u32> result;
	result.reserve(256);
	result.push_back(lsa);
	blocks.set(lsa / 4);

	// Simple block entry workload list
	std::vector<u32> wl;
	wl.push_back(lsa);

	// Value flags (TODO)
	enum class vf : u32
	{
		is_const,
		is_mask,

		__bitset_enum_max
	};

	// Weak constant propagation context (for guessing branch targets)
	std::array<bs_t<vf>, 128> vflags{};

	// Associated constant values for 32-bit preferred slot
	std::array<u32, 128> values;

	if (spu.pc == lsa && g_cfg.core.spu_block_size == spu_block_size_type::giga)
	{
		// TODO: use current register values for speculations
		vflags[0] = +vf::is_const;
		values[0] = spu.gpr[0]._u32[3];
	}

	for (u32 wi = 0; wi < wl.size();)
	{
		const auto next_block = [&]
		{
			// Reset value information
			vflags.fill({});
			wi++;
		};

		const auto add_block = [&](u32 target)
		{
			// Verify validity of the new target (TODO)
			if (target > lsa)
			{
				// Check for redundancy
				if (!blocks[target / 4])
				{
					blocks[target / 4] = true;
					wl.push_back(target);
					return;
				}
			}
		};

		const u32 pos = wl[wi];
		const u32 data = spu._ref<u32>(pos);
		const auto op = spu_opcode_t{data};

		wl[wi] += 4;

		// Analyse instruction
		switch (const auto type = s_spu_itype.decode(data))
		{
		case spu_itype::UNK:
		case spu_itype::DFCEQ:
		case spu_itype::DFCMEQ:
		case spu_itype::DFCGT:
		//case spu_itype::DFCMGT:
		case spu_itype::DFTSV:
		{
			// Stop on invalid instructions (TODO)
			blocks[pos / 4] = true;
			next_block();
			continue;
		}

		case spu_itype::SYNC:
		case spu_itype::DSYNC:
		case spu_itype::STOP:
		case spu_itype::STOPD:
		{
			if (data == 0)
			{
				// Stop before null data
				blocks[pos / 4] = true;
				next_block();
				continue;
			}

			if (g_cfg.core.spu_block_size != spu_block_size_type::giga)
			{
				// Stop on special instructions (TODO)
				next_block();
				break;
			}

			break;
		}

		case spu_itype::IRET:
		{
			next_block();
			break;
		}

		case spu_itype::BI:
		case spu_itype::BISL:
		case spu_itype::BIZ:
		case spu_itype::BINZ:
		case spu_itype::BIHZ:
		case spu_itype::BIHNZ:
		{
			const auto af = vflags[op.ra];
			const auto av = values[op.ra];

			if (type == spu_itype::BISL)
			{
				vflags[op.rt] = +vf::is_const;
				values[op.rt] = pos + 4;
			}

			if (test(af, vf::is_const))
			{
				const u32 target = spu_branch_target(av);

				if (target == pos + 4)
				{
					// Nop (unless BISL)
					break;
				}

				if (type != spu_itype::BISL || g_cfg.core.spu_block_size == spu_block_size_type::giga)
				{
					LOG_WARNING(SPU, "[0x%x] At 0x%x: indirect branch to 0x%x", lsa, pos, target);
					add_block(target);
				}

				if (type == spu_itype::BISL && target < lsa)
				{
					next_block();
					break;
				}
			}
			else if (type == spu_itype::BI && !op.d && !op.e)
			{
				// Analyse jump table (TODO)
				std::basic_string<u32> jt_abs;
				std::basic_string<u32> jt_rel;
				const u32 start = pos + 4;
				const u32 limit = 0x40000;

				for (u32 i = start; i < limit; i += 4)
				{
					const u32 target = spu._ref<u32>(i);

					if (target % 4)
					{
						// Address cannot be misaligned: abort
						break;
					}

					if (target >= lsa && target < limit)
					{
						// Possible jump table entry (absolute)
						jt_abs.push_back(target);
					}

					if (target + start >= lsa && target + start < limit)
					{
						// Possible jump table entry (relative)
						jt_rel.push_back(target + start);
					}

					if (std::max(jt_abs.size(), jt_rel.size()) * 4 + start <= i)
					{
						// Neither type of jump table completes
						break;
					}
				}

				// Add detected jump table blocks (TODO: avoid adding both)
				if (jt_abs.size() >= 3 || jt_rel.size() >= 3)
				{
					if (jt_abs.size() >= jt_rel.size())
					{
						for (u32 target : jt_abs)
						{
							add_block(target);
						}
					}

					if (jt_rel.size() >= jt_abs.size())
					{
						for (u32 target : jt_rel)
						{
							add_block(target);
						}
					}
				}
			}

			if (type == spu_itype::BI || type == spu_itype::BISL || g_cfg.core.spu_block_size == spu_block_size_type::safe)
			{
				if (type == spu_itype::BI || g_cfg.core.spu_block_size != spu_block_size_type::giga)
				{
					next_block();
					break;
				}
			}

			break;
		}

		case spu_itype::BRSL:
		case spu_itype::BRASL:
		{
			const u32 target = spu_branch_target(type == spu_itype::BRASL ? 0 : pos, op.i16);

			vflags[op.rt] = +vf::is_const;
			values[op.rt] = pos + 4;

			if (target == pos + 4)
			{
				// Get next instruction address idiom
				break;
			}

			if (target < lsa || g_cfg.core.spu_block_size != spu_block_size_type::giga)
			{
				// Stop on direct calls
				next_block();
				break;
			}

			if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
			{
				add_block(target);
			}

			break;
		}

		case spu_itype::BR:
		case spu_itype::BRA:
		case spu_itype::BRZ:
		case spu_itype::BRNZ:
		case spu_itype::BRHZ:
		case spu_itype::BRHNZ:
		{
			const u32 target = spu_branch_target(type == spu_itype::BRA ? 0 : pos, op.i16);

			if (target == pos + 4)
			{
				// Nop
				break;
			}

			add_block(target);

			if (type == spu_itype::BR || type == spu_itype::BRA)
			{
				// Stop on direct branches
				next_block();
				break;
			}

			break;
		}

		case spu_itype::HEQ:
		case spu_itype::HEQI:
		case spu_itype::HGT:
		case spu_itype::HGTI:
		case spu_itype::HLGT:
		case spu_itype::HLGTI:
		case spu_itype::HBR:
		case spu_itype::HBRA:
		case spu_itype::HBRR:
		case spu_itype::LNOP:
		case spu_itype::NOP:
		case spu_itype::MTSPR:
		case spu_itype::WRCH:
		case spu_itype::FSCRWR:
		case spu_itype::STQA:
		case spu_itype::STQD:
		case spu_itype::STQR:
		case spu_itype::STQX:
		{
			// Do nothing
			break;
		}

		case spu_itype::IL:
		{
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.si16;
			break;
		}
		case spu_itype::ILA:
		{
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i18;
			break;
		}
		case spu_itype::ILH:
		{
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16 | op.i16;
			break;
		}
		case spu_itype::ILHU:
		{
			vflags[op.rt] = +vf::is_const;
			values[op.rt] = op.i16 << 16;
			break;
		}
		case spu_itype::IOHL:
		{
			values[op.rt] = values[op.rt] | op.i16;
			break;
		}
		case spu_itype::ORI:
		{
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] | op.si10;
			break;
		}
		case spu_itype::OR:
		{
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] | values[op.rb];
			break;
		}
		case spu_itype::AI:
		{
			vflags[op.rt] = vflags[op.ra] & vf::is_const;
			values[op.rt] = values[op.ra] + op.si10;
			break;
		}
		case spu_itype::A:
		{
			vflags[op.rt] = vflags[op.ra] & vflags[op.rb] & vf::is_const;
			values[op.rt] = values[op.ra] + values[op.rb];
			break;
		}
		default:
		{
			// Unconst
			vflags[type & spu_itype::_quadrop ? +op.rt4 : +op.rt] = {};
			break;
		}
		}

		// Insert raw instruction value
		if (result.size() - 1 <= (pos - lsa) / 4)
		{
			if (result.size() - 1 < (pos - lsa) / 4)
			{
				result.resize((pos - lsa) / 4 + 1);
			}

			result.emplace_back(se_storage<u32>::swap(data));
		}
		else if (u32& raw_val = result[(pos - lsa) / 4 + 1])
		{
			verify(HERE), raw_val == se_storage<u32>::swap(data);
		}
		else
		{
			raw_val = se_storage<u32>::swap(data);
		}
	}

	if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
	{
		// Check holes in safe mode (TODO)
		u32 valid_size = 0;

		for (u32 i = 1; i < result.size(); i++)
		{
			if (result[i] == 0)
			{
				const u32 pos = lsa + (i - 1) * 4;
				const u32 data = spu._ref<u32>(pos);
				const auto type = s_spu_itype.decode(data);

				// Allow only NOP or LNOP instructions in holes
				if (type == spu_itype::NOP || type == spu_itype::LNOP)
				{
					if (i + 1 < result.size())
					{
						continue;
					}
				}

				result.resize(valid_size + 1);
				break;
			}
			else
			{
				valid_size = i;
			}
		}
	}

	if (result.size() == 1)
	{
		// Blocks starting from 0x0 or invalid instruction won't be compiled, may need special interpreter fallback
		result.clear();
	}

	return result;
}

#include "Emu/CPU/CPUTranslator.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Analysis/Lint.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Vectorize.h"
#include "Utilities/JIT.h"

class spu_llvm_runtime
{
	shared_mutex m_mutex;

	// All functions
	std::map<std::vector<u32>, spu_function_t> m_map;

	// All dispatchers
	std::array<atomic_t<spu_function_t>, 0x10000> m_dispatcher;

	// JIT instance (TODO: use small code model)
	jit_compiler m_jit{{}, jit_compiler::cpu(g_cfg.core.llvm_cpu), true};

	friend class spu_llvm_recompiler;

public:
	spu_llvm_runtime()
	{
		LOG_SUCCESS(SPU, "SPU Recompiler Runtime (LLVM) initialized...");

		// Initialize lookup table
		for (auto& v : m_dispatcher)
		{
			v.raw() = &spu_recompiler_base::dispatch;
		}

		// Initialize "empty" block
		m_map[std::vector<u32>()] = &spu_recompiler_base::dispatch;
	}
};

class spu_llvm_recompiler : public spu_recompiler_base, public cpu_translator
{
	std::shared_ptr<spu_llvm_runtime> m_spurt;

	llvm::Function* m_function;

	using m_module = void;

	llvm::Value* m_thread;
	llvm::Value* m_lsptr;

	llvm::BasicBlock* m_stop;

	std::array<std::pair<llvm::Value*, llvm::Value*>, 128> m_gpr;
	std::array<llvm::Instruction*, 128> m_flush_gpr;

	std::map<u32, llvm::BasicBlock*> m_instr_map;

	template <typename T>
	llvm::Value* _ptr(llvm::Value* base, u32 offset, std::string name = "")
	{
		const auto off = m_ir->CreateAdd(base, m_ir->getInt64(offset));
		const auto ptr = m_ir->CreateIntToPtr(off, get_type<T>()->getPointerTo(), name);
		return ptr;
	}

	template <typename T, typename... Args>
	llvm::Value* spu_ptr(Args... offset_args)
	{
		return _ptr<T>(m_thread, ::offset32(offset_args...));
	}

	template <typename T = u32[4]>
	auto& init_vr(u32 index)
	{
		auto& gpr = m_gpr.at(index);

		if (!gpr.first)
		{
			// Save and restore current insert point if necessary
			const auto block_cur = m_ir->GetInsertBlock();

			// Emit register pointer at the beginning of function
			m_ir->SetInsertPoint(&*m_function->begin()->getFirstInsertionPt());
			gpr.first = _ptr<T>(m_thread, ::offset32(&SPUThread::gpr, index), fmt::format("Reg$%u", index));
			m_ir->SetInsertPoint(block_cur);
		}

		return gpr;
	}

	template <typename T = u32[4]>
	value_t<T> get_vr(u32 index)
	{
		auto& gpr = init_vr<T>(index);

		if (!gpr.second)
		{
			gpr.second = m_ir->CreateLoad(gpr.first, fmt::format("Load$%u", index));
		}

		value_t<T> r;
		r.value = m_ir->CreateBitCast(gpr.second, get_type<T>());
		return r;
	}

	template <typename T>
	void set_vr(u32 index, T expr)
	{
		auto& gpr = init_vr<typename T::type>(index);

		gpr.second = expr.eval(m_ir);

		// Remember last insertion point for flush
		if (m_ir->GetInsertBlock()->empty())
		{
			// Insert dummy instruction if empty
			m_flush_gpr.at(index) = llvm::cast<llvm::Instruction>(m_ir->CreateAdd(m_thread, m_ir->getInt64(8)));
		}
		else
		{
			m_flush_gpr.at(index) = m_ir->GetInsertBlock()->end()->getPrevNode();
		}
	}

	void flush(std::pair<llvm::Value*, llvm::Value*>& reg, llvm::Instruction*& flush_reg)
	{
		if (reg.first && reg.second && flush_reg)
		{
			// Save and restore current insert point if necessary
			const auto block_cur = m_ir->GetInsertBlock();

			// Try to emit store immediately after its last use
			if (const auto next = flush_reg->getNextNode())
			{
				m_ir->SetInsertPoint(next);
			}

			m_ir->CreateStore(m_ir->CreateBitCast(reg.second, reg.first->getType()->getPointerElementType()), reg.first);
			m_ir->SetInsertPoint(block_cur);
		}

		// Unregister store
		flush_reg = nullptr;

		// Invalidate current value (TODO)
		reg.second = nullptr;
	}

	void flush()
	{
		for (u32 i = 0; i < 128; i++)
		{
			flush(m_gpr[i], m_flush_gpr[i]);
		}
	}

	void update_pc()
	{
		m_ir->CreateStore(m_ir->getInt32(m_pos), spu_ptr<u32>(&SPUThread::pc));
	}

	// Perform external call
	template <typename RT, typename... FArgs, typename... Args>
	llvm::CallInst* call(RT(*_func)(FArgs...), Args... args)
	{
		static_assert(sizeof...(FArgs) == sizeof...(Args), "spu_llvm_recompiler::call(): unexpected arg number");
		const auto iptr = reinterpret_cast<std::uintptr_t>(_func);
		const auto type = llvm::FunctionType::get(get_type<RT>(), {args->getType()...}, false)->getPointerTo();
		return m_ir->CreateCall(m_ir->CreateIntToPtr(m_ir->getInt64(iptr), type), {args...});
	}

	// Perform external call and return
	template <typename RT, typename... FArgs, typename... Args>
	void tail(RT(*_func)(FArgs...), Args... args)
	{
		const auto inst = call(_func, args...);
		inst->setTailCall();

		if (inst->getType() == get_type<void>())
		{
			m_ir->CreateRetVoid();
		}
		else
		{
			m_ir->CreateRet(inst);
		}
	}

	void tail(llvm::Value* func_ptr)
	{
		m_ir->CreateCall(func_ptr, {m_thread, m_lsptr, m_ir->getInt32(0)})->setTailCall();
		m_ir->CreateRetVoid();
	}

	template <typename T1, typename T2>
	value_t<u8[16]> pshufb(T1 a, T2 b)
	{
		value_t<u8[16]> result;
		result.value = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_ssse3_pshuf_b_128), {a.eval(m_ir), b.eval(m_ir)});
		return result;
	}

public:
	spu_llvm_recompiler(class SPUThread& spu)
		: spu_recompiler_base(spu)
		, cpu_translator(nullptr, false)
	{
		if (g_cfg.core.spu_shared_runtime)
		{
			// TODO (local context is unsupported)
			//m_spurt = std::make_shared<spu_llvm_runtime>();
		}
	}

	virtual spu_function_t get(u32 lsa) override
	{
		// Initialize if necessary
		if (!m_spurt)
		{
			m_spurt = fxm::get_always<spu_llvm_runtime>();
			m_context = m_spurt->m_jit.get_context();
		}

		// Simple atomic read
		return m_spurt->m_dispatcher[lsa / 4];
	}

	virtual spu_function_t compile(const std::vector<u32>& func) override
	{
		// Initialize if necessary
		if (!m_spurt)
		{
			m_spurt = fxm::get_always<spu_llvm_runtime>();
			m_context = m_spurt->m_jit.get_context();
		}

		// Don't lock without shared runtime
		std::unique_lock<shared_mutex> lock(m_spurt->m_mutex, std::defer_lock);

		if (g_cfg.core.spu_shared_runtime)
		{
			lock.lock();
		}

		// Try to find existing function, register new
		auto& fn_location = m_spurt->m_map[func];

		if (fn_location)
		{
			return fn_location;
		}

		std::string hash;
		{
			sha1_context ctx;
			u8 output[20];

			sha1_starts(&ctx);
			sha1_update(&ctx, reinterpret_cast<const u8*>(func.data() + 1), func.size() * 4 - 4);
			sha1_finish(&ctx, output);

			fmt::append(hash, "spu-0x%05x-%s", func[0], fmt::base57(output));
		}

		LOG_NOTICE(SPU, "Building function 0x%x... (size %u, %s)", func[0], func.size() - 1, hash);

		using namespace llvm;

		SPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
		dis_asm.offset = reinterpret_cast<const u8*>(func.data() + 1) - func[0];
		std::string log;

		if (g_cfg.core.spu_debug)
		{
			fmt::append(log, "========== SPU BLOCK 0x%05x (size %u, %s) ==========\n\n", func[0], func.size() - 1, hash);
		}

		// Create LLVM module
		std::unique_ptr<Module> module = std::make_unique<Module>(hash, m_context);

		// Initialize target
		module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));

		// Initialize pass manager
		legacy::FunctionPassManager pm(module.get());

		// Basic optimizations
		pm.add(createEarlyCSEPass());
		pm.add(createDeadStoreEliminationPass());
		pm.add(createLintPass()); // Check

		// Add function
		const auto main_func = cast<Function>(module->getOrInsertFunction(hash, get_type<void>(), get_type<u64>(), get_type<u64>()));
		m_function = main_func;
		m_thread = &*m_function->arg_begin();
		m_lsptr = &*(m_function->arg_begin() + 1);

		// Initialize IR Builder
		IRBuilder<> irb(BasicBlock::Create(m_context, "", m_function));
		m_ir = &irb;

		// Start compilation
		m_pos = func[0];
		const u32 start = m_pos;
		const u32 end = m_pos + (func.size() - 1) * 4;

		// Create instruction blocks
		for (u32 i = 1, pos = start; i < func.size(); i++, pos += 4)
		{
			if (func[i] && m_block_info[pos / 4])
			{
				m_instr_map.emplace(pos, BasicBlock::Create(m_context, "", m_function));
			}
		}

		update_pc();

		m_stop = BasicBlock::Create(m_context, "", m_function);
		const auto label_test = BasicBlock::Create(m_context, "", m_function);
		const auto label_diff = BasicBlock::Create(m_context, "", m_function);
		const auto label_body = BasicBlock::Create(m_context, "", m_function);

		// Emit state check
		const auto pstate = spu_ptr<u32>(&SPUThread::state);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(m_ir->CreateLoad(pstate), m_ir->getInt32(0)), m_stop, label_test);

		// Emit code check
		m_ir->SetInsertPoint(label_test);

		if (false)
		{
			// Disable check (not available)
		}
		else if (func.size() - 1 == 1)
		{
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(_ptr<u32>(m_lsptr, m_pos)), m_ir->getInt32(func[1]));
			m_ir->CreateCondBr(cond, label_diff, label_body);
		}
		else if (func.size() - 1 == 2)
		{
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(_ptr<u64>(m_lsptr, m_pos)), m_ir->getInt64(static_cast<u64>(func[2]) << 32 | func[1]));
			m_ir->CreateCondBr(cond, label_diff, label_body);
		}
		else
		{
			const u32 starta = m_pos & -32;
			const u32 enda = ::align(end, 32);
			const u32 sizea = (enda - starta) / 32;
			verify(HERE), sizea;

			llvm::Value* acc = nullptr;

			for (u32 j = starta; j < enda; j += 32)
			{
				u32 indices[8];
				bool holes = false;
				bool data = false;

				for (u32 i = 0; i < 8; i++)
				{
					const u32 k = j + i * 4;

					if (k < m_pos || k >= end || !func[(k - m_pos) / 4 + 1])
					{
						indices[i] = 8;
						holes      = true;
					}
					else
					{
						indices[i] = i;
						data       = true;
					}
				}

				if (!data)
				{
					// Skip aligned holes
					continue;
				}

				// Load aligned code block from LS
				llvm::Value* vls = m_ir->CreateLoad(_ptr<u32[8]>(m_lsptr, j));

				// Mask if necessary
				if (holes)
				{
					vls = m_ir->CreateShuffleVector(vls, ConstantVector::getSplat(8, m_ir->getInt32(0)), indices);
				}

				// Perform bitwise comparison and accumulate
				u32 words[8];

				for (u32 i = 0; i < 8; i++)
				{
					const u32 k = j + i * 4;
					words[i] = k >= m_pos && k < end ? func[(k - m_pos) / 4 + 1] : 0;
				}

				vls = m_ir->CreateXor(vls, ConstantDataVector::get(m_context, words));
				acc = acc ? m_ir->CreateOr(acc, vls) : vls;
			}

			// Pattern for PTEST
			acc = m_ir->CreateBitCast(acc, get_type<u64[4]>());
			llvm::Value* elem = m_ir->CreateExtractElement(acc, u64{0});
			elem = m_ir->CreateOr(elem, m_ir->CreateExtractElement(acc, 1));
			elem = m_ir->CreateOr(elem, m_ir->CreateExtractElement(acc, 2));
			elem = m_ir->CreateOr(elem, m_ir->CreateExtractElement(acc, 3));

			// Compare result with zero
			const auto cond = m_ir->CreateICmpNE(elem, m_ir->getInt64(0));
			m_ir->CreateCondBr(cond, label_diff, label_body);
		}

		// Increase block counter
		m_ir->SetInsertPoint(label_body);
		const auto pbcount = spu_ptr<u64>(&SPUThread::block_counter);
		m_ir->CreateStore(m_ir->CreateAdd(m_ir->CreateLoad(pbcount), m_ir->getInt64(1)), pbcount);

		// Emit instructions
		for (u32 i = 1; i < func.size(); i++)
		{
			const u32 pos = start + (i - 1) * 4;

			if (g_cfg.core.spu_debug)
			{
				// Disasm
				dis_asm.dump_pc = pos;
				dis_asm.disasm(pos);
				log += dis_asm.last_opcode;
				log += '\n';
			}

			// Get opcode
			const u32 op = se_storage<u32>::swap(func[i]);

			if (!op)
			{
				// Ignore hole
				if (!m_ir->GetInsertBlock()->getTerminator())
				{
					flush();
					branch_fixed(spu_branch_target(pos));
					LOG_ERROR(SPU, "Unexpected fallthrough to 0x%x", pos);
				}

				continue;
			}

			// Bind instruction label if necessary (TODO)
			const auto found = m_instr_map.find(pos);

			if (found != m_instr_map.end())
			{
				if (!m_ir->GetInsertBlock()->getTerminator())
				{
					flush();
					m_ir->CreateBr(found->second);
				}

				m_ir->SetInsertPoint(found->second);
			}

			if (!m_ir->GetInsertBlock()->getTerminator())
			{
				// Update position
				m_pos = pos;

				// Execute recompiler function (TODO)
				(this->*g_decoder.decode(op))({op});
			}
		}

		if (g_cfg.core.spu_debug)
		{
			log += '\n';

			for (u32 i = 0; i < 128; i++)
			{
				if (m_gpr[i].first)
				{
					fmt::append(log, "$% -3u = %s\n", i, m_spu.gpr[i]);
				}
			}

			log += '\n';
		}

		// Make fallthrough if necessary
		if (!m_ir->GetInsertBlock()->getTerminator())
		{
			flush();
			branch_fixed(spu_branch_target(end));
		}

		//
		m_ir->SetInsertPoint(m_stop);
		m_ir->CreateRetVoid();

		m_ir->SetInsertPoint(label_diff);
		const auto pbfail = spu_ptr<u64>(&SPUThread::block_failure);
		m_ir->CreateStore(m_ir->CreateAdd(m_ir->CreateLoad(pbfail), m_ir->getInt64(1)), pbfail);
		tail(&spu_recompiler_base::dispatch, m_thread, m_ir->getInt32(0), m_ir->getInt32(0));

		// Clear context
		m_gpr.fill({});
		m_flush_gpr.fill(0);
		m_instr_map.clear();

		// Generate a dispatcher (übertrampoline)
		std::vector<u32> addrv{start};
		const auto beg = m_spurt->m_map.lower_bound(addrv);
		addrv[0] += 4;
		const auto _end = m_spurt->m_map.lower_bound(addrv);
		const u32 size0 = std::distance(beg, _end);

		if (size0 > 1)
		{
			const auto trampoline = cast<Function>(module->getOrInsertFunction(fmt::format("tr_0x%05x_%03u", start, size0), get_type<void>(), get_type<u64>(), get_type<u64>()));
			m_function = trampoline;
			m_thread = &*m_function->arg_begin();
			m_lsptr = &*(m_function->arg_begin() + 1);

			struct work
			{
				u32 size;
				u32 level;
				BasicBlock* label;
				std::map<std::vector<u32>, spu_function_t>::iterator beg;
				std::map<std::vector<u32>, spu_function_t>::iterator end;
			};

			std::vector<work> workload;
			workload.reserve(size0);
			workload.emplace_back();
			workload.back().size = size0;
			workload.back().level = 1;
			workload.back().beg = beg;
			workload.back().end = _end;
			workload.back().label = llvm::BasicBlock::Create(m_context, "", m_function);

			for (std::size_t i = 0; i < workload.size(); i++)
			{
				// Get copy of the workload info
				work w = workload[i];

				// Switch targets
				std::vector<std::pair<u32, llvm::BasicBlock*>> targets;

				llvm::BasicBlock* def{};

				while (true)
				{
					const u32 x1 = w.beg->first.at(w.level);
					auto it = w.beg;
					auto it2 = it;
					u32 x = x1;
					bool split = false;

					while (it2 != w.end)
					{
						it2++;

						const u32 x2 = it2 != w.end ? it2->first.at(w.level) : x1;

						if (x2 != x)
						{
							const u32 dist = std::distance(it, it2);

							const auto b = llvm::BasicBlock::Create(m_context, "", m_function);

							if (dist == 1 && x != 0)
							{
								m_ir->SetInsertPoint(b);

								if (const u64 fval = reinterpret_cast<u64>(it->second))
								{
									const auto ptr = m_ir->CreateIntToPtr(m_ir->getInt64(fval), main_func->getType());
									m_ir->CreateCall(ptr, {m_thread, m_lsptr})->setTailCall();
								}
								else
								{
									verify(HERE, &it->second == &fn_location);
									m_ir->CreateCall(main_func, {m_thread, m_lsptr})->setTailCall();
								}

								m_ir->CreateRetVoid();
							}
							else
							{
								workload.emplace_back(w);
								workload.back().beg = it;
								workload.back().end = it2;
								workload.back().label = b;
								workload.back().size = dist;
							}

							if (x == 0)
							{
								def = b;
							}
							else
							{
								targets.emplace_back(std::make_pair(x, b));
							}

							x = x2;
							it = it2;
							split = true;
						}
					}

					if (!split)
					{
						// Cannot split: words are identical within the range at this level
						w.level++;
					}
					else
					{
						break;
					}
				}

				if (!def)
				{
					def = llvm::BasicBlock::Create(m_context, "", m_function);

					m_ir->SetInsertPoint(def);
					tail(&spu_recompiler_base::dispatch, m_thread, m_ir->getInt32(0), m_ir->getInt32(0));
				}

				m_ir->SetInsertPoint(w.label);
				const auto add = m_ir->CreateAdd(m_lsptr, m_ir->getInt64(start + w.level * 4 - 4));
				const auto ptr = m_ir->CreateIntToPtr(add, get_type<u32*>());
				const auto val = m_ir->CreateLoad(ptr);
				const auto sw = m_ir->CreateSwitch(val, def, ::size32(targets));

				for (auto& pair : targets)
				{
					sw->addCase(m_ir->getInt32(pair.first), pair.second);
				}
			}
		}

		// Run some optimizations
		//pm.run(*main_func);

		spu_function_t fn{}, tr{};

		raw_string_ostream out(log);

		if (g_cfg.core.spu_debug)
		{
			fmt::append(log, "LLVM IR at 0x%x:\n", start);
			out << *module; // print IR
			out << "\n\n";
		}

		if (verifyModule(*module, &out))
		{
			out.flush();
			LOG_ERROR(SPU, "LLVM: Verification failed at 0x%x:\n%s", start, log);
			fmt::raw_error("Compilation failed");
		}

		if (g_cfg.core.spu_debug)
		{
			// Testing only
			m_spurt->m_jit.add(std::move(module), fmt::format("%sSPU/%s.obj", Emu.GetCachePath(), hash));
		}
		else
		{
			m_spurt->m_jit.add(std::move(module));
		}

		m_spurt->m_jit.fin();
		fn = reinterpret_cast<spu_function_t>(m_spurt->m_jit.get_engine().getPointerToFunction(main_func));
		tr = fn;

		if (size0 > 1)
		{
			tr = reinterpret_cast<spu_function_t>(m_spurt->m_jit.get_engine().getPointerToFunction(m_function));
		}

		// Register function pointer
		fn_location = fn;

		// Trampoline
		m_spurt->m_dispatcher[start / 4] = tr;

		LOG_NOTICE(SPU, "[0x%x] Compiled: %p", start, fn);

		if (tr != fn)
			LOG_NOTICE(SPU, "[0x%x] T: %p", start, tr);

		if (g_cfg.core.spu_debug)
		{
			out.flush();
			fs::file(Emu.GetCachePath() + "SPU.log", fs::write + fs::append).write(log);
		}

		return fn;
	}

	template <spu_inter_func_t F>
	static void exec_fall(SPUThread* _spu, spu_opcode_t op)
	{
		if (F(*_spu, op))
		{
			_spu->pc += 4;
		}
	}

	template <spu_inter_func_t F>
	void fall(spu_opcode_t op)
	{
		flush();
		update_pc();
		tail(&exec_fall<F>, m_thread, m_ir->getInt32(op.opcode));
	}

	static void exec_unk(SPUThread* _spu, u32 op)
	{
		fmt::throw_exception("Unknown/Illegal instruction (0x%08x)" HERE, op);
	}

	void UNK(spu_opcode_t op)
	{
		flush();
		update_pc();
		tail(&exec_unk, m_thread, m_ir->getInt32(op.opcode));
	}

	static void exec_stop(SPUThread* _spu, u32 code)
	{
		if (_spu->stop_and_signal(code))
		{
			_spu->pc += 4;
		}
	}

	void STOP(spu_opcode_t op)
	{
		flush();
		update_pc();
		tail(&exec_stop, m_thread, m_ir->getInt32(op.opcode));
	}

	void STOPD(spu_opcode_t op)
	{
		flush();
		update_pc();
		tail(&exec_stop, m_thread, m_ir->getInt32(0x3fff));
	}

	static s64 exec_rdch(SPUThread* _spu, u32 ch)
	{
		return _spu->get_ch_value(ch);
	}

	void RDCH(spu_opcode_t op)
	{
		flush();
		update_pc();
		value_t<s64> res;
		res.value = call(&exec_rdch, m_thread, m_ir->getInt32(op.ra));
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(m_ir->CreateICmpSLT(res.value, m_ir->getInt64(0)), m_stop, next);
		m_ir->SetInsertPoint(next);
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, trunc<u32>(res)));
	}

	static u32 exec_rchcnt(SPUThread* _spu, u32 ch)
	{
		return _spu->get_ch_count(ch);
	}

	void RCHCNT(spu_opcode_t op)
	{
		value_t<u32> res;
		res.value = call(&exec_rchcnt, m_thread, m_ir->getInt32(op.ra));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
	}

	static bool exec_wrch(SPUThread* _spu, u32 ch, u32 value)
	{
		return _spu->set_ch_value(ch, value);
	}

	void WRCH(spu_opcode_t op)
	{
		flush();
		update_pc();
		const auto succ = call(&exec_wrch, m_thread, m_ir->getInt32(op.ra), extract(get_vr(op.rt), 3).value);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(succ, next, m_stop);
		m_ir->SetInsertPoint(next);
	}

	void LNOP(spu_opcode_t op)
	{
		update_pc();
	}

	void NOP(spu_opcode_t op)
	{
		update_pc();
	}

	void SYNC(spu_opcode_t op)
	{
		// This instruction must be used following a store instruction that modifies the instruction stream.
		m_ir->CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
	}

	void DSYNC(spu_opcode_t op)
	{
		// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
		m_ir->CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
	}

	void MFSPR(spu_opcode_t op)
	{
		// Check SPUInterpreter for notes.
		set_vr(op.rt, splat<u32[4]>(0));
	}

	void MTSPR(spu_opcode_t op)
	{
		// Check SPUInterpreter for notes.
	}

	void SF(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rb) - get_vr(op.ra));
	}

	void OR(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) | get_vr(op.rb));
	}

	void BG(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		set_vr(op.rt, ~ucarry(a, eval(b - a), b) >> 31);
	}

	void SFH(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<u16[8]>(op.rb) - get_vr<u16[8]>(op.ra));
	}

	void NOR(spu_opcode_t op)
	{
		set_vr(op.rt, ~(get_vr(op.ra) | get_vr(op.rb)));
	}

	void ABSDB(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);
		set_vr(op.rt, max(a, b) - min(a, b));
	}

	void ROT(spu_opcode_t op)
	{
		set_vr(op.rt, rol(get_vr(op.ra), get_vr(op.rb)));
	}

	void ROTM(spu_opcode_t op)
	{
		set_vr(op.rt, trunc<u32[4]>(zext<u64[4]>(get_vr(op.ra)) >> zext<u64[4]>(-get_vr(op.rb) & 0x3f)));
	}

	void ROTMA(spu_opcode_t op)
	{
		set_vr(op.rt, trunc<u32[4]>(sext<s64[4]>(get_vr(op.ra)) >> zext<s64[4]>(-get_vr(op.rb) & 0x3f)));
	}

	void SHL(spu_opcode_t op)
	{
		set_vr(op.rt, trunc<u32[4]>(zext<u64[4]>(get_vr(op.ra)) << zext<u64[4]>(get_vr(op.rb) & 0x3f)));
	}

	void ROTH(spu_opcode_t op)
	{
		set_vr(op.rt, rol(get_vr<u16[8]>(op.ra), get_vr<u16[8]>(op.rb)));
	}

	void ROTHM(spu_opcode_t op)
	{
		set_vr(op.rt, trunc<u16[8]>(zext<u32[8]>(get_vr<u16[8]>(op.ra)) >> zext<u32[8]>(-get_vr<u16[8]>(op.rb) & 0x1f)));
	}

	void ROTMAH(spu_opcode_t op)
	{
		set_vr(op.rt, trunc<u16[8]>(sext<s32[8]>(get_vr<u16[8]>(op.ra)) >> zext<s32[8]>(-get_vr<u16[8]>(op.rb) & 0x1f)));
	}

	void SHLH(spu_opcode_t op)
	{
		set_vr(op.rt, trunc<u16[8]>(zext<u32[8]>(get_vr<u16[8]>(op.ra)) << zext<u32[8]>(get_vr<u16[8]>(op.rb) & 0x1f)));
	}

	void ROTI(spu_opcode_t op)
	{
		set_vr(op.rt, rol(get_vr(op.ra), op.i7));
	}

	void ROTMI(spu_opcode_t op)
	{
		if (-op.i7 & 0x20)
		{
			return set_vr(op.rt, splat<u32[4]>(0));
		}

		set_vr(op.rt, get_vr(op.ra) >> (-op.i7 & 0x1f));
	}

	void ROTMAI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) >> (-op.i7 & 0x20 ? 0x1f : -op.i7 & 0x1f));
	}

	void SHLI(spu_opcode_t op)
	{
		if (op.i7 & 0x20)
		{
			return set_vr(op.rt, splat<u32[4]>(0));
		}

		set_vr(op.rt, get_vr(op.ra) << (op.i7 & 0x1f));
	}

	void ROTHI(spu_opcode_t op)
	{
		set_vr(op.rt, rol(get_vr<u16[8]>(op.ra), op.i7));
	}

	void ROTHMI(spu_opcode_t op)
	{
		if (-op.i7 & 0x10)
		{
			return set_vr(op.rt, splat<u16[8]>(0));
		}

		set_vr(op.rt, get_vr<u16[8]>(op.ra) >> (-op.i7 & 0xf));
	}

	void ROTMAHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) >> (-op.i7 & 0x10 ? 0xf : -op.i7 & 0xf));
	}

	void SHLHI(spu_opcode_t op)
	{
		if (op.i7 & 0x10)
		{
			return set_vr(op.rt, splat<u16[8]>(0));
		}

		set_vr(op.rt, get_vr<u16[8]>(op.ra) << (op.i7 & 0xf));
	}

	void A(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) + get_vr(op.rb));
	}

	void AND(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) & get_vr(op.rb));
	}

	void CG(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		set_vr(op.rt, ucarry(a, b, eval(a + b)) >> 31);
	}

	void AH(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<u16[8]>(op.ra) + get_vr<u16[8]>(op.rb));
	}

	void NAND(spu_opcode_t op)
	{
		set_vr(op.rt, ~(get_vr(op.ra) & get_vr(op.rb)));
	}

	void AVGB(spu_opcode_t op)
	{
		set_vr(op.rt, avg(get_vr<u8[16]>(op.ra), get_vr<u8[16]>(op.rb)));
	}

	void GB(spu_opcode_t op)
	{
		//return fall<&spu_interpreter_fast::GB>(op);
		// TODO
		value_t<u32> m;
		m.value = eval((get_vr<s32[4]>(op.ra) << 31) < 0).value;
		m.value = m_ir->CreateBitCast(m.value, m_ir->getIntNTy(4));
		m.value = m_ir->CreateZExt(m.value, get_type<u32>());
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, m));
	}

	void GBH(spu_opcode_t op)
	{
		const auto m = zext<u32>(bitcast<u8>((get_vr<s16[8]>(op.ra) << 15) < 0));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, m));
	}

	void GBB(spu_opcode_t op)
	{
		const auto m = zext<u32>(bitcast<u16>((get_vr<s8[16]>(op.ra) << 7) < 0));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, m));
	}

	void FSM(spu_opcode_t op)
	{
		//return fall<&spu_interpreter_fast::FSM>(op);
		// TODO
		value_t<bool[4]> m;
		m.value = extract(get_vr(op.ra), 3).value;
		m.value = m_ir->CreateTrunc(m.value, m_ir->getIntNTy(4));
		m.value = m_ir->CreateBitCast(m.value, get_type<bool[4]>());
		set_vr(op.rt, sext<u32[4]>(m));
	}

	void FSMH(spu_opcode_t op)
	{
		const auto m = bitcast<bool[8]>(trunc<u8>(extract(get_vr(op.ra), 3)));
		set_vr(op.rt, sext<u16[8]>(m));
	}

	void FSMB(spu_opcode_t op)
	{
		const auto m = bitcast<bool[16]>(trunc<u16>(extract(get_vr(op.ra), 3)));
		set_vr(op.rt, sext<u8[16]>(m));
	}

	void ROTQBYBI(spu_opcode_t op)
	{
		value_t<u8[16]> sh;
		u8 initial[16]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
		sh.value = llvm::ConstantDataVector::get(m_context, initial);
		sh = eval((sh - (zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3)) & 0xf);
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void ROTQMBYBI(spu_opcode_t op)
	{
		value_t<u8[16]> sh;
		u8 initial[16]{112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127};
		sh.value = llvm::ConstantDataVector::get(m_context, initial);
		sh = eval(sh + (-(zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3) & 0x1f));
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void SHLQBYBI(spu_opcode_t op)
	{
		value_t<u8[16]> sh;
		u8 initial[16]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
		sh.value = llvm::ConstantDataVector::get(m_context, initial);
		sh = eval(sh - (zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) >> 3));
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void CBX(spu_opcode_t op)
	{
		const auto i = eval(~(extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0xf);
		value_t<u8[16]> r;
		u8 initial[16]{0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10};
		r.value = llvm::ConstantDataVector::get(m_context, initial);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt8(0x3), i.value);
		set_vr(op.rt, r);
	}

	void CHX(spu_opcode_t op)
	{
		const auto i = eval(~(extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) >> 1 & 0x7);
		value_t<u16[8]> r;
		u16 initial[8]{0x1e1f, 0x1c1d, 0x1a1b, 0x1819, 0x1617, 0x1415, 0x1213, 0x1011};
		r.value = llvm::ConstantDataVector::get(m_context, initial);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt16(0x0203), i.value);
		set_vr(op.rt, r);
	}

	void CWX(spu_opcode_t op)
	{
		const auto i = eval(~(extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) >> 2 & 0x3);
		value_t<u32[4]> r;
		u32 initial[4]{0x1c1d1e1f, 0x18191a1b, 0x14151617, 0x10111213};
		r.value = llvm::ConstantDataVector::get(m_context, initial);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt32(0x010203), i.value);
		set_vr(op.rt, r);
	}

	void CDX(spu_opcode_t op)
	{
		const auto i = eval(~(extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) >> 3 & 0x1);
		value_t<u64[2]> r;
		u64 initial[2]{0x18191a1b1c1d1e1f, 0x1011121214151617};
		r.value = llvm::ConstantDataVector::get(m_context, initial);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt64(0x01020304050607), i.value);
		set_vr(op.rt, r);
	}

	void ROTQBI(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = zshuffle<u32[4]>(get_vr<u32[4]>(op.rb) & 0x7, 3, 3, 3, 3);
		set_vr(op.rt, a << b | zshuffle<u32[4]>(a, 3, 0, 1, 2) >> (32 - b));
	}

	void ROTQMBI(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = zshuffle<u32[4]>(-get_vr<u32[4]>(op.rb) & 0x7, 3, 3, 3, 3);
		set_vr(op.rt, a >> b | zshuffle<u32[4]>(a, 1, 2, 3, 4) << (32 - b));
	}

	void SHLQBI(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = zshuffle<u32[4]>(get_vr<u32[4]>(op.rb) & 0x7, 3, 3, 3, 3);
		set_vr(op.rt, a << b | zshuffle<u32[4]>(a, 4, 0, 1, 2) >> (32 - b));
	}

	void ROTQBY(spu_opcode_t op)
	{
		value_t<u8[16]> sh;
		u8 initial[16]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
		sh.value = llvm::ConstantDataVector::get(m_context, initial);
		sh = eval((sh - zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12)) & 0xf);
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void ROTQMBY(spu_opcode_t op)
	{
		value_t<u8[16]> sh;
		u8 initial[16]{112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127};
		sh.value = llvm::ConstantDataVector::get(m_context, initial);
		sh = eval(sh + (-zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) & 0x1f));
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void SHLQBY(spu_opcode_t op)
	{
		value_t<u8[16]> sh;
		u8 initial[16]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
		sh.value = llvm::ConstantDataVector::get(m_context, initial);
		sh = eval(sh - (zshuffle<u8[16]>(get_vr<u8[16]>(op.rb), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12) & 0x1f));
		set_vr(op.rt, pshufb(get_vr<u8[16]>(op.ra), sh));
	}

	void ORX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto x = zshuffle<u32[4]>(a, 2, 3, 0, 1) | a;
		const auto y = zshuffle<u32[4]>(x, 1, 0, 3, 2) | x;
		set_vr(op.rt, zshuffle<u32[4]>(y, 4, 4, 4, 3));
	}

	void CBD(spu_opcode_t op)
	{
		const auto i = eval(~(extract(get_vr(op.ra), 3) + op.i7) & 0xf);
		value_t<u8[16]> r;
		u8 initial[16]{0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10};
		r.value = llvm::ConstantDataVector::get(m_context, initial);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt8(0x3), i.value);
		set_vr(op.rt, r);
	}

	void CHD(spu_opcode_t op)
	{
		const auto i = eval(~(extract(get_vr(op.ra), 3) + op.i7) >> 1 & 0x7);
		value_t<u16[8]> r;
		u16 initial[8]{0x1e1f, 0x1c1d, 0x1a1b, 0x1819, 0x1617, 0x1415, 0x1213, 0x1011};
		r.value = llvm::ConstantDataVector::get(m_context, initial);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt16(0x0203), i.value);
		set_vr(op.rt, r);
	}

	void CWD(spu_opcode_t op)
	{
		const auto i = eval(~(extract(get_vr(op.ra), 3) + op.i7) >> 2 & 0x3);
		value_t<u32[4]> r;
		u32 initial[4]{0x1c1d1e1f, 0x18191a1b, 0x14151617, 0x10111213};
		r.value = llvm::ConstantDataVector::get(m_context, initial);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt32(0x010203), i.value);
		set_vr(op.rt, r);
	}

	void CDD(spu_opcode_t op)
	{
		const auto i = eval(~(extract(get_vr(op.ra), 3) + op.i7) >> 3 & 0x1);
		value_t<u64[2]> r;
		u64 initial[2]{0x18191a1b1c1d1e1f, 0x1011121214151617};
		r.value = llvm::ConstantDataVector::get(m_context, initial);
		r.value = m_ir->CreateInsertElement(r.value, m_ir->getInt64(0x01020304050607), i.value);
		set_vr(op.rt, r);
	}

	void ROTQBII(spu_opcode_t op)
	{
		const auto s = op.i7 & 0x7;
		const auto a = get_vr(op.ra);

		if (s == 0)
		{
			return set_vr(op.rt, a);
		}

		set_vr(op.rt, a << s | zshuffle<u32[4]>(a, 3, 0, 1, 2) >> (32 - s));
	}

	void ROTQMBII(spu_opcode_t op)
	{
		const auto s = -op.i7 & 0x7;
		const auto a = get_vr(op.ra);

		if (s == 0)
		{
			return set_vr(op.rt, a);
		}

		set_vr(op.rt, a >> s | zshuffle<u32[4]>(a, 1, 2, 3, 4) << (32 - s));
	}

	void SHLQBII(spu_opcode_t op)
	{
		const auto s = op.i7 & 0x7;
		const auto a = get_vr(op.ra);

		if (s == 0)
		{
			return set_vr(op.rt, a);
		}

		set_vr(op.rt, a << s | zshuffle<u32[4]>(a, 4, 0, 1, 2) >> (32 - s));
	}

	void ROTQBYI(spu_opcode_t op)
	{
		//return fall<&spu_interpreter_fast::ROTQBYI>(op);
		const u32 s = -op.i7 & 0xf;
		set_vr(op.rt, zshuffle<u8[16]>(get_vr<u8[16]>(op.ra),
			s & 15, (s + 1) & 15, (s + 2) & 15, (s + 3) & 15,
			(s + 4) & 15, (s + 5) & 15, (s + 6) & 15, (s + 7) & 15,
			(s + 8) & 15, (s + 9) & 15, (s + 10) & 15, (s + 11) & 15,
			(s + 12) & 15, (s + 13) & 15, (s + 14) & 15, (s + 15) & 15));
	}

	void ROTQMBYI(spu_opcode_t op)
	{
		const u32 s = -op.i7 & 0x1f;

		if (s >= 16)
		{
			return set_vr(op.rt, splat<u32[4]>(0));
		}

		set_vr(op.rt, zshuffle<u8[16]>(get_vr<u8[16]>(op.ra),
			s, s + 1, s + 2, s + 3,
			s + 4, s + 5, s + 6, s + 7,
			s + 8, s + 9, s + 10, s + 11,
			s + 12, s + 13, s + 14, s + 15));
	}

	void SHLQBYI(spu_opcode_t op)
	{
		const u32 s = op.i7 & 0x1f;

		if (s >= 16)
		{
			return set_vr(op.rt, splat<u32[4]>(0));
		}

		const u32 x = -s;

		set_vr(op.rt, zshuffle<u8[16]>(get_vr<u8[16]>(op.ra),
			x & 31, (x + 1) & 31, (x + 2) & 31, (x + 3) & 31,
			(x + 4) & 31, (x + 5) & 31, (x + 6) & 31, (x + 7) & 31,
			(x + 8) & 31, (x + 9) & 31, (x + 10) & 31, (x + 11) & 31,
			(x + 12) & 31, (x + 13) & 31, (x + 14) & 31, (x + 15) & 31));
	}

	void CGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr<s32[4]>(op.ra) > get_vr<s32[4]>(op.rb)));
	}

	void XOR(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) ^ get_vr(op.rb));
	}

	void CGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<s16[8]>(op.ra) > get_vr<s16[8]>(op.rb)));
	}

	void EQV(spu_opcode_t op)
	{
		set_vr(op.rt, ~(get_vr(op.ra) ^ get_vr(op.rb)));
	}

	void CGTB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<s8[16]>(op.ra) > get_vr<s8[16]>(op.rb)));
	}

	void SUMB(spu_opcode_t op)
	{
		const auto a = get_vr<u16[8]>(op.ra);
		const auto b = get_vr<u16[8]>(op.rb);
		const auto ahs = eval((a >> 8) + (a & 0xff));
		const auto bhs = eval((b >> 8) + (b & 0xff));
		const auto lsh = shuffle2<u16[8]>(ahs, bhs, 0, 9, 2, 11, 4, 13, 6, 15);
		const auto hsh = shuffle2<u16[8]>(ahs, bhs, 1, 8, 3, 10, 5, 12, 7, 14);
		set_vr(op.rt, lsh + hsh);
	}

	void CLZ(spu_opcode_t op)
	{
		set_vr(op.rt, ctlz(get_vr(op.ra)));
	}

	void XSWD(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s64[2]>(op.ra) << 32 >> 32);
	}

	void XSHW(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) << 16 >> 16);
	}

	void CNTB(spu_opcode_t op)
	{
		set_vr(op.rt, ctpop(get_vr<u8[16]>(op.ra)));
	}

	void XSBH(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) << 8 >> 8);
	}

	void CLGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) > get_vr(op.rb)));
	}

	void ANDC(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) & ~get_vr(op.rb));
	}

	void CLGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) > get_vr<u16[8]>(op.rb)));
	}

	void ORC(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) | ~get_vr(op.rb));
	}

	void CLGTB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) > get_vr<u8[16]>(op.rb)));
	}

	void CEQ(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) == get_vr(op.rb)));
	}

	void MPYHHU(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) >> 16) * (get_vr(op.rb) >> 16));
	}

	void ADDX(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) + get_vr(op.rb) + (get_vr(op.rt) & 1));
	}

	void SFX(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rb) - get_vr(op.ra) - (~get_vr(op.rt) & 1));
	}

	void CGX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		const auto c = eval(get_vr(op.rt) << 31);
		const auto s = eval(a + b);
		set_vr(op.rt, (ucarry(a, b, s) | (sext<u32[4]>(s == 0xffffffffu) & c)) >> 31);
	}

	void BGX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);
		const auto c = eval(get_vr(op.rt) << 31);
		const auto d = eval(b - a);
		set_vr(op.rt, (~ucarry(a, d, b) & ~(sext<u32[4]>(d == 0) & ~c)) >> 31);
	}

	void MPYHHA(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) >> 16) * (get_vr<s32[4]>(op.rb) >> 16) + get_vr<s32[4]>(op.rt));
	}

	void MPYHHAU(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) >> 16) * (get_vr(op.rb) >> 16) + get_vr(op.rt));
	}

	void MPY(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) << 16 >> 16) * (get_vr<s32[4]>(op.rb) << 16 >> 16));
	}

	void MPYH(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) >> 16) * (get_vr(op.rb) << 16));
	}

	void MPYHH(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) >> 16) * (get_vr<s32[4]>(op.rb) >> 16));
	}

	void MPYS(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) << 16 >> 16) * (get_vr<s32[4]>(op.rb) << 16 >> 16) >> 16);
	}

	void CEQH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) == get_vr<u16[8]>(op.rb)));
	}

	void MPYU(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) << 16 >> 16) * (get_vr(op.rb) << 16 >> 16));
	}

	void CEQB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) == get_vr<u8[16]>(op.rb)));
	}

	void FSMBI(spu_opcode_t op)
	{
		u8 data[16];
		for (u32 i = 0; i < 16; i++)
			data[i] = op.i16 & (1u << i) ? 0xff : 0;
		value_t<u8[16]> r;
		r.value = llvm::ConstantDataVector::get(m_context, data);
		set_vr(op.rt, r);
	}

	void IL(spu_opcode_t op)
	{
		set_vr(op.rt, splat<s32[4]>(op.si16));
	}

	void ILHU(spu_opcode_t op)
	{
		set_vr(op.rt, splat<u32[4]>(op.i16 << 16));
	}

	void ILH(spu_opcode_t op)
	{
		set_vr(op.rt, splat<u16[8]>(op.i16));
	}

	void IOHL(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rt) | op.i16);
	}

	void ORI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) | op.si10);
	}

	void ORHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) | op.si10);
	}

	void ORBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) | op.si10);
	}

	void SFI(spu_opcode_t op)
	{
		set_vr(op.rt, op.si10 - get_vr<s32[4]>(op.ra));
	}

	void SFHI(spu_opcode_t op)
	{
		set_vr(op.rt, op.si10 - get_vr<s16[8]>(op.ra));
	}

	void ANDI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) & op.si10);
	}

	void ANDHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) & op.si10);
	}

	void ANDBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) & op.si10);
	}

	void AI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) + op.si10);
	}

	void AHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) + op.si10);
	}

	void XORI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s32[4]>(op.ra) ^ op.si10);
	}

	void XORHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s16[8]>(op.ra) ^ op.si10);
	}

	void XORBI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<s8[16]>(op.ra) ^ op.si10);
	}

	void CGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr<s32[4]>(op.ra) > op.si10));
	}

	void CGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<s16[8]>(op.ra) > op.si10));
	}

	void CGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<s8[16]>(op.ra) > op.si10));
	}

	void CLGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) > op.si10));
	}

	void CLGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) > op.si10));
	}

	void CLGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) > op.si10));
	}

	void MPYI(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) << 16 >> 16) * splat<s32[4]>(op.si10));
	}

	void MPYUI(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) << 16 >> 16) * splat<u32[4]>(op.si10 & 0xffff));
	}

	void CEQI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(get_vr(op.ra) == op.si10));
	}

	void CEQHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u16[8]>(get_vr<u16[8]>(op.ra) == op.si10));
	}

	void CEQBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u8[16]>(get_vr<u8[16]>(op.ra) == op.si10));
	}

	void ILA(spu_opcode_t op)
	{
		set_vr(op.rt, splat<u32[4]>(op.i18));
	}

	void SELB(spu_opcode_t op)
	{
		const auto c = get_vr(op.rc);
		set_vr(op.rt4, (get_vr(op.ra) & ~c) | (get_vr(op.rb) & c));
	}

	void SHUFB(spu_opcode_t op)
	{
		const auto c = get_vr<u8[16]>(op.rc);
		const auto x = avg(sext<u8[16]>((c & 0xc0) == 0xc0), sext<u8[16]>((c & 0xe0) == 0xc0));
		const auto cr = c ^ 0xf;
		const auto a = pshufb(get_vr<u8[16]>(op.ra), cr);
		const auto b = pshufb(get_vr<u8[16]>(op.rb), cr);
		set_vr(op.rt4, merge(sext<u8[16]>((c & 0x10) == 0), a, b) | x);
	}

	void MPYA(spu_opcode_t op)
	{
		set_vr(op.rt4, (get_vr<s32[4]>(op.ra) << 16 >> 16) * (get_vr<s32[4]>(op.rb) << 16 >> 16) + get_vr<s32[4]>(op.rc));
	}

	void FSCRRD(spu_opcode_t op)
	{
		// Hack
		set_vr(op.rt, splat<u32[4]>(0));
	}

	void FSCRWR(spu_opcode_t op)
	{
		// Hack
	}

	void DFCGT(spu_opcode_t op)
	{
		return UNK(op);
	}

	void DFCEQ(spu_opcode_t op)
	{
		return UNK(op);
	}

	void DFCMGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u64[2]>(fcmp<llvm::FCmpInst::FCMP_OGT>(fabs(get_vr<f64[2]>(op.ra)), fabs(get_vr<f64[2]>(op.rb)))));
	}

	void DFCMEQ(spu_opcode_t op)
	{
		return UNK(op);
	}

	void DFTSV(spu_opcode_t op)
	{
		return UNK(op);
	}

	void DFA(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) + get_vr<f64[2]>(op.rb));
	}

	void DFS(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) - get_vr<f64[2]>(op.rb));
	}

	void DFM(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb));
	}

	void DFMA(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb) + get_vr<f64[2]>(op.rt));
	}

	void DFMS(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb) - get_vr<f64[2]>(op.rt));
	}

	void DFNMS(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f64[2]>(op.rt) - get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb));
	}

	void DFNMA(spu_opcode_t op)
	{
		set_vr(op.rt, -get_vr<f64[2]>(op.rt) - get_vr<f64[2]>(op.ra) * get_vr<f64[2]>(op.rb));
	}

	void FREST(spu_opcode_t op)
	{
		set_vr(op.rt, fsplat<f32[4]>(1.0) / get_vr<f32[4]>(op.ra));
	}

	void FRSQEST(spu_opcode_t op)
	{
		set_vr(op.rt, fsplat<f32[4]>(1.0) / sqrt(fabs(get_vr<f32[4]>(op.ra))));
	}

	void FCGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OGT>(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb))));
	}

	void FCMGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OGT>(fabs(get_vr<f32[4]>(op.ra)), fabs(get_vr<f32[4]>(op.rb)))));
	}

	void FA(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f32[4]>(op.ra) + get_vr<f32[4]>(op.rb));
	}

	void FS(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f32[4]>(op.ra) - get_vr<f32[4]>(op.rb));
	}

	void FM(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb));
	}

	void FESD(spu_opcode_t op)
	{
		value_t<f64[2]> r;
		r.value = m_ir->CreateFPExt(shuffle2<f32[2]>(get_vr<f32[4]>(op.ra), fsplat<f32[4]>(0.), 1, 3).value, get_type<f64[2]>());
		set_vr(op.rt, r);
	}

	void FRDS(spu_opcode_t op)
	{
		value_t<f32[2]> r;
		r.value = m_ir->CreateFPTrunc(get_vr<f64[2]>(op.ra).value, get_type<f32[2]>());
		set_vr(op.rt, shuffle2<f32[4]>(r, fsplat<f32[2]>(0.), 2, 0, 3, 1));
	}

	void FCEQ(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OEQ>(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb))));
	}

	void FCMEQ(spu_opcode_t op)
	{
		set_vr(op.rt, sext<u32[4]>(fcmp<llvm::FCmpInst::FCMP_OEQ>(fabs(get_vr<f32[4]>(op.ra)), fabs(get_vr<f32[4]>(op.rb)))));
	}

	void FNMS(spu_opcode_t op)
	{
		set_vr(op.rt4, get_vr<f32[4]>(op.rc) - get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb));
	}

	void FMA(spu_opcode_t op)
	{
		set_vr(op.rt4, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb) + get_vr<f32[4]>(op.rc));
	}

	void FMS(spu_opcode_t op)
	{
		set_vr(op.rt4, get_vr<f32[4]>(op.ra) * get_vr<f32[4]>(op.rb) - get_vr<f32[4]>(op.rc));
	}

	void FI(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr<f32[4]>(op.rb));
	}

	void CFLTS(spu_opcode_t op)
	{
		value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
		if (op.i8 != 173)
			a = eval(a * fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))));

		value_t<s32[4]> r;
		r.value = m_ir->CreateFPToSI(a.value, get_type<s32[4]>());
		set_vr(op.rt, r ^ sext<s32[4]>(fcmp<llvm::FCmpInst::FCMP_OGE>(a, fsplat<f32[4]>(std::exp2(31.f)))));
	}

	void CFLTU(spu_opcode_t op)
	{
		value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
		if (op.i8 != 173)
			a = eval(a * fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))));

		value_t<s32[4]> r;
		r.value = m_ir->CreateFPToUI(a.value, get_type<s32[4]>());
		set_vr(op.rt, r & ~(bitcast<s32[4]>(a) >> 31));
	}

	void CSFLT(spu_opcode_t op)
	{
		value_t<f32[4]> r;
		r.value = m_ir->CreateSIToFP(get_vr<s32[4]>(op.ra).value, get_type<f32[4]>());
		if (op.i8 != 155)
			r = eval(r * fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))));
		set_vr(op.rt, r);
	}

	void CUFLT(spu_opcode_t op)
	{
		value_t<f32[4]> r;
		r.value = m_ir->CreateUIToFP(get_vr<s32[4]>(op.ra).value, get_type<f32[4]>());
		if (op.i8 != 155)
			r = eval(r * fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))));
		set_vr(op.rt, r);
	}

	void STQX(spu_opcode_t op)
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0x3fff0);
		addr.value = m_ir->CreateAdd(m_lsptr, addr.value);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateIntToPtr(addr.value, get_type<u8[16]>()->getPointerTo()));
	}

	void LQX(spu_opcode_t op)
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + extract(get_vr(op.rb), 3)) & 0x3fff0);
		addr.value = m_ir->CreateAdd(m_lsptr, addr.value);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateIntToPtr(addr.value, get_type<u8[16]>()->getPointerTo()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQA(spu_opcode_t op)
	{
		value_t<u64> addr = splat<u64>(spu_ls_target(0, op.i16));
		addr.value = m_ir->CreateAdd(m_lsptr, addr.value);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateIntToPtr(addr.value, get_type<u8[16]>()->getPointerTo()));
	}

	void LQA(spu_opcode_t op)
	{
		value_t<u64> addr = splat<u64>(spu_ls_target(0, op.i16));
		addr.value = m_ir->CreateAdd(m_lsptr, addr.value);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateIntToPtr(addr.value, get_type<u8[16]>()->getPointerTo()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQR(spu_opcode_t op)
	{
		value_t<u64> addr = splat<u64>(spu_ls_target(m_pos, op.i16));
		addr.value = m_ir->CreateAdd(m_lsptr, addr.value);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateIntToPtr(addr.value, get_type<u8[16]>()->getPointerTo()));
	}

	void LQR(spu_opcode_t op)
	{
		value_t<u64> addr = splat<u64>(spu_ls_target(m_pos, op.i16));
		addr.value = m_ir->CreateAdd(m_lsptr, addr.value);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateIntToPtr(addr.value, get_type<u8[16]>()->getPointerTo()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void STQD(spu_opcode_t op)
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + (op.si10 << 4)) & 0x3fff0);
		addr.value = m_ir->CreateAdd(m_lsptr, addr.value);
		value_t<u8[16]> r = get_vr<u8[16]>(op.rt);
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		m_ir->CreateStore(r.value, m_ir->CreateIntToPtr(addr.value, get_type<u8[16]>()->getPointerTo()));
	}

	void LQD(spu_opcode_t op)
	{
		value_t<u64> addr = zext<u64>((extract(get_vr(op.ra), 3) + (op.si10 << 4)) & 0x3fff0);
		addr.value = m_ir->CreateAdd(m_lsptr, addr.value);
		value_t<u8[16]> r;
		r.value = m_ir->CreateLoad(m_ir->CreateIntToPtr(addr.value, get_type<u8[16]>()->getPointerTo()));
		r.value = m_ir->CreateShuffleVector(r.value, r.value, {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0});
		set_vr(op.rt, r);
	}

	void make_halt(llvm::BasicBlock* next)
	{
		const auto pstatus = spu_ptr<u32>(&SPUThread::status);
		const auto chalt = m_ir->getInt32(SPU_STATUS_STOPPED_BY_HALT);
		m_ir->CreateAtomicRMW(llvm::AtomicRMWInst::Or, pstatus, chalt, llvm::AtomicOrdering::Release)->setVolatile(true);
		const auto ptr = m_ir->CreateIntToPtr(m_ir->getInt64(reinterpret_cast<u64>(vm::base(0xffdead00))), get_type<u32*>());
		m_ir->CreateStore(m_ir->getInt32("HALT"_u32), ptr)->setVolatile(true);
		m_ir->CreateBr(next);
	}

	void HGT(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr<s32[4]>(op.ra), 3) > extract(get_vr<s32[4]>(op.rb), 3));
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next);
		m_ir->SetInsertPoint(halt);
		make_halt(next);
		m_ir->SetInsertPoint(next);
	}

	void HEQ(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr(op.ra), 3) == extract(get_vr(op.rb), 3));
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next);
		m_ir->SetInsertPoint(halt);
		make_halt(next);
		m_ir->SetInsertPoint(next);
	}

	void HLGT(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr(op.ra), 3) > extract(get_vr(op.rb), 3));
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next);
		m_ir->SetInsertPoint(halt);
		make_halt(next);
		m_ir->SetInsertPoint(next);
	}

	void HGTI(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr<s32[4]>(op.ra), 3) > op.si10);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next);
		m_ir->SetInsertPoint(halt);
		make_halt(next);
		m_ir->SetInsertPoint(next);
	}

	void HEQI(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr(op.ra), 3) == op.si10);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next);
		m_ir->SetInsertPoint(halt);
		make_halt(next);
		m_ir->SetInsertPoint(next);
	}

	void HLGTI(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr(op.ra), 3) > op.si10);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next);
		m_ir->SetInsertPoint(halt);
		make_halt(next);
		m_ir->SetInsertPoint(next);
	}

	void HBR(spu_opcode_t op)
	{
		// TODO: use the hint.
	}

	void HBRA(spu_opcode_t op)
	{
		// TODO: use the hint.
	}

	void HBRR(spu_opcode_t op)
	{
		// TODO: use the hint.
	}

	static void exec_interrupts_disable(SPUThread* _spu, u32 addr)
	{
		_spu->interrupts_enabled = false;
		_spu->pc = addr;
		_spu->jit_dispatcher[addr / 4](*_spu, _spu->_ptr<u8>(0), nullptr);
	}

	static void exec_interrupts_enable(SPUThread* _spu, u32 addr)
	{
		_spu->set_interrupt_status(true);

		if ((_spu->ch_event_mask & _spu->ch_event_stat & SPU_EVENT_INTR_IMPLEMENTED) > 0)
		{
			_spu->interrupts_enabled = false;
			_spu->srr0 = addr;
			addr = 0;
		}

		_spu->pc = addr;
		_spu->jit_dispatcher[addr / 4](*_spu, _spu->_ptr<u8>(0), nullptr);
	}

	void branch_indirect(spu_opcode_t op, value_t<u32> addr)
	{
		if (op.d)
		{
			return tail(&exec_interrupts_disable, m_thread, addr.value);
		}
		else if (op.e)
		{
			return tail(&exec_interrupts_enable, m_thread, addr.value);
		}

		m_ir->CreateStore(addr.value, spu_ptr<u32>(&SPUThread::pc));
		const auto disp = m_ir->CreateAdd(m_thread, m_ir->getInt64(::offset32(&SPUThread::jit_dispatcher)));
		const auto type = llvm::FunctionType::get(get_type<void>(), {get_type<u64>(), get_type<u64>(), get_type<u32>()}, false)->getPointerTo()->getPointerTo();
		tail(m_ir->CreateLoad(m_ir->CreateIntToPtr(m_ir->CreateAdd(disp, zext<u64>(addr << 1).value), type)));
	}

	void branch_fixed(u32 target)
	{
		const auto found = m_instr_map.find(target);

		if (found != m_instr_map.end())
		{
			m_ir->CreateBr(found->second);
			return;
		}

		m_ir->CreateStore(m_ir->getInt32(target), spu_ptr<u32>(&SPUThread::pc));
		const auto addr = m_ir->CreateAdd(m_thread, m_ir->getInt64(::offset32(&SPUThread::jit_dispatcher) + target * 2));
		const auto type = llvm::FunctionType::get(get_type<void>(), {get_type<u64>(), get_type<u64>(), get_type<u32>()}, false)->getPointerTo()->getPointerTo();
		tail(m_ir->CreateLoad(m_ir->CreateIntToPtr(addr, type)));
	}

	void BIZ(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr(op.rt), 3) == 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto jump = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, jump, next);
		m_ir->SetInsertPoint(jump);
		branch_indirect(op, addr);
		m_ir->SetInsertPoint(next);
	}

	void BINZ(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr(op.rt), 3) != 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto jump = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, jump, next);
		m_ir->SetInsertPoint(jump);
		branch_indirect(op, addr);
		m_ir->SetInsertPoint(next);
	}

	void BIHZ(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) == 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto jump = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, jump, next);
		m_ir->SetInsertPoint(jump);
		branch_indirect(op, addr);
		m_ir->SetInsertPoint(next);
	}

	void BIHNZ(spu_opcode_t op)
	{
		flush();
		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) != 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto jump = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, jump, next);
		m_ir->SetInsertPoint(jump);
		branch_indirect(op, addr);
		m_ir->SetInsertPoint(next);
	}

	void BI(spu_opcode_t op)
	{
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		flush();
		branch_indirect(op, addr);
	}

	void BISL(spu_opcode_t op)
	{
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		u32 values[4]{0, 0, 0, spu_branch_target(m_pos + 4)};
		value_t<u32[4]> r;
		r.value = llvm::ConstantDataVector::get(m_context, values);
		set_vr(op.rt, r);
		flush();
		branch_indirect(op, addr);
	}

	void IRET(spu_opcode_t op)
	{
		value_t<u32> srr0;
		srr0.value = m_ir->CreateLoad(spu_ptr<u32>(&SPUThread::srr0));
		flush();
		branch_indirect(op, srr0);
	}

	void BISLED(spu_opcode_t op) { UNK(op); }

	void BRZ(spu_opcode_t op)
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target == m_pos + 4)
		{
			return;
		}

		flush();
		const auto cond = eval(extract(get_vr(op.rt), 3) == 0);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto jump = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, jump, next);
		m_ir->SetInsertPoint(jump);
		branch_fixed(target);
		m_ir->SetInsertPoint(next);
	}

	void BRNZ(spu_opcode_t op)
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target == m_pos + 4)
		{
			return;
		}

		flush();
		const auto cond = eval(extract(get_vr(op.rt), 3) != 0);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto jump = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, jump, next);
		m_ir->SetInsertPoint(jump);
		branch_fixed(target);
		m_ir->SetInsertPoint(next);
	}

	void BRHZ(spu_opcode_t op)
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target == m_pos + 4)
		{
			return;
		}

		flush();
		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) == 0);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto jump = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, jump, next);
		m_ir->SetInsertPoint(jump);
		branch_fixed(target);
		m_ir->SetInsertPoint(next);
	}

	void BRHNZ(spu_opcode_t op)
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target == m_pos + 4)
		{
			return;
		}

		flush();
		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) != 0);
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto jump = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, jump, next);
		m_ir->SetInsertPoint(jump);
		branch_fixed(target);
		m_ir->SetInsertPoint(next);
	}

	void BRA(spu_opcode_t op)
	{
		const u32 target = spu_branch_target(0, op.i16);

		if (target != m_pos + 4)
		{
			flush();
			branch_fixed(target);
		}
	}

	void BRASL(spu_opcode_t op)
	{
		u32 values[4]{0, 0, 0, spu_branch_target(m_pos + 4)};
		value_t<u32[4]> r;
		r.value = llvm::ConstantDataVector::get(m_context, values);
		set_vr(op.rt, r);
		BRA(op);
	}

	void BR(spu_opcode_t op)
	{
		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target != m_pos + 4)
		{
			flush();
			branch_fixed(target);
		}
	}

	void BRSL(spu_opcode_t op)
	{
		u32 values[4]{0, 0, 0, spu_branch_target(m_pos + 4)};
		value_t<u32[4]> r;
		r.value = llvm::ConstantDataVector::get(m_context, values);
		set_vr(op.rt, r);
		BR(op);
	}

	static const spu_decoder<spu_llvm_recompiler> g_decoder;
};

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_llvm_recompiler(SPUThread& spu)
{
	return std::make_unique<spu_llvm_recompiler>(spu);
}

DECLARE(spu_llvm_recompiler::g_decoder);
