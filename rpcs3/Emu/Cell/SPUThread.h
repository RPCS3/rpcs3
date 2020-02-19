#pragma once

#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "Emu/Memory/vm.h"
#include "MFC.h"
#include "Emu/Memory/vm.h"
#include "Utilities/BEType.h"

#include <map>

LOG_CHANNEL(spu_log, "SPU");

struct lv2_event_queue;
struct lv2_spu_group;
struct lv2_int_tag;

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

	SPU_EVENT_IMPLEMENTED  = SPU_EVENT_LR | SPU_EVENT_TM | SPU_EVENT_SN, // Mask of implemented events
	SPU_EVENT_INTR_IMPLEMENTED = SPU_EVENT_SN,

	SPU_EVENT_WAITING      = 0x80000000, // Originally unused, set when SPU thread starts waiting on ch_event_stat
	//SPU_EVENT_AVAILABLE  = 0x40000000, // Originally unused, channel count of the SPU_RdEventStat channel
	//SPU_EVENT_INTR_ENABLED = 0x20000000, // Originally unused, represents "SPU Interrupts Enabled" status

	SPU_EVENT_INTR_TEST = SPU_EVENT_INTR_IMPLEMENTED
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

enum
{
	SPU_RUNCNTL_STOP_REQUEST = 0,
	SPU_RUNCNTL_RUN_REQUEST  = 1,
};

// SPU Status Register bits (not accurate)
enum
{
	SPU_STATUS_STOPPED             = 0x0,
	SPU_STATUS_RUNNING             = 0x1,
	SPU_STATUS_STOPPED_BY_STOP     = 0x2,
	SPU_STATUS_STOPPED_BY_HALT     = 0x4,
	SPU_STATUS_WAITING_FOR_CHANNEL = 0x8,
	SPU_STATUS_SINGLE_STEP         = 0x10,
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
};

struct spu_channel
{
	// Low 32 bits contain value
	atomic_t<u64> data;

public:
	static const u32 off_wait = 32;
	static const u32 off_count = 63;
	static const u64 bit_wait = 1ull << off_wait;
	static const u64 bit_count = 1ull << off_count;

	// Returns true on success
	bool try_push(u32 value)
	{
		const u64 old = data.fetch_op([value](u64& data)
		{
			if (data & bit_count) [[unlikely]]
			{
				data |= bit_wait;
			}
			else
			{
				data = bit_count | value;
			}
		});

		return !(old & bit_count);
	}

	// Push performing bitwise OR with previous value, may require notification
	void push_or(cpu_thread& spu, u32 value)
	{
		const u64 old = data.fetch_op([value](u64& data)
		{
			data &= ~bit_wait;
			data |= bit_count | value;
		});

		if (old & bit_wait)
		{
			spu.notify();
		}
	}

	bool push_and(u32 value)
	{
		return (data.fetch_and(~u64{value}) & value) != 0;
	}

	// Push unconditionally (overwriting previous value), may require notification
	void push(cpu_thread& spu, u32 value)
	{
		if (data.exchange(bit_count | value) & bit_wait)
		{
			spu.notify();
		}
	}

	// Returns true on success
	bool try_pop(u32& out)
	{
		const u64 old = data.fetch_op([&](u64& data)
		{
			if (data & bit_count) [[likely]]
			{
				out = static_cast<u32>(data);
				data = 0;
			}
			else
			{
				data |= bit_wait;
			}
		});

		return (old & bit_count) != 0;
	}

	// Pop unconditionally (loading last value), may require notification
	u32 pop(cpu_thread& spu)
	{
		// Value is not cleared and may be read again
		const u64 old = data.fetch_and(~(bit_count | bit_wait));

		if (old & bit_wait)
		{
			spu.notify();
		}

		return static_cast<u32>(old);
	}

	void set_value(u32 value, bool count = true)
	{
		data.release(u64{count} << off_count | value);
	}

	u32 get_value()
	{
		return static_cast<u32>(data);
	}

	u32 get_count()
	{
		return static_cast<u32>(data >> off_count);
	}
};

struct spu_channel_4_t
{
	struct alignas(16) sync_var_t
	{
		u8 waiting;
		u8 count;
		u32 value0;
		u32 value1;
		u32 value2;
	};

	atomic_t<sync_var_t> values;
	atomic_t<u32> value3;

public:
	void clear()
	{
		values.release({});
	}

	// push unconditionally (overwriting latest value), returns true if needs signaling
	void push(cpu_thread& spu, u32 value)
	{
		value3.release(value);

		if (values.atomic_op([value](sync_var_t& data) -> bool
		{
			switch (data.count++)
			{
			case 0: data.value0 = value; break;
			case 1: data.value1 = value; break;
			case 2: data.value2 = value; break;
			default: data.count = 4;
			}

			if (data.waiting)
			{
				data.waiting = 0;

				return true;
			}

			return false;
		}))
		{
			spu.notify();
		}
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
			else
			{
				data.waiting = 1;
			}

			return result;
		});
	}

	u32 get_count()
	{
		return values.raw().count;
	}

	void set_values(u32 count, u32 value0, u32 value1 = 0, u32 value2 = 0, u32 value3 = 0)
	{
		this->values.raw() = { 0, static_cast<u8>(count), value0, value1, value2 };
		this->value3 = value3;
	}
};

struct spu_int_ctrl_t
{
	atomic_t<u64> mask;
	atomic_t<u64> stat;

	std::weak_ptr<struct lv2_int_tag> tag;

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

		FORCE_INLINE __m128 operator [] (s32 scale) const
		{
			return m_data[scale + 155].vf;
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
	u32 _u32[4];

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
			fmt::throw_exception("Unexpected slice value (%d)" HERE, slice);
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
		_u32[1+slice] |= exceptions;
	}

	// Write the FPSCR
	void Write(const v128 & r)
	{
		_u32[3] = r._u32[3] & 0x00000F07;
		_u32[2] = r._u32[2] & 0x00003F07;
		_u32[1] = r._u32[1] & 0x00003F07;
		_u32[0] = r._u32[0] & 0x00000F07;
	}

	// Read the FPSCR
	void Read(v128 & r)
	{
		r._u32[3] = _u32[3];
		r._u32[2] = _u32[2];
		r._u32[1] = _u32[1];
		r._u32[0] = _u32[0];
	}
};

class spu_thread : public cpu_thread
{
public:
	virtual std::string dump() const override;
	virtual void cpu_task() override final;
	virtual void cpu_mem() override;
	virtual void cpu_unmem() override;
	virtual ~spu_thread() override;
	void cpu_init();
	void cpu_stop();

	static const u32 id_base = 0x02000000; // TODO (used to determine thread type)
	static const u32 id_step = 1;
	static const u32 id_count = 2048;

	spu_thread(vm::addr_t ls, lv2_spu_group* group, u32 index, std::string_view name, u32 lv2_id);

	u32 pc = 0;

	// May be used internally by recompilers.
	u32 base_pc = 0;

	// May be used by recompilers.
	u8* memory_base_addr = vm::g_base_addr;

	// General-Purpose Registers
	std::array<v128, 128> gpr;
	SPU_FPSCR fpscr;

	// MFC command data
	spu_mfc_cmd ch_mfc_cmd;

	// MFC command queue
	spu_mfc_cmd mfc_queue[16]{};
	u32 mfc_size = 0;
	u32 mfc_barrier = -1;
	u32 mfc_fence = -1;

	// MFC proxy command data
	spu_mfc_cmd mfc_prxy_cmd;
	shared_mutex mfc_prxy_mtx;
	atomic_t<u32> mfc_prxy_mask;

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
	std::array<v128, 8> rdata{};
	u32 raddr = 0;

	u32 srr0;
	u32 ch_tag_upd;
	u32 ch_tag_mask;
	spu_channel ch_tag_stat;
	u32 ch_stall_mask;
	spu_channel ch_stall_stat;
	spu_channel ch_atomic_stat;

	spu_channel_4_t ch_in_mbox{};

	spu_channel ch_out_mbox;
	spu_channel ch_out_intr_mbox;

	u64 snr_config = 0; // SPU SNR Config Register

	spu_channel ch_snr1{}; // SPU Signal Notification Register 1
	spu_channel ch_snr2{}; // SPU Signal Notification Register 2

	atomic_t<u32> ch_event_mask;
	atomic_t<u32> ch_event_stat;
	atomic_t<bool> interrupts_enabled;

	u64 ch_dec_start_timestamp; // timestamp of writing decrementer value
	u32 ch_dec_value; // written decrementer value

	atomic_t<u32> run_ctrl; // SPU Run Control register (only provided to get latest data written)
	shared_mutex run_ctrl_mtx;

	struct alignas(8) status_npc_sync_var
	{
		u32 status; // SPU Status register
		u32 npc; // SPU Next Program Counter register
	};

	atomic_t<status_npc_sync_var> status_npc;

	std::array<spu_int_ctrl_t, 3> int_ctrl; // SPU Class 0, 1, 2 Interrupt Management

	std::array<std::pair<u32, std::weak_ptr<lv2_event_queue>>, 32> spuq; // Event Queue Keys for SPU Thread
	std::weak_ptr<lv2_event_queue> spup[64]; // SPU Ports

	const u32 index; // SPU index
	const u32 offset; // SPU LS offset
private:
	lv2_spu_group* const group; // SPU Thread Group (only safe to access in the spu thread itself)
public:
	const u32 lv2_id; // The actual id that is used by syscalls

	// Thread name
	stx::atomic_cptr<std::string> spu_tname;

	std::unique_ptr<class spu_recompiler_base> jit; // Recompiler instance

	u64 block_counter = 0;
	u64 block_recover = 0;
	u64 block_failure = 0;

	u64 saved_native_sp = 0; // Host thread's stack pointer for emulated longjmp

	std::array<v128, 0x4000> stack_mirror; // Return address information

	void push_snr(u32 number, u32 value);
	void do_dma_transfer(const spu_mfc_cmd& args);
	bool do_dma_check(const spu_mfc_cmd& args);
	bool do_list_transfer(spu_mfc_cmd& args);
	void do_putlluc(const spu_mfc_cmd& args);
	void do_mfc(bool wait = true);
	u32 get_mfc_completed();

	bool process_mfc_cmd();
	u32 get_events(bool waiting = false);
	void set_events(u32 mask);
	void set_interrupt_status(bool enable);
	u32 get_ch_count(u32 ch);
	s64 get_ch_value(u32 ch);
	bool set_ch_value(u32 ch, u32 value);
	bool stop_and_signal(u32 code);
	void halt();

	void fast_call(u32 ls_addr);

	// Convert specified SPU LS address to a pointer of specified (possibly converted to BE) type
	template<typename T>
	inline to_be_t<T>* _ptr(u32 lsa)
	{
		return static_cast<to_be_t<T>*>(vm::base(offset + lsa));
	}

	// Convert specified SPU LS address to a reference of specified (possibly converted to BE) type
	template<typename T>
	inline to_be_t<T>& _ref(u32 lsa)
	{
		return *_ptr<T>(lsa);
	}

	bool read_reg(const u32 addr, u32& value);
	bool write_reg(const u32 addr, const u32 value);

	static atomic_t<u32> g_raw_spu_ctr;
	static atomic_t<u32> g_raw_spu_id[5];

	static u32 find_raw_spu(u32 id)
	{
		if (id < std::size(g_raw_spu_id)) [[likely]]
		{
			return g_raw_spu_id[id];
		}

		return -1;
	}
};
