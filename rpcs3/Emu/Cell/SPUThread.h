#pragma once
#include "PPCThread.h"
#include "Emu/event.h"

static const wxString spu_reg_name[128] =
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

static const wxString spu_ch_name[128] =
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

union SPU_GPR_hdr
{
	u128 _u128;
	s128 _i128;
	u64 _u64[2];
	s64 _i64[2];
	u32 _u32[4];
	s32 _i32[4];
	u16 _u16[8];
	s16 _i16[8];
	u8  _u8[16];
	s8  _i8[16];

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

class SPUThread : public PPCThread
{
public:
	SPU_GPR_hdr GPR[128]; //General-Purpose Register
	SizedStack<u32, 1> OutMbox;
	SizedStack<u32, 1> OutIntrMbox;
	SizedStack<u32, 4> InMbox;

	u32 LSA;

	union
	{
		u64 EA;
		struct { u32 EAH, EAL; };
	};

	u32 GetChannelCount(u32 ch)
	{
		switch(ch)
		{
		case SPU_WrOutMbox:
			return OutMbox.GetFreeCount();

		case SPU_RdInMbox:
			return InMbox.GetCount();

		case SPU_WrOutIntrMbox:
			return 0;//return OutIntrMbox.GetFreeCount();

		default:
			ConLog.Error("%s error: unknown/illegal channel (%d).", __FUNCTION__, ch);
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
			ConLog.Warning("SPU_WrOutIntrMbox = 0x%x", v);
			if(!OutIntrMbox.Push(v))
			{
				ConLog.Warning("Not enought free rooms.");
			}
		break;

		case SPU_WrOutMbox:
			ConLog.Warning("SPU_WrOutMbox = 0x%x", v);
			if(!OutMbox.Push(v))
			{
				ConLog.Warning("Not enought free rooms.");
			}
		break;

		default:
			ConLog.Error("%s error: unknown/illegal channel (%d).", __FUNCTION__, ch);
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
			if(!InMbox.Pop(v)) v = 0;
		break;

		default:
			ConLog.Error("%s error: unknown/illegal channel (%d).", __FUNCTION__, ch);
		break;
		}
	}

	bool IsGoodLSA(const u32 lsa) const { return Memory.IsGoodAddr(lsa); }
	u8   ReadLS8  (const u32 lsa) const { return Memory.Read8  (lsa + (m_offset & 0x3fffc)); }
	u16  ReadLS16 (const u32 lsa) const { return Memory.Read16 (lsa + m_offset); }
	u32  ReadLS32 (const u32 lsa) const { return Memory.Read32 (lsa + m_offset); }
	u64  ReadLS64 (const u32 lsa) const { return Memory.Read64 (lsa + m_offset); }
	u128 ReadLS128(const u32 lsa) const { return Memory.Read128(lsa + m_offset); }

	void WriteLS8  (const u32 lsa, const u8&   data) const { Memory.Write8  (lsa + m_offset, data); }
	void WriteLS16 (const u32 lsa, const u16&  data) const { Memory.Write16 (lsa + m_offset, data); }
	void WriteLS32 (const u32 lsa, const u32&  data) const { Memory.Write32 (lsa + m_offset, data); }
	void WriteLS64 (const u32 lsa, const u64&  data) const { Memory.Write64 (lsa + m_offset, data); }
	void WriteLS128(const u32 lsa, const u128& data) const { Memory.Write128(lsa + m_offset, data); }

public:
	SPUThread();
	~SPUThread();

	virtual wxString RegsToString()
	{
		wxString ret = PPCThread::RegsToString();
		for(uint i=0; i<128; ++i) ret += wxString::Format("GPR[%d] = 0x%s\n", i, GPR[i].ToString());
		return ret;
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

private:
	virtual void DoCode(const s32 code);
};

SPUThread& GetCurrentSPUThread();