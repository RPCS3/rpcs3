#pragma once

#include "Emu/Cell/Common.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/SPUContext.h"
#include "MFC.h"

struct lv2_event_queue_t;
struct spu_group_t;
struct lv2_int_tag_t;

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
	SPU_EVENT_QV = 0x4,    // MFC SPU Command Queue available
	SPU_EVENT_SN = 0x2,    // MFC List Command stall-and-notify event
	SPU_EVENT_TG = 0x1,    // MFC Tag Group status update event

	SPU_EVENT_IMPLEMENTED  = SPU_EVENT_LR, // Mask of implemented events

	SPU_EVENT_WAITING      = 0x80000000, // Originally unused, set when SPU thread starts waiting on ch_event_stat
	//SPU_EVENT_AVAILABLE  = 0x40000000, // Originally unused, channel count of the SPU_RdEventStat channel
	SPU_EVENT_INTR_ENABLED = 0x20000000, // Originally unused, represents "SPU Interrupts Enabled" status
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

union spu_channel_t
{
	struct sync_var_t
	{
		struct
		{
			u32 waiting : 1; // waiting flag (0..1)
			u32 count : 1; // channel count (0..1)
		};
		
		u32 value;
	};

	atomic_t<sync_var_t> sync_var;

public:
	// returns true on success
	bool try_push(u32 value)
	{
		return sync_var.atomic_op([=](sync_var_t& data) -> bool
		{
			if (data.count == 0)
			{
				data.waiting = 0;
				data.count = 1;
				data.value = value;

				return true;
			}

			data.waiting = 1;
			return false;
		});
	}

	// push performing bitwise OR with previous value, returns true if needs signaling
	bool push_or(u32 value)
	{
		return sync_var.atomic_op([=](sync_var_t& data) -> bool
		{
			data.count = 1;
			data.value |= value;

			if (data.waiting)
			{
				data.waiting = 0;

				return true;
			}

			return false;
		});
	}

	// push unconditionally (overwriting previous value), returns true if needs signaling
	bool push(u32 value)
	{
		return sync_var.atomic_op([=](sync_var_t& data) -> bool
		{
			data.count = 1;
			data.value = value;

			if (data.waiting)
			{
				data.waiting = 0;

				return true;
			}

			return false;
		});
	}

	// returns true on success and u32 value
	std::tuple<bool, u32> try_pop()
	{
		return sync_var.atomic_op([](sync_var_t& data)
		{
			const auto result = std::make_tuple(data.count != 0, u32{ data.value });

			data.waiting = data.count == 0;
			data.count = 0;
			data.value = 0;

			return result;
		});
	}

	// pop unconditionally (loading last value), returns u32 value and bool value (true if needs signaling)
	std::tuple<u32, bool> pop()
	{
		return sync_var.atomic_op([](sync_var_t& data)
		{
			const auto result = std::make_tuple(u32{ data.value }, data.waiting != 0);

			data.waiting = 0;
			data.count = 0;
			// value is not cleared and may be read again
			
			return result;
		});
	}

	void set_value(u32 value, u32 count = 1)
	{
		sync_var.store({ { 0, count }, value });
	}

	u32 get_value() volatile
	{
		return sync_var.data.value;
	}

	u32 get_count() volatile
	{
		return sync_var.data.count;
	}
};

struct spu_channel_4_t
{
	struct sync_var_t
	{
		struct
		{
			u32 waiting : 1;
			u32 count : 3;
		};
		
		u32 value0;
		u32 value1;
		u32 value2;
	};

	atomic_t<sync_var_t> sync_var;
	atomic_t<u32> value3;

public:
	void clear()
	{
		sync_var = {};
		value3 = {};
	}

	// push unconditionally (overwriting latest value), returns true if needs signaling
	bool push(u32 value)
	{
		value3.exchange(value);

		return sync_var.atomic_op([=](sync_var_t& data) -> bool
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
		});
	}

	// returns true on success and two u32 values: data and count after removing the first element
	std::tuple<bool, u32, u32> try_pop()
	{
		return sync_var.atomic_op([this](sync_var_t& data)
		{
			const auto result = std::make_tuple(data.count != 0, u32{ data.value0 }, u32{ data.count - 1u });

			if (data.count != 0)
			{
				data.waiting = 0;
				data.count--;

				data.value0 = data.value1;
				data.value1 = data.value2;
				data.value2 = value3.load_sync();
			}
			else
			{
				data.waiting = 1;
			}

			return result;
		});
	}

	u32 get_count() volatile
	{
		return sync_var.data.count;
	}

	void set_values(u32 count, u32 value0, u32 value1 = 0, u32 value2 = 0, u32 value3 = 0)
	{
		sync_var.data.waiting = 0;
		sync_var.data.count = count;
		sync_var.data.value0 = value0;
		sync_var.data.value1 = value1;
		sync_var.data.value2 = value2;
		this->value3.store(value3);
	}
};

struct spu_int_ctrl_t
{
	atomic_t<u64> mask;
	atomic_t<u64> stat;

	std::shared_ptr<struct lv2_int_tag_t> tag;

	void set(u64 ints);

	void clear(u64 ints);
};

struct g_spu_imm_table_t
{
	u128 fsmb[65536]; // table for FSMB, FSMBI instructions
	u128 fsmh[256]; // table for FSMH instruction
	u128 fsm[16]; // table for FSM instruction

	u128 sldq_pshufb[32]; // table for SHLQBYBI, SHLQBY, SHLQBYI instructions
	u128 srdq_pshufb[32]; // table for ROTQMBYBI, ROTQMBY, ROTQMBYI instructions
	u128 rldq_pshufb[16]; // table for ROTQBYBI, ROTQBY, ROTQBYI instructions

	class scale_table_t
	{
		std::array<__m128, 155 + 174> m_data;

	public:
		scale_table_t()
		{
			for (s32 i = -155; i < 174; i++)
			{
				m_data[i + 155] = _mm_set1_ps(static_cast<float>(exp2(i)));
			}
		}

		force_inline __m128 operator [] (s32 scale) const
		{
			return m_data[scale + 155];
		}
	}
	const scale;

	g_spu_imm_table_t()
	{
		for (u32 i = 0; i < sizeof(fsm) / sizeof(fsm[0]); i++)
		{
			for (u32 j = 0; j < 4; j++)
			{
				fsm[i]._u32[j] = (i & (1 << j)) ? 0xffffffff : 0;
			}
		}

		for (u32 i = 0; i < sizeof(fsmh) / sizeof(fsmh[0]); i++)
		{
			for (u32 j = 0; j < 8; j++)
			{
				fsmh[i]._u16[j] = (i & (1 << j)) ? 0xffff : 0;
			}
		}

		for (u32 i = 0; i < sizeof(fsmb) / sizeof(fsmb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++)
			{
				fsmb[i]._u8[j] = (i & (1 << j)) ? 0xff : 0;
			}
		}

		for (u32 i = 0; i < sizeof(sldq_pshufb) / sizeof(sldq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++)
			{
				sldq_pshufb[i]._u8[j] = static_cast<u8>(j - i);
			}
		}

		for (u32 i = 0; i < sizeof(srdq_pshufb) / sizeof(srdq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++)
			{
				srdq_pshufb[i]._u8[j] = (j + i > 15) ? 0xff : static_cast<u8>(j + i);
			}
		}

		for (u32 i = 0; i < sizeof(rldq_pshufb) / sizeof(rldq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++)
			{
				rldq_pshufb[i]._u8[j] = static_cast<u8>((j - i) & 0xf);
			}
		}
	}
};

extern const g_spu_imm_table_t g_spu_imm;

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
		return fmt::Format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);
	}

	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}
	//slice -> 0 - 1 (double-precision slice index)
	//NOTE: slices follow u128 indexing, i.e. slice 0 is RIGHT end of register!
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
			throw EXCEPTION("Unexpected slice value (%d)", slice);
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
	void Write(const u128 & r)
	{
		_u32[3] = r._u32[3] & 0x00000F07;
		_u32[2] = r._u32[2] & 0x00003F07;
		_u32[1] = r._u32[1] & 0x00003F07;
		_u32[0] = r._u32[0] & 0x00000F07;
	}

	// Read the FPSCR
	void Read(u128 & r)
	{
		r._u32[3] = _u32[3];
		r._u32[2] = _u32[2];
		r._u32[1] = _u32[1];
		r._u32[0] = _u32[0];
	}
};

class SPUThread : public CPUThread
{
public:
	u128 GPR[128]; // General-Purpose Registers
	SPU_FPSCR FPSCR;

	std::unordered_map<u32, std::function<bool(SPUThread& SPU)>> m_addr_to_hle_function_map;

	spu_mfc_arg_t ch_mfc_args;

	std::vector<std::pair<u32, spu_mfc_arg_t>> mfc_queue; // Only used for stalled list transfers

	u32 ch_tag_mask;
	spu_channel_t ch_tag_stat;
	spu_channel_t ch_stall_stat;
	spu_channel_t ch_atomic_stat;

	spu_channel_4_t ch_in_mbox;

	spu_channel_t ch_out_mbox;
	spu_channel_t ch_out_intr_mbox;

	u64 snr_config; // SPU SNR Config Register

	spu_channel_t ch_snr1; // SPU Signal Notification Register 1
	spu_channel_t ch_snr2; // SPU Signal Notification Register 2

	atomic_t<u32> ch_event_mask;
	atomic_t<u32> ch_event_stat;
	u32 last_raddr; // Last Reservation Address (0 if not set)

	u64 ch_dec_start_timestamp; // timestamp of writing decrementer value
	u32 ch_dec_value; // written decrementer value

	atomic_t<u32> run_ctrl; // SPU Run Control register (only provided to get latest data written)
	atomic_t<u32> status; // SPU Status register
	atomic_t<u32> npc; // SPU Next Program Counter register

	std::array<spu_int_ctrl_t, 3> int_ctrl; // SPU Class 0, 1, 2 Interrupt Management

	std::weak_ptr<spu_group_t> tg; // SPU Thread Group

	std::array<std::pair<u32, std::weak_ptr<lv2_event_queue_t>>, 32> spuq; // Event Queue Keys for SPU Thread
	std::weak_ptr<lv2_event_queue_t> spup[64]; // SPU Ports

	u32 PC = 0;
	const u32 index; // SPU index
	const u32 offset; // SPU LS offset

	void push_snr(u32 number, u32 value)
	{
		if (number == 0)
		{
			if (snr_config & 1)
			{
				if (!ch_snr1.push_or(value)) return;
			}
			else
			{
				if (!ch_snr1.push(value)) return;
			}
		}
		else if (number == 1)
		{
			if (snr_config & 2)
			{
				if (!ch_snr2.push_or(value)) return;
			}
			else
			{
				if (!ch_snr2.push(value)) return;
			}
		}
		else
		{
			throw EXCEPTION("Unexpected");
		}

		// notify if required
		std::lock_guard<std::mutex> lock(mutex);

		cv.notify_one();
	}

	void do_dma_transfer(u32 cmd, spu_mfc_arg_t args);
	void do_dma_list_cmd(u32 cmd, spu_mfc_arg_t args);
	void process_mfc_cmd(u32 cmd);

	u32 get_events(bool waiting = false);
	void set_events(u32 mask);
	void set_interrupt_status(bool enable);
	u32 get_ch_count(u32 ch);
	u32 get_ch_value(u32 ch);
	void set_ch_value(u32 ch, u32 value);

	void stop_and_signal(u32 code);
	void halt();

	u8 read8(u32 lsa) const { return vm::read8(lsa + offset); }
	u16 read16(u32 lsa) const { return vm::ps3::read16(lsa + offset); }
	u32 read32(u32 lsa) const { return vm::ps3::read32(lsa + offset); }
	u64 read64(u32 lsa) const { return vm::ps3::read64(lsa + offset); }
	u128 read128(u32 lsa) const { return vm::ps3::read128(lsa + offset); }

	void write8(u32 lsa, u8 data) const { vm::write8(lsa + offset, data); }
	void write16(u32 lsa, u16 data) const { vm::ps3::write16(lsa + offset, data); }
	void write32(u32 lsa, u32 data) const { vm::ps3::write32(lsa + offset, data); }
	void write64(u32 lsa, u64 data) const { vm::ps3::write64(lsa + offset, data); }
	void write128(u32 lsa, u128 data) const { vm::ps3::write128(lsa + offset, data); }

	void write16(u32 lsa, be_t<u16> data) const { vm::ps3::write16(lsa + offset, data); }
	void write32(u32 lsa, be_t<u32> data) const { vm::ps3::write32(lsa + offset, data); }
	void write64(u32 lsa, be_t<u64> data) const { vm::ps3::write64(lsa + offset, data); }
	void write128(u32 lsa, be_t<u128> data) const { vm::ps3::write128(lsa + offset, data); }

	void RegisterHleFunction(u32 addr, std::function<bool(SPUThread & SPU)> function)
	{
		m_addr_to_hle_function_map[addr] = function;
		write32(addr, 0x00000003); // STOP 3
	}

	void UnregisterHleFunction(u32 addr)
	{
		m_addr_to_hle_function_map.erase(addr);
	}

	void UnregisterHleFunctions(u32 start_addr, u32 end_addr)
	{
		for (auto iter = m_addr_to_hle_function_map.begin(); iter != m_addr_to_hle_function_map.end();)
		{
			if (iter->first >= start_addr && iter->first <= end_addr)
			{
				m_addr_to_hle_function_map.erase(iter++);
			}
			else
			{
				iter++;
			}
		}
	}

	std::function<void(SPUThread&)> custom_task;

protected:
	SPUThread(CPUThreadType type, const std::string& name, std::function<std::string()> thread_name, u32 index, u32 offset);

public:
	SPUThread(const std::string& name, u32 index);
	virtual ~SPUThread() override;

	virtual bool is_paused() const override;

	virtual void dump_info() const override;
	virtual u32 get_pc() const override { return PC; }
	virtual u32 get_offset() const override { return offset; }
	virtual void do_run() override;
	virtual void task() override;

	virtual void init_regs() override;
	virtual void init_stack() override;
	virtual void close_stack() override;

	void fast_call(u32 ls_addr);

	virtual std::string RegsToString() const
	{
		std::string ret = "Registers:\n=========\n";

		for(uint i=0; i<128; ++i) ret += fmt::Format("GPR[%d] = 0x%s\n", i, GPR[i].to_hex().c_str());

		return ret;
	}

	virtual std::string ReadRegString(const std::string& reg) const
	{
		std::string::size_type first_brk = reg.find('[');
		if (first_brk != std::string::npos)
		{
			long reg_index;
			reg_index = atol(reg.substr(first_brk + 1, reg.length()-2).c_str());
			if (reg.find("GPR")==0) return fmt::Format("%016llx%016llx",  GPR[reg_index]._u64[1], GPR[reg_index]._u64[0]);
		}
		return "";
	}

	bool WriteRegString(const std::string& reg, std::string value)
	{
		while (value.length() < 32) value = "0"+value;
		std::string::size_type first_brk = reg.find('[');
		if (first_brk != std::string::npos)
		{
			long reg_index;
			reg_index = atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
			if (reg.find("GPR")==0)
			{
				unsigned long long reg_value0;
				unsigned long long reg_value1;
				try
				{
					reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
					reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
				}
				catch (std::invalid_argument& /*e*/)
				{
					return false;
				}
				GPR[reg_index]._u64[0] = (u64)reg_value0;
				GPR[reg_index]._u64[1] = (u64)reg_value1;
				return true;
			}
		}
		return false;
	}
};

class spu_thread : cpu_thread
{
public:
	spu_thread(u32 entry, const std::string& name = "", u32 stack_size = 0, u32 prio = 0);

	cpu_thread& args(std::initializer_list<std::string> values) override
	{
		return *this;
	}

	cpu_thread& run() override
	{
		auto& spu = static_cast<SPUThread&>(*thread);

		spu.run();

		return *this;
	}
};
