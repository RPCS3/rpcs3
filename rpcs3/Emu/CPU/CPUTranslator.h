#pragma once

#ifdef LLVM_AVAILABLE

#include "restore_new.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include "define_new_memleakdetect.h"

#include "../Utilities/types.h"
#include "../Utilities/StrFmt.h"
#include "../Utilities/BEType.h"
#include "../Utilities/BitField.h"

#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <array>
#include <vector>

enum class i2 : char
{
};

enum class i4 : char
{
};

template <typename T = void>
struct llvm_value_t
{
	static_assert(std::is_same<T, void>::value, "llvm_value_t<> error: unknown type");

	using type = void;
	using base = llvm_value_t;
	static constexpr uint esize      = 0;
	static constexpr bool is_int     = false;
	static constexpr bool is_sint    = false;
	static constexpr bool is_uint    = false;
	static constexpr bool is_float   = false;
	static constexpr uint is_vector  = false;
	static constexpr uint is_pointer = false;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getVoidTy(context);
	}

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return value;
	}

	llvm::Value* value;

	// llvm_value_t() = default;

	// llvm_value_t(llvm::Value* value)
	// 	: value(value)
	// {
	// }
};

template <>
struct llvm_value_t<bool> : llvm_value_t<void>
{
	using type = bool;
	using base = llvm_value_t<void>;
	using base::base;

	static constexpr uint esize  = 1;
	static constexpr uint is_int = true;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getInt1Ty(context);
	}
};

template <>
struct llvm_value_t<i2> : llvm_value_t<void>
{
	using type = i2;
	using base = llvm_value_t<void>;
	using base::base;

	static constexpr uint esize  = 2;
	static constexpr uint is_int = true;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getIntNTy(context, 2);
	}
};

template <>
struct llvm_value_t<i4> : llvm_value_t<void>
{
	using type = i4;
	using base = llvm_value_t<void>;
	using base::base;

	static constexpr uint esize  = 4;
	static constexpr uint is_int = true;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getIntNTy(context, 4);
	}
};

template <>
struct llvm_value_t<char> : llvm_value_t<void>
{
	using type = char;
	using base = llvm_value_t<void>;
	using base::base;

	static constexpr uint esize  = 8;
	static constexpr bool is_int = true;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getInt8Ty(context);
	}
};

template <>
struct llvm_value_t<s8> : llvm_value_t<char>
{
	using type = s8;
	using base = llvm_value_t<char>;
	using base::base;

	static constexpr bool is_sint = true;
};

template <>
struct llvm_value_t<u8> : llvm_value_t<char>
{
	using type = u8;
	using base = llvm_value_t<char>;
	using base::base;

	static constexpr bool is_uint = true;
};

template <>
struct llvm_value_t<s16> : llvm_value_t<s8>
{
	using type = s16;
	using base = llvm_value_t<s8>;
	using base::base;

	static constexpr uint esize = 16;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getInt16Ty(context);
	}
};

template <>
struct llvm_value_t<u16> : llvm_value_t<s16>
{
	using type = u16;
	using base = llvm_value_t<s16>;
	using base::base;

	static constexpr bool is_sint = false;
	static constexpr bool is_uint = true;
};

template <>
struct llvm_value_t<s32> : llvm_value_t<s8>
{
	using type = s32;
	using base = llvm_value_t<s8>;
	using base::base;

	static constexpr uint esize = 32;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getInt32Ty(context);
	}
};

template <>
struct llvm_value_t<u32> : llvm_value_t<s32>
{
	using type = u32;
	using base = llvm_value_t<s32>;
	using base::base;

	static constexpr bool is_sint = false;
	static constexpr bool is_uint = true;
};

template <>
struct llvm_value_t<s64> : llvm_value_t<s8>
{
	using type = s64;
	using base = llvm_value_t<s8>;
	using base::base;

	static constexpr uint esize = 64;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getInt64Ty(context);
	}
};

template <>
struct llvm_value_t<u64> : llvm_value_t<s64>
{
	using type = u64;
	using base = llvm_value_t<s64>;
	using base::base;

	static constexpr bool is_sint = false;
	static constexpr bool is_uint = true;
};

template <>
struct llvm_value_t<s128> : llvm_value_t<s8>
{
	using type = s128;
	using base = llvm_value_t<s8>;
	using base::base;

	static constexpr uint esize = 128;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getIntNTy(context, 128);
	}
};

template <>
struct llvm_value_t<u128> : llvm_value_t<s128>
{
	using type = u128;
	using base = llvm_value_t<s128>;
	using base::base;

	static constexpr bool is_sint = false;
	static constexpr bool is_uint = true;
};

template <>
struct llvm_value_t<f32> : llvm_value_t<void>
{
	using type = f32;
	using base = llvm_value_t<void>;
	using base::base;

	static constexpr uint esize    = 32;
	static constexpr bool is_float = true;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getFloatTy(context);
	}
};

template <>
struct llvm_value_t<f64> : llvm_value_t<void>
{
	using type = f64;
	using base = llvm_value_t<void>;
	using base::base;

	static constexpr uint esize    = 64;
	static constexpr bool is_float = true;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getDoubleTy(context);
	}
};

template <typename T>
struct llvm_value_t<T*> : llvm_value_t<T>
{
	static_assert(!std::is_void<T>::value, "llvm_value_t<> error: invalid pointer to void type");

	using type = T*;
	using base = llvm_value_t<T>;
	using base::base;

	static constexpr uint is_pointer = llvm_value_t<T>::is_pointer + 1;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm_value_t<T>::get_type(context)->getPointerTo();
	}
};

template <typename T, uint N>
struct llvm_value_t<T[N]> : llvm_value_t<T>
{
	static_assert(!llvm_value_t<T>::is_vector, "llvm_value_t<> error: invalid multidimensional vector");
	static_assert(!llvm_value_t<T>::is_pointer, "llvm_value_t<>: vector of pointers is not allowed");

	using type = T[N];
	using base = llvm_value_t<T>;
	using base::base;

	static constexpr uint is_vector  = N;
	static constexpr uint is_pointer = 0;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::VectorType::get(llvm_value_t<T>::get_type(context), N);
	}
};

template <typename T, typename A1, typename A2>
struct llvm_add_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_add_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return ir->CreateAdd(v1, v2);
		}

		if (llvm_value_t<T>::is_float)
		{
			return ir->CreateFAdd(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_add_t<typename T1::type, T1, T2> operator +(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T, typename A1>
struct llvm_add_const_t
{
	using type = T;

	A1 a1;
	u64 c;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_add_const_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return ir->CreateAdd(a1.eval(ir), llvm::ConstantInt::get(llvm_value_t<T>::get_type(ir->getContext()), c, llvm_value_t<T>::is_sint));
	}
};

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_add_const_t<typename T1::type, T1> operator +(T1 a1, u64 c)
{
	return {a1, c};
}

template <typename T, typename A1, typename A2>
struct llvm_sub_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_sub_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return ir->CreateSub(v1, v2);
		}

		if (llvm_value_t<T>::is_float)
		{
			return ir->CreateFSub(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_sub_t<typename T1::type, T1, T2> operator -(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T, typename A1>
struct llvm_sub_const_t
{
	using type = T;

	A1 a1;
	u64 c;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_sub_const_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return ir->CreateSub(a1.eval(ir), llvm::ConstantInt::get(llvm_value_t<T>::get_type(ir->getContext()), c, llvm_value_t<T>::is_sint));
	}
};

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_sub_const_t<typename T1::type, T1> operator -(T1 a1, u64 c)
{
	return {a1, c};
}

template <typename T, typename A1>
struct llvm_const_sub_t
{
	using type = T;

	A1 a1;
	u64 c;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_const_sub_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return ir->CreateSub(llvm::ConstantInt::get(llvm_value_t<T>::get_type(ir->getContext()), c, llvm_value_t<T>::is_sint), a1.eval(ir));
	}
};

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_const_sub_t<typename T1::type, T1> operator -(u64 c, T1 a1)
{
	return {a1, c};
}

template <typename T, typename A1, typename A2>
struct llvm_mul_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_mul_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return ir->CreateMul(v1, v2);
		}

		if (llvm_value_t<T>::is_float)
		{
			return ir->CreateFMul(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_mul_t<typename T1::type, T1, T2> operator *(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T, typename A1, typename A2>
struct llvm_div_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_div_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_sint)
		{
			return ir->CreateSDiv(v1, v2);
		}

		if (llvm_value_t<T>::is_uint)
		{
			return ir->CreateUDiv(v1, v2);
		}

		if (llvm_value_t<T>::is_float)
		{
			return ir->CreateFDiv(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_div_t<typename T1::type, T1, T2> operator /(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T, typename A1>
struct llvm_neg_t
{
	using type = T;

	A1 a1;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_neg_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return ir->CreateNeg(v1);
		}

		if (llvm_value_t<T>::is_float)
		{
			return ir->CreateFNeg(v1);
		}
	}
};

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<(llvm_value_t<typename T1::type>::esize > 1)>>
inline llvm_neg_t<typename T1::type, T1> operator -(T1 a1)
{
	return {a1};
}

// Constant int helper
struct llvm_int_t
{
	u64 value;

	u64 eval(llvm::IRBuilder<>*) const
	{
		return value;
	}
};

template <typename T, typename A1, typename A2>
struct llvm_shl_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_shl_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_sint)
		{
			return ir->CreateShl(v1, v2);
		}

		if (llvm_value_t<T>::is_uint)
		{
			return ir->CreateShl(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_shl_t<typename T1::type, T1, T2> operator <<(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_shl_t<typename T1::type, T1, llvm_int_t> operator <<(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T, typename A1, typename A2>
struct llvm_shr_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_shr_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_sint)
		{
			return ir->CreateAShr(v1, v2);
		}

		if (llvm_value_t<T>::is_uint)
		{
			return ir->CreateLShr(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_shr_t<typename T1::type, T1, T2> operator >>(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_shr_t<typename T1::type, T1, llvm_int_t> operator >>(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T, typename A1, typename A2>
struct llvm_and_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_int, "llvm_and_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return ir->CreateAnd(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_and_t<typename T1::type, T1, T2> operator &(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_and_t<typename T1::type, T1, llvm_int_t> operator &(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T, typename A1, typename A2>
struct llvm_or_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_int, "llvm_or_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return ir->CreateOr(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_or_t<typename T1::type, T1, T2> operator |(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_or_t<typename T1::type, T1, llvm_int_t> operator |(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T, typename A1, typename A2>
struct llvm_xor_t
{
	using type = T;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_int, "llvm_xor_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return ir->CreateXor(v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_xor_t<typename T1::type, T1, T2> operator ^(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_xor_t<typename T1::type, T1, llvm_int_t> operator ^(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T, typename A1>
struct llvm_not_t
{
	using type = T;

	A1 a1;

	static_assert(llvm_value_t<T>::is_int, "llvm_not_t<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return ir->CreateNot(v1);
		}
	}
};

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_not_t<typename T1::type, T1> operator ~(T1 a1)
{
	return {a1};
}

template <typename T, typename A1, typename A2, llvm::CmpInst::Predicate UPred>
struct llvm_icmp_t
{
	using type = std::conditional_t<llvm_value_t<T>::is_vector != 0, bool[llvm_value_t<T>::is_vector], bool>;

	A1 a1;
	A2 a2;

	static_assert(llvm_value_t<T>::is_int, "llvm_eq_t<>: invalid type");

	// Convert unsigned comparison predicate to signed if necessary
	static constexpr llvm::CmpInst::Predicate pred = llvm_value_t<T>::is_uint ? UPred :
		UPred == llvm::ICmpInst::ICMP_UGT ? llvm::ICmpInst::ICMP_SGT :
		UPred == llvm::ICmpInst::ICMP_UGE ? llvm::ICmpInst::ICMP_SGE :
		UPred == llvm::ICmpInst::ICMP_ULT ? llvm::ICmpInst::ICMP_SLT :
		UPred == llvm::ICmpInst::ICMP_ULE ? llvm::ICmpInst::ICMP_SLE : UPred;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || UPred == llvm::ICmpInst::ICMP_EQ || UPred == llvm::ICmpInst::ICMP_NE, "llvm_eq_t<>: invalid type(II)");

	static inline llvm::Value* icmp(llvm::IRBuilder<>* ir, llvm::Value* lhs, llvm::Value* rhs)
	{
		return ir->CreateICmp(pred, lhs, rhs);
	}

	static inline llvm::Value* icmp(llvm::IRBuilder<>* ir, llvm::Value* lhs, u64 value)
	{
		return ir->CreateICmp(pred, lhs, llvm::ConstantInt::get(llvm_value_t<T>::get_type(ir->getContext()), value, llvm_value_t<T>::is_sint));
	}

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm_value_t<T>::is_int)
		{
			return icmp(ir, v1, v2);
		}
	}
};

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_icmp_t<typename T1::type, T1, T2, llvm::ICmpInst::ICMP_EQ> operator ==(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_icmp_t<typename T1::type, T1, llvm_int_t, llvm::ICmpInst::ICMP_EQ> operator ==(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_icmp_t<typename T1::type, T1, T2, llvm::ICmpInst::ICMP_NE> operator !=(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_icmp_t<typename T1::type, T1, llvm_int_t, llvm::ICmpInst::ICMP_NE> operator !=(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_icmp_t<typename T1::type, T1, T2, llvm::ICmpInst::ICMP_UGT> operator >(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_icmp_t<typename T1::type, T1, llvm_int_t, llvm::ICmpInst::ICMP_UGT> operator >(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_icmp_t<typename T1::type, T1, T2, llvm::ICmpInst::ICMP_UGE> operator >=(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_icmp_t<typename T1::type, T1, llvm_int_t, llvm::ICmpInst::ICMP_UGE> operator >=(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_icmp_t<typename T1::type, T1, T2, llvm::ICmpInst::ICMP_ULT> operator <(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_icmp_t<typename T1::type, T1, llvm_int_t, llvm::ICmpInst::ICMP_ULT> operator <(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

template <typename T1, typename T2, typename = decltype(std::declval<T1>().eval(0)), typename = decltype(std::declval<T2>().eval(0)), typename = std::enable_if_t<std::is_same<typename T1::type, typename T2::type>::value>>
inline llvm_icmp_t<typename T1::type, T1, T2, llvm::ICmpInst::ICMP_ULE> operator <=(T1 a1, T2 a2)
{
	return {a1, a2};
}

template <typename T1, typename = decltype(std::declval<T1>().eval(0)), typename = std::enable_if_t<llvm_value_t<typename T1::type>::is_int>>
inline llvm_icmp_t<typename T1::type, T1, llvm_int_t, llvm::ICmpInst::ICMP_ULE> operator <=(T1 a1, u64 a2)
{
	return {a1, llvm_int_t{a2}};
}

class cpu_translator
{
protected:
	cpu_translator(llvm::Module* module, bool is_be);

	// LLVM context
	std::reference_wrapper<llvm::LLVMContext> m_context;

	// Module to which all generated code is output to
	llvm::Module* m_module;

	// Endianness, affects vector element numbering (TODO)
	bool m_is_be;

	// IR builder
	llvm::IRBuilder<>* m_ir;

public:
	// Convert a C++ type to an LLVM type (TODO: remove)
	template <typename T>
	llvm::Type* GetType()
	{
		return llvm_value_t<T>::get_type(m_context);
	}

	template <typename T>
	llvm::Type* get_type()
	{
		return llvm_value_t<T>::get_type(m_context);
	}

	template <typename T>
	using value_t = llvm_value_t<T>;

	template <typename T>
	auto eval(T expr)
	{
		value_t<typename T::type> result;
		result.value = expr.eval(m_ir);
		return result;
	}

	template <typename T, typename T2>
	value_t<T> bitcast(T2 expr)
	{
		value_t<T> result;
		result.value = m_ir->CreateBitCast(expr.eval(m_ir), result.get_type(m_context));
		return result;
	}

	template <typename T, typename T2>
	value_t<T> trunc(T2 expr)
	{
		value_t<T> result;
		result.value = m_ir->CreateTrunc(expr.eval(m_ir), result.get_type(m_context));
		return result;
	}

	template <typename T, typename T2>
	value_t<T> sext(T2 expr)
	{
		value_t<T> result;
		result.value = m_ir->CreateSExt(expr.eval(m_ir), result.get_type(m_context));
		return result;
	}

	template <typename T, typename T2>
	value_t<T> zext(T2 expr)
	{
		value_t<T> result;
		result.value = m_ir->CreateZExt(expr.eval(m_ir), result.get_type(m_context));
		return result;
	}

	// Get unsigned addition carry into the sign bit (s = a + b)
	template <typename T>
	static inline auto ucarry(T a, T b, T s)
	{
		return ((a ^ b) & ~s) | (a & b);
	}

	// Get signed addition overflow into the sign bit (s = a + b)
	template <typename T>
	static inline auto scarry(T a, T b, T s)
	{
		return (b ^ s) & ~(a ^ b);
	}

	// Get signed subtraction overflow into the sign bit (d = a - b)
	template <typename T>
	static inline auto sborrow(T a, T b, T d)
	{
		return (a ^ b) & (a ^ d);
	}

	// Bitwise select (c ? a : b)
	template <typename T>
	static inline auto merge(T c, T a, T b)
	{
		return (a & c) | (b & ~c);
	}

	// Average: (a + b + 1) >> 1
	template <typename T>
	static inline auto avg(T a, T b)
	{
		return (a >> 1) + (b >> 1) + ((a | b) & 1);
	}

	// Rotate left
	template <typename T>
	static inline auto rol(T a, T b)
	{
		static constexpr u64 mask = value_t<typename T::type>::esize - 1;
		return a << (b & mask) | a >> (-b & mask);
	}

	// Rotate left
	template <typename T>
	static inline auto rol(T a, u64 b)
	{
		static constexpr u64 mask = value_t<typename T::type>::esize - 1;
		return a << (b & mask) | a >> ((0 - b) & mask);
	}

	// Select (c ? a : b)
	template <typename T, typename T2>
	auto select(T2 c, T a, T b)
	{
		static_assert(value_t<typename T2::type>::esize == 1, "select: expected bool type (first argument)");
		static_assert(value_t<typename T2::type>::is_vector == value_t<typename T::type>::is_vector, "select: incompatible arguments (vectors)");
		T result;
		result.value = m_ir->CreateSelect(c.eval(m_ir), a.eval(m_ir), b.eval(m_ir));
		return result;
	}

	template <typename T, typename E>
	auto insert(T v, u64 i, E e)
	{
		value_t<typename T::type> result;
		result.value = m_ir->CreateInsertElement(v.eval(m_ir), e.eval(m_ir), i);
		return result;
	}

	template <typename T>
	auto extract(T v, u64 i)
	{
		typename value_t<typename T::type>::base result;
		result.value = m_ir->CreateExtractElement(v.eval(m_ir), i);
		return result;
	}

	template <typename T>
	auto splat(u64 c)
	{
		value_t<T> result;
		result.value = llvm::ConstantInt::get(result.get_type(m_context), c, result.is_sint);
		return result;
	}

	template <typename T>
	auto fsplat(f64 c)
	{
		value_t<T> result;
		result.value = llvm::ConstantFP::get(result.get_type(m_context), c);
		return result;
	}

	// Min
	template <typename T>
	auto min(T a, T b)
	{
		T result;
		result.value = m_ir->CreateSelect((a > b).eval(m_ir), b.eval(m_ir), a.eval(m_ir));
		return result;
	}

	// Max
	template <typename T>
	auto max(T a, T b)
	{
		T result;
		result.value = m_ir->CreateSelect((a > b).eval(m_ir), a.eval(m_ir), b.eval(m_ir));
		return result;
	}

	// Shuffle single vector using all zeros second vector of the same size
	template <typename T, typename T1, typename... Args>
	auto zshuffle(T1 a, Args... args)
	{
		static_assert(sizeof(T) / sizeof(std::remove_extent_t<T>) == sizeof...(Args), "zshuffle: unexpected result type");
		const u32 values[]{static_cast<u32>(args)...};
		value_t<T> result;
		result.value = a.eval(m_ir);
		result.value = m_ir->CreateShuffleVector(result.value, llvm::ConstantInt::get(result.value->getType(), 0), values);
		return result;
	}

	template <typename T, typename T1, typename T2, typename... Args>
	auto shuffle2(T1 a, T2 b, Args... args)
	{
		static_assert(sizeof(T) / sizeof(std::remove_extent_t<T>) == sizeof...(Args), "shuffle2: unexpected result type");
		const u32 values[]{static_cast<u32>(args)...};
		value_t<T> result;
		result.value = a.eval(m_ir);
		result.value = m_ir->CreateShuffleVector(result.value, b.eval(m_ir), values);
		return result;
	}

	template <typename T, typename... Args>
	auto build(Args... args)
	{
		using value_type = std::remove_extent_t<T>;
		const value_type values[]{static_cast<value_type>(args)...};
		static_assert(sizeof(T) / sizeof(value_type) == sizeof...(Args), "build: unexpected number of arguments");
		value_t<T> result;
		result.value = llvm::ConstantDataVector::get(m_context, values);
		return result;
	}

	template <typename... Types>
	llvm::Function* get_intrinsic(llvm::Intrinsic::ID id)
	{
		const auto module = m_ir->GetInsertBlock()->getParent()->getParent();
		return llvm::Intrinsic::getDeclaration(module, id, {get_type<Types>()...});
	}

	template <typename T>
	auto ctlz(T a)
	{
		value_t<typename T::type> result;
		result.value = m_ir->CreateCall(get_intrinsic<typename T::type>(llvm::Intrinsic::ctlz), {a.eval(m_ir), m_ir->getFalse()});
		return result;
	}

	template <typename T>
	auto ctpop(T a)
	{
		value_t<typename T::type> result;
		result.value = m_ir->CreateCall(get_intrinsic<typename T::type>(llvm::Intrinsic::ctpop), {a.eval(m_ir)});
		return result;
	}

	template <typename T>
	auto sqrt(T a)
	{
		value_t<typename T::type> result;
		result.value = m_ir->CreateCall(get_intrinsic<typename T::type>(llvm::Intrinsic::sqrt), {a.eval(m_ir)});
		return result;
	}

	template <typename T>
	auto fabs(T a)
	{
		value_t<typename T::type> result;
		result.value = m_ir->CreateCall(get_intrinsic<typename T::type>(llvm::Intrinsic::fabs), {a.eval(m_ir)});
		return result;
	}

	template <llvm::CmpInst::Predicate FPred, typename T>
	auto fcmp(T a, T b)
	{
		value_t<std::conditional_t<llvm_value_t<typename T::type>::is_vector != 0, bool[llvm_value_t<typename T::type>::is_vector], bool>> result;
		result.value = m_ir->CreateFCmp(FPred, a.eval(m_ir), b.eval(m_ir));
		return result;
	}

	template <typename R = v128>
	R get_const_vector(llvm::Constant*, u32 a, u32 b);

	template <typename T = v128>
	llvm::Constant* make_const_vector(T, llvm::Type*);
};

#endif
