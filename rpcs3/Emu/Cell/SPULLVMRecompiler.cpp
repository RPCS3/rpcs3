#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "SPUDisAsm.h"
#include "SPUThread.h"
#include "SPUInterpreter.h"
#include "SPUASMJITRecompiler.h"
#include "SPULLVMRecompiler.h"

#include "llvm/IR/Value.h"
#include "llvm/IR/Verifier.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Analysis/Lint.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Vectorize.h"

#include "llvm/Support/TargetSelect.h"

using namespace llvm;

const spu_decoder<spu_interpreter_fast> s_spu_interpreter;
const spu_decoder<spu_llvm_recompiler> s_spu_decoder;

spu_llvm_recompiler::spu_llvm_recompiler(SPUThread &thread) 
	: spu_recompiler_base(thread)
{
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	LLVMLinkInMCJIT();

	m_asmjit_recompiler = std::make_unique<spu_recompiler>(thread);

	m_cache_path = Emu.GetCachePath() + "spu_cache/";
	if (!fs::is_dir(m_cache_path))
	{
		fs::create_path(m_cache_path);
	}

	m_jit = std::make_unique<jit_compiler>(std::unordered_map<std::string, u64>(), sys::getHostCPUName());

	m_module = std::make_unique<Module>("SPURecompiler", m_context);
	m_module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));

	std::vector<Type*> funccall_types = { Type::getInt64PtrTy(m_context), Type::getInt32Ty(m_context) };
	FunctionType *spu_funccall_type = FunctionType::get(Type::getInt32Ty(m_context), funccall_types.data());
	m_spufunccall = Function::Create(spu_funccall_type, Function::ExternalLinkage, "SPUFunctionCall", m_module.get());
	m_jit->engine()->addGlobalMapping(m_spufunccall, (void*)&spu_recompiler_base::SPUFunctionCall);

	std::vector<Type*> intcall_types = { Type::getInt64PtrTy(m_context), Type::getInt32Ty(m_context), Type::getInt8PtrTy(m_context) };
	FunctionType *spu_intcall_type = FunctionType::get(Type::getInt32Ty(m_context), intcall_types.data());
	m_spuintcall = Function::Create(spu_intcall_type, Function::ExternalLinkage, "SPUInterpreterCall", m_module.get());
	m_jit->engine()->addGlobalMapping(m_spuintcall, (void*)&spu_recompiler_base::SPUInterpreterCall);

	m_jit->add(std::move(m_module));
	m_jit->fin();
}

void spu_llvm_recompiler::compile(spu_function_t & f)
{
	//std::lock_guard<std::mutex> lock(m_mutex);

	if (f.compiled)
	{
		// return if function already compiled
		return;
	}

	std::string module_name("spu_func_0000000000000000000000000000000000000000");
	for (u32 i = 0; i < sizeof(f.hash); i++)
	{
		constexpr auto pal = "0123456789abcdef";
		module_name[4 + i * 2] = pal[f.hash[i] >> 4];
		module_name[5 + i * 2] = pal[f.hash[i] & 15];
	}

	// Check object file
	std::string module_path = m_cache_path + module_name;
	if (fs::is_file(module_path))
	{
		m_jit->add(module_path);
		LOG_SUCCESS(SPU, "SPU LLVM: Loaded module %s", module_name);
		return;
	}

	//compile with asmjit first to hide the absurdly long llvm compile times, if we don't do this then a lot of games break
	m_asmjit_recompiler->compile(f);

	//spin up a new thread and run our llvm compilation there, swap out f.compiled once its finished
	std::thread compile_thread(&spu_llvm_recompiler::compile_llvm, this, f);
	compile_thread.detach();
}

void spu_llvm_recompiler::compile_llvm(spu_function_t &f)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	//todo avoid duplicating this here
	std::string module_name("spu_func_0000000000000000000000000000000000000000");
	for (u32 i = 0; i < sizeof(f.hash); i++)
	{
		constexpr auto pal = "0123456789abcdef";
		module_name[4 + i * 2] = pal[f.hash[i] >> 4];
		module_name[5 + i * 2] = pal[f.hash[i] & 15];
	}

	m_module = std::make_unique<Module>(module_name, m_context);
	m_module->setTargetTriple(Triple::normalize(sys::getProcessTriple()));

	//add our global functions to the new module
	std::vector<Type*> funccall_types = { Type::getInt64PtrTy(m_context), Type::getInt32Ty(m_context) };
	FunctionType *spu_funccall_type = FunctionType::get(Type::getInt32Ty(m_context), funccall_types.data());
	m_spufunccall = Function::Create(spu_funccall_type, Function::ExternalLinkage, "SPUFunctionCall", m_module.get());

	std::vector<Type*> intcall_types = { Type::getInt64PtrTy(m_context), Type::getInt32Ty(m_context), Type::getInt8PtrTy(m_context) };
	FunctionType *spu_intcall_type = FunctionType::get(Type::getInt32Ty(m_context), intcall_types.data());
	m_spuintcall = Function::Create(spu_intcall_type, Function::ExternalLinkage, "SPUInterpreterCall", m_module.get());

	if (f.addr >= 0x40000 || f.addr % 4 || f.size == 0 || f.size > 0x40000 - f.addr || f.size % 4)
	{
		fmt::throw_exception("Invalid SPU function (addr=0x%05x, size=0x%x)" HERE, f.addr, f.size);
	}

	Function *func = compile_function(f);

	const auto ptr = m_module.get();
	m_jit->add(std::move(m_module), m_cache_path + module_name, false);

#if 0
	std::string result;
	raw_string_ostream out(result);

	std::string func_name = fmt::format("__0x%llx", f.addr);

	legacy::PassManager mpm;
	mpm.add(createLintPass());

	mpm.run(*ptr);

	out << *ptr; // print IR
	fs::file(func_name + "_ir_opt.log", fs::rewrite).write(out.str());
	result.clear();
#endif

	m_jit->fin();

	u32(*fptr)(SPUThread*, be_t<u32>*) = reinterpret_cast<u32(*)(SPUThread*, be_t<u32>*)>(m_jit->engine()->getFunctionAddress(m_llvm_func->getName()));
	f.compiled = fptr;

	//LOG_SUCCESS(SPU, "Replaced asmjit func with llvm func");
}

Function* spu_llvm_recompiler::compile_function(spu_function_t& f)
{
	SPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
	dis_asm.offset = reinterpret_cast<u8*>(f.data.data()) - f.addr;

	std::vector<Type*> spu_func_args(2, Type::getInt64PtrTy(m_context));
	FunctionType *spu_functype = FunctionType::get(Type::getInt32Ty(m_context), spu_func_args, false);

	std::string func_name = fmt::format("__0x%llx", f.addr);
	m_func = &f;
	m_llvm_func = Function::Create(spu_functype, Function::ExternalLinkage, func_name, m_module.get());

	//set up args
	Function::arg_iterator args = m_llvm_func->arg_begin();
	
	Argument& cpu = *args++;
	cpu.setName("cpu");
	m_cpu = &cpu;

	Argument& ls = *args++;
	ls.setName("ls");
	m_ls = &ls;

	//set up entry block
	BasicBlock* entry_block = BasicBlock::Create(m_context, "entry", m_llvm_func);
	m_body = BasicBlock::Create(m_context, "body", m_llvm_func);
	m_end = BasicBlock::Create(m_context, "end", m_llvm_func);
	
	IRBuilder<> builder(entry_block);
	m_ir = &builder;

	m_ir->CreateBr(m_body);
	builder.SetInsertPoint(m_body);

	//set up vars
	m_addr = builder.CreateAlloca(Type::getInt32Ty(m_context), nullptr, "addr");

	//set up blocks
	std::vector<BasicBlock*> pos_labels{ 0x10000 };
	this->m_blocks = pos_labels.data();

	for (const u32 addr : f.blocks)
	{
		if (addr < f.addr || addr >= f.addr + f.size || addr % 4)
		{
			fmt::throw_exception("Invalid function block entry (0x%05x)" HERE, addr);
		}

		BasicBlock* block = BasicBlock::Create(m_context, fmt::format("block_%llx", addr / 4), m_llvm_func);
		pos_labels[addr / 4] = block;
	}

	pos_labels[(f.addr + f.size) / 4 % 0x10000] = BasicBlock::Create(m_context, fmt::format("block_%llx", (f.addr + f.size) / 4 % 0x10000), m_llvm_func);

	std::string result;
	raw_string_ostream out(result);

#if 0
	m_pos = f.addr;

	for (const u32 op : f.data)
	{
		//spew disasm
		dis_asm.dump_pc = m_pos;
		dis_asm.disasm(m_pos);
		out << dis_asm.last_opcode << '\n';

		m_pos += 4;
	}

	fs::file(func_name + "_disasm.log", fs::rewrite).write(out.str());
	result.clear();
#endif

	m_pos = f.addr;
		
	for (const u32 op : f.data)
	{
		if (m_blocks[m_pos / 4] != nullptr)
		{
			//switch to new block
			BasicBlock *block = m_blocks[m_pos / 4];
			m_ir->CreateBr(block); 
			m_ir->SetInsertPoint(block);
		}

#if 0
		//spew disasm
		dis_asm.dump_pc = m_pos;
		dis_asm.disasm(m_pos);
		out << dis_asm.last_opcode << '\n';
#endif

		BasicBlock *insert_block = m_ir->GetInsertBlock();
		u32 old_instr_end = insert_block->getInstList().size();

		(this->*s_spu_decoder.decode(op))({ op });

		u32 new_instr_end = insert_block->getInstList().size();

#if 0
		out << fmt::format("%d -> %d\n", old_instr_end, new_instr_end);

		auto iter = insert_block->begin();
		for (int i = 0; i < new_instr_end; i++)
		{
			if (i >= old_instr_end)
			{
				out << "//LLVM IR: " << *iter << "\n";
			}

			iter++;
		}
#endif

		m_pos += 4;
	}

#if 0
	fs::file(func_name + "_disasm_ir.log", fs::rewrite).write(out.str());
	result.clear();
#endif

	// Generate default function end (go to the next address)
	Value *new_addr = ConstantInt::get(Type::getInt32Ty(m_context), spu_branch_target(m_pos));
	m_ir->CreateBr(m_blocks[m_pos / 4 % 0x10000]);
	m_ir->SetInsertPoint(m_blocks[m_pos / 4 % 0x10000]);
	m_ir->CreateStore(new_addr, m_addr);
	m_ir->CreateBr(m_end);

	m_ir->SetInsertPoint(m_end);
	m_ir->CreateRet(m_ir->CreateLoad(m_addr));

#if 0
	out << *m_module; // print IR
	fs::file(func_name + ".log", fs::rewrite).write(out.str());
	result.clear();
#endif

	//LOG_SUCCESS(SPU, "Finished generating LLVM IR for function %s", func_name);

#if 0
	if (verifyModule(*m_module, &out))
	{
		out.flush();
		fs::file(func_name + "_errors.log", fs::rewrite).write(out.str());
		LOG_ERROR(PPU, "LLVM: Verification failed for %s:\n%s", func_name, result);
		Emu.CallAfter([] { Emu.Stop(); });
		return;
	}
#endif

	legacy::FunctionPassManager fpm(m_module.get());
	fpm.add(createPromoteMemoryToRegisterPass());
	//fpm.add(createInstructionSimplifierPass());
	//fpm.add(createInstructionCombiningPass());
	//fpm.add(createDeadCodeEliminationPass());
	fpm.add(createNewGVNPass());
	fpm.add(createCFGSimplificationPass());
	
	fpm.add(createBBVectorizePass());
	
	fpm.run(*m_llvm_func);

	//LOG_SUCCESS(SPU, "Finished generating code for function %s - code @ 0x%llx", func_name, f.compiled);
	return m_llvm_func;
}

Value* spu_llvm_recompiler::PtrPC()
{
	u32 offset = offset32(&SPUThread::pc);
	Value *cpu_ptr = m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context));
	Value *ptr = m_ir->CreateGEP(cpu_ptr, m_ir->getInt64(offset));
	return m_ir->CreateBitCast(ptr, PointerType::getInt32PtrTy(m_context));
}

Value* spu_llvm_recompiler::PtrGPR8(u8 reg, Value* offset)
{
	Value *gpr_ptr = m_ir->CreateBitCast(PtrGPR128(reg), PointerType::getInt8PtrTy(m_context));
	gpr_ptr = m_ir->CreateGEP(gpr_ptr, m_ir->CreateZExtOrBitCast(offset, m_ir->getInt64Ty()));
	return gpr_ptr;
}

Value* spu_llvm_recompiler::PtrGPR8(u8 reg, s32 offset)
{
	return PtrGPR8(reg, m_ir->getInt32(offset));
}

Value* spu_llvm_recompiler::PtrGPR16(u8 reg, Value* offset)
{
	Value *gpr_ptr = m_ir->CreateBitCast(PtrGPR128(reg), PointerType::getInt16PtrTy(m_context));
	gpr_ptr = m_ir->CreateGEP(gpr_ptr, m_ir->CreateZExtOrBitCast(offset, m_ir->getInt64Ty()));
	return gpr_ptr;
}

Value* spu_llvm_recompiler::PtrGPR16(u8 reg, s32 offset)
{
	return PtrGPR16(reg, m_ir->getInt32(offset));
}

Value* spu_llvm_recompiler::PtrGPR32(u8 reg, Value* offset)
{
	Value *gpr_ptr = m_ir->CreateBitCast(PtrGPR128(reg), PointerType::getInt32PtrTy(m_context));
	gpr_ptr = m_ir->CreateGEP(gpr_ptr, m_ir->CreateZExtOrBitCast(offset, m_ir->getInt64Ty()));
	return gpr_ptr;
}

Value* spu_llvm_recompiler::PtrGPR32(u8 reg, s32 offset)
{
	return PtrGPR32(reg, m_ir->getInt32(offset));
}

Value* spu_llvm_recompiler::PtrGPR64(u8 reg, Value* offset)
{
	Value *gpr_ptr = m_ir->CreateBitCast(PtrGPR128(reg), PointerType::getInt64PtrTy(m_context));
	gpr_ptr = m_ir->CreateGEP(gpr_ptr, m_ir->CreateZExtOrBitCast(offset, m_ir->getInt64Ty()));
	return gpr_ptr;
}

Value* spu_llvm_recompiler::PtrGPR128(u8 reg)
{
	u32 offset = offset32(&SPUThread::gpr, reg);
	Value *cpu_ptr = m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context));
	cpu_ptr = m_ir->CreateGEP(cpu_ptr, m_ir->getInt64(offset));
	return m_ir->CreateBitCast(cpu_ptr, PointerType::getIntNPtrTy(m_context, 128));
}

Value* spu_llvm_recompiler::PtrGPR32(u8 reg)
{
	return PtrGPR32(reg, 0);
}

Value* spu_llvm_recompiler::PtrLS(Value *ls_addr)
{
	Value *ls_ptr = m_ir->CreateBitCast(m_ls, m_ir->getInt8PtrTy());
	ls_ptr = m_ir->CreateGEP(ls_ptr, m_ir->CreateZExtOrBitCast(ls_addr, m_ir->getInt64Ty()));
	return ls_ptr;
}

Value* spu_llvm_recompiler::LoadLS(Value *ls_addr, Type *type)
{
	Value *ls_ptr = PtrLS(ls_addr);
	ls_ptr = m_ir->CreateBitCast(ls_ptr, PointerType::get(type, 0));
	return m_ir->CreateLoad(ls_ptr);
}

void spu_llvm_recompiler::StoreLS(Value *ls_addr, Value *value)
{
	Type *type = value->getType();
	Value *ls_ptr = PtrLS(ls_addr);
	ls_ptr = m_ir->CreateBitCast(ls_ptr, PointerType::get(value->getType(), 0));
	m_ir->CreateStore(value, ls_ptr);
}

Value* spu_llvm_recompiler::LoadGPR8(u8 reg, u32 offset)
{
	LoadInst *load = m_ir->CreateLoad(PtrGPR8(reg, offset));
	//MDNode *md = MDNode::get(m_context, { ConstantAsMetadata::get(m_ir->getInt32(m_invariant_group)) });
	//load->setMetadata("invariant.group", md);
	return load;
}

Value* spu_llvm_recompiler::LoadGPR16(u8 reg, u32 offset)
{
	LoadInst *load = m_ir->CreateLoad(PtrGPR16(reg, offset));
	//MDNode *md = MDNode::get(m_context, { ConstantAsMetadata::get(m_ir->getInt32(m_invariant_group)) });
	//load->setMetadata("invariant.group", md);
	return load;
}

Value* spu_llvm_recompiler::LoadGPR32(u8 reg, u32 offset)
{
	LoadInst *load = m_ir->CreateLoad(PtrGPR32(reg, offset));
	//MDNode *md = MDNode::get(m_context, { ConstantAsMetadata::get(m_ir->getInt32(m_invariant_group)) });
	//load->setMetadata("invariant.group", md);
	return load;
}

Value* spu_llvm_recompiler::LoadGPR128(u8 reg)
{
	LoadInst *load = m_ir->CreateLoad(PtrGPR128(reg));
	//MDNode *md = MDNode::get(m_context, { ConstantAsMetadata::get(m_ir->getInt32(m_invariant_group)) });
	//load->setMetadata("invariant.group", md);
	return load;
}

Value* spu_llvm_recompiler::LoadGPR128(u8 reg, Type* type)
{
	return m_ir->CreateBitCast(LoadGPR128(reg), type);
}

Value* spu_llvm_recompiler::LoadGPRVec2(u8 reg)
{
	return m_ir->CreateBitCast(LoadGPR128(reg), VectorType::get(Type::getInt64Ty(m_context), 2));
}

Value* spu_llvm_recompiler::LoadGPRVec2Double(u8 reg)
{
	return m_ir->CreateBitCast(LoadGPR128(reg), VectorType::get(Type::getDoubleTy(m_context), 2));
}

Value* spu_llvm_recompiler::LoadGPRVec4(u8 reg)
{
	return m_ir->CreateBitCast(LoadGPR128(reg), VectorType::get(Type::getInt32Ty(m_context), 4));
}

Value* spu_llvm_recompiler::LoadGPRVec4Float(u8 reg)
{
	return m_ir->CreateBitCast(LoadGPR128(reg), VectorType::get(Type::getFloatTy(m_context), 4));
}

Value* spu_llvm_recompiler::LoadGPRVec8(u8 reg)
{
	return m_ir->CreateBitCast(LoadGPR128(reg), VectorType::get(Type::getInt16Ty(m_context), 8));
}

Value* spu_llvm_recompiler::LoadGPRVec16(u8 reg)
{
	return m_ir->CreateBitCast(LoadGPR128(reg), VectorType::get(Type::getInt8Ty(m_context), 16));
}

void spu_llvm_recompiler::Store128(Value *ptr, Value *value, int invariant_group)
{
	StoreInst *store = m_ir->CreateStore(m_ir->CreateBitCast(value, m_ir->getInt128Ty()), ptr);
	/*if (invariant_group >= 0)
	{
		MDNode *md = MDNode::get(m_context, { ConstantAsMetadata::get(m_ir->getInt32(m_invariant_group)) });
		store->setMetadata("invariant.group", md);
	}*/
}

void spu_llvm_recompiler::StoreGPR128(u8 reg, __m128i data)
{
	Value *vdata = GetInt128(data);
	Store128(PtrGPR128(reg), vdata/*, m_invariant_group*/);
}

void spu_llvm_recompiler::StoreGPR128(u8 reg, Value *value)
{
	Store128(PtrGPR128(reg), value);
}

void spu_llvm_recompiler::StoreGPR64(u8 reg, Value* data, Value* offset)
{
	Value *vr = PtrGPR64(reg, offset);
	m_ir->CreateStore(data, vr);
}

void spu_llvm_recompiler::StoreGPR64(u8 reg, Value* data, u32 offset)
{
	StoreGPR64(reg, data, m_ir->getInt32(offset));
}

void spu_llvm_recompiler::StoreGPR64(u8 reg, u64 data, Value* offset)
{
	StoreGPR64(reg, m_ir->getInt64(data), offset);
}

void spu_llvm_recompiler::StoreGPR64(u8 reg, u64 data, u32 offset)
{
	StoreGPR64(reg, data, m_ir->getInt32(offset));
}

void spu_llvm_recompiler::StoreGPR32(u8 reg, Value* value, Value* offset)
{
	//Value *vr = PtrGPR32(reg, offset);
	//m_ir->CreateStore(value, vr);

	Value *vec = GetVec4(_mm_set1_epi32(0));
	vec = m_ir->CreateInsertElement(vec, value, offset);
	StoreGPR128(reg, vec);
}

void spu_llvm_recompiler::StoreGPR32(u8 reg, Value* value, u32 offset)
{
	StoreGPR32(reg, value, m_ir->getInt32(offset));
}

void spu_llvm_recompiler::StoreGPR32(u8 reg, u32 data, Value* offset)
{
	StoreGPR32(reg, m_ir->getInt32(data), offset);
}

void spu_llvm_recompiler::StoreGPR16(u8 reg, Value* data, Value* offset)
{
	Value *vr = PtrGPR16(reg, offset);
	m_ir->CreateStore(data, vr);
}

void spu_llvm_recompiler::StoreGPR16(u8 reg, u16 data, Value* offset)
{
	StoreGPR16(reg, m_ir->getInt16(data), offset);
}

void spu_llvm_recompiler::StoreGPR8(u8 reg, Value* data, Value* offset)
{
	Value *vr = PtrGPR8(reg, offset);
	m_ir->CreateStore(data, vr);
}

void spu_llvm_recompiler::StoreGPR32(u8 reg, u32 data, u32 offset)
{
	StoreGPR32(reg, data, m_ir->getInt32(offset));
}

Value* spu_llvm_recompiler::Intrinsic_minnum(Value *v0, Value *v1)
{
	Function *minnum = Intrinsic::getDeclaration(m_module.get(), Intrinsic::minnum);
	return m_ir->CreateCall(minnum, { v0, v1 });
}

Value* spu_llvm_recompiler::Intrinsic_maxnum(Value *v0, Value *v1)
{
	Function *maxxnum = Intrinsic::getDeclaration(m_module.get(), Intrinsic::maxnum);
	return m_ir->CreateCall(maxxnum, { v0, v1 });
}

Value* spu_llvm_recompiler::Intrinsic_sqrt(Value *v0)
{
	std::vector<Type*> sqrt_types = { VectorType::get(m_ir->getFloatTy(), 4) };
	Function *sqrt = Intrinsic::getDeclaration(m_module.get(), Intrinsic::sqrt, sqrt_types);
	return m_ir->CreateCall(sqrt, { v0 });
}

#define USE_X86_INTRINSICS 0

Value* spu_llvm_recompiler::Intrinsic_pshufb(Value *v0, Value *mask)
{
#if USE_X86_INTRINSICS
	Function *pshufb = Intrinsic::getDeclaration(m_module.get(), Intrinsic::x86_ssse3_pshuf_b_128);
	std::vector<Value*> pshufb_args = { v0, mask };
	return m_ir->CreateCall(pshufb, pshufb_args);
#else
	Value *result = v0;
	Value *msb = m_ir->CreateAnd(mask, GetVec16(_mm_set1_epi8(0x80)));
	Value *index = m_ir->CreateAnd(mask, GetVec16(_mm_set1_epi8(0xF)));
	Value *cmp = m_ir->CreateICmpEQ(msb, GetVec16(_mm_set1_epi8(0x80)));
	for (int i = 0; i < 16; i++)
	{
		Value *new_dest = m_ir->CreateExtractElement(v0, m_ir->CreateExtractElement(index, i));
		Value *res = m_ir->CreateSelect(m_ir->CreateExtractElement(cmp, i), m_ir->getInt8(0), new_dest);
		result = m_ir->CreateInsertElement(result, res, i);
	}

	return result;
#endif
}

Value* spu_llvm_recompiler::Intrinsic_pshufb(Value *v0, std::vector<u32> mask)
{
	std::reverse(mask.begin(), mask.end());
	return m_ir->CreateShuffleVector(v0, UndefValue::get(v0->getType()), mask);
}

Value* spu_llvm_recompiler::Intrinsic_rcpps(Value *v0)
{
	Function *rcpps = Intrinsic::getDeclaration(m_module.get(), Intrinsic::x86_sse_rcp_ps);

	std::vector<Value*> rcpps_args = { v0 };
	return m_ir->CreateCall(rcpps, rcpps_args);
}

Value* spu_llvm_recompiler::GetInt128(__m128i n)
{
	std::vector<__m128i> c1_data = { n };
	return m_ir->getInt(APInt(128, ArrayRef<u64>(reinterpret_cast<u64*>(c1_data.data()), 2)));
}

Value* spu_llvm_recompiler::GetInt128(__m128i n, Type *type)
{
	return m_ir->CreateBitCast(GetInt128(n), type);
}

Value* spu_llvm_recompiler::GetVec2(__m128i n)
{
	return GetInt128(n, VectorType::get(m_ir->getInt64Ty(), 2));
}

Value* spu_llvm_recompiler::GetVec2Double(__m128i n)
{
	return GetInt128(n, VectorType::get(m_ir->getDoubleTy(), 2));
}

Value* spu_llvm_recompiler::GetVec2Double(__m128d n)
{
	return GetVec2Double(_mm_castpd_si128(n));
}

Value* spu_llvm_recompiler::GetVec4(__m128i n)
{
	return GetInt128(n, VectorType::get(m_ir->getInt32Ty(), 4));
}

Value* spu_llvm_recompiler::GetVec4Float(__m128i n)
{
	return GetInt128(n, VectorType::get(m_ir->getFloatTy(), 4));
}

Value* spu_llvm_recompiler::GetVec4Float(__m128 n)
{
	return GetVec4Float(_mm_castps_si128(n));
}

Value* spu_llvm_recompiler::GetVec8(__m128i n)
{
	return GetInt128(n, VectorType::get(m_ir->getInt16Ty(), 8));
}

Value* spu_llvm_recompiler::GetVec16(__m128i n)
{
	return GetInt128(n, VectorType::get(m_ir->getInt8Ty(), 16));
}

Value* spu_llvm_recompiler::ZExt128(Value *va)
{
	return m_ir->CreateZExt(va, m_ir->getInt128Ty());
}

void spu_llvm_recompiler::CreateJumpTableSwitch(Value *_addr)
{
	m_ir->CreateStore(_addr, m_addr);

	if (m_func->jtable.size() > 0)
	{
		SwitchInst *jt_switch = m_ir->CreateSwitch(_addr, m_end, m_func->jtable.size());
		for (const u32 addr : m_func->jtable)
		{
			if ((addr % 4) == 0 && addr < 0x40000 && m_blocks[addr / 4] != nullptr)
			{
				jt_switch->addCase(m_ir->getInt32(addr), m_blocks[addr / 4]);
			}
		}
	}
	else
	{
		m_ir->CreateBr(m_end);
	}
}

void spu_llvm_recompiler::FunctionCall()
{
	const u32 target = spu_branch_target(m_pos + 4);
#if 0
	bool is_adjacent = false;
	for (const u32 adjacent : m_func->adjacent)
	{
		if (target == adjacent)
		{
			is_adjacent = true;
			break;
		}
	}

	Function *adjacent = m_compiled_adjacent_funcs[target];
	is_adjacent = is_adjacent && adjacent != nullptr;

	if (is_adjacent)
	{
		m_ir->CreateCall(adjacent, { m_cpu, m_ls });
	}
	else
#endif
	{
		Value *new_addr = m_ir->CreateCall(m_spufunccall, { m_cpu, m_ir->getInt32(target) });
		m_ir->CreateStore(new_addr, m_addr);

		BasicBlock *cont = BasicBlock::Create(m_context, "__fcall_cont", m_llvm_func);
		m_ir->CreateCondBr(m_ir->CreateICmpNE(new_addr, m_ir->getInt32(0)), m_end, cont);
		m_ir->SetInsertPoint(cont);
	}
}

void spu_llvm_recompiler::InterpreterCall(spu_opcode_t op)
{
	m_ir->CreateStore(m_ir->getInt32(m_pos), PtrPC());

	Constant *fptri = m_ir->getInt64((u64)s_spu_interpreter.decode(op.opcode));
	fptri = ConstantExpr::getIntToPtr(fptri, m_ir->getInt8PtrTy());

	std::vector<Value*> args = { m_cpu, m_ir->getInt32(op.opcode), fptri };
	Value *new_addr = m_ir->CreateCall(m_spuintcall, args);
	m_ir->CreateStore(new_addr, m_addr);

	BasicBlock *cont = BasicBlock::Create(m_context, "__interp_cont", m_llvm_func);
	m_ir->CreateCondBr(m_ir->CreateICmpNE(new_addr, m_ir->getInt32(0)), m_end, cont);
	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::STOP(spu_opcode_t op)
{
	InterpreterCall(op);
}

void spu_llvm_recompiler::LNOP(spu_opcode_t op)
{
}

void spu_llvm_recompiler::SYNC(spu_opcode_t op)
{
	m_ir->CreateFence(AtomicOrdering::SequentiallyConsistent);
}

void spu_llvm_recompiler::DSYNC(spu_opcode_t op)
{
	m_ir->CreateFence(AtomicOrdering::SequentiallyConsistent);
}

void spu_llvm_recompiler::MFSPR(spu_opcode_t op)
{
	InterpreterCall(op); //TODO
}

void spu_llvm_recompiler::RDCH(spu_opcode_t op)
{
	switch (op.ra)
	{
	case SPU_RdSRR0:
	{
		u32 offset = offset32(&SPUThread::srr0);
		Value *srr0 = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
		Value *vr = m_ir->CreateShl(ZExt128(m_ir->CreateLoad(srr0)), 12);
		StoreGPR128(op.rt, vr);
		return;
	}
	case MFC_RdTagMask:
	{
		u32 offset = offset32(&SPUThread::ch_tag_mask);
		Value *ch_tag_mask = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
		Value *vr = m_ir->CreateShl(ZExt128(m_ir->CreateLoad(ch_tag_mask)), 12);
		StoreGPR128(op.rt, vr);
		return;
	}
	case SPU_RdEventMask:
	{
		u32 offset = offset32(&SPUThread::ch_event_mask);
		Value *ch_event_mask = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
		Value *vr = m_ir->CreateShl(ZExt128(m_ir->CreateLoad(ch_event_mask)), 12);
		StoreGPR128(op.rt, vr);
		return;
	}
	default:
	{
		InterpreterCall(op); // TODO
	}
	}
}

void spu_llvm_recompiler::RCHCNT(spu_opcode_t op)
{
	InterpreterCall(op); // TODO
}

void spu_llvm_recompiler::SF(spu_opcode_t op)
{
	Value *rb = LoadGPRVec4(op.rb);
	Value *ra = LoadGPRVec4(op.ra);
	Value *rt = m_ir->CreateSub(rb, ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::OR(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	Value *rb = LoadGPR128(op.rb);
	Value *result = m_ir->CreateOr(ra, rb);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::BG(spu_opcode_t op)
{
	Value *mask = GetVec4(_mm_set1_epi32(0x80000000));
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	ra = m_ir->CreateXor(ra, mask);
	rb = m_ir->CreateXor(rb, mask);
	Value *result = m_ir->CreateSelect(m_ir->CreateICmpSGT(ra, rb), GetVec4(_mm_set1_epi32(0)), GetVec4(_mm_set1_epi32(1)));

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::SFH(spu_opcode_t op)
{
	Value *rb = LoadGPRVec8(op.rb);
	Value *ra = LoadGPRVec8(op.ra);
	Value *rt = m_ir->CreateSub(rb, ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::NOR(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *result = m_ir->CreateOr(ra, rb);
	result = m_ir->CreateNot(result);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ABSDB(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *rb = LoadGPRVec16(op.rb);
	Value *max = Intrinsic_maxnum(ra, rb);
	Value *min = Intrinsic_minnum(rb, ra);
	Value *result = m_ir->CreateSub(max, min);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ROT(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	rb = m_ir->CreateAnd(rb, GetVec4(_mm_set1_epi32(0x1f)));

	Value *result = m_ir->CreateShl(ra, rb);
	Value *result2 = m_ir->CreateLShr(ra, m_ir->CreateSub(GetVec4(_mm_set1_epi32(32)), rb));
	result = m_ir->CreateOr(result, result2);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ROTM(spu_opcode_t op)
{
	Value *zero = GetVec4(_mm_set1_epi32(0));
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	rb = m_ir->CreateSub(zero, rb);
	rb = m_ir->CreateAnd(rb, GetVec4(_mm_set1_epi32(0x3f)));

	Value *result = m_ir->CreateLShr(ra, rb);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ROTMA(spu_opcode_t op)
{
	Value *zero = GetVec4(_mm_set1_epi32(0));
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	rb = m_ir->CreateSub(zero, rb);
	rb = m_ir->CreateAnd(rb, GetVec4(_mm_set1_epi32(0x3f)));

	Value *result = m_ir->CreateAShr(ra, rb);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::SHL(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *rt = m_ir->CreateShl(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTH(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = LoadGPRVec8(op.rb);
	rb = m_ir->CreateAnd(rb, GetVec8(_mm_set1_epi16(0xf)));

	Value *result = m_ir->CreateShl(ra, rb);
	result = m_ir->CreateOr(result, m_ir->CreateLShr(ra, m_ir->CreateSub(m_ir->getInt16(16), rb)));

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ROTHM(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = LoadGPRVec8(op.rb);
	rb = m_ir->CreateAnd(m_ir->CreateSub(GetVec8(_mm_set1_epi16(0)), rb), GetVec8(_mm_set1_epi16(0x1f)));
	Value *rt = m_ir->CreateLShr(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTMAH(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = LoadGPRVec8(op.rb);
	rb = m_ir->CreateAnd(m_ir->CreateSub(GetVec8(_mm_set1_epi16(0)), rb), GetVec8(_mm_set1_epi16(0x1f)));
	Value *rt = m_ir->CreateAShr(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SHLH(spu_opcode_t op)
{	
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = LoadGPRVec8(op.rb);
	Value *rt = m_ir->CreateShl(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *shift = GetVec4(_mm_set1_epi32(op.i7 & 0x1f));
	Value *shift2 = m_ir->CreateSub(GetVec4(_mm_set1_epi32(32)), shift);

	Value *result = m_ir->CreateShl(ra, shift);
	result = m_ir->CreateOr(result, m_ir->CreateLShr(ra, shift2));

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ROTMI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = GetVec4(_mm_set1_epi32((0 - op.i7) & 0x3f));

	Value *result = m_ir->CreateLShr(ra, rb);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ROTMAI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = GetVec4(_mm_set1_epi32((0 - op.i7) & 0x3f));

	Value *result = m_ir->CreateAShr(ra, rb);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::SHLI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *shift = GetVec4(_mm_set1_epi32(op.i7 & 0x3f));
	Value *rt = m_ir->CreateShl(ra, shift);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTHI(spu_opcode_t op)
{
	const int s = op.i7 & 0xf;
	const int s_inv = 16 - s;
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = GetVec8(_mm_set1_epi16(s));

	Value *result = m_ir->CreateShl(ra, rb);
	result = m_ir->CreateOr(result, m_ir->CreateLShr(ra, GetVec8(_mm_set1_epi16(s_inv))));

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ROTHMI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	const int s = (0 - op.i7) & 0x1f;
	Value *rt = m_ir->CreateLShr(ra, GetVec8(_mm_set1_epi16(s)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTMAHI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	const int s = (0 - op.i7) & 0x1f;
	Value *rt = m_ir->CreateAShr(ra, GetVec8(_mm_set1_epi16(s)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SHLHI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *shift = GetVec8(_mm_set1_epi16(op.i7 & 0x1f));
	Value *rt = m_ir->CreateShl(ra, shift);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::A(spu_opcode_t op)
{
	Value *rb = LoadGPRVec4(op.rb);
	Value *ra = LoadGPRVec4(op.ra);
	Value *rt = m_ir->CreateAdd(rb, ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::AND(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	Value *rb = LoadGPR128(op.rb);
	Value *result = m_ir->CreateAnd(ra, rb);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CG(spu_opcode_t op)
{
	Value *mask = GetVec4(_mm_set1_epi32(0x80000000));
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *result = m_ir->CreateAdd(rb, ra);
	ra = m_ir->CreateXor(ra, mask);
	result = m_ir->CreateXor(result, mask);
	result = m_ir->CreateSelect(m_ir->CreateICmpSGT(ra, result), GetVec4(_mm_set1_epi32(1)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::AH(spu_opcode_t op)
{
	Value *rb = LoadGPRVec8(op.rb);
	Value *ra = LoadGPRVec8(op.ra);
	Value *rt = m_ir->CreateAdd(rb, ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::NAND(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *result = m_ir->CreateAnd(ra, rb);
	result = m_ir->CreateNot(result);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::AVGB(spu_opcode_t op)
{
	Value *one = GetVec16(_mm_set1_epi8(1));
	Value *one_wide = m_ir->CreateZExt(GetVec16(_mm_set1_epi8(1)), VectorType::get(m_ir->getInt16Ty(), 16));
	Value *ra = LoadGPRVec16(op.ra);
	Value *rb = LoadGPRVec16(op.rb);
	ra = m_ir->CreateZExt(ra, VectorType::get(m_ir->getInt16Ty(), 16));
	rb = m_ir->CreateZExt(ra, VectorType::get(m_ir->getInt16Ty(), 16));

	Value *result = m_ir->CreateAdd(ra, rb);
	result = m_ir->CreateAdd(result, one_wide);
	result = m_ir->CreateLShr(result, one_wide);
	result = m_ir->CreateTrunc(result, VectorType::get(m_ir->getInt8Ty(), 16));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::MTSPR(spu_opcode_t op)
{
	InterpreterCall(op); //TODO
}

void spu_llvm_recompiler::WRCH(spu_opcode_t op)
{
	switch (op.ra)
	{
	case SPU_WrSRR0:
	{
		Value *rt = LoadGPR32(op.rt, 3);
		u32 offset = offset32(&SPUThread::srr0);
		Value *srr0 = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
		m_ir->CreateStore(rt, srr0);
		break;
	}
	case MFC_WrTagMask:
	{
		Value *rt = LoadGPR32(op.rt, 3);
		u32 offset = offset32(&SPUThread::ch_tag_mask);
		Value *ch_tag_mask = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
		m_ir->CreateStore(rt, ch_tag_mask);
		return;
	}
	case MFC_LSA:
	{
		Value *rt = LoadGPR32(op.rt, 3);
		u32 offset = offset32(&SPUThread::ch_mfc_cmd, &spu_mfc_cmd::lsa);
		Value *ch_mfc_cmd = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
		m_ir->CreateStore(rt, ch_mfc_cmd);
		return;
	}
	case MFC_EAH:
	{
		Value *rt = LoadGPR32(op.rt, 3);
		u32 offset = offset32(&SPUThread::ch_mfc_cmd, &spu_mfc_cmd::eah);
		Value *ch_mfc_cmd = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
		m_ir->CreateStore(rt, ch_mfc_cmd);
		return;
	}
	case MFC_EAL:
	{
		Value *rt = LoadGPR32(op.rt, 3);
		u32 offset = offset32(&SPUThread::ch_mfc_cmd, &spu_mfc_cmd::eal);
		Value *ch_mfc_cmd = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
		m_ir->CreateStore(rt, ch_mfc_cmd);
		return;
	}
	case MFC_Size:
	{
		Value *rt = LoadGPR32(op.rt, 3);
		u32 offset = offset32(&SPUThread::ch_mfc_cmd, &spu_mfc_cmd::size);
		Value *ch_mfc_cmd = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt16PtrTy(m_context));
		m_ir->CreateStore(m_ir->CreateTrunc(rt, m_ir->getInt16Ty()), ch_mfc_cmd);
		return;
	}
	case MFC_TagID:
	{
		Value *rt = LoadGPR32(op.rt, 3);
		u32 offset = offset32(&SPUThread::ch_mfc_cmd, &spu_mfc_cmd::tag);
		Value *ch_mfc_cmd = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset));
		m_ir->CreateStore(m_ir->CreateTrunc(rt, m_ir->getInt8Ty()), ch_mfc_cmd);
		return;
	}
	case 69:
	{
		return;
	}
	default:
	{
		InterpreterCall(op); // TODO
	}
	}
}

void spu_llvm_recompiler::BIZ(spu_opcode_t op)
{
	Value *ra = m_ir->CreateAnd(LoadGPR32(op.ra, 3), 0x3fffc);
	if (op.d || op.e) 
		ra = m_ir->CreateOr(ra, op.e << 26 | op.d << 27);

	Value *rt = LoadGPR32(op.rt, 3);
	Value *cmp = m_ir->CreateICmpEQ(rt, m_ir->getInt32(0));

	BasicBlock *cont = BasicBlock::Create(m_context, "__biz_cont", m_llvm_func);
	BasicBlock *indir = BasicBlock::Create(m_context, "__biz_indir", m_llvm_func);
	m_ir->CreateCondBr(cmp, indir, cont);
	
	m_ir->SetInsertPoint(indir);
	CreateJumpTableSwitch(ra);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::BINZ(spu_opcode_t op)
{
	Value *ra = m_ir->CreateAnd(LoadGPR32(op.ra, 3), 0x3fffc);
	if (op.d || op.e)
		ra = m_ir->CreateOr(ra, op.e << 26 | op.d << 27);

	Value *rt = LoadGPR32(op.rt, 3);
	Value *cmp = m_ir->CreateICmpNE(rt, m_ir->getInt32(0));

	BasicBlock *cont = BasicBlock::Create(m_context, "__binz_cont", m_llvm_func);
	BasicBlock *indir = BasicBlock::Create(m_context, "__binz_indir", m_llvm_func);
	m_ir->CreateCondBr(cmp, indir, cont);

	m_ir->SetInsertPoint(indir);
	CreateJumpTableSwitch(ra);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::BIHZ(spu_opcode_t op)
{
	Value *ra = m_ir->CreateAnd(LoadGPR32(op.ra, 3), 0x3fffc);
	if (op.d || op.e)
		ra = m_ir->CreateOr(ra, op.e << 26 | op.d << 27);

	Value *rt = LoadGPR16(op.rt, 6);
	Value *cmp = m_ir->CreateICmpEQ(rt, m_ir->getInt16(0));

	BasicBlock *cont = BasicBlock::Create(m_context, "__bihz_cont", m_llvm_func);
	BasicBlock *indir = BasicBlock::Create(m_context, "__bihz_indir", m_llvm_func);
	m_ir->CreateCondBr(cmp, indir, cont);

	m_ir->SetInsertPoint(indir);
	CreateJumpTableSwitch(ra);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::BIHNZ(spu_opcode_t op)
{
	Value *ra = m_ir->CreateAnd(LoadGPR32(op.ra, 3), 0x3fffc);
	if (op.d || op.e)
		ra = m_ir->CreateOr(ra, op.e << 26 | op.d << 27);

	Value *rt = LoadGPR16(op.rt, 6);
	Value *cmp = m_ir->CreateICmpNE(rt, m_ir->getInt16(0));

	BasicBlock *cont = BasicBlock::Create(m_context, "__bihnz_cont", m_llvm_func);
	BasicBlock *indir = BasicBlock::Create(m_context, "__bihnz_indir", m_llvm_func);
	m_ir->CreateCondBr(cmp, indir, cont);

	m_ir->SetInsertPoint(indir);
	CreateJumpTableSwitch(ra);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::STOPD(spu_opcode_t op)
{
	InterpreterCall(op); //TODO
}

void spu_llvm_recompiler::STQX(spu_opcode_t op)
{
	Value *rt_contents = LoadGPRVec16(op.rt);
	rt_contents = Intrinsic_pshufb(rt_contents, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);
	Value *addr = m_ir->CreateAdd(ra, rb);
	addr = m_ir->CreateAnd(addr, m_ir->getInt32(0x3fff0));

	StoreLS(addr, rt_contents);
}

void spu_llvm_recompiler::BI(spu_opcode_t op)
{
	Value *ra = m_ir->CreateAnd(LoadGPR32(op.ra, 3), 0x3fffc);
	if (op.d || op.e)
		ra = m_ir->CreateOr(ra, op.e << 26 | op.d << 27);

	CreateJumpTableSwitch(ra);

	BasicBlock *cont = BasicBlock::Create(m_context, "__bi_cont", m_llvm_func);
	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::BISL(spu_opcode_t op)
{
	Value *addr = LoadGPR32(op.ra, 3);
	addr = m_ir->CreateAnd(addr, m_ir->getInt32(0x3fffc));
	if (op.d || op.e) 
		addr = m_ir->CreateAnd(addr, m_ir->getInt32(op.e << 26 | op.d << 27)); // interrupt flags stored to PC

	m_ir->CreateStore(addr, PtrPC());

	Value *vr = GetInt128(_mm_set_epi32(spu_branch_target(m_pos + 4), 0, 0, 0));
	StoreGPR128(op.rt, vr);

	FunctionCall();
}

void spu_llvm_recompiler::IRET(spu_opcode_t op)
{
	u32 offset = offset32(&SPUThread::srr0);
	Value *srr0 = m_ir->CreateBitCast(m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy()), m_ir->getInt64(offset)), PointerType::getInt32PtrTy(m_context));
	srr0 = m_ir->CreateLoad(srr0);
	srr0 = m_ir->CreateAnd(srr0, 0x3fffc);
	if (op.d || op.e) 
		m_ir->CreateOr(srr0, op.e << 26 | op.d << 27);

	CreateJumpTableSwitch(srr0);

	BasicBlock *cont = BasicBlock::Create(m_context, "__iret_cont", m_llvm_func);
	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::BISLED(spu_opcode_t op)
{
	fmt::throw_exception("Unimplemented instruction" HERE);
}

void spu_llvm_recompiler::HBR(spu_opcode_t op)
{
	//do nothing, we don't care about branch hints
}

void spu_llvm_recompiler::GB(spu_opcode_t op)
{
	Value *ra = m_ir->CreateAnd(LoadGPRVec4(op.ra), GetVec4(_mm_set1_epi32(1)));
	Value *result = m_ir->getInt16(0);

	for (int i = 0; i < 4; i++)
	{
		Value *raw = m_ir->CreateTrunc(m_ir->CreateExtractElement(ra, i), m_ir->getInt16Ty());
		raw = m_ir->CreateShl(raw, i);
		result = m_ir->CreateOr(result, raw);
	}

	Value *rt = GetVec8(_mm_set1_epi16(0));
	rt = m_ir->CreateInsertElement(rt, result, 6);

	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::GBH(spu_opcode_t op)
{
	Value *ra = m_ir->CreateAnd(LoadGPRVec8(op.ra), GetVec8(_mm_set1_epi16(1)));
	Value *result = m_ir->getInt16(0);

	for (int i = 0; i < 8; i++)
	{
		Value *raw = m_ir->CreateExtractElement(ra, i);
		raw = m_ir->CreateShl(raw, i);
		result = m_ir->CreateOr(result, raw);
	}

	Value *rt = GetVec8(_mm_set1_epi16(0));
	rt = m_ir->CreateInsertElement(rt, result, 6);

	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::GBB(spu_opcode_t op)
{
	Value *ra = m_ir->CreateAnd(LoadGPRVec16(op.ra), GetVec16(_mm_set1_epi8(1)));
	Value *result = m_ir->getInt16(0);

	for (int i = 0; i < 16; i++)
	{
		Value *raw = m_ir->CreateZExt(m_ir->CreateExtractElement(ra, i), m_ir->getInt16Ty());
		raw = m_ir->CreateShl(raw, i);
		result = m_ir->CreateOr(result, raw);
	}

	Value *rt = GetVec8(_mm_set1_epi16(0));
	rt = m_ir->CreateInsertElement(rt, result, 6);

	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::FSM(spu_opcode_t op)
{
	Value *fsm_ptr = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.fsm), m_ir->getInt8PtrTy());

	Value *ra = LoadGPR32(op.ra, 3);
	ra = m_ir->CreateAnd(ra, 0xf);
	ra = m_ir->CreateShl(ra, 4);
	ra = m_ir->CreateZExt(ra, m_ir->getInt64Ty());

	Value *ptr = m_ir->CreateBitCast(m_ir->CreateGEP(fsm_ptr, ra), PointerType::getIntNPtrTy(m_context, 128));
	StoreGPR128(op.rt, m_ir->CreateLoad(ptr));
}

void spu_llvm_recompiler::FSMH(spu_opcode_t op)
{
	Value *fsm_ptr = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.fsmh), m_ir->getInt8PtrTy());

	Value *ra = LoadGPR32(op.ra, 3);
	ra = m_ir->CreateAnd(ra, 0xff);
	ra = m_ir->CreateShl(ra, 4);
	ra = m_ir->CreateZExt(ra, m_ir->getInt64Ty());

	Value *ptr = m_ir->CreateBitCast(m_ir->CreateGEP(fsm_ptr, ra), PointerType::getIntNPtrTy(m_context, 128));
	StoreGPR128(op.rt, m_ir->CreateLoad(ptr));
}

void spu_llvm_recompiler::FSMB(spu_opcode_t op)
{
	Value *fsm_ptr = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.fsmb), m_ir->getInt8PtrTy());

	Value *ra = LoadGPR32(op.ra, 3);
	ra = m_ir->CreateAnd(ra, 0xffff);
	ra = m_ir->CreateShl(ra, 4);
	ra = m_ir->CreateZExt(ra, m_ir->getInt64Ty());

	Value *ptr = m_ir->CreateBitCast(m_ir->CreateGEP(fsm_ptr, ra), PointerType::getIntNPtrTy(m_context, 128));
	StoreGPR128(op.rt, m_ir->CreateLoad(ptr));
}

void spu_llvm_recompiler::FREST(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4Float(op.ra);
	//Value *one = GetVec4Float(_mm_set1_ps(1.0f));
	//Value *rt = m_ir->CreateFDiv(one, ra);

	Value *rt = Intrinsic_rcpps(ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::FRSQEST(spu_opcode_t op)
{
	//Value *one = GetVec4Float(_mm_set1_ps(1.0f));
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *sqrt = Intrinsic_sqrt(ra);
	//Value *rt = m_ir->CreateFDiv(one, sqrt);
	Value *rt = Intrinsic_rcpps(sqrt);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::LQX(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);

	Value *addr = m_ir->CreateAdd(ra, rb);
	addr = m_ir->CreateAnd(addr, m_ir->getInt32(0x3fff0));

	Value *ls_contents = LoadLS(addr, VectorType::get(m_ir->getInt8Ty(), 16));
	ls_contents = Intrinsic_pshufb(ls_contents, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

	StoreGPR128(op.rt, ls_contents);
}

void spu_llvm_recompiler::ROTQBYBI(spu_opcode_t op)
{
	Value *rb = LoadGPR32(op.rb, 3);
	rb = m_ir->CreateAnd(rb, 0xf << 3);
	rb = m_ir->CreateShl(rb, 1);
	rb = m_ir->CreateZExt(rb, m_ir->getInt64Ty());

	Value *rldq_pshufb = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.rldq_pshufb), m_ir->getInt8PtrTy());
	rldq_pshufb = m_ir->CreateBitCast(m_ir->CreateGEP(rldq_pshufb, rb), PointerType::get(VectorType::get(m_ir->getInt8Ty(), 16), 0));

	Value *ra = LoadGPRVec16(op.ra);
	Value *mask = m_ir->CreateLoad(rldq_pshufb);
	Value *result = Intrinsic_pshufb(ra, mask);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ROTQMBYBI(spu_opcode_t op)
{
	Value *srdq_pshufb = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.srdq_pshufb), m_ir->getInt8PtrTy());

	Value *addr = LoadGPR32(op.rb, 3);
	addr = m_ir->CreateLShr(addr, 3);
	addr = m_ir->CreateNeg(addr);
	addr = m_ir->CreateAnd(addr, 0x1f);
	addr = m_ir->CreateShl(addr, 4);
	addr = m_ir->CreateZExt(addr, m_ir->getInt64Ty());

	srdq_pshufb = m_ir->CreateBitCast(m_ir->CreateGEP(srdq_pshufb, addr), PointerType::get(VectorType::get(m_ir->getInt8Ty(), 16), 0));

	Value *mask = m_ir->CreateLoad(srdq_pshufb);
	Value *ra = LoadGPRVec16(op.ra);
	Value *rt = Intrinsic_pshufb(ra, mask);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SHLQBYBI(spu_opcode_t op)
{
	Value *sldq_pshufb = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.sldq_pshufb), PointerType::get(VectorType::get(m_ir->getInt8Ty(), 16), 0));
	
	Value *addr = LoadGPR32(op.rb, 3);
	addr = m_ir->CreateLShr(addr, 3);
	addr = m_ir->CreateAnd(addr, 0x1f);

	sldq_pshufb = m_ir->CreateGEP(sldq_pshufb, addr);

	Value *mask = m_ir->CreateLoad(sldq_pshufb);
	Value *ra = LoadGPRVec16(op.ra);
	Value *rt = Intrinsic_pshufb(ra, mask);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CBX(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);
	Value *addr = m_ir->CreateAdd(ra, rb);
	addr = m_ir->CreateNot(addr);
	addr = m_ir->CreateAnd(addr, m_ir->getInt32(0xf));
	addr = m_ir->CreateZExt(addr, m_ir->getInt64Ty());

	StoreGPR128(op.rt, _mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	
	Value *addr_ptr = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context)), addr);
	addr_ptr = m_ir->CreateGEP(addr_ptr, m_ir->getInt64(offset32(&SPUThread::gpr, op.rt)));
	m_ir->CreateStore(m_ir->getInt8(0x03), addr_ptr);
}

void spu_llvm_recompiler::CHX(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);
	ra = m_ir->CreateAdd(ra, rb);
	ra = m_ir->CreateNot(ra);
	ra = m_ir->CreateAnd(ra, 0xE);
	ra = m_ir->CreateZExt(ra, m_ir->getInt64Ty());

	StoreGPR128(op.rt, _mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));

	Value *addr_ptr = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context)), ra);
	addr_ptr = m_ir->CreateGEP(addr_ptr, m_ir->getInt64(offset32(&SPUThread::gpr, op.rt)));
	m_ir->CreateStore(m_ir->getInt16(0x0203), m_ir->CreateBitCast(addr_ptr, PointerType::getInt16PtrTy(m_context)));
}

void spu_llvm_recompiler::CWX(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);
	ra = m_ir->CreateAdd(ra, rb);
	ra = m_ir->CreateNot(ra);
	ra = m_ir->CreateAnd(ra, 0xC);
	ra = m_ir->CreateZExt(ra, m_ir->getInt64Ty());

	StoreGPR128(op.rt, _mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));

	Value *addr_ptr = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context)), ra);
	addr_ptr = m_ir->CreateGEP(addr_ptr, m_ir->getInt64(offset32(&SPUThread::gpr, op.rt)));
	m_ir->CreateStore(m_ir->getInt32(0x00010203), m_ir->CreateBitCast(addr_ptr, PointerType::getInt32PtrTy(m_context)));
}

void spu_llvm_recompiler::CDX(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);
	ra = m_ir->CreateAdd(ra, rb);
	ra = m_ir->CreateNot(ra);
	ra = m_ir->CreateAnd(ra, 0x8);
	ra = m_ir->CreateZExt(ra, m_ir->getInt64Ty());

	StoreGPR128(op.rt, _mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));

	Value *addr_ptr = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context)), ra);
	addr_ptr = m_ir->CreateGEP(addr_ptr, m_ir->getInt64(offset32(&SPUThread::gpr, op.rt)));
	m_ir->CreateStore(m_ir->getInt64(0x0001020304050607), m_ir->CreateBitCast(addr_ptr, PointerType::getInt64PtrTy(m_context)));
}

void spu_llvm_recompiler::ROTQBI(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	Value *shift = m_ir->CreateAnd(LoadGPR32(op.rb, 3), 0x7);
	Value *shift_inv = m_ir->CreateSub(m_ir->getInt32(128), shift);

	Value *rt = m_ir->CreateShl(ra, ZExt128(shift));
	rt = m_ir->CreateOr(rt, m_ir->CreateLShr(ra, ZExt128(shift_inv)));

	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTQMBI(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);

	Value *addr = LoadGPR32(op.rb, 3);
	addr = m_ir->CreateNeg(addr);
	addr = m_ir->CreateAnd(addr, 7);
	
	Value *result = m_ir->CreateLShr(ra, ZExt128(addr));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::SHLQBI(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	Value *rb = m_ir->CreateZExt(m_ir->CreateAnd(LoadGPR32(op.rb, 3), 0x7), m_ir->getInt128Ty());
	Value *rt = m_ir->CreateShl(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTQBY(spu_opcode_t op)
{
	Value *rldq_pshufb = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.rldq_pshufb), m_ir->getInt8PtrTy());

	Value *addr = LoadGPR32(op.rb, 3);
	addr = m_ir->CreateAnd(addr, 0xf);
	addr = m_ir->CreateShl(addr, 4);
	addr = m_ir->CreateZExt(addr, m_ir->getInt64Ty());

	rldq_pshufb = m_ir->CreateBitCast(m_ir->CreateGEP(rldq_pshufb, addr), PointerType::get(VectorType::get(m_ir->getInt8Ty(), 16), 0));

	Value *mask = m_ir->CreateLoad(rldq_pshufb);
	Value *ra = LoadGPRVec16(op.ra);
	Value *rt = Intrinsic_pshufb(ra, mask);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTQMBY(spu_opcode_t op)
{
	Value *srdq_pshufb = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.srdq_pshufb), m_ir->getInt8PtrTy());

	Value *addr = LoadGPR32(op.rb, 3);
	addr = m_ir->CreateNeg(addr);
	addr = m_ir->CreateAnd(addr, 0x1f);
	addr = m_ir->CreateShl(addr, 4);
	addr = m_ir->CreateZExt(addr, m_ir->getInt64Ty());

	srdq_pshufb = m_ir->CreateBitCast(m_ir->CreateGEP(srdq_pshufb, addr), PointerType::get(VectorType::get(m_ir->getInt8Ty(), 16), 0));

	Value *mask = m_ir->CreateLoad(srdq_pshufb);
	Value *ra = LoadGPRVec16(op.ra);
	Value *rt = Intrinsic_pshufb(ra, mask);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SHLQBY(spu_opcode_t op)
{
	Value *sldq_pshufb = ConstantExpr::getIntToPtr(m_ir->getInt64((u64)&g_spu_imm.sldq_pshufb), m_ir->getInt8PtrTy());

	Value *addr = LoadGPR32(op.rb, 3);
	addr = m_ir->CreateAnd(addr, 0x1f);
	addr = m_ir->CreateShl(addr, 4);

	sldq_pshufb = m_ir->CreateBitCast(m_ir->CreateGEP(sldq_pshufb, addr), PointerType::get(VectorType::get(m_ir->getInt8Ty(), 16), 0));

	Value *mask = m_ir->CreateLoad(sldq_pshufb);
	Value *ra = LoadGPRVec16(op.ra);
	Value *rt = Intrinsic_pshufb(ra, mask);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ORX(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);

	Value *result = m_ir->getInt32(0);
	for (int i = 0; i < 4; i++)
	{
		result = m_ir->CreateOr(result, m_ir->CreateExtractElement(ra, i));
	}

	StoreGPR128(op.rt, GetInt128(_mm_set1_epi32(0)));
	StoreGPR32(op.rt, result, 3);
}

void spu_llvm_recompiler::CBD(spu_opcode_t op)
{
	Value *addr = LoadGPR32(op.ra, 3);
	addr = m_ir->CreateAdd(addr, m_ir->getInt32(op.i7));
	addr = m_ir->CreateNot(addr);
	addr = m_ir->CreateAnd(addr, 0xF);
	addr = m_ir->CreateZExt(addr, m_ir->getInt64Ty());

	StoreGPR128(op.rt, _mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//StoreGPR8(op.rt, m_ir->getInt8(0x03), addr);

	Value *addr_ptr = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context)), addr);
	addr_ptr = m_ir->CreateGEP(addr_ptr, m_ir->getInt64(offset32(&SPUThread::gpr, op.rt)));
	m_ir->CreateStore(m_ir->getInt8(0x03), addr_ptr);
}

void spu_llvm_recompiler::CHD(spu_opcode_t op)
{
	Value *addr = LoadGPR32(op.ra, 3);
	addr = m_ir->CreateAdd(addr, m_ir->getInt32(op.i7));
	addr = m_ir->CreateNot(addr);
	addr = m_ir->CreateAnd(addr, 0xE);
	addr = m_ir->CreateZExt(addr, m_ir->getInt64Ty());

	StoreGPR128(op.rt, _mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//StoreGPR16(op.rt, 0x0203, addr);

	Value *addr_ptr = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context)), addr);
	addr_ptr = m_ir->CreateGEP(addr_ptr, m_ir->getInt64(offset32(&SPUThread::gpr, op.rt)));
	m_ir->CreateStore(m_ir->getInt16(0x0203), m_ir->CreateBitCast(addr_ptr, PointerType::getInt16PtrTy(m_context)));
}

void spu_llvm_recompiler::CWD(spu_opcode_t op)
{
	Value *addr = LoadGPR32(op.ra, 3);
	addr = m_ir->CreateAdd(addr, m_ir->getInt32(op.i7));
	addr = m_ir->CreateNot(addr);
	addr = m_ir->CreateAnd(addr, 0xC);
	addr = m_ir->CreateZExt(addr, m_ir->getInt64Ty());

	StoreGPR128(op.rt, _mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//StoreGPR32(op.rt, 0x00010203, addr);

	Value *addr_ptr = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context)), addr);
	addr_ptr = m_ir->CreateGEP(addr_ptr, m_ir->getInt64(offset32(&SPUThread::gpr, op.rt)));
	m_ir->CreateStore(m_ir->getInt32(0x00010203), m_ir->CreateBitCast(addr_ptr, PointerType::getInt32PtrTy(m_context)));
}

void spu_llvm_recompiler::CDD(spu_opcode_t op)
{
	Value *addr = LoadGPR32(op.ra, 3);
	addr = m_ir->CreateAdd(addr, m_ir->getInt32(op.i7));
	addr = m_ir->CreateNot(addr);
	addr = m_ir->CreateAnd(addr, 0x8);
	addr = m_ir->CreateZExt(addr, m_ir->getInt64Ty());

	StoreGPR128(op.rt, _mm_set_epi32(0x10111213, 0x14151617, 0x18191a1b, 0x1c1d1e1f));
	//StoreGPR64(op.rt, 0x0001020304050607, addr);

	Value *addr_ptr = m_ir->CreateGEP(m_ir->CreateBitCast(m_cpu, PointerType::getInt8PtrTy(m_context)), addr);
	addr_ptr = m_ir->CreateGEP(addr_ptr, m_ir->getInt64(offset32(&SPUThread::gpr, op.rt)));
	m_ir->CreateStore(m_ir->getInt64(0x0001020304050607), m_ir->CreateBitCast(addr_ptr, PointerType::getInt64PtrTy(m_context)));
}

void spu_llvm_recompiler::ROTQBII(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	const int s = op.i7 & 0x7;
	const int s_inv = 128 - s;

	Value *rt = m_ir->CreateShl(ra, s);
	rt = m_ir->CreateOr(rt, m_ir->CreateLShr(ra, s_inv));

	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTQMBII(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	const int s = (0 - op.i7) & 0x7;

	Value *result = m_ir->CreateLShr(ra, ZExt128(m_ir->getInt32(s)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::SHLQBII(spu_opcode_t op)
{
	const int s = op.i7 & 0x7;
	Value *ra = LoadGPR128(op.ra);
	Value *rt = m_ir->CreateShl(ra, s);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ROTQBYI(spu_opcode_t op)
{
#if 0
	int s = (op.i7 & 0x1f) * 8;
	Value *ra = LoadGPR128(op.ra);
	Value *rt = m_ir->CreateShl(ra, s);
	rt = m_ir->CreateOr(rt, m_ir->CreateLShr(ra, 128 - s));
	StoreGPR128(op.rt, rt);
#endif

#if 1
	//might be broken
	const int s = (op.i7 & 0xf) * 8;
	Value *ra = LoadGPR128(op.ra);
	Value *ra_concat = m_ir->CreateOr(m_ir->CreateShl(m_ir->CreateZExt(ra, m_ir->getIntNTy(256)), 128), m_ir->CreateZExt(ra, m_ir->getIntNTy(256)));
	Value *result = m_ir->CreateTrunc(m_ir->CreateLShr(ra_concat, (16 * 8) - s), m_ir->getInt128Ty());
	StoreGPR128(op.rt, result);
#endif

	//InterpreterCall(op);
}

void spu_llvm_recompiler::ROTQMBYI(spu_opcode_t op)
{	
	/*
	broken
	const int s = (0 - op.i7 & 0x1f);
	Value *ra = LoadGPR128(op.ra);
	if(s < 15)
		ra = m_ir->CreateLShr(ra, s * 8);
	StoreGPR128(op.rt, ra);
	*/

	Value *mask = GetVec16(g_spu_imm.srdq_pshufb[0 - op.i7 & 0x1f].vi);
	Value *ra = LoadGPRVec16(op.ra);
	Value *rt = Intrinsic_pshufb(ra, mask);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SHLQBYI(spu_opcode_t op)
{
	Value *mask = GetVec16(g_spu_imm.sldq_pshufb[op.i7 & 0x1f].vi);
	Value *ra = LoadGPRVec16(op.ra);
	Value *rt = Intrinsic_pshufb(ra, mask);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::NOP(spu_opcode_t op)
{
}

void spu_llvm_recompiler::CGT(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *rt = m_ir->CreateICmpSGT(ra, rb);
	rt = m_ir->CreateSelect(rt, GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::XOR(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	Value *rb = LoadGPR128(op.rb);
	Value *result = m_ir->CreateXor(ra, rb);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CGTH(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = LoadGPRVec8(op.rb);
	Value *rt = m_ir->CreateICmpSGT(ra, rb);
	rt = m_ir->CreateSelect(rt, GetVec8(_mm_set1_epi16(0xFFFF)), GetVec8(_mm_set1_epi16(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::EQV(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *eq = m_ir->CreateICmpEQ(ra, rb);
	Value *result = m_ir->CreateSelect(eq, GetVec4(_mm_set1_epi32(1)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CGTB(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *rb = LoadGPRVec16(op.rb);
	Value *rt = m_ir->CreateICmpSGT(ra, rb);
	rt = m_ir->CreateSelect(rt, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SUMB(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *rt = GetVec8(_mm_set1_epi16(0));

	for (int i = 0; i < 4; i++)
	{
		Value *ra_bytes = m_ir->CreateBitCast(m_ir->CreateExtractElement(ra, i), VectorType::get(m_ir->getInt8Ty(), 4));
		Value *rb_bytes = m_ir->CreateBitCast(m_ir->CreateExtractElement(rb, i), VectorType::get(m_ir->getInt8Ty(), 4));

		Value *ra_result = m_ir->getInt16(0);
		Value *rb_result = m_ir->getInt16(0);

		for (int j = 0; j < 4; j++)
		{
			ra_result = m_ir->CreateAdd(ra_result, m_ir->CreateZExt(m_ir->CreateExtractElement(ra_bytes, j), m_ir->getInt16Ty()));
			rb_result = m_ir->CreateAdd(rb_result, m_ir->CreateZExt(m_ir->CreateExtractElement(rb_bytes, j), m_ir->getInt16Ty()));
		}

		rt = m_ir->CreateInsertElement(rt, ra_result, (i * 2));
		rt = m_ir->CreateInsertElement(rt, rb_result, (i * 2) + 1);
	}

	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::HGT(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);
	Value *cmp = m_ir->CreateICmpSGT(ra, rb);

	BasicBlock *exit = BasicBlock::Create(m_context, "__hgt_exit", m_llvm_func);
	BasicBlock *cont = BasicBlock::Create(m_context, "__hgt_cont", m_llvm_func);

	m_ir->CreateCondBr(cmp, exit, cont);

	m_ir->SetInsertPoint(exit);
	m_ir->CreateStore(m_ir->getInt32(m_pos | 0x1000000), m_addr);
	m_ir->CreateBr(m_end);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::CLZ(spu_opcode_t op)
{
	std::vector<Type*> ctlz_types = { VectorType::get(m_ir->getInt32Ty(), 4) };
	Function *ctlz = Intrinsic::getDeclaration(m_module.get(), Intrinsic::ctlz, ctlz_types);

	std::vector<Value*> ctlz_args = { LoadGPRVec4(op.ra), m_ir->getInt1(0) };
	Value *result = m_ir->CreateCall(ctlz, ctlz_args);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::XSWD(spu_opcode_t op)
{
	Value *ra = m_ir->CreateSExt(m_ir->CreateTrunc(LoadGPRVec2(op.ra), VectorType::get(m_ir->getInt32Ty(), 2)), VectorType::get(m_ir->getInt64Ty(), 2));
	StoreGPR128(op.rt, ra);
}

void spu_llvm_recompiler::XSHW(spu_opcode_t op)
{
	Value *ra = m_ir->CreateSExt(m_ir->CreateTrunc(LoadGPRVec4(op.ra), VectorType::get(m_ir->getInt16Ty(), 4)), VectorType::get(m_ir->getInt32Ty(), 4));
	StoreGPR128(op.rt, ra);
}

void spu_llvm_recompiler::CNTB(spu_opcode_t op)
{
	std::vector<Type*> ctpop_types = { VectorType::get(m_ir->getInt8Ty(), 16) };
	Function *ctpop = Intrinsic::getDeclaration(m_module.get(), Intrinsic::ctpop, ctpop_types);

	std::vector<Value*> ctpop_args = { LoadGPRVec16(op.ra) };
	Value *result = m_ir->CreateCall(ctpop, ctpop_args);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::XSBH(spu_opcode_t op)
{
	Value *ra = m_ir->CreateSExt(m_ir->CreateTrunc(LoadGPRVec8(op.ra), VectorType::get(m_ir->getInt8Ty(), 8)), VectorType::get(m_ir->getInt16Ty(), 8));
	StoreGPR128(op.rt, ra);
}

void spu_llvm_recompiler::CLGT(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *rt = m_ir->CreateICmpUGT(ra, rb);
	rt = m_ir->CreateSelect(rt, GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ANDC(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	Value *rb = LoadGPR128(op.rb);
	Value *result = m_ir->CreateAnd(ra, m_ir->CreateNot(rb));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::FCGT(spu_opcode_t op)
{
	//extremely incorrect
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *rb = LoadGPRVec4Float(op.rb);
	Value *result = m_ir->CreateSelect(m_ir->CreateFCmpUGT(ra, rb), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::DFCGT(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_llvm_recompiler::FA(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *rb = LoadGPRVec4Float(op.rb);
	Value *rt = m_ir->CreateFAdd(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::FS(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *rb = LoadGPRVec4Float(op.rb);
	Value *rt = m_ir->CreateFSub(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::FM(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *rb = LoadGPRVec4Float(op.rb);
	Value *rt = m_ir->CreateFMul(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CLGTH(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = LoadGPRVec8(op.rb);
	Value *cmp = m_ir->CreateICmpUGT(ra, rb);
	Value *result = m_ir->CreateSelect(cmp, GetVec8(_mm_set1_epi16(0xFFFF)), GetVec8(_mm_set1_epi16(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::ORC(spu_opcode_t op)
{
	Value *ra = LoadGPR128(op.ra);
	Value *rb = LoadGPR128(op.rb);
	Value *result = m_ir->CreateOr(ra, m_ir->CreateNot(rb));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::FCMGT(spu_opcode_t op)
{
	//extremely incorrect
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *vi = GetVec4(_mm_set1_epi32(0x7fffffff));
	ra = m_ir->CreateBitCast(m_ir->CreateAnd(ra, vi), VectorType::get(m_ir->getFloatTy(), 4));
	rb = m_ir->CreateBitCast(m_ir->CreateAnd(rb, vi), VectorType::get(m_ir->getFloatTy(), 4));
	Value *result = m_ir->CreateSelect(m_ir->CreateFCmpUGT(ra, rb), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::DFCMGT(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_llvm_recompiler::DFA(spu_opcode_t op)
{
	Value *ra = LoadGPRVec2Double(op.ra);
	Value *rb = LoadGPRVec2Double(op.rb);
	Value *rt = m_ir->CreateFAdd(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::DFS(spu_opcode_t op)
{
	Value *ra = LoadGPRVec2Double(op.ra);
	Value *rb = LoadGPRVec2Double(op.rb);
	Value *rt = m_ir->CreateFSub(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::DFM(spu_opcode_t op)
{
	Value *ra = LoadGPRVec2Double(op.ra);
	Value *rb = LoadGPRVec2Double(op.rb);
	Value *rt = m_ir->CreateFMul(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CLGTB(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *rb = LoadGPRVec16(op.rb);
	Value *cmp = m_ir->CreateICmpUGT(ra, rb);
	Value *result = m_ir->CreateSelect(cmp, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::HLGT(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);
	Value *cmp = m_ir->CreateICmpUGT(ra, rb);

	BasicBlock *exit = BasicBlock::Create(m_context, "__hlgt_exit", m_llvm_func);
	BasicBlock *cont = BasicBlock::Create(m_context, "__hlgt_cont", m_llvm_func);

	m_ir->CreateCondBr(cmp, exit, cont);

	m_ir->SetInsertPoint(exit);
	m_ir->CreateStore(m_ir->getInt32(m_pos | 0x1000000), m_addr);
	m_ir->CreateBr(m_end);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::DFMA(spu_opcode_t op)
{
	Value *ra = LoadGPRVec2Double(op.ra);
	Value *rb = LoadGPRVec2Double(op.rb);
	Value *rt = LoadGPRVec2Double(op.rt);
	Value *result = m_ir->CreateFMul(ra, rb);
	result = m_ir->CreateFAdd(result, rt);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::DFMS(spu_opcode_t op)
{
	Value *ra = LoadGPRVec2Double(op.ra);
	Value *rb = LoadGPRVec2Double(op.rb);
	Value *rt = LoadGPRVec2Double(op.rt);
	Value *result = m_ir->CreateFMul(ra, rb);
	result = m_ir->CreateFSub(result, rt);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::DFNMS(spu_opcode_t op)
{
	Value *ra = LoadGPRVec2Double(op.ra);
	Value *rb = LoadGPRVec2Double(op.rb);
	Value *rt = LoadGPRVec2Double(op.rt);
	Value *result = m_ir->CreateFMul(ra, rb);
	result = m_ir->CreateFSub(rt, result);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::DFNMA(spu_opcode_t op)
{
	Value *zero = GetVec2Double(_mm_set1_pd(0.0));
	Value *ra = LoadGPRVec2Double(op.ra);
	Value *rb = LoadGPRVec2Double(op.rb);
	Value *rt = LoadGPRVec2Double(op.rt);
	Value *result = m_ir->CreateFMul(ra, rb);
	result = m_ir->CreateFAdd(rt, result);
	result = m_ir->CreateFSub(zero, result);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CEQ(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *rt = m_ir->CreateICmpEQ(ra, rb);
	rt = m_ir->CreateSelect(rt, GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::MPYHHU(spu_opcode_t op)
{
	MPYHH(op);
}

void spu_llvm_recompiler::ADDX(spu_opcode_t op)
{
	Value *rb = LoadGPRVec4(op.rb);
	Value *ra = LoadGPRVec4(op.ra);
	Value *rt = LoadGPRVec4(op.rt);
	Value *rtx = m_ir->CreateAnd(rt, GetVec4(_mm_set1_epi32(1)));

	Value *result = m_ir->CreateAdd(ra, rb);
	result = m_ir->CreateAdd(result, rtx);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::SFX(spu_opcode_t op)
{
	Value *rb = LoadGPRVec4(op.rb);
	Value *ra = LoadGPRVec4(op.ra);
	Value *rt = LoadGPRVec4(op.rt);

	Value *rtx = m_ir->CreateAnd(m_ir->CreateNot(rt), GetVec4(_mm_set1_epi32(1)));
	Value *result = m_ir->CreateSub(rb, ra);
	result = m_ir->CreateSub(result, rtx);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CGX(spu_opcode_t op)
{
	Value *ra = m_ir->CreateZExt(LoadGPRVec4(op.ra), VectorType::get(m_ir->getInt64Ty(), 4));
	Value *rb = m_ir->CreateZExt(LoadGPRVec4(op.rb), VectorType::get(m_ir->getInt64Ty(), 4));
	Value *rt = m_ir->CreateZExt(m_ir->CreateAnd(LoadGPRVec4(op.rt), GetVec4(_mm_set1_epi32(1))), VectorType::get(m_ir->getInt64Ty(), 4));

	Value *result = m_ir->CreateTrunc(m_ir->CreateLShr(m_ir->CreateAdd(m_ir->CreateAdd(ra, rb), rt), 32), VectorType::get(m_ir->getInt32Ty(), 4));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::BGX(spu_opcode_t op)
{
	Value *ra = m_ir->CreateZExt(LoadGPRVec4(op.ra), VectorType::get(m_ir->getInt64Ty(), 4));
	Value *rb = m_ir->CreateZExt(LoadGPRVec4(op.rb), VectorType::get(m_ir->getInt64Ty(), 4));
	Value *rt = m_ir->CreateZExt(m_ir->CreateSub(GetVec4(_mm_set1_epi32(1)), m_ir->CreateAnd(LoadGPRVec4(op.rt), GetVec4(_mm_set1_epi32(1)))), VectorType::get(m_ir->getInt64Ty(), 4));

	Value *sub = m_ir->CreateSub(rb, ra);
	sub = m_ir->CreateSub(sub, rt);

	Value *res_cmp = m_ir->CreateICmpSGE(sub, GetVec4(_mm_set1_epi32(0)));
	Value *result = m_ir->CreateSelect(res_cmp, GetVec4(_mm_set1_epi32(1)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::MPYHHA(spu_opcode_t op)
{
	Value *mask = GetVec4(_mm_set1_epi32(0xffff0000));
	Value *ra = m_ir->CreateLShr(m_ir->CreateAnd(LoadGPRVec4(op.ra), mask), 16);
	Value *rb = m_ir->CreateLShr(m_ir->CreateAnd(LoadGPRVec4(op.rb), mask), 16);
	Value *rt = LoadGPRVec4(op.rt);
	Value *result = m_ir->CreateAdd(m_ir->CreateMul(ra, rb), rt);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::MPYHHAU(spu_opcode_t op)
{
	MPYHHA(op);
}

void spu_llvm_recompiler::FSCRRD(spu_opcode_t op)
{
	Value *result = GetVec4(_mm_set1_epi32(0));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::FESD(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4Float(op.ra);
	ra = m_ir->CreateFPExt(ra, VectorType::get(m_ir->getDoubleTy(), 4));
	Value *result = GetVec2Double(_mm_set1_pd(0.0));
	result = m_ir->CreateInsertElement(result, m_ir->CreateExtractElement(ra, 1), (u64)0);
	result = m_ir->CreateInsertElement(result, m_ir->CreateExtractElement(ra, 3), 1);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::FRDS(spu_opcode_t op)
{
	Value *ra = LoadGPRVec2Double(op.ra);
	ra = m_ir->CreateFPTrunc(ra, VectorType::get(m_ir->getFloatTy(), 2));
	Value *result = GetVec4Float(_mm_set1_ps(0));
	result = m_ir->CreateInsertElement(result, m_ir->CreateExtractElement(ra, (u64)0), 1);
	result = m_ir->CreateInsertElement(result, m_ir->CreateExtractElement(ra, 1), 3);

	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::FSCRWR(spu_opcode_t op)
{
}

void spu_llvm_recompiler::DFTSV(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_llvm_recompiler::FCEQ(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *rb = LoadGPRVec4Float(op.rb);
	Value *result = m_ir->CreateSelect(m_ir->CreateFCmpUEQ(rb, ra), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::DFCEQ(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_llvm_recompiler::MPY(spu_opcode_t op)
{
	Value *mask = GetVec4(_mm_set1_epi32(0xffff));
	Value *rb = m_ir->CreateAnd(LoadGPRVec4(op.rb), mask);
	Value *ra = m_ir->CreateAnd(LoadGPRVec4(op.ra), mask);
	Value *rt = m_ir->CreateMul(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::MPYH(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);

	ra = m_ir->CreateLShr(ra, GetVec4(_mm_set1_epi32(16)));
	Value *rt = m_ir->CreateMul(ra, rb);
	rt = m_ir->CreateShl(rt, GetVec4(_mm_set1_epi32(16)));
	
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::MPYHH(spu_opcode_t op)
{
	Value *mask = GetVec4(_mm_set1_epi32(0xffff0000));
	Value *ra = m_ir->CreateLShr(m_ir->CreateAnd(LoadGPRVec4(op.ra), mask), 16);
	Value *rb = m_ir->CreateLShr(m_ir->CreateAnd(LoadGPRVec4(op.rb), mask), 16);
	Value *rt = m_ir->CreateMul(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::MPYS(spu_opcode_t op)
{
	Value *mask = GetVec4(_mm_set1_epi32(0xffff));
	Value *rb = m_ir->CreateAnd(LoadGPRVec4(op.rb), mask);
	Value *ra = m_ir->CreateAnd(LoadGPRVec4(op.ra), mask);
	Value *rt = m_ir->CreateMul(ra, rb);
	rt = m_ir->CreateAShr(rt, GetVec4(_mm_set1_epi32(16)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CEQH(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = LoadGPRVec8(op.rb);
	Value *rt = m_ir->CreateICmpEQ(ra, rb);
	rt = m_ir->CreateSelect(rt, GetVec8(_mm_set1_epi16(0xFFFF)), GetVec8(_mm_set1_epi16(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::FCMEQ(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *rb = LoadGPRVec4(op.rb);
	Value *vi = GetVec4(_mm_set1_epi32(0x7fffffff));
	ra = m_ir->CreateBitCast(m_ir->CreateAnd(ra, vi), VectorType::get(m_ir->getFloatTy(), 4));
	rb = m_ir->CreateBitCast(m_ir->CreateAnd(rb, vi), VectorType::get(m_ir->getFloatTy(), 4));
	Value *result = m_ir->CreateSelect(m_ir->CreateFCmpUEQ(rb, ra), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::DFCMEQ(spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_llvm_recompiler::MPYU(spu_opcode_t op)
{
	MPY(op);
}

void spu_llvm_recompiler::CEQB(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *rb = LoadGPRVec16(op.rb);
	Value *rt = m_ir->CreateICmpEQ(ra, rb);
	rt = m_ir->CreateSelect(rt, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::FI(spu_opcode_t op)
{
	//TODO
	Value *rb = LoadGPRVec4Float(op.rb);
	StoreGPR128(op.rt, rb);
}

void spu_llvm_recompiler::HEQ(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = LoadGPR32(op.rb, 3);
	Value *cmp = m_ir->CreateICmpEQ(ra, rb);

	BasicBlock *exit = BasicBlock::Create(m_context, "__heq_exit", m_llvm_func);
	BasicBlock *cont = BasicBlock::Create(m_context, "__heq_cont", m_llvm_func);

	m_ir->CreateCondBr(cmp, exit, cont);

	m_ir->SetInsertPoint(exit);
	m_ir->CreateStore(m_ir->getInt32(m_pos | 0x1000000), m_addr);
	m_ir->CreateBr(m_end);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::CFLTS(spu_opcode_t op)
{
	/*
	const XmmLink& va = XmmGet(op.ra, XmmType::Float);
	const XmmLink& vi = XmmAlloc();
	if (op.i8 != 173) c->mulps(va, XmmConst(_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8)))))); // scale
	c->movaps(vi, XmmConst(_mm_set1_ps(std::exp2(31.f))));
	c->cmpps(vi, va, 2);
	c->cvttps2dq(va, va); // convert to ints with truncation
	c->pxor(va, vi); // fix result saturation (0x80000000 -> 0x7fffffff)
	c->movdqa(SPU_OFF_128(gpr, op.rt), va);
	*/

	Value *ra = LoadGPRVec4Float(op.ra);
	if(op.i8 != 173)
		ra = m_ir->CreateFMul(ra, GetVec4Float(_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(173 - op.i8))))));
	Value *vi = GetVec4Float(_mm_set1_ps(std::exp2(31.f)));
	Value *cmp = m_ir->CreateSelect(m_ir->CreateFCmpULE(vi, ra), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	Value *result = m_ir->CreateFPToSI(ra, VectorType::get(m_ir->getInt32Ty(), 4));
	result = m_ir->CreateXor(result, cmp);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CFLTU(spu_opcode_t op)
{
	InterpreterCall(op); //TODO
}

void spu_llvm_recompiler::CSFLT(spu_opcode_t op)
{
	/*
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	c->cvtdq2ps(va, va); // convert to floats
	if (op.i8 != 155) c->mulps(va, XmmConst(_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))))); // scale
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
	*/

	Value *ra = LoadGPRVec4(op.ra);
	Value *result = m_ir->CreateSIToFP(ra, VectorType::get(m_ir->getFloatTy(), 4));
	if (op.i8 != 155)
		result = m_ir->CreateFMul(result, GetVec4Float((_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))))));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CUFLT(spu_opcode_t op)
{
	/*
	const XmmLink& va = XmmGet(op.ra, XmmType::Int);
	const XmmLink& v1 = XmmAlloc();
	c->movdqa(v1, va);
	c->pand(va, XmmConst(_mm_set1_epi32(0x7fffffff)));
	c->cvtdq2ps(va, va); // convert to floats
	c->psrad(v1, 31); // generate mask from sign bit
	c->andps(v1, XmmConst(_mm_set1_ps(std::exp2(31.f)))); // generate correction component
	c->addps(va, v1); // add correction component
	if (op.i8 != 155) c->mulps(va, XmmConst(_mm_set1_ps(std::exp2(static_cast<float>(static_cast<s16>(op.i8 - 155)))))); // scale
	c->movaps(SPU_OFF_128(gpr, op.rt), va);
	*/

	InterpreterCall(op); //TODO
}

void spu_llvm_recompiler::BRZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	Value *rt = LoadGPR32(op.rt, 3);
	Value *res = m_ir->CreateICmpEQ(rt, m_ir->getInt32(0));

	BasicBlock *tgt = m_blocks[target / 4];
	BasicBlock *cont = BasicBlock::Create(m_context, "__brz_cont", m_llvm_func);

	if (tgt != nullptr)
	{
		m_ir->CreateCondBr(res, tgt, cont);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brz 0x%x)", target);
		}

		m_ir->CreateStore(m_ir->getInt32(target), m_addr);
		m_ir->CreateCondBr(res, m_end, cont);
	}

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::STQA(spu_opcode_t op)
{
	Value *rt_contents = LoadGPRVec16(op.rt);
	rt_contents = Intrinsic_pshufb(rt_contents, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

	StoreLS(m_ir->getInt32(spu_ls_target(0, op.i16)), rt_contents);
}

void spu_llvm_recompiler::BRNZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	Value *rt = LoadGPR32(op.rt, 3);
	Value *res = m_ir->CreateICmpNE(rt, m_ir->getInt32(0));

	BasicBlock *tgt = m_blocks[target / 4];
	BasicBlock *cont = BasicBlock::Create(m_context, "__brnz_cont", m_llvm_func);

	if (tgt != nullptr)
	{
		m_ir->CreateCondBr(res, tgt, cont);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brnz 0x%x)", target);
		}

		m_ir->CreateStore(m_ir->getInt32(target), m_addr);
		m_ir->CreateCondBr(res, m_end, cont);
	}

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::BRHZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	Value *rt = LoadGPR16(op.rt, 6);
	Value *res = m_ir->CreateICmpEQ(rt, m_ir->getInt16(0));

	BasicBlock *tgt = m_blocks[target / 4];
	BasicBlock *cont = BasicBlock::Create(m_context, "__brhz_cont", m_llvm_func);

	if (tgt != nullptr)
	{
		m_ir->CreateCondBr(res, tgt, cont);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brhz 0x%x)", target);
		}

		m_ir->CreateStore(m_ir->getInt32(target), m_addr);
		m_ir->CreateCondBr(res, m_end, cont);
	}

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::BRHNZ(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	Value *rt = LoadGPR16(op.rt, 6);
	Value *res = m_ir->CreateICmpNE(rt, m_ir->getInt16(0));

	BasicBlock *tgt = m_blocks[target / 4];
	BasicBlock *cont = BasicBlock::Create(m_context, "__brhnz_cont", m_llvm_func);

	if (tgt != nullptr)
	{
		m_ir->CreateCondBr(res, tgt, cont);
	}
	else
	{
		if (target >= m_func->addr && target < m_func->addr + m_func->size)
		{
			LOG_ERROR(SPU, "Local block not registered (brhnz 0x%x)", target);
		}

		m_ir->CreateStore(m_ir->getInt32(target), m_addr);
		m_ir->CreateCondBr(res, m_end, cont);
	}

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::STQR(spu_opcode_t op)
{
	Value *rt_contents = LoadGPRVec16(op.rt);
	rt_contents = Intrinsic_pshufb(rt_contents, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

	Value *addr = m_ir->getInt32(spu_ls_target(m_pos, op.i16));
	StoreLS(addr, rt_contents);
}

void spu_llvm_recompiler::BRA(spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	BasicBlock *cont = BasicBlock::Create(m_context, "__bra_exit_cont", m_llvm_func);

	if (target >= m_func->addr && target < m_func->addr + m_func->size)
	{
		//branch to somewhere inside current function
		BasicBlock *block = m_blocks[target / 4];
		if (block == nullptr)
		{
			LOG_ERROR(SPU, "Local block not registered (bra 0x%x)", target);
		}

		m_ir->CreateBr(block);
	}
	else
	{
		m_ir->CreateStore(m_ir->getInt32(target), m_addr);
		m_ir->CreateBr(m_end);
	}

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::LQA(spu_opcode_t op)
{
	const u32 target = spu_ls_target(0, op.i16);

	Value *ls_contents = LoadLS(m_ir->getInt32(target), VectorType::get(m_ir->getInt8Ty(), 16));
	ls_contents = Intrinsic_pshufb(ls_contents, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

	StoreGPR128(op.rt, ls_contents);
}

void spu_llvm_recompiler::BRASL(spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.i16);

	if (target == m_pos) fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);

	StoreGPR128(op.rt, _mm_set_epi32(spu_branch_target(m_pos + 4), 0, 0, 0));

	m_ir->CreateStore(m_ir->getInt32(target), PtrPC());

	FunctionCall();
}

void spu_llvm_recompiler::BR(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.si16); //spu_branch_target(0, op.i16);

	BasicBlock *cont = BasicBlock::Create(m_context, "__br_exit_cont", m_llvm_func);

	if (target == m_pos)
	{
		Value *spu_ptr = m_ir->CreateBitCast(m_cpu, m_ir->getInt8PtrTy());
		spu_ptr = m_ir->CreateBitCast(m_ir->CreateGEP(m_cpu, m_ir->getInt64(offset32(&SPUThread::state))), PointerType::getInt32PtrTy(m_context));

		m_ir->CreateStore(m_ir->getInt32(target | 0x2000000), m_addr);
		m_ir->CreateAtomicRMW(AtomicRMWInst::BinOp::Or, spu_ptr, m_ir->getInt32(static_cast<u32>(cpu_flag::stop + cpu_flag::ret)), AtomicOrdering::SequentiallyConsistent);
		m_ir->CreateBr(m_end);

		m_ir->SetInsertPoint(cont);
		return;
	}

	if (target >= m_func->addr && target < m_func->addr + m_func->size)
	{
		//branch to somewhere inside current function
		BasicBlock *block = m_blocks[target / 4];
		if (block == nullptr)
		{
			LOG_ERROR(SPU, "Local block not registered (bra 0x%x)", target);
		}

		m_ir->CreateBr(block);
	}
	else
	{
		m_ir->CreateStore(m_ir->getInt32(target), m_addr);
		m_ir->CreateBr(m_end);
	}

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::FSMBI(spu_opcode_t op)
{
	Value *rt = GetInt128(g_spu_imm.fsmb[op.i16].vi);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::BRSL(spu_opcode_t op)
{
	const u32 target = spu_branch_target(m_pos, op.si16);

	if (target == m_pos)
	{
		fmt::throw_exception("Branch-to-self (0x%05x)" HERE, target);
	}

	StoreGPR128(op.rt, _mm_set_epi32(spu_branch_target(m_pos + 4), 0, 0, 0));

	if (target == spu_branch_target(m_pos + 4))
	{
		// branch-to-next
		return;
	}

	m_ir->CreateStore(m_ir->getInt32(target), PtrPC());
	FunctionCall();
}

void spu_llvm_recompiler::LQR(spu_opcode_t op)
{
	Value *ls_contents = LoadLS(m_ir->getInt32(spu_ls_target(m_pos, op.i16)), VectorType::get(m_ir->getInt8Ty(), 16));
	ls_contents = Intrinsic_pshufb(ls_contents, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

	StoreGPR128(op.rt, ls_contents);
}

void spu_llvm_recompiler::IL(spu_opcode_t op)
{
	Value *s = GetInt128(_mm_set1_epi32(op.si16));
	StoreGPR128(op.rt, s);
}

void spu_llvm_recompiler::ILHU(spu_opcode_t op)
{
	Value *s = GetInt128(_mm_set1_epi32(op.i16 << 16));
	StoreGPR128(op.rt, s);
}

void spu_llvm_recompiler::ILH(spu_opcode_t op)
{
	Value *s = GetInt128(_mm_set1_epi16(op.i16));
	StoreGPR128(op.rt, s);
}

void spu_llvm_recompiler::IOHL(spu_opcode_t op)
{
	Value *s = GetVec4(_mm_set1_epi32(op.i16));
	Value *rt = LoadGPRVec4(op.rt);
	rt = m_ir->CreateOr(rt, s);

	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ORI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *i = GetVec4(_mm_set1_epi32(op.si10));
	Value *rt = m_ir->CreateOr(ra, i);

	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ORHI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *i = GetVec8(_mm_set1_epi16(op.si10));
	Value *rt = m_ir->CreateOr(ra, i);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ORBI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *i = GetVec16(_mm_set1_epi8(op.si10));
	Value *rt = m_ir->CreateOr(ra, i);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SFI(spu_opcode_t op)
{
	Value *t = GetVec4(_mm_set1_epi32(op.si10));
	Value *ra = LoadGPRVec4(op.ra);
	Value *rt = m_ir->CreateSub(t, ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SFHI(spu_opcode_t op)
{
	Value *t = GetVec8(_mm_set1_epi16(op.si10));
	Value *ra = LoadGPRVec8(op.ra);
	Value *rt = m_ir->CreateSub(t, ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ANDI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *b = GetVec4(_mm_set1_epi32(op.si10));
	Value *rt = m_ir->CreateAnd(ra, b);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::ANDHI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *b = GetVec8(_mm_set1_epi16(op.si10));
	ra = m_ir->CreateAnd(ra, b);
	StoreGPR128(op.rt, ra);
}

void spu_llvm_recompiler::ANDBI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *b = GetVec16(_mm_set1_epi8(op.si10));
	ra = m_ir->CreateAnd(ra, b);
	StoreGPR128(op.rt, ra);
}

void spu_llvm_recompiler::AI(spu_opcode_t op)
{
	Value *t = GetVec4(_mm_set1_epi32(op.si10));
	Value *ra = LoadGPRVec4(op.ra);
	Value *rt = m_ir->CreateAdd(t, ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::AHI(spu_opcode_t op)
{
	Value *t = GetVec8(_mm_set1_epi16(op.si10));
	Value *ra = LoadGPRVec8(op.ra);
	Value *rt = m_ir->CreateAdd(t, ra);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::STQD(spu_opcode_t op)
{
	Value *rt_contents = LoadGPRVec16(op.rt);
	rt_contents = Intrinsic_pshufb(rt_contents, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

	Value* addr = LoadGPR32(op.ra, 3);
	if (op.si10)
		addr = m_ir->CreateAdd(addr, m_ir->getInt32(op.si10 << 4));
	addr = m_ir->CreateAnd(addr, m_ir->getInt32(0x3fff0));

	StoreLS(addr, rt_contents);
}

void spu_llvm_recompiler::LQD(spu_opcode_t op)
{
	Value *addr = LoadGPR32(op.ra, 3);
	if (op.si10)
		addr = m_ir->CreateAdd(addr, m_ir->getInt32(op.si10 << 4));
	addr = m_ir->CreateAnd(addr, m_ir->getInt32(0x3fff0));
	
	Value *ls_contents = LoadLS(addr, VectorType::get(m_ir->getInt8Ty(), 16));
	ls_contents = Intrinsic_pshufb(ls_contents, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

	StoreGPR128(op.rt, ls_contents);
}

void spu_llvm_recompiler::XORI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *imm = GetVec4(_mm_set1_epi32(op.si10));
	Value *result = m_ir->CreateXor(ra, imm);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::XORHI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *imm = GetVec8(_mm_set1_epi16(op.si10));
	Value *result = m_ir->CreateXor(ra, imm);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::XORBI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *imm = GetVec16(_mm_set1_epi8(op.si10));
	Value *result = m_ir->CreateXor(ra, imm);
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CGTI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *cmp = GetVec4(_mm_set1_epi32(op.si10));
	Value *rt = m_ir->CreateICmpSGT(ra, cmp);
	rt = m_ir->CreateSelect(rt, GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CGTHI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *cmp = GetVec8(_mm_set1_epi16(op.si10));
	Value *rt = m_ir->CreateICmpSGT(ra, cmp);
	rt = m_ir->CreateSelect(rt, GetVec8(_mm_set1_epi16(0xFFFF)), GetVec8(_mm_set1_epi16(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CGTBI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *cmp = GetVec16(_mm_set1_epi8(op.si10));
	Value *rt = m_ir->CreateICmpSGT(ra, cmp);
	rt = m_ir->CreateSelect(rt, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::HGTI(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = m_ir->getInt32(op.si10);
	Value *cmp = m_ir->CreateICmpSGT(ra, rb);

	BasicBlock *exit = BasicBlock::Create(m_context, "__hgt_exit", m_llvm_func);
	BasicBlock *cont = BasicBlock::Create(m_context, "__hgt_cont", m_llvm_func);

	m_ir->CreateCondBr(cmp, exit, cont);

	m_ir->SetInsertPoint(exit);
	m_ir->CreateStore(m_ir->getInt32(m_pos | 0x1000000), m_addr);
	m_ir->CreateBr(m_end);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::CLGTI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *cmp = GetVec4(_mm_set1_epi32(op.si10));
	Value *rt = m_ir->CreateICmpUGT(ra, cmp);
	rt = m_ir->CreateSelect(rt, GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CLGTHI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = GetVec8(_mm_set1_epi16(op.si10));
	Value *cmp = m_ir->CreateICmpUGT(ra, rb);
	Value *result = m_ir->CreateSelect(cmp, GetVec8(_mm_set1_epi16(0xFFFF)), GetVec8(_mm_set1_epi16(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::CLGTBI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *rb = GetVec16(_mm_set1_epi8(op.si10));
	Value *cmp = m_ir->CreateICmpUGT(ra, rb);
	Value *result = m_ir->CreateSelect(cmp, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));
	StoreGPR128(op.rt, result);
}

void spu_llvm_recompiler::HLGTI(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *cmp = m_ir->CreateICmpUGT(ra, m_ir->getInt32(op.si10));

	BasicBlock *exit = BasicBlock::Create(m_context, "__hlgti_exit", m_llvm_func);
	BasicBlock *cont = BasicBlock::Create(m_context, "__hlgti_cont", m_llvm_func);

	m_ir->CreateCondBr(cmp, exit, cont);

	m_ir->SetInsertPoint(exit);
	m_ir->CreateStore(m_ir->getInt32(m_pos | 0x1000000), m_addr);
	m_ir->CreateBr(m_end);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::MPYI(spu_opcode_t op)
{
	Value *mask = GetVec4(_mm_set1_epi32(0xffff));
	Value *rb = GetVec4(_mm_set1_epi32(op.si10 & 0xffff));
	Value *ra = m_ir->CreateAnd(LoadGPRVec4(op.ra), mask);
	Value *rt = m_ir->CreateMul(ra, rb);
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::MPYUI(spu_opcode_t op)
{
	MPYI(op);
}

void spu_llvm_recompiler::CEQI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4(op.ra);
	Value *cmp = GetVec4(_mm_set1_epi32(op.si10));
	Value *rt = m_ir->CreateICmpEQ(ra, cmp);
	rt = m_ir->CreateSelect(rt, GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CEQHI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec8(op.ra);
	Value *rb = GetVec8(_mm_set1_epi16(op.si10));
	Value *rt = m_ir->CreateICmpEQ(ra, rb);
	rt = m_ir->CreateSelect(rt, GetVec8(_mm_set1_epi16(0xFFFF)), GetVec8(_mm_set1_epi16(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::CEQBI(spu_opcode_t op)
{
	Value *ra = LoadGPRVec16(op.ra);
	Value *cmp = GetVec16(_mm_set1_epi8(op.si10));
	Value *rt = m_ir->CreateICmpEQ(ra, cmp);
	rt = m_ir->CreateSelect(rt, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::HEQI(spu_opcode_t op)
{
	Value *ra = LoadGPR32(op.ra, 3);
	Value *rb = m_ir->getInt32(op.si10);
	Value *cmp = m_ir->CreateICmpEQ(ra, rb);

	BasicBlock *exit = BasicBlock::Create(m_context, "__heqi_exit", m_llvm_func);
	BasicBlock *cont = BasicBlock::Create(m_context, "__heqi_cont", m_llvm_func);

	m_ir->CreateCondBr(cmp, exit, cont);

	m_ir->SetInsertPoint(exit);
	m_ir->CreateStore(m_ir->getInt32(m_pos | 0x1000000), m_addr);
	m_ir->CreateBr(m_end);

	m_ir->SetInsertPoint(cont);
}

void spu_llvm_recompiler::HBRA(spu_opcode_t op)
{
	//do nothing, we don't care about branch hints
}

void spu_llvm_recompiler::HBRR(spu_opcode_t op)
{
	//do nothing, we don't care about branch hints
}

void spu_llvm_recompiler::ILA(spu_opcode_t op)
{
	Value *rt = GetInt128(_mm_set1_epi32(op.i18));
	StoreGPR128(op.rt, rt);
}

void spu_llvm_recompiler::SELB(spu_opcode_t op)
{
	Value *rb = LoadGPR128(op.rb);
	Value *ra = LoadGPR128(op.ra);
	Value *rc = LoadGPR128(op.rc);
	
	rb = m_ir->CreateAnd(rb, rc);
	rc = m_ir->CreateAnd(m_ir->CreateNot(rc), ra);
	rb = m_ir->CreateOr(rb, rc);

	StoreGPR128(op.rt4, rb);
}

void spu_llvm_recompiler::SHUFB(spu_opcode_t op)
{
	Value *mask = LoadGPRVec16(op.rc);
	Value *mask_111 = m_ir->CreateAnd(mask, GetVec16(_mm_set1_epi8(-0x20)));

	//11000000 = 0xFF
	Value *cmp_111_110 = m_ir->CreateICmpEQ(mask_111, GetVec16(_mm_set1_epi8(-0x40)));
	Value *result_111_110 = m_ir->CreateSelect(cmp_111_110, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));

	//11100000 = 0x80
	Value *cmp_111_111 = m_ir->CreateICmpEQ(mask_111, GetVec16(_mm_set1_epi8(-0x20)));
	Value *result_111_111 = m_ir->CreateSelect(cmp_111_111, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));
	result_111_111 = m_ir->CreateAnd(result_111_111, GetVec16(_mm_set1_epi8(-0x80)));

	//merge result_111_110 and result_111_111
	Value *result_111 = m_ir->CreateOr(result_111_110, result_111_111);

	Value *mask_10 = m_ir->CreateAnd(mask_111, GetVec16(_mm_set1_epi8(-0x80)));
	Value *cmp_10 = m_ir->CreateICmpEQ(mask_10, GetVec16(_mm_set1_epi8(-0x80)));
	Value *result_10 = m_ir->CreateSelect(cmp_10, GetVec16(_mm_set1_epi8(0xFF)), GetVec16(_mm_set1_epi8(0)));

	// select bytes from [op.rb]:
	Value *mask_1f = m_ir->CreateAnd(mask, m_ir->CreateNot(GetVec16(_mm_set1_epi8(-0x20))));

	Value *mask_1f_xor = m_ir->CreateXor(mask_1f, GetVec16(_mm_set1_epi8(0x10)));
	Value *rb_mask = m_ir->CreateSub(GetVec16(_mm_set1_epi8(0x0f)), mask_1f_xor);
	Value *rb = LoadGPRVec16(op.rb);
	Value *rb_result = Intrinsic_pshufb(rb, rb_mask);

	Value *ra_mask = m_ir->CreateXor(rb_mask, GetVec16(_mm_set1_epi8(-0x10)));
	Value *ra = LoadGPRVec16(op.ra);
	Value *ra_result = Intrinsic_pshufb(ra, ra_mask);

	//merge rb_result and ra_result
	Value *result = m_ir->CreateOr(rb_result, ra_result);

	//merge everything else
	result = m_ir->CreateAnd(m_ir->CreateNot(result_10), result);
	result = m_ir->CreateOr(result, result_111);

	StoreGPR128(op.rt4, result);
}

void spu_llvm_recompiler::MPYA(spu_opcode_t op)
{
	Value *mask = GetVec4(_mm_set1_epi32(0xffff));
	Value *ra = m_ir->CreateAnd(LoadGPRVec4(op.ra), mask);
	Value *rb = m_ir->CreateAnd(LoadGPRVec4(op.rb), mask);
	Value *rc = LoadGPRVec4(op.rc);
	Value *rt = m_ir->CreateMul(ra, rb);
	rt = m_ir->CreateAdd(rt, rc);

	StoreGPR128(op.rt4, rt);
}

void spu_llvm_recompiler::FNMS(spu_opcode_t op)
{
	//might be incorrect
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *rb = LoadGPRVec4Float(op.rb);
	Value *rc = LoadGPRVec4Float(op.rc);

	Value *tmp_a = GetVec4Float(_mm_set1_epi32(0));
	Value *tmp_b = GetVec4Float(_mm_set1_epi32(0));
	tmp_a = m_ir->CreateSelect(m_ir->CreateFCmpUNO(tmp_a, ra), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	tmp_a = m_ir->CreateAnd(m_ir->CreateNot(tmp_a), m_ir->CreateBitCast(ra, VectorType::get(m_ir->getInt32Ty(), 4)));

	tmp_b = m_ir->CreateSelect(m_ir->CreateFCmpUNO(tmp_b, rb), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	tmp_b = m_ir->CreateAnd(m_ir->CreateNot(tmp_b), m_ir->CreateBitCast(rb, VectorType::get(m_ir->getInt32Ty(), 4)));

	tmp_a = m_ir->CreateBitCast(tmp_a, VectorType::get(m_ir->getFloatTy(), 4));
	tmp_b = m_ir->CreateBitCast(tmp_b, VectorType::get(m_ir->getFloatTy(), 4));

	Value *rt = m_ir->CreateFMul(tmp_a, tmp_b);
	rt = m_ir->CreateFSub(rc, rt);
	StoreGPR128(op.rt4, rt);
}

void spu_llvm_recompiler::FMA(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *rb = LoadGPRVec4Float(op.rb);
	Value *rc = LoadGPRVec4Float(op.rc);

	Value *tmp_a = GetVec4Float(_mm_set1_epi32(0));
	Value *tmp_b = GetVec4Float(_mm_set1_epi32(0));
	tmp_a = m_ir->CreateSelect(m_ir->CreateFCmpUNO(tmp_a, ra), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	tmp_a = m_ir->CreateAnd(m_ir->CreateNot(tmp_a), m_ir->CreateBitCast(ra, VectorType::get(m_ir->getInt32Ty(), 4)));

	tmp_b = m_ir->CreateSelect(m_ir->CreateFCmpUNO(tmp_b, rb), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	tmp_b = m_ir->CreateAnd(m_ir->CreateNot(tmp_b), m_ir->CreateBitCast(rb, VectorType::get(m_ir->getInt32Ty(), 4)));

	tmp_a = m_ir->CreateBitCast(tmp_a, VectorType::get(m_ir->getFloatTy(), 4));
	tmp_b = m_ir->CreateBitCast(tmp_b, VectorType::get(m_ir->getFloatTy(), 4));

	Value *rt = m_ir->CreateFMul(tmp_a, tmp_b);
	rt = m_ir->CreateFAdd(rt, rc);
	StoreGPR128(op.rt4, rt);
}

void spu_llvm_recompiler::FMS(spu_opcode_t op)
{
	Value *ra = LoadGPRVec4Float(op.ra);
	Value *rb = LoadGPRVec4Float(op.rb);
	Value *rc = LoadGPRVec4Float(op.rc);

	Value *tmp_a = GetVec4Float(_mm_set1_epi32(0));
	Value *tmp_b = GetVec4Float(_mm_set1_epi32(0));
	tmp_a = m_ir->CreateSelect(m_ir->CreateFCmpUNO(tmp_a, ra), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	tmp_a = m_ir->CreateAnd(m_ir->CreateNot(tmp_a), m_ir->CreateBitCast(ra, VectorType::get(m_ir->getInt32Ty(), 4)));

	tmp_b = m_ir->CreateSelect(m_ir->CreateFCmpUNO(tmp_b, rb), GetVec4(_mm_set1_epi32(0xFFFFFFFF)), GetVec4(_mm_set1_epi32(0)));
	tmp_b = m_ir->CreateAnd(m_ir->CreateNot(tmp_b), m_ir->CreateBitCast(rb, VectorType::get(m_ir->getInt32Ty(), 4)));

	tmp_a = m_ir->CreateBitCast(tmp_a, VectorType::get(m_ir->getFloatTy(), 4));
	tmp_b = m_ir->CreateBitCast(tmp_b, VectorType::get(m_ir->getFloatTy(), 4));

	Value *rt = m_ir->CreateFMul(tmp_a, tmp_b);
	rt = m_ir->CreateFSub(rt, rc);
	StoreGPR128(op.rt4, rt);
}

void spu_llvm_recompiler::UNK(spu_opcode_t op)
{
	fmt::throw_exception("Unknown opcode %x" HERE, op.opcode);
}


