#pragma once

#include "Common.h"
#include "../CPU/CPUThread.h"
#include "../Memory/vm.h"

class PPUThread final : public cpu_thread
{
public:
	virtual std::string get_name() const override;
	virtual std::string dump() const override;
	virtual void cpu_init() override;
	virtual void cpu_task() override;
	virtual void cpu_task_main();
	virtual bool handle_interrupt() override;
	virtual ~PPUThread() override;

	PPUThread(const std::string& name);

	u64 GPR[32]{}; // General-Purpose Registers
	f64 FPR[32]{}; // Floating Point Registers
	v128 VR[32]{}; // Vector Registers
	alignas(16) bool CR[32]{}; // Condition Registers
	bool SO{}; // XER: Summary overflow
	bool OV{}; // XER: Overflow
	bool CA{}; // XER: Carry
	u8 XCNT{}; // XER: 0..6

	/*
		Saturation. A sticky status bit indicating that some field in a saturating instruction saturated since the last
		time SAT was cleared. In other words when SAT = '1' it remains set to '1' until it is cleared to '0' by an
		mtvscr instruction.
		1	The vector saturate instruction implicitly sets when saturation has occurred on the results one of
			the vector instructions having saturate in its name:
			Move To VSCR (mtvscr)
			Vector Add Integer with Saturation (vaddubs, vadduhs, vadduws, vaddsbs, vaddshs,
			vaddsws)
			Vector Subtract Integer with Saturation (vsububs, vsubuhs, vsubuws, vsubsbs, vsubshs,
			vsubsws)
			Vector Multiply-Add Integer with Saturation (vmhaddshs, vmhraddshs)
			Vector Multiply-Sum with Saturation (vmsumuhs, vmsumshs, vsumsws)
			Vector Sum-Across with Saturation (vsumsws, vsum2sws, vsum4sbs, vsum4shs,
			vsum4ubs)
			Vector Pack with Saturation (vpkuhus, vpkuwus, vpkshus, vpkswus, vpkshss, vpkswss)
			Vector Convert to Fixed-Point with Saturation (vctuxs, vctsxs)
		0	Indicates no saturation occurred; mtvscr can explicitly clear this bit.
	*/
	bool SAT{}; 

	/*
		Non-Java. A mode control bit that determines whether vector floating-point operations will be performed
		in a Java-IEEE-C9X-compliant mode or a possibly faster non-Java/non-IEEE mode.
		0	The Java-IEEE-C9X-compliant mode is selected. Denormalized values are handled as specified
			by Java, IEEE, and C9X standard.
		1	The non-Java/non-IEEE-compliant mode is selected. If an element in a source vector register
			contains a denormalized value, the value '0' is used instead. If an instruction causes an underflow
			exception, the corresponding element in the target VR is cleared to '0'. In both cases, the '0'
			has the same sign as the denormalized or underflowing value.
	*/
	bool NJ{ true };

	bool FL{}; // FPSCR.FPCC.FL
	bool FG{}; // FPSCR.FPCC.FG
	bool FE{}; // FPSCR.FPCC.FE
	bool FU{}; // FPSCR.FPCC.FU

	u64 LR{}; // Link Register
	u64 CTR{}; // Counter Register
	u32 VRSAVE{};

	u32 pc = 0;
	u32 prio = -1; // Thread priority
	u32 stack_addr = 0; // Stack address
	u32 stack_size = 0; // Stack size
	bool is_joinable = true;
	bool is_joining = false;

	const std::string m_name; // Thread name

	std::function<void(PPUThread&)> custom_task;

	// Function name can be stored here. Used to print the last called function.
	const char* last_function = nullptr;

	// When a thread has met an exception, this variable is used to retro propagate it through stack call.
	std::exception_ptr pending_exception;

	// Pack CR bits
	u32 GetCR() const
	{
		u32 result{};

		for (u32 bit : CR)
		{
			result = (result << 1) | bit;
		}

		return result;
	}

	// Unpack CR bits
	void SetCR(u32 value)
	{
		for (bool& b : CR)
		{
			b = (value & 0x1) != 0;
			value >>= 1;
		}
	}

	// Set CR field
	void SetCR(u32 field, bool le, bool gt, bool eq, bool so)
	{
		CR[field * 4 + 0] = le;
		CR[field * 4 + 1] = gt;
		CR[field * 4 + 2] = eq;
		CR[field * 4 + 3] = so;
	}

	// Set CR field for comparison
	template<typename T>
	void SetCR(u32 field, const T& a, const T& b)
	{
		SetCR(field, a < b, a > b, a == b, SO);
	}

	// Set overflow bit
	void SetOV(const bool set)
	{
		OV = set;
		SO |= set;
	}

	u64 get_next_arg(u32& g_count)
	{
		if (g_count < 8)
		{
			return GPR[g_count++ + 3];
		}
		else
		{
			return *get_stack_arg(++g_count);
		}
	}

	be_t<u64>* get_stack_arg(s32 i, u64 align = alignof(u64));
	void fast_call(u32 addr, u32 rtoc);
};

template<typename T, typename = void>
struct ppu_gpr_cast_impl
{
	static_assert(!sizeof(T), "Invalid type for ppu_gpr_cast<>");
};

template<typename T>
struct ppu_gpr_cast_impl<T, std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
{
	static_assert(sizeof(T) <= 8, "Too big integral type for ppu_gpr_cast<>()");
	static_assert(std::is_same<CV T, CV bool>::value == false, "bool type is deprecated in ppu_gpr_cast<>(), use b8 instead");

	static inline u64 to(const T& value)
	{
		return static_cast<u64>(value);
	}

	static inline T from(const u64 reg)
	{
		return static_cast<T>(reg);
	}
};

template<>
struct ppu_gpr_cast_impl<b8, void>
{
	static inline u64 to(const b8& value)
	{
		return value;
	}

	static inline b8 from(const u64 reg)
	{
		return static_cast<u32>(reg) != 0;
	}
};

template<typename T, typename AT>
struct ppu_gpr_cast_impl<vm::_ptr_base<T, AT>, void>
{
	static inline u64 to(const vm::_ptr_base<T, AT>& value)
	{
		return ppu_gpr_cast_impl<AT>::to(value.addr());
	}

	static inline vm::_ptr_base<T, AT> from(const u64 reg)
	{
		return vm::cast(ppu_gpr_cast_impl<AT>::from(reg));
	}
};

template<typename T, typename AT>
struct ppu_gpr_cast_impl<vm::_ref_base<T, AT>, void>
{
	static inline u64 to(const vm::_ref_base<T, AT>& value)
	{
		return ppu_gpr_cast_impl<AT>::to(value.addr());
	}

	static inline vm::_ref_base<T, AT> from(const u64 reg)
	{
		return vm::cast(ppu_gpr_cast_impl<AT>::from(reg));
	}
};

template<typename To = u64, typename From>
inline To ppu_gpr_cast(const From& value)
{
	return ppu_gpr_cast_impl<To>::from(ppu_gpr_cast_impl<From>::to(value));
}
