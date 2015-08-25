#include "stdafx.h"
#ifdef LLVM_AVAILABLE
#include "Utilities/Log.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
#include "Emu/Memory/Memory.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Vectorize.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/IR/Verifier.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

using namespace llvm;
using namespace ppu_recompiler_llvm;

#ifdef ID_MANAGER_INCLUDED
#error "ID Manager cannot be used in this module"
#endif

u64  Compiler::s_rotate_mask[64][64];
bool Compiler::s_rotate_mask_inited = false;

Compiler::Compiler(RecompilationEngine & recompilation_engine, const Executable execute_unknown_function,
	const Executable execute_unknown_block, bool(*poll_status_function)(PPUThread * ppu_state))
	: m_recompilation_engine(recompilation_engine)
	, m_poll_status_function(poll_status_function) {
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetDisassembler();

	m_llvm_context = new LLVMContext();
	m_ir_builder = new IRBuilder<>(*m_llvm_context);

	std::vector<Type *> arg_types;
	arg_types.push_back(m_ir_builder->getInt8PtrTy());
	arg_types.push_back(m_ir_builder->getInt64Ty());
	m_compiled_function_type = FunctionType::get(m_ir_builder->getInt32Ty(), arg_types, false);

	m_executableMap["execute_unknown_function"] = execute_unknown_function;
	m_executableMap["execute_unknown_block"] = execute_unknown_block;

	if (!s_rotate_mask_inited) {
		InitRotateMask();
		s_rotate_mask_inited = true;
	}
}

Compiler::~Compiler() {
	delete m_ir_builder;
	delete m_llvm_context;
}

std::pair<Executable, llvm::ExecutionEngine *> Compiler::Compile(const std::string & name, const ControlFlowGraph & cfg, bool generate_linkable_exits) {
	auto compilation_start = std::chrono::high_resolution_clock::now();

	m_module = new llvm::Module("Module", *m_llvm_context);
	m_execute_unknown_function = (Function *)m_module->getOrInsertFunction("execute_unknown_function", m_compiled_function_type);
	m_execute_unknown_function->setCallingConv(CallingConv::X86_64_Win64);

	m_execute_unknown_block = (Function *)m_module->getOrInsertFunction("execute_unknown_block", m_compiled_function_type);
	m_execute_unknown_block->setCallingConv(CallingConv::X86_64_Win64);

	std::string targetTriple = "x86_64-pc-windows-elf";
	m_module->setTargetTriple(targetTriple);

	llvm::ExecutionEngine *execution_engine =
		EngineBuilder(std::unique_ptr<llvm::Module>(m_module))
		.setEngineKind(EngineKind::JIT)
		.setMCJITMemoryManager(std::unique_ptr<llvm::SectionMemoryManager>(new CustomSectionMemoryManager(m_executableMap)))
		.setOptLevel(llvm::CodeGenOpt::Aggressive)
		.setMCPU("nehalem")
		.create();
	m_module->setDataLayout(execution_engine->getDataLayout());

	llvm::FunctionPassManager *fpm = new llvm::FunctionPassManager(m_module);
	fpm->add(createNoAAPass());
	fpm->add(createBasicAliasAnalysisPass());
	fpm->add(createNoTargetTransformInfoPass());
	fpm->add(createEarlyCSEPass());
	fpm->add(createTailCallEliminationPass());
	fpm->add(createReassociatePass());
	fpm->add(createInstructionCombiningPass());
	fpm->add(new DominatorTreeWrapperPass());
	fpm->add(new MemoryDependenceAnalysis());
	fpm->add(createGVNPass());
	fpm->add(createInstructionCombiningPass());
	fpm->add(new MemoryDependenceAnalysis());
	fpm->add(createDeadStoreEliminationPass());
	fpm->add(new LoopInfo());
	fpm->add(new ScalarEvolution());
	fpm->add(createSLPVectorizerPass());
	fpm->add(createInstructionCombiningPass());
	fpm->add(createCFGSimplificationPass());
	fpm->doInitialization();

	m_state.cfg = &cfg;
	m_state.generate_linkable_exits = generate_linkable_exits;

	// Create the function
	m_state.function = (Function *)m_module->getOrInsertFunction(name, m_compiled_function_type);
	m_state.function->setCallingConv(CallingConv::X86_64_Win64);
	auto arg_i = m_state.function->arg_begin();
	arg_i->setName("ppu_state");
	m_state.args[CompileTaskState::Args::State] = arg_i;
	(++arg_i)->setName("context");
	m_state.args[CompileTaskState::Args::Context] = arg_i;

	// Create the entry block and add code to branch to the first instruction
	m_ir_builder->SetInsertPoint(GetBasicBlockFromAddress(0));
	m_ir_builder->CreateBr(GetBasicBlockFromAddress(cfg.start_address));

	// Used to decode instructions
	PPUDisAsm dis_asm(CPUDisAsm_DumpMode);
	dis_asm.offset = vm::get_ptr<u8>(cfg.start_address);

	m_recompilation_engine.Log() << "Recompiling block :\n\n";

	// Convert each instruction in the CFG to LLVM IR
	std::vector<PHINode *> exit_instr_list;
	for (u32 instr_i : cfg.instruction_addresses) {
		m_state.hit_branch_instruction = false;
		m_state.current_instruction_address = instr_i;
		BasicBlock *instr_bb = GetBasicBlockFromAddress(m_state.current_instruction_address);
		m_ir_builder->SetInsertPoint(instr_bb);

		if (instr_bb->empty()) {
			u32 instr = vm::ps3::read32(m_state.current_instruction_address);

			// Dump PPU opcode
			dis_asm.dump_pc = m_state.current_instruction_address * 4;
			(*PPU_instr::main_list)(&dis_asm, instr);
			m_recompilation_engine.Log() << dis_asm.last_opcode;

			Decode(instr);
			if (!m_state.hit_branch_instruction)
				m_ir_builder->CreateBr(GetBasicBlockFromAddress(m_state.current_instruction_address + 4));
		}
	}

	// Generate exit logic for all empty blocks
	const std::string &default_exit_block_name = GetBasicBlockNameFromAddress(0xFFFFFFFF);
	for (BasicBlock &block_i : *m_state.function) {
		if (!block_i.getInstList().empty() || block_i.getName() == default_exit_block_name)
			continue;

		// Found an empty block
		m_state.current_instruction_address = GetAddressFromBasicBlockName(block_i.getName());

		m_ir_builder->SetInsertPoint(&block_i);
		PHINode *exit_instr_i32 = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 0);
		exit_instr_list.push_back(exit_instr_i32);

		SetPc(m_ir_builder->getInt32(m_state.current_instruction_address));

		if (generate_linkable_exits) {
			Value *context_i64 = m_ir_builder->CreateZExt(exit_instr_i32, m_ir_builder->getInt64Ty());
			context_i64 = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
			Value *ret_i32 = IndirectCall(m_state.current_instruction_address, context_i64, false);
			Value *cmp_i1 = m_ir_builder->CreateICmpNE(ret_i32, m_ir_builder->getInt32(0));
			BasicBlock *then_bb = GetBasicBlockFromAddress(m_state.current_instruction_address, "then_0");
			BasicBlock *merge_bb = GetBasicBlockFromAddress(m_state.current_instruction_address, "merge_0");
			m_ir_builder->CreateCondBr(cmp_i1, then_bb, merge_bb);

			m_ir_builder->SetInsertPoint(then_bb);
			context_i64 = m_ir_builder->CreateZExt(ret_i32, m_ir_builder->getInt64Ty());
			context_i64 = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
			m_ir_builder->CreateCall2(m_execute_unknown_block, m_state.args[CompileTaskState::Args::State], context_i64);
			m_ir_builder->CreateBr(merge_bb);

			m_ir_builder->SetInsertPoint(merge_bb);
			m_ir_builder->CreateRet(m_ir_builder->getInt32(0));
		}
		else {
			m_ir_builder->CreateRet(exit_instr_i32);
		}
	}

	// If the function has a default exit block then generate code for it
	BasicBlock *default_exit_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "", false);
	if (default_exit_bb) {
		m_ir_builder->SetInsertPoint(default_exit_bb);
		PHINode *exit_instr_i32 = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 0);
		exit_instr_list.push_back(exit_instr_i32);

		if (generate_linkable_exits) {
			Value *cmp_i1 = m_ir_builder->CreateICmpNE(exit_instr_i32, m_ir_builder->getInt32(0));
			BasicBlock *then_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "then_0");
			BasicBlock *merge_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "merge_0");
			m_ir_builder->CreateCondBr(cmp_i1, then_bb, merge_bb);

			m_ir_builder->SetInsertPoint(then_bb);
			Value *context_i64 = m_ir_builder->CreateZExt(exit_instr_i32, m_ir_builder->getInt64Ty());
			context_i64 = m_ir_builder->CreateOr(context_i64, (u64)cfg.function_address << 32);
			m_ir_builder->CreateCall2(m_execute_unknown_block, m_state.args[CompileTaskState::Args::State], context_i64);
			m_ir_builder->CreateBr(merge_bb);

			m_ir_builder->SetInsertPoint(merge_bb);
			m_ir_builder->CreateRet(m_ir_builder->getInt32(0));
		}
		else {
			m_ir_builder->CreateRet(exit_instr_i32);
		}
	}

	// Add incoming values for all exit instr PHI nodes
	for (PHINode *exit_instr_i : exit_instr_list) {
		BasicBlock *block = exit_instr_i->getParent();
		for (pred_iterator pred_i = pred_begin(block); pred_i != pred_end(block); pred_i++) {
			u32 pred_address = GetAddressFromBasicBlockName((*pred_i)->getName());
			exit_instr_i->addIncoming(m_ir_builder->getInt32(pred_address), *pred_i);
		}
	}

	m_recompilation_engine.Log() << "LLVM bytecode:\n";
	m_recompilation_engine.Log() << *m_module;

	std::string        verify;
	raw_string_ostream verify_ostream(verify);
	if (verifyFunction(*m_state.function, &verify_ostream)) {
		m_recompilation_engine.Log() << "Verification failed: " << verify_ostream.str() << "\n";
	}

	auto ir_build_end = std::chrono::high_resolution_clock::now();
	m_stats.ir_build_time += std::chrono::duration_cast<std::chrono::nanoseconds>(ir_build_end - compilation_start);

	// Optimize this function
	fpm->run(*m_state.function);
	auto optimize_end = std::chrono::high_resolution_clock::now();
	m_stats.optimization_time += std::chrono::duration_cast<std::chrono::nanoseconds>(optimize_end - ir_build_end);

	// Translate to machine code
	execution_engine->finalizeObject();
	void *function = execution_engine->getPointerToFunction(m_state.function);
	auto translate_end = std::chrono::high_resolution_clock::now();
	m_stats.translation_time += std::chrono::duration_cast<std::chrono::nanoseconds>(translate_end - optimize_end);

	/*    m_recompilation_engine.Log() << "\nDisassembly:\n";
		auto disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);
		for (size_t pc = 0; pc < mci.size();) {
			char str[1024];

			auto size = LLVMDisasmInstruction(disassembler, ((u8 *)mci.address()) + pc, mci.size() - pc, (uint64_t)(((u8 *)mci.address()) + pc), str, sizeof(str));
			m_recompilation_engine.Log() << fmt::format("0x%08X: ", (u64)(((u8 *)mci.address()) + pc)) << str << '\n';
			pc += size;
		}

		LLVMDisasmDispose(disassembler);*/

	auto compilation_end = std::chrono::high_resolution_clock::now();
	m_stats.total_time += std::chrono::duration_cast<std::chrono::nanoseconds>(compilation_end - compilation_start);
	delete fpm;

	assert(function != nullptr);
	return std::make_pair((Executable)function, execution_engine);
}

Compiler::Stats Compiler::GetStats() {
	return m_stats;
}

void Compiler::Decode(const u32 code) {
	(*PPU_instr::main_list)(this, code);
}

std::mutex                           RecompilationEngine::s_mutex;
std::shared_ptr<RecompilationEngine> RecompilationEngine::s_the_instance = nullptr;

RecompilationEngine::RecompilationEngine()
	: m_log(nullptr)
	, m_currentId(0)
	, m_last_cache_clear_time(std::chrono::high_resolution_clock::now())
	, m_compiler(*this, CPUHybridDecoderRecompiler::ExecuteFunction, CPUHybridDecoderRecompiler::ExecuteTillReturn, CPUHybridDecoderRecompiler::PollStatus) {
	m_compiler.RunAllTests();
}

RecompilationEngine::~RecompilationEngine() {
	m_address_to_function.clear();
	join();
}

Executable executeFunc;
Executable executeUntilReturn;

const Executable *RecompilationEngine::GetExecutable(u32 address, bool isFunction) {
	return isFunction ? &executeFunc : &executeUntilReturn;
}

std::pair<std::mutex, std::atomic<int> >* RecompilationEngine::GetMutexAndCounterForAddress(u32 address) {
	std::lock_guard<std::mutex> lock(m_address_locks_lock);
	std::unordered_map<u32, std::pair<std::mutex, std::atomic<int>> >::iterator It = m_address_locks.find(address);
	if (It == m_address_locks.end())
		return nullptr;
	return &(It->second);
}

const Executable *RecompilationEngine::GetCompiledExecutableIfAvailable(u32 address)
{
	std::lock_guard<std::mutex> lock(m_address_to_function_lock);
	std::unordered_map<u32, ExecutableStorage>::iterator It = m_address_to_function.find(address);
	if (It == m_address_to_function.end())
		return nullptr;
	if (std::get<1>(It->second) == nullptr)
		return nullptr;
	u32 id = std::get<3>(It->second);
	if (Ini.LLVMExclusionRange.GetValue() && (id >= Ini.LLVMMinId.GetValue() && id <= Ini.LLVMMaxId.GetValue()))
		return nullptr;
	return &(std::get<0>(It->second));
}

void RecompilationEngine::RemoveUnusedEntriesFromCache() {
	auto now = std::chrono::high_resolution_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_cache_clear_time).count() > 10000) {
		for (auto i = m_address_to_function.begin(); i != m_address_to_function.end();) {
			auto tmp = i;
			i++;
			if (std::get<2>(tmp->second) == 0)
				m_address_to_function.erase(tmp);
			else
				std::get<2>(tmp->second) = 0;
		}

		m_last_cache_clear_time = now;
	}
}

void RecompilationEngine::NotifyTrace(ExecutionTrace * execution_trace) {
	{
		std::lock_guard<std::mutex> lock(m_pending_execution_traces_lock);
		m_pending_execution_traces.push_back(execution_trace);
	}

	if (!joinable()) {
		start(WRAP_EXPR("PPU Recompilation Engine"), WRAP_EXPR(Task()));
	}

	cv.notify_one();
	// TODO: Increase the priority of the recompilation engine thread
}

raw_fd_ostream & RecompilationEngine::Log() {
	if (!m_log) {
		std::error_code error;
		m_log = new raw_fd_ostream("PPULLVMRecompiler.log", error, sys::fs::F_Text);
		m_log->SetUnbuffered();
	}

	return *m_log;
}

void RecompilationEngine::Task() {
	bool                     is_idling = false;
	std::chrono::nanoseconds idling_time(0);
	std::chrono::nanoseconds recompiling_time(0);

	auto start = std::chrono::high_resolution_clock::now();
	while (joinable() && !Emu.IsStopped()) {
		bool             work_done_this_iteration = false;
		ExecutionTrace * execution_trace = nullptr;

		{
			std::lock_guard<std::mutex> lock(m_pending_execution_traces_lock);

			auto i = m_pending_execution_traces.begin();
			if (i != m_pending_execution_traces.end()) {
				execution_trace = *i;
				m_pending_execution_traces.erase(i);
			}
		}

		if (execution_trace) {
			ProcessExecutionTrace(*execution_trace);
			delete execution_trace;
			work_done_this_iteration = true;
		}

		if (!work_done_this_iteration) {
			// TODO: Reduce the priority of the recompilation engine thread if its set to high priority
		}
		else {
			is_idling = false;
		}

		if (is_idling) {
			auto recompiling_start = std::chrono::high_resolution_clock::now();

			// Recompile the function whose CFG has changed the most since the last time it was compiled
			auto   candidate = (BlockEntry *)nullptr;
			size_t max_diff = 0;
			for (auto block : m_block_table) {
				if (block->IsFunction() && block->is_compiled) {
					auto diff = block->cfg.GetSize() - block->last_compiled_cfg_size;
					if (diff > max_diff) {
						candidate = block;
						max_diff = diff;
					}
				}
			}

			if (candidate != nullptr) {
				Log() << "Recompiling: " << candidate->ToString() << "\n";
				CompileBlock(*candidate);
				work_done_this_iteration = true;
			}

			auto recompiling_end = std::chrono::high_resolution_clock::now();
			recompiling_time += std::chrono::duration_cast<std::chrono::nanoseconds>(recompiling_end - recompiling_start);
		}

		if (!work_done_this_iteration) {
			is_idling = true;

			// Wait a few ms for something to happen
			auto idling_start = std::chrono::high_resolution_clock::now();
			std::unique_lock<std::mutex> lock(mutex);
			cv.wait_for(lock, std::chrono::milliseconds(250));
			auto idling_end = std::chrono::high_resolution_clock::now();
			idling_time += std::chrono::duration_cast<std::chrono::nanoseconds>(idling_end - idling_start);
		}
	}

	std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
	auto total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
	auto compiler_stats = m_compiler.GetStats();

	Log() << "Total time                      = " << total_time.count() / 1000000 << "ms\n";
	Log() << "    Time spent compiling        = " << compiler_stats.total_time.count() / 1000000 << "ms\n";
	Log() << "        Time spent building IR  = " << compiler_stats.ir_build_time.count() / 1000000 << "ms\n";
	Log() << "        Time spent optimizing   = " << compiler_stats.optimization_time.count() / 1000000 << "ms\n";
	Log() << "        Time spent translating  = " << compiler_stats.translation_time.count() / 1000000 << "ms\n";
	Log() << "    Time spent recompiling      = " << recompiling_time.count() / 1000000 << "ms\n";
	Log() << "    Time spent idling           = " << idling_time.count() / 1000000 << "ms\n";
	Log() << "    Time spent doing misc tasks = " << (total_time.count() - idling_time.count() - compiler_stats.total_time.count()) / 1000000 << "ms\n";

	LOG_NOTICE(PPU, "PPU LLVM Recompilation thread exiting.");
	s_the_instance = nullptr; // Can cause deadlock if this is the last instance. Need to fix this.
}

void RecompilationEngine::ProcessExecutionTrace(const ExecutionTrace & execution_trace) {
	auto execution_trace_id = execution_trace.GetId();
	auto processed_execution_trace_i = m_processed_execution_traces.find(execution_trace_id);
	if (processed_execution_trace_i == m_processed_execution_traces.end()) {
		Log() << "Trace: " << execution_trace.ToString() << "\n";
		// Find the function block
		BlockEntry key(execution_trace.function_address, execution_trace.function_address);
		auto       block_i = m_block_table.find(&key);
		if (block_i == m_block_table.end()) {
			block_i = m_block_table.insert(m_block_table.end(), new BlockEntry(key.cfg.start_address, key.cfg.function_address));
		}

		auto function_block = *block_i;
		block_i = m_block_table.end();
		auto split_trace = false;
		std::vector<BlockEntry *> tmp_block_list;
		for (auto trace_i = execution_trace.entries.begin(); trace_i != execution_trace.entries.end(); trace_i++) {
			if (trace_i->type == ExecutionTraceEntry::Type::CompiledBlock) {
				block_i = m_block_table.end();
				split_trace = true;
			}

			if (block_i == m_block_table.end()) {
				BlockEntry key(trace_i->GetPrimaryAddress(), execution_trace.function_address);
				block_i = m_block_table.find(&key);
				if (block_i == m_block_table.end()) {
					block_i = m_block_table.insert(m_block_table.end(), new BlockEntry(key.cfg.start_address, key.cfg.function_address));
				}

				tmp_block_list.push_back(*block_i);
			}

			const ExecutionTraceEntry * next_trace = nullptr;
			if (trace_i + 1 != execution_trace.entries.end()) {
				next_trace = &(*(trace_i + 1));
			}
			else if (!split_trace && execution_trace.type == ExecutionTrace::Type::Loop) {
				next_trace = &(*(execution_trace.entries.begin()));
			}

			UpdateControlFlowGraph((*block_i)->cfg, *trace_i, next_trace);
			if (*block_i != function_block) {
				UpdateControlFlowGraph(function_block->cfg, *trace_i, next_trace);
			}
		}

		processed_execution_trace_i = m_processed_execution_traces.insert(m_processed_execution_traces.end(), std::make_pair(execution_trace_id, std::move(tmp_block_list)));
	}

	for (auto i = processed_execution_trace_i->second.begin(); i != processed_execution_trace_i->second.end(); i++) {
		if (!(*i)->is_compiled) {
			(*i)->num_hits++;
			if ((*i)->num_hits >= Ini.LLVMThreshold.GetValue()) {
				CompileBlock(*(*i));
			}
		}
	}
	// TODO:: Syphurith: It is said that just remove_if would cause some troubles.. I don't know if that would cause Memleak. From CppCheck:
	// The return value of std::remove_if() is ignored. This function returns an iterator to the end of the range containing those elements that should be kept.
	// Elements past new end remain valid but with unspecified values. Use the erase method of the container to delete them.
	std::remove_if(processed_execution_trace_i->second.begin(), processed_execution_trace_i->second.end(), [](const BlockEntry * b)->bool { return b->is_compiled; });
}

void RecompilationEngine::UpdateControlFlowGraph(ControlFlowGraph & cfg, const ExecutionTraceEntry & this_entry, const ExecutionTraceEntry * next_entry) {
	if (this_entry.type == ExecutionTraceEntry::Type::Instruction) {
		cfg.instruction_addresses.insert(this_entry.GetPrimaryAddress());

		if (next_entry) {
			if (next_entry->type == ExecutionTraceEntry::Type::Instruction || next_entry->type == ExecutionTraceEntry::Type::CompiledBlock) {
				if (next_entry->GetPrimaryAddress() != (this_entry.GetPrimaryAddress() + 4)) {
					cfg.branches[this_entry.GetPrimaryAddress()].insert(next_entry->GetPrimaryAddress());
				}
			}
			else if (next_entry->type == ExecutionTraceEntry::Type::FunctionCall) {
				cfg.calls[this_entry.data.instruction.address].insert(next_entry->GetPrimaryAddress());
			}
		}
	}
	else if (this_entry.type == ExecutionTraceEntry::Type::CompiledBlock) {
		if (next_entry) {
			if (next_entry->type == ExecutionTraceEntry::Type::Instruction || next_entry->type == ExecutionTraceEntry::Type::CompiledBlock) {
				cfg.branches[this_entry.data.compiled_block.exit_address].insert(next_entry->GetPrimaryAddress());
			}
			else if (next_entry->type == ExecutionTraceEntry::Type::FunctionCall) {
				cfg.calls[this_entry.data.compiled_block.exit_address].insert(next_entry->GetPrimaryAddress());
			}
		}
	}
}

void RecompilationEngine::CompileBlock(BlockEntry & block_entry) {
	Log() << "Compile: " << block_entry.ToString() << "\n";
	Log() << "CFG: " << block_entry.cfg.ToString() << "\n";

	const std::pair<Executable, llvm::ExecutionEngine *> &compileResult =
		m_compiler.Compile(fmt::format("fn_0x%08X_%u", block_entry.cfg.start_address, block_entry.revision++), block_entry.cfg,
			block_entry.IsFunction() ? true : false /*generate_linkable_exits*/);

	// If entry doesn't exist, create it (using lock)
	std::unordered_map<u32, ExecutableStorage>::iterator It = m_address_to_function.find(block_entry.cfg.start_address);
	if (It == m_address_to_function.end())
	{
		std::lock_guard<std::mutex> lock(m_address_to_function_lock);
		std::get<1>(m_address_to_function[block_entry.cfg.start_address]) = nullptr;
	}

	std::unordered_map<u32, std::pair<std::mutex, std::atomic<int>> >::iterator It2 = m_address_locks.find(block_entry.cfg.start_address);
	if (It2 == m_address_locks.end())
	{
		std::lock_guard<std::mutex> lock(m_address_locks_lock);
		(void)m_address_locks[block_entry.cfg.start_address];
		m_address_locks[block_entry.cfg.start_address].second.store(0);
	}

	std::lock_guard<std::mutex> lock(m_address_locks[block_entry.cfg.start_address].first);

	int loopiteration = 0;
	while (m_address_locks[block_entry.cfg.start_address].second.load() > 0)
	{
		std::this_thread::yield();
		if (loopiteration++ > 10000000)
			return;
	}

	std::get<1>(m_address_to_function[block_entry.cfg.start_address]) = std::unique_ptr<llvm::ExecutionEngine>(compileResult.second);
	std::get<0>(m_address_to_function[block_entry.cfg.start_address]) = compileResult.first;
	std::get<3>(m_address_to_function[block_entry.cfg.start_address]) = m_currentId;
	Log() << "ID IS " << m_currentId << "\n";
	m_currentId++;
	block_entry.last_compiled_cfg_size = block_entry.cfg.GetSize();
	block_entry.is_compiled = true;
}

std::shared_ptr<RecompilationEngine> RecompilationEngine::GetInstance() {
	std::lock_guard<std::mutex> lock(s_mutex);

	if (s_the_instance == nullptr) {
		s_the_instance = std::shared_ptr<RecompilationEngine>(new RecompilationEngine());
	}

	return s_the_instance;
}

Tracer::Tracer()
	: m_recompilation_engine(RecompilationEngine::GetInstance()) {
	m_stack.reserve(100);
}

Tracer::~Tracer() {
	Terminate();
}

void Tracer::Trace(TraceType trace_type, u32 arg1, u32 arg2) {
	ExecutionTrace * execution_trace = nullptr;

	switch (trace_type) {
	case TraceType::CallFunction:
		// arg1 is address of the function
		m_stack.back()->entries.push_back(ExecutionTraceEntry(ExecutionTraceEntry::Type::FunctionCall, arg1));
		break;
	case TraceType::EnterFunction:
		// arg1 is address of the function
		m_stack.push_back(new ExecutionTrace(arg1));
		break;
	case TraceType::ExitFromCompiledFunction:
		// arg1 is address of function.
		// arg2 is the address of the exit instruction.
		if (arg2) {
			m_stack.push_back(new ExecutionTrace(arg1));
			m_stack.back()->entries.push_back(ExecutionTraceEntry(ExecutionTraceEntry::Type::CompiledBlock, arg1, arg2));
		}
		break;
	case TraceType::Return:
		// No args used
		execution_trace = m_stack.back();
		execution_trace->type = ExecutionTrace::Type::Linear;
		m_stack.pop_back();
		break;
	case TraceType::Instruction:
		// arg1 is the address of the instruction
		for (int i = (int)m_stack.back()->entries.size() - 1; i >= 0; i--) {
			if ((m_stack.back()->entries[i].type == ExecutionTraceEntry::Type::Instruction && m_stack.back()->entries[i].data.instruction.address == arg1) ||
				(m_stack.back()->entries[i].type == ExecutionTraceEntry::Type::CompiledBlock && m_stack.back()->entries[i].data.compiled_block.entry_address == arg1)) {
				// Found a loop
				execution_trace = new ExecutionTrace(m_stack.back()->function_address);
				execution_trace->type = ExecutionTrace::Type::Loop;
				std::copy(m_stack.back()->entries.begin() + i, m_stack.back()->entries.end(), std::back_inserter(execution_trace->entries));
				m_stack.back()->entries.erase(m_stack.back()->entries.begin() + i + 1, m_stack.back()->entries.end());
				break;
			}
		}

		if (!execution_trace) {
			// A loop was not found
			m_stack.back()->entries.push_back(ExecutionTraceEntry(ExecutionTraceEntry::Type::Instruction, arg1));
		}
		break;
	case TraceType::ExitFromCompiledBlock:
		// arg1 is address of the compiled block.
		// arg2 is the address of the exit instruction.
		m_stack.back()->entries.push_back(ExecutionTraceEntry(ExecutionTraceEntry::Type::CompiledBlock, arg1, arg2));

		if (arg2 == 0) {
			// Return from function
			execution_trace = m_stack.back();
			execution_trace->type = ExecutionTrace::Type::Linear;
			m_stack.pop_back();
		}
		break;
	default:
		assert(0);
		break;
	}

	if (execution_trace) {
		m_recompilation_engine->NotifyTrace(execution_trace);
	}
}

void Tracer::Terminate() {
	// TODO: Notify recompilation engine
}

ppu_recompiler_llvm::CPUHybridDecoderRecompiler::CPUHybridDecoderRecompiler(PPUThread & ppu)
	: m_ppu(ppu)
	, m_interpreter(new PPUInterpreter(ppu))
	, m_decoder(m_interpreter)
	, m_recompilation_engine(RecompilationEngine::GetInstance()) {
	executeFunc = CPUHybridDecoderRecompiler::ExecuteFunction;
	executeUntilReturn = CPUHybridDecoderRecompiler::ExecuteTillReturn;
}

ppu_recompiler_llvm::CPUHybridDecoderRecompiler::~CPUHybridDecoderRecompiler() {
}

u32 ppu_recompiler_llvm::CPUHybridDecoderRecompiler::DecodeMemory(const u32 address) {
	ExecuteFunction(&m_ppu, 0);
	return 0;
}

u32 ppu_recompiler_llvm::CPUHybridDecoderRecompiler::ExecuteFunction(PPUThread * ppu_state, u64 context) {
	auto execution_engine = (CPUHybridDecoderRecompiler *)ppu_state->GetDecoder();
	execution_engine->m_tracer.Trace(Tracer::TraceType::EnterFunction, ppu_state->PC, 0);
	return ExecuteTillReturn(ppu_state, 0);
}

/// Get the branch type from a branch instruction
static BranchType GetBranchTypeFromInstruction(u32 instruction)
{
	u32 instructionOpcode = PPU_instr::fields::OPCD(instruction);
	u32 lk = instruction & 1;

	if (instructionOpcode == PPU_opcodes::PPU_MainOpcodes::B ||
		instructionOpcode == PPU_opcodes::PPU_MainOpcodes::BC)
		return lk ? BranchType::FunctionCall : BranchType::LocalBranch;
	if (instructionOpcode == PPU_opcodes::PPU_MainOpcodes::G_13) {
		u32 G13Opcode = PPU_instr::fields::GD_13(instruction);
		if (G13Opcode == PPU_opcodes::G_13Opcodes::BCLR)
			return lk ? BranchType::FunctionCall : BranchType::Return;
		if (G13Opcode == PPU_opcodes::G_13Opcodes::BCCTR)
			return lk ? BranchType::FunctionCall : BranchType::LocalBranch;
		return BranchType::NonBranch;
	}
	if (instructionOpcode == PPU_opcodes::PPU_MainOpcodes::HACK && (instruction & EIF_PERFORM_BLR)) // classify HACK instruction
		return instruction & EIF_USE_BRANCH ? BranchType::FunctionCall : BranchType::Return;
	if (instructionOpcode == PPU_opcodes::PPU_MainOpcodes::HACK && (instruction & EIF_USE_BRANCH))
		return BranchType::LocalBranch;
	return BranchType::NonBranch;
}

u32 ppu_recompiler_llvm::CPUHybridDecoderRecompiler::ExecuteTillReturn(PPUThread * ppu_state, u64 context) {
	CPUHybridDecoderRecompiler *execution_engine = (CPUHybridDecoderRecompiler *)ppu_state->GetDecoder();

	if (context)
		execution_engine->m_tracer.Trace(Tracer::TraceType::ExitFromCompiledFunction, context >> 32, context & 0xFFFFFFFF);

	while (PollStatus(ppu_state) == false) {
		std::pair<std::mutex, std::atomic<int>> *mut = execution_engine->m_recompilation_engine->GetMutexAndCounterForAddress(ppu_state->PC);
		if (mut) {
			{
				std::lock_guard<std::mutex> lock(mut->first);
				mut->second.fetch_add(1);
			}
			const Executable *executable = execution_engine->m_recompilation_engine->GetCompiledExecutableIfAvailable(ppu_state->PC);
			if (executable)
			{
				auto entry = ppu_state->PC;
				u32 exit = (u32)(*executable)(ppu_state, 0);
				mut->second.fetch_sub(1);
				execution_engine->m_tracer.Trace(Tracer::TraceType::ExitFromCompiledBlock, entry, exit);
				if (exit == 0)
					return 0;
				continue;
			}
			mut->second.fetch_add(1);
		}
		execution_engine->m_tracer.Trace(Tracer::TraceType::Instruction, ppu_state->PC, 0);
		u32 instruction = vm::ps3::read32(ppu_state->PC);
		u32 oldPC = ppu_state->PC;
		execution_engine->m_decoder.Decode(instruction);
		auto branch_type = ppu_state->PC != oldPC ? GetBranchTypeFromInstruction(instruction) : BranchType::NonBranch;
		ppu_state->PC += 4;

		switch (branch_type) {
		case BranchType::Return:
			execution_engine->m_tracer.Trace(Tracer::TraceType::Return, 0, 0);
			if (Emu.GetCPUThreadStop() == ppu_state->PC) ppu_state->fast_stop();
			return 0;
		case BranchType::FunctionCall: {
			execution_engine->m_tracer.Trace(Tracer::TraceType::CallFunction, ppu_state->PC, 0);
			ExecuteFunction(ppu_state, 0);
			break;
		}
		case BranchType::LocalBranch:
			break;
		case BranchType::NonBranch:
			break;
		default:
			assert(0);
			break;
		}
	}

	return 0;
}

bool ppu_recompiler_llvm::CPUHybridDecoderRecompiler::PollStatus(PPUThread * ppu_state) {
	return ppu_state->check_status();
}
#endif // LLVM_AVAILABLE
