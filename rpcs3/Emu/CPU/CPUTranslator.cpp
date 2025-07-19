#ifdef LLVM_AVAILABLE

#include "CPUTranslator.h"

#include "util/v128.hpp"
#include "util/logs.hpp"

LOG_CHANNEL(llvm_log, "LLVM");

llvm::LLVMContext g_llvm_ctx;

llvm::Value* peek_through_bitcasts(llvm::Value* arg)
{
	llvm::CastInst* i;

	while ((i = llvm::dyn_cast_or_null<llvm::CastInst>(arg)) && i->getOpcode() == llvm::Instruction::BitCast)
	{
		arg = i->getOperand(0);
	}

	return arg;
}

cpu_translator::cpu_translator(llvm::Module* _module, bool is_be)
    : m_context(g_llvm_ctx)
	, m_module(_module)
	, m_is_be(is_be)
{
	register_intrinsic("x86_pshufb", [&](llvm::CallInst* ci) -> llvm::Value*
	{
		const auto data0 = ci->getOperand(0);
		const auto index = ci->getOperand(1);
		const auto zeros = llvm::ConstantAggregateZero::get(get_type<u8[16]>());

		if (m_use_ssse3)
		{
#if defined(ARCH_X64)
			return m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_ssse3_pshuf_b_128), {data0, index});
#elif defined(ARCH_ARM64)
			// Modified from sse2neon
			// movi    v2.16b, #143
			// and     v1.16b, v1.16b, v2.16b
			// tbl     v0.16b, { v0.16b }, v1.16b
			auto mask = llvm::ConstantInt::get(get_type<u8[16]>(), 0x8F);
			auto and_mask = llvm::ConstantInt::get(get_type<bool[16]>(), true);
			auto vec_len = llvm::ConstantInt::get(get_type<u32>(), 16);
			auto index_masked = m_ir->CreateCall(get_intrinsic<u8[16]>(llvm::Intrinsic::vp_and), {index, mask, and_mask, vec_len});
			return m_ir->CreateCall(get_intrinsic<u8[16]>(llvm::Intrinsic::aarch64_neon_tbl1), {data0, index_masked});
#else
#error "Unimplemented"
#endif
		}
		else
		{
			// Emulate PSHUFB (TODO)
			const auto mask = m_ir->CreateAnd(index, 0xf);
			const auto loop = llvm::BasicBlock::Create(m_context, "", m_ir->GetInsertBlock()->getParent());
			const auto prev = ci->getParent();
			const auto next = prev->splitBasicBlock(ci->getNextNode());

			llvm::cast<llvm::BranchInst>(m_ir->GetInsertBlock()->getTerminator())->setOperand(0, loop);

			llvm::Value* result;
			//m_ir->CreateBr(loop);
			m_ir->SetInsertPoint(loop);
			const auto i = m_ir->CreatePHI(get_type<u32>(), 2);
			const auto v = m_ir->CreatePHI(get_type<u8[16]>(), 2);
			i->addIncoming(m_ir->getInt32(0), prev);
			i->addIncoming(m_ir->CreateAdd(i, m_ir->getInt32(1)), loop);
			v->addIncoming(zeros, prev);
			result = m_ir->CreateInsertElement(v, m_ir->CreateExtractElement(data0, m_ir->CreateExtractElement(mask, i)), i);
			v->addIncoming(result, loop);
			m_ir->CreateCondBr(m_ir->CreateICmpULT(i, m_ir->getInt32(16)), loop, next);
			m_ir->SetInsertPoint(next->getFirstNonPHI());
			result = m_ir->CreateSelect(m_ir->CreateICmpSLT(index, zeros), zeros, result);

			return result;
		}
	});

	register_intrinsic("any_select_by_bit4", [&](llvm::CallInst* ci) -> llvm::Value*
	{
		const auto s = bitcast<s8[16]>(m_ir->CreateShl(bitcast<u64[2]>(ci->getOperand(0)), 3));;
		const auto a = bitcast<u8[16]>(ci->getOperand(1));
		const auto b = bitcast<u8[16]>(ci->getOperand(2));
		return m_ir->CreateSelect(m_ir->CreateICmpSLT(s, llvm::ConstantAggregateZero::get(get_type<s8[16]>())), b, a);
	});
}

void cpu_translator::initialize(llvm::LLVMContext& context, llvm::ExecutionEngine& engine)
{
	m_context = context;
	m_engine = &engine;

	auto cpu = m_engine->getTargetMachine()->getTargetCPU();

	if (cpu == "generic")
	{
		// Detection failed, try to guess
		cpu = fallback_cpu_detection();
	}

	// Test SSSE3 feature (TODO)
	if (cpu == "generic" ||
		cpu == "k8" ||
		cpu == "opteron" ||
		cpu == "athlon64" ||
		cpu == "athlon-fx" ||
		cpu == "k8-sse3" ||
		cpu == "opteron-sse3" ||
		cpu == "athlon64-sse3" ||
		cpu == "amdfam10" ||
		cpu == "barcelona")
	{
		m_use_ssse3 = false;
	}

	// Test AVX feature (TODO)
	if (cpu == "sandybridge" ||
		cpu == "ivybridge" ||
		cpu == "bdver1")
	{
		m_use_avx = true;
	}

	// Test FMA feature (TODO)
	if (cpu == "haswell" ||
		cpu == "broadwell" ||
		cpu == "skylake" ||
		cpu == "alderlake" ||
		cpu == "raptorlake" ||
		cpu == "meteorlake" ||
		cpu == "bdver2" ||
		cpu == "bdver3" ||
		cpu == "bdver4" ||
		cpu == "znver1" ||
		cpu == "znver2" ||
		cpu == "znver3" ||
		cpu == "arrowlake" ||
		cpu == "arrowlake-s" ||
		cpu == "lunarlake")
	{
		m_use_fma = true;
		m_use_avx = true;
	}

	// Test AVX-512 feature (TODO)
	if (cpu == "skylake-avx512" ||
		cpu == "cascadelake" ||
		cpu == "cannonlake" ||
		cpu == "cooperlake")
	{
		m_use_avx = true;
		m_use_fma = true;
		m_use_avx512 = true;
	}

	// Test VNNI feature (TODO)
	if (cpu == "cascadelake" ||
		cpu == "cooperlake" ||
		cpu == "alderlake" ||
		cpu == "raptorlake" ||
		cpu == "meteorlake" ||
		cpu == "arrowlake" ||
		cpu == "arrowlake-s" ||
		cpu == "lunarlake")
	{
		m_use_vnni = true;
	}

	// Test GFNI feature (TODO)
	if (cpu == "tremont" ||
		cpu == "gracemont" ||
		cpu == "alderlake" ||
		cpu == "raptorlake" ||
		cpu == "meteorlake" ||
		cpu == "arrowlake" ||
		cpu == "arrowlake-s" ||
		cpu == "lunarlake")
	{
		m_use_gfni = true;
	}

	// Test AVX-512_icelake features (TODO)
	if (cpu == "icelake" ||
		cpu == "icelake-client" ||
		cpu == "icelake-server" ||
		cpu == "tigerlake" ||
		cpu == "rocketlake" ||
		cpu == "sapphirerapids" ||
		(cpu.starts_with("znver") && cpu != "znver1" && cpu != "znver2" && cpu != "znver3"))
	{
		m_use_avx = true;
		m_use_fma = true;
		m_use_avx512 = true;
		m_use_avx512_icl = true;
		m_use_vnni = true;
		m_use_gfni = true;
	}

	// Aarch64 CPUs
	if (cpu == "cyclone" || cpu.contains("cortex"))
	{
		m_use_fma = true;
		// AVX does not use intrinsics so far
		m_use_avx = true;
	}
}

llvm::Value* cpu_translator::bitcast(llvm::Value* val, llvm::Type* type) const
{
	uint s1 = type->getScalarSizeInBits();
	uint s2 = val->getType()->getScalarSizeInBits();

	if (type->isVectorTy())
		s1 *= llvm::cast<llvm::FixedVectorType>(type)->getNumElements();
	if (val->getType()->isVectorTy())
		s2 *= llvm::cast<llvm::FixedVectorType>(val->getType())->getNumElements();

	if (s1 != s2)
	{
		fmt::throw_exception("cpu_translator::bitcast(): incompatible type sizes (%u vs %u)", s1, s2);
	}

	if (const auto c1 = llvm::dyn_cast<llvm::Constant>(val))
	{
		return ensure(llvm::ConstantFoldCastOperand(llvm::Instruction::BitCast, c1, type, m_module->getDataLayout()));
	}

	return m_ir->CreateBitCast(val, type);
}

template <>
std::pair<bool, v128> cpu_translator::get_const_vector<v128>(llvm::Value* c, u32 _pos, u32 _line)
{
	v128 result{};

	if (!llvm::isa<llvm::Constant>(c))
	{
		return {false, result};
	}

	const auto t = c->getType();

	if (!t->isVectorTy())
	{
		if (const auto ci = llvm::dyn_cast<llvm::ConstantInt>(c); ci && ci->getBitWidth() == 128)
		{
			const auto& cv = ci->getValue();

			result._u64[0] = cv.extractBitsAsZExtValue(64, 0);
			result._u64[1] = cv.extractBitsAsZExtValue(64, 64);

			return {true, result};
		}

		fmt::throw_exception("[0x%x, %u] Not a vector", _pos, _line);
	}

	if (auto v = llvm::cast<llvm::FixedVectorType>(t); v->getScalarSizeInBits() * v->getNumElements() != 128)
	{
		fmt::throw_exception("[0x%x, %u] Bad vector size: i%ux%u", _pos, _line, v->getScalarSizeInBits(), v->getNumElements());
	}

	const auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(c);

	if (!cv)
	{
		if (llvm::isa<llvm::ConstantAggregateZero>(c))
		{
			return {true, result};
		}

		std::string result;
		llvm::raw_string_ostream out(result);
		c->print(out, true);
		out.flush();

		if (llvm::isa<llvm::ConstantExpr>(c))
		{
			// Sorry, if we cannot evaluate it we cannot use it
			fmt::throw_exception("[0x%x, %u] Constant Expression!\n%s", _pos, _line, result);
		}

		fmt::throw_exception("[0x%x, %u] Unexpected constant type!\n%s", _pos, _line, result);
	}

	const auto sct = t->getScalarType();

	if (sct->isIntegerTy(8))
	{
		for (u32 i = 0; i < 16; i++)
		{
			result._u8[i] = static_cast<u8>(cv->getElementAsInteger(i));
		}
	}
	else if (sct->isIntegerTy(16))
	{
		for (u32 i = 0; i < 8; i++)
		{
			result._u16[i] = static_cast<u16>(cv->getElementAsInteger(i));
		}
	}
	else if (sct->isIntegerTy(32))
	{
		for (u32 i = 0; i < 4; i++)
		{
			result._u32[i] = static_cast<u32>(cv->getElementAsInteger(i));
		}
	}
	else if (sct->isIntegerTy(64))
	{
		for (u32 i = 0; i < 2; i++)
		{
			result._u64[i] = cv->getElementAsInteger(i);
		}
	}
	else if (sct->isFloatTy())
	{
		for (u32 i = 0; i < 4; i++)
		{
			result._f[i] = cv->getElementAsFloat(i);
		}
	}
	else if (sct->isDoubleTy())
	{
		for (u32 i = 0; i < 2; i++)
		{
			result._d[i] = cv->getElementAsDouble(i);
		}
	}
	else
	{
		fmt::throw_exception("[0x%x, %u] Unexpected vector element type", _pos, _line);
	}

	return {true, result};
}

template <>
llvm::Constant* cpu_translator::make_const_vector<v128>(v128 v, llvm::Type* t, u32 _line)
{
	if (const auto ct = llvm::dyn_cast<llvm::IntegerType>(t); ct && ct->getBitWidth() == 128)
	{
		return llvm::ConstantInt::get(t, llvm::APInt(128, llvm::ArrayRef(reinterpret_cast<const u64*>(v._bytes), 2)));
	}

	ensure(t->isVectorTy());
	ensure(128 == t->getScalarSizeInBits() * llvm::cast<llvm::FixedVectorType>(t)->getNumElements());

	const auto sct = t->getScalarType();

	if (sct->isIntegerTy(8))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u8*>(v._bytes), 16));
	}
	if (sct->isIntegerTy(16))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u16*>(v._bytes), 8));
	}
	if (sct->isIntegerTy(32))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u32*>(v._bytes), 4));
	}
	if (sct->isIntegerTy(64))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u64*>(v._bytes), 2));
	}
	if (sct->isFloatTy())
	{
		return llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const f32*>(v._bytes), 4));
	}
	if (sct->isDoubleTy())
	{
		return llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const f64*>(v._bytes), 2));
	}

	fmt::throw_exception("[line %u] No supported constant type", _line);
}

void cpu_translator::replace_intrinsics(llvm::Function& f)
{
	for (llvm::BasicBlock& bb : f)
	{
		std::set<std::string, std::less<>> names;

		using InstListType = llvm::BasicBlock::InstListType;

		std::function<InstListType::iterator(InstListType::iterator)> fix_funcs;

		fix_funcs = [&](InstListType::iterator inst_bit)
		{
			auto ci = llvm::dyn_cast<llvm::CallInst>(&*inst_bit);

			if (!ci)
			{
				return std::next(inst_bit);
			}

			const auto cf = ci->getCalledFunction();

			if (!cf)
			{
				return std::next(inst_bit);
			}

			std::string_view func_name{cf->getName().data(), cf->getName().size()};

			const auto it = m_intrinsics.find(func_name);

			if (it == m_intrinsics.end())
			{
				return std::next(inst_bit);
			}

			if (!names.empty())
			{
				llvm_log.trace("cpu_translator::replace_intrinsics(): function '%s' names_size=%d, names[0]=%s", func_name, names.size(), *names.begin());
			}

			if (names.contains(func_name))
			{
				fmt::throw_exception("cpu_translator::replace_intrinsics(): Recursion detected at function '%s'!", func_name);
			}

			names.emplace(std::string(func_name));

			// Set insert point after call instruction
			// In order to obtain a clear range of the inserted instructions
			if (llvm::Instruction* next = ci->getNextNode())
			{
				m_ir->SetInsertPoint(next);
			}
			else
			{
				m_ir->SetInsertPoint(std::addressof(bb));
			}

			ci->replaceAllUsesWith(it->second(ci));

			InstListType::iterator end = m_ir->GetInsertPoint();

			for (InstListType::iterator next_it = ci->eraseFromParent(), inner = next_it; inner != end;)
			{
				if (llvm::isa<llvm::CallInst>(&*inner))
				{
					inner = fix_funcs(inner);
				}
				else
				{
					inner++;
				}
			}

			// TODO: Simplify in C++23 with 'names.erase(func_name);'
			names.erase(ensure(names.find(func_name), FN(x != names.end())));
			return end;
		};

		for (auto bit = bb.begin(); bit != bb.end(); bit = fix_funcs(bit))
		{
		}
	}
}

void cpu_translator::run_transforms(llvm::Function& f)
{
	// This pass must run first because the other passes may depend on resolved names.
	replace_intrinsics(f);

	for (auto& pass : m_transform_passes)
	{
		pass->run(m_ir, f);
	}
}

void cpu_translator::register_transform_pass(std::unique_ptr<translator_pass>& pass)
{
	m_transform_passes.emplace_back(std::move(pass));
}

void cpu_translator::clear_transforms()
{
	m_transform_passes.clear();
}

void cpu_translator::reset_transforms()
{
	for (auto& pass : m_transform_passes)
	{
		pass->reset();
	}
}

void cpu_translator::erase_stores(llvm::ArrayRef<llvm::Value*> args)
{
	for (auto v : args)
	{
		for (auto it = v->use_begin(); it != v->use_end(); ++it)
		{
			llvm::Value* i = *it;
			llvm::CastInst* bci = nullptr;

			// Walk through bitcasts
			while (i && (bci = llvm::dyn_cast<llvm::CastInst>(i)) && bci->getOpcode() == llvm::Instruction::BitCast)
			{
				i = *bci->use_begin();
			}

			if (auto si = llvm::dyn_cast_or_null<llvm::StoreInst>(i))
			{
				si->eraseFromParent();
			}
		}
	}
}

#endif
