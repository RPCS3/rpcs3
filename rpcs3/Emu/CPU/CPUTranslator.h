#pragma once

#ifdef LLVM_AVAILABLE

#include "util/types.hpp"
#include "util/sysinfo.hpp"
#include "Utilities/StrFmt.h"
#include "Utilities/JIT.h"
#include "util/v128.hpp"

#ifdef _MSC_VER
#pragma warning(push, 0)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wredundant-decls"
#endif
#include "llvm/IR/LLVMContext.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/IntrinsicsX86.h"
#ifdef ARCH_ARM64
#include "llvm/IR/IntrinsicsAArch64.h"
#endif
#include "llvm/IR/InlineAsm.h"

#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

#include <functional>
#include <unordered_map>

// Helper function
llvm::Value* peek_through_bitcasts(llvm::Value*);

enum class i2 : char
{
};

enum class i4 : char
{
};

template <typename T>
concept LLVMType = (std::is_pointer_v<T>) && (std::is_base_of_v<llvm::Type, std::remove_pointer_t<T>>);

template <typename T>
concept LLVMValue = (std::is_pointer_v<T>) && (std::is_base_of_v<llvm::Value, std::remove_pointer_t<T>>);

template <typename T>
concept DSLValue = requires(T& v, llvm::IRBuilder<>* ir) {
	{ v.eval(ir) } -> LLVMValue;
};

template <usz N>
struct get_int_bits
{
};

template <>
struct get_int_bits<1>
{
	using utype = bool;
};

template <>
struct get_int_bits<2>
{
	using utype = i2;
};

template <>
struct get_int_bits<4>
{
	using utype = i4;
};

template <>
struct get_int_bits<8>
{
	using utype = u8;
};

template <>
struct get_int_bits<16>
{
	using utype = u16;
};

template <>
struct get_int_bits<32>
{
	using utype = u32;
};

template <>
struct get_int_bits<64>
{
	using utype = u64;
};

template <>
struct get_int_bits<128>
{
	using utype = u128;
};

template <usz Bits>
using get_int_vt = typename get_int_bits<Bits>::utype;

template <typename T = void>
struct llvm_value_t
{
	static_assert(std::is_same_v<T, void>, "llvm_value_t<> error: unknown type");

	using type = void;
	using base = llvm_value_t;
	static constexpr uint esize      = 0;
	static constexpr bool is_int     = false;
	static constexpr bool is_sint    = false;
	static constexpr bool is_uint    = false;
	static constexpr bool is_float   = false;
	static constexpr uint is_array   = false;
	static constexpr uint is_vector  = false;
	static constexpr uint is_pointer = false;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getVoidTy(context);
	}

	llvm::Value* eval(llvm::IRBuilder<>*) const
	{
		return value;
	}

	std::tuple<> match(llvm::Value*& value, llvm::Module*) const
	{
		if (peek_through_bitcasts(value) != peek_through_bitcasts(this->value))
		{
			value = nullptr;
		}

		return {};
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
struct llvm_value_t<int> : llvm_value_t<s8>
{
	using type = int;
	using base = llvm_value_t<s8>;
	using base::base;

	static constexpr uint esize = 32;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getInt32Ty(context);
	}
};

template <>
struct llvm_value_t<uint> : llvm_value_t<int>
{
	using type = uint;
	using base = llvm_value_t<int>;
	using base::base;

	static constexpr bool is_sint = false;
	static constexpr bool is_uint = true;
};

template <>
struct llvm_value_t<long> : llvm_value_t<s8>
{
	using type = long;
	using base = llvm_value_t<s8>;
	using base::base;

	static constexpr uint esize = 8 * sizeof(long);

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getInt64Ty(context);
	}
};

template <>
struct llvm_value_t<ulong> : llvm_value_t<long>
{
	using type = ulong;
	using base = llvm_value_t<long>;
	using base::base;

	static constexpr bool is_sint = false;
	static constexpr bool is_uint = true;
};

template <>
struct llvm_value_t<llong> : llvm_value_t<s8>
{
	using type = llong;
	using base = llvm_value_t<s8>;
	using base::base;

	static constexpr uint esize = 64;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::Type::getInt64Ty(context);
	}
};

template <>
struct llvm_value_t<ullong> : llvm_value_t<llong>
{
	using type = ullong;
	using base = llvm_value_t<llong>;
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
	static_assert(!std::is_void_v<T>, "llvm_value_t<> error: invalid pointer to void type");

	using type = T*;
	using base = llvm_value_t<T>;
	using base::base;

	static constexpr uint esize      = 64;
	static constexpr bool is_int     = false;
	static constexpr bool is_sint    = false;
	static constexpr bool is_uint    = false;
	static constexpr bool is_float   = false;
	static constexpr uint is_array   = false;
	static constexpr uint is_vector  = false;
	static constexpr uint is_pointer = llvm_value_t<T>::is_pointer + 1;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		return llvm::PointerType::getUnqual(context);
	}
};

// u32[4] : vector of 4 u32 elements
// u32[123][4] : array of 123 u32[4] vectors
// u32[123][1] : array of 123 u32 scalars
template <typename T, uint N>
struct llvm_value_t<T[N]> : llvm_value_t<std::conditional_t<(std::extent_v<T> > 1), T, std::remove_extent_t<T>>>
{
	using type = T[N];
	using base = llvm_value_t<std::conditional_t<(std::extent_v<T> > 1), T, std::remove_extent_t<T>>>;
	using base::base;

	static constexpr uint esize      = std::is_array_v<T> ? 0 : base::esize;
	static constexpr bool is_int     = !std::is_array_v<T> && base::is_int;
	static constexpr bool is_sint    = !std::is_array_v<T> && base::is_sint;
	static constexpr bool is_uint    = !std::is_array_v<T> && base::is_uint;
	static constexpr bool is_float   = !std::is_array_v<T> && base::is_float;
	static constexpr uint is_array   = std::is_array_v<T> ? N : 0;
	static constexpr uint is_vector  = std::is_array_v<T> ? 0 : N;
	static constexpr uint is_pointer = 0;

	static llvm::Type* get_type(llvm::LLVMContext& context)
	{
		if constexpr (std::is_array_v<T>)
		{
			return llvm::ArrayType::get(base::get_type(context), N);
		}
		else if constexpr (N > 1)
		{
			return llvm::VectorType::get(base::get_type(context), N, false);
		}
		else
		{
			return base::get_type(context);
		}
	}
};

template <typename T>
using llvm_expr_t = std::decay_t<T>;

template <typename T>
struct is_llvm_expr
{
};

template <DSLValue T>
struct is_llvm_expr<T>
{
	using type = typename std::decay_t<T>::type;
};

template <typename T, typename Of>
struct is_llvm_expr_of
{
	static constexpr bool ok = false;
};

template <typename T, typename Of>
	requires(requires { typename is_llvm_expr<T>::type; } && requires { typename is_llvm_expr<Of>::type; })
struct is_llvm_expr_of<T, Of>
{
	static constexpr bool ok = std::is_same_v<typename is_llvm_expr<T>::type, typename is_llvm_expr<Of>::type>;
};

template <typename T, typename... Types>
	requires(is_llvm_expr_of<T, Types>::ok && ...)
using llvm_common_t = typename is_llvm_expr<T>::type;

template <typename... Args>
using llvm_match_tuple = decltype(std::tuple_cat(std::declval<llvm_expr_t<Args>&>().match(std::declval<llvm::Value*&>(), nullptr)...));

template <typename T, typename U = llvm_common_t<llvm_value_t<T>>>
struct llvm_match_t
{
	using type = T;

	llvm::Value* value = nullptr;

	explicit operator bool() const
	{
		return value != nullptr;
	}

	template <typename... Args>
	bool eq(const Args&... args) const
	{
		llvm::Value* lhs = nullptr;
		return value && (lhs = peek_through_bitcasts(value)) && ((lhs == peek_through_bitcasts(args.value)) && ...);
	}

	llvm::Value* eval(llvm::IRBuilder<>*) const
	{
		return value;
	}

	std::tuple<> match(llvm::Value*& value, llvm::Module*) const
	{
		if (peek_through_bitcasts(value) != peek_through_bitcasts(this->value))
		{
			value = nullptr;
		}

		return {};
	}
};

template <typename T, typename U = llvm_common_t<llvm_value_t<T>>>
struct llvm_placeholder_t
{
	// TODO: placeholder extracting actual constant values (u64, f64, vector, etc)

	using type = T;

	llvm::Value* eval(llvm::IRBuilder<>*) const
	{
		return nullptr;
	}

	std::tuple<llvm_match_t<T>> match(llvm::Value*& value, llvm::Module*) const
	{
		if (value && value->getType() == llvm_value_t<T>::get_type(value->getContext()))
		{
			return {{value}};
		}

		value = nullptr;
		return {};
	}
};

template <typename T, bool ForceSigned = false>
struct llvm_const_int
{
	using type = T;

	u64 val;

	static constexpr bool is_ok = llvm_value_t<T>::is_int;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		static_assert(llvm_value_t<T>::is_int, "llvm_const_int<>: invalid type");

		return llvm::ConstantInt::get(llvm_value_t<T>::get_type(ir->getContext()), val, ForceSigned || llvm_value_t<T>::is_sint);
	}

	std::tuple<> match(llvm::Value*& value, llvm::Module*) const
	{
		if (value && value == llvm::ConstantInt::get(llvm_value_t<T>::get_type(value->getContext()), val, ForceSigned || llvm_value_t<T>::is_sint))
		{
			return {};
		}

		value = nullptr;
		return {};
	}
};

template <typename T>
struct llvm_const_float
{
	using type = T;

	f64 val;

	static constexpr bool is_ok = llvm_value_t<T>::is_float;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		static_assert(llvm_value_t<T>::is_float, "llvm_const_float<>: invalid type");

		return llvm::ConstantFP::get(llvm_value_t<T>::get_type(ir->getContext()), val);
	}

	std::tuple<> match(llvm::Value*& value, llvm::Module*) const
	{
		if (value && value == llvm::ConstantFP::get(llvm_value_t<T>::get_type(value->getContext()), val))
		{
			return {};
		}

		value = nullptr;
		return {};
	}
};

template <uint N, typename T>
struct llvm_const_vector
{
	using type = T;

	T data;

	static constexpr bool is_ok = N && llvm_value_t<T>::is_vector == N;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		static_assert(N && llvm_value_t<T>::is_vector == N, "llvm_const_vector<>: invalid type");

		return llvm::ConstantDataVector::get(ir->getContext(), data);
	}

	std::tuple<> match(llvm::Value*& value, llvm::Module*) const
	{
		if (value && value == llvm::ConstantDataVector::get(value->getContext(), data))
		{
			return {};
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_add
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_add<>: invalid type");

	static constexpr auto opc = llvm_value_t<T>::is_float ? llvm::Instruction::FAdd : llvm::Instruction::Add;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		return ir->CreateBinOp(opc, v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == opc)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}

			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			// Argument order does not matter here, try when swapped
			if (auto r1 = a1.match(v2, _m); v2)
			{
				if (auto r2 = a2.match(v1, _m); v1)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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
		return ir->CreateAdd(ir->CreateAdd(v1, v2), v3);
	}

	llvm_match_tuple<A1, A2, A3> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};
		llvm::Value* v3 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == llvm::Instruction::Add)
		{
			v3 = i->getOperand(1);

			if (auto r3 = a3.match(v3, _m); v3)
			{
				i = llvm::dyn_cast<llvm::BinaryOperator>(i->getOperand(0));

				if (i && i->getOpcode() == llvm::Instruction::Add)
				{
					v1 = i->getOperand(0);
					v2 = i->getOperand(1);

					if (auto r1 = a1.match(v1, _m); v1)
					{
						if (auto r2 = a2.match(v2, _m); v2)
						{
							return std::tuple_cat(r1, r2, r3);
						}
					}
				}
			}
		}

		value = nullptr;
		return {};
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

	static constexpr auto opc = llvm_value_t<T>::is_float ? llvm::Instruction::FSub : llvm::Instruction::Sub;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		return ir->CreateBinOp(opc, v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == opc)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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

	static constexpr auto opc = llvm_value_t<T>::is_float ? llvm::Instruction::FMul : llvm::Instruction::Mul;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		return ir->CreateBinOp(opc, v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == opc)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}

			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			// Argument order does not matter here, try when swapped
			if (auto r1 = a1.match(v2, _m); v2)
			{
				if (auto r2 = a2.match(v1, _m); v1)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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

	static constexpr auto opc =
		llvm_value_t<T>::is_float ? llvm::Instruction::FDiv :
		llvm_value_t<T>::is_uint ? llvm::Instruction::UDiv : llvm::Instruction::SDiv;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		return ir->CreateBinOp(opc, v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == opc)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename T1, typename T2>
inline llvm_div<T1, T2> operator /(T1&& a1, T2&& a2)
{
	return {a1, a2};
}

inline llvm::Constant* getZeroValueForNegation(llvm::Type* Ty)
{
	if (Ty->isFPOrFPVectorTy())
		return llvm::ConstantFP::getNegativeZero(Ty);

	return llvm::Constant::getNullValue(Ty);
}

template <typename A1, typename T = llvm_common_t<A1>>
struct llvm_neg
{
	using type = T;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint || llvm_value_t<T>::is_float, "llvm_neg<>: invalid type");

	static constexpr int opc = llvm_value_t<T>::is_float ? +llvm::Instruction::FNeg : +llvm::Instruction::Sub;

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

		// TODO: return value ?
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if constexpr (llvm_value_t<T>::is_float)
		{
			if (auto i = llvm::dyn_cast_or_null<llvm::UnaryOperator>(value); i && i->getOpcode() == opc)
			{
				v1 = i->getOperand(0);

				if (auto r1 = a1.match(v1, _m); v1)
				{
					return r1;
				}
			}
		}

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == opc)
		{
			v1 = i->getOperand(1);

			if (i->getOperand(0) == getZeroValueForNegation(v1->getType()))
			{
				if (auto r1 = a1.match(v1, _m); v1)
				{
					return r1;
				}
			}
		}

		value = nullptr;
		return {};
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
		return ir->CreateShl(v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == llvm::Instruction::Shl)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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

	static constexpr auto opc = llvm_value_t<T>::is_uint ? llvm::Instruction::LShr : llvm::Instruction::AShr;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		return ir->CreateBinOp(opc, v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == opc)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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
		const auto _module = ir->GetInsertBlock()->getParent()->getParent();
		return llvm::Intrinsic::getDeclaration(_module, llvm::Intrinsic::fshl, {llvm_value_t<T>::get_type(ir->getContext())});
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

	llvm_match_tuple<A1, A2, A3> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};
		llvm::Value* v3 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == llvm::Intrinsic::fshl)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);
			v3 = i->getOperand(2);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					if (auto r3 = a3.match(v3, _m); v3)
					{
						return std::tuple_cat(r1, r2, r3);
					}
				}
			}
		}

		value = nullptr;
		return {};
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
		const auto _module = ir->GetInsertBlock()->getParent()->getParent();
		return llvm::Intrinsic::getDeclaration(_module, llvm::Intrinsic::fshr, {llvm_value_t<T>::get_type(ir->getContext())});
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

	llvm_match_tuple<A1, A2, A3> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};
		llvm::Value* v3 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == llvm::Intrinsic::fshr)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);
			v3 = i->getOperand(2);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					if (auto r3 = a3.match(v3, _m); v3)
					{
						return std::tuple_cat(r1, r2, r3);
					}
				}
			}
		}

		value = nullptr;
		return {};
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

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == llvm::Intrinsic::fshl)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(2);

			if (i->getOperand(1) == v1)
			{
				if (auto r1 = a1.match(v1, _m); v1)
				{
					if (auto r2 = a2.match(v2, _m); v2)
					{
						return std::tuple_cat(r1, r2);
					}
				}
			}
		}

		value = nullptr;
		return {};
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
		return ir->CreateAnd(v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == llvm::Instruction::And)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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
		return ir->CreateOr(v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == llvm::Instruction::Or)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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
		return ir->CreateXor(v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::BinaryOperator>(value); i && i->getOpcode() == llvm::Instruction::Xor)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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
	return {a1, {u64{umax}}};
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
		return ir->CreateICmp(pred, v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::ICmpInst>(value); i && i->getPredicate() == pred)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
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

	llvm_match_tuple<Cmp> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::FCmpInst>(value); i && i->getPredicate() == pred)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = cmp.a1.match(v1, _m); v1)
			{
				if (auto r2 = cmp.a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename T>
	requires is_llvm_cmp<std::decay_t<T>>::value
llvm_ord(T&&) -> llvm_ord<T&&>;

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

	llvm_match_tuple<Cmp> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::FCmpInst>(value); i && i->getPredicate() == pred)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = cmp.a1.match(v1, _m); v1)
			{
				if (auto r2 = cmp.a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename T>
	requires is_llvm_cmp<std::decay_t<T>>::value
llvm_uno(T&&) -> llvm_uno<T&&>;

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

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		if (value)
		{
			if (auto r1 = a1.match(value, _m); value)
			{
				return r1;
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename U, typename A1, typename T = llvm_common_t<A1>>
struct llvm_bitcast
{
	using type = U;

	llvm_expr_t<A1> a1;
	llvm::Module* _module;

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
			return v1;
		}

		if (const auto c1 = llvm::dyn_cast<llvm::Constant>(v1))
		{
			const auto result = llvm::ConstantFoldCastOperand(llvm::Instruction::BitCast, c1, rt, ir->GetInsertBlock()->getParent()->getParent()->getDataLayout());

			if (result)
			{
				return result;
			}
		}

		return ir->CreateBitCast(v1, rt);
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		if constexpr (llvm_value_t<T>::is_int == llvm_value_t<U>::is_int && llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector)
		{
			if (value)
			{
				if (auto r1 = a1.match(value, _m); value)
				{
					return r1;
				}
			}

			return {};
		}

		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CastInst>(value); i && i->getOpcode() == llvm::Instruction::BitCast)
		{
			v1 = i->getOperand(0);

			if (llvm_value_t<U>::get_type(v1->getContext()) == i->getDestTy())
			{
				if (auto r1 = a1.match(v1, _m); v1)
				{
					return r1;
				}
			}
		}

		if (auto c = llvm::dyn_cast_or_null<llvm::Constant>(value))
		{
			const auto target = llvm_value_t<T>::get_type(c->getContext());

			// Reverse bitcast on a constant
			if (llvm::Value* cv = llvm::ConstantFoldCastOperand(llvm::Instruction::BitCast, c, target, _m->getDataLayout()))
			{
				if (auto r1 = a1.match(cv, _m); cv)
				{
					return r1;
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename U, typename A1, typename T = llvm_common_t<A1>>
struct llvm_fpcast
{
	using type = U;

	static constexpr auto opc =
		llvm_value_t<T>::is_sint ? llvm::Instruction::SIToFP :
		llvm_value_t<U>::is_sint ? llvm::Instruction::FPToSI :
		llvm_value_t<T>::is_int ? llvm::Instruction::UIToFP :
		llvm_value_t<U>::is_int ? llvm::Instruction::FPToUI :
		llvm_value_t<T>::esize > llvm_value_t<U>::esize ? llvm::Instruction::FPTrunc :
		llvm_value_t<T>::esize < llvm_value_t<U>::esize ? llvm::Instruction::FPExt : llvm::Instruction::BitCast;

	llvm_expr_t<A1> a1;
	static_assert(llvm_value_t<T>::is_float || llvm_value_t<U>::is_float, "llvm_fpcast<>: invalid type(s)");
	static_assert(opc != llvm::Instruction::BitCast, "llvm_fpcast<>: possible cast to the same type");
	static_assert(llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector, "llvm_fpcast<>: vector element mismatch");

	static constexpr bool is_ok =
		(llvm_value_t<T>::is_float || llvm_value_t<U>::is_float) && opc != llvm::Instruction::BitCast &&
		llvm_value_t<T>::is_vector == llvm_value_t<U>::is_vector;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return ir->CreateCast(opc, a1.eval(ir), llvm_value_t<U>::get_type(ir->getContext()));
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CastInst>(value); i && i->getOpcode() == opc)
		{
			v1 = i->getOperand(0);

			if (llvm_value_t<U>::get_type(v1->getContext()) == i->getDestTy())
			{
				if (auto r1 = a1.match(v1, _m); v1)
				{
					return r1;
				}
			}
		}

		value = nullptr;
		return {};
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

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CastInst>(value); i && i->getOpcode() == llvm::Instruction::Trunc)
		{
			v1 = i->getOperand(0);

			if (llvm_value_t<U>::get_type(v1->getContext()) == i->getDestTy())
			{
				if (auto r1 = a1.match(v1, _m); v1)
				{
					return r1;
				}
			}
		}

		value = nullptr;
		return {};
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

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CastInst>(value); i && i->getOpcode() == llvm::Instruction::SExt)
		{
			v1 = i->getOperand(0);

			if (llvm_value_t<U>::get_type(v1->getContext()) == i->getDestTy())
			{
				if (auto r1 = a1.match(v1, _m); v1)
				{
					return r1;
				}
			}
		}

		value = nullptr;
		return {};
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

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CastInst>(value); i && i->getOpcode() == llvm::Instruction::ZExt)
		{
			v1 = i->getOperand(0);

			if (llvm_value_t<U>::get_type(v1->getContext()) == i->getDestTy())
			{
				if (auto r1 = a1.match(v1, _m); v1)
				{
					return r1;
				}
			}
		}

		value = nullptr;
		return {};
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

	llvm_match_tuple<A1, A2, A3> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};
		llvm::Value* v3 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::SelectInst>(value))
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);
			v3 = i->getOperand(2);

			if (auto r1 = cond.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					if (auto r3 = a3.match(v3, _m); v3)
					{
						return std::tuple_cat(r1, r2, r3);
					}
				}
			}
		}

		value = nullptr;
		return {};
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

	static constexpr auto pred = llvm_value_t<T>::is_sint ? llvm::ICmpInst::ICMP_SLT : llvm::ICmpInst::ICMP_ULT;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		return ir->CreateSelect(ir->CreateICmp(pred, v1, v2), v1, v2);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::SelectInst>(value))
		{
			v1 = i->getOperand(1);
			v2 = i->getOperand(2);

			if (auto j = llvm::dyn_cast<llvm::ICmpInst>(i->getOperand(0)); j && j->getPredicate() == pred)
			{
				if (v1 == j->getOperand(0) && v2 == j->getOperand(1))
				{
					if (auto r1 = a1.match(v1, _m); v1)
					{
						if (auto r2 = a2.match(v2, _m); v2)
						{
							return std::tuple_cat(r1, r2);
						}
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_max
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_max<>: invalid type");

	static constexpr auto pred = llvm_value_t<T>::is_sint ? llvm::ICmpInst::ICMP_SLT : llvm::ICmpInst::ICMP_ULT;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);
		return ir->CreateSelect(ir->CreateICmp(pred, v1, v2), v2, v1);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::SelectInst>(value))
		{
			v1 = i->getOperand(2);
			v2 = i->getOperand(1);

			if (auto j = llvm::dyn_cast<llvm::ICmpInst>(i->getOperand(0)); j && j->getPredicate() == pred)
			{
				if (v1 == j->getOperand(0) && v2 == j->getOperand(1))
				{
					if (auto r1 = a1.match(v1, _m); v1)
					{
						if (auto r2 = a2.match(v2, _m); v2)
						{
							return std::tuple_cat(r1, r2);
						}
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_add_sat
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_add_sat<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	static constexpr auto intr = llvm_value_t<T>::is_sint ? llvm::Intrinsic::sadd_sat : llvm::Intrinsic::uadd_sat;

	static llvm::Function* get_add_sat(llvm::IRBuilder<>* ir)
	{
		const auto _module = ir->GetInsertBlock()->getParent()->getParent();
		return llvm::Intrinsic::getDeclaration(_module, intr, {llvm_value_t<T>::get_type(ir->getContext())});
	}

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm::isa<llvm::Constant>(v1) && llvm::isa<llvm::Constant>(v2))
		{
			const auto sum = ir->CreateAdd(v1, v2);

			if constexpr (llvm_value_t<T>::is_sint)
			{
				const auto max = llvm::ConstantInt::get(v1->getType(), llvm::APInt::getSignedMaxValue(llvm_value_t<T>::esize));
				const auto sat = ir->CreateXor(ir->CreateAShr(v1, llvm_value_t<T>::esize - 1), max); // Max -> min if v1 < 0
				const auto ovf = ir->CreateAnd(ir->CreateXor(v2, sum), ir->CreateNot(ir->CreateXor(v1, v2))); // Get overflow
				return ir->CreateSelect(ir->CreateICmpSLT(ovf, llvm::ConstantInt::get(v1->getType(), 0)), sat, sum);
			}

			if constexpr (llvm_value_t<T>::is_uint)
			{
				const auto max = llvm::ConstantInt::get(v1->getType(), llvm::APInt::getMaxValue(llvm_value_t<T>::esize));
				return ir->CreateSelect(ir->CreateICmpULT(sum, v1), max, sum);
			}
		}

		return ir->CreateCall(get_add_sat(ir), {v1, v2});
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == intr)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}

			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			// Argument order does not matter here, try when swapped
			if (auto r1 = a1.match(v2, _m); v2)
			{
				if (auto r2 = a2.match(v1, _m); v1)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_sub_sat
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_sub_sat<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	static constexpr auto intr = llvm_value_t<T>::is_sint ? llvm::Intrinsic::ssub_sat : llvm::Intrinsic::usub_sat;

	static llvm::Function* get_sub_sat(llvm::IRBuilder<>* ir)
	{
		const auto _module = ir->GetInsertBlock()->getParent()->getParent();
		return llvm::Intrinsic::getDeclaration(_module, intr, {llvm_value_t<T>::get_type(ir->getContext())});
	}

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		if (llvm::isa<llvm::Constant>(v1) && llvm::isa<llvm::Constant>(v2))
		{
			const auto dif = ir->CreateSub(v1, v2);

			if constexpr (llvm_value_t<T>::is_sint)
			{
				const auto max = llvm::ConstantInt::get(v1->getType(), llvm::APInt::getSignedMaxValue(llvm_value_t<T>::esize));
				const auto sat = ir->CreateXor(ir->CreateAShr(v1, llvm_value_t<T>::esize - 1), max); // Max -> min if v1 < 0
				const auto ovf = ir->CreateAnd(ir->CreateXor(v1, dif), ir->CreateXor(v1, v2)); // Get overflow (subtraction)
				return ir->CreateSelect(ir->CreateICmpSLT(ovf, llvm::ConstantInt::get(v1->getType(), 0)), sat, dif);
			}

			if constexpr (llvm_value_t<T>::is_uint)
			{
				return ir->CreateSelect(ir->CreateICmpULT(v1, v2), llvm::ConstantInt::get(v1->getType(), 0), dif);
			}
		}

		return ir->CreateCall(get_sub_sat(ir), {v1, v2});
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == intr)
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename I2, typename T = llvm_common_t<A1>, typename U = llvm_common_t<I2>>
struct llvm_extract
{
	using type = std::remove_extent_t<T>;

	llvm_expr_t<A1> a1;
	llvm_expr_t<I2> i2;

	static_assert(llvm_value_t<T>::is_vector, "llvm_extract<>: invalid type");
	static_assert(llvm_value_t<U>::is_int && !llvm_value_t<U>::is_vector, "llvm_extract<>: invalid index type");

	static constexpr bool is_ok = llvm_value_t<T>::is_vector &&
		llvm_value_t<U>::is_int && !llvm_value_t<U>::is_vector;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = i2.eval(ir);

		return ir->CreateExtractElement(v1, v2);
	}

	llvm_match_tuple<A1, I2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::ExtractElementInst>(value))
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = i2.match(v2, _m); v2)
				{
					return std::tuple_cat(r1, r2);
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename I2, typename A3, typename T = llvm_common_t<A1>, typename U = llvm_common_t<I2>, typename V = llvm_common_t<A3>>
struct llvm_insert
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<I2> i2;
	llvm_expr_t<A3> a3;

	static_assert(llvm_value_t<T>::is_vector, "llvm_insert<>: invalid type");
	static_assert(llvm_value_t<U>::is_int && !llvm_value_t<U>::is_vector, "llvm_insert<>: invalid index type");
	static_assert(std::is_same_v<V, std::remove_extent_t<T>>, "llvm_insert<>: invalid element type");

	static constexpr bool is_ok = llvm_extract<A1, I2>::is_ok &&
		std::is_same_v<V, std::remove_extent_t<T>>;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = i2.eval(ir);
		const auto v3 = a3.eval(ir);

		return ir->CreateInsertElement(v1, v3, v2);
	}

	llvm_match_tuple<A1, I2, A3> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};
		llvm::Value* v3 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::InsertElementInst>(value))
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(2);
			v3 = i->getOperand(1);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = i2.match(v2, _m); v2)
				{
					if (auto r3 = a3.match(v3, _m); v3)
					{
						return std::tuple_cat(r1, r2, r3);
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename U, typename A1, typename T = llvm_common_t<A1>>
struct llvm_splat
{
	using type = U;

	llvm_expr_t<A1> a1;

	static_assert(!llvm_value_t<T>::is_vector, "llvm_splat<>: invalid type");
	static_assert(llvm_value_t<U>::is_vector, "llvm_splat<>: invalid result type");
	static_assert(std::is_same_v<T, std::remove_extent_t<U>>, "llvm_splat<>: incompatible splat type");

	static constexpr bool is_ok =
		!llvm_value_t<T>::is_vector &&
		llvm_value_t<U>::is_vector &&
		std::is_same_v<T, std::remove_extent_t<U>>;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);

		return ir->CreateVectorSplat(llvm_value_t<U>::is_vector, v1);
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::ShuffleVectorInst>(value))
		{
			if (llvm::isa<llvm::ConstantAggregateZero>(i->getOperand(1)) || llvm::isa<llvm::UndefValue>(i->getOperand(1)))
			{
				static constexpr int zero_array[llvm_value_t<U>::is_vector]{};

				if (auto j = llvm::dyn_cast<llvm::InsertElementInst>(i->getOperand(0)); j && i->getShuffleMask().equals(zero_array))
				{
					if (llvm::cast<llvm::ConstantInt>(j->getOperand(2))->isZero())
					{
						v1 = j->getOperand(1);

						if (auto r1 = a1.match(v1, _m); v1)
						{
							return r1;
						}
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <uint N, typename A1, typename T = llvm_common_t<A1>>
struct llvm_zshuffle
{
	using type = std::remove_extent_t<T>[N];

	llvm_expr_t<A1> a1;
	int index_array[N];

	static_assert(llvm_value_t<T>::is_vector, "llvm_zshuffle<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_vector && 1;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);

		return ir->CreateShuffleVector(v1, llvm::ConstantAggregateZero::get(v1->getType()), index_array);
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::ShuffleVectorInst>(value))
		{
			v1 = i->getOperand(0);

			if (auto z = llvm::dyn_cast<llvm::ConstantAggregateZero>(i->getOperand(1)); z && z->getType() == v1->getType())
			{
				if (i->getShuffleMask().equals(index_array))
				{
					if (auto r1 = a1.match(v1, _m); v1)
					{
						return r1;
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <uint N, typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_shuffle2
{
	using type = std::remove_extent_t<T>[N];

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;
	int index_array[N];

	static_assert(llvm_value_t<T>::is_vector, "llvm_shuffle2<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_vector && 1;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		return ir->CreateShuffleVector(v1, v2, index_array);
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::ShuffleVectorInst>(value))
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);

			if (v1->getType() == v2->getType() && v1->getType() == llvm_value_t<T>::get_type(v1->getContext()))
			{
				if (i->getShuffleMask().equals(index_array))
				{
					if (auto r1 = a1.match(v1, _m); v1)
					{
						if (auto r2 = a2.match(v2, _m); v2)
						{
							return std::tuple_cat(r1, r2);
						}
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename T = llvm_common_t<A1>>
struct llvm_ctlz
{
	using type = T;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_ctlz<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		llvm::Value* v = a1.eval(ir);

		if (llvm::isa<llvm::Constant>(v))
		{
			return llvm::ConstantFoldInstruction(ir->CreateIntrinsic(llvm::Intrinsic::ctlz, {v->getType()}, {v, ir->getFalse()}), llvm::DataLayout(""));
		}

		return ir->CreateIntrinsic(llvm::Intrinsic::ctlz, {v->getType()}, {v, ir->getFalse()});
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == llvm::Intrinsic::ctlz)
		{
			v1 = i->getOperand(0);

			if (i->getOperand(2) == llvm::ConstantInt::getFalse(value->getContext()))
			{
				if (auto r1 = a1.match(v1, _m); v1)
				{
					return r1;
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename T = llvm_common_t<A1>>
struct llvm_ctpop
{
	using type = T;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_ctpop<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		llvm::Value* v = a1.eval(ir);

		if (llvm::isa<llvm::Constant>(v))
		{
			return llvm::ConstantFoldInstruction(ir->CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, v), llvm::DataLayout(""));
		}

		return ir->CreateUnaryIntrinsic(llvm::Intrinsic::ctpop, v);
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == llvm::Intrinsic::ctpop)
		{
			v1 = i->getOperand(0);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				return r1;
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename A2, typename T = llvm_common_t<A1, A2>>
struct llvm_avg
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;

	static_assert(llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint, "llvm_avg<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_sint || llvm_value_t<T>::is_uint;

	static constexpr auto cast_op = llvm_value_t<T>::is_sint ? llvm::Instruction::SExt : llvm::Instruction::ZExt;

	static llvm::Type* cast_dst_type(llvm::LLVMContext& context)
	{
		llvm::Type* cast_to = llvm::IntegerType::get(context, llvm_value_t<T>::esize * 2);
		if constexpr (llvm_value_t<T>::is_vector != 0)
			cast_to = llvm::VectorType::get(cast_to, llvm_value_t<T>::is_vector, false);

		return cast_to;
	}

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		const auto v1 = a1.eval(ir);
		const auto v2 = a2.eval(ir);

		const auto dty = cast_dst_type(ir->getContext());
		const auto axt = ir->CreateCast(cast_op, v1, dty);
		const auto bxt = ir->CreateCast(cast_op, v2, dty);
		const auto cxt = llvm::ConstantInt::get(dty, 1, false);
		const auto abc = ir->CreateAdd(ir->CreateAdd(axt, bxt), cxt);
		return ir->CreateTrunc(ir->CreateLShr(abc, 1), llvm_value_t<T>::get_type(ir->getContext()));
	}

	llvm_match_tuple<A1, A2> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};

		const auto dty = cast_dst_type(value->getContext());

		if (auto i = llvm::dyn_cast_or_null<llvm::CastInst>(value); i && i->getOpcode() == llvm::Instruction::Trunc && i->getSrcTy() == dty)
		{
			const auto cxt = llvm::ConstantInt::get(dty, 1, false);

			if (auto j = llvm::dyn_cast_or_null<llvm::BinaryOperator>(i->getOperand(0)); j && j->getOpcode() == llvm::Instruction::LShr && j->getOperand(1) == cxt)
			{
				if (j = llvm::dyn_cast_or_null<llvm::BinaryOperator>(j->getOperand(0)); j && j->getOpcode() == llvm::Instruction::Add && j->getOperand(1) == cxt)
				{
					if (j = llvm::dyn_cast_or_null<llvm::BinaryOperator>(j->getOperand(0)); j && j->getOpcode() == llvm::Instruction::Add)
					{
						auto a = llvm::dyn_cast_or_null<llvm::CastInst>(j->getOperand(0));
						auto b = llvm::dyn_cast_or_null<llvm::CastInst>(j->getOperand(1));

						if (a && b && a->getOpcode() == cast_op && b->getOpcode() == cast_op)
						{
							v1 = a->getOperand(0);
							v2 = b->getOperand(0);

							if (auto r1 = a1.match(v1, _m); v1)
							{
								if (auto r2 = a2.match(v2, _m); v2)
								{
									return std::tuple_cat(r1, r2);
								}
							}
						}
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename T = llvm_common_t<A1>>
struct llvm_fsqrt
{
	using type = T;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_float, "llvm_fsqrt<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_float;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		llvm::Value* v = a1.eval(ir);

		if (llvm::isa<llvm::Constant>(v))
		{
			if (auto c = llvm::ConstantFoldInstruction(ir->CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, v), llvm::DataLayout("")))
			{
				// Will fail in some cases (such as negative constant)
				return c;
			}
		}

		return ir->CreateUnaryIntrinsic(llvm::Intrinsic::sqrt, v);
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == llvm::Intrinsic::sqrt)
		{
			v1 = i->getOperand(0);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				return r1;
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename T = llvm_common_t<A1>>
struct llvm_fabs
{
	using type = T;

	llvm_expr_t<A1> a1;

	static_assert(llvm_value_t<T>::is_float, "llvm_fabs<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_float;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		llvm::Value* v = a1.eval(ir);

		if (llvm::isa<llvm::Constant>(v))
		{
			return llvm::ConstantFoldInstruction(ir->CreateUnaryIntrinsic(llvm::Intrinsic::fabs, v), llvm::DataLayout(""));
		}

		return ir->CreateUnaryIntrinsic(llvm::Intrinsic::fabs, v);
	}

	llvm_match_tuple<A1> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == llvm::Intrinsic::fabs)
		{
			v1 = i->getOperand(0);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				return r1;
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename A1, typename A2, typename A3, typename T = llvm_common_t<A1, A2, A3>>
struct llvm_fmuladd
{
	using type = T;

	llvm_expr_t<A1> a1;
	llvm_expr_t<A2> a2;
	llvm_expr_t<A3> a3;
	bool strict_fma;

	static_assert(llvm_value_t<T>::is_float, "llvm_fmuladd<>: invalid type");

	static constexpr bool is_ok = llvm_value_t<T>::is_float;

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		llvm::Value* v1 = a1.eval(ir);
		llvm::Value* v2 = a2.eval(ir);
		llvm::Value* v3 = a3.eval(ir);

		if (llvm::isa<llvm::Constant>(v1) && llvm::isa<llvm::Constant>(v2) && llvm::isa<llvm::Constant>(v3))
		{
			return llvm::ConstantFoldInstruction(ir->CreateIntrinsic(llvm::Intrinsic::fma, {v1->getType()}, {v1, v2, v3}), llvm::DataLayout(""));
		}

		return ir->CreateIntrinsic(strict_fma ? llvm::Intrinsic::fma : llvm::Intrinsic::fmuladd, {v1->getType()}, {v1, v2, v3});
	}

	llvm_match_tuple<A1, A2, A3> match(llvm::Value*& value, llvm::Module* _m) const
	{
		llvm::Value* v1 = {};
		llvm::Value* v2 = {};
		llvm::Value* v3 = {};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value); i && i->getIntrinsicID() == (strict_fma ? llvm::Intrinsic::fma : llvm::Intrinsic::fmuladd))
		{
			v1 = i->getOperand(0);
			v2 = i->getOperand(1);
			v3 = i->getOperand(2);

			if (auto r1 = a1.match(v1, _m); v1)
			{
				if (auto r2 = a2.match(v2, _m); v2)
				{
					if (auto r3 = a3.match(v3, _m); v3)
					{
						return std::tuple_cat(r1, r2, r3);
					}
				}
			}

			v1 = i->getOperand(0);
			v2 = i->getOperand(1);
			v3 = i->getOperand(2);

			// With multiplication args swapped
			if (auto r1 = a1.match(v2, _m); v2)
			{
				if (auto r2 = a2.match(v1, _m); v1)
				{
					if (auto r3 = a3.match(v3, _m); v3)
					{
						return std::tuple_cat(r1, r2, r3);
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

template <typename RT, typename... A>
struct llvm_calli
{
	using type = RT;

	llvm::StringRef iname;

	std::tuple<llvm_expr_t<A>...> a;

	std::array<usz, std::max<usz>(sizeof...(A), 1)> order_equality_hint = []()
	{
		std::array<usz, std::max<usz>(sizeof...(A), 1)> r{};

		for (usz i = 0; i < r.size(); i++)
		{
			r[i] = i;
		}

		return r;
	}();

	llvm::Value*(*c)(llvm::Value**, llvm::IRBuilder<>*){};

	llvm::Value* eval(llvm::IRBuilder<>* ir) const
	{
		return eval(ir, std::make_index_sequence<sizeof...(A)>());
	}

	template <usz... I>
	llvm::Value* eval(llvm::IRBuilder<>* ir, std::index_sequence<I...>) const
	{
		llvm::Value* v[std::max<usz>(sizeof...(A), 1)]{std::get<I>(a).eval(ir)...};

		if (c && (llvm::isa<llvm::Constant>(v[I]) || ...))
		{
			if (llvm::Value* r = c(v, ir))
			{
				return r;
			}
		}

		const auto _rty = llvm_value_t<RT>::get_type(ir->getContext());
		const auto type = llvm::FunctionType::get(_rty, {v[I]->getType()...}, false);
		const auto func = llvm::cast<llvm::Function>(ir->GetInsertBlock()->getParent()->getParent()->getOrInsertFunction(iname, type).getCallee());
		return ir->CreateCall(func, v);
	}

	template <typename F>
	llvm_calli& if_const(F func)
	{
		c = +func;
		return *this;
	}

	template <typename... Args> requires (sizeof...(Args) == sizeof...(A))
	llvm_calli& set_order_equality_hint(Args... args)
	{
		order_equality_hint = {static_cast<usz>(args)...};
		return *this;
	}

	llvm_match_tuple<A...> match(llvm::Value*& value, llvm::Module* _m) const
	{
		return match(value, _m, std::make_index_sequence<sizeof...(A)>());
	}

	template <usz... I>
	llvm_match_tuple<A...> match(llvm::Value*& value, llvm::Module* _m, std::index_sequence<I...>) const
	{
		llvm::Value* v[sizeof...(A)]{};

		if (auto i = llvm::dyn_cast_or_null<llvm::CallInst>(value))
		{
			if (auto cf = i->getCalledFunction(); cf && cf->getName() == iname)
			{
				((v[I] = i->getOperand(I)), ...);

				std::tuple<decltype(std::get<I>(a).match(v[I], _m))...> r;

				if (((std::get<I>(r) = std::get<I>(a).match(v[I], _m), v[I]) && ...))
				{
					return std::tuple_cat(std::get<I>(r)...);
				}

				if constexpr (sizeof...(A) >= 2)
				{
					if (order_equality_hint[0] == order_equality_hint[1])
					{
						// Test if it works with the first pair swapped
						((v[I <= 1 ? I ^ 1 : I] = i->getOperand(I)), ...);

						if (((std::get<I>(r) = std::get<I>(a).match(v[I], _m), v[I]) && ...))
						{
							return std::tuple_cat(std::get<I>(r)...);
						}
					}
				}
			}
		}

		value = nullptr;
		return {};
	}
};

class translator_pass
{
public:
	translator_pass() = default;
	virtual ~translator_pass() {}

	virtual void run(llvm::IRBuilder<>* irb, llvm::Function& func) = 0;
	virtual void reset() = 0;
};

class cpu_translator
{
protected:
	cpu_translator(llvm::Module* _module, bool is_be);

	// LLVM context
	std::reference_wrapper<llvm::LLVMContext> m_context;

	// Module to which all generated code is output to
	llvm::Module* m_module;

	// Execution engine from JIT instance
	llvm::ExecutionEngine* m_engine{};

	// Endianness, affects vector element numbering (TODO)
	bool m_is_be;

	// Allow PSHUFB intrinsic
	bool m_use_ssse3 = true;

	// Allow FMA
	bool m_use_fma = false;

	// Allow AVX
	bool m_use_avx = false;

	// Allow skylake-x tier AVX-512
	bool m_use_avx512 = false;

	// Allow VNNI
	bool m_use_vnni = false;

	// Allow GFNI
	bool m_use_gfni = false;

	// Allow Icelake tier AVX-512
	bool m_use_avx512_icl = false;

	// IR builder
	llvm::IRBuilder<>* m_ir = nullptr;

	// CUstomized transformation passes. Technically the intrinsics replacement belongs here.
	std::vector<std::unique_ptr<translator_pass>> m_transform_passes;

	void initialize(llvm::LLVMContext& context, llvm::ExecutionEngine& engine);

	// Run intrinsics replacement pass
	void replace_intrinsics(llvm::Function&);

public:
	// Register a transformation pass to be run before final compilation by llvm
	void register_transform_pass(std::unique_ptr<translator_pass>& pass);

	// Delete all transform passes
	void clear_transforms();

	// Reset internal state of all passes to evict caches and such. Use when resetting a JIT.
	void reset_transforms();

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

	// Call external function: provide name and function pointer
	template <typename RetT = void, typename RT, typename... FArgs, LLVMValue... Args>
	llvm::CallInst* call(std::string_view lame, RT(*_func)(FArgs...), Args... args)
	{
		static_assert(sizeof...(FArgs) == sizeof...(Args), "spu_llvm_recompiler::call(): unexpected arg number");
		const auto type = llvm::FunctionType::get(get_type<std::conditional_t<std::is_void_v<RetT>, RT, RetT>>(), {args->getType()...}, false);
		const auto func = llvm::cast<llvm::Function>(m_module->getOrInsertFunction({lame.data(), lame.size()}, type).getCallee());
#ifdef _WIN32
		func->setCallingConv(llvm::CallingConv::Win64);
#endif
		m_engine->updateGlobalMapping({lame.data(), lame.size()}, reinterpret_cast<uptr>(_func));

		const auto inst = m_ir->CreateCall(func, {args...});
		inst->setTailCallKind(llvm::CallInst::TCK_NoTail);
#ifdef _WIN32
		inst->setCallingConv(llvm::CallingConv::Win64);
#endif
		return inst;
	}

	template <typename RT, typename... FArgs, DSLValue... Args> requires (sizeof...(Args) != 0)
	auto call(std::string_view name, RT(*_func)(FArgs...), Args&&... args)
	{
		llvm_value_t<RT> r;
		r.value = call(name, _func, std::forward<Args>(args).eval(m_ir)...);
		return r;
	}

	template <typename RT, DSLValue... Args>
	auto callf(llvm::Function* func, Args&&... args)
	{
		llvm_value_t<RT> r;
		r.value = m_ir->CreateCall(func, {std::forward<Args>(args).eval(m_ir)...});
		return r;
	}

	// Bitcast with immediate constant folding
	llvm::Value* bitcast(llvm::Value* val, llvm::Type* type) const;

	template <typename T>
	llvm::Value* bitcast(llvm::Value* val)
	{
		return bitcast(val, get_type<T>());
	}

	template <typename T>
	static llvm_placeholder_t<T> match()
	{
		return {};
	}

	template <typename T>
		requires requires { typename llvm_common_t<T>; }
	static auto match_expr(llvm::Value* v, llvm::Module* _m, T&& expr)
	{
		auto r = expr.match(v, _m);
		return std::tuple_cat(std::make_tuple(v != nullptr), r);
	}

	template <typename T, typename U>
		requires requires { typename llvm_common_t<T, U>; }
	auto match_expr(T&& arg, U&& expr) -> decltype(std::tuple_cat(std::make_tuple(false), expr.match(std::declval<llvm::Value*&>(), nullptr)))
	{
		auto v = arg.eval(m_ir);
		auto r = expr.match(v, m_module);
		return std::tuple_cat(std::make_tuple(v != nullptr), r);
	}

	template <typename... Types, typename F>
	bool match_for(F&& pred)
	{
		// Execute pred(.) for each type until one of them returns true
		return (pred(llvm_placeholder_t<Types>{}) || ...);
	}

	template <typename T, typename F>
	struct expr_t
	{
		using type = llvm_common_t<T>;

		T a;
		F match;

		llvm::Value* eval(llvm::IRBuilder<>* ir) const
		{
			return a.eval(ir);
		}
	};

	template <typename T, typename F>
	static auto expr(T&& expr, F matcher)
	{
		return expr_t<T, F>{std::forward<T>(expr), std::move(matcher)};
	}

	template <typename T>
		requires is_llvm_cmp<std::decay_t<T>>::value
	static auto fcmp_ord(T&& cmp_expr)
	{
		return llvm_ord{std::forward<T>(cmp_expr)};
	}

	template <typename T>
		requires is_llvm_cmp<std::decay_t<T>>::value
	static auto fcmp_uno(T&& cmp_expr)
	{
		return llvm_uno{std::forward<T>(cmp_expr)};
	}

	template <typename U, typename T>
		requires llvm_noncast<U, T>::is_ok
	static auto noncast(T&& expr)
	{
		return llvm_noncast<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T>
		requires llvm_bitcast<U, T>::is_ok
	static auto bitcast(T&& expr)
	{
		return llvm_bitcast<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T>
		requires llvm_fpcast<U, T>::is_ok
	static auto fpcast(T&& expr)
	{
		return llvm_fpcast<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T>
		requires llvm_trunc<U, T>::is_ok
	static auto trunc(T&& expr)
	{
		return llvm_trunc<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T>
		requires llvm_sext<U, T>::is_ok
	static auto sext(T&& expr)
	{
		return llvm_sext<U, T>{std::forward<T>(expr)};
	}

	template <typename U, typename T>
		requires llvm_zext<U, T>::is_ok
	static auto zext(T&& expr)
	{
		return llvm_zext<U, T>{std::forward<T>(expr)};
	}

	template <typename T, typename U, typename V>
		requires llvm_select<T, U, V>::is_ok
	static auto select(T&& c, U&& a, V&& b)
	{
		return llvm_select<T, U, V>{std::forward<T>(c), std::forward<U>(a), std::forward<V>(b)};
	}

	template <typename T, typename U>
		requires llvm_min<T, U>::is_ok
	static auto min(T&& a, U&& b)
	{
		return llvm_min<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	template <typename T, typename U>
		requires llvm_min<T, U>::is_ok
	static auto max(T&& a, U&& b)
	{
		return llvm_max<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	template <typename T, typename U, typename V>
		requires llvm_fshl<T, U, V>::is_ok
	static auto fshl(T&& a, U&& b, V&& c)
	{
		return llvm_fshl<T, U, V>{std::forward<T>(a), std::forward<U>(b), std::forward<V>(c)};
	}

	template <typename T, typename U, typename V>
		requires llvm_fshr<T, U, V>::is_ok
	static auto fshr(T&& a, U&& b, V&& c)
	{
		return llvm_fshr<T, U, V>{std::forward<T>(a), std::forward<U>(b), std::forward<V>(c)};
	}

	template <typename T, typename U>
		requires llvm_rol<T, U>::is_ok
	static auto rol(T&& a, U&& b)
	{
		return llvm_rol<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	template <typename T, typename U>
		requires llvm_add_sat<T, U>::is_ok
	static auto add_sat(T&& a, U&& b)
	{
		return llvm_add_sat<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	template <typename T, typename U>
		requires llvm_sub_sat<T, U>::is_ok
	static auto sub_sat(T&& a, U&& b)
	{
		return llvm_sub_sat<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	template <typename T, typename U>
		requires llvm_extract<T, U>::is_ok
	static auto extract(T&& v, U&& i)
	{
		return llvm_extract<T, U>{std::forward<T>(v), std::forward<U>(i)};
	}

	template <typename T>
		requires llvm_extract<T, llvm_const_int<u32>>::is_ok
	static auto extract(T&& v, u32 i)
	{
		return llvm_extract<T, llvm_const_int<u32>>{std::forward<T>(v), llvm_const_int<u32>{i}};
	}

	template <typename T, typename U, typename V>
		requires llvm_insert<T, U, V>::is_ok
	static auto insert(T&& v, U&& i, V&& e)
	{
		return llvm_insert<T, U, V>{std::forward<T>(v), std::forward<U>(i), std::forward<V>(e)};
	}

	template <typename T, typename V>
		requires llvm_insert<T, llvm_const_int<u32>, V>::is_ok
	static auto insert(T&& v, u32 i, V&& e)
	{
		return llvm_insert<T, llvm_const_int<u32>, V>{std::forward<T>(v), llvm_const_int<u32>{i}, std::forward<V>(e)};
	}

	template <typename T>
		requires llvm_const_int<T>::is_ok
	static auto splat(u64 c)
	{
		return llvm_const_int<T>{c};
	}

	template <typename T>
		requires llvm_const_float<T>::is_ok
	static auto fsplat(f64 c)
	{
		return llvm_const_float<T>{c};
	}

	template <typename T, typename U>
		requires llvm_splat<T, U>::is_ok
	static auto vsplat(U&& v)
	{
		return llvm_splat<T, U>{std::forward<U>(v)};
	}

	template <typename T, typename... Args>
		requires llvm_const_vector<sizeof...(Args), T>::is_ok
	static auto build(Args... args)
	{
		return llvm_const_vector<sizeof...(Args), T>{static_cast<std::remove_extent_t<T>>(args)...};
	}

	template <typename T, typename... Args>
		requires llvm_zshuffle<sizeof...(Args), T>::is_ok
	static auto zshuffle(T&& v, Args... indices)
	{
		return llvm_zshuffle<sizeof...(Args), T>{std::forward<T>(v), {static_cast<int>(indices)...}};
	}

	template <typename T, typename U, typename... Args>
		requires llvm_shuffle2<sizeof...(Args), T, U>::is_ok
	static auto shuffle2(T&& v1, U&& v2, Args... indices)
	{
		return llvm_shuffle2<sizeof...(Args), T, U>{std::forward<T>(v1), std::forward<U>(v2), {static_cast<int>(indices)...}};
	}

	template <typename T>
		requires llvm_ctlz<T>::is_ok
	static auto ctlz(T&& a)
	{
		return llvm_ctlz<T>{std::forward<T>(a)};
	}

	template <typename T>
		requires llvm_ctpop<T>::is_ok
	static auto ctpop(T&& a)
	{
		return llvm_ctpop<T>{std::forward<T>(a)};
	}

	// Average: (a + b + 1) >> 1
	template <typename T, typename U>
		requires llvm_avg<T, U>::is_ok
	static auto avg(T&& a, U&& b)
	{
		return llvm_avg<T, U>{std::forward<T>(a), std::forward<U>(b)};
	}

	template <typename T>
		requires llvm_fsqrt<T>::is_ok
	static auto fsqrt(T&& a)
	{
		return llvm_fsqrt<T>{std::forward<T>(a)};
	}

	template <typename T>
		requires llvm_fabs<T>::is_ok
	static auto fabs(T&& a)
	{
		return llvm_fabs<T>{std::forward<T>(a)};
	}

	// Optionally opportunistic hardware FMA, can be used if results are identical for all possible input values
	template <typename T, typename U, typename V>
		requires llvm_fmuladd<T, U, V>::is_ok
	static auto fmuladd(T&& a, U&& b, V&& c, bool strict_fma)
	{
		return llvm_fmuladd<T, U, V>{std::forward<T>(a), std::forward<U>(b), std::forward<V>(c), strict_fma};
	}

	// Opportunistic hardware FMA, can be used if results are identical for all possible input values
	template <typename T, typename U, typename V>
		requires llvm_fmuladd<T, U, V>::is_ok
	auto fmuladd(T&& a, U&& b, V&& c)
	{
		return llvm_fmuladd<T, U, V>{std::forward<T>(a), std::forward<U>(b), std::forward<V>(c), m_use_fma};
	}

	// Absolute difference
	template <typename T, typename U, typename CT = llvm_common_t<T, U>>
	static auto absd(T&& a, U&& b)
	{
		return expr(max(a, b) - min(a, b), [](llvm::Value*& value, llvm::Module* _m) -> llvm_match_tuple<T, U>
		{
			static const auto M = match<CT>();

			if (auto [ok, _max, _min] = match_expr(value, _m, M - M); ok)
			{
				if (auto [ok1, a, b] = match_expr(_max.value, _m, max(M, M)); ok1 && !a.eq(b))
				{
					if (auto [ok2, c, d] = match_expr(_min.value, _m, min(M, M)); ok2 && !c.eq(d))
					{
						if ((a.eq(c) && b.eq(d)) || (a.eq(d) && b.eq(c)))
						{
							if (auto r1 = llvm_expr_t<T>{}.match(a.value, _m); a.eq())
							{
								if (auto r2 = llvm_expr_t<U>{}.match(b.value, _m); b.eq())
								{
									return std::tuple_cat(r1, r2);
								}
							}
						}
					}
				}
			}

			value = nullptr;
			return {};
		});
	}

	// Infinite-precision shift left
	template <typename T, typename U, typename CT = llvm_common_t<T, U>>
	auto inf_shl(T&& a, U&& b)
	{
		static constexpr u32 esz = llvm_value_t<CT>::esize;

		return expr(select(b < esz, a << b, splat<CT>(0)), [](llvm::Value*& value, llvm::Module* _m) -> llvm_match_tuple<T, U>
		{
			static const auto M = match<CT>();

			if (auto [ok, b, a, b2] = match_expr(value, _m, select(M < esz, M << M, splat<CT>(0))); ok && b.eq(b2))
			{
				if (auto r1 = llvm_expr_t<T>{}.match(a.value, _m); a.eq())
				{
					if (auto r2 = llvm_expr_t<U>{}.match(b.value, _m); b.eq())
					{
						return std::tuple_cat(r1, r2);
					}
				}
			}

			value = nullptr;
			return {};
		});
	}

	// Infinite-precision logical shift right (unsigned)
	template <typename T, typename U, typename CT = llvm_common_t<T, U>>
	auto inf_lshr(T&& a, U&& b)
	{
		static constexpr u32 esz = llvm_value_t<CT>::esize;

		return expr(select(b < esz, a >> b, splat<CT>(0)), [](llvm::Value*& value, llvm::Module* _m) -> llvm_match_tuple<T, U>
		{
			static const auto M = match<CT>();

			if (auto [ok, b, a, b2] = match_expr(value, _m, select(M < esz, M >> M, splat<CT>(0))); ok && b.eq(b2))
			{
				if (auto r1 = llvm_expr_t<T>{}.match(a.value, _m); a.eq())
				{
					if (auto r2 = llvm_expr_t<U>{}.match(b.value, _m); b.eq())
					{
						return std::tuple_cat(r1, r2);
					}
				}
			}

			value = nullptr;
			return {};
		});
	}

	// Infinite-precision arithmetic shift right (signed)
	template <typename T, typename U, typename CT = llvm_common_t<T, U>>
	auto inf_ashr(T&& a, U&& b)
	{
		static constexpr u32 esz = llvm_value_t<CT>::esize;

		return expr(a >> select(b > (esz - 1), splat<CT>(esz - 1), b), [](llvm::Value*& value, llvm::Module* _m) -> llvm_match_tuple<T, U>
		{
			static const auto M = match<CT>();

			if (auto [ok, a, b, b2] = match_expr(value, _m, M >> select(M > (esz - 1), splat<CT>(esz - 1), M)); ok && b.eq(b2))
			{
				if (auto r1 = llvm_expr_t<T>{}.match(a.value, _m); a.eq())
				{
					if (auto r2 = llvm_expr_t<U>{}.match(b.value, _m); b.eq())
					{
						return std::tuple_cat(r1, r2);
					}
				}
			}

			value = nullptr;
			return {};
		});
	}

	template <typename... Types>
	llvm::Function* get_intrinsic(llvm::Intrinsic::ID id)
	{
		const auto _module = m_ir->GetInsertBlock()->getParent()->getParent();
		return llvm::Intrinsic::getDeclaration(_module, id, {get_type<Types>()...});
	}

	template <typename T1, typename T2>
	value_t<u8[16]> gf2p8affineqb(T1 a, T2 b, u8 c)
	{
		value_t<u8[16]> result;

		const auto data0 = a.eval(m_ir);
		const auto data1 = b.eval(m_ir);

		const auto immediate = (llvm_const_int<u8>{c});
		const auto imm8 = immediate.eval(m_ir);

		result.value = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_vgf2p8affineqb_128), {data0, data1, imm8});
		return result;
	}

	template <typename T1, typename T2, typename T3>
	value_t<u32[4]> vpdpbusd(T1 a, T2 b, T3 c)
	{
		value_t<u32[4]> result;

		const auto data0 = a.eval(m_ir);
		const auto data1 = b.eval(m_ir);
		const auto data2 = c.eval(m_ir);
		result.value = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_avx512_vpdpbusd_128), {data0, data1, data2});
		return result;
	}

	template <typename T1, typename T2>
	value_t<u8[16]> vpermb(T1 a, T2 b)
	{
		value_t<u8[16]> result;

		const auto data0 = a.eval(m_ir);
		const auto index = b.eval(m_ir);
		const auto zeros = llvm::ConstantAggregateZero::get(get_type<u8[16]>());

		if (auto c = llvm::dyn_cast<llvm::Constant>(index))
		{
			// Convert VPERMB index back to LLVM vector shuffle mask
			v128 mask{};

			const auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(c);

			if (cv)
			{
				for (u32 i = 0; i < 16; i++)
				{
					const u64 b = cv->getElementAsInteger(i);
					mask._u8[i] = b & 0xf;
				}
			}

			if (cv || llvm::isa<llvm::ConstantAggregateZero>(c))
			{
				result.value = llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u8*>(&mask), 16));
				result.value = m_ir->CreateZExt(result.value, get_type<u32[16]>());
				result.value = m_ir->CreateShuffleVector(data0, zeros, result.value);
				return result;
			}
		}

		result.value = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_avx512_permvar_qi_128), {data0, index});
		return result;
	}

	template <typename T1, typename T2, typename T3>
	value_t<u8[16]> vperm2b(T1 a, T2 b, T3 c)
	{
		if (!utils::has_fast_vperm2b())
		{
			return vperm2b256to128(a, b, c);
		}

		value_t<u8[16]> result;

		const auto data0 = a.eval(m_ir);
		const auto data1 = b.eval(m_ir);
		const auto index = c.eval(m_ir);

		if (auto c = llvm::dyn_cast<llvm::Constant>(index))
		{
			// Convert VPERM2B index back to LLVM vector shuffle mask
			v128 mask{};

			const auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(c);

			if (cv)
			{
				for (u32 i = 0; i < 16; i++)
				{
					const u64 b = cv->getElementAsInteger(i);
					mask._u8[i] = b & 0x1f;
				}
			}

			if (cv || llvm::isa<llvm::ConstantAggregateZero>(c))
			{
				result.value = llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u8*>(&mask), 16));
				result.value = m_ir->CreateZExt(result.value, get_type<u32[16]>());
				result.value = m_ir->CreateShuffleVector(data0, data1, result.value);
				return result;
			}
		}

		result.value = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_avx512_vpermi2var_qi_128), {data0, index, data1});
		return result;
	}

	// Emulate the behavior of VPERM2B by using a 256 bit wide VPERMB
	template <typename T1, typename T2, typename T3>
	value_t<u8[16]> vperm2b256to128(T1 a, T2 b, T3 c)
	{
		value_t<u8[16]> result;

		const auto data0 = a.eval(m_ir);
		const auto data1 = b.eval(m_ir);
		const auto index = c.eval(m_ir);

		// May be slower than non constant path?
		if (auto c = llvm::dyn_cast<llvm::Constant>(index))
		{
			// Convert VPERM2B index back to LLVM vector shuffle mask
			v128 mask{};

			const auto cv = llvm::dyn_cast<llvm::ConstantDataVector>(c);

			if (cv)
			{
				for (u32 i = 0; i < 16; i++)
				{
					const u64 b = cv->getElementAsInteger(i);
					mask._u8[i] = b & 0x1f;
				}
			}

			if (cv || llvm::isa<llvm::ConstantAggregateZero>(c))
			{
				result.value = llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u8*>(&mask), 16));
				result.value = m_ir->CreateZExt(result.value, get_type<u32[16]>());
				result.value = m_ir->CreateShuffleVector(data0, data1, result.value);
				return result;
			}
		}

		const auto zeroes = llvm::ConstantAggregateZero::get(get_type<u8[16]>());
		const auto zeroes32 = llvm::ConstantAggregateZero::get(get_type<u8[32]>());

		value_t<u8[32]> intermediate;
		value_t<u8[32]> shuffle;
		value_t<u8[32]> shuffleindex;

		u8 mask32[32] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
		u8 mask16[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};

		// insert the second source operand into the same vector as the first source operand and expand to 256 bit width
		shuffle.value = llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u8*>(&mask32), 32));
		shuffle.value = m_ir->CreateZExt(shuffle.value, get_type<u32[32]>());
		intermediate.value = m_ir->CreateShuffleVector(data0, data1, shuffle.value);

		// expand the shuffle index to 256 bits with zeroes
		shuffleindex.value = m_ir->CreateShuffleVector(index, zeroes, shuffle.value);

		// permute
		intermediate.value = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_avx512_permvar_qi_256), {intermediate.value, shuffleindex.value});

		// convert the 256 bit vector back to 128 bits
		result.value = llvm::ConstantDataVector::get(m_context, llvm::ArrayRef(reinterpret_cast<const u8*>(&mask16), 16));
		result.value = m_ir->CreateZExt(result.value, get_type<u32[16]>());
		result.value = m_ir->CreateShuffleVector(intermediate.value, zeroes32, result.value);
		return result;
	}

	template <typename T1, typename T2, typename T3>
	value_t<f32[4]> vfixupimmps(T1 a, T2 b, T3 c, u8 d, u8 e)
	{
		value_t<f32[4]> result;

		const auto data0 = a.eval(m_ir);
		const auto data1 = b.eval(m_ir);
		const auto data2 = c.eval(m_ir);
		const auto immediate = (llvm_const_int<u32>{d});
		const auto imm32 = immediate.eval(m_ir);
		const auto immediate2 = (llvm_const_int<u8>{e});
		const auto imm8 = immediate2.eval(m_ir);
		result.value = m_ir->CreateCall(get_intrinsic(llvm::Intrinsic::x86_avx512_mask_fixupimm_ps_128), {data0, data1, data2, imm32, imm8});
		return result;
	}

	llvm::Value* load_const(llvm::GlobalVariable* g, llvm::Value* i, llvm::Type* type = nullptr)
	{
		return m_ir->CreateLoad(type ? type : g->getValueType(), m_ir->CreateGEP(g->getValueType(), g, {m_ir->getInt64(0), m_ir->CreateZExtOrTrunc(i, get_type<u64>())}));
	}

	template <typename T>
	llvm::Value* load_const(llvm::GlobalVariable* g, llvm::Value* i)
	{
		return load_const(g, i, get_type<T>());
	}

	template <typename T, typename I>
		requires requires(I& i, llvm::IRBuilder<>* ir) { i.eval(ir); }
	value_t<T> load_const(llvm::GlobalVariable* g, I i)
	{
		value_t<T> result;
		result.value = load_const<T>(g, i.eval(m_ir));
		return result;
	}

	template <typename T>
	llvm::GlobalVariable* make_local_variable(T initializing_value)
	{
		return new llvm::GlobalVariable(*m_module, get_type<T>(), false, llvm::GlobalVariable::PrivateLinkage, llvm::ConstantInt::get(get_type<T>(), initializing_value));
	}

	template <typename R = v128>
	std::pair<bool, R> get_const_vector(llvm::Value*, u32 pos, u32 = __builtin_LINE());

	template <typename T = v128>
	llvm::Constant* make_const_vector(T, llvm::Type*, u32 = __builtin_LINE());

	template <typename T>
	llvm::KnownBits get_known_bits(T a)
	{
		return llvm::computeKnownBits(a.eval(m_ir), m_module->getDataLayout());
	}

	template <typename T>
	llvm::KnownBits kbc(T value)
	{
		return llvm::KnownBits::makeConstant(llvm::APInt(sizeof(T) * 8, u64(value)));
	}

private:
	// Custom intrinsic table
	std::unordered_map<std::string_view, std::function<llvm::Value*(llvm::CallInst*)>> m_intrinsics;

public:
	// Call custom intrinsic by name
	template <typename RT, typename... Args>
	llvm::CallInst* _calli(std::string_view name, Args... args)
	{
		const auto type = llvm::FunctionType::get(get_type<RT>(), {args->getType()...}, false);
		const auto func = llvm::cast<llvm::Function>(m_module->getOrInsertFunction({name.data(), name.size()}, type).getCallee());
		return m_ir->CreateCall(func, {args...});
	}

	// Initialize custom intrinsic
	template <typename F>
	void register_intrinsic(std::string_view name, F replace_with)
	{
		if constexpr (std::is_same_v<std::invoke_result_t<F, llvm::CallInst*>, llvm::Value*>)
		{
			m_intrinsics.try_emplace(name, replace_with);
		}
		else
		{
			m_intrinsics.try_emplace(name, [=, this](llvm::CallInst* ci)
			{
				return replace_with(ci).eval(m_ir);
			});
		}
	}

	// Finalize processing
	void run_transforms(llvm::Function&);

	// Erase store instructions of provided
	void erase_stores(llvm::ArrayRef<llvm::Value*> args);

	template <typename... Args>
	void erase_stores(Args... args)
	{
		erase_stores({args.value...});
	}

	template <typename T, typename U>
	static auto pshufb(T&& a, U&& b)
	{
		return llvm_calli<u8[16], T, U>{"x86_pshufb", {std::forward<T>(a), std::forward<U>(b)}}.if_const([](llvm::Value* args[], llvm::IRBuilder<>* ir) -> llvm::Value*
		{
			const auto zeros = llvm::ConstantAggregateZero::get(llvm_value_t<u8[16]>::get_type(ir->getContext()));

			if (auto c = llvm::dyn_cast<llvm::Constant>(args[1]))
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
					llvm::Value* r = nullptr;
					r = llvm::ConstantDataVector::get(ir->getContext(), llvm::ArrayRef(reinterpret_cast<const u8*>(&mask), 16));
					r = ir->CreateZExt(r, llvm_value_t<u32[16]>::get_type(ir->getContext()));
					r = ir->CreateShuffleVector(args[0], zeros, r);
					return r;
				}
			}

			return nullptr;
		});
	}

	// (m << 3) >= 0 ? a : b
	template <typename T, typename U, typename V>
	static auto select_by_bit4(T&& m, U&& a, V&& b)
	{
		return llvm_calli<u8[16], T, U, V>{"any_select_by_bit4", {std::forward<T>(m), std::forward<U>(a), std::forward<V>(b)}};
	}

	template <typename T>
		requires std::is_same_v<llvm_common_t<T>, f32[4]>
	static auto fre(T&& a)
	{
#if defined(ARCH_X64)
		return llvm_calli<f32[4], T>{"llvm.x86.sse.rcp.ps", {std::forward<T>(a)}};
#elif defined(ARCH_ARM64)
		return llvm_calli<f32[4], T>{"llvm.aarch64.neon.frecpe.v4f32", {std::forward<T>(a)}};
#endif
	}

	template <typename T>
		requires std::is_same_v<llvm_common_t<T>, f32[4]>
	static auto frsqe(T&& a)
	{
#if defined(ARCH_X64)
		return llvm_calli<f32[4], T>{"llvm.x86.sse.rsqrt.ps", {std::forward<T>(a)}};
#elif defined(ARCH_ARM64)
		return llvm_calli<f32[4], T>{"llvm.aarch64.neon.frsqrte.v4f32", {std::forward<T>(a)}};
#endif
	}

	template <typename T, typename U>
		requires std::is_same_v<llvm_common_t<T, U>, f32[4]>
	static auto fmax(T&& a, U&& b)
	{
#if defined(ARCH_X64)
		return llvm_calli<f32[4], T, U>{"llvm.x86.sse.max.ps", {std::forward<T>(a), std::forward<U>(b)}};
#elif defined(ARCH_ARM64)
		return llvm_calli<f32[4], T, U>{"llvm.aarch64.neon.fmax.v4f32", {std::forward<T>(a), std::forward<U>(b)}};
#endif
	}

	template <typename T, typename U>
		requires std::is_same_v<llvm_common_t<T, U>, f32[4]>
	static auto fmin(T&& a, U&& b)
	{
#if defined(ARCH_X64)
		return llvm_calli<f32[4], T, U>{"llvm.x86.sse.min.ps", {std::forward<T>(a), std::forward<U>(b)}};
#elif defined(ARCH_ARM64)
		return llvm_calli<f32[4], T, U>{"llvm.aarch64.neon.fmin.v4f32", {std::forward<T>(a), std::forward<U>(b)}};
#endif
	}

	template <typename T, typename U>
		requires std::is_same_v<llvm_common_t<T, U>, u8[16]>
	static auto vdbpsadbw(T&& a, U&& b, u8 c)
	{
		return llvm_calli<u16[8], T, U, llvm_const_int<u32>>{"llvm.x86.avx512.dbpsadbw.128", {std::forward<T>(a), std::forward<U>(b), llvm_const_int<u32>{c}}};
	}

	template <typename T, typename U>
		requires std::is_same_v<llvm_common_t<T, U>, f32[4]>
	static auto vrangeps(T&& a, U&& b, u8 c, u8 d)
	{
		return llvm_calli<f32[4], T, U, llvm_const_int<u32>, T, llvm_const_int<u8>>{"llvm.x86.avx512.mask.range.ps.128", {std::forward<T>(a), std::forward<U>(b), llvm_const_int<u32>{c}, std::forward<T>(a), llvm_const_int<u8>{d}}};
	}
};

// Format llvm::SizeType
template <>
struct fmt_unveil<llvm::TypeSize>
{
	using type = usz;

	static inline usz get(const llvm::TypeSize& arg)
	{
		return arg;
	}
};

// Inline assembly wrappers.
// TODO: Move these to proper location and replace macros with templates
static inline
llvm::InlineAsm* compile_inline_asm(
	llvm::Type* returnType,
	llvm::ArrayRef<llvm::Type*> argTypes,
	const std::string& code,
	const std::string& constraints)
{
	const auto callSig = llvm::FunctionType::get(returnType, argTypes, false);
	return llvm::InlineAsm::get(callSig, code, constraints, true, false);
}

// Helper for ASM generation with dynamic number of arguments
static inline
llvm::CallInst* llvm_asm(
	llvm::IRBuilder<>* irb,
	const std::string& asm_,
	llvm::ArrayRef<llvm::Value*> args,
	const std::string& constraints,
	llvm::LLVMContext& context)
{
	llvm::ArrayRef<llvm::Type*> types_ref = std::nullopt;
	std::vector<llvm::Type*> types;
	types.reserve(args.size());

	if (!args.empty())
	{
		for (const auto& arg : args)
		{
			types.push_back(arg->getType());
		}
		types_ref = types;
	}

	auto return_type = llvm::Type::getVoidTy(context);
	auto callee = compile_inline_asm(return_type, types_ref, asm_, constraints);
	auto c = irb->CreateCall(callee, args);
	c->addFnAttr(llvm::Attribute::AlwaysInline);
	return c;
}

#define LLVM_ASM(asm_, args, constraints, irb, ctx)\
	llvm_asm(irb, asm_, args, constraints, ctx)

// Helper for ASM generation with 0 args
#define LLVM_ASM_VOID(asm_, irb, ctx)\
	llvm_asm(irb, asm_, {}, "", ctx)

#endif
