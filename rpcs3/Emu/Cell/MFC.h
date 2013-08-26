#pragma once
#include <atomic>

enum
{
	MFC_PUT_CMD		= 0x20,
	MFC_GET_CMD		= 0x40,
	MFC_MASK_CMD	= 0xffff,
};

enum
{
	MFC_SPU_TO_PPU_MAILBOX_STATUS_MASK		= 0x000000FF,
	MFC_SPU_TO_PPU_MAILBOX_STATUS_SHIFT		= 0x0,
	MFC_PPU_TO_SPU_MAILBOX_STATUS_MASK		= 0x0000FF00,
	MFC_PPU_TO_SPU_MAILBOX_STATUS_SHIFT		= 0x8,
	MFC_PPU_TO_SPU_MAILBOX_MAX				= 0x4,
	MFC_SPU_TO_PPU_INT_MAILBOX_STATUS_MASK	= 0x00FF0000,
	MFC_SPU_TO_PPU_INT_MAILBOX_STATUS_SHIFT	= 0x10,
};

enum
{
	MFC_PPU_DMA_CMD_ENQUEUE_SUCCESSFUL		= 0x00,
	MFC_PPU_DMA_CMD_SEQUENCE_ERROR			= 0x01,
	MFC_PPU_DMA_QUEUE_FULL					= 0x02,
};

enum
{
	MFC_PPU_MAX_QUEUE_SPACE					= 0x08,
	MFC_SPU_MAX_QUEUE_SPACE					= 0x10,
};

struct DMAC_Queue
{
	bool is_valid;
	u64 ea;
	u32 lsa;
	u16 size;
	u32 op;
	u8 tag;
	u8 rt;
	u16 list_addr;
	u16 list_size;
	u32 dep_state;
	u32 cmd;
	u32 dep_type;
};

struct DMAC_Proxy
{
	u64 ea;
	u32 lsa;
	u16 size;
	u32 op;
	u8 tag;
	u8 rt;
	u16 list_addr;
	u16 list_size;
	u32 dep_state;
	u32 cmd;
	u32 dep_type;
};

template<size_t _max_count>
class SPUReg
{
	u64 m_addr;
	u32 m_pos;

public:
	static const size_t max_count = _max_count;
	static const size_t size = max_count * 4;

	SPUReg()
	{
		Init();
	}

	void Init()
	{
		m_pos = 0;
	}

	void SetAddr(u64 addr)
	{
		m_addr = addr;
	}

	u64 GetAddr() const
	{
		return m_addr;
	}

	__forceinline bool Pop(u32& res)
	{
		if(!m_pos) return false;
		res = Memory.Read32(m_addr + m_pos--);
		return true;
	}

	__forceinline bool Push(u32 value)
	{
		if(m_pos >= max_count) return false;
		Memory.Write32(m_addr + m_pos++, value);
		return true;
	}

	u32 GetCount() const
	{
		return m_pos;
	}

	u32 GetFreeCount() const
	{
		return max_count - m_pos;
	}

	void SetValue(u32 value)
	{
		Memory.Write32(m_addr, value);
	}

	u32 GetValue() const
	{
		return Memory.Read32(m_addr);
	}
};

struct DMAC
{
	DMAC_Queue queue[MFC_SPU_MAX_QUEUE_SPACE];
	DMAC_Proxy proxy[MFC_PPU_MAX_QUEUE_SPACE];
	u64 ls_offset;
	std::atomic<u32> queue_pos;
	std::atomic<u32> proxy_pos;

	u32 Cmd(u32 cmd, u32 tag, u32 lsa, u64 ea, u32 size)
	{
		if(!Memory.IsGoodAddr(ls_offset + lsa, size) || !Memory.IsGoodAddr(ea, size))
		{
			return MFC_PPU_DMA_CMD_SEQUENCE_ERROR;
		}

		if(proxy_pos >= MFC_PPU_MAX_QUEUE_SPACE)
		{
			return MFC_PPU_DMA_QUEUE_FULL;
		}

		DMAC_Proxy& p = proxy[proxy_pos++];
		p.cmd = cmd;
		p.tag = tag;
		p.lsa = lsa;
		p.ea = ea;
		p.size = size;

		return MFC_PPU_DMA_CMD_ENQUEUE_SUCCESSFUL;
	}

	void DoCmd()
	{
		if(proxy_pos)
		{
			DMAC_Proxy p = proxy[0];
			memcpy(proxy, proxy + 1, proxy_pos--);

			switch(p.cmd)
			{
			case MFC_PUT_CMD:
				memcpy(Memory + ls_offset + p.lsa, Memory + p.ea, p.size);
			break;

			case MFC_GET_CMD:
				memcpy(Memory + p.ea, Memory + ls_offset + p.lsa, p.size);
			break;

			default:
				ConLog.Error("Unknown DMA cmd.");
			break;
			}
		}
	}
};

struct MFC
{
	SPUReg<1> MFC_LSA;
	SPUReg<1> MFC_EAH;
	SPUReg<1> MFC_EAL;
	SPUReg<1> MFC_Size_Tag;
	SPUReg<1> MFC_CMDStatus;
	SPUReg<1> MFC_QStatus;
	SPUReg<1> Prxy_QueryType;
	SPUReg<1> Prxy_QueryMask;
	SPUReg<1> Prxy_TagStatus;
	SPUReg<1> SPU_Out_MBox;
	SPUReg<4> SPU_In_MBox;
	SPUReg<1> SPU_MBox_Status;
	SPUReg<1> SPU_RunCntl;
	SPUReg<1> SPU_Status;
	SPUReg<1> SPU_NPC;
	SPUReg<1> SPU_RdSigNotify1;
	SPUReg<1> SPU_RdSigNotify2;

	DMAC dmac;

	void Handle()
	{
		u32 cmd = MFC_CMDStatus.GetValue();

		if(cmd)
		{
			u16 op = cmd & MFC_MASK_CMD;

			switch(op)
			{
			case MFC_PUT_CMD:
			case MFC_GET_CMD:
			{
				u32 lsa = MFC_LSA.GetValue();
				u64 ea = (u64)MFC_EAL.GetValue() | ((u64)MFC_EAH.GetValue() << 32);
				u32 size_tag = MFC_Size_Tag.GetValue();
				u16 tag = (u16)size_tag;
				u16 size = size_tag >> 16;

				ConLog.Warning("RawSPU DMA %s:", op == MFC_PUT_CMD ? "PUT" : "GET");
				ConLog.Warning("*** lsa  = 0x%x", lsa);
				ConLog.Warning("*** ea   = 0x%llx", ea);
				ConLog.Warning("*** tag  = 0x%x", tag);
				ConLog.Warning("*** size = 0x%x", size);
				ConLog.SkipLn();

				MFC_CMDStatus.SetValue(dmac.Cmd(cmd, tag, lsa, ea, size));
			}
			break;

			default:
				ConLog.Error("Unknown MFC cmd. (opcode=0x%x, cmd=0x%x)", op, cmd);
			break;
			}
		}

		if(Prxy_QueryType.GetValue() == 2)
		{
			Prxy_QueryType.SetValue(0);
			u32 mask = Prxy_QueryMask.GetValue();
			//
			MFC_QStatus.SetValue(mask);
		}
	}
};