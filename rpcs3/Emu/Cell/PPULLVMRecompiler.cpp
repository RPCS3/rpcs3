#include "stdafx.h"
#ifdef LLVM_AVAILABLE
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPULLVMRecompiler.h"
#include "Emu/Memory/Memory.h"
#include "Utilities/VirtualMemory.h"
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

// PS3 can address 32 bits aligned on 4 bytes boundaries : 2^30 pointers
#define VIRTUAL_INSTRUCTION_COUNT 0x40000000
#define PAGE_SIZE 4096

u64  Compiler::s_rotate_mask[64][64];
bool Compiler::s_rotate_mask_inited = false;

std::unique_ptr<Module> Compiler::create_module(LLVMContext &llvm_context)
{
	const std::vector<Type *> arg_types = { Type::getInt8PtrTy(llvm_context), Type::getInt64Ty(llvm_context) };
	FunctionType *compiled_function_type = FunctionType::get(Type::getInt32Ty(llvm_context), arg_types, false);

	std::unique_ptr<llvm::Module> result(new llvm::Module("Module", llvm_context));
	Function *execute_unknown_function = (Function *)result->getOrInsertFunction("execute_unknown_function", compiled_function_type);
	execute_unknown_function->setCallingConv(CallingConv::X86_64_Win64);

	Function *execute_unknown_block = (Function *)result->getOrInsertFunction("execute_unknown_block", compiled_function_type);
	execute_unknown_block->setCallingConv(CallingConv::X86_64_Win64);

	std::string targetTriple = "x86_64-pc-windows-elf";
	result->setTargetTriple(targetTriple);

	return result;
}

void Compiler::optimise_module(llvm::Module *module)
{
	llvm::FunctionPassManager fpm(module);
	fpm.add(createNoAAPass());
	fpm.add(createBasicAliasAnalysisPass());
	fpm.add(createNoTargetTransformInfoPass());
	fpm.add(createEarlyCSEPass());
	fpm.add(createTailCallEliminationPass());
	fpm.add(createReassociatePass());
	fpm.add(createInstructionCombiningPass());
	fpm.add(new DominatorTreeWrapperPass());
	fpm.add(new MemoryDependenceAnalysis());
	fpm.add(createGVNPass());
	fpm.add(createInstructionCombiningPass());
	fpm.add(new MemoryDependenceAnalysis());
	fpm.add(createDeadStoreEliminationPass());
	fpm.add(new LoopInfo());
	fpm.add(new ScalarEvolution());
	fpm.add(createSLPVectorizerPass());
	fpm.add(createInstructionCombiningPass());
	fpm.add(createCFGSimplificationPass());
	fpm.doInitialization();

	for (auto I = module->begin(), E = module->end(); I != E; ++I)
		fpm.run(*I);
}


Compiler::Compiler(LLVMContext *context, llvm::IRBuilder<> *builder, std::unordered_map<std::string, void*> &function_ptrs)
	: m_llvm_context(context),
	m_ir_builder(builder),
	m_executable_map(function_ptrs) {

	std::vector<Type *> arg_types;
	arg_types.push_back(m_ir_builder->getInt8PtrTy());
	arg_types.push_back(m_ir_builder->getInt64Ty());
	m_compiled_function_type = FunctionType::get(m_ir_builder->getInt32Ty(), arg_types, false);

	if (!s_rotate_mask_inited) {
		InitRotateMask();
		s_rotate_mask_inited = true;
	}
}

Compiler::~Compiler() {
}

void Compiler::initiate_function(const std::string &name)
{
	m_state.function = (Function *)m_module->getOrInsertFunction(name, m_compiled_function_type);
	m_state.function->setCallingConv(CallingConv::X86_64_Win64);
	auto arg_i = m_state.function->arg_begin();
	arg_i->setName("ppu_state");
	m_state.args[CompileTaskState::Args::State] = arg_i;
	(++arg_i)->setName("context");
	m_state.args[CompileTaskState::Args::Context] = arg_i;
}

void ppu_recompiler_llvm::Compiler::translate_to_llvm_ir(llvm::Module *module, const std::string & name, u32 start_address, u32 instruction_count)
{
	m_module = module;

	m_execute_unknown_function = module->getFunction("execute_unknown_function");
	m_execute_unknown_block = module->getFunction("execute_unknown_block");

	initiate_function(name);

	// Create the entry block and add code to branch to the first instruction
	m_ir_builder->SetInsertPoint(GetBasicBlockFromAddress(0));
	m_ir_builder->CreateBr(GetBasicBlockFromAddress(start_address));

	// Convert each instruction in the CFG to LLVM IR
	std::vector<PHINode *> exit_instr_list;
	for (u32 instructionAddress = start_address; instructionAddress < start_address + instruction_count * 4; instructionAddress += 4) {
		m_state.hit_branch_instruction = false;
		m_state.current_instruction_address = instructionAddress;
		BasicBlock *instr_bb = GetBasicBlockFromAddress(instructionAddress);
		m_ir_builder->SetInsertPoint(instr_bb);

		u32 instr = vm::ps3::read32(instructionAddress);

		Decode(instr);
		if (!m_state.hit_branch_instruction)
			m_ir_builder->CreateBr(GetBasicBlockFromAddress(instructionAddress + 4));
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

		m_ir_builder->CreateRet(m_ir_builder->getInt32(ExecutionStatus::ExecutionStatusBlockEnded));
	}

	// If the function has a default exit block then generate code for it
	BasicBlock *default_exit_bb = GetBasicBlockFromAddress(0xFFFFFFFF, "", false);
	if (default_exit_bb) {
		m_ir_builder->SetInsertPoint(default_exit_bb);
		PHINode *exit_instr_i32 = m_ir_builder->CreatePHI(m_ir_builder->getInt32Ty(), 0);
		exit_instr_list.push_back(exit_instr_i32);

		m_ir_builder->CreateRet(m_ir_builder->getInt32(0));
	}

	// Add incoming values for all exit instr PHI nodes
	for (PHINode *exit_instr_i : exit_instr_list) {
		BasicBlock *block = exit_instr_i->getParent();
		for (pred_iterator pred_i = pred_begin(block); pred_i != pred_end(block); pred_i++) {
			u32 pred_address = GetAddressFromBasicBlockName((*pred_i)->getName());
			exit_instr_i->addIncoming(m_ir_builder->getInt32(pred_address), *pred_i);
		}
	}

	std::string        verify;
	raw_string_ostream verify_ostream(verify);
	if (verifyFunction(*m_state.function, &verify_ostream)) {
//		m_recompilation_engine.Log() << "Verification failed: " << verify_ostream.str() << "\n";
	}

	m_module = nullptr;
	m_state.function = nullptr;
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
	, m_llvm_context(getGlobalContext())
	, m_ir_builder(getGlobalContext()) {
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetDisassembler();

	FunctionCache = (ExecutableStorageType *)memory_helper::reserve_memory(VIRTUAL_INSTRUCTION_COUNT * sizeof(ExecutableStorageType));
	// Each char can store 8 page status
	FunctionCachePagesCommited = (char *)malloc(VIRTUAL_INSTRUCTION_COUNT / (8 * PAGE_SIZE));
	memset(FunctionCachePagesCommited, 0, VIRTUAL_INSTRUCTION_COUNT / (8 * PAGE_SIZE));
}

RecompilationEngine::~RecompilationEngine() {
	m_executable_storage.clear();
	memory_helper::free_reserved_memory(FunctionCache, VIRTUAL_INSTRUCTION_COUNT * sizeof(ExecutableStorageType));
	free(FunctionCachePagesCommited);
}

bool RecompilationEngine::isAddressCommited(u32 address) const
{
	size_t offset = address * sizeof(ExecutableStorageType);
	size_t page = offset / 4096;
	// Since bool is stored in char, the char index is page / 8 (or page >> 3)
	// and we shr the value with the remaining bits (page & 7)
	return (FunctionCachePagesCommited[page >> 3] >> (page & 7)) & 1;
}

void RecompilationEngine::commitAddress(u32 address)
{
	size_t offset = address * sizeof(ExecutableStorageType);
	size_t page = offset / 4096;
	memory_helper::commit_page_memory((u8*)FunctionCache + page * 4096, 4096);
	// Reverse of isAddressCommited : we set the (page & 7)th bit of (page / 8) th char
	// in the array
	FunctionCachePagesCommited[page >> 3] |= (1 << (page & 7));
}

const Executable RecompilationEngine::GetCompiledExecutableIfAvailable(u32 address) const
{
	if (!isAddressCommited(address / 4))
		return nullptr;
	u32 id = FunctionCache[address / 4].second;
	if (rpcs3::state.config.core.llvm.exclusion_range.value() &&
		(id >= rpcs3::state.config.core.llvm.min_id.value() && id <= rpcs3::state.config.core.llvm.max_id.value()))
		return nullptr;
	return FunctionCache[address / 4].first;
}

void RecompilationEngine::NotifyBlockStart(u32 address) {
	{
		std::lock_guard<std::mutex> lock(m_pending_address_start_lock);
		if (m_pending_address_start.size() > 10000)
			m_pending_address_start.clear();
		m_pending_address_start.push_back(address);
	}

	if (!is_started()) {
		start();
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

void RecompilationEngine::on_task() {
	std::chrono::nanoseconds idling_time(0);
	std::chrono::nanoseconds recompiling_time(0);

	auto start = std::chrono::high_resolution_clock::now();
	while (!Emu.IsStopped()) {
		bool             work_done_this_iteration = false;
		std::list <u32> m_current_execution_traces;

		{
			std::lock_guard<std::mutex> lock(m_pending_address_start_lock);
			m_current_execution_traces.swap(m_pending_address_start);
		}

		if (!m_current_execution_traces.empty()) {
			for (u32 address : m_current_execution_traces)
				work_done_this_iteration |= IncreaseHitCounterAndBuild(address);
		}

		if (!work_done_this_iteration) {
			// Wait a few ms for something to happen
			auto idling_start = std::chrono::high_resolution_clock::now();
			std::unique_lock<std::mutex> lock(mutex);
			cv.wait_for(lock, std::chrono::milliseconds(10));
			auto idling_end = std::chrono::high_resolution_clock::now();
			idling_time += std::chrono::duration_cast<std::chrono::nanoseconds>(idling_end - idling_start);
		}
	}

	s_the_instance = nullptr; // Can cause deadlock if this is the last instance. Need to fix this.
}

bool RecompilationEngine::IncreaseHitCounterAndBuild(u32 address) {
	auto It = m_block_table.find(address);
	if (It == m_block_table.end())
		It = m_block_table.emplace(address, BlockEntry(address)).first;
	BlockEntry &block = It->second;
	if (!block.is_compiled) {
		block.num_hits++;
		if (block.num_hits >= rpcs3::state.config.core.llvm.threshold.value()) {
			CompileBlock(block);
			return true;
		}
	}
	return false;
}

extern void execute_ppu_func_by_index(PPUThread& ppu, u32 id);
extern void execute_syscall_by_index(PPUThread& ppu, u64 code);

static u32
wrappedExecutePPUFuncByIndex(PPUThread &CPU, u32 index) noexcept {
	try
	{
		execute_ppu_func_by_index(CPU, index);
		return ExecutionStatus::ExecutionStatusBlockEnded;
	}
	catch (...)
	{
		CPU.pending_exception = std::current_exception();
		return ExecutionStatus::ExecutionStatusPropagateException;
	}
}

static u32 wrappedDoSyscall(PPUThread &CPU, u64 code) noexcept {
	try
	{
		execute_syscall_by_index(CPU, code);
		return ExecutionStatus::ExecutionStatusBlockEnded;
	}
	catch (...)
	{
		CPU.pending_exception = std::current_exception();
		return ExecutionStatus::ExecutionStatusPropagateException;
	}
}

static void wrapped_fast_stop(PPUThread &CPU)
{
	CPU.fast_stop();
}

std::pair<Executable, llvm::ExecutionEngine *> RecompilationEngine::compile(const std::string & name, u32 start_address, u32 instruction_count) {
	std::unique_ptr<llvm::Module> module = Compiler::create_module(m_llvm_context);

	std::unordered_map<std::string, void*> function_ptrs;
	function_ptrs["execute_unknown_function"] = reinterpret_cast<void*>(CPUHybridDecoderRecompiler::ExecuteFunction);
	function_ptrs["execute_unknown_block"] = reinterpret_cast<void*>(CPUHybridDecoderRecompiler::ExecuteTillReturn);
	function_ptrs["PollStatus"] = reinterpret_cast<void*>(CPUHybridDecoderRecompiler::PollStatus);
	function_ptrs["PPUThread.fast_stop"] = reinterpret_cast<void*>(wrapped_fast_stop);
	function_ptrs["vm.reservation_acquire"] = reinterpret_cast<void*>(vm::reservation_acquire);
	function_ptrs["vm.reservation_update"] = reinterpret_cast<void*>(vm::reservation_update);
	function_ptrs["get_timebased_time"] = reinterpret_cast<void*>(get_timebased_time);
	function_ptrs["wrappedExecutePPUFuncByIndex"] = reinterpret_cast<void*>(wrappedExecutePPUFuncByIndex);
	function_ptrs["wrappedDoSyscall"] = reinterpret_cast<void*>(wrappedDoSyscall);

#define REGISTER_FUNCTION_PTR(name) \
	function_ptrs[#name] = reinterpret_cast<void*>(PPUInterpreter::name##_impl);

	MACRO_PPU_INST_MAIN_EXPANDERS(REGISTER_FUNCTION_PTR)
	MACRO_PPU_INST_G_13_EXPANDERS(REGISTER_FUNCTION_PTR)
	MACRO_PPU_INST_G_1E_EXPANDERS(REGISTER_FUNCTION_PTR)
	MACRO_PPU_INST_G_1F_EXPANDERS(REGISTER_FUNCTION_PTR)
	MACRO_PPU_INST_G_3A_EXPANDERS(REGISTER_FUNCTION_PTR)
	MACRO_PPU_INST_G_3E_EXPANDERS(REGISTER_FUNCTION_PTR)

	Compiler(&m_llvm_context, &m_ir_builder, function_ptrs)
		.translate_to_llvm_ir(module.get(), name, start_address, instruction_count);

	llvm::Module *module_ptr = module.get();

	Log() << *module_ptr;
	Compiler::optimise_module(module_ptr);

	llvm::ExecutionEngine *execution_engine =
		EngineBuilder(std::move(module))
		.setEngineKind(EngineKind::JIT)
		.setMCJITMemoryManager(std::unique_ptr<llvm::SectionMemoryManager>(new CustomSectionMemoryManager(function_ptrs)))
		.setOptLevel(llvm::CodeGenOpt::Aggressive)
		.setMCPU("nehalem")
		.create();
	module_ptr->setDataLayout(execution_engine->getDataLayout());

	// Translate to machine code
	execution_engine->finalizeObject();

	Function *llvm_function = module_ptr->getFunction(name);
	void *function = execution_engine->getPointerToFunction(llvm_function);

	/*    m_recompilation_engine.Log() << "\nDisassembly:\n";
	auto disassembler = LLVMCreateDisasm(sys::getProcessTriple().c_str(), nullptr, 0, nullptr, nullptr);
	for (size_t pc = 0; pc < mci.size();) {
	char str[1024];

	auto size = LLVMDisasmInstruction(disassembler, ((u8 *)mci.address()) + pc, mci.size() - pc, (uint64_t)(((u8 *)mci.address()) + pc), str, sizeof(str));
	m_recompilation_engine.Log() << fmt::format("0x%08X: ", (u64)(((u8 *)mci.address()) + pc)) << str << '\n';
	pc += size;
	}

	LLVMDisasmDispose(disassembler);*/

	assert(function != nullptr);
	return std::make_pair((Executable)function, execution_engine);
}

/**
* This code is inspired from Dolphin PPC Analyst
*/
inline s32 SignExt16(s16 x) { return (s32)(s16)x; }
inline s32 SignExt26(u32 x) { return x & 0x2000000 ? (s32)(x | 0xFC000000) : (s32)(x); }

bool RecompilationEngine::AnalyseBlock(BlockEntry &functionData, size_t maxSize)
{
	u32 startAddress = functionData.address;
	u32 farthestBranchTarget = startAddress;
	functionData.instructionCount = 0;
	functionData.calledFunctions.clear();
	functionData.is_analysed = true;
	functionData.is_compilable_function = true;
	Log() << "Analysing " << (void*)(uint64_t)startAddress << "hit " << functionData.num_hits << "\n";
	// Used to decode instructions
	PPUDisAsm dis_asm(CPUDisAsm_DumpMode);
	dis_asm.offset = vm::ps3::_ptr<u8>(startAddress);
	for (size_t instructionAddress = startAddress; instructionAddress < startAddress + maxSize; instructionAddress += 4)
	{
		u32 instr = vm::ps3::read32((u32)instructionAddress);

		dis_asm.dump_pc = instructionAddress - startAddress;
		(*PPU_instr::main_list)(&dis_asm, instr);
		Log() << dis_asm.last_opcode;
		functionData.instructionCount++;
		if (instr == PPU_instr::implicts::BLR() && instructionAddress >= farthestBranchTarget && functionData.is_compilable_function)
		{
			Log() << "Analysis: Block is compilable into a function \n";
			return true;
		}
		else if (PPU_instr::fields::GD_13(instr) == PPU_opcodes::G_13Opcodes::BCCTR)
		{
			if (!PPU_instr::fields::LK(instr))
			{
				Log() << "Analysis: indirect branching found \n";
				functionData.is_compilable_function = false;
				return true;
			}
		}
		else if (PPU_instr::fields::OPCD(instr) == PPU_opcodes::PPU_MainOpcodes::BC)
		{
			u32 target = SignExt16(PPU_instr::fields::BD(instr));
			if (!PPU_instr::fields::AA(instr)) // Absolute address
				target += (u32)instructionAddress;
			if (target > farthestBranchTarget && !PPU_instr::fields::LK(instr))
				farthestBranchTarget = target;
		}
		else if (PPU_instr::fields::OPCD(instr) == PPU_opcodes::PPU_MainOpcodes::B)
		{
			u32 target = SignExt26(PPU_instr::fields::LL(instr));
			if (!PPU_instr::fields::AA(instr)) // Absolute address
				target += (u32)instructionAddress;

			if (!PPU_instr::fields::LK(instr))
			{
				if (target < startAddress)
				{
					Log() << "Analysis: branch to previous block\n";
					functionData.is_compilable_function = false;
					return true;
				}
				else if (target > farthestBranchTarget)
					farthestBranchTarget = target;
			}
			else
				functionData.calledFunctions.insert(target);
		}
	}
	Log() << "Analysis: maxSize reached \n";
	functionData.is_compilable_function = false;
	return true;
}

void RecompilationEngine::CompileBlock(BlockEntry & block_entry) {
	if (block_entry.is_analysed)
		return;

	if (!AnalyseBlock(block_entry))
		return;
	Log() << "Compile: " << block_entry.ToString() << "\n";

	{
		// We create a lock here so that data are properly stored at the end of the function.
		/// Lock for accessing compiler
		std::mutex local_mutex;
		std::unique_lock<std::mutex> lock(local_mutex);

		const std::pair<Executable, llvm::ExecutionEngine *> &compileResult =
			compile(fmt::format("fn_0x%08X", block_entry.address), block_entry.address, block_entry.instructionCount);

		if (!isAddressCommited(block_entry.address / 4))
			commitAddress(block_entry.address / 4);

		m_executable_storage.push_back(std::unique_ptr<llvm::ExecutionEngine>(compileResult.second));
		Log() << "Associating " << (void*)(uint64_t)block_entry.address << " with ID " << m_currentId << "\n";
		FunctionCache[block_entry.address / 4] = std::make_pair(compileResult.first, m_currentId);
		m_currentId++;
		block_entry.is_compiled = true;
	}
}

std::shared_ptr<RecompilationEngine> RecompilationEngine::GetInstance() {
	std::lock_guard<std::mutex> lock(s_mutex);

	if (s_the_instance == nullptr) {
		s_the_instance = std::shared_ptr<RecompilationEngine>(new RecompilationEngine());
	}

	return s_the_instance;
}

ppu_recompiler_llvm::CPUHybridDecoderRecompiler::CPUHybridDecoderRecompiler(PPUThread & ppu)
	: m_ppu(ppu)
	, m_interpreter(new PPUInterpreter(ppu))
	, m_decoder(m_interpreter)
	, m_recompilation_engine(RecompilationEngine::GetInstance()) {
}

ppu_recompiler_llvm::CPUHybridDecoderRecompiler::~CPUHybridDecoderRecompiler() {
}

u32 ppu_recompiler_llvm::CPUHybridDecoderRecompiler::DecodeMemory(const u32 address) {
	ExecuteFunction(&m_ppu, 0);
	if (m_ppu.pending_exception != nullptr) {
		std::exception_ptr exn = m_ppu.pending_exception;
		m_ppu.pending_exception = nullptr;
		std::rethrow_exception(exn);
	}
	return 0;
}

u32 ppu_recompiler_llvm::CPUHybridDecoderRecompiler::ExecuteFunction(PPUThread * ppu_state, u64 context) {
	auto execution_engine = (CPUHybridDecoderRecompiler *)ppu_state->GetDecoder();
	if (ExecuteTillReturn(ppu_state, 0) == ExecutionStatus::ExecutionStatusPropagateException)
		return ExecutionStatus::ExecutionStatusPropagateException;
	return ExecutionStatus::ExecutionStatusReturn;
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

	// A block is a sequence of contiguous address.
	bool previousInstContigousAndInterp = false;

	while (PollStatus(ppu_state) == false) {
		const Executable executable = execution_engine->m_recompilation_engine->GetCompiledExecutableIfAvailable(ppu_state->PC);
		if (executable)
		{
			auto entry = ppu_state->PC;
			u32 exit = (u32)executable(ppu_state, 0);
			if (exit == ExecutionStatus::ExecutionStatusReturn)
			{
				if (Emu.GetCPUThreadStop() == ppu_state->PC) ppu_state->fast_stop();
				return ExecutionStatus::ExecutionStatusReturn;
			}
			if (exit == ExecutionStatus::ExecutionStatusPropagateException)
				return ExecutionStatus::ExecutionStatusPropagateException;
			previousInstContigousAndInterp = false;
			continue;
		}
		// if previousInstContigousAndInterp is true, ie previous step was either a compiled block or a branch inst
		// that caused a "gap" in instruction flow, we notify a new block.
		if (!previousInstContigousAndInterp)
			execution_engine->m_recompilation_engine->NotifyBlockStart(ppu_state->PC);
		u32 instruction = vm::ps3::read32(ppu_state->PC);
		u32 oldPC = ppu_state->PC;
		try
		{
			execution_engine->m_decoder.Decode(instruction);
		}
		catch (...)
		{
			ppu_state->pending_exception = std::current_exception();
			return ExecutionStatus::ExecutionStatusPropagateException;
		}
		previousInstContigousAndInterp = (oldPC == ppu_state->PC);
		auto branch_type = ppu_state->PC != oldPC ? GetBranchTypeFromInstruction(instruction) : BranchType::NonBranch;
		ppu_state->PC += 4;

		switch (branch_type) {
		case BranchType::Return:
			if (Emu.GetCPUThreadStop() == ppu_state->PC) ppu_state->fast_stop();
			return 0;
		case BranchType::FunctionCall: {
			u32 status = ExecuteFunction(ppu_state, 0);
			if (status == ExecutionStatus::ExecutionStatusPropagateException)
				return ExecutionStatus::ExecutionStatusPropagateException;
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
	try
	{
		return ppu_state->check_status();
	}
	catch (...)
	{
		ppu_state->pending_exception = std::current_exception();
		return true;
	}
}
#endif // LLVM_AVAILABLE
