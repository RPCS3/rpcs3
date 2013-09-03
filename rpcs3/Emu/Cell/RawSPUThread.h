#pragma once
#include "SPUThread.h"
#include "Emu/event.h"

enum
{
	MFC_LSA_offs			= 0x3004,
	MFC_EAH_offs			= 0x3008,
	MFC_EAL_offs			= 0x300C,
	MFC_Size_Tag_offs		= 0x3010,
	MFC_Class_CMD_offs		= 0x3014,
	MFC_CMDStatus_offs		= 0x3014,
	MFC_QStatus_offs		= 0x3104,
	Prxy_QueryType_offs		= 0x3204,
	Prxy_QueryMask_offs		= 0x321C,
	Prxy_TagStatus_offs		= 0x322C,
	SPU_Out_MBox_offs		= 0x4004,
	SPU_In_MBox_offs		= 0x400C,
	SPU_MBox_Status_offs	= 0x4014,
	SPU_RunCntl_offs		= 0x401C,
	SPU_Status_offs			= 0x4024,
	SPU_NPC_offs			= 0x4034,
	SPU_RdSigNotify1_offs	= 0x1400C,
	SPU_RdSigNotify2_offs	= 0x1C00C,
};

enum : u64
{
	RAW_SPU_OFFSET		= 0x0000000000100000,
	RAW_SPU_BASE_ADDR   = 0x00000000E0000000,
	RAW_SPU_LS_OFFSET	= 0x0000000000000000,
	RAW_SPU_PROB_OFFSET	= 0x0000000000040000,
};

__forceinline static u32 GetRawSPURegAddrByNum(int num, int offset)
{
	return RAW_SPU_OFFSET * num + RAW_SPU_BASE_ADDR + RAW_SPU_PROB_OFFSET + offset;
}

__forceinline static u32 GetRawSPURegAddrById(int id, int offset)
{
	return GetRawSPURegAddrByNum(Emu.GetCPU().GetThreadNumById(PPC_THREAD_RAW_SPU, id), offset);
}


class RawSPUThread : public SPUThread
{
public:
	RawSPUThread(PPCThreadType type = PPC_THREAD_RAW_SPU);
	~RawSPUThread();

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
	virtual void InitRegs();

private:
	virtual void Task();
};

SPUThread& GetCurrentSPUThread();