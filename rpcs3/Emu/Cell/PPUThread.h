#pragma once

#include "../CPU/CPUThread.h"
#include "../Memory/vm_ptr.h"
#include "Utilities/lockless.h"

LOG_CHANNEL(ppu_log, "PPU");

enum class ppu_cmd : u32
{
	null,

	opcode, // Execute PPU instruction from arg
	set_gpr, // Set gpr[arg] (+1 cmd)
	set_args, // Set general-purpose args (+arg cmd)
	lle_call, // Load addr and rtoc at *arg or *gpr[arg] and execute
	hle_call, // Execute function by index (arg)
	ptr_call, // Execute function by pointer
	initialize, // ppu_initialize()
	sleep,
	reset_stack, // resets stack address
};

// Formatting helper
enum class ppu_syscall_code : u64
{
};

// ppu_thread constructor argument
struct ppu_thread_params
{
	vm::addr_t stack_addr;
	u32 stack_size;
	u32 tls_addr;
	u32 entry;
	u64 arg0;
	u64 arg1;
};

class ppu_thread : public cpu_thread
{
public:
	static const u32 id_base = 0x01000000; // TODO (used to determine thread type)
	static const u32 id_step = 1;
	static const u32 id_count = 2048;

	static void on_cleanup(named_thread<ppu_thread>*);

	virtual std::string get_name() const override;
	virtual std::string dump() const override;
	virtual void cpu_task() override final;
	virtual void cpu_sleep() override;
	virtual void cpu_mem() override;
	virtual void cpu_unmem() override;
	virtual ~ppu_thread() override;

	ppu_thread(const ppu_thread_params&, std::string_view name, u32 prio, int detached = 0);

	u64 gpr[32] = {}; // General-Purpose Registers
	f64 fpr[32] = {}; // Floating Point Registers
	v128 vr[32] = {}; // Vector Registers

	union alignas(16) cr_bits
	{
		u8 bits[32];
		u32 fields[8];

		u8& operator [](std::size_t i)
		{
			return bits[i];
		}

		// Pack CR bits
		u32 pack() const
		{
			u32 result{};

			for (u32 bit : bits)
			{
				result <<= 1;
				result |= bit;
			}

			return result;
		}

		// Unpack CR bits
		void unpack(u32 value)
		{
			for (u8& b : bits)
			{
				b = value & 0x1;
				value >>= 1;
			}
		}
	};

	cr_bits cr{}; // Condition Registers (unpacked)

	// Floating-Point Status and Control Register (unpacked)
	union
	{
		struct
		{
			// TODO
			bool _start[16];
			bool fl; // FPCC.FL
			bool fg; // FPCC.FG
			bool fe; // FPCC.FE
			bool fu; // FPCC.FU
			bool _end[12];
		};

		u32 fields[8];
		cr_bits bits;
	}
	fpscr{};

	u64 lr{}; // Link Register
	u64 ctr{}; // Counter Register
	u32 vrsave{0xffffffff}; // VR Save Register
	u32 cia{}; // Current Instruction Address

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
	bool nj = false;

	u32 raddr{0}; // Reservation addr
	u64 rtime{0};
	u64 rdata{0}; // Reservation data

	atomic_t<s32> prio{0}; // Thread priority (0..3071)
	const u32 stack_size; // Stack size
	const u32 stack_addr; // Stack address

	atomic_t<u32> joiner{~0u}; // Joining thread (-1 if detached)

	lf_fifo<atomic_t<cmd64>, 127> cmd_queue; // Command queue for asynchronous operations.

	void cmd_push(cmd64);
	void cmd_list(std::initializer_list<cmd64>);
	void cmd_pop(u32 = 0);
	cmd64 cmd_wait(); // Empty command means caller must return, like true from cpu_thread::check_status().
	cmd64 cmd_get(u32 index) { return cmd_queue[cmd_queue.peek() + index].load(); }

	u64 start_time{0}; // Sleep start timepoint
	const char* current_function{}; // Current function name for diagnosis, optimized for speed.
	const char* last_function{}; // Sticky copy of current_function, is not cleared on function return

	lf_value<std::string> ppu_name; // Thread name

	be_t<u64>* get_stack_arg(s32 i, u64 align = alignof(u64));
	void exec_task();
	void fast_call(u32 addr, u32 rtoc);

	static u32 stack_push(u32 size, u32 align_v);
	static void stack_pop_verbose(u32 addr, u32 size) noexcept;
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
	static_assert(std::is_same<std::decay_t<T>, bool>::value == false, "bool type is deprecated in ppu_gpr_cast<>(), use b8 instead");

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

template<>
struct ppu_gpr_cast_impl<error_code, void>
{
	static inline u64 to(const error_code& code)
	{
		return code;
	}

	static inline error_code from(const u64 reg)
	{
		return not_an_error(reg);
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

template <>
struct ppu_gpr_cast_impl<vm::null_t, void>
{
	static inline u64 to(const vm::null_t& /*value*/)
	{
		return 0;
	}

	static inline vm::null_t from(const u64 /*reg*/)
	{
		return vm::null;
	}
};

template<typename To = u64, typename From>
inline To ppu_gpr_cast(const From& value)
{
	return ppu_gpr_cast_impl<To>::from(ppu_gpr_cast_impl<From>::to(value));
}
