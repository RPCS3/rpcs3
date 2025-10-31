#pragma once

#include "Emu/CPU/CPUThread.h"
#include "Emu/CPU/Hypervisor.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "Emu/Memory/vm.h"
#include "MFC.h"

#include "util/v128.hpp"
#include "util/logs.hpp"
#include "util/to_endian.hpp"

#include "Utilities/mutex.h"

#include "Loader/ELF.h"

#include <span>

LOG_CHANNEL(spu_log, "SPU");

struct lv2_event_queue;
struct lv2_spu_group;
struct lv2_int_tag;

namespace utils
{
	class shm;
}

// LUTs for SPU
extern const u32 spu_frest_fraction_lut[32];
extern const u32 spu_frest_exponent_lut[256];
extern const u32 spu_frsqest_fraction_lut[64];
extern const u32 spu_frsqest_exponent_lut[256];

// JIT Block
using spu_function_t = void(*)(spu_thread&, void*, u8*);

// SPU Channels
enum : u32
{
	SPU_RdEventStat     = 0,  // Read event status with mask applied
	SPU_WrEventMask     = 1,  // Write event mask
	SPU_WrEventAck      = 2,  // Write end of event processing
	SPU_RdSigNotify1    = 3,  // Signal notification 1
	SPU_RdSigNotify2    = 4,  // Signal notification 2
	SPU_WrDec           = 7,  // Write decrementer count
	SPU_RdDec           = 8,  // Read decrementer count
	SPU_RdEventMask     = 11, // Read event mask
	SPU_RdMachStat      = 13, // Read SPU run status
	SPU_WrSRR0          = 14, // Write SPU machine state save/restore register 0 (SRR0)
	SPU_RdSRR0          = 15, // Read SPU machine state save/restore register 0 (SRR0)
	SPU_WrOutMbox       = 28, // Write outbound mailbox contents
	SPU_RdInMbox        = 29, // Read inbound mailbox contents
	SPU_WrOutIntrMbox   = 30, // Write outbound interrupt mailbox contents (interrupting PPU)
	SPU_Set_Bkmk_Tag    = 69, // Causes an event that can be logged in the performance monitor logic if enabled in the SPU
	SPU_PM_Start_Ev     = 70, // Starts the performance monitor event if enabled
	SPU_PM_Stop_Ev      = 71, // Stops the performance monitor event if enabled
};

// MFC Channels
enum : u32
{
	MFC_WrMSSyncReq     = 9,  // Write multisource synchronization request
	MFC_RdTagMask       = 12, // Read tag mask
	MFC_LSA             = 16, // Write local memory address command parameter
	MFC_EAH             = 17, // Write high order DMA effective address command parameter
	MFC_EAL             = 18, // Write low order DMA effective address command parameter
	MFC_Size            = 19, // Write DMA transfer size command parameter
	MFC_TagID           = 20, // Write tag identifier command parameter
	MFC_Cmd             = 21, // Write and enqueue DMA command with associated class ID
	MFC_WrTagMask       = 22, // Write tag mask
	MFC_WrTagUpdate     = 23, // Write request for conditional or unconditional tag status update
	MFC_RdTagStat       = 24, // Read tag status with mask applied
	MFC_RdListStallStat = 25, // Read DMA list stall-and-notify status
	MFC_WrListStallAck  = 26, // Write DMA list stall-and-notify acknowledge
	MFC_RdAtomicStat    = 27, // Read completion status of last completed immediate MFC atomic update command
};

// SPU Events
enum : u32
{
	SPU_EVENT_MS = 0x1000, // Multisource Synchronization event
	SPU_EVENT_A  = 0x800,  // Privileged Attention event
	SPU_EVENT_LR = 0x400,  // Lock Line Reservation Lost event
	SPU_EVENT_S1 = 0x200,  // Signal Notification Register 1 available
	SPU_EVENT_S2 = 0x100,  // Signal Notification Register 2 available
	SPU_EVENT_LE = 0x80,   // SPU Outbound Mailbox available
	SPU_EVENT_ME = 0x40,   // SPU Outbound Interrupt Mailbox available
	SPU_EVENT_TM = 0x20,   // SPU Decrementer became negative (?)
	SPU_EVENT_MB = 0x10,   // SPU Inbound mailbox available
	SPU_EVENT_QV = 0x8,    // MFC SPU Command Queue available
	SPU_EVENT_SN = 0x2,    // MFC List Command stall-and-notify event
	SPU_EVENT_TG = 0x1,    // MFC Tag Group status update event

	SPU_EVENT_IMPLEMENTED  = SPU_EVENT_LR | SPU_EVENT_TM | SPU_EVENT_SN | SPU_EVENT_S1 | SPU_EVENT_S2, // Mask of implemented events
	SPU_EVENT_INTR_IMPLEMENTED = SPU_EVENT_SN,
	SPU_EVENT_INTR_BUSY_CHECK = SPU_EVENT_IMPLEMENTED & ~SPU_EVENT_INTR_IMPLEMENTED,
};

// SPU Class 0 Interrupts
enum : u64
{
	SPU_INT0_STAT_DMA_ALIGNMENT_INT   = (1ull << 0),
	SPU_INT0_STAT_INVALID_DMA_CMD_INT = (1ull << 1),
	SPU_INT0_STAT_SPU_ERROR_INT       = (1ull << 2),
};

// SPU Class 2 Interrupts
enum : u64
{
	SPU_INT2_STAT_MAILBOX_INT                  = (1ull << 0),
	SPU_INT2_STAT_SPU_STOP_AND_SIGNAL_INT      = (1ull << 1),
	SPU_INT2_STAT_SPU_HALT_OR_STEP_INT         = (1ull << 2),
	SPU_INT2_STAT_DMA_TAG_GROUP_COMPLETION_INT = (1ull << 3),
	SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT    = (1ull << 4),
};

enum : u32
{
	SPU_RUNCNTL_STOP_REQUEST = 0,
	SPU_RUNCNTL_RUN_REQUEST  = 1,
};

// SPU Status Register bits (not accurate)
enum : u32
{
	SPU_STATUS_STOPPED             = 0x0,
	SPU_STATUS_RUNNING             = 0x1,
	SPU_STATUS_STOPPED_BY_STOP     = 0x2,
	SPU_STATUS_STOPPED_BY_HALT     = 0x4,
	SPU_STATUS_WAITING_FOR_CHANNEL = 0x8,
	SPU_STATUS_SINGLE_STEP         = 0x10,
	SPU_STATUS_IS_ISOLATED         = 0x80,
};

enum : s32
{
	SPU_LS_SIZE = 0x40000,
};

enum : u32
{
	SYS_SPU_THREAD_BASE_LOW  = 0xf0000000,
	SYS_SPU_THREAD_OFFSET    = 0x100000,
	SYS_SPU_THREAD_SNR1      = 0x5400c,
	SYS_SPU_THREAD_SNR2      = 0x5C00c,
};

enum
{
	MFC_LSA_offs = 0x3004,
	MFC_EAH_offs = 0x3008,
	MFC_EAL_offs = 0x300C,
	MFC_Size_Tag_offs = 0x3010,
	MFC_Class_CMD_offs = 0x3014,
	MFC_CMDStatus_offs = 0x3014,
	MFC_QStatus_offs = 0x3104,
	Prxy_QueryType_offs = 0x3204,
	Prxy_QueryMask_offs = 0x321C,
	Prxy_TagStatus_offs = 0x322C,
	SPU_Out_MBox_offs = 0x4004,
	SPU_In_MBox_offs = 0x400C,
	SPU_MBox_Status_offs = 0x4014,
	SPU_RunCntl_offs = 0x401C,
	SPU_Status_offs = 0x4024,
	SPU_NPC_offs = 0x4034,
	SPU_RdSigNotify1_offs = 0x1400C,
	SPU_RdSigNotify2_offs = 0x1C00C,
};

enum : u32
{
	RAW_SPU_BASE_ADDR   = 0xE0000000,
	RAW_SPU_OFFSET      = 0x00100000,
	RAW_SPU_LS_OFFSET   = 0x00000000,
	RAW_SPU_PROB_OFFSET = 0x00040000,

	SPU_FAKE_BASE_ADDR  = 0xE8000000,
};

struct spu_channel_op_state
{
	u8 old_count;
	u8 count;
	bool notify;
	bool op_done;
};

struct alignas(16) spu_channel
{
	// Low 32 bits contain value
	atomic_t<u64> data{};

	// Pending value to be inserted when it is possible in pop() or pop_wait()
	atomic_t<u64> jostling_value{};

public:
	static constexpr u32 off_wait  = 32;
	static constexpr u32 off_occupy = 32;
	static constexpr u32 off_count = 63;
	static constexpr u64 bit_wait  = 1ull << off_wait;
	static constexpr u64 bit_occupy = 1ull << off_occupy;
	static constexpr u64 bit_count = 1ull << off_count;

	// Returns true on success
	bool try_push(u32 value)
	{
		return data.fetch_op([value](u64& data)
		{
			if (!(data & bit_count)) [[likely]]
			{
				data = bit_count | value;
				return true;
			}

			return false;
		}).second;
	}

	// Push unconditionally, may require notification
	// Performing bitwise OR with previous value if specified, otherwise overwiting it
	// Returns old count and new count
	spu_channel_op_state push(u32 value, bool to_or = false, bool postpone_notify = false)
	{
		while (true)
		{
			const auto [old, pushed_to_data] = data.fetch_op([&](u64& data)
			{
				if (data & bit_occupy)
				{
					return false;
				}

				if (to_or)
				{
					data = bit_count | (static_cast<u32>(data) | value);
				}
				else
				{
					data = bit_count | value;
				}

				return true;
			});

			if (!pushed_to_data)
			{
				// Insert the pending value in special storage for waiting SPUs, leave no time in which the channel has data
				if (!jostling_value.compare_and_swap_test(bit_occupy, value))
				{
					// Other thread has inserted a value through jostling_value, retry
					continue;
				}
			}

			if (old & bit_wait)
			{
				// Turn off waiting bit manually (must succeed because waiting bit can only be resetted by the thread pushed to jostling_value)
				if (!this->data.bit_test_reset(off_wait))
				{
					// Could be fatal or at emulation stopping, to be checked by the caller
					return { (old & bit_count) == 0, 0, false, false };
				}

				if (!postpone_notify)
				{
					utils::bless<atomic_t<u32>>(&data)[1].notify_one();
				}
			}

			// Return true if count has changed from 0 to 1, this condition is considered satisfied even if we pushed a value directly to the special storage for waiting SPUs
			return { (old & bit_count) == 0, 1, (old & bit_wait) != 0, true };
		}
	}

	void notify()
	{
		utils::bless<atomic_t<u32>>(&data)[1].notify_one();
	}

	// Returns true on success
	bool try_pop(u32& out)
	{
		return data.fetch_op([&out](u64& data)
		{
			if (data & bit_count) [[likely]]
			{
				out = static_cast<u32>(data);
				data = 0;
				return true;
			}

			return false;
		}).second;
	}

	// Reading without modification
	bool try_read(u32& out) const
	{
		const u64 old = data.load();
		out = static_cast<u32>(old);

		if (old & bit_count) [[likely]]
		{
			return true;
		}

		return false;
	}

	// Pop unconditionally (loading last value), may require notification
	// If the SPU tries to insert a value, do it instead the SPU
	u32 pop()
	{
		// Value is not cleared and may be read again
		constexpr u64 mask = bit_count | bit_occupy;

		const u64 old = data.fetch_op([&](u64& data)
		{
			if ((data & mask) == mask)
			{
				// Insert the pending value, leave no time in which the channel has no data
				data = bit_count | static_cast<u32>(jostling_value);
				return;
			}

			data &= ~(mask | bit_wait);
		});

		if (old & bit_wait)
		{
			utils::bless<atomic_t<u32>>(&data)[1].notify_one();
		}

		return static_cast<u32>(old);
	}

	// Waiting for channel pop state availability, actually popping if specified
	s64 pop_wait(cpu_thread& spu, bool pop = true);

	// Waiting for channel push state availability, actually pushing if specified
	bool push_wait(cpu_thread& spu, u32 value, bool push = true);

	void set_value(u32 value, bool count = true)
	{
		data.release(u64{count} << off_count | value);
	}

	u32 get_value() const
	{
		return static_cast<u32>(data);
	}

	u32 get_count() const
	{
		return (data & bit_count) ? 1 : 0;
	}
};

struct spu_channel_4_t
{
	struct alignas(16) sync_var_t
	{
		u8 waiting;
		u8 count;
		u16 value3_inval;
		u32 value0;
		u32 value1;
		u32 value2;
	};

	atomic_t<sync_var_t> values;
	atomic_t<u64> jostling_value;
	atomic_t<u32> value3;

	static constexpr u32 off_wait  = 0;
	static constexpr u32 off_occupy = 7;
	static constexpr u64 bit_wait  = 1ull << off_wait;
	static constexpr u64 bit_occupy = 1ull << off_occupy;
	static constexpr u64 jostling_flag = 1ull << 63;

	void clear()
	{
		values.release({});
		jostling_value.release(0);
		value3.release(0);
	}

	// push unconditionally (overwriting latest value), returns true if needs signaling
	// returning if could be aborted (operation failed unexpectedly)
	spu_channel_op_state push(u32 value, bool postpone_notify = false);

	void notify()
	{
		utils::bless<atomic_t<u32>>(&values)[0].notify_one();
	}

	// returns non-zero value on success: queue size before removal
	uint try_pop(u32& out)
	{
		return values.atomic_op([&](sync_var_t& data)
		{
			const uint result = data.count;

			if (result != 0)
			{
				data.waiting = 0;
				data.count--;
				out = data.value0;

				data.value0 = data.value1;
				data.value1 = data.value2;
				data.value2 = this->value3;
			}

			return result;
		});
	}

	// Returns [previous count, value] (if aborted 0 count is returned)
	std::pair<u32, u32> pop_wait(cpu_thread& spu, bool pop_value = true);

	// returns current queue size without modification
	uint try_read(u32 (&out)[4]) const
	{
		const sync_var_t data = values.load();
		const uint result = data.count;

		if (result != 0)
		{
			out[0] = data.value0;
			out[1] = data.value1;
			out[2] = data.value2;
			out[3] = value3;
		}

		return result;
	}

	u32 get_count() const
	{
		return atomic_storage<u8>::load(std::as_const(values).raw().count);
	}

	void set_values(u32 count, u32 value0, u32 value1 = 0, u32 value2 = 0, u32 value3 = 0)
	{
		this->values.raw() = { 0, static_cast<u8>(count), {}, value0, value1, value2 };
		this->value3 = value3;
	}
};

struct spu_int_ctrl_t
{
	atomic_t<u64> mask;
	atomic_t<u64> stat;

	shared_ptr<struct lv2_int_tag> tag;

	void set(u64 ints);

	void clear(u64 ints)
	{
		stat &= ~ints;
	}

	void clear()
	{
		mask.release(0);
		stat.release(0);
		tag.reset();
	}
};

struct spu_imm_table_t
{
	v128 sldq_pshufb[32]; // table for SHLQBYBI, SHLQBY, SHLQBYI instructions
	v128 srdq_pshufb[32]; // table for ROTQMBYBI, ROTQMBY, ROTQMBYI instructions
	v128 rldq_pshufb[16]; // table for ROTQBYBI, ROTQBY, ROTQBYI instructions

	class scale_table_t
	{
		std::array<v128, 155 + 174> m_data;

	public:
		scale_table_t();

		FORCE_INLINE const v128& operator [](s32 scale) const
		{
			return m_data[scale + 155];
		}
	}
	const scale;

	spu_imm_table_t();
};

extern const spu_imm_table_t g_spu_imm;

enum FPSCR_EX
{
	//Single-precision exceptions
	FPSCR_SOVF = 1 << 2,    //Overflow
	FPSCR_SUNF = 1 << 1,    //Underflow
	FPSCR_SDIFF = 1 << 0,   //Different (could be IEEE non-compliant)
	//Double-precision exceptions
	FPSCR_DOVF = 1 << 13,   //Overflow
	FPSCR_DUNF = 1 << 12,   //Underflow
	FPSCR_DINX = 1 << 11,   //Inexact
	FPSCR_DINV = 1 << 10,   //Invalid operation
	FPSCR_DNAN = 1 << 9,    //NaN
	FPSCR_DDENORM = 1 << 8, //Denormal
};

//Is 128 bits, but bits 0-19, 24-28, 32-49, 56-60, 64-81, 88-92, 96-115, 120-124 are unused
class SPU_FPSCR
{
public:
	u32 _u32[4]{};

	SPU_FPSCR() {}

	std::string ToString() const
	{
		return fmt::format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);
	}

	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}

	//slice -> 0 - 1 (double-precision slice index)
	//NOTE: slices follow v128 indexing, i.e. slice 0 is RIGHT end of register!
	//roundTo -> FPSCR_RN_*
	void setSliceRounding(u8 slice, u8 roundTo)
	{
		int shift = 8 + 2*slice;
		//rounding is located in the left end of the FPSCR
		this->_u32[3] = (this->_u32[3] & ~(3 << shift)) | (roundTo << shift);
	}

	//Slice 0 or 1
	u8 checkSliceRounding(u8 slice) const
	{
		switch(slice)
		{
		case 0:
			return this->_u32[3] >> 8 & 0x3;

		case 1:
			return this->_u32[3] >> 10 & 0x3;

		default:
			fmt::throw_exception("Unexpected slice value (%d)", slice);
		}
	}

	//Single-precision exception flags (all 4 slices)
	//slice -> slice number (0-3)
	//exception: FPSCR_S* bitmask
	void setSinglePrecisionExceptionFlags(u8 slice, u32 exceptions)
	{
		_u32[slice] |= exceptions;
	}

	//Single-precision divide-by-zero flags (all 4 slices)
	//slice -> slice number (0-3)
	void setDivideByZeroFlag(u8 slice)
	{
		_u32[0] |= 1 << (8 + slice);
	}

	//Double-precision exception flags
	//slice -> slice number (0-1)
	//exception: FPSCR_D* bitmask
	void setDoublePrecisionExceptionFlags(u8 slice, u32 exceptions)
	{
		_u32[1 + slice] |= exceptions;
	}

	// Write the FPSCR
	void Write(const v128& r)
	{
		_u32[3] = r._u32[3] & 0x00000F07;
		_u32[2] = r._u32[2] & 0x00003F07;
		_u32[1] = r._u32[1] & 0x00003F07;
		_u32[0] = r._u32[0] & 0x00000F07;
	}

	// Read the FPSCR
	void Read(v128& r) const
	{
		r._u32[3] = _u32[3];
		r._u32[2] = _u32[2];
		r._u32[1] = _u32[1];
		r._u32[0] = _u32[0];
	}
};

enum class spu_type : u32
{
	threaded,
	raw,
	isolated,
};

struct spu_memory_segment_dump_data
{
	u32 ls_addr;
	const u8* src_addr;
	u32 segment_size;
	u32 flags = umax;
};

enum class spu_debugger_mode : u32
{
	_default,
	is_float,
	is_decimal,

	max_mode,
};

enum class spu_block_hash : u64 {};

class spu_thread : public cpu_thread
{
public:
	virtual void dump_regs(std::string&, std::any& custom_data) const override;
	virtual std::string dump_callstack() const override;
	virtual std::vector<std::pair<u32, u32>> dump_callstack_list() const override;
	virtual std::string dump_misc() const override;
	virtual void cpu_task() override final;
	virtual void cpu_on_stop() override;
	virtual void cpu_return() override;
	virtual void cpu_work() override;
	virtual ~spu_thread() override;
	void cleanup();
	void cpu_init();
	void init_spu_decoder();

	static const u32 id_base = 0x02000000; // TODO (used to determine thread type)
	static const u32 id_step = 1;
	static const u32 id_count = (0xFFFC0000 - SPU_FAKE_BASE_ADDR) / SPU_LS_SIZE;

	spu_thread(lv2_spu_group* group, u32 index, std::string_view name, u32 lv2_id, bool is_isolated = false, u32 option = 0);

	spu_thread(const spu_thread&) = delete;
	spu_thread& operator=(const spu_thread&) = delete;

	using cpu_thread::operator=;

	SAVESTATE_INIT_POS(5);

	spu_thread(utils::serial& ar, lv2_spu_group* group = nullptr);
	void serialize_common(utils::serial& ar);
	void save(utils::serial& ar);
	bool savable() const { return get_type() != spu_type::threaded; } // Threaded SPUs are saved as part of the SPU group

	u32 pc = 0;
	u32 dbg_step_pc = 0;

	// May be used internally by recompilers.
	u32 base_pc = 0;

	// May be used by recompilers.
	u8* memory_base_addr = vm::g_base_addr;
	u8* memory_sudo_addr = vm::g_sudo_addr;
	u8* reserv_base_addr = vm::g_reservations;

	// General-Purpose Registers
	std::array<v128, 128> gpr{};
	SPU_FPSCR fpscr{};

	// MFC command data
	spu_mfc_cmd ch_mfc_cmd{};

	// MFC command queue
	spu_mfc_cmd mfc_queue[16]{};
	u32 mfc_size = 0;
	u32 mfc_barrier = -1;
	u32 mfc_fence = -1;

	// Timestamp of the first postponed command (transfers shuffling related)
	u64 mfc_last_timestamp = 0;

	// MFC proxy command data
	spu_mfc_cmd mfc_prxy_cmd{};
	shared_mutex mfc_prxy_mtx;
	atomic_t<u32> mfc_prxy_mask = 0;

	// Tracks writes to MFC proxy command data
	union
	{
		u8 all;
		bf_t<u8, 0, 1> lsa;
		bf_t<u8, 1, 1> eal;
		bf_t<u8, 2, 1> eah;
		bf_t<u8, 3, 1> tag_size;
		bf_t<u8, 4, 1> cmd;
	} mfc_prxy_write_state{};

	// Reservation Data
	u64 rtime = 0;
	alignas(64) std::byte rdata[128]{};
	u32 raddr = 0;
	const decltype(rdata)* resrv_mem{};

	// Range Lock pointer
	atomic_t<u64, 64>* range_lock{};

	u32 srr0 = 0;
	u32 ch_tag_upd = 0;
	u32 ch_tag_mask = 0;
	spu_channel ch_tag_stat;
	u32 ch_stall_mask = 0;
	spu_channel ch_stall_stat;
	spu_channel ch_atomic_stat;

	spu_channel_4_t ch_in_mbox{};

	spu_channel ch_out_mbox;
	spu_channel ch_out_intr_mbox;

	u64 snr_config = 0; // SPU SNR Config Register

	spu_channel ch_snr1{}; // SPU Signal Notification Register 1
	spu_channel ch_snr2{}; // SPU Signal Notification Register 2

	union ch_events_t
	{
		u64 all;
		bf_t<u64, 0, 16> events;
		bf_t<u64, 16, 8> locks;
		bf_t<u64, 30, 1> waiting;
		bf_t<u64, 31, 1> count;
		bf_t<u64, 32, 32> mask;
	};

	atomic_t<ch_events_t> ch_events;
	bool interrupts_enabled = false;

	u64 ch_dec_start_timestamp = 0; // timestamp of writing decrementer value
	u32 ch_dec_value = 0; // written decrementer value
	bool is_dec_frozen = false;
	std::pair<u32, u32> read_dec() const; // Read decrementer

	atomic_t<u32> run_ctrl = 0; // SPU Run Control register (only provided to get latest data written)
	shared_mutex run_ctrl_mtx;

	struct alignas(8) status_npc_sync_var
	{
		u32 status; // SPU Status register
		u32 npc; // SPU Next Program Counter register
	};

	atomic_t<status_npc_sync_var> status_npc{};
	std::array<spu_int_ctrl_t, 3> int_ctrl{}; // SPU Class 0, 1, 2 Interrupt Management

	std::array<std::pair<u32, shared_ptr<lv2_event_queue>>, 32> spuq{}; // Event Queue Keys for SPU Thread
	shared_ptr<lv2_event_queue> spup[64]; // SPU Ports
	spu_channel exit_status{}; // Threaded SPU exit status (not a channel, but the interface fits)
	atomic_t<u32> last_exit_status; // Value to be written in exit_status after checking group termination
	lv2_spu_group* const group; // SPU Thread Group (access by the spu threads in the group only! From other threads obtain a shared pointer to group using group ID)
	const u32 index; // SPU index
	const spu_type thread_type;
	std::shared_ptr<utils::shm> shm; // SPU memory
	const std::add_pointer_t<u8> ls; // SPU LS pointer
	const u32 option; // sys_spu_thread_initialize option
	u32 lv2_id; // The actual id that is used by syscalls
	u32 spurs_addr = 0;
	bool spurs_waited = false;
	bool spurs_entered_wait = false;
	u64 spurs_wait_duration_last = 0;
	u64 spurs_average_task_duration = 0;
	u64 spurs_last_task_timestamp = 0;
	static constexpr u64 spurs_task_count_to_calculate = 10;

	spu_thread* next_cpu{}; // LV2 thread queues' node link

	// Thread name
	atomic_ptr<std::string> spu_tname;

	std::unique_ptr<class spu_recompiler_base> jit; // Recompiler instance

	u64 block_counter = 0;
	u64 block_recover = 0;
	u64 block_failure = 0;

	rpcs3::hypervisor_context_t hv_ctx; // NOTE: The offset within the class must be within the first 1MiB

	u64 ftx = 0; // Failed transactions
	u64 stx = 0; // Succeeded transactions (pure counters)

	u64 last_ftsc = 0;
	u64 last_ftime = 0;
	u32 last_faddr = 0;
	u64 last_fail = 0;
	u64 last_succ = 0;
	u64 last_gtsc = 0;
	u32 last_getllar = umax; // LS address of last GETLLAR (if matches current GETLLAR we can let the thread rest)
	u32 last_getllar_gpr1 = umax;
	u32 last_getllar_addr = umax;
	u32 last_getllar_lsa = umax;
	u32 getllar_spin_count = 0;
	u32 getllar_busy_waiting_switch = umax; // umax means the test needs evaluation, otherwise it's a boolean
	u64 getllar_evaluate_time = 0;

	u32 eventstat_raddr = 0;
	u32 eventstat_getllar = 0;
	u64 eventstat_block_counter = 0;
	u64 eventstat_spu_group_restart = 0;
	u64 eventstat_spin_count = 0;
	u64 eventstat_evaluate_time = 0;
	u32 eventstat_busy_waiting_switch = 0;

	std::vector<mfc_cmd_dump> mfc_history;
	u64 mfc_dump_idx = 0;
	static constexpr u32 max_mfc_dump_idx = 4096;

	bool in_cpu_work = false;
	bool allow_interrupts_in_cpu_work = false;
	u8 cpu_work_iteration_count = 0;

	std::array<v128, 0x4000> stack_mirror; // Return address information
	std::array<u32, 8> raddr_busy_wait_addr{}; // Return address information

	const char* current_func{}; // Current STOP or RDCH blocking function
	u64 start_time{}; // Starting time of STOP or RDCH bloking function
	bool unsavable = false; // Flag indicating whether saving the spu thread state is currently unsafe

	atomic_t<spu_debugger_mode> debugger_mode{};

	// PC-based breakpoint list
	std::array<atomic_t<u8>, SPU_LS_SIZE / 4 / 8> local_breakpoints{};
	atomic_t<bool> has_active_local_bps = false;
	u32 current_bp_pc = umax;
	bool stop_flag_removal_protection = false;

	std::array<std::array<u8, 4>, SPU_LS_SIZE / 128> getllar_wait_time{};
	std::array<std::array<u8, 16>, SPU_LS_SIZE / 128> eventstat_wait_time{};
 
	void push_snr(u32 number, u32 value);
	static void do_dma_transfer(spu_thread* _this, const spu_mfc_cmd& args, u8* ls);
	bool do_dma_check(const spu_mfc_cmd& args);
	bool do_list_transfer(spu_mfc_cmd& args);
	void do_putlluc(const spu_mfc_cmd& args);
	bool do_putllc(const spu_mfc_cmd& args);
	bool do_mfc(bool can_escape = true, bool must_finish = true);
	u32 get_mfc_completed() const;

	bool process_mfc_cmd();
	ch_events_t get_events(u64 mask_hint = umax, bool waiting = false, bool reading = false);
	void set_events(u32 bits);
	void set_interrupt_status(bool enable);
	bool check_mfc_interrupts(u32 next_pc);
	static bool is_exec_code(u32 addr, std::span<const u8> ls_ptr, u32 base_addr = 0, bool avoid_dead_code = false, bool is_range_limited = false); // A hint, do not rely on it for true execution compatibility
	static std::vector<u32> discover_functions(u32 base_addr, std::span<const u8> ls, bool is_known_addr, u32 /*entry*/);
	u32 get_ch_count(u32 ch);
	s64 get_ch_value(u32 ch);
	bool set_ch_value(u32 ch, u32 value);
	bool stop_and_signal(u32 code);
	void halt();

	void fast_call(u32 ls_addr);

	std::array<std::shared_ptr<utils::serial>, 32> rewind_captures; // shared_ptr to avoid header inclusion
	u8 current_rewind_capture_idx = 0;

	static spu_exec_object capture_memory_as_elf(std::span<spu_memory_segment_dump_data> segs, u32 pc_hint = umax);
	bool capture_state();
	bool try_load_debug_capture();
	void wakeup_delay(u32 div = 1) const;

	// Convert specified SPU LS address to a pointer of specified (possibly converted to BE) type
	template<typename T>
	to_be_t<T>* _ptr(u32 lsa) const
	{
		return reinterpret_cast<to_be_t<T>*>(ls + (lsa % SPU_LS_SIZE));
	}

	// Convert specified SPU LS address to a reference of specified (possibly converted to BE) type
	template<typename T>
	to_be_t<T>& _ref(u32 lsa) const
	{
		return *_ptr<T>(lsa);
	}

	spu_type get_type() const
	{
		return thread_type;
	}

	u32 vm_offset() const
	{
		return group ? SPU_FAKE_BASE_ADDR + SPU_LS_SIZE * (id & 0xffffff) : RAW_SPU_BASE_ADDR + RAW_SPU_OFFSET * index;
	}

	static u8* map_ls(utils::shm& shm, void* ptr = nullptr);

	// Returns true if reservation existed but was just discovered to be lost
	// It is safe to use on any address, even if not directly accessed by SPU (so it's slower)
	// Optionally pass a known allocated address for internal optimization (the current Effective-Address of the MFC command)
	bool reservation_check(u32 addr, const decltype(rdata)& data, u32 current_eal = 0) const;
	static bool reservation_check(u32 addr, u32 hash, atomic_t<u64, 64>* range_lock);
	usz register_cache_line_waiter(u32 addr);
	void deregister_cache_line_waiter(usz index);

	bool read_reg(const u32 addr, u32& value);
	bool write_reg(const u32 addr, const u32 value);
	static bool test_is_problem_state_register_offset(u32 offset, bool for_read, bool for_write) noexcept;

	static atomic_t<u32> g_raw_spu_ctr;
	static atomic_t<u32> g_raw_spu_id[5];
	static atomic_t<u32> g_spu_work_count;

	static atomic_t<u64> g_spu_waiters_by_value[6];

	static u32 find_raw_spu(u32 id)
	{
		if (id < std::size(g_raw_spu_id)) [[likely]]
		{
			return g_raw_spu_id[id];
		}

		return -1;
	}

	// For named_thread ctor
	const struct thread_name_t
	{
		const spu_thread* _this;

		operator std::string() const;
	} thread_name{ this };

	union spu_prio_t
	{
		u64 all;
		bf_t<s64, 0, 9> prio; // Thread priority (0..3071) (firs 9-bits)
		bf_t<s64, 9, 55> order; // Thread enqueue order (TODO, last 52-bits)
	};

	// For lv2_obj::schedule<spu_thread>
	struct priority_t
	{
		const spu_thread* _this;

		spu_prio_t load() const;

		template <typename Func>
		auto atomic_op(Func&& func)
		{
			return static_cast<std::conditional_t<std::is_void_v<Func>, Func, decltype(_this->group)>>(_this->group)->prio.atomic_op(std::move(func));
		}
	} prio{ this };
};

class spu_function_logger
{
	spu_thread& spu;

public:
	spu_function_logger(spu_thread& spu, const char* func) noexcept;

	~spu_function_logger() noexcept
	{
		if (!spu.is_stopped())
		{
			spu.start_time = 0;
		}
	}
};
