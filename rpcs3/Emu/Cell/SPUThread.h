#pragma once

#include "Emu/Cell/Common.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "MFC.h"

struct lv2_event_queue;
struct lv2_spu_group;
struct lv2_int_tag;

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
	SPU_EVENT_LR = 0x400,  // Lock Line Reservation Lost event ~ TODO: check if an immediate update needed
	SPU_EVENT_S1 = 0x200,  // Signal Notification Register 1 available
	SPU_EVENT_S2 = 0x100,  // Signal Notification Register 2 available
	SPU_EVENT_LE = 0x80,   // SPU Outbound Mailbox available
	SPU_EVENT_ME = 0x40,   // SPU Outbound Interrupt Mailbox available
	SPU_EVENT_TM = 0x20,   // SPU Decrementer became negative (?)
	SPU_EVENT_MB = 0x10,   // SPU Inbound mailbox available
	SPU_EVENT_QV = 0x4,    // MFC SPU Command Queue available
	SPU_EVENT_SN = 0x2,    // MFC List Command stall-and-notify event
	SPU_EVENT_TG = 0x1,    // MFC Tag Group status update event

	SPU_EVENT_WAITING      = 0x80000000, // Originally unused, set when SPU thread starts waiting on ch_event_stat
	//SPU_EVENT_AVAILABLE  = 0x40000000, // Originally unused, channel count of the SPU_RdEventStat channel
	//SPU_EVENT_INTR_ENABLED = 0x20000000, // Originally unused, represents "SPU Interrupts Enabled" status
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

enum : u32 //orginally unused.
{ 
	dec_msb = 0x1, // the most significant bit of the decrementer
	dec_run = 0x2, // the state of the decrementer
};

struct spu_channel_t
{
	struct alignas(8) sync_var_t
	{
		bool count; // value available
		bool wait; // notification required
		u32 value;
	};

	atomic_t<sync_var_t> data;

public:
	// returns true on success
	bool try_push(u32 value)
	{
		const auto old = data.fetch_op([=](sync_var_t& data)
		{
			if ((data.wait = data.count) == false)
			{
				data.count = true;
				data.value = value;
			}
		});

		return !old.count;
	}

	// push performing bitwise OR with previous value, may require notification
	void push_or(cpu_thread& spu, u32 value)
	{
		const auto old = data.fetch_op([=](sync_var_t& data)
		{
			data.count = true;
			data.wait = false;
			data.value |= value;
		});

		if (old.wait) spu.notify();
	}

	bool push_and(u32 value)
	{
		const auto old = data.fetch_op([=](sync_var_t& data)
		{
			data.value &= ~value;
		});

		return (old.value & value) != 0;
	}

	// push unconditionally (overwriting previous value), may require notification
	void push(cpu_thread& spu, u32 value)
	{
		const auto old = data.fetch_op([=](sync_var_t& data)
		{
			data.count = true;
			data.wait = false;
			data.value = value;
		});

		if (old.wait) spu.notify();
	}

	// returns true on success
	bool try_pop(u32& out)
	{
		const auto old = data.fetch_op([&](sync_var_t& data)
		{
			if (data.count)
			{
				data.wait = false;
				out = data.value;
			}
			else
			{
				data.wait = true;
			}

			data.count = false;
			data.value = 0; // ???
		});

		return old.count;
	}

	// pop unconditionally (loading last value), may require notification
	u32 pop(cpu_thread& spu)
	{
		const auto old = data.fetch_op([](sync_var_t& data)
		{
			data.wait = false;
			data.count = false;
			// value is not cleared and may be read again
		});

		if (old.wait) spu.notify();

		return old.value;
	}

	void set_value(u32 value, bool count = true)
	{
		data.store({ count, false, value });
	}

	u32 get_value()
	{
		return data.load().value;
	}

	u32 get_count()
	{
		return data.load().count;
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
		values.store({});
		value3 = 0;
	}

	// push unconditionally (overwriting latest value), returns true if needs signaling
	void push(cpu_thread& spu, u32 value)
	{
		value3 = value; _mm_sfence();

		if (values.atomic_op([=](sync_var_t& data) -> bool
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
				_mm_lfence();
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

	std::shared_ptr<struct lv2_int_tag> tag;

	void set(u64 ints);

	void clear(u64 ints)
	{
		stat &= ~ints;
	}

	void clear()
	{
		mask = 0;
		stat = 0;
		tag = nullptr;
	}
};

struct spu_imm_table_t
{
	v128 fsmb[65536]; // table for FSMB, FSMBI instructions
	v128 fsmh[256]; // table for FSMH instruction
	v128 fsm[16]; // table for FSM instruction

	v128 sldq_pshufb[32]; // table for SHLQBYBI, SHLQBY, SHLQBYI instructions
	v128 srdq_pshufb[32]; // table for ROTQMBYBI, ROTQMBY, ROTQMBYI instructions
	v128 rldq_pshufb[16]; // table for ROTQBYBI, ROTQBY, ROTQBYI instructions

	class scale_table_t
	{
		std::array<__m128, 155 + 174> m_data;

	public:
		scale_table_t();

		FORCE_INLINE __m128 operator [] (s32 scale) const
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

class SPUThread : public cpu_thread
{
public:
	virtual void on_spawn() override;
	virtual void on_init(const std::shared_ptr<void>&) override;
	virtual std::string get_name() const override;
	virtual std::string dump() const override;
	virtual void cpu_task() override;
	virtual ~SPUThread() override;
	void cpu_init();

protected:
	SPUThread(const std::string& name);

public:
	static const u32 id_base = 0x02000000; // TODO (used to determine thread type)
	static const u32 id_step = 1;
	static const u32 id_count = 2048;

	SPUThread(const std::string& name, u32 index, lv2_spu_group* group);

	// General-Purpose Registers
	std::array<v128, 128> gpr;
	SPU_FPSCR fpscr;

	// MFC command data
	spu_mfc_cmd ch_mfc_cmd;

	// MFC command queue (consumer: MFC thread)
	lf_spsc<spu_mfc_cmd, 16> mfc_queue;

	// MFC command proxy queue (consumer: MFC thread)
	lf_mpsc<spu_mfc_cmd, 8> mfc_proxy;

	// Reservation Data
	u64 rtime = 0;
	std::array<u128, 8> rdata{};
	u32 raddr = 0;

	u32 srr0;
	atomic_t<u32> ch_tag_upd;
	atomic_t<u32> ch_tag_mask;
	spu_channel_t ch_tag_stat;
	atomic_t<u32> ch_stall_mask;
	spu_channel_t ch_stall_stat;
	spu_channel_t ch_atomic_stat;

	spu_channel_4_t ch_in_mbox;

	atomic_t<u32> mfc_prxy_mask;

	spu_channel_t ch_out_mbox;
	spu_channel_t ch_out_intr_mbox;

	u64 snr_config; // SPU SNR Config Register

	spu_channel_t ch_snr1; // SPU Signal Notification Register 1
	spu_channel_t ch_snr2; // SPU Signal Notification Register 2

	atomic_t<u32> ch_event_mask;
	atomic_t<u32> ch_event_stat;
    atomic_t<bool> interrupts_enabled;

	u64 ch_dec_start_timestamp; // timestamp of writing decrementer value
	u32 ch_dec_value; // written decrementer value and the decrementer value when it stops.
	atomic_t<u32> dec_state; // contains various decrementer related contidions 

	atomic_t<u32> run_ctrl; // SPU Run Control register (only provided to get latest data written)
	atomic_t<u32> status; // SPU Status register
	atomic_t<u32> npc; // SPU Next Program Counter register

	std::array<spu_int_ctrl_t, 3> int_ctrl; // SPU Class 0, 1, 2 Interrupt Management

	std::array<std::pair<u32, std::weak_ptr<lv2_event_queue>>, 32> spuq; // Event Queue Keys for SPU Thread
	std::weak_ptr<lv2_event_queue> spup[64]; // SPU Ports

	u32 pc = 0; // 
	const u32 index; // SPU index
	const u32 offset; // SPU LS offset
	lv2_spu_group* const group; // SPU Thread Group

	const std::string m_name; // Thread name

	std::exception_ptr pending_exception;

	std::array<struct spu_function_t*, 65536> compiled_cache{};
	std::shared_ptr<class SPUDatabase> spu_db;
	std::shared_ptr<class spu_recompiler_base> spu_rec;
	u32 recursion_level = 0;

	void push_snr(u32 number, u32 value);
	void do_dma_transfer(const spu_mfc_cmd& args, bool from_mfc = true);

	void process_mfc_cmd();
	u32 get_events(bool waiting = false);
	void set_events(u32 mask);
	u32 get_ch_count(u32 ch);
	bool get_ch_value(u32 ch, u32& out);
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
};
