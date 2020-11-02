#ifdef LLVM_AVAILABLE

#include "CPUTranslator.h"

llvm::LLVMContext g_llvm_ctx;

cpu_translator::cpu_translator(llvm::Module* _module, bool is_be)
    : m_context(g_llvm_ctx)
	, m_module(_module)
	, m_is_be(is_be)
{
}

void cpu_translator::initialize(llvm::LLVMContext& context, llvm::ExecutionEngine& engine)
{
	m_context = context;
	m_engine = &engine;

	const auto cpu = m_engine->getTargetMachine()->getTargetCPU();

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

	// Test FMA feature (TODO)
	if (cpu == "haswell" ||
		cpu == "broadwell" ||
		cpu == "skylake" ||
		cpu == "bdver2" ||
		cpu == "bdver3" ||
		cpu == "bdver4" ||
		cpu.startswith("znver"))
	{
		m_use_fma = true;
	}

	// Test AVX-512 feature (TODO)
	if (cpu == "skylake-avx512" ||
		cpu == "cascadelake" ||
		cpu == "cannonlake" ||
		cpu == "cooperlake" ||
		cpu == "icelake" ||
		cpu == "icelake-client" ||
		cpu == "icelake-server" ||
		cpu == "tigerlake")
	{
		m_use_fma = true;
	}

	// Test AVX-512_icelake features (TODO)
	if (cpu == "icelake" ||
		cpu == "icelake-client" ||
		cpu == "icelake-server" ||
		cpu == "tigerlake")
	{
		m_use_avx512_icl = true;
	}
}

llvm::Value* cpu_translator::bitcast(llvm::Value* val, llvm::Type* type)
{
	uint s1 = type->getScalarSizeInBits();
	uint s2 = val->getType()->getScalarSizeInBits();

	if (type->isVectorTy())
		s1 *= llvm::cast<llvm::VectorType>(type)->getNumElements();
	if (val->getType()->isVectorTy())
		s2 *= llvm::cast<llvm::VectorType>(val->getType())->getNumElements();

	if (s1 != s2)
	{
		fmt::throw_exception("cpu_translator::bitcast(): incompatible type sizes (%u vs %u)", s1, s2);
	}

	if (const auto c1 = llvm::dyn_cast<llvm::Constant>(val))
	{
		return verify(HERE, llvm::ConstantFoldCastOperand(llvm::Instruction::BitCast, c1, type, m_module->getDataLayout()));
	}

	return m_ir->CreateBitCast(val, type);
}

template <>
std::pair<bool, v128> cpu_translator::get_const_vector<v128>(llvm::Value* c, u32 a, u32 b)
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
			auto cv = ci->getValue();

			for (int i = 0; i < 128; i++)
			{
				result._bit[i] = cv[i];
			}

			return {true, result};
		}

		fmt::throw_exception("[0x%x, %u] Not a vector" HERE, a, b);
	}

	if (auto v = llvm::cast<llvm::VectorType>(t); v->getScalarSizeInBits() * v->getNumElements() != 128)
	{
		fmt::throw_exception("[0x%x, %u] Bad vector size: i%ux%u" HERE, a, b, v->getScalarSizeInBits(), v->getNumElements());
	}

	const auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(c);

	if (!cv)
	{
		if (llvm::isa<llvm::ConstantAggregateZero>(c))
		{
			return {};
		}

		if (llvm::isa<llvm::ConstantExpr>(c))
		{
			// Sorry, if we cannot evaluate it we cannot use it
			fmt::throw_exception("[0x%x, %u] Constant Expression!" HERE, a, b);
		}

		fmt::throw_exception("[0x%x, %u] Unexpected constant type" HERE, a, b);
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
		fmt::throw_exception("[0x%x, %u] Unexpected vector element type" HERE, a, b);
	}

	return {true, result};
}

template <>
llvm::Constant* cpu_translator::make_const_vector<v128>(v128 v, llvm::Type* t)
{
	if (const auto ct = llvm::dyn_cast<llvm::IntegerType>(t); ct && ct->getBitWidth() == 128)
	{
		return llvm::ConstantInt::get(t, llvm::APInt(128, llvm::makeArrayRef(reinterpret_cast<const u64*>(v._bytes), 2)));
	}

	verify(HERE), t->isVectorTy();
	verify(HERE), 128 == t->getScalarSizeInBits() * llvm::cast<llvm::VectorType>(t)->getNumElements();

	const auto sct = t->getScalarType();

	if (sct->isIntegerTy(8))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef(reinterpret_cast<const u8*>(v._bytes), 16));
	}
	else if (sct->isIntegerTy(16))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef(reinterpret_cast<const u16*>(v._bytes), 8));
	}
	else if (sct->isIntegerTy(32))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef(reinterpret_cast<const u32*>(v._bytes), 4));
	}
	else if (sct->isIntegerTy(64))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef(reinterpret_cast<const u64*>(v._bytes), 2));
	}
	else if (sct->isFloatTy())
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef(reinterpret_cast<const f32*>(v._bytes), 4));
	}
	else if (sct->isDoubleTy())
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef(reinterpret_cast<const f64*>(v._bytes), 2));
	}

	fmt::raw_error("No supported constant type" HERE);
}

#endif
