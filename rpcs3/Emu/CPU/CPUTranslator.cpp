#ifdef LLVM_AVAILABLE

#include "CPUTranslator.h"

llvm::LLVMContext g_llvm_ctx;

cpu_translator::cpu_translator(llvm::Module* module, bool is_be)
    : m_context(g_llvm_ctx)
	, m_module(module)
	, m_is_be(is_be)
{

}

template <>
v128 cpu_translator::get_const_vector<v128>(llvm::Constant* c, u32 a, u32 b)
{
	const auto t = c->getType();

	if (!t->isVectorTy())
	{
		fmt::throw_exception("[0x%x, %u] Not a vector" HERE, a, b);
	}

	if (uint sz = llvm::cast<llvm::VectorType>(t)->getBitWidth() - 128)
	{
		fmt::throw_exception("[0x%x, %u] Bad vector size: %u" HERE, a, b, sz + 128);
	}

	const auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(c);

	if (!cv)
	{
		if (llvm::isa<llvm::ConstantAggregateZero>(c))
		{
			return {};
		}

		fmt::throw_exception("[0x%x, %u] Unexpected constant type" HERE, a, b);
	}

	const auto sct = t->getScalarType();

	v128 result;

	if (sct->isIntegerTy(8))
	{
		for (u32 i = 0; i < 16; i++)
		{
			result._u8[i] = cv->getElementAsInteger(i);
		}
	}
	else if (sct->isIntegerTy(16))
	{
		for (u32 i = 0; i < 8; i++)
		{
			result._u16[i] = cv->getElementAsInteger(i);
		}
	}
	else if (sct->isIntegerTy(32))
	{
		for (u32 i = 0; i < 4; i++)
		{
			result._u32[i] = cv->getElementAsInteger(i);
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

	return result;
}

template <>
llvm::Constant* cpu_translator::make_const_vector<v128>(v128 v, llvm::Type* t)
{
	verify(HERE), t->isVectorTy() && llvm::cast<llvm::VectorType>(t)->getBitWidth() == 128;

	const auto sct = t->getScalarType();

	if (sct->isIntegerTy(8))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const u8*)v._bytes, 16));
	}
	else if (sct->isIntegerTy(16))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const u16*)v._bytes, 8));
	}
	else if (sct->isIntegerTy(32))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const u32*)v._bytes, 4));
	}
	else if (sct->isIntegerTy(64))
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const u64*)v._bytes, 2));
	}
	else if (sct->isFloatTy())
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const f32*)v._bytes, 4));
	}
	else if (sct->isDoubleTy())
	{
		return llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const f64*)v._bytes, 2));
	}

	fmt::raw_error("No supported constant type" HERE);
}

#endif
