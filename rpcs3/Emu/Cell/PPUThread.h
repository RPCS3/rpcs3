#pragma once

#include "Common.h"
#include "../CPU/CPUThread.h"
#include "../Memory/vm.h"
#include "Utilities/lockless.h"

// Lightweight PPU command queue element
struct ppu_cmd : any64
{
	enum : u32
	{
		none = 0,

		opcode, // Execute PPU instruction from arg
		set_gpr, // Set gpr[arg] (+1 cmd)
		set_args, // Set general-purpose args (+arg cmd)
		lle_call, // Load addr and rtoc at *arg or *gpr[arg] and execute
		hle_call, // Execute function by index (arg)
	};

	struct pair_t
	{
		any32 arg1;
		any32 arg2;
	};

	ppu_cmd() = default;

	template<typename T>
	ppu_cmd(const T& value)
		: any64(value)
	{
	}

	template<typename T1, typename T2>
	ppu_cmd(const T1& arg1, const T2& arg2)
		: any64(pair_t{arg1, arg2})
	{
	}

	explicit operator bool() const
	{
		return as<u64>() != 0;
	}

	template<typename T>
	decltype(auto) arg1()
	{
		return as<pair_t>().arg1.as<T>();
	}

	template<typename T>
	decltype(auto) arg1() const
	{
		return as<const pair_t>().arg1.as<const T>();
	}

	template<typename T>
	decltype(auto) arg2()
	{
		return as<pair_t>().arg2.as<T>();
	}

	template<typename T>
	decltype(auto) arg2() const
	{
		return as<const pair_t>().arg2.as<const T>();
	}
};

static_assert(sizeof(ppu_cmd) == 8 && std::is_pod<ppu_cmd>::value, "Incorrect ppu_cmd struct");

class ppu_thread : public cpu_thread
{
public:
	virtual std::string get_name() const override;
	virtual std::string dump() const override;
	virtual void cpu_init() override final {}
	virtual void cpu_task() override;
	virtual ~ppu_thread() override;

	ppu_thread(const std::string& name, u32 prio = 0, u32 stack = 0x10000);

	u64 gpr[32] = {}; // General-Purpose Registers
	f64 fpr[32] = {}; // Floating Point Registers
	v128 vr[32] = {}; // Vector Registers

	alignas(16) bool cr[32] = {}; // Condition Registers (abstract representation)

	// Pack CR bits
	u32 cr_pack() const
	{
		u32 result{};

		for (u32 bit : cr)
		{
			result = (result << 1) | bit;
		}

		return result;
	}

	// Unpack CR bits
	void cr_unpack(u32 value)
	{
		for (bool& b : cr)
		{
			b = (value & 0x1) != 0;
			value >>= 1;
		}
	}

	// Fixed-Point Exception Register (abstract representation)
	struct
	{
		bool so{}; // Summary Overflow
		bool ov{}; // Overflow
		bool ca{}; // Carry
		u8 cnt{};  // 0..6
	}
	xer;
	
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
	bool sat{};

	/*
		Non-Java. A mode control bit that determines whether vector floating-point operations will be performed
		in a Java-IEEE-C9X-compliant mode or a possibly faster non-Java/non-IEEE mode.
		0	The Java-IEEE-C9X-compliant mode is selected. Denormalized values are handled as specified
			by Java, IEEE, and C9X standard.
		1	The non-Java/non-IEEE-compliant mode is selected. If an element in a source vector register
			contains a denormalized value, the value '0' is used instead. If an instruction causes an underflow
			exception, the corresponding element in the target vr is cleared to '0'. In both cases, the '0'
			has the same sign as the denormalized or underflowing value.
	*/
	bool nj = true;

	struct // Floating-Point Status and Control Register (abstract representation)
	{
		bool fl{}; // FPCC.FL
		bool fg{}; // FPCC.FG
		bool fe{}; // FPCC.FE
		bool fu{}; // FPCC.FU
	}
	fpscr;

	u32 cia{}; // Current Instruction Address
	u64 lr{}; // Link Register
	u64 ctr{}; // Counter Register
	u32 vrsave{0xffffffff}; // VR Save Register (almost unused)

	u32 prio = 0; // Thread priority (0..3071)
	const u32 stack_size; // Stack size
	const u32 stack_addr; // Stack address
	bool is_joinable = true;
	bool is_joining = false;

	lf_fifo<atomic_t<ppu_cmd>, 255> cmd_queue; // Command queue for asynchronous operations.

	void cmd_push(ppu_cmd);
	void cmd_list(std::initializer_list<ppu_cmd>);
	void cmd_pop(u32 = 0);
	ppu_cmd cmd_wait(); // Empty command means caller must return, like true from cpu_thread::check_status().

	const char* last_function{}; // Last function name for diagnosis, optimized for speed.

	const std::string m_name; // Thread name

	u64 get_next_arg(u32& g_count)
	{
		if (g_count < 8)
		{
			return gpr[g_count++ + 3];
		}
		else
		{
			return *get_stack_arg(++g_count);
		}
	}

	be_t<u64>* get_stack_arg(s32 i, u64 align = alignof(u64));
	void exec_task();
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
