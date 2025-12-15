#pragma once

#include "../CPU/CPUThread.h"
#include "../CPU/Hypervisor.h"
#include "../Memory/vm_ptr.h"
#include "Utilities/lockless.h"
#include "Utilities/BitField.h"

#include "util/logs.hpp"
#include "util/v128.hpp"

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
	opd_call, // Execute function by provided rtoc and address (unlike lle_call, does not read memory)
	cia_call, // Execute from current CIA, mo GPR modification applied
	entry_call, // Load addr and rtoc from entry_func
	initialize, // ppu_initialize()
	sleep,
	reset_stack, // resets stack address
};

enum class ppu_join_status : u32
{
	joinable = 0,
	detached = 1,
	zombie = 2,
	exited = 3,
	max = 4, // Values above it indicate PPU id of joining thread
};

enum ppu_thread_status : u32
{
	PPU_THREAD_STATUS_IDLE,
	PPU_THREAD_STATUS_RUNNABLE,
	PPU_THREAD_STATUS_ONPROC,
	PPU_THREAD_STATUS_SLEEP,
	PPU_THREAD_STATUS_STOP,
	PPU_THREAD_STATUS_ZOMBIE,
	PPU_THREAD_STATUS_DELETED,
	PPU_THREAD_STATUS_UNKNOWN,
};

// Formatting helper
enum class ppu_syscall_code : u64
{
};

enum : u32
{
	ppu_stack_start_offset = 0x70,
};

// ppu function descriptor
struct ppu_func_opd_t
{
	be_t<u32> addr;
	be_t<u32> rtoc;
};

// ppu_thread constructor argument
struct ppu_thread_params
{
	vm::addr_t stack_addr;
	u32 stack_size;
	u32 tls_addr;
	ppu_func_opd_t entry;
	u64 arg0;
	u64 arg1;
};

struct cmd64
{
	u64 m_data = 0;

	constexpr cmd64() noexcept = default;

	struct pair_t
	{
		u32 arg1;
		u32 arg2;
	};

	template <typename T, typename T2 = std::common_type_t<T>>
	cmd64(const T& value)
		: m_data(std::bit_cast<u64, T2>(value))
	{
	}

	template <typename T1, typename T2>
	cmd64(const T1& arg1, const T2& arg2)
		: cmd64(pair_t{std::bit_cast<u32>(arg1), std::bit_cast<u32>(arg2)})
	{
	}

	explicit operator bool() const
	{
		return m_data != 0;
	}

	template <typename T>
	T as() const
	{
		return std::bit_cast<T>(m_data);
	}

	template <typename T>
	T arg1() const
	{
		return std::bit_cast<T>(std::bit_cast<pair_t>(m_data).arg1);
	}

	template <typename T>
	T arg2() const
	{
		return std::bit_cast<T>(std::bit_cast<pair_t>(m_data).arg2);
	}
};

enum class ppu_debugger_mode : u32
{
	_default,
	is_decimal,

	max_mode,
};

class ppu_thread : public cpu_thread
{
public:
	static const u32 id_base = 0x01000000; // TODO (used to determine thread type)
	static const u32 id_step = 1;
	static const u32 id_count = 100;
	static constexpr std::pair<u32, u32> id_invl_range = {12, 12};

	virtual void dump_regs(std::string&, std::any& custom_data) const override;
	virtual std::string dump_callstack() const override;
	virtual std::vector<std::pair<u32, u32>> dump_callstack_list() const override;
	virtual std::string dump_misc() const override;
	virtual void dump_all(std::string&) const override;
	virtual void cpu_task() override final;
	virtual void cpu_sleep() override;
	virtual void cpu_on_stop() override;
	virtual void cpu_wait(bs_t<cpu_flag> old) override;
	virtual ~ppu_thread() override;

	SAVESTATE_INIT_POS(3);

	ppu_thread(const ppu_thread_params&, std::string_view name, u32 prio, int detached = 0);
	ppu_thread(utils::serial& ar);
	ppu_thread(const ppu_thread&) = delete;
	ppu_thread& operator=(const ppu_thread&) = delete;
	bool savable() const;
	void serialize_common(utils::serial& ar);
	void save(utils::serial& ar);

	using cpu_thread::operator=;

	u64 gpr[32] = {}; // General-Purpose Registers
	f64 fpr[32] = {}; // Floating Point Registers
	v128 vr[32] = {}; // Vector Registers

	union alignas(16) cr_bits
	{
		u8 bits[32];
		u32 fields[8];

		u8& operator [](usz i)
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
				b = !!(value & (1u << 31));
				value <<= 1;
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
		ENABLE_BITWISE_SERIALIZATION;

		bool so{}; // Summary Overflow
		bool ov{}; // Overflow
		bool ca{}; // Carry
		u8 cnt{};  // 0..6
	}
	xer;

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

	// Sticky saturation bit
	v128 sat{};

	// Optimization: precomputed java-mode mask for handling denormals
	u32 jm_mask = 0x7f80'0000;

	u32 raddr{0}; // Reservation addr
	u64 rtime{0};
	alignas(64) std::byte rdata[128]{}; // Reservation data
	bool use_full_rdata{};
	u32 res_cached{0}; // Reservation "cached" addresss
	u32 res_notify{0};
	u64 res_notify_time{0};

	union ppu_prio_t
	{
		u64 all;
		bf_t<s64, 0, 13> prio; // Thread priority (0..3071) (firs 12-bits)
		bf_t<s64, 13, 50> order; // Thread enqueue order (last 52-bits)
		bf_t<u64, 63, 1> preserve_bit; // Preserve value for savestates
	};

	atomic_t<ppu_prio_t> prio{};
	const u32 stack_size; // Stack size
	const u32 stack_addr; // Stack address

	atomic_t<ppu_join_status> joiner; // Joining thread or status
	u32 hw_sleep_time = 0; // Very specific delay for hardware threads switching, see lv2_obj::awake_unlocked for more details

	lf_fifo<atomic_t<cmd64>, 127> cmd_queue; // Command queue for asynchronous operations.

	void cmd_push(cmd64);
	void cmd_list(std::initializer_list<cmd64>);
	void cmd_pop(u32 = 0);
	cmd64 cmd_wait(); // Empty command means caller must return, like true from cpu_thread::check_status().
	cmd64 cmd_get(u32 index) { return cmd_queue[cmd_queue.peek() + index].load(); }
	atomic_t<u32> cmd_notify = 0;

	alignas(64) const ppu_func_opd_t entry_func;
	u64 start_time{0}; // Sleep start timepoint
	u64 end_time{umax}; // Sleep end timepoint
	s32 cancel_sleep{0}; // Flag to cancel the next lv2_obj::sleep call (when equals 2)
	u64 syscall_args[8]{0}; // Last syscall arguments stored
	const char* current_function{}; // Current function name for diagnosis, optimized for speed.
	const char* last_function{}; // Sticky copy of current_function, is not cleared on function return
	const char* current_module{}; // Current module name, for savestates.

	const bool is_interrupt_thread; // True for interrupts-handler threads

	// Thread name
	atomic_ptr<std::string> ppu_tname;

	// Hypervisor context data
	rpcs3::hypervisor_context_t hv_ctx; // HV context for gate enter exit. Keep at a low struct offset.

	u64 last_ftsc = 0;
	u64 last_ftime = 0;
	u32 last_faddr = 0;
	u64 last_fail = 0;
	u64 last_succ = 0;
	u64 exec_bytes = 0; // Amount of "bytes" executed (4 for each instruction)

	u32 dbg_step_pc = 0;
	atomic_t<ppu_debugger_mode> debugger_mode{};

	struct call_history_t
	{
		std::vector<u32> data;
		u64 index = 0;
		u64 last_r1 = umax;
		u64 last_r2 = umax;
	} call_history;

	static constexpr u32 call_history_max_size = 4096;

	struct syscall_history_t
	{
		struct entry_t
		{
			u64 cia;
			const char* func_name;
			u64 error;
			std::array<u64, 4> args;
		};

		std::vector<entry_t> data;
		u64 index = 0;
		u32 count_debug_arguments;
	} syscall_history;

	static constexpr u32 syscall_history_max_size = 2048;

	struct hle_func_call_with_toc_info_t
	{
		u32 cia;
		u64 saved_lr;
		u64 saved_r2;
	};

	std::vector<hle_func_call_with_toc_info_t> hle_func_calls_with_toc_info;

	// For named_thread ctor
	const struct thread_name_t
	{
		const ppu_thread* _this;

		operator std::string() const;
	} thread_name{ this };

	// For savestates
	bool stop_flag_removal_protection = false; // If set, Emulator::Run won't remove stop flag
	bool loaded_from_savestate = false; // Indicates the thread had just started straight from savestate load
	std::shared_ptr<utils::serial> optional_savestate_state;
	bool interrupt_thread_executing = false;

	ppu_thread* next_cpu{}; // LV2 sleep queues' node link
	ppu_thread* next_ppu{}; // LV2 PPU running queue's node link
	bool ack_suspend = false;

	be_t<u64>* get_stack_arg(s32 i, u64 align = alignof(u64));
	void exec_task();
	void fast_call(u32 addr, u64 rtoc, bool is_thread_entry = false);

	static std::pair<vm::addr_t, u32> stack_push(u32 size, u32 align_v);
	static void stack_pop_verbose(u32 addr, u32 size) noexcept;
};

static_assert(ppu_join_status::max <= ppu_join_status{ppu_thread::id_base});

template <typename T>
struct ppu_gpr_cast_impl
{
	static_assert(!sizeof(T), "Invalid type for ppu_gpr_cast<>");
};

template <typename T>
	requires std::is_integral_v<T> || std::is_enum_v<T>
struct ppu_gpr_cast_impl<T>
{
	static_assert(sizeof(T) <= 8, "Too big integral type for ppu_gpr_cast<>()");
	static_assert(std::is_same_v<std::decay_t<T>, bool> == false, "bool type is deprecated in ppu_gpr_cast<>(), use b8 instead");

	static inline u64 to(const T& value)
	{
		return static_cast<u64>(value);
	}

	static inline T from(const u64 reg)
	{
		return static_cast<T>(reg);
	}
};

template <>
struct ppu_gpr_cast_impl<b8>
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

template <typename T, typename AT>
struct ppu_gpr_cast_impl<vm::_ptr_base<T, AT>>
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

template <typename T, typename AT>
struct ppu_gpr_cast_impl<vm::_ref_base<T, AT>>
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
struct ppu_gpr_cast_impl<vm::null_t>
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
