#pragma once

#include "Emu/CPU/CPUThread.h"
#include "Emu/Memory/vm.h"

enum ARMv7InstructionSet
{
	ARM,
	Thumb,
	Jazelle,
	ThumbEE
};

class ARMv7Thread final : public cpu_thread
{
public:
	virtual std::string get_name() const override;
	virtual std::string dump() const override;
	virtual void cpu_init() override;
	virtual void cpu_task() override;
	virtual void cpu_task_main();
	virtual ~ARMv7Thread() override;

	ARMv7Thread(const std::string& name);

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

			union
			{
				struct
				{
					u32 reserved0 : 16;
					u32 GE : 4;
					u32 reserved1 : 4;
					u32 dummy : 3;
					u32 Q : 1; // Set to 1 if an SSAT or USAT instruction changes (saturates) the input value for the signed or unsigned range of the result
					u32 V : 1; // Overflow condition code flag
					u32 C : 1; // Carry condition code flag
					u32 Z : 1; // Zero condition code flag
					u32 N : 1; // Negative condition code flag
				};

				u32 APSR;

			} APSR;
		};

		struct
		{
			u64 GPR_D[8];
		};
	};

	union
	{
		struct
		{
			u32 dummy : 24;
			u32 exception : 8;
		};

		u32 IPSR;

	} IPSR;

	ARMv7InstructionSet ISET;

	union
	{
		struct
		{
			u8 shift_state : 5;
			u8 cond_base : 3;
		};

		struct
		{
			u8 check_state : 4;
			u8 condition : 4;
		};

		u8 IT;

		u32 advance()
		{
			// 0xf is "always true" and indicates that this instruction was not in IT block.
			// 0xe is "always true" too  and represents the AL condition of IT block.
			// This makes a significant difference in some instructions.
			const u32 res = check_state ? condition : 0xf;

			shift_state <<= 1;
			if (!check_state)
			{
				IT = 0; // clear
			}

			return res;
		}

		operator bool() const
		{
			return check_state != 0;
		}

	} ITSTATE;

	u32 TLS = 0;

	struct perf_counter
	{
		u32 event;
		u32 value;
	};

	std::array<perf_counter, 6> counters{};

	u32 PC = 0;
	u32 prio = -1;
	u32 stack_addr = 0;
	u32 stack_size = 0;

	const std::string m_name;

	std::function<void(ARMv7Thread&)> custom_task;

	const char* last_function = nullptr;

	void write_pc(u32 value, u32 size)
	{
		ISET = value & 1 ? Thumb : ARM;
		PC = (value & ~1) - size;
	}

	u32 read_pc()
	{
		return ISET == ARM ? PC + 8 : PC + 4;
	}

	u32 get_stack_arg(u32 pos)
	{
		return vm::psv::read32(SP + sizeof(u32) * (pos - 5));
	}

	void write_gpr(u32 n, u32 value, u32 size)
	{
		EXPECTS(n < 16);

		if (n < 15)
		{
			GPR[n] = value;
		}
		else
		{
			write_pc(value, size);
		}
	}

	u32 read_gpr(u32 n)
	{
		EXPECTS(n < 16);

		if (n < 15)
		{
			return GPR[n];
		}

		return read_pc();
	}

	// function for processing va_args in printf-like functions
	u32 get_next_gpr_arg(u32& g_count)
	{
		if (g_count < 4)
		{
			return GPR[g_count++];
		}
		else
		{
			return get_stack_arg(g_count++);
		}
	}

	void fast_call(u32 addr);
};

template<typename T, typename = void>
struct arm_gpr_cast_impl
{
	static_assert(!sizeof(T), "Invalid type for arm_gpr_cast<>");
};

template<typename T>
struct arm_gpr_cast_impl<T, std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
{
	static_assert(sizeof(T) <= 4, "Too big integral type for arm_gpr_cast<>()");
	static_assert(std::is_same<CV T, CV bool>::value == false, "bool type is deprecated in arm_gpr_cast<>(), use b8 instead");

	static inline u32 to(const T& value)
	{
		return static_cast<u32>(value);
	}

	static inline T from(const u32 reg)
	{
		return static_cast<T>(reg);
	}
};

template<>
struct arm_gpr_cast_impl<b8, void>
{
	static inline u32 to(const b8& value)
	{
		return value;
	}

	static inline b8 from(const u32 reg)
	{
		return reg != 0;
	}
};

template<typename T, typename AT>
struct arm_gpr_cast_impl<vm::_ptr_base<T, AT>, void>
{
	static inline u32 to(const vm::_ptr_base<T, AT>& value)
	{
		return arm_gpr_cast_impl<AT>::to(value.addr());
	}

	static inline vm::_ptr_base<T, AT> from(const u32 reg)
	{
		return vm::cast(arm_gpr_cast_impl<AT>::from(reg));
	}
};

template<typename T, typename AT>
struct arm_gpr_cast_impl<vm::_ref_base<T, AT>, void>
{
	static inline u32 to(const vm::_ref_base<T, AT>& value)
	{
		return arm_gpr_cast_impl<AT>::to(value.addr());
	}

	static inline vm::_ref_base<T, AT> from(const u32 reg)
	{
		return vm::cast(arm_gpr_cast_impl<AT>::from(reg));
	}
};

template<typename To = u32, typename From>
inline To arm_gpr_cast(const From& value)
{
	return arm_gpr_cast_impl<To>::from(arm_gpr_cast_impl<From>::to(value));
}
