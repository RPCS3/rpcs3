#pragma once

#ifdef LLVM_AVAILABLE

#include "restore_new.h"
#ifdef _MSC_VER
#pragma warning(push, 0)
#endif
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Analysis/ConstantFolding.h"
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
	static constexpr bool is_int = true;

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
	static constexpr bool is_int = true;

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
	static constexpr bool is_int = true;

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

	static constexpr uint esize      = 64;
	static constexpr bool is_int     = false;
	static constexpr bool is_sint    = false;
	static constexpr bool is_uint    = false;
	static constexpr bool is_float   = false;
	static constexpr uint is_vector  = false;
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

template <typename T>
using llvm_expr_t = std::decay_t<T>;

template <typename T, typename = void>
struct is_llvm_expr
{
};

template <typename T>
struct is_llvm_expr<T, std::void_t<decltype(std::declval<T>().eval(std::declval<llvm::IRBuilder<>*>()))>>
{
	using type = typename std::decay_t<T>::type;
};

template <typename T, typename Of, typename = void>
struct is_llvm_expr_of
{
	static constexpr bool ok = false;
};

template <typename T, typename Of>
struct is_llvm_expr_of<T, Of, std::void_t<typename is_llvm_expr<T>::type, typename is_llvm_expr<Of>::type>>
{
	static constexpr bool ok = std::is_same_v<typename is_llvm_expr<T>::type, typename is_llvm_expr<Of>::type>;
};

template <typename T, typename... Types>
using llvm_common_t = std::enable_if_t<(is_llvm_expr_of<T, Types>::ok && ...), typename is_llvm_expr<T>::type>;

template <typename T, bool ForceSigned = false>
struct llvm_const_int
{
	using type = T;

	u64 val;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		static_assert(llvm_value_t<T>::is_int, "llvm_const_int<>: invalid type");

		return llvm::ConstantInt::get(llvm_value_t<T>::get_type(ir->getContext()), val, ForceSigned || llvm_value_t<T>::is_sint);
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_add
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_add<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateAdd(v1, v2);
		}

		if constexpr (llvm_value_t<T>::is_float)
		{
			return ir->CreateFAdd(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_add<T1, T2> operator +(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_add<T1, llvm_const_int<typename is_llvm_expr<T1>::type>> operator +(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename A1, typename A2, typename A3, typename T = llvm_common_t<A1, A2, A3>>
struct llvm_sum
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;
	llvm_expr_t<A3> a3;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_sum<>: invalid_type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		const auto v3 = a3.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateAdd(ir->CreateAdd(v1, v2), v3);
		}
	}
};

template <typename T1, typename T2, typename T3>
llvm_sum(T1&& a1, T2&& a2, T3&& a3) -> llvm_sum<T1, T2, T3>;

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_sub
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_sub<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateSub(v1, v2);
		}

		if constexpr (llvm_value_t<T>::is_float)
		{
			return ir->CreateFSub(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_sub<T1, T2> operator -(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_sub<T1, llvm_const_int<typename is_llvm_expr<T1>::type>> operator -(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename T1>
inline llvm_sub<llvm_const_int<typename is_llvm_expr<T1>::type>, T1> operator -(u64 c, T1&& a1)
{
	return {{c}, a1};
}

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_mul
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_mul<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateMul(v1, v2);
		}

		if constexpr (llvm_value_t<T>::is_float)
		{
			return ir->CreateFMul(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_mul<T1, T2> operator *(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_div
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_div<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_sint)
		{
			return ir->CreateSDiv(v1, v2);
		}

		if constexpr (llvm_value_t<T>::is_uint)
		{
			return ir->CreateUDiv(v1, v2);
		}

		if constexpr (llvm_value_t<T>::is_float)
		{
			return ir->CreateFDiv(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_div<T1, T2> operator /(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename A1, typename T = llvm_common_t<A1>>
struct llvm_neg
{
	using type = T;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_neg<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateNeg(v1);
		}

		if constexpr (llvm_value_t<T>::is_float)
		{
			return ir->CreateFNeg(v1);
		}
	}
};

template <typename T1>
inline llvm_neg<T1> operator -(T1 a1)
{
	return {a1};
}

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_shl
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_shl<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_sint)
		{
			return ir->CreateShl(v1, v2);
		}

		if constexpr (llvm_value_t<T>::is_uint)
		{
			return ir->CreateShl(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_shl<T1, T2> operator <<(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_shl<T1, llvm_const_int<typename is_llvm_expr<T1>::type>> operator <<(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_shr
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_shr<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_sint)
		{
			return ir->CreateAShr(v1, v2);
		}

		if constexpr (llvm_value_t<T>::is_uint)
		{
			return ir->CreateLShr(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_shr<T1, T2> operator >>(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_shr<T1, llvm_const_int<typename is_llvm_expr<T1>::type>> operator >>(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename A1, typename A2, typename A3, typename T = llvm_common_t<A1, A2, A3>>
struct llvm_fshl
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;
	llvm_expr_t<A3> a3;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_fshl<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	static llvm::Function* get_fshl(llvm::IRBuilder<>* ir)
	{
		const auto module = ir->GetInsertBlock()->getParent()->getParent();
		return llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::fshl, {llvm_value_t<T>::get_type(ir->getContext())});
	}

	static llvm::Value* fold(llvm::IRBuilder<>* ir, llvm::Value* v1, llvm::Value* v2, llvm::Value* v3)
	{
		// Compute constant result.
		const u64 size = v3->getType()->getScalarSizeInBits();
		const auto val = ir->CreateURem(v3, llvm::ConstantInt::get(v3->getType(), size));
		const auto shl = ir->CreateShl(v1, val);
		const auto shr = ir->CreateLShr(v2, ir->CreateSub(llvm::ConstantInt::get(v3->getType(), size - 1), val));
		return ir->CreateOr(shl, ir->CreateLShr(shr, 1));
	}

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		const auto v3 = a3.eval(ir);

		if (llvm::isa<llvm::Constant>(v1) && llvm::isa<llvm::Constant>(v2) && llvm::isa<llvm::Constant>(v3))
		{
			return fold(ir, v1, v2, v3);
		}

		return ir->CreateCall(get_fshl(ir), {v1, v2, v3});
	}
};

template <typename A1, typename A2, typename A3, typename T = llvm_common_t<A1, A2, A3>>
struct llvm_fshr
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;
	llvm_expr_t<A3> a3;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_fshr<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	static llvm::Function* get_fshr(llvm::IRBuilder<>* ir)
	{
		const auto module = ir->GetInsertBlock()->getParent()->getParent();
		return llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::fshr, {llvm_value_t<T>::get_type(ir->getContext())});
	}

	static llvm::Value* fold(llvm::IRBuilder<>* ir, llvm::Value* v1, llvm::Value* v2, llvm::Value* v3)
	{
		// Compute constant result.
		const u64 size = v3->getType()->getScalarSizeInBits();
		const auto val = ir->CreateURem(v3, llvm::ConstantInt::get(v3->getType(), size));
		const auto shr = ir->CreateLShr(v2, val);
		const auto shl = ir->CreateShl(v1, ir->CreateSub(llvm::ConstantInt::get(v3->getType(), size - 1), val));
		return ir->CreateOr(shr, ir->CreateShl(shl, 1));
	}

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		const auto v3 = a3.eval(ir);

		if (llvm::isa<llvm::Constant>(v1) && llvm::isa<llvm::Constant>(v2) && llvm::isa<llvm::Constant>(v3))
		{
			return fold(ir, v1, v2, v3);
		}

		return ir->CreateCall(get_fshr(ir), {v1, v2, v3});
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_rol
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_rol<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm::isa<llvm::Constant>(v1) && llvm::isa<llvm::Constant>(v2))
		{
			return llvm_fshl<A1, A1, A2>::fold(ir, v1, v1, v2);
		}

		return ir->CreateCall(llvm_fshl<A1, A1, A2>::get_fshl(ir), {v1, v1, v2});
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_and
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_int, "llvm_and<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateAnd(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_and<T1, T2> operator &(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_and<T1, llvm_const_int<typename is_llvm_expr<T1>::type>> operator &(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_or
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_int, "llvm_or<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateOr(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_or<T1, T2> operator |(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_or<T1, llvm_const_int<typename is_llvm_expr<T1>::type>> operator |(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_xor
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_int, "llvm_xor<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateXor(v1, v2);
		}
	}
};

template <typename T1, typename T2>
inline llvm_xor<T1, T2> operator ^(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_xor<T1, llvm_const_int<typename is_llvm_expr<T1>::type>> operator ^(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename T1>
inline llvm_xor<T1, llvm_const_int<typename is_llvm_expr<T1>::type, true>> operator ~(T1&& a1)
{
	return {a1, {UINT64_MAX}};
}

template <typename A1, typename A2, llvm::CmpInst::Predicate UPred, typename T = llvm_common_t<A1, A2>>
struct llvm_cmp
{
	using type = std::conditional_t<llvm_value_t<T>::is_vector != 0, bool[llvm_value_t<T>::is_vector], bool>;

	static constexpr bool is_float = llvm_value_t<T>::is_float;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_int || is_float, "llvm_cmp<>: invalid type");

	// Convert unsigned comparison predicate to signed if necessary
	static constexpr llvm::CmpInst::Predicate pred = llvm_value_t<T>::is_uint ? UPred :
		UPred == llvm::ICmpInst::ICMP_UGT ? llvm::ICmpInst::ICMP_SGT :
		UPred == llvm::ICmpInst::ICMP_UGE ? llvm::ICmpInst::ICMP_SGE :
		UPred == llvm::ICmpInst::ICMP_ULT ? llvm::ICmpInst::ICMP_SLT :
		UPred == llvm::ICmpInst::ICMP_ULE ? llvm::ICmpInst::ICMP_SLE : UPred;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || is_float || UPred == llvm::ICmpInst::ICMP_EQ || UPred == llvm::ICmpInst::ICMP_NE, "llvm_cmp<>: invalid operation on sign-undefined type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		static_assert(!is_float, "llvm_cmp<>: invalid operation (missing fcmp_ord or fcmp_uno)");

		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_int)
		{
			return ir->CreateICmp(pred, v1, v2);
		}
	}
};

template <typename T>
struct is_llvm_cmp : std::bool_constant<false>
{
};

template <typename A1, typename A2, auto UPred, typename T>
struct is_llvm_cmp<llvm_cmp<A1, A2, UPred, T>> : std::bool_constant<true>
{
};

template <typename Cmp, typename T = llvm_common_t<Cmp>>
struct llvm_ord
{
	using base = std::decay_t<Cmp>;
	using type = typename base::type;

	llvm_expr_t<Cmp> cmp;

	// Convert comparison predicate to ordered
	static constexpr llvm::CmpInst::Predicate pred =
		base::pred == llvm::ICmpInst::ICMP_EQ ? llvm::ICmpInst::FCMP_OEQ :
		base::pred == llvm::ICmpInst::ICMP_NE ? llvm::ICmpInst::FCMP_ONE :
		base::pred == llvm::ICmpInst::ICMP_SGT ? llvm::ICmpInst::FCMP_OGT :
		base::pred == llvm::ICmpInst::ICMP_SGE ? llvm::ICmpInst::FCMP_OGE :
		base::pred == llvm::ICmpInst::ICMP_SLT ? llvm::ICmpInst::FCMP_OLT :
		base::pred == llvm::ICmpInst::ICMP_SLE ? llvm::ICmpInst::FCMP_OLE : base::pred;

	static_assert(base::is_float, "llvm_ord<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = cmp.a1.eval(ir);
		const auto v2 = cmp.a2.eval(ir);
		return ir->CreateFCmp(pred, v1, v2);
	}
};

template <typename T>
llvm_ord(T&&) -> llvm_ord<std::enable_if_t<is_llvm_cmp<std::decay_t<T>>::value, T&&>>;

template <typename Cmp, typename T = llvm_common_t<Cmp>>
struct llvm_uno
{
	using base = std::decay_t<Cmp>;
	using type = typename base::type;

	llvm_expr_t<Cmp> cmp;

	// Convert comparison predicate to unordered
	static constexpr llvm::CmpInst::Predicate pred =
		base::pred == llvm::ICmpInst::ICMP_EQ ? llvm::ICmpInst::FCMP_UEQ :
		base::pred == llvm::ICmpInst::ICMP_NE ? llvm::ICmpInst::FCMP_UNE :
		base::pred == llvm::ICmpInst::ICMP_SGT ? llvm::ICmpInst::FCMP_UGT :
		base::pred == llvm::ICmpInst::ICMP_SGE ? llvm::ICmpInst::FCMP_UGE :
		base::pred == llvm::ICmpInst::ICMP_SLT ? llvm::ICmpInst::FCMP_ULT :
		base::pred == llvm::ICmpInst::ICMP_SLE ? llvm::ICmpInst::FCMP_ULE : base::pred;

	static_assert(base::is_float, "llvm_uno<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = cmp.a1.eval(ir);
		const auto v2 = cmp.a2.eval(ir);
		return ir->CreateFCmp(pred, v1, v2);
	}
};

template <typename T>
llvm_uno(T&&) -> llvm_uno<std::enable_if_t<is_llvm_cmp<std::decay_t<T>>::value, T&&>>;

template <typename T1, typename T2>
inline llvm_cmp<T1, T2, llvm::ICmpInst::ICMP_EQ> operator ==(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_cmp<T1, llvm_const_int<typename is_llvm_expr<T1>::type>, llvm::ICmpInst::ICMP_EQ> operator ==(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename T1, typename T2>
inline llvm_cmp<T1, T2, llvm::ICmpInst::ICMP_NE> operator !=(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_cmp<T1, llvm_const_int<typename is_llvm_expr<T1>::type>, llvm::ICmpInst::ICMP_NE> operator !=(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename T1, typename T2>
inline llvm_cmp<T1, T2, llvm::ICmpInst::ICMP_UGT> operator >(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_cmp<T1, llvm_const_int<typename is_llvm_expr<T1>::type>, llvm::ICmpInst::ICMP_UGT> operator >(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename T1, typename T2>
inline llvm_cmp<T1, T2, llvm::ICmpInst::ICMP_UGE> operator >=(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_cmp<T1, llvm_const_int<typename is_llvm_expr<T1>::type>, llvm::ICmpInst::ICMP_UGE> operator >=(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename T1, typename T2>
inline llvm_cmp<T1, T2, llvm::ICmpInst::ICMP_ULT> operator <(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_cmp<T1, llvm_const_int<typename is_llvm_expr<T1>::type>, llvm::ICmpInst::ICMP_ULT> operator <(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename T1, typename T2>
inline llvm_cmp<T1, T2, llvm::ICmpInst::ICMP_ULE> operator <=(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

template <typename T1>
inline llvm_cmp<T1, llvm_const_int<typename is_llvm_expr<T1>::type>, llvm::ICmpInst::ICMP_ULE> operator <=(T1&& a1, u64 c)
{
	return {a1, {c}};
}

template <typename U, typename A1, typename T = llvm_common_t<A1>>
struct llvm_noncast
{
	using type = U;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_int, "llvm_noncast<>: invalid type");
	static_assert(llvm_value_t<U>::is_int, "llvm_noncast<>: invalid result type");
	static_assert(llvm_value_t<T>::esize == llvm_value_t<U>::esize, "llvm_noncast<>: result is resized");
	static_assert(llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector, "llvm_noncast<>: vector element mismatch");

	static constexpr bool is_ok =
		llvm_value_t<T>::is_int &&
		llvm_value_t<U>::is_int &&
		llvm_value_t<T>::esize == llvm_value_t<U>::esize &&
		llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		// No operation required
		return a1.eval(ir);
	}
};

template <typename U, typename A1, typename T = llvm_common_t<A1>>
struct llvm_bitcast
{
	using type = U;

	llvm_expr_t<A1> a1;

	static constexpr uint bitsize0 = llvm_value_t<T>::is_vector ? llvm_value_t<T>::is_vector * llvm_value_t<T>::esize : llvm_value_t<T>::esize;
	static constexpr uint bitsize1 = llvm_value_t<U>::is_vector ? llvm_value_t<U>::is_vector * llvm_value_t<U>::esize : llvm_value_t<U>::esize;

	static_assert(bitsize0 == bitsize1, "llvm_bitcast<>: invalid type (size mismatch)");
	static_assert(llvm_value_t<T>::is_int || llvm_value_t<T>::is_float, "llvm_bitcast<>: invalid type");
	static_assert(llvm_value_t<U>::is_int || llvm_value_t<U>::is_float, "llvm_bitcast<>: invalid result type");

	static constexpr bool is_ok =
		bitsize0 && bitsize0 == bitsize1 &&
		(llvm_value_t<T>::is_int || llvm_value_t<T>::is_float) &&
		(llvm_value_t<U>::is_int || llvm_value_t<U>::is_float);

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto rt = llvm_value_t<U>::get_type(ir->getContext());

		if constexpr (llvm_value_t<T>::is_int == llvm_value_t<U>::is_int && llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector)
		{
			// No-op case
			return v1;
		}

		if (const auto c1 = llvm::dyn_cast<llvm::Constant>(v1))
		{
			const auto module = ir->GetInsertBlock()->getParent()->getParent();
			const auto result = llvm::ConstantFoldCastOperand(llvm::Instruction::BitCast, c1, rt, module->getDataLayout());

			if (result)
			{
				return result;
			}
		}

		return ir->CreateBitCast(v1, rt);
	}
};

template <typename U, typename A1, typename T = llvm_common_t<A1>>
struct llvm_trunc
{
	using type = U;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_int, "llvm_trunc<>: invalid type");
	static_assert(llvm_value_t<U>::is_int, "llvm_trunc<>: invalid result type");
	static_assert(llvm_value_t<T>::esize > llvm_value_t<U>::esize, "llvm_trunc<>: result is not truncated");
	static_assert(llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector, "llvm_trunc<>: vector element mismatch");

	static constexpr bool is_ok =
		llvm_value_t<T>::is_int &&
		llvm_value_t<U>::is_int &&
		llvm_value_t<T>::esize > llvm_value_t<U>::esize &&
		llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return ir->CreateTrunc(a1.eval(ir), llvm_value_t<U>::get_type(ir->getContext()));
	}
};

template <typename U, typename A1, typename T = llvm_common_t<A1>>
struct llvm_sext
{
	using type = U;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_int, "llvm_sext<>: invalid type");
	static_assert(llvm_value_t<U>::is_sint, "llvm_sext<>: invalid result type");
	static_assert(llvm_value_t<T>::esize < llvm_value_t<U>::esize, "llvm_sext<>: result is not extended");
	static_assert(llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector, "llvm_sext<>: vector element mismatch");

	static constexpr bool is_ok =
		llvm_value_t<T>::is_int &&
		llvm_value_t<U>::is_sint &&
		llvm_value_t<T>::esize < llvm_value_t<U>::esize &&
		llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return ir->CreateSExt(a1.eval(ir), llvm_value_t<U>::get_type(ir->getContext()));
	}
};

template <typename U, typename A1, typename T = llvm_common_t<A1>>
struct llvm_zext
{
	using type = U;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_int, "llvm_zext<>: invalid type");
	static_assert(llvm_value_t<U>::is_uint, "llvm_zext<>: invalid result type");
	static_assert(llvm_value_t<T>::esize < llvm_value_t<U>::esize, "llvm_zext<>: result is not extended");
	static_assert(llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector, "llvm_zext<>: vector element mismatch");

	static constexpr bool is_ok =
		llvm_value_t<T>::is_int &&
		llvm_value_t<U>::is_uint &&
		llvm_value_t<T>::esize < llvm_value_t<U>::esize &&
		llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return ir->CreateZExt(a1.eval(ir), llvm_value_t<U>::get_type(ir->getContext()));
	}
};

template <typename A1, typename A2, typename A3, typename T = llvm_common_t<A2, A3>, typename U = llvm_common_t<A1>>
struct llvm_select
{
	using type = T;

	llvm_expr_t<A1> cond;
	llvm_expr_t<A2> a2;
	llvm_expr_t<A3> a3;

	static_assert(llvm_value_t<U>::esize == 1 && llvm_value_t<U>::is_int, "llvm_select<>: invalid condition type (bool expected)");
	static_assert(llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector, "llvm_select<>: vector element mismatch");

	static constexpr bool is_ok =
		llvm_value_t<U>::esize == 1 && llvm_value_t<U>::is_int &&
		llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return ir->CreateSelect(cond.eval(ir), a2.eval(ir), a3.eval(ir));
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_min
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_min<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_sint)
		{
			return ir->CreateSelect(ir->CreateICmpSLT(v1, v2), v1, v2);
		}

		if constexpr (llvm_value_t<T>::is_uint)
		{
			return ir->CreateSelect(ir->CreateICmpULT(v1, v2), v1, v2);
		}
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_max
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_max<>: invalid type");

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if constexpr (llvm_value_t<T>::is_sint)
		{
			return ir->CreateSelect(ir->CreateICmpSLT(v1, v2), v2, v1);
		}

		if constexpr (llvm_value_t<T>::is_uint)
		{
			return ir->CreateSelect(ir->CreateICmpULT(v1, v2), v2, v1);
		}
	}
};

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

	// Allow PSHUFB intrinsic
	bool m_use_ssse3;

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

	template <typename R, typename... Args>
	llvm::FunctionType* get_ftype()
	{
		return llvm::FunctionType::get(get_type<R>(), {get_type<Args>()...}, false);
	}

	template <typename T>
	using value_t = llvm_value_t<T>;

	template <typename T>
	value_t<T> value(llvm::Value* value)
	{
		if (!value || value->getType() != get_type<T>())
		{
			fmt::throw_exception("cpu_translator::value<>(): invalid value type");
		}

		value_t<T> result;
		result.value = value;
		return result;
	}

	template <typename T>
	auto eval(T&& expr)
	{
		value_t<typename std::decay_t<T>::type> result;
		result.value = expr.eval(m_ir);
		return result;
	}

	template <typename T, typename = std::enable_if_t<is_llvm_cmp<std::decay_t<T>>::value>>
	static auto fcmp_ord(T&& cmp_expr)
	{
		return llvm_ord{std::forward<T>(cmp_expr)};
	}

	template <typename T, typename = std::enable_if_t<is_llvm_cmp<std::decay_t<T>>::value>>
	static auto fcmp_uno(T&& cmp_expr)
	{
		return llvm_uno{std::forward<T>(cmp_expr)};
	}

	template <typename U, typename T, typename = std::enable_if_t<llvm_noncast<U, T>::is_ok>>
	static auto noncast(T&& expr)
	{
		return llvm_noncast<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T, typename = std::enable_if_t<llvm_bitcast<U, T>::is_ok>>
	static auto bitcast(T&& expr)
	{
		return llvm_bitcast<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T, typename = std::enable_if_t<llvm_trunc<U, T>::is_ok>>
	static auto trunc(T&& expr)
	{
		return llvm_trunc<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T, typename = std::enable_if_t<llvm_sext<U, T>::is_ok>>
	static auto sext(T&& expr)
	{
		return llvm_sext<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T, typename = std::enable_if_t<llvm_zext<U, T>::is_ok>>
	static auto zext(T&& expr)
	{
		return llvm_zext<U, T>{std::forward<T>(expr)};
	}

	template <typename T, typename U, typename V, typename = std::enable_if_t<llvm_select<T, U, V>::is_ok>>
	static auto select(T&& c, U&& a, V&& b)
	{
		return llvm_select<T, U, V>{std::forward<T>(c), std::forward<U>(a), std::forward<V>(b)};
	}

	template <typename T, typename U, typename = std::enable_if_t<llvm_min<T, U>::is_ok>>
	static auto min(T&& a, U&& b)
	{
		return llvm_min<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	template <typename T, typename U, typename = std::enable_if_t<llvm_min<T, U>::is_ok>>
	static auto max(T&& a, U&& b)
	{
		return llvm_max<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	template <typename T, typename U, typename V, typename = std::enable_if_t<llvm_fshl<T, U, V>::is_ok>>
	static auto fshl(T&& a, U&& b, V&& c)
	{
		return llvm_fshl<T, U, V>{std::forward<T>(a), std::forward<U>(b), std::forward<V>(c)};
	}

	template <typename T, typename U, typename V, typename = std::enable_if_t<llvm_fshr<T, U, V>::is_ok>>
	static auto fshr(T&& a, U&& b, V&& c)
	{
		return llvm_fshr<T, U, V>{std::forward<T>(a), std::forward<U>(b), std::forward<V>(c)};
	}

	template <typename T, typename U, typename = std::enable_if_t<llvm_rol<T, U>::is_ok>>
	static auto rol(T&& a, U&& b)
	{
		return llvm_rol<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	// Add with saturation
	template <typename T>
	inline auto add_sat(T a, T b)
	{
		value_t<typename T::type> result;
		const auto eva = a.eval(m_ir);
		const auto evb = b.eval(m_ir);

		// Compute constant result immediately if possible
		if (llvm::isa<llvm::Constant>(eva) && llvm::isa<llvm::Constant>(evb))
		{
			static_assert(result.is_sint || result.is_uint);

			if constexpr (result.is_sint)
			{
				llvm::Type* cast_to = m_ir->getIntNTy(result.esize * 2);
				if constexpr (result.is_vector != 0)
					cast_to = llvm::VectorType::get(cast_to, result.is_vector);

				const auto axt = m_ir->CreateSExt(eva, cast_to);
				const auto bxt = m_ir->CreateSExt(evb, cast_to);
				result.value = m_ir->CreateAdd(axt, bxt);
				const auto _max = m_ir->getInt(llvm::APInt::getSignedMaxValue(result.esize * 2).ashr(result.esize));
				const auto _min = m_ir->getInt(llvm::APInt::getSignedMinValue(result.esize * 2).ashr(result.esize));
				const auto smax = result.is_vector != 0 ? llvm::ConstantVector::getSplat(result.is_vector, _max) : _max;
				const auto smin = result.is_vector != 0 ? llvm::ConstantVector::getSplat(result.is_vector, _min) : _min;
				result.value = m_ir->CreateSelect(m_ir->CreateICmpSGT(result.value, smax), smax, result.value);
				result.value = m_ir->CreateSelect(m_ir->CreateICmpSLT(result.value, smin), smin, result.value);
				result.value = m_ir->CreateTrunc(result.value, result.get_type(m_context));
			}
			else
			{
				const auto _max = m_ir->getInt(llvm::APInt::getMaxValue(result.esize));
				const auto ones = result.is_vector != 0 ? llvm::ConstantVector::getSplat(result.is_vector, _max) : _max;
				result.value = m_ir->CreateAdd(eva, evb);
				result.value = m_ir->CreateSelect(m_ir->CreateICmpULT(result.value, eva), ones, result.value);
			}
		}
		else
		{
			result.value = m_ir->CreateCall(get_intrinsic<typename T::type>(result.is_sint ? llvm::Intrinsic::sadd_sat : llvm::Intrinsic::uadd_sat), {eva, evb});
		}

		return result;
	}

	// Subtract with saturation
	template <typename T>
	inline auto sub_sat(T a, T b)
	{
		value_t<typename T::type> result;
		const auto eva = a.eval(m_ir);
		const auto evb = b.eval(m_ir);

		// Compute constant result immediately if possible
		if (llvm::isa<llvm::Constant>(eva) && llvm::isa<llvm::Constant>(evb))
		{
			static_assert(result.is_sint || result.is_uint);

			if constexpr (result.is_sint)
			{
				llvm::Type* cast_to = m_ir->getIntNTy(result.esize * 2);
				if constexpr (result.is_vector != 0)
					cast_to = llvm::VectorType::get(cast_to, result.is_vector);

				const auto axt = m_ir->CreateSExt(eva, cast_to);
				const auto bxt = m_ir->CreateSExt(evb, cast_to);
				result.value = m_ir->CreateSub(axt, bxt);
				const auto _max = m_ir->getInt(llvm::APInt::getSignedMaxValue(result.esize * 2).ashr(result.esize));
				const auto _min = m_ir->getInt(llvm::APInt::getSignedMinValue(result.esize * 2).ashr(result.esize));
				const auto smax = result.is_vector != 0 ? llvm::ConstantVector::getSplat(result.is_vector, _max) : _max;
				const auto smin = result.is_vector != 0 ? llvm::ConstantVector::getSplat(result.is_vector, _min) : _min;
				result.value = m_ir->CreateSelect(m_ir->CreateICmpSGT(result.value, smax), smax, result.value);
				result.value = m_ir->CreateSelect(m_ir->CreateICmpSLT(result.value, smin), smin, result.value);
				result.value = m_ir->CreateTrunc(result.value, result.get_type(m_context));
			}
			else
			{
				const auto _min = m_ir->getInt(llvm::APInt::getMinValue(result.esize));
				const auto zero = result.is_vector != 0 ? llvm::ConstantVector::getSplat(result.is_vector, _min) : _min;
				result.value = m_ir->CreateSub(eva, evb);
				result.value = m_ir->CreateSelect(m_ir->CreateICmpULT(eva, evb), zero, result.value);
			}
		}
		else
		{
			result.value = m_ir->CreateCall(get_intrinsic<typename T::type>(result.is_sint ? llvm::Intrinsic::ssub_sat : llvm::Intrinsic::usub_sat), {eva, evb});
		}

		return result;
	}

	// Average: (a + b + 1) >> 1
	template <typename T>
	inline auto avg(T a, T b)
	{
		//return (a >> 1) + (b >> 1) + ((a | b) & 1);

		value_t<typename T::type> result;
		static_assert(result.is_sint || result.is_uint);
		const auto cast_op = result.is_sint ? llvm::Instruction::SExt : llvm::Instruction::ZExt;
		llvm::Type* cast_to = m_ir->getIntNTy(result.esize * 2);
		if constexpr (result.is_vector != 0)
			cast_to = llvm::VectorType::get(cast_to, result.is_vector);

		const auto axt = m_ir->CreateCast(cast_op, a.eval(m_ir), cast_to);
		const auto bxt = m_ir->CreateCast(cast_op, b.eval(m_ir), cast_to);
		const auto cxt = llvm::ConstantInt::get(cast_to, 1, false);
		const auto abc = m_ir->CreateAdd(m_ir->CreateAdd(axt, bxt), cxt);
		result.value = m_ir->CreateTrunc(m_ir->CreateLShr(abc, 1), result.get_type(m_context));
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

	template <typename T, typename V>
	auto vsplat(V v)
	{
		value_t<T> result;
		static_assert(result.is_vector);
		result.value = m_ir->CreateVectorSplat(result.is_vector, v.eval(m_ir));
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

	// Opportunistic hardware FMA, can be used if results are identical for all possible input values
	template <typename T>
	auto fmuladd(T a, T b, T c)
	{
		value_t<typename T::type> result;
		const auto av = a.eval(m_ir);
		const auto bv = b.eval(m_ir);
		const auto cv = c.eval(m_ir);
		result.value = m_ir->CreateCall(get_intrinsic<typename T::type>(llvm::Intrinsic::fmuladd), {av, bv, cv});
		return result;
	}

	template <typename T1, typename T2>
	value_t<u8[16]> pshufb(T1 a, T2 b)
	{
		value_t<u8[16]> result;

		const auto data0 = a.eval(m_ir);
		const auto index = b.eval(m_ir);
		const auto zeros = llvm::ConstantAggregateZero::get(get_type<u8[16]>());

		if (auto c = llvm::dyn_cast<llvm::Constant>(index))
		{
			// Convert PSHUFB index back to LLVM vector shuffle mask
			v128 mask{};

			const auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(c);

			if (cv)
			{
				for (u32 i = 0; i < 16; i++)
				{
					const u64 b = cv->getElementAsInteger(i);
					mask._u8[i] = b < 128 ? b % 16 : 16;
				}
			}

			if (cv || llvm::isa<llvm::ConstantAggregateZero>(c))
			{
				result.value = llvm::ConstantDataVector::get(m_context, llvm::makeArrayRef((const u8*)mask._bytes, 16));
				result.value = m_ir->CreateZExt(result.value, get_type<u32[16]>());
				result.value = m_ir->CreateShuffleVector(data0, zeros, result.value);
				return result;
			}
		}

		if (m_use_ssse3)
		{
			result.value = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_ssse3_pshuf_b_128), {data0, index});
		}
		else
		{
			// Emulate PSHUFB (TODO)
			const auto mask = m_ir->CreateAnd(index, 0xf);
			const auto loop = llvm::BasicBlock::Create(m_context, "", m_ir->GetInsertBlock()->getParent());
			const auto next = llvm::BasicBlock::Create(m_context, "", m_ir->GetInsertBlock()->getParent());
			const auto prev = m_ir->GetInsertBlock();

			m_ir->CreateBr(loop);
			m_ir->SetInsertPoint(loop);
			const auto i = m_ir->CreatePHI(get_type<u32>(), 2);
			const auto v = m_ir->CreatePHI(get_type<u8[16]>(), 2);
			i->addIncoming(m_ir->getInt32(0), prev);
			i->addIncoming(m_ir->CreateAdd(i, m_ir->getInt32(1)), loop);
			v->addIncoming(zeros, prev);
			result.value = m_ir->CreateInsertElement(v, m_ir->CreateExtractElement(data0, m_ir->CreateExtractElement(mask, i)), i);
			v->addIncoming(result.value, loop);
			m_ir->CreateCondBr(m_ir->CreateICmpULT(i, m_ir->getInt32(16)), loop, next);
			m_ir->SetInsertPoint(next);
			result.value = m_ir->CreateSelect(m_ir->CreateICmpSLT(index, zeros), zeros, result.value);
		}

		return result;
	}

	llvm::Value* load_const(llvm::GlobalVariable* g, llvm::Value* i)
	{
		return m_ir->CreateLoad(m_ir->CreateGEP(g, {m_ir->getInt64(0), m_ir->CreateZExtOrTrunc(i, get_type<u64>())}));
	}

	template <typename T, typename I>
	value_t<T> load_const(llvm::GlobalVariable* g, I i)
	{
		value_t<T> result;
		result.value = load_const(g, i.eval(m_ir));
		return result;
	}

	template <typename R = v128>
	R get_const_vector(llvm::Constant*, u32 a, u32 b);

	template <typename T = v128>
	llvm::Constant* make_const_vector(T, llvm::Type*);
};

#endif
