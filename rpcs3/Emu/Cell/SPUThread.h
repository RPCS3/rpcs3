#pragma once
#include "PPCThread.h"
#include "Emu/event.h"
#include "MFC.h"
#include <mutex>

extern std::mutex g_SyncMutex; //can provide compatability for CellSyncMutex through SPU<>PPU and SPU<>SPU

static const char* spu_reg_name[128] =
{
	"$LR",  "$SP",  "$2",   "$3",   "$4",   "$5",   "$6",   "$7",
	"$8",   "$9",   "$10",  "$11",  "$12",  "$13",  "$14",  "$15",
	"$16",  "$17",  "$18",  "$19",  "$20",  "$21",  "$22",  "$23",
	"$24",  "$25",  "$26",  "$27",  "$28",  "$29",  "$30",  "$31",
	"$32",  "$33",  "$34",  "$35",  "$36",  "$37",  "$38",  "$39",
	"$40",  "$41",  "$42",  "$43",  "$44",  "$45",  "$46",  "$47",
	"$48",  "$49",  "$50",  "$51",  "$52",  "$53",  "$54",  "$55",
	"$56",  "$57",  "$58",  "$59",  "$60",  "$61",  "$62",  "$63",
	"$64",  "$65",  "$66",  "$67",  "$68",  "$69",  "$70",  "$71",
	"$72",  "$73",  "$74",  "$75",  "$76",  "$77",  "$78",  "$79",
	"$80",  "$81",  "$82",  "$83",  "$84",  "$85",  "$86",  "$87",
	"$88",  "$89",  "$90",  "$91",  "$92",  "$93",  "$94",  "$95",
	"$96",  "$97",  "$98",  "$99",  "$100", "$101", "$102", "$103",
	"$104", "$105", "$106", "$107", "$108", "$109", "$110", "$111",
	"$112", "$113", "$114", "$115", "$116", "$117", "$118", "$119",
	"$120", "$121", "$122", "$123", "$124", "$125", "$126", "$127",
};
//SPU reg $0 is a dummy reg, and is used for certain instructions.
static const char* spu_specialreg_name[128] = {
	"$0",  "$1",  "$2",   "$3",   "$4",   "$5",   "$6",   "$7",
	"$8",   "$9",   "$10",  "$11",  "$12",  "$13",  "$14",  "$15",
	"$16",  "$17",  "$18",  "$19",  "$20",  "$21",  "$22",  "$23",
	"$24",  "$25",  "$26",  "$27",  "$28",  "$29",  "$30",  "$31",
	"$32",  "$33",  "$34",  "$35",  "$36",  "$37",  "$38",  "$39",
	"$40",  "$41",  "$42",  "$43",  "$44",  "$45",  "$46",  "$47",
	"$48",  "$49",  "$50",  "$51",  "$52",  "$53",  "$54",  "$55",
	"$56",  "$57",  "$58",  "$59",  "$60",  "$61",  "$62",  "$63",
	"$64",  "$65",  "$66",  "$67",  "$68",  "$69",  "$70",  "$71",
	"$72",  "$73",  "$74",  "$75",  "$76",  "$77",  "$78",  "$79",
	"$80",  "$81",  "$82",  "$83",  "$84",  "$85",  "$86",  "$87",
	"$88",  "$89",  "$90",  "$91",  "$92",  "$93",  "$94",  "$95",
	"$96",  "$97",  "$98",  "$99",  "$100", "$101", "$102", "$103",
	"$104", "$105", "$106", "$107", "$108", "$109", "$110", "$111",
	"$112", "$113", "$114", "$115", "$116", "$117", "$118", "$119",
	"$120", "$121", "$122", "$123", "$124", "$125", "$126", "$127",
};

static const char* spu_ch_name[128] =
{
	"$SPU_RdEventStat", "$SPU_WrEventMask", "$SPU_WrEventAck", "$SPU_RdSigNotify1",
	"$SPU_RdSigNotify2", "$ch5",  "$ch6",  "$SPU_WrDec", "$SPU_RdDec",
	"$MFC_WrMSSyncReq",   "$ch10",  "$SPU_RdEventMask",  "$MFC_RdTagMask",  "$SPU_RdMachStat",
	"$SPU_WrSRR0",  "$SPU_RdSRR0",  "$MFC_LSA", "$MFC_EAH",  "$MFC_EAL",  "$MFC_Size",
	"$MFC_TagID",  "$MFC_Cmd",  "$MFC_WrTagMask",  "$MFC_WrTagUpdate",  "$MFC_RdTagStat",
	"$MFC_RdListStallStat",  "$MFC_WrListStallAck",  "$MFC_RdAtomicStat",
	"$SPU_WrOutMbox", "$SPU_RdInMbox", "$SPU_WrOutIntrMbox", "$ch31",  "$ch32",
	"$ch33",  "$ch34",  "$ch35",  "$ch36",  "$ch37",  "$ch38",  "$ch39",  "$ch40",
	"$ch41",  "$ch42",  "$ch43",  "$ch44",  "$ch45",  "$ch46",  "$ch47",  "$ch48",
	"$ch49",  "$ch50",  "$ch51",  "$ch52",  "$ch53",  "$ch54",  "$ch55",  "$ch56",
	"$ch57",  "$ch58",  "$ch59",  "$ch60",  "$ch61",  "$ch62",  "$ch63",  "$ch64",
	"$ch65",  "$ch66",  "$ch67",  "$ch68",  "$ch69",  "$ch70",  "$ch71",  "$ch72",
	"$ch73",  "$ch74",  "$ch75",  "$ch76",  "$ch77",  "$ch78",  "$ch79",  "$ch80",
	"$ch81",  "$ch82",  "$ch83",  "$ch84",  "$ch85",  "$ch86",  "$ch87",  "$ch88",
	"$ch89",  "$ch90",  "$ch91",  "$ch92",  "$ch93",  "$ch94",  "$ch95",  "$ch96",
	"$ch97",  "$ch98",  "$ch99",  "$ch100", "$ch101", "$ch102", "$ch103", "$ch104",
	"$ch105", "$ch106", "$ch107", "$ch108", "$ch109", "$ch110", "$ch111", "$ch112",
	"$ch113", "$ch114", "$ch115", "$ch116", "$ch117", "$ch118", "$ch119", "$ch120",
	"$ch121", "$ch122", "$ch123", "$ch124", "$ch125", "$ch126", "$ch127",
};

enum SPUchannels 
{
	SPU_RdEventStat		= 0,	//Read event status with mask applied
	SPU_WrEventMask		= 1,	//Write event mask
	SPU_WrEventAck		= 2,	//Write end of event processing
	SPU_RdSigNotify1	= 3,	//Signal notification 1
	SPU_RdSigNotify2	= 4,	//Signal notification 2
	SPU_WrDec			= 7,	//Write decrementer count
	SPU_RdDec			= 8,	//Read decrementer count
	SPU_RdEventMask		= 11,	//Read event mask
	SPU_RdMachStat		= 13,	//Read SPU run status
	SPU_WrSRR0			= 14,	//Write SPU machine state save/restore register 0 (SRR0)
	SPU_RdSRR0			= 15,	//Read SPU machine state save/restore register 0 (SRR0)
	SPU_WrOutMbox		= 28,	//Write outbound mailbox contents
	SPU_RdInMbox		= 29,	//Read inbound mailbox contents
	SPU_WrOutIntrMbox	= 30,	//Write outbound interrupt mailbox contents (interrupting PPU)
};

enum MFCchannels 
{
	MFC_WrMSSyncReq		= 9,	//Write multisource synchronization request
	MFC_RdTagMask		= 12,	//Read tag mask
	MFC_LSA				= 16,	//Write local memory address command parameter
	MFC_EAH				= 17,	//Write high order DMA effective address command parameter
	MFC_EAL				= 18,	//Write low order DMA effective address command parameter
	MFC_Size			= 19,	//Write DMA transfer size command parameter
	MFC_TagID			= 20,	//Write tag identifier command parameter
	MFC_Cmd				= 21,	//Write and enqueue DMA command with associated class ID
	MFC_WrTagMask		= 22,	//Write tag mask
	MFC_WrTagUpdate		= 23,	//Write request for conditional or unconditional tag status update
	MFC_RdTagStat		= 24,	//Read tag status with mask applied
	MFC_RdListStallStat = 25,	//Read DMA list stall-and-notify status
	MFC_WrListStallAck	= 26,	//Write DMA list stall-and-notify acknowledge
	MFC_RdAtomicStat	= 27,	//Read completion status of last completed immediate MFC atomic update command
};

enum
{
	SPU_RUNCNTL_STOP		= 0,
	SPU_RUNCNTL_RUNNABLE	= 1,
};

enum
{
	SPU_STATUS_STOPPED				= 0x0,
	SPU_STATUS_RUNNING				= 0x1,
	SPU_STATUS_STOPPED_BY_STOP		= 0x2,
	SPU_STATUS_STOPPED_BY_HALT		= 0x4,
	SPU_STATUS_WAITING_FOR_CHANNEL	= 0x8,
	SPU_STATUS_SINGLE_STEP			= 0x10,
};

//Floating point status and control register.  Unsure if this is one of the GPRs or SPRs
//Is 128 bits, but bits 0-19, 24-28, 32-49, 56-60, 64-81, 88-92, 96-115, 120-124 are unused
class FPSCR
{
public:
	u64 low;
	u64 hi;

	FPSCR() {}

	wxString ToString() const
	{
		return "FPSCR writer not yet implemented"; //wxString::Format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);
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
	u8 checkSliceRounding(u8 slice)
	{
		switch(slice)
		{
		case 0:
			return this->low >> 20 & 0x3;
		
		case 1:
			return this->low >> 22 & 0x3;
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

union SPU_GPR_hdr
{
	u128 _u128;
	s128 _i128;
	__m128 _m128;
	__m128i _m128i;
	u64 _u64[2];
	s64 _i64[2];
	u32 _u32[4];
	s32 _i32[4];
	u16 _u16[8];
	s16 _i16[8];
	u8  _u8[16];
	s8  _i8[16];
	double _d[2];
	float _f[4];

	SPU_GPR_hdr() {}

	wxString ToString() const
	{
		return wxString::Format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);
	}

	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}
};

union SPU_SPR_hdr
{
	u128 _u128;
	s128 _i128;
	u32 _u32[4];

	SPU_SPR_hdr() {}

	wxString ToString() const
	{
		return wxString::Format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);
	}

	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}
};

union SPU_SNRConfig_hdr
{
	u64 value;

	SPU_SNRConfig_hdr() {}

	wxString ToString() const
	{
		return wxString::Format("%01x", value);
	}

	void Reset()
	{
		memset(this, 0, sizeof(*this));
	}
};

class SPUThread : public PPCThread
{
public:
	SPU_GPR_hdr GPR[128]; //General-Purpose Register
	SPU_SPR_hdr SPR[128]; //Special-Purpose Registers
	FPSCR FPSCR;
	SPU_SNRConfig_hdr cfg; //Signal Notification Registers Configuration (OR-mode enabled: 0x1 for SNR1, 0x2 for SNR2)

	template<size_t _max_count>
	class Channel
	{
	public:
		static const size_t max_count = _max_count;
#ifdef _M_X64
		static const bool x86 = false;
#else
		static const bool x86 = true;
#endif

	private:
		union _CRT_ALIGN(8) {
			struct {
				volatile u32 m_index;
				u32 m_value[max_count];
			};
			volatile u64 m_indval;
		};
		std::mutex m_lock;

	public:

		Channel()
		{
			Init();
		}

		void Init()
		{
			m_index = 0;
		}

		__forceinline bool Pop(u32& res)
		{
			if (max_count > 1 || x86)
			{
				std::lock_guard<std::mutex> lock(m_lock);
				if(!m_index) 
				{
					return false;
				}
				res = m_value[--m_index];
				m_value[m_index] = 0;
				return true;
			}
			else
			{ //lock-free
				if ((m_indval & 0xffffffff) == 0)
					return false;
				else
				{
					res = (m_indval >> 32);
					m_indval = 0;
					return true;
				}				
			}
		}

		__forceinline bool Push(u32 value)
		{
			if (max_count > 1 || x86)
			{
				std::lock_guard<std::mutex> lock(m_lock);
				if(m_index >= max_count) 
				{
					return false;
				}
				m_value[m_index++] = value;
				return true;
			}
			else
			{ //lock-free
				if (m_indval & 0xffffffff)
					return false;
				else
				{
					const u64 new_value = ((u64)value << 32) | 1;
					m_indval = new_value;
					return true;
				}
			}
		}

		__forceinline void PushUncond(u32 value)
		{
			if (max_count > 1 || x86)
			{
				std::lock_guard<std::mutex> lock(m_lock);
				if(m_index >= max_count) 
					m_value[max_count-1] = value; //last message is overwritten
				else
					m_value[m_index++] = value;
			}
			else
			{ //lock-free
				const u64 new_value = ((u64)value << 32) | 1;
				m_indval = new_value;
			}
		}

		__forceinline void PushUncond_OR(u32 value)
		{
			if (max_count > 1 || x86)
			{
				std::lock_guard<std::mutex> lock(m_lock);
				if(m_index >= max_count) 
					m_value[max_count-1] |= value; //last message is logically ORed
				else
					m_value[m_index++] = value;
			}
			else
			{
#ifdef _M_X64
				_InterlockedOr64((volatile __int64*)m_indval, ((u64)value << 32) | 1);
#else
				ConLog.Error("PushUncond_OR(): no code compiled");
#endif
			}
		}

		__forceinline void PopUncond(u32& res)
		{
			if (max_count > 1 || x86)
			{
				std::lock_guard<std::mutex> lock(m_lock);
				if(!m_index) 
					res = 0; //result is undefined
				else
				{
					res = m_value[--m_index];
					m_value[m_index] = 0;
				}
			}
			else
			{ //lock-free
				if(!m_index)
					res = 0;
				else
				{
					res = (m_indval >> 32);
					m_indval = 0;
				}
			}
		}

		__forceinline u32 GetCount()
		{
			if (max_count > 1 || x86)
			{
				std::lock_guard<std::mutex> lock(m_lock);
				return m_index;
			}
			else
			{
				return m_index;
			}
		}

		__forceinline u32 GetFreeCount()
		{
			if (max_count > 1 || x86)
			{
				std::lock_guard<std::mutex> lock(m_lock);
				return max_count - m_index;
			}
			else
			{
				return max_count - m_index;
			}
		}

		void SetValue(u32 value)
		{
			m_value[0] = value;
		}

		u32 GetValue() const
		{
			return m_value[0];
		}
	};

	struct MFCReg
	{
		Channel<1> LSA;
		Channel<1> EAH;
		Channel<1> EAL;
		Channel<1> Size_Tag;
		Channel<1> CMDStatus;
		Channel<1> QStatus;
	} MFC1, MFC2;

	struct
	{
		Channel<1> QueryType;
		Channel<1> QueryMask;
		Channel<1> TagStatus;
		Channel<1> AtomicStat;
	} Prxy;

	struct
	{
		Channel<1> Out_MBox;
		Channel<1> OutIntr_Mbox;
		Channel<4> In_MBox;
		Channel<1> MBox_Status;
		Channel<1> RunCntl;
		Channel<1> Status;
		Channel<1> NPC;
		Channel<1> SNR[2];
	} SPU;

	u32 LSA;

	union
	{
		u64 EA;
		struct { u32 EAH, EAL; };
	};

	DMAC dmac;

	void EnqMfcCmd(MFCReg& MFCArgs)
	{
		u32 cmd = MFCArgs.CMDStatus.GetValue();
		u16 op = cmd & MFC_MASK_CMD;

		u32 lsa = MFCArgs.LSA.GetValue();
		u64 ea = (u64)MFCArgs.EAL.GetValue() | ((u64)MFCArgs.EAH.GetValue() << 32);
		u32 size_tag = MFCArgs.Size_Tag.GetValue();
		u16 tag = (u16)size_tag;
		u16 size = size_tag >> 16;

		switch(op & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK))
		{
		case MFC_PUT_CMD:
		case MFC_GET_CMD:
		{
			if (enable_log)	ConLog.Write("DMA %s%s%s: lsa = 0x%x, ea = 0x%llx, tag = 0x%x, size = 0x%x, cmd = 0x%x", 
				op & MFC_PUT_CMD ? "PUT" : "GET",
				op & MFC_BARRIER_MASK ? "B" : "", 
				op & MFC_FENCE_MASK ? "F" : "", 
				lsa, ea, tag, size, cmd);
			MFCArgs.CMDStatus.SetValue(dmac.Cmd(cmd, tag, lsa, ea, size));
		}
		break;

		case MFC_GETLLAR_CMD:
		case MFC_PUTLLC_CMD:
		case MFC_PUTLLUC_CMD:
		{
			if (op == MFC_GETLLAR_CMD) 
			{
				g_SyncMutex.lock();
			}
			
			ConLog.Warning("DMA %s: lsa=0x%x, ea = 0x%llx, (tag) = 0x%x, (size) = 0x%x, cmd = 0x%x",
				op == MFC_GETLLAR_CMD ? "GETLLAR" : op == MFC_PUTLLC_CMD ? "PUTLLC" : "PUTLLUC",
				lsa, ea, tag, size, cmd);

			dmac.ProcessCmd(op == MFC_GETLLAR_CMD ? MFC_GET_CMD : MFC_PUT_CMD, tag, lsa, ea, 128);
			Prxy.AtomicStat.PushUncond(op == MFC_GETLLAR_CMD ? MFC_GETLLAR_SUCCESS : op == MFC_PUTLLC_CMD ? MFC_PUTLLC_SUCCESS : MFC_PUTLLUC_SUCCESS);

			if (op == MFC_PUTLLC_CMD || op == MFC_PUTLLUC_CMD) 
			{
				g_SyncMutex.unlock();
			}
		}
		break;

		default:
			ConLog.Error("Unknown MFC cmd. (opcode=0x%x, cmd=0x%x, lsa = 0x%x, ea = 0x%llx, tag = 0x%x, size = 0x%x)", 
				op, cmd, lsa, ea, tag, size);
		break;
		}
	}

	u32 GetChannelCount(u32 ch)
	{
		switch(ch)
		{
		case SPU_WrOutMbox:
			return SPU.Out_MBox.GetFreeCount();

		case SPU_RdInMbox:
			return SPU.In_MBox.GetCount();

		case SPU_WrOutIntrMbox:
			ConLog.Warning("GetChannelCount(%s) = 0", spu_ch_name[ch]);
			return 0;//return SPU.OutIntr_Mbox.GetFreeCount();

		case MFC_RdTagStat:
			return Prxy.TagStatus.GetCount();

		case SPU_RdSigNotify1:
			return SPU.SNR[0].GetCount();

		case SPU_RdSigNotify2:
			return SPU.SNR[1].GetCount();

		case MFC_RdAtomicStat:
			return Prxy.AtomicStat.GetCount();

		default:
			ConLog.Error("%s error: unknown/illegal channel (%d [%s]).", __FUNCTION__, ch, spu_ch_name[ch]);
		break;
		}

		return 0;
	}

	void WriteChannel(u32 ch, const SPU_GPR_hdr& r)
	{
		const u32 v = r._u32[3];

		switch(ch)
		{
		case SPU_WrOutIntrMbox:
			ConLog.Warning("%s: %s = 0x%x", __FUNCTION__, spu_ch_name[ch], v);
			while (!SPU.OutIntr_Mbox.Push(v) && !Emu.IsStopped()) Sleep(1);
		break;

		case SPU_WrOutMbox:
			ConLog.Warning("%s: %s = 0x%x", __FUNCTION__, spu_ch_name[ch], v);
			while (!SPU.Out_MBox.Push(v) && !Emu.IsStopped()) Sleep(1);
		break;

		case MFC_WrTagMask:
			//ConLog.Warning("%s: %s = 0x%x", __FUNCTION__, spu_ch_name[ch], v);
			Prxy.QueryMask.SetValue(v);
		break;

		case MFC_WrTagUpdate:
			//ConLog.Warning("%s: %s = 0x%x", __FUNCTION__, spu_ch_name[ch], v);
			Prxy.TagStatus.PushUncond(Prxy.QueryMask.GetValue());
		break;

		case MFC_LSA:
			MFC1.LSA.SetValue(v);
		break;

		case MFC_EAH:
			MFC1.EAH.SetValue(v);
		break;

		case MFC_EAL:
			MFC1.EAL.SetValue(v);
		break;

		case MFC_Size:
			MFC1.Size_Tag.SetValue((MFC1.Size_Tag.GetValue() & 0xffff) | (v << 16));
		break;

		case MFC_TagID:
			MFC1.Size_Tag.SetValue((MFC1.Size_Tag.GetValue() & ~0xffff) | (v & 0xffff));
		break;

		case MFC_Cmd:
			MFC1.CMDStatus.SetValue(v);
			EnqMfcCmd(MFC1);
		break;

		default:
			ConLog.Error("%s error: unknown/illegal channel (%d [%s]).", __FUNCTION__, ch, spu_ch_name[ch]);
		break;
		}
	}

	void ReadChannel(SPU_GPR_hdr& r, u32 ch)
	{
		r.Reset();
		u32& v = r._u32[3];

		switch(ch)
		{
		case SPU_RdInMbox:
			while (!SPU.In_MBox.Pop(v) && !Emu.IsStopped()) Sleep(1);
			ConLog.Warning("%s: 0x%x = %s", __FUNCTION__, v, spu_ch_name[ch]);
		break;

		case MFC_RdTagStat:
			while (!Prxy.TagStatus.Pop(v) && !Emu.IsStopped()) Sleep(1);
			//ConLog.Warning("%s: 0x%x = %s", __FUNCTION__, v, spu_ch_name[ch]);
		break;

		case SPU_RdSigNotify1:
			while (!SPU.SNR[0].Pop(v) && !Emu.IsStopped()) Sleep(1);
			//ConLog.Warning("%s: 0x%x = %s", __FUNCTION__, v, spu_ch_name[ch]);
		break;

		case SPU_RdSigNotify2:
			while (!SPU.SNR[1].Pop(v) && !Emu.IsStopped()) Sleep(1);
			//ConLog.Warning("%s: 0x%x = %s", __FUNCTION__, v, spu_ch_name[ch]);
		break;

		case MFC_RdAtomicStat:
			while (!Prxy.AtomicStat.Pop(v) && !Emu.IsStopped()) Sleep(1);
		break;

		default:
			ConLog.Error("%s error: unknown/illegal channel (%d [%s]).", __FUNCTION__, ch, spu_ch_name[ch]);
		break;
		}
	}

	bool IsGoodLSA(const u32 lsa) const { return Memory.IsGoodAddr(lsa + m_offset) && lsa < 0x40000; }
	virtual u8   ReadLS8  (const u32 lsa) const { return Memory.Read8  (lsa + (m_offset & 0x3fffc)); }
	virtual u16  ReadLS16 (const u32 lsa) const { return Memory.Read16 (lsa + m_offset); }
	virtual u32  ReadLS32 (const u32 lsa) const { return Memory.Read32 (lsa + m_offset); }
	virtual u64  ReadLS64 (const u32 lsa) const { return Memory.Read64 (lsa + m_offset); }
	virtual u128 ReadLS128(const u32 lsa) const { return Memory.Read128(lsa + m_offset); }

	virtual void WriteLS8  (const u32 lsa, const u8&   data) const { Memory.Write8  (lsa + m_offset, data); }
	virtual void WriteLS16 (const u32 lsa, const u16&  data) const { Memory.Write16 (lsa + m_offset, data); }
	virtual void WriteLS32 (const u32 lsa, const u32&  data) const { Memory.Write32 (lsa + m_offset, data); }
	virtual void WriteLS64 (const u32 lsa, const u64&  data) const { Memory.Write64 (lsa + m_offset, data); }
	virtual void WriteLS128(const u32 lsa, const u128& data) const { Memory.Write128(lsa + m_offset, data); }

public:
	SPUThread(CPUThreadType type = CPU_THREAD_SPU);
	virtual ~SPUThread();

	virtual wxString RegsToString()
	{
		wxString ret = "Registers:\n=========\n";

		for(uint i=0; i<128; ++i) ret += wxString::Format("GPR[%d] = 0x%s\n", i, GPR[i].ToString().mb_str());

		return ret;
	}

	virtual wxString ReadRegString(wxString reg)
	{
		if (reg.Contains("["))
		{
			long reg_index;
			reg.AfterFirst('[').RemoveLast().ToLong(&reg_index);
			if (reg.StartsWith("GPR")) return wxString::Format("%016llx%016llx",  GPR[reg_index]._u64[1], GPR[reg_index]._u64[0]);
		}
		return wxEmptyString;
	}

	bool WriteRegString(wxString reg, wxString value)
	{
		while (value.Len() < 32) value = "0"+value;
		if (reg.Contains("["))
		{
			long reg_index;
			reg.AfterFirst('[').RemoveLast().ToLong(&reg_index);
			if (reg.StartsWith("GPR"))
			{
				unsigned long long reg_value0;
				unsigned long long reg_value1;
				if (!value.SubString(16,31).ToULongLong(&reg_value0, 16)) return false;
				if (!value.SubString(0,15).ToULongLong(&reg_value1, 16)) return false;
				GPR[reg_index]._u64[0] = (u64)reg_value0;
				GPR[reg_index]._u64[1] = (u64)reg_value1;
				return true;
			}
		}
		return false;
	}

public:
	virtual void InitRegs();
	virtual u64 GetFreeStackSize() const;

protected:
	virtual void DoReset();
	virtual void DoRun();
	virtual void DoPause();
	virtual void DoResume();
	virtual void DoStop();
};

SPUThread& GetCurrentSPUThread();
