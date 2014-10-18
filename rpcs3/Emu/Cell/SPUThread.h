#pragma once
#include "Emu/Memory/atomic_type.h"
#include "PPCThread.h"
#include "Emu/Event.h"
#include "MFC.h"

enum SPUchannels 
{
	SPU_RdEventStat     = 0,  //Read event status with mask applied
	SPU_WrEventMask     = 1,  //Write event mask
	SPU_WrEventAck      = 2,  //Write end of event processing
	SPU_RdSigNotify1    = 3,  //Signal notification 1
	SPU_RdSigNotify2    = 4,  //Signal notification 2
	SPU_WrDec           = 7,  //Write decrementer count
	SPU_RdDec           = 8,  //Read decrementer count
	SPU_RdEventMask     = 11, //Read event mask
	SPU_RdMachStat      = 13, //Read SPU run status
	SPU_WrSRR0          = 14, //Write SPU machine state save/restore register 0 (SRR0)
	SPU_RdSRR0          = 15, //Read SPU machine state save/restore register 0 (SRR0)
	SPU_WrOutMbox       = 28, //Write outbound mailbox contents
	SPU_RdInMbox        = 29, //Read inbound mailbox contents
	SPU_WrOutIntrMbox   = 30, //Write outbound interrupt mailbox contents (interrupting PPU)
};

enum MFCchannels 
{
	MFC_WrMSSyncReq     = 9,  //Write multisource synchronization request
	MFC_RdTagMask       = 12, //Read tag mask
	MFC_LSA             = 16, //Write local memory address command parameter
	MFC_EAH             = 17, //Write high order DMA effective address command parameter
	MFC_EAL             = 18, //Write low order DMA effective address command parameter
	MFC_Size            = 19, //Write DMA transfer size command parameter
	MFC_TagID           = 20, //Write tag identifier command parameter
	MFC_Cmd             = 21, //Write and enqueue DMA command with associated class ID
	MFC_WrTagMask       = 22, //Write tag mask
	MFC_WrTagUpdate     = 23, //Write request for conditional or unconditional tag status update
	MFC_RdTagStat       = 24, //Read tag status with mask applied
	MFC_RdListStallStat = 25, //Read DMA list stall-and-notify status
	MFC_WrListStallAck  = 26, //Write DMA list stall-and-notify acknowledge
	MFC_RdAtomicStat    = 27, //Read completion status of last completed immediate MFC atomic update command
};

enum SPUEvents
{
	SPU_EVENT_MS = 0x1000, // multisource synchronization event
	SPU_EVENT_A  = 0x800, // privileged attention event
	SPU_EVENT_LR = 0x400, // lock line reservation lost event
	SPU_EVENT_S1 = 0x200, // signal notification register 1 available
	SPU_EVENT_S2 = 0x100, // signal notification register 2 available
	SPU_EVENT_LE = 0x80, // SPU outbound mailbox available
	SPU_EVENT_ME = 0x40, // SPU outbound interrupt mailbox available
	SPU_EVENT_TM = 0x20, // SPU decrementer became negative (?)
	SPU_EVENT_MB = 0x10, // SPU inbound mailbox available
	SPU_EVENT_QV = 0x4, // MFC SPU command queue available
	SPU_EVENT_SN = 0x2, // MFC list command stall-and-notify event
	SPU_EVENT_TG = 0x1, // MFC tag-group status update event

	SPU_EVENT_IMPLEMENTED = SPU_EVENT_LR,
};

enum
{
	SPU_RUNCNTL_STOP     = 0,
	SPU_RUNCNTL_RUNNABLE = 1,
};

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
	SYS_SPU_THREAD_BASE_MASK = 0xfffffff,
	SYS_SPU_THREAD_OFFSET    = 0x00100000,
	SYS_SPU_THREAD_SNR1      = 0x05400c,
	SYS_SPU_THREAD_SNR2      = 0x05C00c,
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

#define mmToU64Ptr(x) ((u64*)(&x))
#define mmToU32Ptr(x) ((u32*)(&x))
#define mmToU16Ptr(x) ((u16*)(&x))
#define mmToU8Ptr(x) ((u8*)(&x))

struct g_imm_table_struct
{
	//u16 cntb_table[65536];

	__m128i fsmb_table[65536];
	__m128i fsmh_table[256];
	__m128i fsm_table[16];

	__m128i sldq_pshufb[32];
	__m128i srdq_pshufb[32];
	__m128i rldq_pshufb[16];

	g_imm_table_struct()
	{
		/*static_assert(offsetof(g_imm_table_struct, cntb_table) == 0, "offsetof(cntb_table) != 0");
		for (u32 i = 0; i < sizeof(cntb_table) / sizeof(cntb_table[0]); i++)
		{
		u32 cnt_low = 0, cnt_high = 0;
		for (u32 j = 0; j < 8; j++)
		{
		cnt_low += (i >> j) & 1;
		cnt_high += (i >> (j + 8)) & 1;
		}
		cntb_table[i] = (cnt_high << 8) | cnt_low;
		}*/
		for (u32 i = 0; i < sizeof(fsm_table) / sizeof(fsm_table[0]); i++)
		{

			for (u32 j = 0; j < 4; j++) mmToU32Ptr(fsm_table[i])[j] = (i & (1 << j)) ? ~0 : 0;
		}
		for (u32 i = 0; i < sizeof(fsmh_table) / sizeof(fsmh_table[0]); i++)
		{
			for (u32 j = 0; j < 8; j++) mmToU16Ptr(fsmh_table[i])[j] = (i & (1 << j)) ? ~0 : 0;
		}
		for (u32 i = 0; i < sizeof(fsmb_table) / sizeof(fsmb_table[0]); i++)
		{
			for (u32 j = 0; j < 16; j++) mmToU8Ptr(fsmb_table[i])[j] = (i & (1 << j)) ? ~0 : 0;
		}
		for (u32 i = 0; i < sizeof(sldq_pshufb) / sizeof(sldq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++) mmToU8Ptr(sldq_pshufb[i])[j] = (u8)(j - i);
		}
		for (u32 i = 0; i < sizeof(srdq_pshufb) / sizeof(srdq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++) mmToU8Ptr(srdq_pshufb[i])[j] = (j + i > 15) ? 0xff : (u8)(j + i);
		}
		for (u32 i = 0; i < sizeof(rldq_pshufb) / sizeof(rldq_pshufb[0]); i++)
		{
			for (u32 j = 0; j < 16; j++) mmToU8Ptr(rldq_pshufb[i])[j] = (u8)(j - i) & 0xf;
		}
	}
};

extern const g_imm_table_struct g_imm_table;

//Floating point status and control register.  Unsure if this is one of the GPRs or SPRs
//Is 128 bits, but bits 0-19, 24-28, 32-49, 56-60, 64-81, 88-92, 96-115, 120-124 are unused
class FPSCR
{
public:
	u64 low;
	u64 hi;

	FPSCR() {}

	std::string ToString() const
	{
		return "FPSCR writer not yet implemented"; //fmt::Format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);
	}

	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}
	//slice -> 0 - 1 (4 slices total, only two have rounding)
	//0 -> round even
	//1 -> round towards zero (truncate)
	//2 -> round towards positive inf
	//3 -> round towards neg inf
	void setSliceRounding(u8 slice, u8 roundTo)
	{
		u64 mask = roundTo;
		switch(slice)
		{
		case 0:
			mask = mask << 20;
			break;
		case 1:
			mask = mask << 22;
			break;	
		}

		//rounding is located in the low end of the FPSCR
		this->low = this->low & mask;
	}
	//Slice 0 or 1
	u8 checkSliceRounding(u8 slice) const
	{
		switch(slice)
		{
		case 0:
			return this->low >> 20 & 0x3;
		
		case 1:
			return this->low >> 22 & 0x3;

		default:
			throw fmt::Format("Unexpected slice value in FPSCR::checkSliceRounding(): %d", slice);
			return 0;
		}
	}

	//Single Precision Exception Flags (all 3 slices)
	//slice -> slice number (0-3)
	//exception: 1 -> Overflow 2 -> Underflow 4-> Diff (could be IE^3 non compliant)
	void setSinglePrecisionExceptionFlags(u8 slice, u8 exception)
	{
		u64 mask = exception;
		switch(slice)
		{
		case 0:
			mask = mask << 29;
			this->low = this->low & mask;
			break;
		case 1: 
			mask = mask << 61;
			this->low = this->low & mask;
			break;
		case 2:
			mask = mask << 29;
			this->hi = this->hi & mask;
			break;
		case 3:
			mask = mask << 61;
			this->hi = this->hi & mask;
			break;
		}
		
	}
	
};

union SPU_SNRConfig_hdr
{
	u64 value;

	SPU_SNRConfig_hdr() {}

	std::string ToString() const
	{
		return fmt::Format("%01x", value);
	}

	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}
};

struct SpuGroupInfo;

class SPUThread : public PPCThread
{
public:
	u128 GPR[128]; // General-Purpose Registers
	//FPSCR FPSCR;
	SPU_SNRConfig_hdr cfg; // Signal Notification Registers Configuration (OR-mode enabled: 0x1 for SNR1, 0x2 for SNR2)

	u64 R_ADDR; // reservation address
	u64 R_DATA[16]; // lock line data (BE)

	EventPort SPUPs[64]; // SPU Thread Event Ports
	EventManager SPUQs; // SPU Queue Mapping
	SpuGroupInfo* group; // associated SPU Thread Group (null for raw spu)

	u64 m_dec_start; // timestamp of writing decrementer value
	u32 m_dec_value; // written decrementer value

	u32 m_event_mask;
	u32 m_events;

	struct IntrTag
	{
		u32 enabled; // 1 == true
		u32 thread; // established interrupt PPU thread
		u64 mask;
		u64 stat;

		IntrTag()
			: enabled(0)
			, thread(0)
			, mask(0)
			, stat(0)
		{
		}
	} m_intrtag[3];

	// limited lock-free queue, most functions are barrier-free
	template<size_t max_count>
	class Channel
	{
		static_assert(max_count >= 1, "Invalid channel count");

		struct ChannelData
		{
			u32 value;
			u32 is_set;
		};

		atomic_t<ChannelData> m_data[max_count];
		size_t m_push;
		size_t m_pop;

	public:
		__noinline Channel()
		{
			for (size_t i = 0; i < max_count; i++)
			{
				m_data[i].write_relaxed({});
			}
			m_push = 0;
			m_pop = 0;
		}

		__forceinline void PopUncond(u32& res)
		{
			res = m_data[m_pop].read_relaxed().value;
			m_data[m_pop].write_relaxed({});
			m_pop = (m_pop + 1) % max_count;
		}

		__forceinline bool Pop(u32& res)
		{
			const auto data = m_data[m_pop].read_relaxed();
			if (data.is_set)
			{
				res = data.value;
				m_data[m_pop].write_relaxed({});
				m_pop = (m_pop + 1) % max_count;
				return true;
			}
			else
			{
				return false;
			}
		}

		__forceinline bool Pop_XCHG(u32& res) // not barrier-free, not tested
		{
			const auto data = m_data[m_pop].exchange({});
			if (data.is_set)
			{
				res = data.value;
				m_pop = (m_pop + 1) % max_count;
				return true;
			}
			else
			{
				return false;
			}
		}

		__forceinline void PushUncond_OR(const u32 value) // not barrier-free, not tested
		{
			m_data[m_push]._or({ value, 1 });
			m_push = (m_push + 1) % max_count;
		}

		__forceinline void PushUncond(const u32 value)
		{
			m_data[m_push].write_relaxed({ value, 1 });
			m_push = (m_push + 1) % max_count;
		}

		__forceinline bool Push(const u32 value)
		{
			if (m_data[m_push].read_relaxed().is_set)
			{
				return false;
			}
			else
			{
				PushUncond(value);
				return true;
			}
		}

		__forceinline u32 GetCount() const
		{
			u32 res = 0;
			for (size_t i = 0; i < max_count; i++)
			{
				res += m_data[i].read_relaxed().is_set ? 1 : 0;
			}
			return res;
		}

		__forceinline u32 GetFreeCount() const
		{
			u32 res = 0;
			for (size_t i = 0; i < max_count; i++)
			{
				res += m_data[i].read_relaxed().is_set ? 0 : 1;
			}
			return res;
		}

		__forceinline void SetValue(const u32 value)
		{
			m_data[m_push].direct_op([value](ChannelData& v)
			{
				v.value = value;
			});
		}

		__forceinline u32 GetValue() const
		{
			return m_data[m_pop].read_relaxed().value;
		}
	};

	struct MFCReg
	{
		Channel<1> LSA;
		Channel<1> EAH;
		Channel<1> EAL;
		Channel<1> Size_Tag;
		Channel<1> CMDStatus;
		Channel<1> QueryType; // only for prxy
		Channel<1> QueryMask;
		Channel<1> TagStatus;
		Channel<1> AtomicStat;
	} MFC1, MFC2;

	struct StalledList
	{
		u32 lsa;
		u64 ea;
		u16 tag;
		u16 size;
		u32 cmd;
		MFCReg* MFCArgs;

		StalledList()
			: MFCArgs(nullptr)
		{
		}
	} StallList[32];
	Channel<1> StallStat;

	struct
	{
		Channel<1> Out_MBox;
		Channel<1> Out_IntrMBox;
		Channel<4> In_MBox;
		Channel<1> Status;
		Channel<1> NPC;
		Channel<1> SNR[2];
	} SPU;

	void WriteSNR(bool number, u32 value);

	u32 LSA;

	union
	{
		u64 EA;
		struct { u32 EAH, EAL; };
	};

	u32 ls_offset;

	void ProcessCmd(u32 cmd, u32 tag, u32 lsa, u64 ea, u32 size);

	void ListCmd(u32 lsa, u64 ea, u16 tag, u16 size, u32 cmd, MFCReg& MFCArgs);

	void EnqMfcCmd(MFCReg& MFCArgs);

	bool CheckEvents();

	u32 GetChannelCount(u32 ch);

	void WriteChannel(u32 ch, const u128& r);

	void ReadChannel(u128& r, u32 ch);

	void StopAndSignal(u32 code);

	u8   ReadLS8  (const u32 lsa) const { return vm::read8  (lsa + m_offset); }
	u16  ReadLS16 (const u32 lsa) const { return vm::read16 (lsa + m_offset); }
	u32  ReadLS32 (const u32 lsa) const { return vm::read32 (lsa + m_offset); }
	u64  ReadLS64 (const u32 lsa) const { return vm::read64 (lsa + m_offset); }
	u128 ReadLS128(const u32 lsa) const { return vm::read128(lsa + m_offset); }

	void WriteLS8  (const u32 lsa, const u8&   data) const { vm::write8  (lsa + m_offset, data); }
	void WriteLS16 (const u32 lsa, const u16&  data) const { vm::write16 (lsa + m_offset, data); }
	void WriteLS32 (const u32 lsa, const u32&  data) const { vm::write32 (lsa + m_offset, data); }
	void WriteLS64 (const u32 lsa, const u64&  data) const { vm::write64 (lsa + m_offset, data); }
	void WriteLS128(const u32 lsa, const u128& data) const { vm::write128(lsa + m_offset, data); }

	std::function<void(SPUThread& SPU)> m_custom_task;
	std::function<u64(SPUThread& SPU)> m_code3_func;

public:
	SPUThread(CPUThreadType type = CPU_THREAD_SPU);
	virtual ~SPUThread();

	virtual std::string RegsToString()
	{
		std::string ret = "Registers:\n=========\n";

		for(uint i=0; i<128; ++i) ret += fmt::Format("GPR[%d] = 0x%s\n", i, GPR[i].to_hex().c_str());

		return ret;
	}

	virtual std::string ReadRegString(const std::string& reg)
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

public:
	virtual void InitRegs();
	virtual void Task();
	void FastCall(u32 ls_addr);
	void FastStop();

protected:
	virtual void DoReset();
	virtual void DoRun();
	virtual void DoPause();
	virtual void DoResume();
	virtual void DoStop();
	virtual void DoClose();
};

SPUThread& GetCurrentSPUThread();
