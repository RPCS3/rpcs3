#include "stdafx.h"
#include "SPURecompiler.h"

#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/Cell/lv2/sys_time.h"
#include "Emu/Memory/vm_reservation.h"
#include "Emu/RSX/Core/RSXReservationLock.hpp"
#include "Crypto/sha1.h"
#include "Utilities/JIT.h"

#include "SPUThread.h"
#include "SPUAnalyser.h"
#include "SPUInterpreter.h"
#include <algorithm>
#include <thread>

#include "util/v128.hpp"
#include "util/simd.hpp"
#include "util/sysinfo.hpp"

const extern spu_decoder<spu_itype> g_spu_itype;
const extern spu_decoder<spu_iname> g_spu_iname;
const extern spu_decoder<spu_iflag> g_spu_iflag;

#ifdef LLVM_AVAILABLE

#include "Emu/CPU/CPUTranslator.h"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Verifier.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Transforms/Scalar/ADCE.h>
#include <llvm/Transforms/Scalar/DeadStoreElimination.h>
#include <llvm/Transforms/Scalar/EarlyCSE.h>
#include <llvm/Transforms/Scalar/LICM.h>
#include <llvm/Transforms/Scalar/LoopPassManager.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#ifdef ARCH_ARM64
#include "Emu/CPU/Backends/AArch64/AArch64JIT.h"
#endif

class spu_llvm_recompiler : public spu_recompiler_base, public cpu_translator
{
	// JIT Instance
	jit_compiler m_jit{{}, jit_compiler::cpu(g_cfg.core.llvm_cpu)};

	// Interpreter table size power
	const u8 m_interp_magn;

	// Constant opcode bits
	u32 m_op_const_mask = -1;

	// Current function chunk entry point
	u32 m_entry;

	// Main entry point offset
	u32 m_base;

	// Module name
	std::string m_hash;

	// Patchpoint unique id
	u32 m_pp_id = 0;

	// Next opcode
	u32 m_next_op = 0;

	// Current function (chunk)
	llvm::Function* m_function;

	llvm::Value* m_thread;
	llvm::Value* m_lsptr;
	llvm::Value* m_interp_op;
	llvm::Value* m_interp_pc;
	llvm::Value* m_interp_table;
	llvm::Value* m_interp_7f0;
	llvm::Value* m_interp_regs;

	// Helpers
	llvm::Value* m_base_pc;
	llvm::Value* m_interp_pc_next;
	llvm::BasicBlock* m_interp_bblock;

	// i8*, contains constant vm::g_base_addr value
	llvm::Value* m_memptr;

	// Pointers to registers in the thread context
	std::array<llvm::Value*, s_reg_max> m_reg_addr;

	// Global variable (function table)
	llvm::GlobalVariable* m_function_table{};

	// Global LUTs
	llvm::GlobalVariable* m_spu_frest_fraction_lut{};
	llvm::GlobalVariable* m_spu_frsqest_fraction_lut{};
	llvm::GlobalVariable* m_spu_frsqest_exponent_lut{};

	// Helpers (interpreter)
	llvm::GlobalVariable* m_scale_float_to{};
	llvm::GlobalVariable* m_scale_to_float{};

	// Function for check_state execution
	llvm::Function* m_test_state{};

	// Chunk for external tail call (dispatch)
	llvm::Function* m_dispatch{};

	llvm::MDNode* m_md_unlikely;
	llvm::MDNode* m_md_likely;

	struct block_info
	{
		// Pointer to the analyser
		spu_recompiler_base::block_info* bb{};

		// Current block's entry block
		llvm::BasicBlock* block;

		// Final block (for PHI nodes, set after completion)
		llvm::BasicBlock* block_end{};

		// Additional blocks for sinking instructions after block_end:
		std::unordered_map<u32, llvm::BasicBlock*, value_hash<u32, 2>> block_edges;

		// Current register values
		std::array<llvm::Value*, s_reg_max> reg{};

		// PHI nodes created for this block (if any)
		std::array<llvm::PHINode*, s_reg_max> phi{};

		// Store instructions
		std::array<llvm::StoreInst*, s_reg_max> store{};

		// Store reordering/elimination protection
		std::array<usz, s_reg_max> store_context_last_id = fill_array<usz>(0); // Protects against illegal forward ordering
		std::array<usz, s_reg_max> store_context_first_id = fill_array<usz>(usz{umax}); // Protects against illegal past store elimination (backwards ordering is not implemented)
		std::array<usz, s_reg_max> store_context_ctr = fill_array<usz>(1); // Store barrier counter
		bool has_gpr_memory_barriers = false; // Summarizes whether GPR barriers exist this block (as if checking all store_context_ctr entries)

		bool does_gpr_barrier_proceed_last_store(u32 i) const noexcept
		{
			const usz counter = store_context_ctr[i];
			return counter != 1 && counter > store_context_last_id[i];
		}

		bool does_gpr_barrier_preceed_first_store(u32 i) const noexcept
		{
			const usz counter = store_context_ctr[i];
			const usz first_id = store_context_first_id[i];
			return counter != 1 && first_id != umax && counter < first_id;
		}
	};

	struct function_info
	{
		// Standard callable chunk
		llvm::Function* chunk{};

		// Callable function
		llvm::Function* fn{};

		// Registers possibly loaded in the entry block
		std::array<llvm::Value*, s_reg_max> load{};
	};

	// Current block
	block_info* m_block;

	// Current function or chunk
	function_info* m_finfo;

	// All blocks in the current function chunk
	std::unordered_map<u32, block_info, value_hash<u32, 2>> m_blocks;

	// Block list for processing
	std::vector<u32> m_block_queue;

	// All function chunks in current SPU compile unit
	std::unordered_map<u32, function_info, value_hash<u32, 2>> m_functions;

	// Function chunk list for processing
	std::vector<u32> m_function_queue;

	// Add or get the function chunk
	function_info* add_function(u32 addr)
	{
		// Enqueue if necessary
		const auto empl = m_functions.try_emplace(addr);

		if (!empl.second)
		{
			return &empl.first->second;
		}

		// Chunk function type
		// 0. Result (tail call target)
		// 1. Thread context
		// 2. Local storage pointer
		// 3.
#if 0
		const auto chunk_type = get_ftype<u8*, u8*, u8*, u32>();
#else
		const auto chunk_type = get_ftype<void, u8*, u8*, u32>();
#endif

		// Get function chunk name
		const std::string name = fmt::format("__spu-cx%05x-%s", addr, fmt::base57(be_t<u64>{m_hash_start}));
		llvm::Function* result = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(name, chunk_type).getCallee());

		// Set parameters
		result->setLinkage(llvm::GlobalValue::InternalLinkage);
		result->addParamAttr(0, llvm::Attribute::NoAlias);
		result->addParamAttr(1, llvm::Attribute::NoAlias);
#if 1
		result->setCallingConv(llvm::CallingConv::GHC);
#endif

		empl.first->second.chunk = result;

		if (g_cfg.core.spu_block_size == spu_block_size_type::giga)
		{
			// Find good real function
			const auto ffound = m_funcs.find(addr);

			if (ffound != m_funcs.end() && ffound->second.good)
			{
				// Real function type (not equal to chunk type)
				// 4. $SP
				// 5. $3
				const auto func_type = get_ftype<u32[4], u8*, u8*, u32, u32[4], u32[4]>();

				const std::string fname = fmt::format("__spu-fx%05x-%s", addr, fmt::base57(be_t<u64>{m_hash_start}));
				llvm::Function* fn = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(fname, func_type).getCallee());

				fn->setLinkage(llvm::GlobalValue::InternalLinkage);
				fn->addParamAttr(0, llvm::Attribute::NoAlias);
				fn->addParamAttr(1, llvm::Attribute::NoAlias);
#if 1
				fn->setCallingConv(llvm::CallingConv::GHC);
#endif
				empl.first->second.fn = fn;
			}
		}

		// Enqueue
		m_function_queue.push_back(addr);

		return &empl.first->second;
	}

	// Create tail call to the function chunk (non-tail calls are just out of question)
	void tail_chunk(llvm::FunctionCallee callee, llvm::Value* base_pc = nullptr)
	{
		if (!callee && !g_cfg.core.spu_verification)
		{
			// Disable patchpoints if verification is disabled
			callee = m_dispatch;
		}
		else if (!callee)
		{
			// Create branch patchpoint if chunk == nullptr
			ensure(m_finfo && (!m_finfo->fn || m_function == m_finfo->chunk));

			// Register under a unique linkable name
			const std::string ppname = fmt::format("%s-pp-%u", m_hash, m_pp_id++);
			m_engine->updateGlobalMapping(ppname, reinterpret_cast<u64>(m_spurt->make_branch_patchpoint()));

			// Create function with not exactly correct type
			const auto ppfunc = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(ppname, m_finfo->chunk->getFunctionType()).getCallee());
			ppfunc->setCallingConv(m_finfo->chunk->getCallingConv());

			if (m_finfo->chunk->getReturnType() != get_type<void>())
			{
				m_ir->CreateRet(ppfunc);
				return;
			}

			callee = ppfunc;
			base_pc = m_ir->getInt32(0);
		}

		ensure(callee);
		auto call = m_ir->CreateCall(callee, {m_thread, m_lsptr, base_pc ? base_pc : m_base_pc});
		auto func = m_finfo ? m_finfo->chunk : llvm::dyn_cast<llvm::Function>(callee.getCallee());
		call->setCallingConv(func->getCallingConv());
		call->setTailCall();

		if (func->getReturnType() == get_type<void>())
		{
			m_ir->CreateRetVoid();
		}
		else
		{
			m_ir->CreateRet(call);
		}
	}

	// Call the real function
	void call_function(llvm::Function* fn, bool tail = false)
	{
		llvm::Value* lr{};
		llvm::Value* sp{};
		llvm::Value* r3{};

		if (!m_finfo->fn && !m_block)
		{
			lr = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::gpr, +s_reg_lr, &v128::_u32, 3));
			sp = m_ir->CreateLoad(get_type<u32[4]>(), spu_ptr(&spu_thread::gpr, +s_reg_sp));
			r3 = m_ir->CreateLoad(get_type<u32[4]>(), spu_ptr(&spu_thread::gpr, 3));
		}
		else
		{
			lr = m_ir->CreateExtractElement(get_reg_fixed<u32[4]>(s_reg_lr).value, 3);
			sp = get_reg_fixed<u32[4]>(s_reg_sp).value;
			r3 = get_reg_fixed<u32[4]>(3).value;
		}

		const auto _call = m_ir->CreateCall(ensure(fn), {m_thread, m_lsptr, m_base_pc, sp, r3});

		_call->setCallingConv(fn->getCallingConv());

		// Tail call using loaded LR value (gateway from a chunk)
		if (!m_finfo->fn)
		{
			lr = m_ir->CreateAnd(lr, 0x3fffc);
			m_ir->CreateStore(lr, spu_ptr(&spu_thread::pc));
			m_ir->CreateStore(_call, spu_ptr(&spu_thread::gpr, 3));
			m_ir->CreateBr(add_block_indirect({}, value<u32>(lr)));
		}
		else if (tail)
		{
			_call->setTailCall();
			m_ir->CreateRet(_call);
		}
		else
		{
			// TODO: initialize $LR with a constant
			for (u32 i = 0; i < s_reg_max; i++)
			{
				if (i != s_reg_lr && i != s_reg_sp && (i < s_reg_80 || i > s_reg_127))
				{
					m_block->reg[i] = m_ir->CreateLoad(get_reg_type(i), init_reg_fixed(i));
				}
			}

			// Set result
			m_block->reg[3] = _call;
		}
	}

	// Emit return from the real function
	void ret_function()
	{
		m_ir->CreateRet(get_reg_fixed<u32[4]>(3).value);
	}

	void set_function(llvm::Function* func)
	{
		m_function = func;
		m_thread = func->getArg(0);
		m_lsptr = func->getArg(1);
		m_base_pc = func->getArg(2);

		m_reg_addr.fill(nullptr);
		m_block = nullptr;
		m_finfo = nullptr;
		m_blocks.clear();
		m_block_queue.clear();
		m_ir->SetInsertPoint(llvm::BasicBlock::Create(m_context, "", m_function));
		m_memptr = m_ir->CreateLoad(get_type<u8*>(), spu_ptr(&spu_thread::memory_base_addr));
	}

	// Add block with current block as a predecessor
	llvm::BasicBlock* add_block(u32 target, bool absolute = false)
	{
		// Check the predecessor
		const bool pred_found = m_block_info[target / 4] && std::find(m_preds[target].begin(), m_preds[target].end(), m_pos) != m_preds[target].end();

		if (m_blocks.empty())
		{
			// Special case: first block, proceed normally
			if (auto fn = std::exchange(m_finfo->fn, nullptr))
			{
				// Create a gateway
				call_function(fn, true);

				m_finfo->fn = fn;
				m_function = fn;
				m_thread = fn->getArg(0);
				m_lsptr = fn->getArg(1);
				m_base_pc = fn->getArg(2);
				m_ir->SetInsertPoint(llvm::BasicBlock::Create(m_context, "", fn));
				m_memptr = m_ir->CreateLoad(get_type<u8*>(), spu_ptr(&spu_thread::memory_base_addr));

				// Load registers at the entry chunk
				for (u32 i = 0; i < s_reg_max; i++)
				{
					if (i >= s_reg_80 && i <= s_reg_127)
					{
						// TODO
						//m_finfo->load[i] = llvm::UndefValue::get(get_reg_type(i));
					}

					m_finfo->load[i] = m_ir->CreateLoad(get_reg_type(i), init_reg_fixed(i));
				}

				// Load $SP
				m_finfo->load[s_reg_sp] = fn->getArg(3);

				// Load first args
				m_finfo->load[3] = fn->getArg(4);
			}
		}
		else if (m_block_info[target / 4] && m_entry_info[target / 4] && !(pred_found && m_entry == target) && (!m_finfo->fn || !m_ret_info[target / 4]))
		{
			// Generate a tail call to the function chunk
			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			const auto pfinfo = add_function(target);

			if (absolute)
			{
				ensure(!m_finfo->fn);

				const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
				m_ir->CreateCondBr(m_ir->CreateICmpEQ(m_base_pc, m_ir->getInt32(m_base)), next, fail);
				m_ir->SetInsertPoint(fail);
				m_ir->CreateStore(m_ir->getInt32(target), spu_ptr(&spu_thread::pc));
				tail_chunk(nullptr);
				m_ir->SetInsertPoint(next);
			}

			if (pfinfo->fn)
			{
				// Tail call to the real function
				call_function(pfinfo->fn, true);

				if (!result->getTerminator())
					ret_function();
			}
			else
			{
				// Just a boring tail call to another chunk
				update_pc(target);
				tail_chunk(pfinfo->chunk);
			}

			m_ir->SetInsertPoint(cblock);
			return result;
		}
		else if (!pred_found || !m_block_info[target / 4])
		{
			if (m_block_info[target / 4])
			{
				spu_log.error("[%s] [0x%x] Predecessor not found for target 0x%x (chunk=0x%x, entry=0x%x, size=%u)", m_hash, m_pos, target, m_entry, m_function_queue[0], m_size / 4);
			}

			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);

			if (absolute)
			{
				ensure(!m_finfo->fn);

				m_ir->CreateStore(m_ir->getInt32(target), spu_ptr(&spu_thread::pc));
			}
			else
			{
				update_pc(target);
			}

			tail_chunk(nullptr);
			m_ir->SetInsertPoint(cblock);
			return result;
		}

		auto& result = m_blocks[target].block;

		if (!result)
		{
			result = llvm::BasicBlock::Create(m_context, fmt::format("b-0x%x", target), m_function);

			// Add the block to the queue
			m_block_queue.push_back(target);
		}
		else if (m_block && m_blocks[target].block_end)
		{
			// Connect PHI nodes if necessary
			for (u32 i = 0; i < s_reg_max; i++)
			{
				if (const auto phi = m_blocks[target].phi[i])
				{
					const auto typ = phi->getType() == get_type<f64[4]>() ? get_type<f64[4]>() : get_reg_type(i);
					phi->addIncoming(get_reg_fixed(i, typ), m_block->block_end);
				}
			}
		}

		return result;
	}

	llvm::Value* _ptr(llvm::Value* base, u32 offset)
	{
		return m_ir->CreatePtrAdd(base, m_ir->getInt64(offset));
	}

	llvm::Value* _ptr(llvm::Value* base, llvm::Value* offset)
	{
		return m_ir->CreatePtrAdd(base, offset);
	}

	template <typename... Args>
	llvm::Value* _ptr(llvm::Value* base, Args... offset_args)
	{
		return m_ir->CreatePtrAdd(base, m_ir->getInt64(::offset32(offset_args...)));
	}

	template <typename... Args>
	llvm::Value* spu_ptr(Args... offset_args)
	{
		return _ptr(m_thread, ::offset32(offset_args...));
	}

	// Return default register type
	llvm::Type* get_reg_type(u32 index)
	{
		if (index < 128)
		{
			return get_type<u32[4]>();
		}

		switch (index)
		{
		case s_reg_mfc_eal:
		case s_reg_mfc_lsa:
			return get_type<u32>();
		case s_reg_mfc_tag:
			return get_type<u8>();
		case s_reg_mfc_size:
			return get_type<u16>();
		default:
			fmt::throw_exception("get_reg_type(%u): invalid register index", index);
		}
	}

	u32 get_reg_offset(u32 index)
	{
		if (index < 128)
		{
			return ::offset32(&spu_thread::gpr, index);
		}

		switch (index)
		{
		case s_reg_mfc_eal: return ::offset32(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::eal);
		case s_reg_mfc_lsa: return ::offset32(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::lsa);
		case s_reg_mfc_tag: return ::offset32(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::tag);
		case s_reg_mfc_size: return ::offset32(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::size);
		default:
			fmt::throw_exception("get_reg_offset(%u): invalid register index", index);
		}
	}

	llvm::Value* init_reg_fixed(u32 index)
	{
		if (!m_block)
		{
			return _ptr(m_thread, get_reg_offset(index));
		}

		auto& ptr = ::at32(m_reg_addr, index);

		if (!ptr)
		{
			// Save and restore current insert point if necessary
			const auto block_cur = m_ir->GetInsertBlock();

			// Emit register pointer at the beginning of the function chunk
			m_ir->SetInsertPoint(m_function->getEntryBlock().getTerminator());
			ptr = _ptr(m_thread, get_reg_offset(index));
			m_ir->SetInsertPoint(block_cur);
		}

		return ptr;
	}

	// Get pointer to the vector register (interpreter only)
	template <uint I>
	llvm::Value* init_vr(const bf_t<u32, I, 7>&)
	{
		if (!m_interp_magn)
		{
			m_interp_7f0 = m_ir->getInt32(0x7f0);
			m_interp_regs = _ptr(m_thread, get_reg_offset(0));
		}

		// Extract reg index
		const auto isl = I >= 4 ? m_interp_op : m_ir->CreateShl(m_interp_op, u64{4 - I});
		const auto isr = I <= 4 ? m_interp_op : m_ir->CreateLShr(m_interp_op, u64{I - 4});
		const auto idx = m_ir->CreateAnd(I > 4 ? isr : isl, m_interp_7f0);

		// Pointer to the register
		return _ptr(m_interp_regs, m_ir->CreateZExt(idx, get_type<u64>()));
	}

	llvm::Value* double_as_uint64(llvm::Value* val)
	{
		return bitcast<u64[4]>(val);
	}

	llvm::Value* uint64_as_double(llvm::Value* val)
	{
		return bitcast<f64[4]>(val);
	}

	llvm::Value* double_to_xfloat(llvm::Value* val)
	{
		ensure(val && val->getType() == get_type<f64[4]>());

		const auto d = double_as_uint64(val);
		const auto s = m_ir->CreateAnd(m_ir->CreateLShr(d, 32), 0x80000000);
		const auto m = m_ir->CreateXor(m_ir->CreateLShr(d, 29), 0x40000000);
		const auto r = m_ir->CreateOr(m_ir->CreateAnd(m, 0x7fffffff), s);
		return m_ir->CreateTrunc(m_ir->CreateSelect(m_ir->CreateIsNotNull(d), r, splat<u64[4]>(0).eval(m_ir)), get_type<u32[4]>());
	}

	llvm::Value* xfloat_to_double(llvm::Value* val)
	{
		ensure(val && val->getType() == get_type<u32[4]>());

		const auto x = m_ir->CreateZExt(val, get_type<u64[4]>());
		const auto s = m_ir->CreateShl(m_ir->CreateAnd(x, 0x80000000), 32);
		const auto a = m_ir->CreateAnd(x, 0x7fffffff);
		const auto m = m_ir->CreateShl(m_ir->CreateAdd(a, splat<u64[4]>(0x1c0000000).eval(m_ir)), 29);
		const auto r = m_ir->CreateSelect(m_ir->CreateICmpSGT(a, splat<u64[4]>(0x7fffff).eval(m_ir)), m, splat<u64[4]>(0).eval(m_ir));
		const auto f = m_ir->CreateOr(s, r);
		return uint64_as_double(f);
	}

	// Clamp double values to ±Smax, flush values smaller than ±Smin to positive zero
	llvm::Value* xfloat_in_double(llvm::Value* val)
	{
		ensure(val && val->getType() == get_type<f64[4]>());

		const auto smax = uint64_as_double(splat<u64[4]>(0x47ffffffe0000000).eval(m_ir));
		const auto smin = uint64_as_double(splat<u64[4]>(0x3810000000000000).eval(m_ir));

		const auto d = double_as_uint64(val);
		const auto s = m_ir->CreateAnd(d, 0x8000000000000000);
		const auto a = uint64_as_double(m_ir->CreateAnd(d, 0x7fffffffe0000000));
		const auto n = m_ir->CreateFCmpOLT(a, smax);
		const auto z = m_ir->CreateFCmpOLT(a, smin);
		const auto c = double_as_uint64(m_ir->CreateSelect(n, a, smax));
		return m_ir->CreateSelect(z, fsplat<f64[4]>(0.).eval(m_ir), uint64_as_double(m_ir->CreateOr(c, s)));
	}

	// Expand 32-bit mask for xfloat values to 64-bit, 29 least significant bits are always zero
	llvm::Value* conv_xfloat_mask(llvm::Value* val)
	{
		const auto d = m_ir->CreateZExt(val, get_type<u64[4]>());
		const auto s = m_ir->CreateShl(m_ir->CreateAnd(d, 0x80000000), 32);
		const auto e = m_ir->CreateLShr(m_ir->CreateAShr(m_ir->CreateShl(d, 33), 4), 1);
		return m_ir->CreateOr(s, e);
	}

	llvm::Value* get_reg_raw(u32 index)
	{
		if (!m_block || index >= m_block->reg.size())
		{
			return nullptr;
		}

		return m_block->reg[index];
	}

	llvm::Value* get_reg_fixed(u32 index, llvm::Type* type)
	{
		llvm::Value* dummy{};

		auto& reg = *(m_block ? &::at32(m_block->reg, index) : &dummy);

		if (!reg)
		{
			// Load register value if necessary
			reg = m_finfo && m_finfo->load[index] ? m_finfo->load[index] : m_ir->CreateLoad(get_reg_type(index), init_reg_fixed(index));
		}

		if (reg->getType() == get_type<f64[4]>())
		{
			if (type == reg->getType())
			{
				return reg;
			}

			return bitcast(double_to_xfloat(reg), type);
		}

		if (type == get_type<f64[4]>())
		{
			return xfloat_to_double(bitcast<u32[4]>(reg));
		}

		return bitcast(reg, type);
	}

	template <typename T = u32[4]>
	value_t<T> get_reg_fixed(u32 index)
	{
		value_t<T> r;
		r.value = get_reg_fixed(index, get_type<T>());
		return r;
	}

	template <typename T = u32[4], uint I>
	value_t<T> get_vr(const bf_t<u32, I, 7>& index)
	{
		value_t<T> r;

		if ((m_op_const_mask & index.data_mask()) != index.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32u - m_interp_magn))
			{
				m_op_const_mask |= index.data_mask();
			}

			// Load reg
			if (get_type<T>() == get_type<f64[4]>())
			{
				r.value = xfloat_to_double(m_ir->CreateLoad(get_type<u32[4]>(), init_vr(index)));
			}
			else
			{
				r.value = m_ir->CreateLoad(get_type<T>(), init_vr(index));
			}
		}
		else
		{
			r.value = get_reg_fixed(index, get_type<T>());
		}

		return r;
	}

	template <typename U, uint I>
	auto get_vr_as(U&&, const bf_t<u32, I, 7>& index)
	{
		return get_vr<typename llvm_expr_t<U>::type>(index);
	}

	template <typename T = u32[4], typename... Args>
	std::tuple<std::conditional_t<false, Args, value_t<T>>...> get_vrs(const Args&... args)
	{
		return {get_vr<T>(args)...};
	}

	template <typename T = u32[4], uint I>
	llvm_match_t<T> match_vr(const bf_t<u32, I, 7>& index)
	{
		llvm_match_t<T> r;

		if (m_block)
		{
			auto v = ::at32(m_block->reg, index);

			if (v && v->getType() == get_type<T>())
			{
				r.value = v;
				return r;
			}
		}

		return r;
	}

	template <typename U, uint I>
	auto match_vr_as(U&&, const bf_t<u32, I, 7>& index)
	{
		return match_vr<typename llvm_expr_t<U>::type>(index);
	}

	template <typename... Types, uint I, typename F>
	bool match_vr(const bf_t<u32, I, 7>& index, F&& pred)
	{
		return (( match_vr<Types>(index) ? pred(match_vr<Types>(index), match<Types>()) : false ) || ...);
	}

	template <typename T = u32[4], typename... Args>
	std::tuple<std::conditional_t<false, Args, llvm_match_t<T>>...> match_vrs(const Args&... args)
	{
		return {match_vr<T>(args)...};
	}

	// Extract scalar value from the preferred slot
	template <typename T>
	auto get_scalar(value_t<T> value)
	{
		using e_type = std::remove_extent_t<T>;

		static_assert(sizeof(T) == 16 || std::is_same_v<f64[4], T>, "Unknown vector type");

		if (auto [ok, v] = match_expr(value, vsplat<T>(match<e_type>())); ok)
		{
			return eval(v);
		}

		if constexpr (sizeof(e_type) == 1)
		{
			return eval(extract(value, 12));
		}
		else if constexpr (sizeof(e_type) == 2)
		{
			return eval(extract(value, 6));
		}
		else if constexpr (sizeof(e_type) == 4 || sizeof(T) == 32)
		{
			return eval(extract(value, 3));
		}
		else
		{
			return eval(extract(value, 1));
		}
	}

	// Splat scalar value from the preferred slot
	template <typename T>
	auto splat_scalar(T&& arg)
	{
		using VT = std::remove_extent_t<typename std::decay_t<T>::type>;

		if constexpr (sizeof(VT) == 1)
		{
			return zshuffle(std::forward<T>(arg), 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12);
		}
		else if constexpr (sizeof(VT) == 2)
		{
			return zshuffle(std::forward<T>(arg), 6, 6, 6, 6, 6, 6, 6, 6);
		}
		else if constexpr (sizeof(VT) == 4)
		{
			return zshuffle(std::forward<T>(arg), 3, 3, 3, 3);
		}
		else if constexpr (sizeof(VT) == 8)
		{
			return zshuffle(std::forward<T>(arg), 1, 1);
		}
		else
		{
			static_assert(sizeof(VT) == 16);
			return std::forward<T>(arg);
		}
	}

	void set_reg_fixed(u32 index, llvm::Value* value, bool fixup = true)
	{
		llvm::StoreInst* dummy{};

		// Check
		ensure(!m_block || m_regmod[m_pos / 4] == index);

		// Test for special case
		const bool is_xfloat = value->getType() == get_type<f64[4]>();

		// Clamp value if necessary
		const auto saved_value = is_xfloat && fixup ? xfloat_in_double(value) : value;

		// Set register value
		if (m_block)
		{
#ifndef _WIN32
			if (g_cfg.core.spu_debug)
				value->setName(fmt::format("result_0x%05x", m_pos));
#endif

			::at32(m_block->reg, index) = saved_value;
		}

		// Get register location
		const auto addr = init_reg_fixed(index);

		auto& _store = *(m_block ? &m_block->store[index] : &dummy);

		// Erase previous dead store instruction if necessary
		if (_store)
		{
			if (m_block->store_context_last_id[index] == m_block->store_context_ctr[index])
			{
				// Erase store of it is not preserved by ensure_gpr_stores()
				_store->eraseFromParent();
			}
		}

		if (m_block)
		{
			// Keep the store's location in history of gpr preservaions
			m_block->store_context_last_id[index] = m_block->store_context_ctr[index];
			m_block->store_context_first_id[index] = std::min<usz>(m_block->store_context_first_id[index], m_block->store_context_ctr[index]);
		}

		if (m_finfo && m_finfo->fn)
		{
			if (index <= 3 || (index >= s_reg_80 && index <= s_reg_127))
			{
				// Don't save some registers in true functions
				return;
			}
		}

		// Write register to the context
		_store = m_ir->CreateStore(is_xfloat ? double_to_xfloat(saved_value) : m_ir->CreateBitCast(value, get_reg_type(index)), addr);
	}

	template <typename T, uint I>
	void set_vr(const bf_t<u32, I, 7>& index, T expr, std::function<llvm::KnownBits()> vr_assume = nullptr, bool fixup = true)
	{
		// Process expression
		const auto value = expr.eval(m_ir);

		// Test for special case
		const bool is_xfloat = value->getType() == get_type<f64[4]>();

		if ((m_op_const_mask & index.data_mask()) != index.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32u - m_interp_magn))
			{
				m_op_const_mask |= index.data_mask();
			}

			// Clamp value if necessary
			const auto saved_value = is_xfloat && fixup ? xfloat_in_double(value) : value;

			// Store value
			m_ir->CreateStore(is_xfloat ? double_to_xfloat(saved_value) : m_ir->CreateBitCast(value, get_type<u32[4]>()), init_vr(index));
			return;
		}

		if (vr_assume)
		{
		}

		set_reg_fixed(index, value, fixup);
	}

	template <typename T = u32[4], uint I, uint N>
	value_t<T> get_imm(const bf_t<u32, I, N>& imm, bool mask = true)
	{
		if ((m_op_const_mask & imm.data_mask()) != imm.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32u - m_interp_magn))
			{
				m_op_const_mask |= imm.data_mask();
			}

			// Extract unsigned immediate (skip AND if mask == false or truncated anyway)
			value_t<T> r;
			r.value = m_interp_op;
			r.value = I == 0 ? r.value : m_ir->CreateLShr(r.value, u64{I});
			r.value = !mask || N >= r.esize ? r.value : m_ir->CreateAnd(r.value, imm.data_mask() >> I);

			if constexpr (r.esize != 32)
			{
				r.value = m_ir->CreateZExtOrTrunc(r.value, get_type<T>()->getScalarType());
			}

			if (r.is_vector)
			{
				r.value = m_ir->CreateVectorSplat(r.is_vector, r.value);
			}

			return r;
		}

		return eval(splat<T>(imm));
	}

	template <typename T = u32[4], uint I, uint N>
	value_t<T> get_imm(const bf_t<s32, I, N>& imm)
	{
		if ((m_op_const_mask & imm.data_mask()) != imm.data_mask())
		{
			// Update const mask if necessary
			if (I >= (32u - m_interp_magn))
			{
				m_op_const_mask |= imm.data_mask();
			}

			// Extract signed immediate (skip sign ext if truncated anyway)
			value_t<T> r;
			r.value = m_interp_op;
			r.value = I + N == 32 || N >= r.esize ? r.value : m_ir->CreateShl(r.value, u64{32u - I - N});
			r.value = N == 32 || N >= r.esize ? r.value : m_ir->CreateAShr(r.value, u64{32u - N});
			r.value = I == 0 || N < r.esize ? r.value : m_ir->CreateLShr(r.value, u64{I});

			if constexpr (r.esize != 32)
			{
				r.value = m_ir->CreateSExtOrTrunc(r.value, get_type<T>()->getScalarType());
			}

			if (r.is_vector)
			{
				r.value = m_ir->CreateVectorSplat(r.is_vector, r.value);
			}

			return r;
		}

		return eval(splat<T>(imm));
	}

	// Get PC for given instruction address
	llvm::Value* get_pc(u32 addr)
	{
		return m_ir->CreateAdd(m_base_pc, m_ir->getInt32(addr - m_base));
	}

	// Update PC for current or explicitly specified instruction address
	void update_pc(u32 target = -1)
	{
		m_ir->CreateStore(m_ir->CreateAnd(get_pc(target + 1 ? target : m_pos), 0x3fffc), spu_ptr(&spu_thread::pc))->setVolatile(true);
	}

	// Call cpu_thread::check_state if necessary and return or continue (full check)
	void check_state(u32 addr, bool may_be_unsafe_for_savestate = true)
	{
		const auto pstate = spu_ptr(&spu_thread::state);
		const auto _body = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto check = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(m_ir->CreateICmpEQ(m_ir->CreateLoad(get_type<u32>(), pstate, true), m_ir->getInt32(0)), _body, check, m_md_likely);
		m_ir->SetInsertPoint(check);
		update_pc(addr);

		if (may_be_unsafe_for_savestate && m_block && m_block->bb->preds.empty())
		{
			may_be_unsafe_for_savestate = false;
		}

		if (may_be_unsafe_for_savestate)
		{
			m_ir->CreateStore(m_ir->getInt8(1), spu_ptr(&spu_thread::unsavable))->setVolatile(true);
		}

		m_ir->CreateCall(m_test_state, {m_thread});

		if (may_be_unsafe_for_savestate)
		{
			m_ir->CreateStore(m_ir->getInt8(0), spu_ptr(&spu_thread::unsavable))->setVolatile(true);
		}

		m_ir->CreateBr(_body);
		m_ir->SetInsertPoint(_body);
	}

	void putllc16_pattern(const spu_program& /*prog*/, utils::address_range32 range)
	{
		// Prevent store elimination
		m_block->store_context_ctr[s_reg_mfc_eal]++;
		m_block->store_context_ctr[s_reg_mfc_lsa]++;
		m_block->store_context_ctr[s_reg_mfc_tag]++;
		m_block->store_context_ctr[s_reg_mfc_size]++;

		static const auto on_fail = [](spu_thread* _spu, u32 addr)
		{
			if (const u32 raddr = _spu->raddr)
			{
				// Last check for event before we clear the reservation
				if (~_spu->ch_events.load().events & SPU_EVENT_LR)
				{
					if (raddr == addr)
					{
						_spu->set_events(SPU_EVENT_LR);
					}
					else
					{
						_spu->get_events(SPU_EVENT_LR);
					}
				}

				_spu->raddr = 0;
			}
		};

		const union putllc16_info
		{
			u32 data;
			bf_t<u32, 30, 2> type;
			bf_t<u32, 29, 1> runtime16_select;
			bf_t<u32, 28, 1> no_notify;
			bf_t<u32, 18, 8> reg;
			bf_t<u32, 0, 18> off18;
			bf_t<u32, 0, 8> reg2;
		} info = std::bit_cast<putllc16_info>(range.end);

		enum : u32
		{
			v_const = 0,
			v_relative = 1,
			v_reg_offs = 2,
			v_reg2 = 3,
		};

		const auto _raddr_match = llvm::BasicBlock::Create(m_context, "__raddr_match", m_function);
		const auto _lock_success = llvm::BasicBlock::Create(m_context, "__putllc16_lock", m_function);
		const auto _begin_op = llvm::BasicBlock::Create(m_context, "__putllc16_begin", m_function);
		const auto _repeat_lock = llvm::BasicBlock::Create(m_context, "__putllc16_repeat", m_function);
		const auto _repeat_lock_fail = llvm::BasicBlock::Create(m_context, "__putllc16_lock_fail", m_function);
		const auto _success = llvm::BasicBlock::Create(m_context, "__putllc16_success", m_function);
		const auto _inc_res = llvm::BasicBlock::Create(m_context, "__putllc16_inc_resv", m_function);
		const auto _inc_res_unlocked = llvm::BasicBlock::Create(m_context, "__putllc16_inc_resv_unlocked", m_function);
		const auto _success_and_unlock = llvm::BasicBlock::Create(m_context, "__putllc16_succ_unlock", m_function);
		const auto _fail = llvm::BasicBlock::Create(m_context, "__putllc16_fail", m_function);
		const auto _fail_and_unlock = llvm::BasicBlock::Create(m_context, "__putllc16_unlock", m_function);
		const auto _final = llvm::BasicBlock::Create(m_context, "__putllc16_final", m_function);

		const auto _eal = (get_reg_fixed<u32>(s_reg_mfc_eal) & -128).eval(m_ir);
		const auto _raddr = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::raddr));

		m_ir->CreateCondBr(m_ir->CreateAnd(m_ir->CreateICmpEQ(_eal, _raddr), m_ir->CreateIsNotNull(_raddr)), _raddr_match, _fail, m_md_likely);
		m_ir->SetInsertPoint(_raddr_match);

		value_t<u32> eal_val;
		eal_val.value = _eal;

		auto get_reg32 = [&](u32 reg)
		{
			if (get_reg_type(reg) != get_type<u32[4]>())
			{
				return get_reg_fixed(reg, get_type<u32>());
			}

			return extract(get_reg_fixed(reg), 3).eval(m_ir);
		};

		const auto _lsa = (get_reg_fixed<u32>(s_reg_mfc_lsa) & 0x3ff80).eval(m_ir);

		llvm::Value* dest{};

		if (info.type == v_const)
		{
			dest = m_ir->getInt32(info.off18);
		}
		else if (info.type == v_relative)
		{
			dest = m_ir->CreateAnd(get_pc(spu_branch_target(info.off18 + m_base)), 0x3fff0);
		}
		else if (info.type == v_reg_offs)
		{
			dest = m_ir->CreateAnd(m_ir->CreateAdd(get_reg32(info.reg), m_ir->getInt32(info.off18)), 0x3fff0);
		}
		else
		{
			dest = m_ir->CreateAnd(m_ir->CreateAdd(get_reg32(info.reg), get_reg32(info.reg2)), 0x3fff0);
		}

		if (g_cfg.core.rsx_accurate_res_access)
		{
			const auto success = call("spu_putllc16_rsx_res", +[](spu_thread* _spu, u32 ls_dst, u32 lsa, u32 eal, u32 notify) -> bool
			{
				const u32 raddr = eal;

				const v128 rdata = read_from_ptr<v128>(_spu->rdata, ls_dst % 0x80);
				const v128 to_write = _spu->_ref<const nse_t<v128>>(ls_dst);

				const auto dest = raddr | (ls_dst & 127);
				const auto _dest = vm::get_super_ptr<atomic_t<nse_t<v128>>>(dest);

				if (rdata == to_write || ((lsa ^ ls_dst) & (SPU_LS_SIZE - 128)))
				{
					vm::reservation_update(raddr);
					_spu->ch_atomic_stat.set_value(MFC_PUTLLC_SUCCESS);
					_spu->raddr = 0;
					return true;
				}

				auto& res = vm::reservation_acquire(eal);

				if (res % 128)
				{
					return false;
				}

				{
					rsx::reservation_lock rsx_lock(raddr, 128);

					// Touch memory
					utils::trigger_write_page_fault(vm::base(dest ^ (4096 / 2)));

					auto [old_res, ok] = res.fetch_op([&](u64& rval)
					{
						if (rval % 128)
						{
							return false;
						}

						rval |= 127;
						return true;
					});

					if (!ok)
					{
						return false;
					}

					if (!_dest->compare_and_swap_test(rdata, to_write))
					{
						res.release(old_res);
						return false;
					}

					// Success
					res.release(old_res + 128);
				}

				_spu->ch_atomic_stat.set_value(MFC_PUTLLC_SUCCESS);
				_spu->raddr = 0;

				if (notify)
				{
					res.notify_all();
				}

				return true;
			}, m_thread, dest, _lsa, _eal, m_ir->getInt32(!info.no_notify));


			m_ir->CreateCondBr(success, _final, _fail);

			m_ir->SetInsertPoint(_fail);
			call("PUTLLC16_fail", +on_fail, m_thread, _eal);
			m_ir->CreateStore(m_ir->getInt64(spu_channel::bit_count | MFC_PUTLLC_FAILURE), spu_ptr(&spu_thread::ch_atomic_stat));
			m_ir->CreateBr(_final);

			m_ir->SetInsertPoint(_final);
			return;
		}

		const auto diff = m_ir->CreateZExt(m_ir->CreateSub(dest, _lsa), get_type<u64>());

		const auto _new = m_ir->CreateAlignedLoad(get_type<u128>(), _ptr(m_lsptr, dest), llvm::MaybeAlign{16});
		const auto _rdata = m_ir->CreateAlignedLoad(get_type<u128>(), _ptr(spu_ptr(&spu_thread::rdata), m_ir->CreateAnd(diff, 0x70)), llvm::MaybeAlign{16});

		const bool is_accurate_op = !!g_cfg.core.spu_accurate_reservations;

		const auto compare_data_change_res = is_accurate_op ? m_ir->getTrue() : m_ir->CreateICmpNE(_new, _rdata);

		if (info.runtime16_select)
		{
			m_ir->CreateCondBr(m_ir->CreateAnd(m_ir->CreateICmpULT(diff, m_ir->getInt64(128)), compare_data_change_res), _begin_op, _inc_res, m_md_likely);
		}
		else
		{
			m_ir->CreateCondBr(compare_data_change_res, _begin_op, _inc_res, m_md_unlikely);
		}

		m_ir->SetInsertPoint(_begin_op);

		// Touch memory (on the opposite side of the page)
		m_ir->CreateAtomicRMW(llvm::AtomicRMWInst::Or, _ptr(m_memptr, m_ir->CreateXor(_eal, 4096 / 2)), m_ir->getInt8(0), llvm::MaybeAlign{16}, llvm::AtomicOrdering::SequentiallyConsistent);

		const auto rptr = _ptr(m_ir->CreateLoad(get_type<u8*>(), spu_ptr(&spu_thread::reserv_base_addr)), ((eal_val & 0xff80) >> 1).eval(m_ir));
		const auto rtime = m_ir->CreateLoad(get_type<u64>(), spu_ptr(&spu_thread::rtime));

		m_ir->CreateBr(_repeat_lock);
		m_ir->SetInsertPoint(_repeat_lock);

		const auto rval = m_ir->CreatePHI(get_type<u64>(), 2);
		rval->addIncoming(rtime, _begin_op);

		// Lock reservation
		const auto cmp_res = m_ir->CreateAtomicCmpXchg(rptr, rval, m_ir->CreateOr(rval, 0x7f), llvm::MaybeAlign{16}, llvm::AtomicOrdering::SequentiallyConsistent, llvm::AtomicOrdering::SequentiallyConsistent);

		m_ir->CreateCondBr(m_ir->CreateExtractValue(cmp_res, 1), _lock_success, _repeat_lock_fail, m_md_likely);

		m_ir->SetInsertPoint(_repeat_lock_fail);

		const auto last_rval = m_ir->CreateExtractValue(cmp_res, 0);
		rval->addIncoming(last_rval, _repeat_lock_fail);

		m_ir->CreateCondBr(is_accurate_op ? m_ir->CreateICmpEQ(last_rval, rval) : m_ir->CreateIsNull(m_ir->CreateAnd(last_rval, 0x7f)), _repeat_lock, _fail);

		m_ir->SetInsertPoint(_lock_success);

		// Commit 16 bytes compare-exchange
		const auto sudo_ptr = _ptr(m_ir->CreateLoad(get_type<u8*>(), spu_ptr(&spu_thread::memory_sudo_addr)), _eal);

		m_ir->CreateCondBr(
			m_ir->CreateExtractValue(m_ir->CreateAtomicCmpXchg(_ptr(sudo_ptr, diff), _rdata, _new, llvm::MaybeAlign{16}, llvm::AtomicOrdering::SequentiallyConsistent, llvm::AtomicOrdering::SequentiallyConsistent), 1)
			, _success_and_unlock
			, _fail_and_unlock);

		// Unlock and notify
		m_ir->SetInsertPoint(_success_and_unlock);
		m_ir->CreateAlignedStore(m_ir->CreateAdd(rval, m_ir->getInt64(128)), rptr, llvm::MaybeAlign{8});

		if (!info.no_notify)
		{
			call("atomic_wait_engine::notify_all", static_cast<void(*)(const void*)>(atomic_wait_engine::notify_all), rptr);
		}

		m_ir->CreateBr(_success);

		// Perform unlocked vm::reservation_update if no physical memory changes needed
		m_ir->SetInsertPoint(_inc_res);
		const auto rptr2 = _ptr(m_ir->CreateLoad(get_type<u8*>(), spu_ptr(&spu_thread::reserv_base_addr)), ((eal_val & 0xff80) >> 1).eval(m_ir));

		llvm::Value* old_val{};

		if (true || is_accurate_op)
		{
			old_val = m_ir->CreateLoad(get_type<u64>(), spu_ptr(&spu_thread::rtime));
		}
		else
		{
			old_val = m_ir->CreateAlignedLoad(get_type<u64>(), rptr2, llvm::MaybeAlign{8});
			m_ir->CreateCondBr(m_ir->CreateIsNotNull(m_ir->CreateAnd(old_val, 0x7f)), _success, _inc_res_unlocked);
			m_ir->SetInsertPoint(_inc_res_unlocked);
		}

		const auto cmp_res2 = m_ir->CreateAtomicCmpXchg(rptr2, old_val, m_ir->CreateAdd(old_val, m_ir->getInt64(128)), llvm::MaybeAlign{16}, llvm::AtomicOrdering::SequentiallyConsistent, llvm::AtomicOrdering::SequentiallyConsistent);

		if (true || is_accurate_op)
		{
			m_ir->CreateCondBr(m_ir->CreateExtractValue(cmp_res2, 1), _success, _fail);
		}
		else
		{
			m_ir->CreateBr(_success);
		}

		m_ir->SetInsertPoint(_success);
		m_ir->CreateStore(m_ir->getInt64(spu_channel::bit_count | MFC_PUTLLC_SUCCESS), spu_ptr(&spu_thread::ch_atomic_stat));
		m_ir->CreateStore(m_ir->getInt32(0), spu_ptr(&spu_thread::raddr));
		m_ir->CreateBr(_final);

		m_ir->SetInsertPoint(_fail_and_unlock);
		m_ir->CreateAlignedStore(rval, rptr, llvm::MaybeAlign{8});
		m_ir->CreateBr(_fail);

		m_ir->SetInsertPoint(_fail);
		call("PUTLLC16_fail", +on_fail, m_thread, _eal);
		m_ir->CreateStore(m_ir->getInt64(spu_channel::bit_count | MFC_PUTLLC_FAILURE), spu_ptr(&spu_thread::ch_atomic_stat));
		m_ir->CreateBr(_final);

		m_ir->SetInsertPoint(_final);
	}

	void putllc0_pattern(const spu_program& /*prog*/, utils::address_range32 /*range*/)
	{
		// Prevent store elimination
		m_block->store_context_ctr[s_reg_mfc_eal]++;
		m_block->store_context_ctr[s_reg_mfc_lsa]++;
		m_block->store_context_ctr[s_reg_mfc_tag]++;
		m_block->store_context_ctr[s_reg_mfc_size]++;

		static const auto on_fail = [](spu_thread* _spu, u32 addr)
		{
			if (const u32 raddr = _spu->raddr)
			{
				// Last check for event before we clear the reservation
				if (~_spu->ch_events.load().events & SPU_EVENT_LR)
				{
					if (raddr == addr)
					{
						_spu->set_events(SPU_EVENT_LR);
					}
					else
					{
						_spu->get_events(SPU_EVENT_LR);
					}
				}
				_spu->raddr = 0;
			}
		};

		const auto _next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto _next0 = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto _fail = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto _final = llvm::BasicBlock::Create(m_context, "", m_function);

		const auto _eal = (get_reg_fixed<u32>(s_reg_mfc_eal) & -128).eval(m_ir);
		const auto _raddr = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::raddr));

		m_ir->CreateCondBr(m_ir->CreateAnd(m_ir->CreateICmpEQ(_eal, _raddr), m_ir->CreateIsNotNull(_raddr)), _next, _fail, m_md_likely);
		m_ir->SetInsertPoint(_next);

		value_t<u32> eal_val;
		eal_val.value = _eal;

		const auto rptr = _ptr(m_ir->CreateLoad(get_type<u8*>(), spu_ptr(&spu_thread::reserv_base_addr)), ((eal_val & 0xff80) >> 1).eval(m_ir));
		const auto rval = m_ir->CreateLoad(get_type<u64>(), spu_ptr(&spu_thread::rtime));
		m_ir->CreateCondBr(
			m_ir->CreateExtractValue(m_ir->CreateAtomicCmpXchg(rptr, rval, m_ir->CreateAdd(rval, m_ir->getInt64(128)), llvm::MaybeAlign{16}, llvm::AtomicOrdering::SequentiallyConsistent, llvm::AtomicOrdering::SequentiallyConsistent), 1)
			, _next0
			, g_cfg.core.spu_accurate_reservations ? _fail : _next0); // Succeed unconditionally

		m_ir->SetInsertPoint(_next0);
		//call("atomic_wait_engine::notify_all", static_cast<void(*)(const void*)>(atomic_wait_engine::notify_all), rptr);
		m_ir->CreateStore(m_ir->getInt64(spu_channel::bit_count | MFC_PUTLLC_SUCCESS), spu_ptr(&spu_thread::ch_atomic_stat));
		m_ir->CreateBr(_final);

		m_ir->SetInsertPoint(_fail);
		call("PUTLLC0_fail", +on_fail, m_thread, _eal);
		m_ir->CreateStore(m_ir->getInt64(spu_channel::bit_count | MFC_PUTLLC_FAILURE), spu_ptr(&spu_thread::ch_atomic_stat));
		m_ir->CreateBr(_final);

		m_ir->SetInsertPoint(_final);
		m_ir->CreateStore(m_ir->getInt32(0), spu_ptr(&spu_thread::raddr));
	}

public:
	spu_llvm_recompiler(u8 interp_magn = 0)
		: spu_recompiler_base()
		, cpu_translator(nullptr, false)
		, m_interp_magn(interp_magn)
	{
	}

	virtual void init() override
	{
		// Initialize if necessary
		if (!m_spurt)
		{
			m_spurt = &g_fxo->get<spu_runtime>();
			cpu_translator::initialize(m_jit.get_context(), m_jit.get_engine());

			const auto md_name = llvm::MDString::get(m_context, "branch_weights");
			const auto md_low = llvm::ValueAsMetadata::get(llvm::ConstantInt::get(GetType<u32>(), 1));
			const auto md_high = llvm::ValueAsMetadata::get(llvm::ConstantInt::get(GetType<u32>(), 999));

			// Metadata for branch weights
			m_md_likely = llvm::MDTuple::get(m_context, {md_name, md_high, md_low});
			m_md_unlikely = llvm::MDTuple::get(m_context, {md_name, md_low, md_high});

			// Initialize transform passes
			clear_transforms();
#ifdef ARCH_ARM64
			{
				auto should_exclude_function = [](const std::string& fn_name)
				{
					return fn_name.starts_with("spu_") || fn_name.starts_with("tr_");
				};

				aarch64::GHC_frame_preservation_pass::config_t config =
				{
					.debug_info = false,         // Set to "true" to insert debug frames on x27
					.use_stack_frames = false,   // We don't need this since the SPU GW allocates global scratch on the stack
					.hypervisor_context_offset = ::offset32(&spu_thread::hv_ctx),
					.exclusion_callback = should_exclude_function,
					.base_register_lookup = {}   // Unused, always x19 on SPU
				};

				// Create transform pass
				std::unique_ptr<translator_pass> ghc_fixup_pass = std::make_unique<aarch64::GHC_frame_preservation_pass>(config);

				// Register it
				register_transform_pass(ghc_fixup_pass);
			}
#endif
		}

		reset_transforms();
	}

	void init_luts()
	{
		// LUTs for some instructions
		m_spu_frest_fraction_lut = new llvm::GlobalVariable(*m_module, llvm::ArrayType::get(GetType<u32>(), 32), true, llvm::GlobalValue::PrivateLinkage, llvm::ConstantDataArray::get(m_context, spu_frest_fraction_lut));
		m_spu_frsqest_fraction_lut = new llvm::GlobalVariable(*m_module, llvm::ArrayType::get(GetType<u32>(), 64), true, llvm::GlobalValue::PrivateLinkage, llvm::ConstantDataArray::get(m_context, spu_frsqest_fraction_lut));
		m_spu_frsqest_exponent_lut = new llvm::GlobalVariable(*m_module, llvm::ArrayType::get(GetType<u32>(), 256), true, llvm::GlobalValue::PrivateLinkage, llvm::ConstantDataArray::get(m_context, spu_frsqest_exponent_lut));
	}

	virtual spu_function_t compile(spu_program&& _func) override
	{
		if (_func.data.empty() && m_interp_magn)
		{
			return compile_interpreter();
		}

		const u32 start0 = _func.entry_point;
		const usz func_size = _func.data.size();

		const auto add_loc = m_spurt->add_empty(std::move(_func));

		if (!add_loc)
		{
			return nullptr;
		}

		const spu_program& func = add_loc->data;

		if (func.entry_point != start0)
		{
			// Wait for the duplicate
			while (!add_loc->compiled)
			{
				add_loc->compiled.wait(nullptr);
			}

			return add_loc->compiled;
		}

		bool add_to_file = false;

		if (auto& cache = g_fxo->get<spu_cache>(); cache && g_cfg.core.spu_cache && !add_loc->cached.exchange(1))
		{
			add_to_file = true;
		}

		{
			sha1_context ctx;
			u8 output[20];

			sha1_starts(&ctx);
			sha1_update(&ctx, reinterpret_cast<const u8*>(func.data.data()), func.data.size() * 4);
			sha1_finish(&ctx, output);

			m_hash.clear();
			fmt::append(m_hash, "__spu-0x%05x-%s", func.entry_point, fmt::base57(output));

			be_t<u64> hash_start;
			std::memcpy(&hash_start, output, sizeof(hash_start));
			m_hash_start = hash_start;
		}

		spu_log.notice("Building function 0x%x... (size %u, %s)", func.entry_point, func.data.size(), m_hash);

		m_pos = func.lower_bound;
		m_base = func.entry_point;
		m_size = ::size32(func.data) * 4;
		const u32 start = m_pos;
		const u32 end = start + m_size;

		m_pp_id = 0;

		std::string function_log;

		this->dump(func, function_log);
		bool to_log_func = false;

		if (g_cfg.core.spu_debug && !add_loc->logged.exchange(1))
		{
			if (!fs::write_file(m_spurt->get_cache_path() + "spu.log", fs::write + fs::append, function_log))
			{
				// Fallback: write to main log
				to_log_func = true;
			}
		}

		for (u32 data : func.data)
		{
			const spu_opcode_t op{std::bit_cast<be_t<u32>>(data)};

			const auto itype = g_spu_itype.decode(op.opcode);

			if (itype == spu_itype::RDCH && op.ra == SPU_RdDec)
			{
				to_log_func = true;
			}
		}

		if (to_log_func)
		{
			spu_log.notice("Function %s dump:\n%s", m_hash, function_log);
		}

		using namespace llvm;

		m_engine->clearAllGlobalMappings();

		// Create LLVM module
		std::unique_ptr<Module> _module = std::make_unique<Module>(m_hash + ".obj", m_context);
#if LLVM_VERSION_MAJOR >= 21 && (LLVM_VERSION_MINOR >= 1 || LLVM_VERSION_MAJOR >= 22)
		_module->setTargetTriple(Triple(jit_compiler::triple2()));
#else
		_module->setTargetTriple(jit_compiler::triple2());
#endif
		_module->setDataLayout(m_jit.get_engine().getTargetMachine()->createDataLayout());
		m_module = _module.get();

		// Initialize IR Builder
		IRBuilder<> irb(m_context);
		m_ir = &irb;

		// Add entry function (contains only state/code check)
		const auto main_func = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(m_hash, get_ftype<void, u8*, u8*, u64>()).getCallee());
		const auto main_arg2 = main_func->getArg(2);
		main_func->setCallingConv(CallingConv::GHC);
		set_function(main_func);

		init_luts();

		// Start compilation
		const auto label_test = BasicBlock::Create(m_context, "", m_function);
		const auto label_diff = BasicBlock::Create(m_context, "", m_function);
		const auto label_body = BasicBlock::Create(m_context, "", m_function);
		const auto label_stop = BasicBlock::Create(m_context, "", m_function);

		// Load PC, which will be the actual value of 'm_base'
		m_base_pc = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::pc));

		// Emit state check
		const auto pstate = spu_ptr(&spu_thread::state);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(m_ir->CreateLoad(get_type<u32>(), pstate), m_ir->getInt32(0)), label_stop, label_test, m_md_unlikely);

		// Emit code check
		u32 check_iterations = 0;
		m_ir->SetInsertPoint(label_test);

		// Set block hash for profiling (if enabled)
		if ((g_cfg.core.spu_prof || g_cfg.core.spu_debug) && g_cfg.core.spu_verification)
			m_ir->CreateStore(m_ir->getInt64((m_hash_start & -65536)), spu_ptr(&spu_thread::block_hash));

		if (!g_cfg.core.spu_verification)
		{
			// Disable check (unsafe)
			m_ir->CreateBr(label_body);
		}
		else if (func.data.size() == 1)
		{
			const auto pu32 = _ptr(m_lsptr, m_base_pc);
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(get_type<u32>(), pu32), m_ir->getInt32(func.data[0]));
			m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
		}
		else if (func.data.size() == 2)
		{
			const auto pu64 = _ptr(m_lsptr, m_base_pc);
			const auto cond = m_ir->CreateICmpNE(m_ir->CreateLoad(get_type<u64>(), pu64), m_ir->getInt64(static_cast<u64>(func.data[1]) << 32 | func.data[0]));
			m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
		}
		else
		{
			u32 starta = start;

			// Skip holes at the beginning (giga only)
			for (u32 j = start; j < end; j += 4)
			{
				if (!func.data[(j - start) / 4])
				{
					starta += 4;
				}
				else
				{
					break;
				}
			}

			u32 stride;
			u32 elements;
			u32 dwords;

			if (m_use_avx512)
			{
				stride = 64;
				elements = 16;
				dwords = 8;
			}
			else if (m_use_avx)
			{
				stride = 32;
				elements = 8;
				dwords = 4;
			}
			else
			{
				stride = 16;
				elements = 4;
				dwords = 2;
			}

			// Get actual pc corresponding to the found beginning of the data
			llvm::Value* starta_pc = m_ir->CreateAnd(get_pc(starta), 0x3fffc);
			llvm::Value* data_addr = _ptr(m_lsptr, starta_pc);

			llvm::Value* acc0 = nullptr;
			llvm::Value* acc1 = nullptr;
			bool toggle = true;

			// Use a 512bit simple checksum to verify integrity if size is atleast 512b * 3
			// This code uses a 512bit vector for all hardware to ensure behavior matches.
			// The checksum path is still faster even on narrow hardware.
			if ((end - starta) >= 192 && !g_cfg.core.precise_spu_verification)
			{
				for (u32 j = starta; j < end; j += 64)
				{
					int indices[16];
					bool holes = false;
					bool data = false;

					for (u32 i = 0; i < 16; i++)
					{
						const u32 k = j + i * 4;

						if (k < start || k >= end || !func.data[(k - start) / 4])
						{
							indices[i] = 16;
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
						// Skip full-sized holes
						continue;
					}

					llvm::Value* vls = nullptr;

					// Load unaligned code block from LS
					vls = m_ir->CreateAlignedLoad(get_type<u32[16]>(), _ptr(data_addr, j - starta), llvm::MaybeAlign{4});

					// Mask if necessary
					if (holes)
					{
						vls = m_ir->CreateShuffleVector(vls, ConstantAggregateZero::get(vls->getType()), llvm::ArrayRef(indices, 16));
					}

					// Interleave accumulators for more performance
					if (toggle)
					{
						acc0 = acc0 ? m_ir->CreateAdd(acc0, vls) : vls;
					}
					else
					{
						acc1 = acc1 ? m_ir->CreateAdd(acc1, vls) : vls;
					}
					toggle = !toggle;
					check_iterations++;
				}

				llvm::Value* acc = (acc0 && acc1) ? m_ir->CreateAdd(acc0, acc1): (acc0 ? acc0 : acc1);

				// Create the checksum
				u32 checksum[16] = {0};

				for (u32 j = 0; j < func.data.size(); j += 16) // Process 16 elements per iteration
				{
					for (u32 i = 0; i < 16; i++)
					{
						if (j + i < func.data.size())
						{
							checksum[i] += func.data[j + i];
						}
					}
				}

				auto* const_vector = ConstantDataVector::get(m_context, llvm::ArrayRef(checksum, 16));
				acc = m_ir->CreateXor(acc, const_vector);

				// Pattern for PTEST
				acc = m_ir->CreateBitCast(acc, get_type<u64[8]>());

				llvm::Value* elem = m_ir->CreateExtractElement(acc, u64{0});

				for (u32 i = 1; i < 8; i++)
				{
					elem = m_ir->CreateOr(elem, m_ir->CreateExtractElement(acc, i));
				}

				// Compare result with zero
				const auto cond = m_ir->CreateICmpNE(elem, m_ir->getInt64(0));
				m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
			}
			else
			{
				for (u32 j = starta; j < end; j += stride)
				{
					int indices[16];
					bool holes = false;
					bool data = false;

					for (u32 i = 0; i < elements; i++)
					{
						const u32 k = j + i * 4;

						if (k < start || k >= end || !func.data[(k - start) / 4])
						{
							indices[i] = elements;
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
						// Skip full-sized holes
						continue;
					}

					llvm::Value* vls = nullptr;

					// Load unaligned code block from LS
					if (m_use_avx512)
					{
						vls = m_ir->CreateAlignedLoad(get_type<u32[16]>(), _ptr(data_addr, j - starta), llvm::MaybeAlign{4});
					}
					else if (m_use_avx)
					{
						vls = m_ir->CreateAlignedLoad(get_type<u32[8]>(), _ptr(data_addr, j - starta), llvm::MaybeAlign{4});
					}
					else
					{
						vls = m_ir->CreateAlignedLoad(get_type<u32[4]>(), _ptr(data_addr, j - starta), llvm::MaybeAlign{4});
					}

					// Mask if necessary
					if (holes)
					{
						vls = m_ir->CreateShuffleVector(vls, ConstantAggregateZero::get(vls->getType()), llvm::ArrayRef(indices, elements));
					}

					// Perform bitwise comparison and accumulate
					u32 words[16];

					for (u32 i = 0; i < elements; i++)
					{
						const u32 k = j + i * 4;
						words[i] = k >= start && k < end ? func.data[(k - start) / 4] : 0;
					}

					vls = m_ir->CreateXor(vls, ConstantDataVector::get(m_context, llvm::ArrayRef(words, elements)));

					// Interleave accumulators for more performance
					if (toggle)
					{
						acc0 = acc0 ? m_ir->CreateAdd(acc0, vls) : vls;
					}
					else
					{
						acc1 = acc1 ? m_ir->CreateAdd(acc1, vls) : vls;
					}
					toggle = !toggle;
					check_iterations++;
				}
				llvm::Value* acc = (acc0 && acc1) ? m_ir->CreateAdd(acc0, acc1): (acc0 ? acc0 : acc1);

				// Pattern for PTEST
				if (m_use_avx512)
				{
					acc = m_ir->CreateBitCast(acc, get_type<u64[8]>());
				}
				else if (m_use_avx)
				{
					acc = m_ir->CreateBitCast(acc, get_type<u64[4]>());
				}
				else
				{
					acc = m_ir->CreateBitCast(acc, get_type<u64[2]>());
				}

				llvm::Value* elem = m_ir->CreateExtractElement(acc, u64{0});

				for (u32 i = 1; i < dwords; i++)
				{
					elem = m_ir->CreateOr(elem, m_ir->CreateExtractElement(acc, i));
				}

				// Compare result with zero
				const auto cond = m_ir->CreateICmpNE(elem, m_ir->getInt64(0));
				m_ir->CreateCondBr(cond, label_diff, label_body, m_md_unlikely);
			}
		}

		// Increase block counter with statistics
		m_ir->SetInsertPoint(label_body);
		const auto pbcount = spu_ptr(&spu_thread::block_counter);
		m_ir->CreateStore(m_ir->CreateAdd(m_ir->CreateLoad(get_type<u64>(), pbcount), m_ir->getInt64(check_iterations)), pbcount);

		// Call the entry function chunk
		const auto entry_chunk = add_function(m_pos);
		const auto entry_call = m_ir->CreateCall(entry_chunk->chunk, {m_thread, m_lsptr, m_base_pc});
		entry_call->setCallingConv(entry_chunk->chunk->getCallingConv());

		const auto dispatcher = llvm::cast<llvm::Function>(m_module->getOrInsertFunction("spu_dispatcher", main_func->getType()).getCallee());
		m_engine->updateGlobalMapping("spu_dispatcher", reinterpret_cast<u64>(spu_runtime::tr_all));
		dispatcher->setCallingConv(main_func->getCallingConv());

		// Proceed to the next code
		if (entry_chunk->chunk->getReturnType() != get_type<void>())
		{
			const auto next_call = m_ir->CreateCall(main_func->getFunctionType(), entry_call, {m_thread, m_lsptr, m_ir->getInt64(0)});
			next_call->setCallingConv(main_func->getCallingConv());
			next_call->setTailCall();
		}
		else
		{
			entry_call->setTailCall();
		}

		m_ir->CreateRetVoid();

		m_ir->SetInsertPoint(label_stop);
		call("spu_escape", spu_runtime::g_escape, m_thread)->setTailCall();
		m_ir->CreateRetVoid();

		m_ir->SetInsertPoint(label_diff);

		if (g_cfg.core.spu_verification)
		{
			const auto pbfail = spu_ptr(&spu_thread::block_failure);
			m_ir->CreateStore(m_ir->CreateAdd(m_ir->CreateLoad(get_type<u64>(), pbfail), m_ir->getInt64(1)), pbfail);
			const auto dispci = call("spu_dispatch", spu_runtime::tr_dispatch, m_thread, m_lsptr, main_arg2);
			dispci->setCallingConv(CallingConv::GHC);
			dispci->setTailCall();
			m_ir->CreateRetVoid();
		}
		else
		{
			m_ir->CreateUnreachable();
		}

		m_dispatch = cast<Function>(_module->getOrInsertFunction("__spu-null", entry_chunk->chunk->getFunctionType()).getCallee());
		m_dispatch->setLinkage(llvm::GlobalValue::InternalLinkage);
		m_dispatch->setCallingConv(entry_chunk->chunk->getCallingConv());
		set_function(m_dispatch);

		if (entry_chunk->chunk->getReturnType() == get_type<void>())
		{
			const auto next_call = m_ir->CreateCall(main_func->getFunctionType(), dispatcher, {m_thread, m_lsptr, m_ir->getInt64(0)});
			next_call->setCallingConv(main_func->getCallingConv());
			next_call->setTailCall();
			m_ir->CreateRetVoid();
		}
		else
		{
			m_ir->CreateRet(dispatcher);
		}

		// Function that executes check_state and escapes if necessary
		m_test_state = llvm::cast<llvm::Function>(m_module->getOrInsertFunction("spu_test_state", get_ftype<void, u8*>()).getCallee());
		m_test_state->setLinkage(GlobalValue::InternalLinkage);
#ifdef ARCH_ARM64
		// LLVM doesn't support PreserveAll on arm64.
		m_test_state->setCallingConv(CallingConv::PreserveMost);
#else
		m_test_state->setCallingConv(CallingConv::PreserveAll);
#endif
		m_ir->SetInsertPoint(BasicBlock::Create(m_context, "", m_test_state));
		const auto escape_yes = BasicBlock::Create(m_context, "", m_test_state);
		const auto escape_no = BasicBlock::Create(m_context, "", m_test_state);
		m_ir->CreateCondBr(call("spu_exec_check_state", &exec_check_state, m_test_state->getArg(0)), escape_yes, escape_no);
		m_ir->SetInsertPoint(escape_yes);
		call("spu_escape", spu_runtime::g_escape, m_test_state->getArg(0));
		m_ir->CreateRetVoid();
		m_ir->SetInsertPoint(escape_no);
		m_ir->CreateRetVoid();

		// Create function table (uninitialized)
		m_function_table = new llvm::GlobalVariable(*m_module, llvm::ArrayType::get(entry_chunk->chunk->getType(), m_size / 4), true, llvm::GlobalValue::InternalLinkage, nullptr);

		// Create function chunks
		for (usz fi = 0; fi < m_function_queue.size(); fi++)
		{
			// Initialize function info
			m_entry = m_function_queue[fi];
			set_function(m_functions[m_entry].chunk);

			// Set block hash for profiling (if enabled)
			if (g_cfg.core.spu_prof || g_cfg.core.spu_debug)
				m_ir->CreateStore(m_ir->getInt64((m_hash_start & -65536) | (m_entry >> 2)), spu_ptr(&spu_thread::block_hash));

			m_finfo = &m_functions[m_entry];
			m_ir->CreateBr(add_block(m_entry));

			// Emit instructions for basic blocks
			for (usz bi = 0; bi < m_block_queue.size(); bi++)
			{
				// Initialize basic block info
				const u32 baddr = m_block_queue[bi];
				m_block = &m_blocks[baddr];
				m_ir->SetInsertPoint(m_block->block);
				auto& bb = ::at32(m_bbs, baddr);
				bool need_check = false;
				m_block->bb = &bb;

				if (!bb.preds.empty())
				{
					// Initialize registers and build PHI nodes if necessary
					for (u32 i = 0; i < s_reg_max; i++)
					{
						const u32 src = m_finfo->fn ? bb.reg_origin_abs[i] : bb.reg_origin[i];

						if (src > 0x40000)
						{
							// Use the xfloat hint to create 256-bit (4x double) PHI
							llvm::Type* type = g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate && bb.reg_maybe_xf[i] ? get_type<f64[4]>() : get_reg_type(i);

							const auto _phi = m_ir->CreatePHI(type, ::size32(bb.preds), fmt::format("phi0x%05x_r%u", baddr, i));
							m_block->phi[i] = _phi;
							m_block->reg[i] = _phi;

							for (u32 pred : bb.preds)
							{
								const auto bfound = m_blocks.find(pred);

								if (bfound != m_blocks.end() && bfound->second.block_end)
								{
									auto& value = bfound->second.reg[i];

									if (!value || value->getType() != _phi->getType())
									{
										const auto regptr = init_reg_fixed(i);
										const auto cblock = m_ir->GetInsertBlock();
										m_ir->SetInsertPoint(bfound->second.block_end->getTerminator());

										if (!value)
										{
											// Value hasn't been loaded yet
											value = m_finfo && m_finfo->load[i] ? m_finfo->load[i] : m_ir->CreateLoad(get_reg_type(i), regptr);
										}

										if (value->getType() == get_type<f64[4]>() && type != get_type<f64[4]>())
										{
											value = double_to_xfloat(value);
										}
										else if (value->getType() != get_type<f64[4]>() && type == get_type<f64[4]>())
										{
											value = xfloat_to_double(bitcast<u32[4]>(value));
										}
										else
										{
											value = bitcast(value, _phi->getType());
										}

										m_ir->SetInsertPoint(cblock);

										ensure(bfound->second.block_end->getTerminator());
									}

									_phi->addIncoming(value, bfound->second.block_end);
								}
							}

							if (baddr == m_entry)
							{
								// Load value at the function chunk's entry block if necessary
								const auto regptr = init_reg_fixed(i);
								const auto cblock = m_ir->GetInsertBlock();
								m_ir->SetInsertPoint(m_function->getEntryBlock().getTerminator());
								const auto value = m_finfo && m_finfo->load[i] ? m_finfo->load[i] : m_ir->CreateLoad(get_reg_type(i), regptr);
								m_ir->SetInsertPoint(cblock);
								_phi->addIncoming(value, &m_function->getEntryBlock());
							}
						}
						else if (src < SPU_LS_SIZE)
						{
							// Passthrough register value
							const auto bfound = m_blocks.find(src);

							if (bfound != m_blocks.end())
							{
								m_block->reg[i] = bfound->second.reg[i];
							}
							else
							{
								spu_log.error("[0x%05x] Value not found ($%u from 0x%05x)", baddr, i, src);
							}
						}
						else
						{
							m_block->reg[i] = m_finfo->load[i];
						}
					}

					// Emit state check if necessary (TODO: more conditions)
					for (u32 pred : bb.preds)
					{
						if (pred >= baddr)
						{
							// If this block is a target of a backward branch (possibly loop), emit a check
							need_check = true;
							break;
						}
					}
				}

				// State check at the beginning of the chunk
				if (need_check || (bi == 0 && g_cfg.core.spu_block_size != spu_block_size_type::safe))
				{
					check_state(baddr);
				}

				// Emit instructions
				for (m_pos = baddr; m_pos >= start && m_pos < end && !m_ir->GetInsertBlock()->getTerminator(); m_pos += 4)
				{
					if (m_pos != baddr && m_block_info[m_pos / 4])
					{
						break;
					}

					const u32 op = std::bit_cast<be_t<u32>>(func.data[(m_pos - start) / 4]);

					if (!op)
					{
						spu_log.error("[%s] Unexpected fallthrough to 0x%x (chunk=0x%x, entry=0x%x)", m_hash, m_pos, m_entry, m_function_queue[0]);
						break;
					}

					// Set variable for set_link()
					if (m_pos + 4 >= end)
						m_next_op = 0;
					else
						m_next_op = func.data[(m_pos - start) / 4 + 1];

					switch (m_inst_attrs[(m_pos - start) / 4])
					{
					case inst_attr::putllc0:
					{
						putllc0_pattern(func, m_patterns.at(m_pos - start).range);
						continue;
					}
					case inst_attr::putllc16:
					{
						putllc16_pattern(func, m_patterns.at(m_pos - start).range);
						continue;
					}
					case inst_attr::omit:
					{
						// TODO
						continue;
					}
					default: break;
					}

					// Execute recompiler function (TODO)
					(this->*decode(op))({op});
				}

				// Finalize block with fallthrough if necessary
				if (!m_ir->GetInsertBlock()->getTerminator())
				{
					const u32 target = m_pos == baddr ? baddr : m_pos & 0x3fffc;

					if (m_pos != baddr)
					{
						m_pos -= 4;

						if (target >= start && target < end)
						{
							const auto tfound = m_targets.find(m_pos);

							if (tfound == m_targets.end() || std::find(tfound->second.begin(), tfound->second.end(), target) == tfound->second.end())
							{
								spu_log.error("[%s] Unregistered fallthrough to 0x%x (chunk=0x%x, entry=0x%x)", m_hash, target, m_entry, m_function_queue[0]);
							}
						}
					}

					m_block->block_end = m_ir->GetInsertBlock();
					m_ir->CreateBr(add_block(target));
				}

				ensure(m_block->block_end);
			}

			// Work on register stores.
			// 1. Remove stores which are overwritten later.
			// 2. Sink stores to post-dominating blocks.
			llvm::PostDominatorTree pdt(*m_function);
			llvm::DominatorTree dt(*m_function);

			// Post-order indices
			std::unordered_map<llvm::BasicBlock*, usz> pois;
			{
				usz i = 0;
				for (auto* bb : llvm::post_order(m_function))
					pois[bb] = i++;
			}

			// Basic block to block_info
			std::unordered_map<llvm::BasicBlock*, block_info*> bb_to_info;

			std::vector<std::pair<u32, block_info*>> block_q;
			block_q.reserve(m_blocks.size());

			bool has_gpr_memory_barriers = false;

			for (auto& [a, b] : m_blocks)
			{
				block_q.emplace_back(a, &b);
				bb_to_info[b.block] = &b;
				has_gpr_memory_barriers |= b.has_gpr_memory_barriers;
			}

			for (usz bi = 0; bi < block_q.size(); bi++)
			{
				auto bqbi = block_q[bi].second;

				// TODO: process all registers up to s_reg_max
				for (u32 i = 0; i <= s_reg_127; i++)
				{
					// Check if the store is beyond the last barrier
					if (auto& bs = bqbi->store[i]; bs && !bqbi->does_gpr_barrier_proceed_last_store(i))
					{
						for (auto& [a, b] : m_blocks)
						{
							if (has_gpr_memory_barriers)
							{
								// Dive deeper and inspect GPR store barriers
								break;
							}

							// Check if the store occurs before any barrier in the block
							if (b.store[i] && b.store[i] != bs && b.store_context_first_id[i] == 1)
							{
								if (pdt.dominates(b.store[i], bs))
								{
									spu_log.trace("Erased r%u store from block 0x%x (simple)", i, block_q[bi].first);

									bs->eraseFromParent();
									bs = nullptr;
									break;
								}
							}
						}

						if (!bs)
							continue;

						// Set of store instructions which overwrite bs
						std::vector<llvm::BasicBlock*> killers;

						for (auto& [a, b] : m_blocks)
						{
							const auto si = b.store[i];

							if (si && si != bs)
							{
								if (pois[bs->getParent()] > pois[si->getParent()])
								{
									killers.emplace_back(si->getParent());
								}
								else
								{
									// Reset: store is not the first in the set
									killers.clear();
									break;
								}
							}
						}

						if (killers.empty())
							continue;

						// Find nearest common post-dominator
						llvm::BasicBlock* common_pdom = killers[0];

						if (has_gpr_memory_barriers)
						{
							// Cannot optimize block walk-through, need to inspect all possible memory barriers in the way
							common_pdom = nullptr;
						}

						for (auto* bbb : llvm::drop_begin(killers))
						{
							if (!common_pdom)
							{
								break;
							}

							common_pdom = pdt.findNearestCommonDominator(common_pdom, bbb);
						}

						// Shortcut
						if (common_pdom && !pdt.dominates(common_pdom, bs->getParent()))
						{
							common_pdom = nullptr;
						}

						// Look for possibly-dead store in CFG starting from the exit nodes
						llvm::SetVector<llvm::BasicBlock*> work_list;
						std::unordered_map<llvm::BasicBlock*, bool> worked_on;

						if (!common_pdom || std::none_of(killers.begin(), killers.end(), [common_pdom](const llvm::BasicBlock* block){ return block == common_pdom;}))
						{
							if (common_pdom)
							{
								// Shortcut
								work_list.insert(common_pdom);
								worked_on[common_pdom] = true;
							}
							else
							{
								// Check all exits
								for (auto* r : pdt.roots())
								{
									worked_on[r] = true;
									work_list.insert(r);
								}
							}
						}

						// bool flag indicates the presence of a memory barrier before the killer store
						std::vector<std::pair<llvm::BasicBlock*, bool>> work2_list;

						for (usz wi = 0; wi < work_list.size(); wi++)
						{
							auto* cur = work_list[wi];
							if (std::any_of(killers.begin(), killers.end(), [cur](const llvm::BasicBlock* block){ return block == cur; }))
							{
								work2_list.emplace_back(cur, bb_to_info[cur] && bb_to_info[cur]->does_gpr_barrier_preceed_first_store(i));
								continue;
							}

							if (cur == bs->getParent())
							{
								// Reset: store is not dead
								killers.clear();
								break;
							}

							for (auto* p : llvm::predecessors(cur))
							{
								if (!worked_on[p])
								{
									worked_on[p] = true;
									work_list.insert(p);
								}
							}
						}

						if (killers.empty())
							continue;

						worked_on.clear();

						for (usz wi = 0; wi < work2_list.size(); wi++)
						{
							worked_on[work2_list[wi].first] = true;
						}

						// Need to treat tails differently: do not require checking barrier (checked before in a suitable manner)
						const usz work_list_tail_blocks_max_index = work2_list.size();

						for (usz wi = 0; wi < work2_list.size(); wi++)
						{
							auto [cur, found_user] = work2_list[wi];

							ensure(cur != bs->getParent());

							if (!found_user && wi >= work_list_tail_blocks_max_index)
							{
								if (auto info = bb_to_info[cur])
								{
									if (info->store_context_ctr[i] != 1)
									{
										found_user = true;
									}
								}
							}

							for (auto* p : llvm::predecessors(cur))
							{
								if (p == bs->getParent())
								{
									if (found_user)
									{
										// Reset: store is being used and preserved by ensure_gpr_stores()
										killers.clear();
										break;
									}

									continue;
								}

								if (!worked_on[p])
								{
									worked_on[p] = true;
									work2_list.push_back(std::make_pair(p, found_user));
								}
								// Enqueue a second iteration for found_user=true if only found with found_user=false
								else if (found_user && !std::find_if(work2_list.rbegin(), work2_list.rend(), [&](auto& it){ return it.first == p; })->second)
								{
									work2_list.push_back(std::make_pair(p, true));
								}
							}

							if (killers.empty())
							{
								break;
							}
						}

						// Finally erase the dead store
						if (!killers.empty())
						{
							spu_log.trace("Erased r%u store from block 0x%x (reversed)", i, block_q[bi].first);

							bs->eraseFromParent();
							bs = nullptr;

							// Run the loop from the start
							bi = 0;
						}
					}
				}
			}

			block_q.clear();
			for (auto& [a, b] : m_blocks)
			{
				block_q.emplace_back(a, &b);
			}

			for (usz bi = 0; bi < block_q.size(); bi++)
			{
				auto bqbi = block_q[bi].second;

				std::vector<std::pair<u32, bool>> work_list;
				std::map<u32, block_info*, std::greater<>> sucs;
				std::unordered_map<u32, bool> worked_on;

				for (u32 i = 0; i <= s_reg_127; i++)
				{
					if (i == s_reg_sp)
					{
						// If we postpone R1 store we lose effortless meta-analytical capabilities for little gain
						continue;
					}

					// If store isn't erased, try to sink it
					if (auto& bs = bqbi->store[i]; bs && bqbi->bb->targets.size() > 1 && !bqbi->does_gpr_barrier_proceed_last_store(i))
					{
						if (sucs.empty())
						{
							for (u32 tj : bqbi->bb->targets)
							{
								auto b2it = m_blocks.find(tj);

								if (b2it != m_blocks.end())
								{
									sucs.emplace(tj, &b2it->second);
								}
							}
						}

						// Reset
						work_list.clear();

						for (auto& [_, worked] : worked_on)
						{
							worked = false;
						}

						bool has_gpr_barriers_in_the_way = false;

						for (auto [a2, b2] : sucs)
						{
							if (a2 == block_q[bi].first)
							{
								if (bqbi->store_context_ctr[i] != 1)
								{
									has_gpr_barriers_in_the_way = true;
									break;
								}

								continue;
							}

							if (!worked_on[a2])
							{
								work_list.emplace_back(a2, b2->store_context_ctr[i] != 1);
								worked_on[a2] = true;
							}
						}

						if (has_gpr_barriers_in_the_way)
						{
							// Cannot sink store, has barriers in the way
							continue;
						}

						for (usz wi = 0; wi < work_list.size(); wi++)
						{
							auto [cur, found_barrier] = work_list[wi];

							if (!found_barrier)
							{
								if (const auto it = m_blocks.find(cur); it != m_blocks.cend())
								{
									if (it->second.store_context_ctr[i] != 1)
									{
										found_barrier = true;
									}
								}
							}

							if (cur == block_q[bi].first)
							{
								if (found_barrier)
								{
									has_gpr_barriers_in_the_way = true;
									break;
								}

								continue;
							}

							for (u32 target : m_bbs[cur].targets)
							{
								if (!m_block_info[target / 4])
								{
									continue;
								}

								if (m_blocks.find(target) == m_blocks.end())
								{
									continue;
								}

								if (!worked_on[target])
								{
									worked_on[target] = true;
									work_list.emplace_back(target, found_barrier);
								}
								// Enqueue a second iteration for found_barrier=true if only found with found_barrier=false
								else if (found_barrier && !std::find_if(work_list.rbegin(), work_list.rend(), [&](auto& it){ return it.first == target; })->second)
								{
									work_list.emplace_back(target, true);
								}
							}
						}

						if (has_gpr_barriers_in_the_way)
						{
							// Cannot sink store, has barriers in the way
							continue;
						}

						for (auto [a2, b2] : sucs)
						{
							if (b2 != bqbi)
							{
								auto ins = b2->block->getFirstNonPHI();

								if (b2->bb->preds.size() == 1)
								{
									if (!dt.dominates(bs->getOperand(0), ins))
										continue;
									if (!pdt.dominates(ins, bs))
										continue;

									m_ir->SetInsertPoint(ins);
									auto si = llvm::cast<StoreInst>(m_ir->Insert(bs->clone()));
									if (b2->store[i] == nullptr)
									{
										// Protect against backwards ordering now
										b2->store[i] = si;
										b2->store_context_last_id[i] = 0;
										b2->store_context_first_id[i] = b2->store_context_ctr[i] + 1;

										if (std::none_of(block_q.begin() + bi, block_q.end(), [b_info = b2](auto&& a) { return a.second == b_info; }))
										{
											// Sunk store can be checked again
											block_q.emplace_back(a2, b2);
										}
									}

									spu_log.trace("Postponed r%u store from block 0x%x (single)", i, block_q[bi].first);
								}
								else
								{
									// Initialize additional block between two basic blocks
									auto& edge = bqbi->block_edges[a2];
									if (!edge)
									{
										const auto succ_range = llvm::successors(bqbi->block_end);

										auto succ = b2->block;

										llvm::SmallSetVector<llvm::BasicBlock*, 32> succ_q;
										succ_q.insert(b2->block);

										for (usz j = 0; j < 32 && j < succ_q.size(); j++)
										{
											if (!llvm::count(succ_range, (succ = succ_q[j])))
											{
												for (auto pred : llvm::predecessors(succ))
												{
													succ_q.insert(pred);
												}
											}
											else
											{
												break;
											}
										}

										if (!llvm::count(succ_range, succ))
										{
											// TODO: figure this out
											spu_log.notice("[%s] Failed successor to 0x%05x", fmt::base57(be_t<u64>{m_hash_start}), a2);
											continue;
										}

										edge = llvm::SplitEdge(bqbi->block_end, succ);
										pdt.recalculate(*m_function);
										dt.recalculate(*m_function);

										spu_log.trace("Postponed r%u store from block 0x%x (multiple)", i, block_q[bi].first);
									}

									ins = edge->getTerminator();
									if (!dt.dominates(bs->getOperand(0), ins))
										continue;
									if (!pdt.dominates(ins, bs))
										continue;

									m_ir->SetInsertPoint(ins);
									m_ir->Insert(bs->clone());
								}

								bs->eraseFromParent();
								bs = nullptr;

								pdt.recalculate(*m_function);
								dt.recalculate(*m_function);
								break;
							}
						}
					}
				}
			}
		}

		// Create function table if necessary
		if (m_function_table->getNumUses())
		{
			std::vector<llvm::Constant*> chunks;
			chunks.reserve(m_size / 4);

			for (u32 i = start; i < end; i += 4)
			{
				const auto found = m_functions.find(i);

				if (found == m_functions.end())
				{
					if (false && g_cfg.core.spu_verification)
					{
						const std::string ppname = fmt::format("%s-chunkpp-0x%05x", m_hash, i);
						m_engine->updateGlobalMapping(ppname, reinterpret_cast<u64>(m_spurt->make_branch_patchpoint(i / 4)));

						const auto ppfunc = llvm::cast<llvm::Function>(m_module->getOrInsertFunction(ppname, m_finfo->chunk->getFunctionType()).getCallee());
						ppfunc->setCallingConv(m_finfo->chunk->getCallingConv());

						chunks.push_back(ppfunc);
						continue;
					}

					chunks.push_back(m_dispatch);
					continue;
				}

				chunks.push_back(found->second.chunk);
			}

			m_function_table->setInitializer(llvm::ConstantArray::get(llvm::ArrayType::get(entry_chunk->chunk->getType(), m_size / 4), chunks));
		}
		else
		{
			m_function_table->eraseFromParent();
		}

		// Create the analysis managers.
		// These must be declared in this order so that they are destroyed in the
		// correct order due to inter-analysis-manager references.
		LoopAnalysisManager lam;
		FunctionAnalysisManager fam;
		CGSCCAnalysisManager cgam;
		ModuleAnalysisManager mam;

		// Create the new pass manager builder.
		// Take a look at the PassBuilder constructor parameters for more
		// customization, e.g. specifying a TargetMachine or various debugging
		// options.
		PassBuilder pb;

		// Register all the basic analyses with the managers.
		pb.registerModuleAnalyses(mam);
		pb.registerCGSCCAnalyses(cgam);
		pb.registerFunctionAnalyses(fam);
		pb.registerLoopAnalyses(lam);
		pb.crossRegisterProxies(lam, fam, cgam, mam);

		FunctionPassManager fpm;
		// Basic optimizations
		fpm.addPass(EarlyCSEPass(true));
		fpm.addPass(SimplifyCFGPass());
		fpm.addPass(DSEPass());
		fpm.addPass(createFunctionToLoopPassAdaptor(LICMPass(LICMOptions()), true));
		fpm.addPass(ADCEPass());

		for (auto& f : *m_module)
		{
			run_transforms(f);
		}

		for (const auto& func : m_functions)
		{
			const auto f = func.second.fn ? func.second.fn : func.second.chunk;
			fpm.run(*f, fam);
		}

		// Clear context (TODO)
		m_blocks.clear();
		m_block_queue.clear();
		m_functions.clear();
		m_function_queue.clear();
		m_function_table = nullptr;

		// Append for now
		std::string& llvm_log = function_log;
		raw_string_ostream out(llvm_log);

		if (g_cfg.core.spu_debug)
		{
			fmt::append(llvm_log, "LLVM IR at 0x%x:\n", func.entry_point);
			out << *_module; // print IR
			out << "\n\n";
		}

		if (verifyModule(*_module, &out))
		{
			out.flush();
			spu_log.error("LLVM: Verification failed at 0x%x:\n%s", func.entry_point, llvm_log);

			if (g_cfg.core.spu_debug)
			{
				fs::write_file(m_spurt->get_cache_path() + "spu-ir.log", fs::write + fs::append, llvm_log);
			}

			if (auto& cache = g_fxo->get<spu_cache>())
			{
				if (add_to_file)
				{
					cache.add(func);
				}
			}

			fmt::throw_exception("Compilation failed");
		}

#if defined(__APPLE__)
		pthread_jit_write_protect_np(false);
#endif

		if (g_cfg.core.spu_debug)
		{
			// Testing only
			m_jit.add(std::move(_module), m_spurt->get_cache_path() + "llvm/");
		}
		else
		{
			m_jit.add(std::move(_module));
		}

		m_jit.fin();

		// Register function pointer
		const spu_function_t fn = reinterpret_cast<spu_function_t>(m_jit.get_engine().getPointerToFunction(main_func));

		// Install unconditionally, possibly replacing existing one from spu_fast
		add_loc->compiled = fn;

		// Rebuild trampoline if necessary
		if (!m_spurt->rebuild_ubertrampoline(func.data[0]))
		{
			if (auto& cache = g_fxo->get<spu_cache>())
			{
				if (add_to_file)
				{
					cache.add(func);
				}
			}

			return nullptr;
		}

		add_loc->compiled.notify_all();

		if (g_cfg.core.spu_debug)
		{
			out.flush();
			fs::write_file(m_spurt->get_cache_path() + "spu-ir.log", fs::create + fs::write + fs::append, llvm_log);
		}

#if defined(__APPLE__)
		pthread_jit_write_protect_np(true);
#endif
#if defined(ARCH_ARM64)
		// Flush all cache lines after potentially writing executable code
		asm("ISB");
		asm("DSB ISH");
#endif

		if (auto& cache = g_fxo->get<spu_cache>())
		{
			if (add_to_file)
			{
				cache.add(func);
			}

			spu_log.success("New SPU block compiled successfully (size=%u)", func_size);
		}

		return fn;
	}

	static void interp_check(spu_thread* _spu, bool after)
	{
		static thread_local std::array<v128, 128> s_gpr;

		if (!after)
		{
			// Preserve reg state
			s_gpr = _spu->gpr;

			// Execute interpreter instruction
			const u32 op = *reinterpret_cast<const be_t<u32>*>(_spu->_ptr<u8>(0) + _spu->pc);
			if (!g_fxo->get<spu_interpreter_rt>().decode(op)(*_spu, {op}))
				spu_log.fatal("Bad instruction");

			// Swap state
			for (u32 i = 0; i < s_gpr.size(); ++i)
				std::swap(_spu->gpr[i], s_gpr[i]);
		}
		else
		{
			// Check saved state
			for (u32 i = 0; i < s_gpr.size(); ++i)
			{
				if (_spu->gpr[i] != s_gpr[i])
				{
					spu_log.fatal("Register mismatch: $%u\n%s\n%s", i, _spu->gpr[i], s_gpr[i]);
					_spu->state += cpu_flag::dbg_pause;
				}
			}
		}
	}

	spu_function_t compile_interpreter()
	{
		using namespace llvm;

		m_engine->clearAllGlobalMappings();

		// Create LLVM module
		std::unique_ptr<Module> _module = std::make_unique<Module>("spu_interpreter.obj", m_context);
#if LLVM_VERSION_MAJOR >= 21 && (LLVM_VERSION_MINOR >= 1 || LLVM_VERSION_MAJOR >= 22)
		_module->setTargetTriple(Triple(jit_compiler::triple2()));
#else
		_module->setTargetTriple(jit_compiler::triple2());
#endif
		_module->setDataLayout(m_jit.get_engine().getTargetMachine()->createDataLayout());
		m_module = _module.get();

		// Initialize IR Builder
		IRBuilder<> irb(m_context);
		m_ir = &irb;

		// Create interpreter table
		const auto if_type = get_ftype<void, u8*, u8*, u32, u32, u8*, u32, u8*>();
		m_function_table = new GlobalVariable(*m_module, ArrayType::get(m_ir->getPtrTy(), 1ull << m_interp_magn), true, GlobalValue::InternalLinkage, nullptr);

		init_luts();

		// Add return function
		const auto ret_func = cast<Function>(_module->getOrInsertFunction("spu_ret", if_type).getCallee());
		ret_func->setCallingConv(CallingConv::GHC);
		ret_func->setLinkage(GlobalValue::InternalLinkage);
		m_ir->SetInsertPoint(BasicBlock::Create(m_context, "", ret_func));
		m_thread = ret_func->getArg(1);
		m_interp_pc = ret_func->getArg(2);
		m_ir->CreateRetVoid();

		// Add entry function, serves as a trampoline
		const auto main_func = llvm::cast<Function>(m_module->getOrInsertFunction("spu_interpreter", get_ftype<void, u8*, u8*, u8*>()).getCallee());
#ifdef _WIN32
		main_func->setCallingConv(CallingConv::Win64);
#endif
		set_function(main_func);

		// Load pc and opcode
		m_interp_pc = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::pc));
		m_interp_op = m_ir->CreateLoad(get_type<u32>(), _ptr(m_lsptr, m_ir->CreateZExt(m_interp_pc, get_type<u64>())));
		m_interp_op = m_ir->CreateCall(get_intrinsic<u32>(Intrinsic::bswap), {m_interp_op});

		// Pinned constant, address of interpreter table
		m_interp_table = m_ir->CreateGEP(m_function_table->getValueType(), m_function_table, {m_ir->getInt64(0), m_ir->getInt64(0)});

		// Pinned constant, mask for shifted register index
		m_interp_7f0 = m_ir->getInt32(0x7f0);

		// Pinned constant, address of first register
		m_interp_regs = _ptr(m_thread, get_reg_offset(0));

		// Save host thread's stack pointer
		const auto native_sp = spu_ptr(&spu_thread::hv_ctx, &rpcs3::hypervisor_context_t::regs);
#if defined(ARCH_X64)
		const auto rsp_name = MetadataAsValue::get(m_context, MDNode::get(m_context, {MDString::get(m_context, "rsp")}));
#elif defined(ARCH_ARM64)
		const auto rsp_name = MetadataAsValue::get(m_context, MDNode::get(m_context, {MDString::get(m_context, "sp")}));
#endif
		m_ir->CreateStore(m_ir->CreateCall(get_intrinsic<u64>(Intrinsic::read_register), {rsp_name}), native_sp);

		// Decode (shift) and load function pointer
		const auto first = m_ir->CreateLoad(m_ir->getPtrTy(), m_ir->CreateGEP(m_ir->getPtrTy(), m_interp_table, m_ir->CreateLShr(m_interp_op, 32u - m_interp_magn)));
		const auto call0 = m_ir->CreateCall(if_type, first, {m_lsptr, m_thread, m_interp_pc, m_interp_op, m_interp_table, m_interp_7f0, m_interp_regs});
		call0->setCallingConv(CallingConv::GHC);
		m_ir->CreateRetVoid();

		// Create helper globals
		{
			std::vector<llvm::Constant*> float_to;
			std::vector<llvm::Constant*> to_float;
			float_to.reserve(256);
			to_float.reserve(256);

			for (int i = 0; i < 256; ++i)
			{
				float_to.push_back(ConstantFP::get(get_type<f32>(), std::exp2(173 - i)));
				to_float.push_back(ConstantFP::get(get_type<f32>(), std::exp2(i - 155)));
			}

			const auto atype = ArrayType::get(get_type<f32>(), 256);
			m_scale_float_to = new GlobalVariable(*m_module, atype, true, GlobalValue::InternalLinkage, ConstantArray::get(atype, float_to));
			m_scale_to_float = new GlobalVariable(*m_module, atype, true, GlobalValue::InternalLinkage, ConstantArray::get(atype, to_float));
		}

		// Fill interpreter table
		std::array<llvm::Function*, 256> ifuncs{};
		std::vector<llvm::Constant*> iptrs;
		iptrs.reserve(1ull << m_interp_magn);

		m_block = nullptr;

		auto last_itype = spu_itype::type{255};

		for (u32 i = 0; i < 1u << m_interp_magn;)
		{
			// Fake opcode
			const u32 op = i << (32u - m_interp_magn);

			// Instruction type
			const auto itype = g_spu_itype.decode(op);

			// Function name
			std::string fname = fmt::format("spu_%s", g_spu_iname.decode(op));

			if (last_itype != itype)
			{
				// Trigger automatic information collection (probing)
				m_op_const_mask = 0;
			}
			else
			{
				// Inject const mask into function name
				fmt::append(fname, "_%X", (i & (m_op_const_mask >> (32u - m_interp_magn))) | (1u << m_interp_magn));
			}

			// Decode instruction name, access function
			const auto f = cast<Function>(_module->getOrInsertFunction(fname, if_type).getCallee());

			// Build if necessary
			if (f->empty())
			{
				if (last_itype != itype)
				{
					ifuncs[static_cast<usz>(itype)] = f;
				}

				f->setCallingConv(CallingConv::GHC);

				m_function = f;
				m_lsptr  = f->getArg(0);
				m_thread = f->getArg(1);
				m_interp_pc = f->getArg(2);
				m_interp_op = f->getArg(3);
				m_interp_table = f->getArg(4);
				m_interp_7f0 = f->getArg(5);
				m_interp_regs = f->getArg(6);

				m_ir->SetInsertPoint(BasicBlock::Create(m_context, "", f));
				m_memptr = m_ir->CreateLoad(get_type<u8*>(), spu_ptr(&spu_thread::memory_base_addr));

				switch (itype)
				{
				case spu_itype::UNK:
				case spu_itype::DFCEQ:
				case spu_itype::DFCMEQ:
				case spu_itype::DFCGT:
				case spu_itype::DFCMGT:
				case spu_itype::DFTSV:
				case spu_itype::STOP:
				case spu_itype::STOPD:
				case spu_itype::RDCH:
				case spu_itype::WRCH:
				{
					// Invalid or abortable instruction. Save current address.
					m_ir->CreateStore(m_interp_pc, spu_ptr(&spu_thread::pc));
					[[fallthrough]];
				}
				default:
				{
					break;
				}
				}

				{
					m_interp_bblock = nullptr;

					// Next instruction (no wraparound at the end of LS)
					m_interp_pc_next = m_ir->CreateAdd(m_interp_pc, m_ir->getInt32(4));

					bool check = false;

					if (itype == spu_itype::WRCH ||
						itype == spu_itype::RDCH ||
						itype == spu_itype::RCHCNT ||
						itype == spu_itype::STOP ||
						itype == spu_itype::STOPD ||
						itype & spu_itype::floating ||
						itype & spu_itype::branch)
					{
						check = false;
					}

					if (itype & spu_itype::branch)
					{
						// Instruction changes pc - change order.
						(this->*decode(op))({op});

						if (m_interp_bblock)
						{
							m_ir->SetInsertPoint(m_interp_bblock);
							m_interp_bblock = nullptr;
						}
					}

					if (!m_ir->GetInsertBlock()->getTerminator())
					{
						if (check)
						{
							m_ir->CreateStore(m_interp_pc, spu_ptr(&spu_thread::pc));
						}

						// Decode next instruction.
						const auto next_pc = itype & spu_itype::branch ? m_interp_pc : m_interp_pc_next;
						const auto be32_op = m_ir->CreateLoad(get_type<u32>(), _ptr(m_lsptr, m_ir->CreateZExt(next_pc, get_type<u64>())));
						const auto next_op = m_ir->CreateCall(get_intrinsic<u32>(Intrinsic::bswap), {be32_op});
						const auto next_if = m_ir->CreateLoad(m_ir->getPtrTy(), m_ir->CreateGEP(m_ir->getPtrTy(), m_interp_table, m_ir->CreateLShr(next_op, 32u - m_interp_magn)));
						llvm::cast<LoadInst>(next_if)->setVolatile(true);

						if (!(itype & spu_itype::branch))
						{
							if (check)
							{
								call("spu_interp_check", &interp_check, m_thread, m_ir->getFalse());
							}

							// Normal instruction.
							(this->*decode(op))({op});

							if (check && !m_ir->GetInsertBlock()->getTerminator())
							{
								call("spu_interp_check", &interp_check, m_thread, m_ir->getTrue());
							}

							m_interp_pc = m_interp_pc_next;
						}

						if (last_itype != itype)
						{
							// Reset to discard dead code
							llvm::cast<LoadInst>(next_if)->setVolatile(false);

							if (itype & spu_itype::branch)
							{
								const auto _stop = BasicBlock::Create(m_context, "", f);
								const auto _next = BasicBlock::Create(m_context, "", f);
								m_ir->CreateCondBr(m_ir->CreateIsNotNull(m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::state))), _stop, _next, m_md_unlikely);
								m_ir->SetInsertPoint(_stop);
								m_ir->CreateStore(m_interp_pc, spu_ptr(&spu_thread::pc));

								const auto escape_yes = BasicBlock::Create(m_context, "", f);
								const auto escape_no = BasicBlock::Create(m_context, "", f);
								m_ir->CreateCondBr(call("spu_exec_check_state", &exec_check_state, m_thread), escape_yes, escape_no);
								m_ir->SetInsertPoint(escape_yes);
								call("spu_escape", spu_runtime::g_escape, m_thread);
								m_ir->CreateBr(_next);
								m_ir->SetInsertPoint(escape_no);
								m_ir->CreateBr(_next);
								m_ir->SetInsertPoint(_next);
							}

							llvm::Value* fret = m_interp_table;

							if (itype == spu_itype::WRCH ||
								itype == spu_itype::RDCH ||
								itype == spu_itype::RCHCNT ||
								itype == spu_itype::STOP ||
								itype == spu_itype::STOPD ||
								itype == spu_itype::UNK ||
								itype == spu_itype::DFCMEQ ||
								itype == spu_itype::DFCMGT ||
								itype == spu_itype::DFCGT ||
								itype == spu_itype::DFCEQ ||
								itype == spu_itype::DFTSV)
							{
								m_interp_7f0  = m_ir->getInt32(0x7f0);
								m_interp_regs = _ptr(m_thread, get_reg_offset(0));
								fret = ret_func;
							}
							else if (!(itype & spu_itype::branch))
							{
								// Hack: inline ret instruction before final jmp; this is not reliable.
#ifdef ARCH_X64
								m_ir->CreateCall(InlineAsm::get(get_ftype<void>(), "ret", "", true, false, InlineAsm::AD_Intel));
#else
								m_ir->CreateCall(InlineAsm::get(get_ftype<void>(), "ret", "", true, false));
#endif
								fret = ret_func;
							}

							const auto arg3 = UndefValue::get(get_type<u32>());
							const auto _ret = m_ir->CreateCall(if_type, fret, {m_lsptr, m_thread, m_interp_pc, arg3, m_interp_table, m_interp_7f0, m_interp_regs});
							_ret->setCallingConv(CallingConv::GHC);
							_ret->setTailCall();
							m_ir->CreateRetVoid();
						}

						if (!m_ir->GetInsertBlock()->getTerminator())
						{
							// Call next instruction.
							const auto _stop = BasicBlock::Create(m_context, "", f);
							const auto _next = BasicBlock::Create(m_context, "", f);
							m_ir->CreateCondBr(m_ir->CreateIsNotNull(m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::state))), _stop, _next, m_md_unlikely);
							m_ir->SetInsertPoint(_next);

							if (itype == spu_itype::WRCH ||
								itype == spu_itype::RDCH ||
								itype == spu_itype::RCHCNT ||
								itype == spu_itype::STOP ||
								itype == spu_itype::STOPD)
							{
								m_interp_7f0  = m_ir->getInt32(0x7f0);
								m_interp_regs = _ptr(m_thread, get_reg_offset(0));
							}

							const auto ncall = m_ir->CreateCall(if_type, next_if, {m_lsptr, m_thread, m_interp_pc, next_op, m_interp_table, m_interp_7f0, m_interp_regs});
							ncall->setCallingConv(CallingConv::GHC);
							ncall->setTailCall();
							m_ir->CreateRetVoid();
							m_ir->SetInsertPoint(_stop);
							m_ir->CreateStore(m_interp_pc, spu_ptr(&spu_thread::pc));
							call("spu_escape", spu_runtime::g_escape, m_thread)->setTailCall();
							m_ir->CreateRetVoid();
						}
					}
				}
			}

			if (last_itype != itype && g_cfg.core.spu_decoder != spu_decoder_type::llvm)
			{
				// Repeat after probing
				last_itype = itype;
			}
			else
			{
				// Add to the table
				iptrs.push_back(f);
				i++;
			}
		}

		m_function_table->setInitializer(ConstantArray::get(ArrayType::get(m_ir->getPtrTy(), 1ull << m_interp_magn), iptrs));
		m_function_table = nullptr;

		for (auto& f : *_module)
		{
			run_transforms(f);
		}

		std::string llvm_log;
		raw_string_ostream out(llvm_log);

		if (g_cfg.core.spu_debug)
		{
			fmt::append(llvm_log, "LLVM IR (interpreter):\n");
			out << *_module; // print IR
			out << "\n\n";
		}

		if (verifyModule(*_module, &out))
		{
			out.flush();
			spu_log.error("LLVM: Verification failed:\n%s", llvm_log);

			if (g_cfg.core.spu_debug)
			{
				fs::write_file(m_spurt->get_cache_path() + "spu-ir.log", fs::create + fs::write + fs::append, llvm_log);
			}

			fmt::throw_exception("Compilation failed");
		}

		if (g_cfg.core.spu_debug)
		{
			// Testing only
			m_jit.add(std::move(_module), m_spurt->get_cache_path() + "llvm/");
		}
		else
		{
			m_jit.add(std::move(_module));
		}

		m_jit.fin();

		// Register interpreter entry point
		spu_runtime::g_interpreter = reinterpret_cast<spu_function_t>(m_jit.get_engine().getPointerToFunction(main_func));

		for (u32 i = 0; i < spu_runtime::g_interpreter_table.size(); i++)
		{
			// Fill exported interpreter table
			spu_runtime::g_interpreter_table[i] = ifuncs[i] ? reinterpret_cast<u64>(m_jit.get_engine().getPointerToFunction(ifuncs[i])) : 0;
		}

		if (!spu_runtime::g_interpreter)
		{
			return nullptr;
		}

		if (g_cfg.core.spu_debug)
		{
			out.flush();
			fs::write_file(m_spurt->get_cache_path() + "spu-ir.log", fs::create + fs::write + fs::append, llvm_log);
		}

		return spu_runtime::g_interpreter;
	}

	static bool exec_check_state(spu_thread* _spu)
	{
		return _spu->check_state();
	}

	template <spu_intrp_func_t F>
	static void exec_fall(spu_thread* _spu, spu_opcode_t op)
	{
		if (F(*_spu, op))
		{
			_spu->pc += 4;
		}
	}

	template <spu_intrp_func_t F>
	void fall(spu_opcode_t op)
	{
		std::string name = fmt::format("spu_%s", g_spu_iname.decode(op.opcode));

		if (m_interp_magn)
		{
			call(name, F, m_thread, m_interp_op);
			return;
		}

		update_pc();
		call(name, &exec_fall<F>, m_thread, m_ir->getInt32(op.opcode));
	}

	[[noreturn]] static void exec_unk(spu_thread*, u32 op)
	{
		fmt::throw_exception("Unknown/Illegal instruction (0x%08x)", op);
	}

	void UNK(spu_opcode_t op_unk)
	{
		if (m_interp_magn)
		{
			m_ir->CreateStore(m_interp_pc, spu_ptr(&spu_thread::pc));
			call("spu_unknown", &exec_unk, m_thread, m_ir->getInt32(op_unk.opcode));
			return;
		}

		m_block->block_end = m_ir->GetInsertBlock();
		update_pc();
		call("spu_unknown", &exec_unk, m_thread, m_ir->getInt32(op_unk.opcode));
	}

	static void exec_stop(spu_thread* _spu, u32 code)
	{
		if (!_spu->stop_and_signal(code) || _spu->state & cpu_flag::again)
		{
			spu_runtime::g_escape(_spu);
		}

		if (_spu->test_stopped())
		{
			_spu->pc += 4;
			spu_runtime::g_escape(_spu);
		}
	}

	void STOP(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			call("spu_syscall", &exec_stop, m_thread, m_ir->CreateAnd(m_interp_op, m_ir->getInt32(0x3fff)));
			return;
		}

		update_pc();
		ensure_gpr_stores();
		call("spu_syscall", &exec_stop, m_thread, m_ir->getInt32(op.opcode & 0x3fff));

		if (g_cfg.core.spu_block_size == spu_block_size_type::safe)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			update_pc(m_pos + 4);
			tail_chunk(m_dispatch);
			return;
		}
	}

	void STOPD(spu_opcode_t) //
	{
		if (m_interp_magn)
		{
			call("spu_syscall", &exec_stop, m_thread, m_ir->getInt32(0x3fff));
			return;
		}

		STOP(spu_opcode_t{0x3fff});
	}

	static u32 exec_rdch(spu_thread* _spu, u32 ch)
	{
		const s64 result = _spu->get_ch_value(ch);

		if (result < 0 || _spu->state & cpu_flag::again)
		{
			spu_runtime::g_escape(_spu);
		}

		static_cast<void>(_spu->test_stopped());
		return static_cast<u32>(result & 0xffffffff);
	}

	static u32 exec_read_in_mbox(spu_thread* _spu)
	{
		// TODO
		return exec_rdch(_spu, SPU_RdInMbox);
	}

	static u32 exec_read_dec(spu_thread* _spu)
	{
		const u32 res = _spu->read_dec().first;

		if (res > 1500 && g_cfg.core.spu_loop_detection)
		{
			_spu->state += cpu_flag::wait;
			std::this_thread::yield();
			static_cast<void>(_spu->test_stopped());
		}

		return res;
	}

	static u32 exec_read_events(spu_thread* _spu)
	{
		// TODO
		return exec_rdch(_spu, SPU_RdEventStat);
	}

	void ensure_gpr_stores()
	{
		if (m_block)
		{
			// Make previous stores not able to be reordered beyond this point or be deleted
			std::for_each(m_block->store_context_ctr.begin(), m_block->store_context_ctr.end(), FN(x++));
			m_block->has_gpr_memory_barriers = true;
		}
	}

	llvm::Value* get_rdch(spu_opcode_t op, u32 off, bool atomic)
	{
		const auto ptr = _ptr(m_thread, off);
		llvm::Value* val0;

		if (atomic)
		{
			const auto val = m_ir->CreateAtomicRMW(llvm::AtomicRMWInst::Xchg, ptr, m_ir->getInt64(0), llvm::MaybeAlign{8}, llvm::AtomicOrdering::Acquire);
			val0 = val;
		}
		else
		{
			const auto val = m_ir->CreateLoad(get_type<u64>(), ptr);
			val->setAtomic(llvm::AtomicOrdering::Acquire);
			m_ir->CreateStore(m_ir->getInt64(0), ptr)->setAtomic(llvm::AtomicOrdering::Release);
			val0 = val;
		}

		const auto _cur = m_ir->GetInsertBlock();
		const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto wait = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto cond = m_ir->CreateICmpSLT(val0, m_ir->getInt64(0));
		val0 = m_ir->CreateTrunc(val0, get_type<u32>());
		m_ir->CreateCondBr(cond, done, wait);
		m_ir->SetInsertPoint(wait);
		update_pc();
		const auto val1 = call("spu_read_channel", &exec_rdch, m_thread, m_ir->getInt32(op.ra));
		m_ir->CreateBr(done);
		m_ir->SetInsertPoint(done);
		const auto rval = m_ir->CreatePHI(get_type<u32>(), 2);
		rval->addIncoming(val0, _cur);
		rval->addIncoming(val1, wait);
		return rval;
	}

	void RDCH(spu_opcode_t op) //
	{
		value_t<u32> res;

		if (m_interp_magn)
		{
			res.value = call("spu_read_channel", &exec_rdch, m_thread, get_imm<u32>(op.ra).value);
			set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
			return;
		}

		switch (op.ra)
		{
		case SPU_RdSRR0:
		{
			res.value = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::srr0));
			break;
		}
		case SPU_RdInMbox:
		{
			update_pc();
			ensure_gpr_stores();
			res.value = call("spu_read_in_mbox", &exec_read_in_mbox, m_thread);
			break;
		}
		case MFC_RdTagStat:
		{
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_tag_stat), false);
			break;
		}
		case MFC_RdTagMask:
		{
			res.value = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::ch_tag_mask));
			break;
		}
		case SPU_RdSigNotify1:
		{
			update_pc();
			ensure_gpr_stores();
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_snr1), true);
			break;
		}
		case SPU_RdSigNotify2:
		{
			update_pc();
			ensure_gpr_stores();
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_snr2), true);
			break;
		}
		case MFC_RdAtomicStat:
		{
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_atomic_stat), false);
			break;
		}
		case MFC_RdListStallStat:
		{
			res.value = get_rdch(op, ::offset32(&spu_thread::ch_stall_stat), false);
			break;
		}
		case SPU_RdDec:
		{
#if defined(ARCH_X64)
			if (utils::get_tsc_freq() && !(g_cfg.core.spu_loop_detection) && (g_cfg.core.clocks_scale == 100))
			{
				const auto timebase_offs = m_ir->CreateLoad(get_type<u64>(), m_ir->CreateIntToPtr(m_ir->getInt64(reinterpret_cast<u64>(&g_timebase_offs)), get_type<u64*>()));
				const auto timestamp = m_ir->CreateLoad(get_type<u64>(), spu_ptr(&spu_thread::ch_dec_start_timestamp));
				const auto dec_value = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::ch_dec_value));
				const auto tsc = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_rdtsc));
				const auto tscx = m_ir->CreateMul(m_ir->CreateUDiv(tsc, m_ir->getInt64(utils::get_tsc_freq())), m_ir->getInt64(80000000));
				const auto tscm = m_ir->CreateUDiv(m_ir->CreateMul(m_ir->CreateURem(tsc, m_ir->getInt64(utils::get_tsc_freq())), m_ir->getInt64(80000000)), m_ir->getInt64(utils::get_tsc_freq()));
				const auto tsctb = m_ir->CreateSub(m_ir->CreateAdd(tscx, tscm), timebase_offs);
				const auto frz = m_ir->CreateLoad(get_type<u8>(), spu_ptr(&spu_thread::is_dec_frozen));
				const auto frzev = m_ir->CreateICmpEQ(frz, m_ir->getInt8(0));

				const auto delta = m_ir->CreateTrunc(m_ir->CreateSub(tsctb, timestamp), get_type<u32>());
				const auto deltax = m_ir->CreateSelect(frzev, delta, m_ir->getInt32(0));
				res.value = m_ir->CreateSub(dec_value, deltax);
				break;
			}
#endif
			res.value = call("spu_read_decrementer", &exec_read_dec, m_thread);
			break;
		}
		case SPU_RdEventMask:
		{
			const auto value = m_ir->CreateLoad(get_type<u64>(), spu_ptr(&spu_thread::ch_events));
			value->setAtomic(llvm::AtomicOrdering::Acquire);
			res.value = m_ir->CreateTrunc(m_ir->CreateLShr(value, 32), get_type<u32>());
			break;
		}
		case SPU_RdEventStat:
		{
			update_pc();

			if (g_cfg.savestate.compatible_mode)
			{
				ensure_gpr_stores();
			}
			else
			{
				m_ir->CreateStore(m_ir->getInt8(1), spu_ptr(&spu_thread::unsavable));
			}

			res.value = call("spu_read_events", &exec_read_events, m_thread);

			if (!g_cfg.savestate.compatible_mode)
			{
				m_ir->CreateStore(m_ir->getInt8(0), spu_ptr(&spu_thread::unsavable));
			}

			break;
		}
		case SPU_RdMachStat:
		{
			res.value = m_ir->CreateZExt(m_ir->CreateLoad(get_type<u8>(), spu_ptr(&spu_thread::interrupts_enabled)), get_type<u32>());
			res.value = m_ir->CreateOr(res.value, m_ir->CreateAnd(m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::thread_type)), m_ir->getInt32(2)));
			break;
		}

		default:
		{
			update_pc();
			ensure_gpr_stores();
			res.value = call("spu_read_channel", &exec_rdch, m_thread, m_ir->getInt32(op.ra));
			break;
		}
		}

		set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
	}

	static u32 exec_rchcnt(spu_thread* _spu, u32 ch)
	{
		return _spu->get_ch_count(ch);
	}

	static u32 exec_get_events(spu_thread* _spu, u32 mask)
	{
		return _spu->get_events(mask).count;
	}

	llvm::Value* get_rchcnt(u32 off, u64 inv = 0)
	{
		const auto val = m_ir->CreateLoad(get_type<u64>(), _ptr(m_thread, off));
		val->setAtomic(llvm::AtomicOrdering::Acquire);
		const auto shv = m_ir->CreateLShr(val, spu_channel::off_count);
		return m_ir->CreateTrunc(m_ir->CreateXor(shv, inv), get_type<u32>());
	}

	llvm::Value* wait_rchcnt(u32 off, u32 inv = 0)
	{
		auto wait_on_channel = [](spu_thread* _spu, spu_channel* ch, u32 is_read) -> u32
		{
			if (is_read)
			{
				ch->pop_wait(*_spu, false);
			}
			else
			{
				ch->push_wait(*_spu, 0, false);
			}

			return ch->get_count();
		};

		return m_ir->CreateXor(call("wait_on_spu_channel", +wait_on_channel, m_thread, _ptr(m_thread, off), m_ir->getInt32(inv == 0u)), m_ir->getInt32(inv));
	}

	void RCHCNT(spu_opcode_t op) //
	{
		value_t<u32> res{};

		if (m_interp_magn)
		{
			res.value = call("spu_read_channel_count", &exec_rchcnt, m_thread, get_imm<u32>(op.ra).value);
			set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
			return;
		}

		switch (op.ra)
		{
		case SPU_WrOutMbox:
		case SPU_WrOutIntrMbox:
		case SPU_RdSigNotify1:
		case SPU_RdSigNotify2:
		case SPU_RdInMbox:
		case SPU_RdEventStat:
		{
			bool loop_is_likely = op.ra == SPU_RdSigNotify1 || op.ra == SPU_RdSigNotify2;

			for (u32 block_start : m_block->bb->preds)
			{
				if (block_start >= m_pos)
				{
					loop_is_likely = true;
					break;
				}
			}

			if (loop_is_likely || g_cfg.savestate.compatible_mode)
			{
				ensure_gpr_stores();
				check_state(m_pos, false);
			}

			break;
		}
		default:
		{
			break;
		}
		}

		if (m_inst_attrs[(m_pos - m_base) / 4] == inst_attr::rchcnt_loop)
		{
			switch (op.ra)
			{
			case SPU_WrOutMbox:
			{
				res.value = wait_rchcnt(::offset32(&spu_thread::ch_out_mbox), true);
				break;
			}
			case SPU_WrOutIntrMbox:
			{
				res.value = wait_rchcnt(::offset32(&spu_thread::ch_out_intr_mbox), true);
				break;
			}
			case SPU_RdSigNotify1:
			{
				res.value = wait_rchcnt(::offset32(&spu_thread::ch_snr1));
				break;
			}
			case SPU_RdSigNotify2:
			{
				res.value = wait_rchcnt(::offset32(&spu_thread::ch_snr2));
				break;
			}
			case SPU_RdInMbox:
			{
				auto wait_inbox = [](spu_thread* _spu, spu_channel_4_t* ch) -> u32
				{
					return ch->pop_wait(*_spu, false), ch->get_count();
				};

				res.value = call("wait_spu_inbox", +wait_inbox, m_thread, spu_ptr(&spu_thread::ch_in_mbox));
				break;
			}
			default: break;
			}

			if (res.value)
			{
				set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
				return;
			}
		}

		switch (op.ra)
		{
		case SPU_WrOutMbox:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_out_mbox), true);
			break;
		}
		case SPU_WrOutIntrMbox:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_out_intr_mbox), true);
			break;
		}
		case MFC_RdTagStat:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_tag_stat));
			break;
		}
		case MFC_RdListStallStat:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_stall_stat));
			break;
		}
		case SPU_RdSigNotify1:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_snr1));
			break;
		}
		case SPU_RdSigNotify2:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_snr2));
			break;
		}
		case MFC_RdAtomicStat:
		{
			res.value = get_rchcnt(::offset32(&spu_thread::ch_atomic_stat));
			break;
		}
		case MFC_WrTagUpdate:
		{
			res.value = m_ir->getInt32(1);
			break;
		}
		case MFC_Cmd:
		{
			res.value = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::mfc_size));
			res.value = m_ir->CreateSub(m_ir->getInt32(16), res.value);
			break;
		}
		case SPU_RdInMbox:
		{
			const auto value = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::ch_in_mbox));
			value->setAtomic(llvm::AtomicOrdering::Acquire);
			res.value = value;
			res.value = m_ir->CreateLShr(res.value, 8);
			res.value = m_ir->CreateAnd(res.value, 7);
			break;
		}
		case SPU_RdEventStat:
		{
			const auto mask = m_ir->CreateTrunc(m_ir->CreateLShr(m_ir->CreateLoad(get_type<u64>(), spu_ptr(&spu_thread::ch_events)), 32), get_type<u32>());
			res.value = call("spu_get_events", &exec_get_events, m_thread, mask);
			break;
		}

		// Channels with a constant count of 1:
		case SPU_WrEventMask:
		case SPU_WrEventAck:
		case SPU_WrDec:
		case SPU_RdDec:
		case SPU_RdEventMask:
		case SPU_RdMachStat:
		case SPU_WrSRR0:
		case SPU_RdSRR0:
		case SPU_Set_Bkmk_Tag:
		case SPU_PM_Start_Ev:
		case SPU_PM_Stop_Ev:
		case MFC_RdTagMask:
		case MFC_LSA:
		case MFC_EAH:
		case MFC_EAL:
		case MFC_Size:
		case MFC_TagID:
		case MFC_WrTagMask:
		case MFC_WrListStallAck:
		{
			res.value = m_ir->getInt32(1);
			break;
		}

		default:
		{
			res.value = call("spu_read_channel_count", &exec_rchcnt, m_thread, m_ir->getInt32(op.ra));
			break;
		}
		}

		set_vr(op.rt, insert(splat<u32[4]>(0), 3, res));
	}

	static void exec_wrch(spu_thread* _spu, u32 ch, u32 value)
	{
		if (!_spu->set_ch_value(ch, value) || _spu->state & cpu_flag::again)
		{
			spu_runtime::g_escape(_spu);
		}

		static_cast<void>(_spu->test_stopped());
	}

	static void exec_list_unstall(spu_thread* _spu, u32 tag)
	{
		for (u32 i = 0; i < _spu->mfc_size; i++)
		{
			if (_spu->mfc_queue[i].tag == (tag | 0x80))
			{
				_spu->mfc_queue[i].tag &= 0x7f;
			}
		}

		_spu->do_mfc();
	}

	template <bool Saveable>
	static void exec_mfc_cmd(spu_thread* _spu)
	{
		if constexpr (!Saveable)
		{
			_spu->unsavable = true;
		}

		if (!_spu->process_mfc_cmd() || _spu->state & cpu_flag::again)
		{
			fmt::throw_exception("exec_mfc_cmd(): Should not abort!");
		}

		static_cast<void>(_spu->test_stopped());

		if constexpr (!Saveable)
		{
			_spu->unsavable = false;
		}
	}

	void WRCH(spu_opcode_t op) //
	{
		const auto val = eval(extract(get_vr(op.rt), 3));

		if (m_interp_magn)
		{
			call("spu_write_channel", &exec_wrch, m_thread, get_imm<u32>(op.ra).value, val.value);
			return;
		}

		switch (op.ra)
		{
		case SPU_WrSRR0:
		{
			m_ir->CreateStore(eval(val & 0x3fffc).value, spu_ptr(&spu_thread::srr0));
			return;
		}
		case SPU_WrOutIntrMbox:
		{
			// TODO
			break;
		}
		case SPU_WrOutMbox:
		{
			// TODO
			break;
		}
		case MFC_WrTagMask:
		{
			// TODO
			m_ir->CreateStore(val.value, spu_ptr(&spu_thread::ch_tag_mask));
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto _mfc = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpNE(m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::ch_tag_upd)), m_ir->getInt32(MFC_TAG_UPDATE_IMMEDIATE)), _mfc, next);
			m_ir->SetInsertPoint(_mfc);
			update_pc();
			call("spu_write_channel", &exec_wrch, m_thread, m_ir->getInt32(op.ra), val.value);
			m_ir->CreateBr(next);
			m_ir->SetInsertPoint(next);
			return;
		}
		case MFC_WrTagUpdate:
		{
			if (true)
			{
				const auto tag_mask  = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::ch_tag_mask));
				const auto mfc_fence = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::mfc_fence));
				const auto completed = m_ir->CreateAnd(tag_mask, m_ir->CreateNot(mfc_fence));
				const auto upd_ptr   = spu_ptr(&spu_thread::ch_tag_upd);
				const auto stat_ptr  = spu_ptr(&spu_thread::ch_tag_stat);
				const auto stat_val  = m_ir->CreateOr(m_ir->CreateZExt(completed, get_type<u64>()), s64{smin});

				const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto next0 = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto imm = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto any = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto update = llvm::BasicBlock::Create(m_context, "", m_function);

				m_ir->CreateCondBr(m_ir->CreateICmpEQ(val.value, m_ir->getInt32(MFC_TAG_UPDATE_IMMEDIATE)), imm, next0);
				m_ir->SetInsertPoint(imm);
				m_ir->CreateStore(val.value, upd_ptr);
				m_ir->CreateStore(stat_val, stat_ptr);
				m_ir->CreateBr(next);
				m_ir->SetInsertPoint(next0);
				m_ir->CreateCondBr(m_ir->CreateICmpULE(val.value, m_ir->getInt32(MFC_TAG_UPDATE_ALL)), any, fail, m_md_likely);

				// Illegal update, access violate with special address
				m_ir->SetInsertPoint(fail);
				const auto ptr = _ptr(m_memptr, 0xffdead04);
				m_ir->CreateStore(m_ir->getInt32("TAG\0"_u32), ptr);
				m_ir->CreateBr(next);

				m_ir->SetInsertPoint(any);
				const auto cond = m_ir->CreateSelect(m_ir->CreateICmpEQ(val.value, m_ir->getInt32(MFC_TAG_UPDATE_ANY))
					,  m_ir->CreateICmpNE(completed, m_ir->getInt32(0)), m_ir->CreateICmpEQ(completed, tag_mask));

				m_ir->CreateStore(m_ir->CreateSelect(cond, m_ir->getInt32(MFC_TAG_UPDATE_IMMEDIATE), val.value), upd_ptr);
				m_ir->CreateCondBr(cond, update, next, m_md_likely);
				m_ir->SetInsertPoint(update);
				m_ir->CreateStore(stat_val, stat_ptr);
				m_ir->CreateBr(next);
				m_ir->SetInsertPoint(next);
			}
			return;
		}
		case MFC_LSA:
		{
			set_reg_fixed(s_reg_mfc_lsa, val.value);
			return;
		}
		case MFC_EAH:
		{
			if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(val.value))
			{
				if (ci->getZExtValue() == 0)
				{
					return;
				}
			}

			spu_log.warning("[0x%x] MFC_EAH: $%u is not a zero constant", m_pos, +op.rt);
			//m_ir->CreateStore(val.value, spu_ptr(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::eah));
			return;
		}
		case MFC_EAL:
		{
			set_reg_fixed(s_reg_mfc_eal, val.value);
			return;
		}
		case MFC_Size:
		{
			set_reg_fixed(s_reg_mfc_size, trunc<u16>(val).eval(m_ir));
			return;
		}
		case MFC_TagID:
		{
			set_reg_fixed(s_reg_mfc_tag, trunc<u8>(val & 0x1f).eval(m_ir));
			return;
		}
		case MFC_Cmd:
		{
			// Prevent store elimination (TODO)
			m_block->store_context_ctr[s_reg_mfc_eal]++;
			m_block->store_context_ctr[s_reg_mfc_lsa]++;
			m_block->store_context_ctr[s_reg_mfc_tag]++;
			m_block->store_context_ctr[s_reg_mfc_size]++;

			if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(trunc<u8>(val).eval(m_ir)))
			{
				if (g_cfg.core.mfc_debug)
				{
					break;
				}

				bool must_use_cpp_functions = !!g_cfg.core.spu_accurate_dma;

				if (u64 cmdh = ci->getZExtValue() & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK | MFC_RESULT_MASK); g_cfg.core.rsx_fifo_accuracy || g_cfg.video.strict_rendering_mode || /*!g_use_rtm*/ true)
				{
					// TODO: don't require TSX (current implementation is TSX-only)
					if (cmdh == MFC_PUT_CMD || cmdh == MFC_SNDSIG_CMD)
					{
						must_use_cpp_functions = true;
					}
				}

				const auto eal = get_reg_fixed<u32>(s_reg_mfc_eal);
				const auto lsa = get_reg_fixed<u32>(s_reg_mfc_lsa);
				const auto tag = get_reg_fixed<u8>(s_reg_mfc_tag);

				const auto size = get_reg_fixed<u16>(s_reg_mfc_size);
				const auto mask = m_ir->CreateShl(m_ir->getInt32(1), zext<u32>(tag).eval(m_ir));
				const auto exec = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
				const auto next = llvm::BasicBlock::Create(m_context, "", m_function);

				const auto pf = spu_ptr(&spu_thread::mfc_fence);
				const auto pb = spu_ptr(&spu_thread::mfc_barrier);

				switch (u64 cmd = ci->getZExtValue())
				{
				case MFC_SDCRT_CMD:
				case MFC_SDCRTST_CMD:
				{
					return;
				}
				case MFC_PUTL_CMD:
				case MFC_PUTLB_CMD:
				case MFC_PUTLF_CMD:
				case MFC_PUTRL_CMD:
				case MFC_PUTRLB_CMD:
				case MFC_PUTRLF_CMD:
				case MFC_GETL_CMD:
				case MFC_GETLB_CMD:
				case MFC_GETLF_CMD:
				{
					m_ir->CreateBr(next);
					m_ir->SetInsertPoint(exec);
					m_ir->CreateUnreachable();
					m_ir->SetInsertPoint(fail);
					m_ir->CreateUnreachable();
					m_ir->SetInsertPoint(next);
					m_ir->CreateStore(ci, spu_ptr(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::cmd));
					update_pc();
					ensure_gpr_stores();
					call("spu_exec_mfc_cmd_saveable", &exec_mfc_cmd<true>, m_thread);
					return;
				}
				case MFC_SDCRZ_CMD:
				case MFC_GETLLAR_CMD:
				case MFC_PUTLLC_CMD:
				case MFC_PUTLLUC_CMD:
				case MFC_PUTQLLUC_CMD:
				{
					// TODO
					m_ir->CreateBr(next);
					m_ir->SetInsertPoint(exec);
					m_ir->CreateUnreachable();
					m_ir->SetInsertPoint(fail);
					m_ir->CreateUnreachable();
					m_ir->SetInsertPoint(next);
					m_ir->CreateStore(ci, spu_ptr(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::cmd));
					update_pc();
					call("spu_exec_mfc_cmd", &exec_mfc_cmd<false>, m_thread);
					return;
				}
				case MFC_SNDSIG_CMD:
				case MFC_SNDSIGB_CMD:
				case MFC_SNDSIGF_CMD:
				case MFC_PUT_CMD:
				case MFC_PUTB_CMD:
				case MFC_PUTF_CMD:
				case MFC_PUTR_CMD:
				case MFC_PUTRB_CMD:
				case MFC_PUTRF_CMD:
				case MFC_GET_CMD:
				case MFC_GETB_CMD:
				case MFC_GETF_CMD:
				{
					// Try to obtain constant size
					u64 csize = -1;

					if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(size.value))
					{
						csize = ci->getZExtValue();
					}

					if (cmd >= MFC_SNDSIG_CMD && csize != 4)
					{
						csize = -1;
					}

					llvm::Value* src = _ptr(m_lsptr, zext<u64>(lsa).eval(m_ir));
					llvm::Value* dst = _ptr(m_memptr, zext<u64>(eal).eval(m_ir));

					if (cmd & MFC_GET_CMD)
					{
						std::swap(src, dst);
					}

					llvm::Value* barrier = m_ir->CreateLoad(get_type<u32>(), pb);

					if (cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK))
					{
						barrier = m_ir->CreateOr(barrier, m_ir->CreateLoad(get_type<u32>(), pf));
					}

					const auto cond = m_ir->CreateIsNull(m_ir->CreateAnd(mask, barrier));
					m_ir->CreateCondBr(cond, exec, fail, m_md_likely);
					m_ir->SetInsertPoint(exec);

					const auto copy = llvm::BasicBlock::Create(m_context, "", m_function);

					// Always use interpreter function for MFC debug option
					if (!must_use_cpp_functions)
					{
						const auto mmio = llvm::BasicBlock::Create(m_context, "", m_function);
						m_ir->CreateCondBr(m_ir->CreateICmpUGE(eal.value, m_ir->getInt32(0xe0000000)), mmio, copy, m_md_unlikely);
						m_ir->SetInsertPoint(mmio);
					}

					m_ir->CreateStore(ci, spu_ptr(&spu_thread::ch_mfc_cmd, &spu_mfc_cmd::cmd));
					call("spu_exec_mfc_cmd", &exec_mfc_cmd<false>, m_thread);
					m_ir->CreateBr(next);
					m_ir->SetInsertPoint(copy);

					llvm::Type* vtype = get_type<u8[16]>();

					switch (csize)
					{
					case 0:
					case umax:
					{
						break;
					}
					case 1:
					{
						vtype = get_type<u8>();
						break;
					}
					case 2:
					{
						vtype = get_type<u16>();
						break;
					}
					case 4:
					{
						vtype = get_type<u32>();
						break;
					}
					case 8:
					{
						vtype = get_type<u64>();
						break;
					}
					default:
					{
						if (csize % 16 || csize > 0x4000)
						{
							spu_log.error("[0x%x] MFC_Cmd: invalid size %u", m_pos, csize);
						}
					}
					}

					// Check if the LS address is constant and 256 bit aligned
					u64 clsa = umax;

					if (auto ci = llvm::dyn_cast<llvm::ConstantInt>(lsa.value))
					{
						clsa = ci->getZExtValue();
					}

					u32 stride = 16;

					if (m_use_avx && csize >= 32 && !(clsa % 32))
					{
						vtype = get_type<u8[32]>();
						stride = 32;
					}

					if (csize > 0 && csize <= 16)
					{
						// Generate single copy operation
						m_ir->CreateStore(m_ir->CreateLoad(vtype, src), dst);
					}
					else if (csize <= stride * 16 && !(csize % 32))
					{
						// Generate fixed sequence of copy operations
						for (u32 i = 0; i < csize; i += stride)
						{
							const auto _src = _ptr(src, i);
							const auto _dst = _ptr(dst, i);
							if (csize - i < stride)
							{
								m_ir->CreateStore(m_ir->CreateLoad(get_type<u8[16]>(), _src), _dst);
							}
							else
							{
								m_ir->CreateAlignedStore(m_ir->CreateAlignedLoad(vtype, _src, llvm::MaybeAlign{16}), _dst, llvm::MaybeAlign{16});
							}
						}
					}
					else if (csize)
					{
						// TODO
						auto spu_memcpy = [](u8* dst, const u8* src, u32 size)
						{
							std::memcpy(dst, src, size);
						};
						call("spu_memcpy", +spu_memcpy, dst, src, zext<u32>(size).eval(m_ir));
					}

					// Disable certain thing
					m_ir->CreateStore(m_ir->getInt32(0), spu_ptr(&spu_thread::last_faddr));
					m_ir->CreateBr(next);
					break;
				}
				case MFC_BARRIER_CMD:
				case MFC_EIEIO_CMD:
				case MFC_SYNC_CMD:
				{
					const auto cond = m_ir->CreateIsNull(m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::mfc_size)));
					m_ir->CreateCondBr(cond, exec, fail, m_md_likely);
					m_ir->SetInsertPoint(exec);
					m_ir->CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
					m_ir->CreateBr(next);
					break;
				}
				default:
				{
					// TODO
					spu_log.error("[0x%x] MFC_Cmd: unknown command (0x%x)", m_pos, cmd);
					m_ir->CreateBr(next);
					m_ir->SetInsertPoint(exec);
					m_ir->CreateUnreachable();
					break;
				}
				}

				// Fallback: enqueue the command
				m_ir->SetInsertPoint(fail);

				// Get MFC slot, redirect to invalid memory address
				const auto slot = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::mfc_size));
				const auto off0 = m_ir->CreateAdd(m_ir->CreateMul(slot, m_ir->getInt32(sizeof(spu_mfc_cmd))), m_ir->getInt32(::offset32(&spu_thread::mfc_queue)));
				const auto ptr0 = _ptr(m_thread, m_ir->CreateZExt(off0, get_type<u64>()));
				const auto ptr1 = _ptr(m_memptr, 0xffdeadf0);
				const auto pmfc = m_ir->CreateSelect(m_ir->CreateICmpULT(slot, m_ir->getInt32(16)), ptr0, ptr1);
				m_ir->CreateStore(ci, _ptr(pmfc, &spu_mfc_cmd::cmd));

				switch (u64 cmd = ci->getZExtValue())
				{
				case MFC_GETLLAR_CMD:
				case MFC_PUTLLC_CMD:
				case MFC_PUTLLUC_CMD:
				case MFC_PUTQLLUC_CMD:
				{
					break;
				}
				case MFC_PUTL_CMD:
				case MFC_PUTLB_CMD:
				case MFC_PUTLF_CMD:
				case MFC_PUTRL_CMD:
				case MFC_PUTRLB_CMD:
				case MFC_PUTRLF_CMD:
				case MFC_GETL_CMD:
				case MFC_GETLB_CMD:
				case MFC_GETLF_CMD:
				{
					break;
				}
				case MFC_SDCRZ_CMD:
				{
					break;
				}
				case MFC_SNDSIG_CMD:
				case MFC_SNDSIGB_CMD:
				case MFC_SNDSIGF_CMD:
				case MFC_PUT_CMD:
				case MFC_PUTB_CMD:
				case MFC_PUTF_CMD:
				case MFC_PUTR_CMD:
				case MFC_PUTRB_CMD:
				case MFC_PUTRF_CMD:
				case MFC_GET_CMD:
				case MFC_GETB_CMD:
				case MFC_GETF_CMD:
				{
					m_ir->CreateStore(tag.value, _ptr(pmfc, &spu_mfc_cmd::tag));
					m_ir->CreateStore(size.value, _ptr(pmfc, &spu_mfc_cmd::size));
					m_ir->CreateStore(lsa.value, _ptr(pmfc, &spu_mfc_cmd::lsa));
					m_ir->CreateStore(eal.value, _ptr(pmfc, &spu_mfc_cmd::eal));
					m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(get_type<u32>(), pf), mask), pf);
					if (cmd & MFC_BARRIER_MASK)
						m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(get_type<u32>(), pb), mask), pb);
					break;
				}
				case MFC_BARRIER_CMD:
				case MFC_EIEIO_CMD:
				case MFC_SYNC_CMD:
				{
					m_ir->CreateStore(m_ir->getInt32(-1), pb);
					m_ir->CreateStore(m_ir->CreateOr(m_ir->CreateLoad(get_type<u32>(), pf), mask), pf);
					break;
				}
				default:
				{
					m_ir->CreateUnreachable();
					break;
				}
				}

				m_ir->CreateStore(m_ir->CreateAdd(slot, m_ir->getInt32(1)), spu_ptr(&spu_thread::mfc_size));
				m_ir->CreateBr(next);
				m_ir->SetInsertPoint(next);
				return;
			}

			// Fallback to unoptimized WRCH implementation (TODO)
			spu_log.warning("[0x%x] MFC_Cmd: $%u is not a constant", m_pos, +op.rt);
			break;
		}
		case MFC_WrListStallAck:
		{
			const auto mask = eval(splat<u32>(1) << (val & 0x1f));
			const auto _ptr = spu_ptr(&spu_thread::ch_stall_mask);
			const auto _old = m_ir->CreateLoad(get_type<u32>(), _ptr);
			const auto _new = m_ir->CreateAnd(_old, m_ir->CreateNot(mask.value));
			m_ir->CreateStore(_new, _ptr);
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto _mfc = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpNE(_old, _new), _mfc, next);
			m_ir->SetInsertPoint(_mfc);
			ensure_gpr_stores();
			update_pc();
			call("spu_list_unstall", &exec_list_unstall, m_thread, eval(val & 0x1f).value);
			m_ir->CreateBr(next);
			m_ir->SetInsertPoint(next);
			return;
		}
		case SPU_WrDec:
		{
			call("spu_get_events", &exec_get_events, m_thread, m_ir->getInt32(SPU_EVENT_TM));

#if defined(ARCH_X64)
			if (utils::get_tsc_freq() && !(g_cfg.core.spu_loop_detection) && (g_cfg.core.clocks_scale == 100))
			{
				const auto timebase_offs = m_ir->CreateLoad(get_type<u64>(), m_ir->CreateIntToPtr(m_ir->getInt64(reinterpret_cast<u64>(&g_timebase_offs)), get_type<u64*>()));
				const auto tsc = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_rdtsc));
				const auto tscx = m_ir->CreateMul(m_ir->CreateUDiv(tsc, m_ir->getInt64(utils::get_tsc_freq())), m_ir->getInt64(80000000));
				const auto tscm = m_ir->CreateUDiv(m_ir->CreateMul(m_ir->CreateURem(tsc, m_ir->getInt64(utils::get_tsc_freq())), m_ir->getInt64(80000000)), m_ir->getInt64(utils::get_tsc_freq()));
				const auto tsctb = m_ir->CreateSub(m_ir->CreateAdd(tscx, tscm), timebase_offs);
				m_ir->CreateStore(tsctb, spu_ptr(&spu_thread::ch_dec_start_timestamp));
			}
			else
#endif
			{
				m_ir->CreateStore(call("get_timebased_time", &get_timebased_time), spu_ptr(&spu_thread::ch_dec_start_timestamp));
			}

			m_ir->CreateStore(val.value, spu_ptr(&spu_thread::ch_dec_value));
			m_ir->CreateStore(m_ir->getInt8(0), spu_ptr(&spu_thread::is_dec_frozen));
			return;
		}
		case SPU_Set_Bkmk_Tag:
		case SPU_PM_Start_Ev:
		case SPU_PM_Stop_Ev:
		{
			return;
		}
		default: break;
		}

		update_pc();
		ensure_gpr_stores();
		call("spu_write_channel", &exec_wrch, m_thread, m_ir->getInt32(op.ra), val.value);
	}

	void LNOP(spu_opcode_t) //
	{
	}

	void NOP(spu_opcode_t) //
	{
	}

	void SYNC(spu_opcode_t) //
	{
		// This instruction must be used following a store instruction that modifies the instruction stream.
		m_ir->CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);

		if (g_cfg.core.spu_block_size == spu_block_size_type::safe && !m_interp_magn)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			update_pc(m_pos + 4);
			tail_chunk(m_dispatch);
		}
	}

	void DSYNC(spu_opcode_t) //
	{
		// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
		m_ir->CreateFence(llvm::AtomicOrdering::SequentiallyConsistent);
	}

	void MFSPR(spu_opcode_t op) //
	{
		// Check SPUInterpreter for notes.
		set_vr(op.rt, splat<u32[4]>(0));
	}

	void MTSPR(spu_opcode_t) //
	{
		// Check SPUInterpreter for notes.
	}

	template <typename TA, typename TB>
	auto mpyh(TA&& a, TB&& b)
	{
		return bitcast<u32[4]>(bitcast<u16[8]>((std::forward<TA>(a) >> 16)) * bitcast<u16[8]>(std::forward<TB>(b))) << 16;
	}

	template <typename TA, typename TB>
	auto mpyu(TA&& a, TB&& b)
	{
		return (std::forward<TA>(a) << 16 >> 16) * (std::forward<TB>(b) << 16 >> 16);
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
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
		set_vr(op.rt, zext<u32[4]>(a <= b));
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
		const auto [a, b] = get_vrs<u8[16]>(op.ra, op.rb);
		set_vr(op.rt, absd(a, b));
	}

	void ROT(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
		set_vr(op.rt, rol(a, b));
	}

	void ROTM(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);

		auto minusb = eval(-b);
		if (auto [ok, x] = match_expr(b, -match<u32[4]>()); ok)
		{
			minusb = eval(x);
		}

		if (auto k = get_known_bits(minusb); !!(k.Zero & 32))
		{
			set_vr(op.rt, a >> (minusb & 31));
			return;
		}

		set_vr(op.rt, inf_lshr(a, minusb & 63));
	}

	void ROTMA(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<s32[4]>(op.ra, op.rb);

		auto minusb = eval(-b);
		if (auto [ok, x] = match_expr(b, -match<s32[4]>()); ok)
		{
			minusb = eval(x);
		}

		if (auto k = get_known_bits(minusb); !!(k.Zero & 32))
		{
			set_vr(op.rt, a >> (minusb & 31));
			return;
		}

		set_vr(op.rt, inf_ashr(a, minusb & 63));
	}

	void SHL(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);

		if (auto k = get_known_bits(b); !!(k.Zero & 32))
		{
			set_vr(op.rt, a << (b & 31));
			return;
		}

		set_vr(op.rt, inf_shl(a, b & 63));
	}

	void ROTH(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u16[8]>(op.ra, op.rb);
		set_vr(op.rt, rol(a, b));
	}

	void ROTHM(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u16[8]>(op.ra, op.rb);

		auto minusb = eval(-b);
		if (auto [ok, x] = match_expr(b, -match<u16[8]>()); ok)
		{
			minusb = eval(x);
		}

		if (auto k = get_known_bits(minusb); !!(k.Zero & 16))
		{
			set_vr(op.rt, a >> (minusb & 15));
			return;
		}

		set_vr(op.rt, inf_lshr(a, minusb & 31));
	}

	void ROTMAH(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<s16[8]>(op.ra, op.rb);

		auto minusb = eval(-b);
		if (auto [ok, x] = match_expr(b, -match<s16[8]>()); ok)
		{
			minusb = eval(x);
		}

		if (auto k = get_known_bits(minusb); !!(k.Zero & 16))
		{
			set_vr(op.rt, a >> (minusb & 15));
			return;
		}

		set_vr(op.rt, inf_ashr(a, minusb & 31));
	}

	void SHLH(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u16[8]>(op.ra, op.rb);

		if (auto k = get_known_bits(b); !!(k.Zero & 16))
		{
			set_vr(op.rt, a << (b & 15));
			return;
		}

		set_vr(op.rt, inf_shl(a, b & 31));
	}

	void ROTI(spu_opcode_t op)
	{
		const auto a = get_vr<u32[4]>(op.ra);
		const auto i = get_imm<u32[4]>(op.i7, false);
		set_vr(op.rt, rol(a, i));
	}

	void ROTMI(spu_opcode_t op)
	{
		const auto a = get_vr<u32[4]>(op.ra);
		const auto i = get_imm<u32[4]>(op.i7, false);
		set_vr(op.rt, inf_lshr(a, -i & 63));
	}

	void ROTMAI(spu_opcode_t op)
	{
		const auto a = get_vr<s32[4]>(op.ra);
		const auto i = get_imm<s32[4]>(op.i7, false);
		set_vr(op.rt, inf_ashr(a, -i & 63));
	}

	void SHLI(spu_opcode_t op)
	{
		const auto a = get_vr<u32[4]>(op.ra);
		const auto i = get_imm<u32[4]>(op.i7, false);
		set_vr(op.rt, inf_shl(a, i & 63));
	}

	void ROTHI(spu_opcode_t op)
	{
		const auto a = get_vr<u16[8]>(op.ra);
		const auto i = get_imm<u16[8]>(op.i7, false);
		set_vr(op.rt, rol(a, i));
	}

	void ROTHMI(spu_opcode_t op)
	{
		const auto a = get_vr<u16[8]>(op.ra);
		const auto i = get_imm<u16[8]>(op.i7, false);
		set_vr(op.rt, inf_lshr(a, -i & 31));
	}

	void ROTMAHI(spu_opcode_t op)
	{
		const auto a = get_vr<s16[8]>(op.ra);
		const auto i = get_imm<s16[8]>(op.i7, false);
		set_vr(op.rt, inf_ashr(a, -i & 31));
	}

	void SHLHI(spu_opcode_t op)
	{
		const auto a = get_vr<u16[8]>(op.ra);
		const auto i = get_imm<u16[8]>(op.i7, false);
		set_vr(op.rt, inf_shl(a, i & 31));
	}

	void A(spu_opcode_t op)
	{
		if (auto [a, b] = match_vrs<u32[4]>(op.ra, op.rb); a && b)
		{
			static const auto MP = match<u32[4]>();

			if (auto [ok, a0, b0, b1, a1] = match_expr(a, mpyh(MP, MP) + mpyh(MP, MP)); ok)
			{
				if (auto [ok, a2, b2] = match_expr(b, mpyu(MP, MP)); ok && a2.eq(a0, a1) && b2.eq(b0, b1))
				{
					// 32-bit multiplication
					spu_log.notice("mpy32 in %s at 0x%05x", m_hash, m_pos);
					set_vr(op.rt, a0 * b0);
					return;
				}
			}
		}

		set_vr(op.rt, get_vr(op.ra) + get_vr(op.rb));
	}

	void AND(spu_opcode_t op)
	{
		if (match_vr<u8[16], u16[8], u64[2]>(op.ra, [&](auto a, auto /*MP1*/)
		{
			if (auto b = match_vr_as(a, op.rb))
			{
				set_vr(op.rt, a & b);
				return true;
			}

			return match_vr<u8[16], u16[8], u64[2]>(op.rb, [&](auto /*b*/, auto /*MP2*/)
			{
				set_vr(op.rt, a & get_vr_as(a, op.rb));
				return true;
			});
		}))
		{
			return;
		}

		set_vr(op.rt, get_vr(op.ra) & get_vr(op.rb));
	}

	void CG(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
		set_vr(op.rt, zext<u32[4]>(a + b < a));
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
		// GFNI trick to extract selected bit from bytes
		// By treating the first input as constant, and the second input as variable,
		// with only 1 bit set in our constant, gf2p8affineqb will extract that selected bit
		// from each byte of the second operand
		if (m_use_gfni)
		{
			const auto a = get_vr<u8[16]>(op.ra);
			const auto as = zshuffle(a, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 12, 8, 4, 0);
			set_vr(op.rt, gf2p8affineqb(build<u8[16]>(0x0, 0x0, 0x0, 0x0, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01, 0x0, 0x0, 0x0), as, 0x0));
			return;
		}

		const auto a = get_vr<s32[4]>(op.ra);
		const auto m = zext<u32>(bitcast<i4>(trunc<bool[4]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, eval(m)));
	}

	void GBH(spu_opcode_t op)
	{
		if (m_use_gfni)
		{
			const auto a = get_vr<u8[16]>(op.ra);
			const auto as = zshuffle(a, 16, 16, 16, 16, 16, 16, 16, 16, 14, 12, 10, 8, 6, 4, 2, 0);
			set_vr(op.rt, gf2p8affineqb(build<u8[16]>(0x0, 0x0, 0x0, 0x0, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01, 0x0, 0x0, 0x0), as, 0x0));
			return;
		}

		const auto a = get_vr<s16[8]>(op.ra);
		const auto m = zext<u32>(bitcast<u8>(trunc<bool[8]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, eval(m)));
	}

	void GBB(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);

		if (m_use_gfni)
		{
			const auto as = zshuffle(a, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8);
			const auto m = gf2p8affineqb(build<u8[16]>(0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0), as, 0x0);
			set_vr(op.rt, zshuffle(m, 16, 16, 16, 16, 16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10));
			return;
		}

		const auto m = zext<u32>(bitcast<u16>(trunc<bool[16]>(a)));
		set_vr(op.rt, insert(splat<u32[4]>(0), 3, eval(m)));
	}

	void FSM(spu_opcode_t op)
	{
		// FSM following a comparison instruction
		if (match_vr<s8[16], s16[8], s32[4], s64[2]>(op.ra, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
					set_vr(op.rt, (splat_scalar(c)));
					return true;
			}

			return false;
		}))
		{
			return;
		}

		const auto v = extract(get_vr(op.ra), 3);
		const auto m = bitcast<bool[4]>(trunc<i4>(v));
		set_vr(op.rt, sext<s32[4]>(m));
	}

	void FSMH(spu_opcode_t op)
	{
		const auto v = extract(get_vr(op.ra), 3);
		const auto m = bitcast<bool[8]>(trunc<u8>(v));
		set_vr(op.rt, sext<s16[8]>(m));
	}

	void FSMB(spu_opcode_t op)
	{
		const auto v = extract(get_vr(op.ra), 3);
		const auto m = bitcast<bool[16]>(trunc<u16>(v));
		set_vr(op.rt, sext<s8[16]>(m));
	}

	template <typename TA>
	static auto byteswap(TA&& a)
	{
		return zshuffle(std::forward<TA>(a), 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
	}

	template <typename T, typename U>
	static llvm_calli<u8[16], T, U> rotqbybi(T&& a, U&& b)
	{
		return {"spu_rotqbybi", {std::forward<T>(a), std::forward<U>(b)}};
	}

	void ROTQBYBI(spu_opcode_t op)
	{
		register_intrinsic("spu_rotqbybi", [&](llvm::CallInst* ci)
		{
			const auto a = value<u8[16]>(ci->getOperand(0));
			const auto b = value<u8[16]>(ci->getOperand(1));

			// Data with swapped endian from a load instruction
			if (auto [ok, as] = match_expr(a, byteswap(match<u8[16]>())); ok)
			{
				const auto sc = build<u8[16]>(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
				const auto sh = sc + (splat_scalar(b) >> 3);

				if (m_use_avx512_icl)
				{
					return eval(vpermb(as, sh));
				}

				return eval(pshufb(as, (sh & 0xf)));
			}
			const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
			const auto sh = sc - (splat_scalar(b) >> 3);

			if (m_use_avx512_icl)
			{
				return eval(vpermb(a, sh));
			}

			return eval(pshufb(a, (sh & 0xf)));
		});

		set_vr(op.rt, rotqbybi(get_vr<u8[16]>(op.ra), get_vr<u8[16]>(op.rb)));
	}

	void ROTQMBYBI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<s32[4]>(op.rb);

		auto minusb = eval(-(b >> 3));
		if (auto [ok, v0, v1] = match_expr(b, match<s32[4]>() - match<s32[4]>()); ok)
		{
			if (auto [ok1, data] = get_const_vector(v0.value, m_pos); ok1)
			{
				if (data == v128::from32p(7))
				{
					minusb = eval(v1 >> 3);
				}
			}
		}

		const auto minusbx = eval(bitcast<u8[16]>(minusb) & 0x1f);

		// Data with swapped endian from a load instruction
		if (auto [ok, as] = match_expr(a, byteswap(match<u8[16]>())); ok)
		{
			const auto sc = build<u8[16]>(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
			const auto sh = sc - splat_scalar(minusbx);
			set_vr(op.rt, pshufb(as, sh));
			return;
		}

		const auto sc = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		const auto sh = sc + splat_scalar(minusbx);
		set_vr(op.rt, pshufb(a, sh));
	}

	void SHLQBYBI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);

		// Data with swapped endian from a load instruction
		if (auto [ok, as] = match_expr(a, byteswap(match<u8[16]>())); ok)
		{
			const auto sc = build<u8[16]>(127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112);
			const auto sh = sc + (splat_scalar(b) >> 3);
			set_vr(op.rt, pshufb(as, sh));
			return;
		}

		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = sc - (splat_scalar(b) >> 3);
		set_vr(op.rt, pshufb(a, sh));
	}

	template <typename RT, typename T>
	auto spu_get_insertion_shuffle_mask(T&& index)
	{
		const auto c = bitcast<RT>(build<u8[16]>(0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10));
		using e_type = std::remove_extent_t<RT>;
		const auto v = splat<e_type>(static_cast<e_type>(sizeof(e_type) == 8 ? 0x01020304050607ull : 0x010203ull));
		return insert(c, std::forward<T>(index), v);
	}

	void CBX(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// Optimization with aligned stack assumption. Strange because SPU code could use CBD instead, but encountered in wild.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u8[16]>(~get_scalar(get_vr(op.rb)) & 0xf));
			return;
		}

		const auto s = get_scalar(get_vr(op.ra)) + get_scalar(get_vr(op.rb));
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u8[16]>(~s & 0xf));
	}

	void CHX(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBX.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u16[8]>(~get_scalar(get_vr(op.rb)) >> 1 & 0x7));
			return;
		}

		const auto s = get_scalar(get_vr(op.ra)) + get_scalar(get_vr(op.rb));
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u16[8]>(~s >> 1 & 0x7));
	}

	void CWX(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBX.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u32[4]>(~get_scalar(get_vr(op.rb)) >> 2 & 0x3));
			return;
		}

		const auto s = get_scalar(get_vr(op.ra)) + get_scalar(get_vr(op.rb));
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u32[4]>(~s >> 2 & 0x3));
	}

	void CDX(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBX.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u64[2]>(~get_scalar(get_vr(op.rb)) >> 3 & 0x1));
			return;
		}

		const auto s = get_scalar(get_vr(op.ra)) + get_scalar(get_vr(op.rb));
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u64[2]>(~s >> 3 & 0x1));
	}

	void ROTQBI(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto ax = get_vr<u8[16]>(op.ra);
		const auto bx = get_vr<u8[16]>(op.rb);

		// Combined bit and bytes shift
		if (auto [ok, v0, v1] = match_expr(ax, rotqbybi(match<u8[16]>(), match<u8[16]>())); ok && v1.eq(bx))
		{
			const auto b32 = get_vr<s32[4]>(op.rb);

			// Is the rotate less than 31 bits?
			if (auto k = get_known_bits(b32); (k.Zero & 0x60) == 0x60u)
			{
				const auto b = splat_scalar(get_vr(op.rb));
				set_vr(op.rt, fshl(bitcast<u32[4]>(v0), zshuffle(bitcast<u32[4]>(v0), 3, 0, 1, 2), b));
				return;
			}

			// Inverted shift count
			if (auto [ok1, v10, v11] = match_expr(b32, match<s32[4]>() - match<s32[4]>()); ok1)
			{
				if (auto [ok2, data] = get_const_vector(v10.value, m_pos); ok2)
				{
					if ((data & v128::from32p(0x7f)) == v128{})
					{
						if (auto k = get_known_bits(v11); (k.Zero & 0x60) == 0x60u)
						{
							set_vr(op.rt, fshr(zshuffle(bitcast<u32[4]>(v0), 1, 2, 3, 0), bitcast<u32[4]>(v0), splat_scalar(bitcast<u32[4]>(v11))));
							return;
						}
					}
				}
			}
		}

		const auto b = splat_scalar(get_vr(op.rb) & 0x7);
		set_vr(op.rt, fshl(a, zshuffle(a, 3, 0, 1, 2), b));
	}

	void ROTQMBI(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);

		auto minusb = eval(-b);
		if (auto [ok, x] = match_expr(b, -match<u32[4]>()); ok)
		{
			minusb = eval(x);
		}

		const auto bx = splat_scalar(minusb) & 0x7;
		set_vr(op.rt, fshr(zshuffle(a, 1, 2, 3, 4), a, bx));
	}

	void SHLQBI(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = splat_scalar(get_vr(op.rb) & 0x7);
		set_vr(op.rt, fshl(a, zshuffle(a, 4, 0, 1, 2), b));
	}

#if defined(ARCH_X64)
	static __m128i exec_rotqby(__m128i a, u8 b)
	{
		alignas(32) const __m128i buf[2]{a, a};
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + (16 - (b & 0xf))));
	}
#elif defined(ARCH_ARM64)
#else
#error "Unimplemented"
#endif

	void ROTQBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);

#if defined(ARCH_X64)
		if (!m_use_ssse3)
		{
			value_t<u8[16]> r;
			r.value = call<u8[16]>("spu_rotqby", &exec_rotqby, a.value, eval(extract(b, 12)).value);
			set_vr(op.rt, r);
			return;
		}
#endif

		// Data with swapped endian from a load instruction
		if (auto [ok, as] = match_expr(a, byteswap(match<u8[16]>())); ok)
		{
			const auto sc = build<u8[16]>(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
			const auto sh = eval(sc + splat_scalar(b));

			if (m_use_avx512_icl)
			{
				set_vr(op.rt, vpermb(as, sh));
				return;
			}

			set_vr(op.rt, pshufb(as, (sh & 0xf)));
			return;
		}

		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = eval(sc - splat_scalar(b));

		if (m_use_avx512_icl)
		{
			set_vr(op.rt, vpermb(a, sh));
			return;
		}

		set_vr(op.rt, pshufb(a, (sh & 0xf)));
	}

	void ROTQMBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u32[4]>(op.rb);

		auto minusb = eval(-b);
		if (auto [ok, x] = match_expr(b, -match<u32[4]>()); ok)
		{
			minusb = eval(x);
		}

		const auto minusbx = bitcast<u8[16]>(minusb);

		// Data with swapped endian from a load instruction
		if (auto [ok, as] = match_expr(a, byteswap(match<u8[16]>())); ok)
		{
			const auto sc = build<u8[16]>(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
			const auto sh = sc - (splat_scalar(minusbx) & 0x1f);
			set_vr(op.rt, pshufb(as, sh));
			return;
		}

		const auto sc = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		const auto sh = sc + (splat_scalar(minusbx) & 0x1f);
		set_vr(op.rt, pshufb(a, sh));
	}

	void SHLQBY(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);

		// Data with swapped endian from a load instruction
		if (auto [ok, as] = match_expr(a, byteswap(match<u8[16]>())); ok)
		{
			const auto sc = build<u8[16]>(127, 126, 125, 124, 123, 122, 121, 120, 119, 118, 117, 116, 115, 114, 113, 112);
			const auto sh = sc + (splat_scalar(b) & 0x1f);
			set_vr(op.rt, pshufb(as, sh));
			return;
		}

		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = sc - (splat_scalar(b) & 0x1f);
		set_vr(op.rt, pshufb(a, sh));
	}

	template <typename T>
	static llvm_calli<u32[4], T> orx(T&& a)
	{
		return {"spu_orx", {std::forward<T>(a)}};
	}

	void ORX(spu_opcode_t op)
	{
		register_intrinsic("spu_orx", [&](llvm::CallInst* ci)
		{
			const auto a = value<u32[4]>(ci->getOperand(0));
			const auto x = zshuffle(a, 2, 3, 0, 1) | a;
			const auto y = zshuffle(x, 1, 0, 3, 2) | x;
			return zshuffle(y, 4, 4, 4, 3);
		});

		set_vr(op.rt, orx(get_vr(op.ra)));
	}

	void CBD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// Known constant with aligned stack assumption (optimization).
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u8[16]>(~get_imm<u32>(op.i7) & 0xf));
			return;
		}

		const auto a = get_scalar(get_vr(op.ra)) + get_imm<u32>(op.i7);
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u8[16]>(~a & 0xf));
	}

	void CHD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBD.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u16[8]>(~get_imm<u32>(op.i7) >> 1 & 0x7));
			return;
		}

		const auto a = get_scalar(get_vr(op.ra)) + get_imm<u32>(op.i7);
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u16[8]>(~a >> 1 & 0x7));
	}

	void CWD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBD.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u32[4]>(~get_imm<u32>(op.i7) >> 2 & 0x3));
			return;
		}

		const auto a = get_scalar(get_vr(op.ra)) + get_imm<u32>(op.i7);
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u32[4]>(~a >> 2 & 0x3));
	}

	void CDD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn && op.ra == s_reg_sp)
		{
			// See CBD.
			set_vr(op.rt, spu_get_insertion_shuffle_mask<u64[2]>(~get_imm<u32>(op.i7) >> 3 & 0x1));
			return;
		}

		const auto a = get_scalar(get_vr(op.ra)) + get_imm<u32>(op.i7);
		set_vr(op.rt, spu_get_insertion_shuffle_mask<u64[2]>(~a >> 3 & 0x1));
	}

	void ROTQBII(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = eval(get_imm(op.i7, false) & 0x7);
		set_vr(op.rt, fshl(a, zshuffle(a, 3, 0, 1, 2), b));
	}

	void ROTQMBII(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = eval(-get_imm(op.i7, false) & 0x7);
		set_vr(op.rt, fshr(zshuffle(a, 1, 2, 3, 4), a, b));
	}

	void SHLQBII(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = eval(get_imm(op.i7, false) & 0x7);
		set_vr(op.rt, fshl(a, zshuffle(a, 4, 0, 1, 2), b));
	}

	void ROTQBYI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = (sc - get_imm<u8[16]>(op.i7, false)) & 0xf;
		set_vr(op.rt, pshufb(a, sh));
	}

	void ROTQMBYI(spu_opcode_t op)
	{
		const auto a = get_vr<u8[16]>(op.ra);
		const auto sc = build<u8[16]>(112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127);
		const auto sh = sc + (-get_imm<u8[16]>(op.i7, false) & 0x1f);
		set_vr(op.rt, pshufb(a, sh));
	}

	void SHLQBYI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.i7) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false); // For expressions matching
		const auto a = get_vr<u8[16]>(op.ra);
		const auto sc = build<u8[16]>(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
		const auto sh = sc - (get_imm<u8[16]>(op.i7, false) & 0x1f);
		set_vr(op.rt, pshufb(a, sh));
	}

	void CGT(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s32[4]>(get_vr<s32[4]>(op.ra) > get_vr<s32[4]>(op.rb)));
	}

	void XOR(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) ^ get_vr(op.rb));
	}

	void CGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<s16[8]>(op.ra) > get_vr<s16[8]>(op.rb)));
	}

	void EQV(spu_opcode_t op)
	{
		set_vr(op.rt, ~(get_vr(op.ra) ^ get_vr(op.rb)));
	}

	void CGTB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<s8[16]>(op.ra) > get_vr<s8[16]>(op.rb)));
	}

	void SUMB(spu_opcode_t op)
	{
		if (m_use_avx512)
		{
			const auto [a, b] = get_vrs<u8[16]>(op.ra, op.rb);
			const auto zeroes = splat<u8[16]>(0);

			if (op.ra == op.rb && !m_interp_magn)
			{
				set_vr(op.rt, vdbpsadbw(a, zeroes, 0));
				return;
			}

			const auto ax = vdbpsadbw(a, zeroes, 0);
			const auto bx = vdbpsadbw(b, zeroes, 0);
			set_vr(op.rt, shuffle2(ax, bx, 0, 9, 2, 11, 4, 13, 6, 15));
			return;
		}

		if (m_use_vnni)
		{
			const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
			const auto zeroes = splat<u32[4]>(0);
			const auto ones = splat<u32[4]>(0x01010101);
			const auto ax = bitcast<u16[8]>(vpdpbusd(zeroes, a, ones));
			const auto bx = bitcast<u16[8]>(vpdpbusd(zeroes, b, ones));
			set_vr(op.rt, shuffle2(ax, bx, 0, 8, 2, 10, 4, 12, 6, 14));
			return;
		}

		const auto [a, b] = get_vrs<u16[8]>(op.ra, op.rb);
		const auto ahs = eval((a >> 8) + (a & 0xff));
		const auto bhs = eval((b >> 8) + (b & 0xff));
		const auto lsh = shuffle2(ahs, bhs, 0, 9, 2, 11, 4, 13, 6, 15);
		const auto hsh = shuffle2(ahs, bhs, 1, 8, 3, 10, 5, 12, 7, 14);
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
		set_vr(op.rt, sext<s32[4]>(get_vr(op.ra) > get_vr(op.rb)));
	}

	void ANDC(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) & ~get_vr(op.rb));
	}

	void CLGTH(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<u16[8]>(op.ra) > get_vr<u16[8]>(op.rb)));
	}

	void ORC(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.ra) | ~get_vr(op.rb));
	}

	void CLGTB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<u8[16]>(op.ra) > get_vr<u8[16]>(op.rb)));
	}

	void CEQ(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s32[4]>(get_vr(op.ra) == get_vr(op.rb)));
	}

	void MPYHHU(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) >> 16) * (get_vr(op.rb) >> 16));
	}

	void ADDX(spu_opcode_t op)
	{
		set_vr(op.rt, llvm_sum{get_vr(op.ra), get_vr(op.rb), get_vr(op.rt) & 1});
	}

	void SFX(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rb) - get_vr(op.ra) - (~get_vr(op.rt) & 1));
	}

	void CGX(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
		const auto x = (get_vr<s32[4]>(op.rt) << 31) >> 31;
		const auto s = eval(a + b);
		set_vr(op.rt, noncast<u32[4]>(sext<s32[4]>(s < a) | (sext<s32[4]>(s == noncast<u32[4]>(x)) & x)) >> 31);
	}

	void BGX(spu_opcode_t op)
	{
		const auto [a, b] = get_vrs<u32[4]>(op.ra, op.rb);
		const auto c = get_vr<s32[4]>(op.rt) << 31;
		set_vr(op.rt, noncast<u32[4]>(sext<s32[4]>(b > a) | (sext<s32[4]>(a == b) & c)) >> 31);
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
		set_vr(op.rt, mpyh(get_vr(op.ra), get_vr(op.rb)));
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
		set_vr(op.rt, sext<s16[8]>(get_vr<u16[8]>(op.ra) == get_vr<u16[8]>(op.rb)));
	}

	void MPYU(spu_opcode_t op)
	{
		set_vr(op.rt, mpyu(get_vr(op.ra), get_vr(op.rb)));
	}

	void CEQB(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<u8[16]>(op.ra) == get_vr<u8[16]>(op.rb)));
	}

	void FSMBI(spu_opcode_t op)
	{
		const auto m = bitcast<bool[16]>(get_imm<u16>(op.i16));
		set_vr(op.rt, sext<s8[16]>(m));
	}

	void IL(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<s32[4]>(op.si16));
	}

	void ILHU(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<u32[4]>(op.i16) << 16);
	}

	void ILH(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<u16[8]>(op.i16));
	}

	void IOHL(spu_opcode_t op)
	{
		set_vr(op.rt, get_vr(op.rt) | get_imm(op.i16));
	}

	void ORI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.si10) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false); // For expressions matching
		set_vr(op.rt, get_vr<s32[4]>(op.ra) | get_imm<s32[4]>(op.si10));
	}

	void ORHI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.si10) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s16[8]>(op.ra) | get_imm<s16[8]>(op.si10));
	}

	void ORBI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.si10) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s8[16]>(op.ra) | get_imm<s8[16]>(op.si10));
	}

	void SFI(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<s32[4]>(op.si10) - get_vr<s32[4]>(op.ra));
	}

	void SFHI(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm<s16[8]>(op.si10) - get_vr<s16[8]>(op.ra));
	}

	void ANDI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && op.si10 == -1) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s32[4]>(op.ra) & get_imm<s32[4]>(op.si10));
	}

	void ANDHI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && op.si10 == -1) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s16[8]>(op.ra) & get_imm<s16[8]>(op.si10));
	}

	void ANDBI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && static_cast<s8>(op.si10) == -1) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s8[16]>(op.ra) & get_imm<s8[16]>(op.si10));
	}

	void AI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.si10) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s32[4]>(op.ra) + get_imm<s32[4]>(op.si10));
	}

	void AHI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.si10) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s16[8]>(op.ra) + get_imm<s16[8]>(op.si10));
	}

	void XORI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.si10) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s32[4]>(op.ra) ^ get_imm<s32[4]>(op.si10));
	}

	void XORHI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.si10) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s16[8]>(op.ra) ^ get_imm<s16[8]>(op.si10));
	}

	void XORBI(spu_opcode_t op)
	{
		if (get_reg_raw(op.ra) && !op.si10) return set_reg_fixed(op.rt, get_reg_raw(op.ra), false);
		set_vr(op.rt, get_vr<s8[16]>(op.ra) ^ get_imm<s8[16]>(op.si10));
	}

	void CGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s32[4]>(get_vr<s32[4]>(op.ra) > get_imm<s32[4]>(op.si10)));
	}

	void CGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<s16[8]>(op.ra) > get_imm<s16[8]>(op.si10)));
	}

	void CGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<s8[16]>(op.ra) > get_imm<s8[16]>(op.si10)));
	}

	void CLGTI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s32[4]>(get_vr(op.ra) > get_imm(op.si10)));
	}

	void CLGTHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<u16[8]>(op.ra) > get_imm<u16[8]>(op.si10)));
	}

	void CLGTBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<u8[16]>(op.ra) > get_imm<u8[16]>(op.si10)));
	}

	void MPYI(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr<s32[4]>(op.ra) << 16 >> 16) * get_imm<s32[4]>(op.si10));
	}

	void MPYUI(spu_opcode_t op)
	{
		set_vr(op.rt, (get_vr(op.ra) << 16 >> 16) * (get_imm(op.si10) & 0xffff));
	}

	void CEQI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s32[4]>(get_vr(op.ra) == get_imm(op.si10)));
	}

	void CEQHI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s16[8]>(get_vr<u16[8]>(op.ra) == get_imm<u16[8]>(op.si10)));
	}

	void CEQBI(spu_opcode_t op)
	{
		set_vr(op.rt, sext<s8[16]>(get_vr<u8[16]>(op.ra) == get_imm<u8[16]>(op.si10)));
	}

	void ILA(spu_opcode_t op)
	{
		set_vr(op.rt, get_imm(op.i18));
	}

	void SELB(spu_opcode_t op)
	{
		if (match_vr<s8[16], s16[8], s32[4], s64[2]>(op.rc, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			// If the control mask comes from a comparison instruction, replace SELB with select
			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				if constexpr (std::extent_v<VT> == 2) // u64[2]
				{
					// Try to select floats as floats if a OR b is typed as f64[2]
					if (auto [a, b] = match_vrs<f64[2]>(op.ra, op.rb); a || b)
					{
						set_vr(op.rt4, select(x, get_vr<f64[2]>(op.rb), get_vr<f64[2]>(op.ra)));
						return true;
					}
				}

				if constexpr (std::extent_v<VT> == 4) // u32[4]
				{
					// Match division (adjusted) (TODO)
					if (auto a = match_vr<f32[4]>(op.ra))
					{
						static const auto MT = match<f32[4]>();

						if (auto [div_ok, diva, divb] = match_expr(a, MT / MT); div_ok)
						{
							if (auto b = match_vr<s32[4]>(op.rb))
							{
								if (auto [add1_ok] = match_expr(b, bitcast<s32[4]>(a) + splat<s32[4]>(1)); add1_ok)
								{
									if (auto [fm_ok, a1, b1] = match_expr(x, bitcast<s32[4]>(fm(MT, MT)) > splat<s32[4]>(-1)); fm_ok)
									{
										if (auto [fnma_ok] = match_expr(a1, fnms(divb, bitcast<f32[4]>(b), diva)); fnma_ok)
										{
											if (fabs(b1).eval(m_ir) == fsplat<f32[4]>(1.0).eval(m_ir))
											{
												set_vr(op.rt4, diva / divb);
												return true;
											}

											if (auto [sel_ok] = match_expr(b1, bitcast<f32[4]>((bitcast<u32[4]>(diva) & 0x80000000) | 0x3f800000)); sel_ok)
											{
												set_vr(op.rt4, diva / divb);
												return true;
											}
										}
									}
								}
							}
						}
					}

					if (auto [a, b] = match_vrs<f64[4]>(op.ra, op.rb); a || b)
					{
						set_vr(op.rt4, select(x, get_vr<f64[4]>(op.rb), get_vr<f64[4]>(op.ra)));
						return true;
					}

					if (auto [a, b] = match_vrs<f32[4]>(op.ra, op.rb); a || b)
					{
						set_vr(op.rt4, select(x, get_vr<f32[4]>(op.rb), get_vr<f32[4]>(op.ra)));
						return true;
					}
				}

				if (auto [ok, y] = match_expr(x, bitcast<bool[std::extent_v<VT>]>(match<get_int_vt<std::extent_v<VT>>>())); ok)
				{
					// Don't ruin FSMB/FSM/FSMH instructions
					return false;
				}

				set_vr(op.rt4, select(x, get_vr<VT>(op.rb), get_vr<VT>(op.ra)));
				return true;
			}

			return false;
		}))
		{
			return;
		}

		const auto c = get_vr(op.rc);

		// Check if the constant mask doesn't require bit granularity
		if (auto [ok, mask] = get_const_vector(c.value, m_pos); ok)
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
				if (auto [a, b] = match_vrs<f64[4]>(op.ra, op.rb); a || b)
				{
					set_vr(op.rt4, select(noncast<s32[4]>(c) != 0,  get_vr<f64[4]>(op.rb), get_vr<f64[4]>(op.ra)));
					return;
				}
				else if (auto [a, b] = match_vrs<f32[4]>(op.ra, op.rb); a || b)
				{
					set_vr(op.rt4, select(noncast<s32[4]>(c) != 0,  get_vr<f32[4]>(op.rb), get_vr<f32[4]>(op.ra)));
					return;
				}

				set_vr(op.rt4, select(noncast<s32[4]>(c) != 0, get_vr<u32[4]>(op.rb), get_vr<u32[4]>(op.ra)));
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
				set_vr(op.rt4, select(bitcast<s16[8]>(c) != 0, get_vr<u16[8]>(op.rb), get_vr<u16[8]>(op.ra)));
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
				set_vr(op.rt4, select(bitcast<s8[16]>(c) != 0,get_vr<u8[16]>(op.rb), get_vr<u8[16]>(op.ra)));
				return;
			}
		}

		const auto op1 = get_reg_raw(op.rb);
		const auto op2 = get_reg_raw(op.ra);

		if ((op1 && op1->getType() == get_type<f64[4]>()) || (op2 && op2->getType() == get_type<f64[4]>()))
		{
			// Optimization: keep xfloat values in doubles even if the mask is unpredictable (hard way)
			const auto c = get_vr<u32[4]>(op.rc);
			const auto b = get_vr<f64[4]>(op.rb);
			const auto a = get_vr<f64[4]>(op.ra);
			const auto m = conv_xfloat_mask(c.value);
			const auto x = m_ir->CreateAnd(double_as_uint64(b.value), m);
			const auto y = m_ir->CreateAnd(double_as_uint64(a.value), m_ir->CreateNot(m));
			set_reg_fixed(op.rt4, uint64_as_double(m_ir->CreateOr(x, y)));
			return;
		}

		set_vr(op.rt4, (get_vr(op.rb) & c) | (get_vr(op.ra) & ~c));
	}

	void SHUFB(spu_opcode_t op) //
	{
		if (match_vr<u8[16], u16[8], u32[4], u64[2]>(op.rc, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			// If the mask comes from a constant generation instruction, replace SHUFB with insert
			if (auto [ok, i] = match_expr(c, spu_get_insertion_shuffle_mask<VT>(match<u32>())); ok)
			{
				set_vr(op.rt4, insert(get_vr<VT>(op.rb), i, get_scalar(get_vr<VT>(op.ra))));
				return true;
			}

			return false;
		}))
		{
			return;
		}

		const auto c = get_vr<u8[16]>(op.rc);

		if (auto [ok, mask] = get_const_vector(c.value, m_pos); ok)
		{
			// Optimization: SHUFB with constant mask
			if (((mask._u64[0] | mask._u64[1]) & 0xe0e0e0e0e0e0e0e0) == 0)
			{
				// Trivial insert or constant shuffle (TODO)
				static constexpr struct mask_info
				{
					u64 i1;
					u64 i0;
					decltype(&cpu_translator::get_type<void>) type;
					u64 extract_from;
					u64 insert_to;
				} s_masks[30]
				{
					{ 0x0311121314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 15 },
					{ 0x1003121314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 14 },
					{ 0x1011031314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 13 },
					{ 0x1011120314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 12 },
					{ 0x1011121303151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 11 },
					{ 0x1011121314031617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 10 },
					{ 0x1011121314150317, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 9 },
					{ 0x1011121314151603, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 8 },
					{ 0x1011121314151617, 0x03191a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 7 },
					{ 0x1011121314151617, 0x18031a1b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 6 },
					{ 0x1011121314151617, 0x1819031b1c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 5 },
					{ 0x1011121314151617, 0x18191a031c1d1e1f, &cpu_translator::get_type<u8[16]>, 12, 4 },
					{ 0x1011121314151617, 0x18191a1b031d1e1f, &cpu_translator::get_type<u8[16]>, 12, 3 },
					{ 0x1011121314151617, 0x18191a1b1c031e1f, &cpu_translator::get_type<u8[16]>, 12, 2 },
					{ 0x1011121314151617, 0x18191a1b1c1d031f, &cpu_translator::get_type<u8[16]>, 12, 1 },
					{ 0x1011121314151617, 0x18191a1b1c1d1e03, &cpu_translator::get_type<u8[16]>, 12, 0 },
					{ 0x0203121314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 7 },
					{ 0x1011020314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 6 },
					{ 0x1011121302031617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 5 },
					{ 0x1011121314150203, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 4 },
					{ 0x1011121314151617, 0x02031a1b1c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 3 },
					{ 0x1011121314151617, 0x181902031c1d1e1f, &cpu_translator::get_type<u16[8]>, 6, 2 },
					{ 0x1011121314151617, 0x18191a1b02031e1f, &cpu_translator::get_type<u16[8]>, 6, 1 },
					{ 0x1011121314151617, 0x18191a1b1c1d0203, &cpu_translator::get_type<u16[8]>, 6, 0 },
					{ 0x0001020314151617, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u32[4]>, 3, 3 },
					{ 0x1011121300010203, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u32[4]>, 3, 2 },
					{ 0x1011121314151617, 0x000102031c1d1e1f, &cpu_translator::get_type<u32[4]>, 3, 1 },
					{ 0x1011121314151617, 0x18191a1b00010203, &cpu_translator::get_type<u32[4]>, 3, 0 },
					{ 0x0001020304050607, 0x18191a1b1c1d1e1f, &cpu_translator::get_type<u64[2]>, 1, 1 },
					{ 0x1011121303151617, 0x0001020304050607, &cpu_translator::get_type<u64[2]>, 1, 0 },
				};

				// Check important constants from CWD-like constant generation instructions
				for (const auto& cm : s_masks)
				{
					if (mask._u64[0] == cm.i0 && mask._u64[1] == cm.i1)
					{
						const auto t = (this->*cm.type)();
						const auto a = get_reg_fixed(op.ra, t);
						const auto b = get_reg_fixed(op.rb, t);
						const auto e = m_ir->CreateExtractElement(a, cm.extract_from);
						set_reg_fixed(op.rt4, m_ir->CreateInsertElement(b, e, cm.insert_to));
						return;
					}
				}
			}

			// Adjusted shuffle mask
			v128 smask = ~mask & v128::from8p(op.ra == op.rb ? 0xf : 0x1f);

			// Blend mask for encoded constants
			v128 bmask{};

			for (u32 i = 0; i < 16; i++)
			{
				if (mask._bytes[i] >= 0xe0)
					bmask._bytes[i] = 0x80;
				else if (mask._bytes[i] >= 0xc0)
					bmask._bytes[i] = 0xff;
			}

			const auto a = get_vr<u8[16]>(op.ra);
			const auto b = get_vr<u8[16]>(op.rb);
			const auto c = make_const_vector(smask, get_type<u8[16]>());
			const auto d = make_const_vector(bmask, get_type<u8[16]>());

			llvm::Value* r = d;

			if ((~mask._u64[0] | ~mask._u64[1]) & 0x8080808080808080) [[likely]]
			{
				r = m_ir->CreateShuffleVector(b.value, op.ra == op.rb ? b.value : a.value, m_ir->CreateZExt(c, get_type<u32[16]>()));

				if ((mask._u64[0] | mask._u64[1]) & 0x8080808080808080)
				{
					r = m_ir->CreateSelect(m_ir->CreateICmpSLT(make_const_vector(mask, get_type<u8[16]>()), llvm::ConstantInt::get(get_type<u8[16]>(), 0)), d, r);
				}
			}

			set_reg_fixed(op.rt4, r);
			return;
		}

		// Check whether shuffle mask doesn't contain fixed value selectors
		bool perm_only = false;

		if (auto k = get_known_bits(c); !!(k.Zero & 0x80))
		{
			perm_only = true;
		}

		const auto a = get_vr<u8[16]>(op.ra);
		const auto b = get_vr<u8[16]>(op.rb);

		// Data with swapped endian from a load instruction
		if (auto [ok, as] = match_expr(a, byteswap(match<u8[16]>())); ok)
		{
			if (auto [ok, bs] = match_expr(b, byteswap(match<u8[16]>())); ok)
			{
				// Undo endian swapping, and rely on pshufb/vperm2b to re-reverse endianness
				if (m_use_avx512_icl && (op.ra != op.rb))
				{
					if (perm_only)
					{
						set_vr(op.rt4, vperm2b(as, bs, c));
						return;
					}

					const auto m = gf2p8affineqb(c, build<u8[16]>(0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20), 0x7f);
					const auto mm = select(noncast<s8[16]>(m) >= 0, splat<u8[16]>(0), m);
					const auto ab = vperm2b(as, bs, c);
					set_vr(op.rt4, select(noncast<s8[16]>(c) >= 0, ab, mm));
					return;
				}

				const auto x = pshufb(build<u8[16]>(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x80), (c >> 4));
				const auto ax = pshufb(as, c);
				const auto bx = pshufb(bs, c);

				if (perm_only)
					set_vr(op.rt4, select_by_bit4(c, ax, bx));
				else
					set_vr(op.rt4, select_by_bit4(c, ax, bx) | x);
				return;
			}

			if (auto [ok, data] = get_const_vector(b.value, m_pos); ok)
			{
				if (data == v128::from8p(data._u8[0]))
				{
					if (m_use_avx512_icl)
					{
						if (perm_only)
						{
							set_vr(op.rt4, vperm2b(as, b, c));
							return;
						}

						const auto m = gf2p8affineqb(c, build<u8[16]>(0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20), 0x7f);
						const auto mm = select(noncast<s8[16]>(m) >= 0, splat<u8[16]>(0), m);
						const auto ab = vperm2b(as, b, c);
						set_vr(op.rt4, select(noncast<s8[16]>(c) >= 0, ab, mm));
						return;
					}
					// See above
					const auto x = pshufb(build<u8[16]>(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x80), (c >> 4));
					const auto ax = pshufb(as, c);

					if (perm_only)
						set_vr(op.rt4, select_by_bit4(c, ax, b));
					else
						set_vr(op.rt4, select_by_bit4(c, ax, b) | x);
					return;
				}
			}
		}

		if (auto [ok, bs] = match_expr(b, byteswap(match<u8[16]>())); ok)
		{
			if (auto [ok, data] = get_const_vector(a.value, m_pos); ok)
			{
				if (data == v128::from8p(data._u8[0]))
				{
					// See above
					const auto x = pshufb(build<u8[16]>(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x80), (c >> 4));
					const auto bx = pshufb(bs, c);

					if (perm_only)
						set_vr(op.rt4, select_by_bit4(c, a, bx));
					else
						set_vr(op.rt4, select_by_bit4(c, a, bx) | x);
					return;
				}
			}
		}

		if (m_use_avx512_icl && (op.ra != op.rb || m_interp_magn))
		{
			if (auto [ok, data] = get_const_vector(b.value, m_pos); ok)
			{
				if (data == v128::from8p(data._u8[0]))
				{
					if (perm_only)
					{
						set_vr(op.rt4, vperm2b(a, b, eval(c ^ 0xf)));
						return;
					}

					const auto m = gf2p8affineqb(c, build<u8[16]>(0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20), 0x7f);
					const auto mm = select(noncast<s8[16]>(m) >= 0, splat<u8[16]>(0), m);
					const auto ab = vperm2b(a, b, eval(c ^ 0xf));
					set_vr(op.rt4, select(noncast<s8[16]>(c) >= 0, ab, mm));
					return;
				}
			}

			if (auto [ok, data] = get_const_vector(a.value, m_pos); ok)
			{
				if (data == v128::from8p(data._u8[0]))
				{
					if (perm_only)
					{
						set_vr(op.rt4, vperm2b(b, a, eval(c ^ 0x1f)));
						return;
					}

					const auto m = gf2p8affineqb(c, build<u8[16]>(0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20), 0x7f);
					const auto mm = select(noncast<s8[16]>(m) >= 0, splat<u8[16]>(0), m);
					const auto ab = vperm2b(b, a, eval(c ^ 0x1f));
					set_vr(op.rt4, select(noncast<s8[16]>(c) >= 0, ab, mm));
					return;
				}
			}

			if (perm_only)
			{
				set_vr(op.rt4, vperm2b(a, b, eval(c ^ 0xf)));
				return;
			}

			const auto m = gf2p8affineqb(c, build<u8[16]>(0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x40, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20), 0x7f);
			const auto mm = select(noncast<s8[16]>(m) >= 0, splat<u8[16]>(0), m);
			const auto cr = eval(c ^ 0xf);
			const auto ab = vperm2b(a, b, cr);
			set_vr(op.rt4, select(noncast<s8[16]>(c) >= 0, ab, mm));
			return;
		}

		const auto x = pshufb(build<u8[16]>(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x80, 0x80), (c >> 4));
		const auto cr = eval(c ^ 0xf);
		const auto ax = pshufb(a, cr);
		const auto bx = pshufb(b, cr);

		if (perm_only)
			set_vr(op.rt4, select_by_bit4(cr, ax, bx));
		else
			set_vr(op.rt4, select_by_bit4(cr, ax, bx) | x);
	}

	void MPYA(spu_opcode_t op)
	{
		set_vr(op.rt4, (get_vr<s32[4]>(op.ra) << 16 >> 16) * (get_vr<s32[4]>(op.rb) << 16 >> 16) + get_vr<s32[4]>(op.rc));
	}

	void FSCRRD(spu_opcode_t op) //
	{
		// Hack
		set_vr(op.rt, splat<u32[4]>(0));
	}

	void FSCRWR(spu_opcode_t /*op*/) //
	{
		// Hack
	}

	void DFCGT(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFCEQ(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFCMGT(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFCMEQ(spu_opcode_t op) //
	{
		return UNK(op);
	}

	void DFTSV(spu_opcode_t op) //
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
		const auto [a, b, c] = get_vrs<f64[2]>(op.ra, op.rb, op.rt);

		if (g_cfg.core.use_accurate_dfma)
			set_vr(op.rt, fmuladd(a, b, c, true));
		else
			set_vr(op.rt, a * b + c);
	}

	void DFMS(spu_opcode_t op)
	{
		const auto [a, b, c] = get_vrs<f64[2]>(op.ra, op.rb, op.rt);

		if (g_cfg.core.use_accurate_dfma)
			set_vr(op.rt, fmuladd(a, b, -c, true));
		else
			set_vr(op.rt, a * b - c);
	}

	void DFNMS(spu_opcode_t op)
	{
		const auto [a, b, c] = get_vrs<f64[2]>(op.ra, op.rb, op.rt);

		if (g_cfg.core.use_accurate_dfma)
			set_vr(op.rt, fmuladd(-a, b, c, true));
		else
			set_vr(op.rt, c - (a * b));
	}

	void DFNMA(spu_opcode_t op)
	{
		const auto [a, b, c] = get_vrs<f64[2]>(op.ra, op.rb, op.rt);

		if (g_cfg.core.use_accurate_dfma)
			set_vr(op.rt, -fmuladd(a, b, c, true));
		else
			set_vr(op.rt, -(a * b + c));
	}

	bool is_input_positive(value_t<f32[4]> a)
	{
		if (auto [ok, v0, v1] = match_expr(a, match<f32[4]>() * match<f32[4]>()); ok && v0.eq(v1))
		{
			return true;
		}

		return false;
	}

	// clamping helpers
	value_t<f32[4]> clamp_positive_smax(value_t<f32[4]> v)
	{
		return eval(bitcast<f32[4]>(min(bitcast<s32[4]>(v),splat<s32[4]>(0x7f7fffff))));
	}

	value_t<f32[4]> clamp_negative_smax(value_t<f32[4]> v)
	{
		if (is_input_positive(v))
		{
			return v;
		}

		return eval(bitcast<f32[4]>(min(bitcast<u32[4]>(v),splat<u32[4]>(0xff7fffff))));
	}

	value_t<f32[4]> clamp_smax(value_t<f32[4]> v)
	{
		if (m_use_avx512)
		{
			if (is_input_positive(v))
			{
				return eval(clamp_positive_smax(v));
			}

			if (auto [ok, data] = get_const_vector(v.value, m_pos); ok)
			{
				// Avoid pessimation when input is constant
				return eval(clamp_positive_smax(clamp_negative_smax(v)));
			}

			return eval(vrangeps(v, fsplat<f32[4]>(std::bit_cast<f32, u32>(0x7f7fffff)), 0x2, 0xff));
		}

		return eval(clamp_positive_smax(clamp_negative_smax(v)));
	}

	// FMA favouring zeros
	value_t<f32[4]> xmuladd(value_t<f32[4]> a, value_t<f32[4]> b, value_t<f32[4]> c)
	{
		const auto ma = eval(sext<s32[4]>(fcmp_uno(a != fsplat<f32[4]>(0.))));
		const auto mb = eval(sext<s32[4]>(fcmp_uno(b != fsplat<f32[4]>(0.))));
		const auto ca = eval(bitcast<f32[4]>(bitcast<s32[4]>(a) & mb));
		const auto cb = eval(bitcast<f32[4]>(bitcast<s32[4]>(b) & ma));
		return eval(fmuladd(ca, cb, c));
	}

	// Checks for postive and negative zero, or Denormal (treated as zero)
	// If sign is +-1 check equality againts all sign bits
	bool is_spu_float_zero(v128 a, int sign = 0)
	{
		for (u32 i = 0; i < 4; i++)
		{
			const u32 exponent = a._u32[i] & 0x7f800000u;

			if (exponent || (sign && (sign >= 0) != (a._s32[i] >= 0)))
			{
				// Normalized number
				return false;
			}
		}
		return true;
	}

	template <typename T>
	static llvm_calli<f32[4], T> frest(T&& a)
	{
		return {"spu_frest", {std::forward<T>(a)}};
	}

	void FREST(spu_opcode_t op)
	{
		register_intrinsic("spu_frest", [&](llvm::CallInst* ci)
		{
			const auto a = bitcast<u32[4]>(value<f32[4]>(ci->getOperand(0)));

			const auto a_fraction = (a >> splat<u32[4]>(18)) & splat<u32[4]>(0x1F);
			const auto a_exponent = (a & splat<u32[4]>(0x7F800000u));
			const auto r_exponent = sub_sat(build<u16[8]>(0000, 0x7E80, 0000, 0x7E80, 0000, 0x7E80, 0000, 0x7E80), bitcast<u16[8]>(a_exponent));
			const auto fix_exponent = select((a_exponent > 0), bitcast<u32[4]>(r_exponent), splat<u32[4]>(0x7F800000u));
			const auto a_sign = (a & splat<u32[4]>(0x80000000));
			value_t<u32[4]> final_result = eval(splat<u32[4]>(0));

			for (u32 i = 0; i < 4; i++)
			{
				const auto eval_fraction = eval(extract(a_fraction, i));

				value_t<u32> r_fraction = load_const<u32>(m_spu_frest_fraction_lut, eval_fraction);

				final_result = eval(insert(final_result, i, r_fraction));
			}

			//final_result = eval(select(final_result != (0), final_result, bitcast<u32[4]>(pshufb(splat<u8[16]>(0), bitcast<u8[16]>(final_result)))));

			return bitcast<f32[4]>(bitcast<u32[4]>(final_result | bitcast<u32[4]>(fix_exponent) | a_sign));
		});

		set_vr(op.rt, frest(get_vr<f32[4]>(op.ra)));
	}

	template <typename T>
	static llvm_calli<f32[4], T> frsqest(T&& a)
	{
		return {"spu_frsqest", {std::forward<T>(a)}};
	}

	void FRSQEST(spu_opcode_t op)
	{
		register_intrinsic("spu_frsqest", [&](llvm::CallInst* ci)
		{
			const auto a = bitcast<u32[4]>(value<f32[4]>(ci->getOperand(0)));

			const auto a_fraction = (a >> splat<u32[4]>(18)) & splat<u32[4]>(0x3F);
			const auto a_exponent = (a >> splat<u32[4]>(23)) & splat<u32[4]>(0xFF);
			value_t<u32[4]> final_result = eval(splat<u32[4]>(0));

			for (u32 i = 0; i < 4; i++)
			{
				const auto eval_fraction = eval(extract(a_fraction, i));
				const auto eval_exponent = eval(extract(a_exponent, i));

				value_t<u32> r_fraction = load_const<u32>(m_spu_frsqest_fraction_lut, eval_fraction);
				value_t<u32> r_exponent = load_const<u32>(m_spu_frsqest_exponent_lut, eval_exponent);

				final_result = eval(insert(final_result, i, eval(r_fraction | r_exponent)));
			}

			return bitcast<f32[4]>(final_result);
		});

		set_vr(op.rt, frsqest(get_vr<f32[4]>(op.ra)));
	}

	template <typename T, typename U>
	static llvm_calli<s32[4], T, U> fcgt(T&& a, U&& b)
	{
		return {"spu_fcgt", {std::forward<T>(a), std::forward<U>(b)}};
	}

	void FCGT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(get_vr<f64[4]>(op.ra) > get_vr<f64[4]>(op.rb))));
			return;
		}

		register_intrinsic("spu_fcgt", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));

			const value_t<f32[4]> ab[2]{a, b};

			std::bitset<2> safe_int_compare(0);
			std::bitset<2> safe_finite_compare(0);

			for (u32 i = 0; i < 2; i++)
			{
				if (auto [ok, data] = get_const_vector(ab[i].value, m_pos, __LINE__ + i); ok)
				{
					safe_int_compare.set(i);
					safe_finite_compare.set(i);

					for (u32 j = 0; j < 4; j++)
					{
						const u32 value = data._u32[j];
						const u8 exponent = static_cast<u8>(value >> 23);

						if (value >= 0x7f7fffffu || !exponent)
						{
							// Postive or negative zero, Denormal (treated as zero), Negative constant, or Normalized number with exponent +127
							// Cannot used signed integer compare safely
							// Note: Technically this optimization is accurate for any positive value, but due to the fact that
							// we don't produce "extended range" values the same way as real hardware, it's not safe to apply
							// this optimization for values outside of the range of x86 floating point hardware.
							safe_int_compare.reset(i);
							if ((value & 0x7fffffffu) >= 0x7f7ffffeu) safe_finite_compare.reset(i);
						}
					}
				}
			}

			if (safe_int_compare.any())
			{
				return eval(sext<s32[4]>(bitcast<s32[4]>(a) > bitcast<s32[4]>(b)));
			}

			if  (safe_finite_compare.test(1))
			{
				return eval(sext<s32[4]>(fcmp_uno(clamp_negative_smax(a) > b)));
			}

			if  (safe_finite_compare.test(0))
			{
				return eval(sext<s32[4]>(fcmp_ord(a > clamp_smax(b))));
			}

			const auto ai = eval(bitcast<s32[4]>(a));
			const auto bi = eval(bitcast<s32[4]>(b));

			return eval(sext<s32[4]>(fcmp_uno(a != b) & select((ai & bi) >= 0, ai > bi, ai < bi)));
		});

		set_vr(op.rt, fcgt(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)));
	}

	template <typename T, typename U>
	static llvm_calli<s32[4], T, U> fcmgt(T&& a, U&& b)
	{
		return {"spu_fcmgt", {std::forward<T>(a), std::forward<U>(b)}};
	}

	void FCMGT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(fabs(get_vr<f64[4]>(op.ra)) > fabs(get_vr<f64[4]>(op.rb)))));
			return;
		}

		register_intrinsic("spu_fcmgt", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));

			const value_t<f32[4]> ab[2]{a, b};

			std::bitset<2> safe_int_compare(0);

			for (u32 i = 0; i < 2; i++)
			{
				if (auto [ok, data] = get_const_vector(ab[i].value, m_pos, __LINE__ + i); ok)
				{
					safe_int_compare.set(i);

					for (u32 j = 0; j < 4; j++)
					{
						const u32 value = data._u32[j];
						const u8 exponent = static_cast<u8>(value >> 23);

						if ((value & 0x7fffffffu) >= 0x7f7fffffu || !exponent)
						{
							// See above
							safe_int_compare.reset(i);
						}
					}
				}
			}

			const auto ma = eval(fabs(a));
			const auto mb = eval(fabs(b));

			const auto mai = eval(bitcast<s32[4]>(ma));
			const auto mbi = eval(bitcast<s32[4]>(mb));

			if (safe_int_compare.any())
			{
				return eval(sext<s32[4]>(mai > mbi));
			}

			if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::approximate)
			{
				return eval(sext<s32[4]>(fcmp_uno(ma > mb) & (mai > mbi)));
			}
			else
			{
				return eval(sext<s32[4]>(fcmp_ord(ma > mb)));
			}
		});

		set_vr(op.rt, fcmgt(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)));
	}

	template <typename T, typename U>
	static llvm_calli<f32[4], T, U> fa(T&& a, U&& b)
	{
		return {"spu_fa", {std::forward<T>(a), std::forward<U>(b)}};
	}

	void FA(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			set_vr(op.rt, get_vr<f64[4]>(op.ra) + get_vr<f64[4]>(op.rb));
			return;
		}

		register_intrinsic("spu_fa", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));

			return a + b;
		});

		set_vr(op.rt, fa(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)));
	}

	template <typename T, typename U>
	static llvm_calli<f32[4], T, U> fs(T&& a, U&& b)
	{
		return {"spu_fs", {std::forward<T>(a), std::forward<U>(b)}};
	}

	void FS(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			set_vr(op.rt, get_vr<f64[4]>(op.ra) - get_vr<f64[4]>(op.rb));
			return;
		}

		register_intrinsic("spu_fs", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));

			if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::approximate)
			{
				const auto bc = clamp_smax(b); // for #4478
				return eval(a - bc);
			}
			else
			{
				return eval(a - b);
			}
		});

		set_vr(op.rt, fs(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)));
	}

	template <typename T, typename U>
	static llvm_calli<f32[4], T, U> fm(T&& a, U&& b)
	{
		return llvm_calli<f32[4], T, U>{"spu_fm", {std::forward<T>(a), std::forward<U>(b)}}.set_order_equality_hint(1, 1);
	}

	void FM(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			set_vr(op.rt, get_vr<f64[4]>(op.ra) * get_vr<f64[4]>(op.rb));
			return;
		}

		register_intrinsic("spu_fm", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));

			if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::approximate)
			{
				if (a.value == b.value)
				{
					return eval(a * b);
				}

				const auto ma = sext<s32[4]>(fcmp_uno(a != fsplat<f32[4]>(0.)));
				const auto mb = sext<s32[4]>(fcmp_uno(b != fsplat<f32[4]>(0.)));
				return eval(bitcast<f32[4]>(bitcast<s32[4]>(a * b) & ma & mb));
			}
			else
			{
				return eval(a * b);
			}
		});

		if (op.ra == op.rb && !m_interp_magn)
		{
			const auto a = get_vr<f32[4]>(op.ra);
			set_vr(op.rt, fm(a, a));
			return;
		}

		const auto [a, b] = get_vrs<f32[4]>(op.ra, op.rb);

		// This causes issues in LBP 1(first platform on first temple level doesn't come down when grabbed)
		// Presumably 1/x might result in Zero/NaN when a/x doesn't
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::relaxed)
		{
			auto full_fm_accurate = [&](const auto& a, const auto& div)
			{
				const auto div_result = a / div;
				const auto result_and = bitcast<u32[4]>(div_result) & 0x7fffffffu;
				const auto result_cmp_inf = sext<s32[4]>(result_and == splat<u32[4]>(0x7F800000u));
				const auto result_cmp_nan = sext<s32[4]>(result_and <= splat<u32[4]>(0x7F800000u));

				const auto and_mask = bitcast<u32[4]>(result_cmp_nan) & splat<u32[4]>(0xFFFFFFFFu);
				const auto or_mask = bitcast<u32[4]>(result_cmp_inf) & splat<u32[4]>(0xFFFFFFFu);
				set_vr(op.rt, bitcast<f32[4]>((bitcast<u32[4]>(div_result) & and_mask) | or_mask));
			};

			// FM(a, re_accurate(div))
			if (const auto [ok_re_acc, div, one] = match_expr(b, re_accurate(match<f32[4]>(), match<f32[4]>())); ok_re_acc)
			{
				full_fm_accurate(a, div);
				erase_stores(one, b);
				return;
			}

			// FM(re_accurate(div), b)
			if (const auto [ok_re_acc, div, one] = match_expr(a, re_accurate(match<f32[4]>(), match<f32[4]>())); ok_re_acc)
			{
				full_fm_accurate(b, div);
				erase_stores(one, a);
				return;
			}
		}

		set_vr(op.rt, fm(a, b));
	}

	template <typename T>
	static llvm_calli<f64[2], T> fesd(T&& a)
	{
		return {"spu_fesd", {std::forward<T>(a)}};
	}

	void FESD(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			const auto r = zshuffle(get_vr<f64[4]>(op.ra), 1, 3);
			const auto d = bitcast<s64[2]>(r);
			const auto a = eval(d & 0x7fffffffffffffff);
			const auto s = eval(d & 0x8000000000000000);
			const auto i = select(a == 0x47f0000000000000, eval(s | 0x7ff0000000000000), d);
			const auto n = select(a > 0x47f0000000000000, splat<s64[2]>(0x7ff8000000000000), i);
			set_vr(op.rt, bitcast<f64[2]>(n));
			return;
		}

		register_intrinsic("spu_fesd", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));

			return fpcast<f64[2]>(zshuffle(a, 1, 3));
		});

		set_vr(op.rt, fesd(get_vr<f32[4]>(op.ra)));
	}

	template <typename T>
	static llvm_calli<f32[4], T> frds(T&& a)
	{
		return {"spu_frds", {std::forward<T>(a)}};
	}

	void FRDS(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			const auto r = get_vr<f64[2]>(op.ra);
			const auto d = bitcast<s64[2]>(r);
			const auto a = eval(d & 0x7fffffffffffffff);
			const auto s = eval(d & 0x8000000000000000);
			const auto i = select(a > 0x47f0000000000000, eval(s | 0x47f0000000000000), d);
			const auto n = select(a > 0x7ff0000000000000, splat<s64[2]>(0x47f8000000000000), i);
			const auto z = select(a < 0x3810000000000000, s, n);
			set_vr(op.rt, zshuffle(bitcast<f64[2]>(z), 2, 0, 3, 1), nullptr, false);
			return;
		}

		register_intrinsic("spu_frds", [&](llvm::CallInst* ci)
		{
			const auto a = value<f64[2]>(ci->getOperand(0));

			return zshuffle(fpcast<f32[2]>(a), 2, 0, 3, 1);
		});

		set_vr(op.rt, frds(get_vr<f64[2]>(op.ra)));
	}

	template <typename T, typename U>
	static llvm_calli<s32[4], T, U> fceq(T&& a, U&& b)
	{
		return {"spu_fceq", {std::forward<T>(a), std::forward<U>(b)}};
	}

	void FCEQ(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(get_vr<f64[4]>(op.ra) == get_vr<f64[4]>(op.rb))));
			return;
		}

		register_intrinsic("spu_fceq", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));

			const value_t<f32[4]> ab[2]{a, b};

			std::bitset<2> safe_float_compare(0);
			std::bitset<2> safe_int_compare(0);

			for (u32 i = 0; i < 2; i++)
			{
				if (auto [ok, data] = get_const_vector(ab[i].value, m_pos, __LINE__ + i); ok)
				{
					safe_float_compare.set(i);
					safe_int_compare.set(i);

					for (u32 j = 0; j < 4; j++)
					{
						const u32 value = data._u32[j];
						const u8 exponent = static_cast<u8>(value >> 23);

						// unsafe if nan
						if (exponent == 255)
						{
							safe_float_compare.reset(i);
						}

						// unsafe if denormal or 0
						if (!exponent)
						{
							safe_int_compare.reset(i);
						}
					}
				}
			}

			if (safe_float_compare.any())
			{
				return eval(sext<s32[4]>(fcmp_ord(a == b)));
			}

			if (safe_int_compare.any())
			{
				return eval(sext<s32[4]>(bitcast<s32[4]>(a) == bitcast<s32[4]>(b)));
			}

			if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::approximate)
			{
				return eval(sext<s32[4]>(fcmp_ord(a == b)) | sext<s32[4]>(bitcast<s32[4]>(a) == bitcast<s32[4]>(b)));
			}
			else
			{
				return eval(sext<s32[4]>(fcmp_ord(a == b)));
			}
		});

		set_vr(op.rt, fceq(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)));
	}

	template <typename T, typename U>
	static llvm_calli<s32[4], T, U> fcmeq(T&& a, U&& b)
	{
		return {"spu_fcmeq", {std::forward<T>(a), std::forward<U>(b)}};
	}

	void FCMEQ(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			set_vr(op.rt, sext<s32[4]>(fcmp_ord(fabs(get_vr<f64[4]>(op.ra)) == fabs(get_vr<f64[4]>(op.rb)))));
			return;
		}

		register_intrinsic("spu_fcmeq", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));

			const value_t<f32[4]> ab[2]{a, b};

			std::bitset<2> safe_float_compare(0);
			std::bitset<2> safe_int_compare(0);

			for (u32 i = 0; i < 2; i++)
			{
				if (auto [ok, data] = get_const_vector(ab[i].value, m_pos, __LINE__ + i); ok)
				{
					safe_float_compare.set(i);
					safe_int_compare.set(i);

					for (u32 j = 0; j < 4; j++)
					{
						const u32 value = data._u32[j];
						const u8 exponent = static_cast<u8>(value >> 23);

						// unsafe if nan
						if (exponent == 255)
						{
							safe_float_compare.reset(i);
						}

						// unsafe if denormal or 0
						if (!exponent)
						{
							safe_int_compare.reset(i);
						}
					}
				}
			}

			const auto fa = eval(fabs(a));
			const auto fb = eval(fabs(b));

			if (safe_float_compare.any())
			{
				return eval(sext<s32[4]>(fcmp_ord(fa == fb)));
			}

			if (safe_int_compare.any())
			{
				return eval(sext<s32[4]>(bitcast<s32[4]>(fa) == bitcast<s32[4]>(fb)));
			}

			if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::approximate)
			{
				return eval(sext<s32[4]>(fcmp_ord(fa == fb)) | sext<s32[4]>(bitcast<s32[4]>(fa) == bitcast<s32[4]>(fb)));
			}
			else
			{
				return eval(sext<s32[4]>(fcmp_ord(fa == fb)));
			}
		});

		set_vr(op.rt, fcmeq(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb)));
	}

	value_t<f32[4]> fma32x4(value_t<f32[4]> a, value_t<f32[4]> b, value_t<f32[4]> c)
	{
		// Optimization: Emit only a floating multiply if the addend is zero
		// This is odd since SPU code could just use the FM instruction, but it seems common enough
		if (auto [ok, data] = get_const_vector(c.value, m_pos); ok)
		{
			if (is_spu_float_zero(data, 0))
			{
				return eval(a * b);
			}
		}

		if ([&]()
		{
			if (auto [ok, data] = get_const_vector(a.value, m_pos); ok)
			{
				if (is_spu_float_zero(data, 0))
				{
					return true;
				}
			}

			if (auto [ok, data] = get_const_vector(b.value, m_pos); ok)
			{
				if (is_spu_float_zero(data, 0))
				{
					return true;
				}
			}

			return false;
		}())
		{
			// Just return the added value if either a or b are +-0
			return c;
		}

		if (m_use_fma)
		{
			return eval(fmuladd(a, b, c, true));
		}

		// Convert to doubles
		const auto xa = fpcast<f64[4]>(a);
		const auto xb = fpcast<f64[4]>(b);
		const auto xc = fpcast<f64[4]>(c);
		const auto xr = fmuladd(xa, xb, xc, false);
		return eval(fpcast<f32[4]>(xr));
	}

	template <typename T, typename U, typename V>
	static llvm_calli<f32[4], T, U, V> fnms(T&& a, U&& b, V&& c)
	{
		return llvm_calli<f32[4], T, U, V>{"spu_fnms", {std::forward<T>(a), std::forward<U>(b), std::forward<V>(c)}}.set_order_equality_hint(1, 1, 0);
	}

	void FNMS(spu_opcode_t op)
	{
		// See FMA.
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			const auto [a, b, c] = get_vrs<f64[4]>(op.ra, op.rb, op.rc);
			set_vr(op.rt4, fmuladd(-a, b, c));
			return;
		}

		register_intrinsic("spu_fnms", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));
			const auto c = value<f32[4]>(ci->getOperand(2));

			return fma32x4(eval(-clamp_smax(a)), clamp_smax(b), c);
		});

		set_vr(op.rt4, fnms(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb), get_vr<f32[4]>(op.rc)));
	}

	template <typename T, typename U, typename V>
	static llvm_calli<f32[4], T, U, V> fma(T&& a, U&& b, V&& c)
	{
		return llvm_calli<f32[4], T, U, V>{"spu_fma", {std::forward<T>(a), std::forward<U>(b), std::forward<V>(c)}}.set_order_equality_hint(1, 1, 0);
	}

	template <typename T, typename U>
	static llvm_calli<f32[4], T, U> re_accurate(T&& a, U&& b)
	{
		return {"spu_re_acc", {std::forward<T>(a), std::forward<U>(b)}};
	}

	void FMA(spu_opcode_t op)
	{
		// Hardware FMA produces the same result as multiple + add on the limited double range (xfloat).
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			const auto [a, b, c] = get_vrs<f64[4]>(op.ra, op.rb, op.rc);
			set_vr(op.rt4, fmuladd(a, b, c));
			return;
		}

		register_intrinsic("spu_fma", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));
			const auto c = value<f32[4]>(ci->getOperand(2));

			if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::approximate)
			{
				const auto ma = sext<s32[4]>(fcmp_uno(a != fsplat<f32[4]>(0.)));
				const auto mb = sext<s32[4]>(fcmp_uno(b != fsplat<f32[4]>(0.)));
				const auto ca = bitcast<f32[4]>(bitcast<s32[4]>(a) & mb);
				const auto cb = bitcast<f32[4]>(bitcast<s32[4]>(b) & ma);
				return fma32x4(eval(ca), eval(cb), c);
			}
			else
			{
				return fma32x4(a, b, c);
			}
		});

		if (m_use_avx512)
		{
			register_intrinsic("spu_re_acc", [&](llvm::CallInst* ci)
			{
				const auto div = value<f32[4]>(ci->getOperand(0));
				const auto the_one = value<f32[4]>(ci->getOperand(1));

				const auto div_result = the_one / div;

				return vfixupimmps(div_result, div_result, splat<u32[4]>(0x00220088u), 0, 0xff);
			});
		}
		else
		{
			register_intrinsic("spu_re_acc", [&](llvm::CallInst* ci)
			{
				const auto div = value<f32[4]>(ci->getOperand(0));
				const auto the_one = value<f32[4]>(ci->getOperand(1));

				const auto div_result = the_one / div;

				// from ps3 hardware testing: Inf => NaN and NaN => Zero
				const auto result_and = bitcast<u32[4]>(div_result) & 0x7fffffffu;
				const auto result_cmp_inf = sext<s32[4]>(result_and == splat<u32[4]>(0x7F800000u));
				const auto result_cmp_nan = sext<s32[4]>(result_and <= splat<u32[4]>(0x7F800000u));

				const auto and_mask = bitcast<u32[4]>(result_cmp_nan) & splat<u32[4]>(0xFFFFFFFFu);
				const auto or_mask = bitcast<u32[4]>(result_cmp_inf) & splat<u32[4]>(0xFFFFFFFu);
				return bitcast<f32[4]>((bitcast<u32[4]>(div_result) & and_mask) | or_mask);
			});
		}


		const auto [a, b, c] = get_vrs<f32[4]>(op.ra, op.rb, op.rc);
		static const auto MT = match<f32[4]>();

		auto check_sqrt_pattern_for_float = [&](f32 float_value) -> bool
		{
			auto match_fnms = [&](f32 float_value)
			{
				auto res = match_expr(a, fnms(MT, MT, fsplat<f32[4]>(float_value)));
				if (std::get<0>(res))
					return res;

				return match_expr(b, fnms(MT, MT, fsplat<f32[4]>(float_value)));
			};

			auto match_fm_half = [&]()
			{
				auto res = match_expr(a, fm(MT, fsplat<f32[4]>(0.5)));
				if (std::get<0>(res))
					return res;

				res = match_expr(a, fm(fsplat<f32[4]>(0.5), MT));
				if (std::get<0>(res))
					return res;

				res = match_expr(b, fm(MT, fsplat<f32[4]>(0.5)));
				if (std::get<0>(res))
					return res;

				return match_expr(b, fm(fsplat<f32[4]>(0.5), MT));
			};


			if (auto [ok_fnma, a1, b1] = match_fnms(float_value); ok_fnma)
			{
				if (auto [ok_fm2, fm_half_mul] = match_fm_half(); ok_fm2 && fm_half_mul.eq(b1))
				{
					if (fm_half_mul.eq(b1))
					{
						if (auto [ok_fm1, a3, b3] = match_expr(c, fm(MT, MT)); ok_fm1 && a3.eq(a1))
						{
							if (auto [ok_sqrte, src] = match_expr(a3, spu_rsqrte(MT)); ok_sqrte && src.eq(b3))
							{
								erase_stores(a, b, c, a3);
								set_vr(op.rt4, fsqrt(fabs(src)));
								return true;
							}
						}
						else if (auto [ok_fm1, a3, b3] = match_expr(c, fm(MT, MT)); ok_fm1 && b3.eq(a1))
						{
							if (auto [ok_sqrte, src] = match_expr(b3, spu_rsqrte(MT)); ok_sqrte && src.eq(a3))
							{
								erase_stores(a, b, c, b3);
								set_vr(op.rt4, fsqrt(fabs(src)));
								return true;
							}
						}
					}
					else if (fm_half_mul.eq(a1))
					{
						if (auto [ok_fm1, a3, b3] = match_expr(c, fm(MT, MT)); ok_fm1 && a3.eq(b1))
						{
							if (auto [ok_sqrte, src] = match_expr(a3, spu_rsqrte(MT)); ok_sqrte && src.eq(b3))
							{
								erase_stores(a, b, c, a3);
								set_vr(op.rt4, fsqrt(fabs(src)));
								return true;
							}
						}
						else if (auto [ok_fm1, a3, b3] = match_expr(c, fm(MT, MT)); ok_fm1 && b3.eq(b1))
						{
							if (auto [ok_sqrte, src] = match_expr(b3, spu_rsqrte(MT)); ok_sqrte && src.eq(a3))
							{
								erase_stores(a, b, c, b3);
								set_vr(op.rt4, fsqrt(fabs(src)));
								return true;
							}
						}
					}
				}
			}

			return false;
		};

		if (check_sqrt_pattern_for_float(1.0f))
			return;

		if (check_sqrt_pattern_for_float(std::bit_cast<f32>(std::bit_cast<u32>(1.0f) + 1)))
			return;

		auto check_accurate_reciprocal_pattern_for_float = [&](f32 float_value) -> bool
		{
			// FMA(FNMS(div, spu_re(div), float_value), spu_re(div), spu_re(div))
			if (auto [ok_fnms, div] = match_expr(a, fnms(MT, b, fsplat<f32[4]>(float_value))); ok_fnms && op.rb == op.rc)
			{
				if (auto [ok_re] = match_expr(b, spu_re(div)); ok_re)
				{
					erase_stores(a, b, c);
					set_vr(op.rt4, re_accurate(div, fsplat<f32[4]>(float_value)));
					return true;
				}
			}

			// FMA(FNMS(spu_re(div), div, float_value), spu_re(div), spu_re(div))
			if (auto [ok_fnms, div] = match_expr(a, fnms(b, MT, fsplat<f32[4]>(float_value))); ok_fnms && op.rb == op.rc)
			{
				if (auto [ok_re] = match_expr(b, spu_re(div)); ok_re)
				{
					erase_stores(a, b, c);
					set_vr(op.rt4, re_accurate(div, fsplat<f32[4]>(float_value)));
					return true;
				}
			}

			// FMA(spu_re(div), FNMS(div, spu_re(div), float_value), spu_re(div))
			if (auto [ok_fnms, div] = match_expr(b, fnms(MT, a, fsplat<f32[4]>(float_value))); ok_fnms && op.ra == op.rc)
			{
				if (auto [ok_re] = match_expr(a, spu_re(div)); ok_re)
				{
					erase_stores(a, b, c);
					set_vr(op.rt4, re_accurate(div, fsplat<f32[4]>(float_value)));
					return true;
				}
			}

			// FMA(spu_re(div), FNMS(spu_re(div), div, float_value), spu_re(div))
			if (auto [ok_fnms, div] = match_expr(b, fnms(a, MT, fsplat<f32[4]>(float_value))); ok_fnms && op.ra == op.rc)
			{
				if (auto [ok_re] = match_expr(a, spu_re(div)); ok_re)
				{
					erase_stores(a, b, c);
					set_vr(op.rt4, re_accurate(div, fsplat<f32[4]>(float_value)));
					return true;
				}
			}

			return false;
		};

		if (check_accurate_reciprocal_pattern_for_float(1.0f))
			return;

		if (check_accurate_reciprocal_pattern_for_float(std::bit_cast<f32>(std::bit_cast<u32>(1.0f) + 1)))
			return;

		// NFS Most Wanted doesn't like this
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::relaxed)
		{
			// Those patterns are not safe vs non optimization as inaccuracy from spu_re will spread with early fm before the accuracy is improved

			// Match division (fast)
			// FMA(FNMS(fm(diva<*> spu_re(divb)), divb, diva), spu_re(divb), fm(diva<*> spu_re(divb)))
			if (auto [ok_fnma, divb, diva] = match_expr(a, fnms(c, MT, MT)); ok_fnma)
			{
				if (auto [ok_fm, fm1, fm2] = match_expr(c, fm(MT, MT)); ok_fm && ((fm1.eq(diva) && fm2.eq(b)) || (fm1.eq(b) && fm2.eq(diva))))
				{
					if (auto [ok_re] = match_expr(b, spu_re(divb)); ok_re)
					{
						erase_stores(b, c);
						set_vr(op.rt4, diva / divb);
						return;
					}
				}
			}

			// FMA(spu_re(divb), FNMS(fm(diva <*> spu_re(divb)), divb, diva), fm(diva <*> spu_re(divb)))
			if (auto [ok_fnma, divb, diva] = match_expr(b, fnms(c, MT, MT)); ok_fnma)
			{
				if (auto [ok_fm, fm1, fm2] = match_expr(c, fm(MT, MT)); ok_fm && ((fm1.eq(diva) && fm2.eq(a)) || (fm1.eq(a) && fm2.eq(diva))))
				{
					if (auto [ok_re] = match_expr(a, spu_re(divb)); ok_re)
					{
						erase_stores(a, c);
						set_vr(op.rt4, diva / divb);
						return;
					}
				}
			}
		}

		// Not all patterns can be simplified because of block scope
		// Those todos don't necessarily imply a missing pattern

		if (auto [ok_re, mystery] = match_expr(a, spu_re(MT)); ok_re)
		{
			spu_log.todo("[%s:0x%05x] Unmatched spu_re(a) found in FMA", m_hash, m_pos);
		}

		if (auto [ok_re, mystery] = match_expr(b, spu_re(MT)); ok_re)
		{
			spu_log.todo("[%s:0x%05x] Unmatched spu_re(b) found in FMA", m_hash, m_pos);
		}

		if (auto [ok_resq, mystery] = match_expr(c, spu_rsqrte(MT)); ok_resq)
		{
			spu_log.todo("[%s:0x%05x] Unmatched spu_rsqrte(c) found in FMA", m_hash, m_pos);
		}

		set_vr(op.rt4, fma(a, b, c));
	}

	template <typename T, typename U, typename V>
	static llvm_calli<f32[4], T, U, V> fms(T&& a, U&& b, V&& c)
	{
		return llvm_calli<f32[4], T, U, V>{"spu_fms", {std::forward<T>(a), std::forward<U>(b), std::forward<V>(c)}}.set_order_equality_hint(1, 1, 0);
	}

	void FMS(spu_opcode_t op)
	{
		// See FMA.
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			const auto [a, b, c] = get_vrs<f64[4]>(op.ra, op.rb, op.rc);
			set_vr(op.rt4, fmuladd(a, b, -c));
			return;
		}

		register_intrinsic("spu_fms", [&](llvm::CallInst* ci)
		{
			const auto a = value<f32[4]>(ci->getOperand(0));
			const auto b = value<f32[4]>(ci->getOperand(1));
			const auto c = value<f32[4]>(ci->getOperand(2));

			if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::approximate)
			{
				return fma32x4(clamp_smax(a), clamp_smax(b), eval(-c));
			}
			else
			{
				return fma32x4(a, b, eval(-c));
			}
		});

		set_vr(op.rt4, fms(get_vr<f32[4]>(op.ra), get_vr<f32[4]>(op.rb), get_vr<f32[4]>(op.rc)));
	}

	template <typename T, typename U>
	static llvm_calli<f32[4], T, U> fi(T&& a, U&& b)
	{
		return {"spu_fi", {std::forward<T>(a), std::forward<U>(b)}};
	}

	template <typename T>
	static llvm_calli<f32[4], T> spu_re(T&& a)
	{
		return {"spu_re", {std::forward<T>(a)}};
	}

	template <typename T>
	static llvm_calli<f32[4], T> spu_rsqrte(T&& a)
	{
		return {"spu_rsqrte", {std::forward<T>(a)}};
	}

	void FI(spu_opcode_t op)
	{
		register_intrinsic("spu_fi", [&](llvm::CallInst* ci)
		{
			// TODO: adjustment for denormals(for accurate xfloat only?)
			const auto a = bitcast<u32[4]>(value<f32[4]>(ci->getOperand(0)));
			const auto b = bitcast<u32[4]>(value<f32[4]>(ci->getOperand(1)));

			const auto base = (b & 0x007ffc00u) << 9; // Base fraction
			const auto ymul = (b & 0x3ff) * (a & 0x7ffff); // Step fraction * Y fraction (fixed point at 2^-32)
			const auto comparison = (ymul > base); // Should exponent be adjusted?
			const auto bnew = (base - ymul) >> (zext<u32[4]>(comparison) ^ 9); // Shift one less bit if exponent is adjusted
			const auto base_result = (b & 0xff800000u) | (bnew & ~0xff800000u); // Inject old sign and exponent
			const auto adjustment = bitcast<u32[4]>(sext<s32[4]>(comparison)) & (1 << 23); // exponent adjustement for negative bnew
			return clamp_smax(eval(bitcast<f32[4]>(base_result - adjustment)));
		});

		const auto [a, b] = get_vrs<f32[4]>(op.ra, op.rb);

		switch (g_cfg.core.spu_xfloat_accuracy)
		{
		case xfloat_accuracy::approximate:
		{
			// For approximate, create a pattern but do not optimize yet
			register_intrinsic("spu_re", [&](llvm::CallInst* ci)
			{
				const auto a = bitcast<u32[4]>(value<f32[4]>(ci->getOperand(0)));
				const auto a_fraction = (a >> splat<u32[4]>(18)) & splat<u32[4]>(0x1F);
				const auto a_exponent = (a & splat<u32[4]>(0x7F800000u));
				const auto r_exponent = sub_sat(build<u16[8]>(0000, 0x7E80, 0000, 0x7E80, 0000, 0x7E80, 0000, 0x7E80), bitcast<u16[8]>(a_exponent));
				const auto fix_exponent = select((a_exponent > 0), bitcast<u32[4]>(r_exponent), splat<u32[4]>(0x7F800000u));
				const auto a_sign = (a & splat<u32[4]>(0x80000000));
				value_t<u32[4]> b = eval(splat<u32[4]>(0));

				for (u32 i = 0; i < 4; i++)
				{
					const auto eval_fraction = eval(extract(a_fraction, i));

					value_t<u32> r_fraction = load_const<u32>(m_spu_frest_fraction_lut, eval_fraction);

					b = eval(insert(b, i, r_fraction));
				}

				b = eval(b | fix_exponent | a_sign);

				const auto base = (b & 0x007ffc00u) << 9; // Base fraction
				const auto ymul = (b & 0x3ff) * (a & 0x7ffff); // Step fraction * Y fraction (fixed point at 2^-32)
				const auto comparison = (ymul > base); // Should exponent be adjusted?
				const auto bnew = (base - ymul) >> (zext<u32[4]>(comparison) ^ 9); // Shift one less bit if exponent is adjusted
				const auto base_result = (b & 0xff800000u) | (bnew & ~0xff800000u); // Inject old sign and exponent
				const auto adjustment = bitcast<u32[4]>(sext<s32[4]>(comparison)) & (1 << 23); // exponent adjustement for negative bnew
				return clamp_smax(eval(bitcast<f32[4]>(base_result - adjustment)));
			});

			register_intrinsic("spu_rsqrte", [&](llvm::CallInst* ci)
			{
				const auto a = bitcast<u32[4]>(value<f32[4]>(ci->getOperand(0)));
				const auto a_fraction = (a >> splat<u32[4]>(18)) & splat<u32[4]>(0x3F);
				const auto a_exponent = (a >> splat<u32[4]>(23)) & splat<u32[4]>(0xFF);
				value_t<u32[4]> b = eval(splat<u32[4]>(0));

				for (u32 i = 0; i < 4; i++)
				{
					const auto eval_fraction = eval(extract(a_fraction, i));
					const auto eval_exponent = eval(extract(a_exponent, i));

					value_t<u32> r_fraction = load_const<u32>(m_spu_frsqest_fraction_lut, eval_fraction);
					value_t<u32> r_exponent = load_const<u32>(m_spu_frsqest_exponent_lut, eval_exponent);

					b = eval(insert(b, i, eval(r_fraction | r_exponent)));
				}

				const auto base = (b & 0x007ffc00u) << 9; // Base fraction
				const auto ymul = (b & 0x3ff) * (a & 0x7ffff); // Step fraction * Y fraction (fixed point at 2^-32)
				const auto comparison = (ymul > base); // Should exponent be adjusted?
				const auto bnew = (base - ymul) >> (zext<u32[4]>(comparison) ^ 9); // Shift one less bit if exponent is adjusted
				const auto base_result = (b & 0xff800000u) | (bnew & ~0xff800000u); // Inject old sign and exponent
				const auto adjustment = bitcast<u32[4]>(sext<s32[4]>(comparison)) & (1 << 23); // exponent adjustement for negative bnew
				return clamp_smax(eval(bitcast<f32[4]>(base_result - adjustment)));
			});
			break;
		}
		case xfloat_accuracy::relaxed:
		{
			// For relaxed, agressively optimize and use intrinsics, those make the results vary per cpu
			register_intrinsic("spu_re", [&](llvm::CallInst* ci)
			{
				const auto a = value<f32[4]>(ci->getOperand(0));
				return fre(a);
			});

			register_intrinsic("spu_rsqrte", [&](llvm::CallInst* ci)
			{
				const auto a = value<f32[4]>(ci->getOperand(0));
				return frsqe(a);
			});
			break;
		}
		default:
			break;
		}

		// Do not pattern match for accurate
		if(g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::approximate || g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::relaxed)
		{
			if (const auto [ok, mb] = match_expr(b, frest(match<f32[4]>())); ok && mb.eq(a))
			{
				erase_stores(b);
				set_vr(op.rt, spu_re(a));
				return;
			}

			if (const auto [ok, mb] = match_expr(b, frsqest(match<f32[4]>())); ok && mb.eq(a))
			{
				erase_stores(b);
				set_vr(op.rt, spu_rsqrte(a));
				return;
			}
		}

		const auto r = eval(fi(a, b));
		if (!m_interp_magn && g_cfg.core.spu_xfloat_accuracy != xfloat_accuracy::accurate)
			spu_log.todo("[%s:0x%05x] Unmatched spu_fi found", m_hash, m_pos);

		set_vr(op.rt, r);
	}

	void CFLTS(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			value_t<f64[4]> a = get_vr<f64[4]>(op.ra);
			value_t<f64[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f64[4]>(bitcast<f64>(((1023 + 173) - get_imm<u64>(op.i8)) << 52)));
			else
				s = eval(fsplat<f64[4]>(std::exp2(static_cast<int>(173 - op.i8))));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;

			if (auto ca = llvm::dyn_cast<llvm::ConstantDataVector>(a.value))
			{
				const f64 data[4]
				{
					ca->getElementAsDouble(0),
					ca->getElementAsDouble(1),
					ca->getElementAsDouble(2),
					ca->getElementAsDouble(3)
				};

				v128 result;

				for (u32 i = 0; i < 4; i++)
				{
					if (data[i] >= std::exp2(31.f))
					{
						result._s32[i] = smax;
					}
					else if (data[i] < std::exp2(-31.f))
					{
						result._s32[i] = smin;
					}
					else
					{
						result._s32[i] = static_cast<s32>(data[i]);
					}
				}

				r.value = make_const_vector(result, get_type<s32[4]>());
				set_vr(op.rt, r);
				return;
			}

			if (llvm::isa<llvm::ConstantAggregateZero>(a.value))
			{
				set_vr(op.rt, splat<u32[4]>(0));
				return;
			}

			r.value = m_ir->CreateFPToSI(a.value, get_type<s32[4]>());
			set_vr(op.rt, r ^ sext<s32[4]>(fcmp_ord(a >= fsplat<f64[4]>(std::exp2(31.f)))));
		}
		else
		{
			value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f32[4]>(load_const<f32>(m_scale_float_to, get_imm<u8>(op.i8))));
			else
				s = eval(fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;
			r.value = m_ir->CreateFPToSI(a.value, get_type<s32[4]>());
			set_vr(op.rt, r ^ sext<s32[4]>(bitcast<s32[4]>(a) > splat<s32[4]>(((31 + 127) << 23) - 1)));
		}
	}

	void CFLTU(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			value_t<f64[4]> a = get_vr<f64[4]>(op.ra);
			value_t<f64[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f64[4]>(bitcast<f64>(((1023 + 173) - get_imm<u64>(op.i8)) << 52)));
			else
				s = eval(fsplat<f64[4]>(std::exp2(static_cast<int>(173 - op.i8))));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;

			if (auto ca = llvm::dyn_cast<llvm::ConstantDataVector>(a.value))
			{
				const f64 data[4]
				{
					ca->getElementAsDouble(0),
					ca->getElementAsDouble(1),
					ca->getElementAsDouble(2),
					ca->getElementAsDouble(3)
				};

				v128 result;

				for (u32 i = 0; i < 4; i++)
				{
					if (data[i] >= std::exp2(32.f))
					{
						result._u32[i] = umax;
					}
					else if (data[i] < 0.)
					{
						result._u32[i] = 0;
					}
					else
					{
						result._u32[i] = static_cast<u32>(data[i]);
					}
				}

				r.value = make_const_vector(result, get_type<s32[4]>());
				set_vr(op.rt, r);
				return;
			}

			if (llvm::isa<llvm::ConstantAggregateZero>(a.value))
			{
				set_vr(op.rt, splat<u32[4]>(0));
				return;
			}

			r.value = m_ir->CreateFPToUI(a.value, get_type<s32[4]>());
			set_vr(op.rt, select(fcmp_ord(a >= fsplat<f64[4]>(std::exp2(32.f))), splat<s32[4]>(-1), r & sext<s32[4]>(fcmp_ord(a >= fsplat<f64[4]>(0.)))));
		}
		else
		{
			value_t<f32[4]> a = get_vr<f32[4]>(op.ra);
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f32[4]>(load_const<f32>(m_scale_float_to, get_imm<u8>(op.i8))));
			else
				s = eval(fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))));
			if (op.i8 != 173 || m_interp_magn)
				a = eval(a * s);

			value_t<s32[4]> r;

			if (m_use_avx512)
			{
				const auto sc = eval(bitcast<f32[4]>(max(bitcast<s32[4]>(a),splat<s32[4]>(0x0))));
				r.value = m_ir->CreateFPToUI(sc.value, get_type<s32[4]>());
				set_vr(op.rt, r);
				return;
			}

			r.value = m_ir->CreateFPToUI(a.value, get_type<s32[4]>());
			set_vr(op.rt, select(bitcast<s32[4]>(a) > splat<s32[4]>(((32 + 127) << 23) - 1), splat<s32[4]>(-1), r & ~(bitcast<s32[4]>(a) >> 31)));
		}
	}

	void CSFLT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			value_t<s32[4]> a = get_vr<s32[4]>(op.ra);
			value_t<f64[4]> r;

			if (auto [ok, data] = get_const_vector(a.value, m_pos); ok)
			{
				r.value = build<f64[4]>(data._s32[0], data._s32[1], data._s32[2], data._s32[3]).eval(m_ir);
			}
			else
			{
				r.value = m_ir->CreateSIToFP(a.value, get_type<f64[4]>());
			}

			value_t<f64[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f64[4]>(bitcast<f64>((get_imm<u64>(op.i8) + (1023 - 155)) << 52)));
			else
				s = eval(fsplat<f64[4]>(std::exp2(static_cast<int>(op.i8 - 155))));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
		else
		{
			value_t<f32[4]> r;
			r.value = m_ir->CreateSIToFP(get_vr<s32[4]>(op.ra).value, get_type<f32[4]>());
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f32[4]>(load_const<f32>(m_scale_to_float, get_imm<u8>(op.i8))));
			else
				s = eval(fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
	}

	void CUFLT(spu_opcode_t op)
	{
		if (g_cfg.core.spu_xfloat_accuracy == xfloat_accuracy::accurate)
		{
			value_t<s32[4]> a = get_vr<s32[4]>(op.ra);
			value_t<f64[4]> r;

			if (auto [ok, data] = get_const_vector(a.value, m_pos); ok)
			{
				r.value = build<f64[4]>(data._u32[0], data._u32[1], data._u32[2], data._u32[3]).eval(m_ir);
			}
			else
			{
				r.value = m_ir->CreateUIToFP(a.value, get_type<f64[4]>());
			}

			value_t<f64[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f64[4]>(bitcast<f64>((get_imm<u64>(op.i8) + (1023 - 155)) << 52)));
			else
				s = eval(fsplat<f64[4]>(std::exp2(static_cast<int>(op.i8 - 155))));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
		else
		{
			value_t<f32[4]> r;
			r.value = m_ir->CreateUIToFP(get_vr<s32[4]>(op.ra).value, get_type<f32[4]>());
			value_t<f32[4]> s;
			if (m_interp_magn)
				s = eval(vsplat<f32[4]>(load_const<f32>(m_scale_to_float, get_imm<u8>(op.i8))));
			else
				s = eval(fsplat<f32[4]>(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))));
			if (op.i8 != 155 || m_interp_magn)
				r = eval(r * s);
			set_vr(op.rt, r);
		}
	}

	void make_store_ls(value_t<u64> addr, value_t<u8[16]> data)
	{
		const auto bswapped = byteswap(data);
		m_ir->CreateStore(bswapped.eval(m_ir), _ptr(m_lsptr, addr.value));
	}

	auto make_load_ls(value_t<u64> addr)
	{
		value_t<u8[16]> data;
		data.value = m_ir->CreateLoad(get_type<u8[16]>(), _ptr(m_lsptr, addr.value));
		return byteswap(data);
	}

	void STQX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);

		for (auto pair : std::initializer_list<std::pair<value_t<u32[4]>, value_t<u32[4]>>>{{a, b}, {b, a}})
		{
			if (auto [ok, data] = get_const_vector(pair.first.value, m_pos); ok)
			{
				data._u32[3] %= SPU_LS_SIZE;

				if (data._u32[3] % 0x10 == 0)
				{
					value_t<u64> addr = eval(splat<u64>(data._u32[3]) + zext<u64>(extract(pair.second, 3) & 0x3fff0));
					make_store_ls(addr, get_vr<u8[16]>(op.rt));
					return;
				}
			}
		}

		value_t<u64> addr = eval(zext<u64>((extract(a, 3) + extract(b, 3)) & 0x3fff0));
		make_store_ls(addr, get_vr<u8[16]>(op.rt));
	}

	void LQX(spu_opcode_t op)
	{
		const auto a = get_vr(op.ra);
		const auto b = get_vr(op.rb);

		for (auto pair : std::initializer_list<std::pair<value_t<u32[4]>, value_t<u32[4]>>>{{a, b}, {b, a}})
		{
			if (auto [ok, data] = get_const_vector(pair.first.value, m_pos); ok)
			{
				data._u32[3] %= SPU_LS_SIZE;

				if (data._u32[3] % 0x10 == 0)
				{
					value_t<u64> addr = eval(splat<u64>(data._u32[3]) + zext<u64>(extract(pair.second, 3) & 0x3fff0));
					set_vr(op.rt, make_load_ls(addr));
					return;
				}
			}
		}

		value_t<u64> addr = eval(zext<u64>((extract(a, 3) + extract(b, 3)) & 0x3fff0));
		set_vr(op.rt, make_load_ls(addr));
	}

	void STQA(spu_opcode_t op)
	{
		value_t<u64> addr = eval((get_imm<u64>(op.i16, false) << 2) & 0x3fff0);
		make_store_ls(addr, get_vr<u8[16]>(op.rt));
	}

	void LQA(spu_opcode_t op)
	{
		value_t<u64> addr = eval((get_imm<u64>(op.i16, false) << 2) & 0x3fff0);
		set_vr(op.rt, make_load_ls(addr));
	}

	llvm::Value* get_pc_as_u64(u32 addr)
	{
		return m_ir->CreateAdd(m_ir->CreateZExt(m_base_pc, get_type<u64>()), m_ir->getInt64(addr - m_base));
	}

	void STQR(spu_opcode_t op) //
	{
		value_t<u64> addr;
		addr.value = m_interp_magn ? m_ir->CreateZExt(m_interp_pc, get_type<u64>()) : get_pc_as_u64(m_pos);
		addr = eval(((get_imm<u64>(op.i16, false) << 2) + addr) & (m_interp_magn ? 0x3fff0 : ~0xf));
		make_store_ls(addr, get_vr<u8[16]>(op.rt));
	}

	void LQR(spu_opcode_t op) //
	{
		value_t<u64> addr;
		addr.value = m_interp_magn ? m_ir->CreateZExt(m_interp_pc, get_type<u64>()) : get_pc_as_u64(m_pos);
		addr = eval(((get_imm<u64>(op.i16, false) << 2) + addr) & (m_interp_magn ? 0x3fff0 : ~0xf));
		set_vr(op.rt, make_load_ls(addr));
	}

	void STQD(spu_opcode_t op)
	{
		if (m_finfo && m_finfo->fn)
		{
			if (op.rt <= s_reg_sp || (op.rt >= s_reg_80 && op.rt <= s_reg_127))
			{
				if (m_block->bb->reg_save_dom[op.rt] && get_reg_raw(op.rt) == m_finfo->load[op.rt])
				{
					return;
				}
			}
		}

		value_t<u64> addr = eval(zext<u64>(extract(get_vr(op.ra), 3) & 0x3fff0) + (get_imm<u64>(op.si10) << 4));
		make_store_ls(addr, get_vr<u8[16]>(op.rt));
	}

	void LQD(spu_opcode_t op)
	{
		value_t<u64> addr = eval(zext<u64>(extract(get_vr(op.ra), 3) & 0x3fff0) + (get_imm<u64>(op.si10) << 4));
		set_vr(op.rt, make_load_ls(addr));
	}

	void make_halt(value_t<bool> cond)
	{
		const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
		const auto halt = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->CreateCondBr(cond.value, halt, next, m_md_unlikely);
		m_ir->SetInsertPoint(halt);
		if (m_interp_magn)
			m_ir->CreateStore(m_function->getArg(2), spu_ptr(&spu_thread::pc));
		else
			update_pc();
		const auto ptr = _ptr(m_memptr, 0xffdead00);
		m_ir->CreateStore(m_ir->getInt32("HALT"_u32), ptr);
		m_ir->CreateBr(next);
		m_ir->SetInsertPoint(next);
	}

	void HGT(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr<s32[4]>(op.ra), 3) > extract(get_vr<s32[4]>(op.rb), 3));
		make_halt(cond);
	}

	void HEQ(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) == extract(get_vr(op.rb), 3));
		make_halt(cond);
	}

	void HLGT(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) > extract(get_vr(op.rb), 3));
		make_halt(cond);
	}

	void HGTI(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr<s32[4]>(op.ra), 3) > get_imm<s32>(op.si10));
		make_halt(cond);
	}

	void HEQI(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) == get_imm<u32>(op.si10));
		make_halt(cond);
	}

	void HLGTI(spu_opcode_t op)
	{
		const auto cond = eval(extract(get_vr(op.ra), 3) > get_imm<u32>(op.si10));
		make_halt(cond);
	}

	void HBR([[maybe_unused]] spu_opcode_t op) //
	{
		// TODO: use the hint.
	}

	void HBRA([[maybe_unused]] spu_opcode_t op) //
	{
		// TODO: use the hint.
	}

	void HBRR([[maybe_unused]] spu_opcode_t op) //
	{
		// TODO: use the hint.
	}

	// TODO
	static u32 exec_check_interrupts(spu_thread* _spu, u32 addr)
	{
		_spu->set_interrupt_status(true);

		if (_spu->ch_events.load().count)
		{
			_spu->interrupts_enabled = false;
			_spu->srr0 = addr;

			// Test for BR/BRA instructions (they are equivalent at zero pc)
			const u32 br = _spu->_ref<const u32>(0);

			if ((br & 0xfd80007f) == 0x30000000)
			{
				return (br >> 5) & 0x3fffc;
			}

			return 0;
		}

		return addr;
	}

	llvm::BasicBlock* add_block_indirect(spu_opcode_t op, value_t<u32> addr, bool ret = true)
	{
		if (m_interp_magn)
		{
			m_interp_bblock = llvm::BasicBlock::Create(m_context, "", m_function);

			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto e_exec = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto d_test = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto d_exec = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto d_done = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			m_ir->CreateCondBr(get_imm<bool>(op.e).value, e_exec, d_test, m_md_unlikely);
			m_ir->SetInsertPoint(e_exec);
			const auto e_addr = call("spu_check_interrupts", &exec_check_interrupts, m_thread, addr.value);
			m_ir->CreateBr(d_test);
			m_ir->SetInsertPoint(d_test);
			const auto target = m_ir->CreatePHI(get_type<u32>(), 2);
			target->addIncoming(addr.value, result);
			target->addIncoming(e_addr, e_exec);
			m_ir->CreateCondBr(get_imm<bool>(op.d).value, d_exec, d_done, m_md_unlikely);
			m_ir->SetInsertPoint(d_exec);
			m_ir->CreateStore(m_ir->getFalse(), spu_ptr(&spu_thread::interrupts_enabled));
			m_ir->CreateBr(d_done);
			m_ir->SetInsertPoint(d_done);
			m_ir->CreateBr(m_interp_bblock);
			m_ir->SetInsertPoint(cblock);
			m_interp_pc = target;
			return result;
		}

		if (llvm::isa<llvm::Constant>(addr.value))
		{
			// Fixed branch excludes the possibility it's a function return (TODO)
			ret = false;
		}

		if (m_finfo && m_finfo->fn && op.opcode)
		{
			const auto cblock = m_ir->GetInsertBlock();
			const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->SetInsertPoint(result);
			ret_function();
			m_ir->SetInsertPoint(cblock);
			return result;
		}

		// Load stack addr if necessary
		value_t<u32> sp;

		if (ret && g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			if (op.opcode)
			{
				sp = eval(extract(get_reg_fixed(1), 3) & 0x3fff0);
			}
			else
			{
				sp.value = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::gpr, 1, &v128::_u32, 3));
			}
		}

		const auto cblock = m_ir->GetInsertBlock();
		const auto result = llvm::BasicBlock::Create(m_context, "", m_function);
		m_ir->SetInsertPoint(result);

		if (op.e)
		{
			addr.value = call("spu_check_interrupts", &exec_check_interrupts, m_thread, addr.value);
		}

		if (op.d)
		{
			m_ir->CreateStore(m_ir->getFalse(), spu_ptr(&spu_thread::interrupts_enabled));
		}

		m_ir->CreateStore(addr.value, spu_ptr(&spu_thread::pc));

		if (ret && g_cfg.core.spu_block_size >= spu_block_size_type::mega)
		{
			// Compare address stored in stack mirror with addr
			const auto stack0 = eval(zext<u64>(sp) + ::offset32(&spu_thread::stack_mirror));
			const auto stack1 = eval(stack0 + 8);
			const auto _ret = m_ir->CreateLoad(get_type<u64>(), _ptr(m_thread, stack0.value));
			const auto link = m_ir->CreateLoad(get_type<u64>(), _ptr(m_thread, stack1.value));
			const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto next = llvm::BasicBlock::Create(m_context, "", m_function);
			m_ir->CreateCondBr(m_ir->CreateICmpEQ(addr.value, m_ir->CreateTrunc(link, get_type<u32>())), next, fail, m_md_likely);
			m_ir->SetInsertPoint(next);
			const auto cmp2 = m_ir->CreateLoad(get_type<u32>(), _ptr(m_lsptr, addr.value));
			m_ir->CreateCondBr(m_ir->CreateICmpEQ(cmp2, m_ir->CreateTrunc(_ret, get_type<u32>())), done, fail, m_md_likely);
			m_ir->SetInsertPoint(done);

			// Clear stack mirror and return by tail call to the provided return address
			m_ir->CreateStore(splat<u64[2]>(-1).eval(m_ir), _ptr(m_thread, stack0.value));
			const auto type = m_finfo->chunk->getFunctionType();
			const auto fval = _ptr(get_segment_base(), m_ir->CreateLShr(_ret, 32));
			tail_chunk({type, fval}, m_ir->CreateTrunc(m_ir->CreateLShr(link, 32), get_type<u32>()));
			m_ir->SetInsertPoint(fail);
		}

		if (g_cfg.core.spu_block_size >= spu_block_size_type::mega)
		{
			// Try to load chunk address from the function table
			const auto fail = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto done = llvm::BasicBlock::Create(m_context, "", m_function);
			const auto ad32 = m_ir->CreateSub(addr.value, m_base_pc);
			m_ir->CreateCondBr(m_ir->CreateICmpULT(ad32, m_ir->getInt32(m_size)), done, fail, m_md_likely);
			m_ir->SetInsertPoint(done);

			const auto ad64 = m_ir->CreateZExt(ad32, get_type<u64>());
			const auto pptr = dyn_cast<llvm::GetElementPtrInst>(m_ir->CreateGEP(m_function_table->getValueType(), m_function_table, {m_ir->getInt64(0), m_ir->CreateLShr(ad64, 2, "", true)}));
			tail_chunk({m_dispatch->getFunctionType(), m_ir->CreateLoad(pptr->getResultElementType(), pptr)});
			m_ir->SetInsertPoint(fail);
		}

		tail_chunk(nullptr);
		m_ir->SetInsertPoint(cblock);
		return result;
	}

	llvm::BasicBlock* add_block_next()
	{
		if (m_interp_magn)
		{
			const auto cblock = m_ir->GetInsertBlock();
			m_ir->SetInsertPoint(m_interp_bblock);
			const auto target = m_ir->CreatePHI(get_type<u32>(), 2);
			target->addIncoming(m_interp_pc_next, cblock);
			target->addIncoming(m_interp_pc, m_interp_bblock->getSinglePredecessor());
			m_ir->SetInsertPoint(cblock);
			m_interp_pc = target;
			return m_interp_bblock;
		}

		return add_block(m_pos + 4);
	}

	void BIZ(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();

		const auto rt = get_vr<u8[16]>(op.rt);

		// Checking for zero doesn't care about the order of the bytes,
		// so load the data before it's byteswapped
		if (auto [ok, as] = match_expr(rt, byteswap(match<u8[16]>())); ok)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(bitcast<u32[4]>(as), 0) == 0);
			const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
			const auto target = add_block_indirect(op, addr);
			m_ir->CreateCondBr(cond.value, target, add_block_next());
			return;
		}

		const auto ox = get_vr<u32[4]>(op.rt);

		// Instead of extracting the value generated by orx, just test the input to orx with ptest
		if (auto [ok, as] = match_expr(ox, orx(match<u32[4]>())); ok)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto a = extract(bitcast<u64[2]>(as), 0);
			const auto b = extract(bitcast<u64[2]>(as), 1);
			const auto cond = eval((a | b) == 0);
			const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
			const auto target = add_block_indirect(op, addr);
			m_ir->CreateCondBr(cond.value, target, add_block_next());
			return;
		}


		// Check sign bit instead (optimization)
		if (match_vr<s32[4], s64[2]>(op.rt, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				const auto a = get_vr<s8[16]>(op.rt);
				const auto cond = eval(bitcast<s16>(trunc<bool[16]>(a)) >= 0);
				const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
				const auto target = add_block_indirect(op, addr);
				m_ir->CreateCondBr(cond.value, target, add_block_next());
				return true;
			}

			return false;
		}))
		{
			return;
		}

		const auto cond = eval(extract(get_vr(op.rt), 3) == 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block_next());
	}

	void BINZ(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();

		const auto rt = get_vr<u8[16]>(op.rt);

		// Checking for zero doesn't care about the order of the bytes,
		// so load the data before it's byteswapped
		if (auto [ok, as] = match_expr(rt, byteswap(match<u8[16]>())); ok)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(bitcast<u32[4]>(as), 0) != 0);
			const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
			const auto target = add_block_indirect(op, addr);
			m_ir->CreateCondBr(cond.value, target, add_block_next());
			return;
		}

		const auto ox = get_vr<u32[4]>(op.rt);

		// Instead of extracting the value generated by orx, just test the input to orx with ptest
		if (auto [ok, as] = match_expr(ox, orx(match<u32[4]>())); ok)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto a = extract(bitcast<u64[2]>(as), 0);
			const auto b = extract(bitcast<u64[2]>(as), 1);
			const auto cond = eval((a | b) != 0);
			const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
			const auto target = add_block_indirect(op, addr);
			m_ir->CreateCondBr(cond.value, target, add_block_next());
			return;
		}


		// Check sign bit instead (optimization)
		if (match_vr<s32[4], s64[2]>(op.rt, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				const auto a = get_vr<s8[16]>(op.rt);
				const auto cond = eval(bitcast<s16>(trunc<bool[16]>(a)) < 0);
				const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
				const auto target = add_block_indirect(op, addr);
				m_ir->CreateCondBr(cond.value, target, add_block_next());
				return true;
			}

			return false;
		}))
		{
			return;
		}

		const auto cond = eval(extract(get_vr(op.rt), 3) != 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block_next());
	}

	void BIHZ(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();

		// Check sign bits of 2 vector elements (optimization)
		if (match_vr<s8[16], s16[8], s32[4], s64[2]>(op.rt, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				const auto a = get_vr<s8[16]>(op.rt);
				const auto cond = eval((bitcast<s16>(trunc<bool[16]>(a)) & 0x3000) == 0);
				const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
				const auto target = add_block_indirect(op, addr);
				m_ir->CreateCondBr(cond.value, target, add_block_next());
				return true;
			}

			return false;
		}))
		{
			return;
		}

		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) == 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block_next());
	}

	void BIHNZ(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();

		// Check sign bits of 2 vector elements (optimization)
		if (match_vr<s8[16], s16[8], s32[4], s64[2]>(op.rt, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				const auto a = get_vr<s8[16]>(op.rt);
				const auto cond = eval((bitcast<s16>(trunc<bool[16]>(a)) & 0x3000) != 0);
				const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
				const auto target = add_block_indirect(op, addr);
				m_ir->CreateCondBr(cond.value, target, add_block_next());
				return true;
			}

			return false;
		}))
		{
			return;
		}

		const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) != 0);
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(cond.value, target, add_block_next());
	}

	void BI(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);

		if (m_interp_magn)
		{
			m_ir->CreateBr(add_block_indirect(op, addr));
			return;
		}

		// Create jump table if necessary (TODO)
		const auto tfound = m_targets.find(m_pos);

		if (op.d && tfound != m_targets.end() && tfound->second.size() == 1 && tfound->second[0] == spu_branch_target(m_pos, 1))
		{
			// Interrupts-disable pattern
			m_ir->CreateStore(m_ir->getFalse(), spu_ptr(&spu_thread::interrupts_enabled));
			return;
		}

		if (!op.d && !op.e && tfound != m_targets.end() && tfound->second.size() > 1)
		{
			// Shift aligned address for switch
			const auto addrfx = m_ir->CreateSub(addr.value, m_base_pc);
			const auto sw_arg = m_ir->CreateLShr(addrfx, 2, "", true);

			// Initialize jump table targets
			std::map<u32, llvm::BasicBlock*> targets;

			for (u32 target : tfound->second)
			{
				if (m_block_info[target / 4])
				{
					targets.emplace(target, nullptr);
				}
			}

			// Initialize target basic blocks
			for (auto& pair : targets)
			{
				pair.second = add_block(pair.first);
			}

			if (targets.empty())
			{
				// Emergency exit
				spu_log.error("[%s] [0x%05x] No jump table targets at 0x%05x (%u)", m_hash, m_entry, m_pos, tfound->second.size());
				m_ir->CreateBr(add_block_indirect(op, addr));
				return;
			}

			// Get jump table bounds (optimization)
			const u32 start = targets.begin()->first;
			const u32 end = targets.rbegin()->first + 4;

			// Emit switch instruction aiming for a jumptable in the end (indirectbr could guarantee it)
			const auto sw = m_ir->CreateSwitch(sw_arg, llvm::BasicBlock::Create(m_context, "", m_function), (end - start) / 4);

			for (u32 pos = start; pos < end; pos += 4)
			{
				if (m_block_info[pos / 4] && targets.count(pos))
				{
					const auto found = targets.find(pos);

					if (found != targets.end())
					{
						sw->addCase(m_ir->getInt32(pos / 4 - m_base / 4), found->second);
						continue;
					}
				}

				sw->addCase(m_ir->getInt32(pos / 4 - m_base / 4), sw->getDefaultDest());
			}

			// Exit function on unexpected target
			m_ir->SetInsertPoint(sw->getDefaultDest());
			m_ir->CreateStore(addr.value, spu_ptr(&spu_thread::pc));

			if (m_finfo && m_finfo->fn)
			{
				// Can't afford external tail call in true functions
				m_ir->CreateStore(m_ir->getInt32("BIJT"_u32), _ptr(m_memptr, 0xffdead20));
				m_ir->CreateCall(m_test_state, {m_thread});
				m_ir->CreateBr(sw->getDefaultDest());
			}
			else
			{
				tail_chunk(nullptr);
			}
		}
		else
		{
			// Simple indirect branch
			m_ir->CreateBr(add_block_indirect(op, addr));
		}
	}

	void BISL(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		set_link(op);
		m_ir->CreateBr(add_block_indirect(op, addr, false));
	}

	void IRET(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		value_t<u32> srr0;
		srr0.value = m_ir->CreateLoad(get_type<u32>(), spu_ptr(&spu_thread::srr0));
		m_ir->CreateBr(add_block_indirect(op, srr0));
	}

	void BISLED(spu_opcode_t op) //
	{
		if (m_block) m_block->block_end = m_ir->GetInsertBlock();
		const auto addr = eval(extract(get_vr(op.ra), 3) & 0x3fffc);
		set_link(op);
		const auto mask = m_ir->CreateTrunc(m_ir->CreateLShr(m_ir->CreateLoad(get_type<u64>(), spu_ptr(&spu_thread::ch_events), true), 32), get_type<u32>());
		const auto res = call("spu_get_events", &exec_get_events, m_thread, mask);
		const auto target = add_block_indirect(op, addr);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(res, m_ir->getInt32(0)), target, add_block_next());
	}

	void BRZ(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = m_ir->CreateSelect(eval(extract(get_vr(op.rt), 3) == 0).value, target.value, m_interp_pc_next);
			return;
		}

		const u32 target = spu_branch_target(m_pos, op.i16);

		const auto rt = get_vr<u8[16]>(op.rt);

		// Checking for zero doesn't care about the order of the bytes,
		// so load the data before it's byteswapped
		if (auto [ok, as] = match_expr(rt, byteswap(match<u8[16]>())); ok)
		{
			if (target != m_pos + 4)
			{
				m_block->block_end = m_ir->GetInsertBlock();
				const auto cond = eval(extract(bitcast<u32[4]>(as), 0) == 0);
				m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
				return;
			}
		}

		const auto ox = get_vr<u32[4]>(op.rt);

		// Instead of extracting the value generated by orx, just test the input to orx with ptest
		if (auto [ok, as] = match_expr(ox, orx(match<u32[4]>())); ok)
		{
			if (target != m_pos + 4)
			{
				m_block->block_end = m_ir->GetInsertBlock();
				const auto a = extract(bitcast<u64[2]>(as), 0);
				const auto b = extract(bitcast<u64[2]>(as), 1);
				const auto cond = eval((a | b) == 0);
				m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
				return;
			}
		}


		// Check sign bit instead (optimization)
		if (match_vr<s32[4], s64[2]>(op.rt, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				if (target != m_pos + 4)
				{
					m_block->block_end = m_ir->GetInsertBlock();
					const auto a = get_vr<s8[16]>(op.rt);
					const auto cond = eval(bitcast<s16>(trunc<bool[16]>(a)) >= 0);
					m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
					return true;
				}
			}

			return false;
		}))
		{
			return;
		}

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(get_vr(op.rt), 3) == 0);
			m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
		}
	}

	void BRNZ(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = m_ir->CreateSelect(eval(extract(get_vr(op.rt), 3) != 0).value, target.value, m_interp_pc_next);
			return;
		}

		const u32 target = spu_branch_target(m_pos, op.i16);

		const auto rt = get_vr<u8[16]>(op.rt);

		// Checking for zero doesn't care about the order of the bytes,
		// so load the data before it's byteswapped
		if (auto [ok, as] = match_expr(rt, byteswap(match<u8[16]>())); ok)
		{
			if (target != m_pos + 4)
			{
				m_block->block_end = m_ir->GetInsertBlock();
				const auto cond = eval(extract(bitcast<u32[4]>(as), 0) != 0);
				m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
				return;
			}
		}

		const auto ox = get_vr<u32[4]>(op.rt);

		// Instead of extracting the value generated by orx, just test the input to orx with ptest
		if (auto [ok, as] = match_expr(ox, orx(match<u32[4]>())); ok)
		{
			if (target != m_pos + 4)
			{
				m_block->block_end = m_ir->GetInsertBlock();
				const auto a = extract(bitcast<u64[2]>(as), 0);
				const auto b = extract(bitcast<u64[2]>(as), 1);
				const auto cond = eval((a | b) != 0);
				m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
				return;
			}
		}

		// Check sign bit instead (optimization)
		if (match_vr<s32[4], s64[2]>(op.rt, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				if (target != m_pos + 4)
				{
					m_block->block_end = m_ir->GetInsertBlock();
					const auto a = get_vr<s8[16]>(op.rt);
					const auto cond = eval(bitcast<s16>(trunc<bool[16]>(a)) < 0);
					m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
					return true;
				}
			}

			return false;
		}))
		{
			return;
		}

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(get_vr(op.rt), 3) != 0);
			m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
		}
	}

	void BRHZ(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = m_ir->CreateSelect(eval(extract(get_vr<u16[8]>(op.rt), 6) == 0).value, target.value, m_interp_pc_next);
			return;
		}

		const u32 target = spu_branch_target(m_pos, op.i16);

		// Check sign bits of 2 vector elements (optimization)
		if (match_vr<s8[16], s16[8], s32[4], s64[2]>(op.rt, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				if (target != m_pos + 4)
				{
					m_block->block_end = m_ir->GetInsertBlock();
					const auto a = get_vr<s8[16]>(op.rt);
					const auto cond = eval((bitcast<s16>(trunc<bool[16]>(a)) & 0x3000) == 0);
					m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
					return true;
				}
			}

			return false;
		}))
		{
			return;
		}

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) == 0);
			m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
		}
	}

	void BRHNZ(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = m_ir->CreateSelect(eval(extract(get_vr<u16[8]>(op.rt), 6) != 0).value, target.value, m_interp_pc_next);
			return;
		}

		const u32 target = spu_branch_target(m_pos, op.i16);

		// Check sign bits of 2 vector elements (optimization)
		if (match_vr<s8[16], s16[8], s32[4], s64[2]>(op.rt, [&](auto c, auto MP)
		{
			using VT = typename decltype(MP)::type;

			if (auto [ok, x] = match_expr(c, sext<VT>(match<bool[std::extent_v<VT>]>())); ok)
			{
				if (target != m_pos + 4)
				{
					m_block->block_end = m_ir->GetInsertBlock();
					const auto a = get_vr<s8[16]>(op.rt);
					const auto cond = eval((bitcast<s16>(trunc<bool[16]>(a)) & 0x3000) != 0);
					m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
					return true;
				}
			}

			return false;
		}))
		{
			return;
		}

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			const auto cond = eval(extract(get_vr<u16[8]>(op.rt), 6) != 0);
			m_ir->CreateCondBr(cond.value, add_block(target), add_block(m_pos + 4));
		}
	}

	void BRA(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			m_interp_pc = eval((get_imm<u32>(op.i16, false) << 2) & 0x3fffc).value;
			return;
		}

		const auto compiled_pos = m_ir->getInt32(m_pos);
		const u32 target = spu_branch_target(0, op.i16);

		m_block->block_end = m_ir->GetInsertBlock();
		const auto real_pos = get_pc(m_pos);
		value_t<u32> target_val;
		target_val.value = m_ir->getInt32(target);
		m_ir->CreateCondBr(m_ir->CreateICmpEQ(real_pos, compiled_pos), add_block(target, true), add_block_indirect({}, target_val));
	}

	void BRASL(spu_opcode_t op) //
	{
		set_link(op);
		BRA(op);
	}

	void BR(spu_opcode_t op) //
	{
		if (m_interp_magn)
		{
			value_t<u32> target;
			target.value = m_interp_pc;
			target = eval((target + (get_imm<u32>(op.i16, false) << 2)) & 0x3fffc);
			m_interp_pc = target.value;
			return;
		}

		const u32 target = spu_branch_target(m_pos, op.i16);

		if (target != m_pos + 4)
		{
			m_block->block_end = m_ir->GetInsertBlock();
			m_ir->CreateBr(add_block(target));
		}
	}

	void BRSL(spu_opcode_t op) //
	{
		set_link(op);

		const u32 target = spu_branch_target(m_pos, op.i16);

		if (m_finfo && m_finfo->fn && target != m_pos + 4)
		{
			if (auto fn = add_function(target)->fn)
			{
				call_function(fn);
				return;
			}
			else
			{
				spu_log.fatal("[0x%x] Can't add function 0x%x", m_pos, target);
				return;
			}
		}

		BR(op);
	}

	void set_link(spu_opcode_t op)
	{
		if (m_interp_magn)
		{
			value_t<u32> next;
			next.value = m_interp_pc_next;
			set_vr(op.rt, insert(splat<u32[4]>(0), 3, next));
			return;
		}

		set_vr(op.rt, insert(splat<u32[4]>(0), 3, value<u32>(get_pc(m_pos + 4)) & 0x3fffc));

		if (m_finfo && m_finfo->fn)
		{
			return;
		}

		if (g_cfg.core.spu_block_size >= spu_block_size_type::mega && m_block_info[m_pos / 4 + 1] && m_entry_info[m_pos / 4 + 1])
		{
			// Store the return function chunk address at the stack mirror
			const auto pfunc = add_function(m_pos + 4);
			const auto stack0 = eval(zext<u64>(extract(get_reg_fixed(1), 3) & 0x3fff0) + ::offset32(&spu_thread::stack_mirror));
			const auto stack1 = eval(stack0 + 8);
			const auto rel_ptr = m_ir->CreateSub(m_ir->CreatePtrToInt(pfunc->chunk, get_type<u64>()), m_ir->CreatePtrToInt(get_segment_base(), get_type<u64>()));
			const auto ptr_plus_op = m_ir->CreateOr(m_ir->CreateShl(rel_ptr, 32), m_ir->getInt64(m_next_op));
			const auto base_plus_pc = m_ir->CreateOr(m_ir->CreateShl(m_ir->CreateZExt(m_base_pc, get_type<u64>()), 32), m_ir->getInt64(m_pos + 4));
			m_ir->CreateStore(ptr_plus_op, _ptr(m_thread, stack0.value));
			m_ir->CreateStore(base_plus_pc, _ptr(m_thread, stack1.value));
		}
	}

	llvm::Value* get_segment_base()
	{
		const auto type = llvm::FunctionType::get(get_type<void>(), {}, false);
		const auto func = llvm::cast<llvm::Function>(m_module->getOrInsertFunction("spu_segment_base", type).getCallee());
		m_engine->updateGlobalMapping("spu_segment_base", reinterpret_cast<u64>(jit_runtime::alloc(0, 0)));
		return func;
	}

	static decltype(&spu_llvm_recompiler::UNK) decode(u32 op);
};

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_llvm_recompiler(u8 magn)
{
	return std::make_unique<spu_llvm_recompiler>(magn);
}

const spu_decoder<spu_llvm_recompiler> s_spu_llvm_decoder;

decltype(&spu_llvm_recompiler::UNK) spu_llvm_recompiler::decode(u32 op)
{
	return s_spu_llvm_decoder.decode(op);
}

#else

std::unique_ptr<spu_recompiler_base> spu_recompiler_base::make_llvm_recompiler(u8 magn)
{
	if (magn)
	{
		return nullptr;
	}

	fmt::throw_exception("LLVM is not available in this build.");
}

#endif // LLVM_AVAILABLE
