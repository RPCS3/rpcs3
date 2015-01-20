#pragma once

class ARMv7Thread;

enum ARMv7InstructionSet
{
	ARM,
	Thumb,
	Jazelle,
	ThumbEE
};

union ARMv7Code
{
	struct
	{
		u16 code0;
		u16 code1;
	};

	u32 data;
};

struct ARMv7Context
{
	ARMv7Thread& thread;

	ARMv7Context(ARMv7Thread& thread) : thread(thread) {}

	void write_pc(u32 value);
	u32 read_pc();
	u32 get_stack_arg(u32 pos);

	union
	{
		u32 GPR[15];

		struct
		{
			u32 pad[13];

			union
			{
				u32 SP;

				struct { u16 SP_main, SP_process; };
			};

			u32 LR;
		};
	};

	union
	{
		struct
		{
			u32 N : 1; //Negative condition code flag
			u32 Z : 1; //Zero condition code flag
			u32 C : 1; //Carry condition code flag
			u32 V : 1; //Overflow condition code flag
			u32 Q : 1; //Set to 1 if an SSAT or USAT instruction changes (saturates) the input value for the signed or unsigned range of the result
		u32: 27;
		};

		u32 APSR;

	} APSR;

	union
	{
		struct
		{
		u32: 24;
			u32 exception : 8;
		};

		u32 IPSR;

	} IPSR;

	ARMv7InstructionSet ISET;

	union
	{
		struct
		{
			u8 cond : 3;
			u8 state : 5;
		};

		u8 IT;

		u32 advance()
		{
			const u32 res = (state & 0xf) ? (cond << 1 | state >> 4) : 0xe /* true */;

			state <<= 1;
			if ((state & 0xf) == 0) // if no d
			{
				IT = 0; // clear ITSTATE
			}

			return res;
		}

		operator bool() const
		{
			return (state & 0xf) != 0;
		}

	} ITSTATE;

	void write_gpr(u32 n, u32 value)
	{
		assert(n < 16);

		if (n < 15)
		{
			GPR[n] = value;
		}
		else
		{
			write_pc(value & ~1);
		}
	}

	u32 read_gpr(u32 n)
	{
		assert(n < 16);

		if (n < 15)
		{
			return GPR[n];
		}

		return read_pc();
	}
};

template<typename T, bool is_enum = std::is_enum<T>::value>
struct cast_armv7_gpr
{
	static_assert(is_enum, "Invalid type for cast_armv7_gpr");

	typedef typename std::underlying_type<T>::type underlying_type;

	__forceinline static u32 to_gpr(const T& value)
	{
		return cast_armv7_gpr<underlying_type>::to_gpr(static_cast<underlying_type>(value));
	}

	__forceinline static T from_gpr(const u32 reg)
	{
		return static_cast<T>(cast_armv7_gpr<underlying_type>::from_gpr(reg));
	}
};

template<>
struct cast_armv7_gpr<u8, false>
{
	__forceinline static u32 to_gpr(const u8& value)
	{
		return value;
	}

	__forceinline static u8 from_gpr(const u32 reg)
	{
		return static_cast<u8>(reg);
	}
};

template<>
struct cast_armv7_gpr<u16, false>
{
	__forceinline static u32 to_gpr(const u16& value)
	{
		return value;
	}

	__forceinline static u16 from_gpr(const u32 reg)
	{
		return static_cast<u16>(reg);
	}
};

template<>
struct cast_armv7_gpr<u32, false>
{
	__forceinline static u32 to_gpr(const u32& value)
	{
		return value;
	}

	__forceinline static u32 from_gpr(const u32 reg)
	{
		return reg;
	}
};

template<>
struct cast_armv7_gpr<s8, false>
{
	__forceinline static u32 to_gpr(const s8& value)
	{
		return value;
	}

	__forceinline static s8 from_gpr(const u32 reg)
	{
		return static_cast<s8>(reg);
	}
};

template<>
struct cast_armv7_gpr<s16, false>
{
	__forceinline static u32 to_gpr(const s16& value)
	{
		return value;
	}

	__forceinline static s16 from_gpr(const u32 reg)
	{
		return static_cast<s16>(reg);
	}
};

template<>
struct cast_armv7_gpr<s32, false>
{
	__forceinline static u32 to_gpr(const s32& value)
	{
		return value;
	}

	__forceinline static s32 from_gpr(const u32 reg)
	{
		return static_cast<s32>(reg);
	}
};

template<>
struct cast_armv7_gpr<bool, false>
{
	__forceinline static u32 to_gpr(const bool& value)
	{
		return value;
	}

	__forceinline static bool from_gpr(const u32 reg)
	{
		return reinterpret_cast<const bool&>(reg);
	}
};

template<typename T>
__forceinline u32 cast_to_armv7_gpr(const T& value)
{
	return cast_armv7_gpr<T>::to_gpr(value);
}

template<typename T>
__forceinline T cast_from_armv7_gpr(const u32 reg)
{
	return cast_armv7_gpr<T>::from_gpr(reg);
}

