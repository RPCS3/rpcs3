#pragma once

#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/DisAsm.h"
#include "Emu/Cell/PPCThread.h"
#include "Gui/DisAsmFrame.h"
#include "Emu/Memory/Memory.h"

#define START_OPCODES_GROUP(x) /*x*/
#define END_OPCODES_GROUP(x) /*x*/

class PPU_DisAsm
	: public PPU_Opcodes
	, public DisAsm
{
public:
	PPCThread& CPU;

	PPU_DisAsm()
		: DisAsm(*(PPCThread*)NULL, DumpMode)
		, CPU(*(PPCThread*)NULL)
	{
	}

	PPU_DisAsm(PPCThread& cpu, DisAsmModes mode = NormalMode)
		: DisAsm(cpu, mode)
		, CPU(cpu)
	{
	}

	~PPU_DisAsm()
	{
	}

private:
	virtual void Exit()
	{
		if(m_mode == NormalMode && !disasm_frame->exit)
		{
			disasm_frame->Close();
		}
	}

	virtual u32 DisAsmBranchTarget(const s32 imm)
	{
		return branchTarget(m_mode == NormalMode ? CPU.PC : dump_pc, imm);
	}
	
private:
	virtual void NULL_OP()
	{
		Write( "null" );
	}

	virtual void NOP()
	{
		Write( "nop" );
	}

	virtual void TDI(OP_REG to, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_INT1_R1_IMM("tdi", to, ra, simm16);
	}
	virtual void TWI(OP_REG to, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_INT1_R1_IMM("twi", to, ra, simm16);
	}

	START_OPCODES_GROUP(G_04)
		virtual void VXOR(OP_REG vrd, OP_REG vra, OP_REG vrb)
		{
			DisAsm_V3("vxor", vrd, vra, vrb);
		}
	END_OPCODES_GROUP(G_04);
	
	virtual void MULLI(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_R2_IMM("mulli", rd, ra, simm16);
	}
	virtual void SUBFIC(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_R2_IMM("subfic", rd, ra, simm16);
	}
	virtual void CMPLI(OP_REG crfd, OP_REG l, OP_REG ra, OP_uIMM uimm16)
	{
		DisAsm_CR1_R1_IMM(wxString::Format("cmpl%si", l ? "d" : "w"), crfd, ra, uimm16);
	}
	virtual void CMPI(OP_REG crfd, OP_REG l, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_CR1_R1_IMM(wxString::Format("cmp%si", l ? "d" : "w"), crfd, ra, simm16);
	}
	virtual void ADDIC(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_R2_IMM("addic", rd, ra, simm16);
	}
	virtual void ADDIC_(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		DisAsm_R2_IMM("addic.", rd, ra, simm16);
	}
	virtual void ADDI(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		if(ra == 0)
		{
			DisAsm_R1_IMM("li", rd, simm16);
		}
		else
		{
			DisAsm_R2_IMM("addi", rd, ra, simm16);
		}
	}
	virtual void ADDIS(OP_REG rd, OP_REG ra, OP_sIMM simm16)
	{
		if(ra == 0)
		{
			DisAsm_R1_IMM("lis", rd, simm16);
		}
		else
		{
			DisAsm_R2_IMM("addis", rd, ra, simm16);
		}
	}

	virtual void BC(OP_REG bo, OP_REG bi, OP_sIMM bd, OP_REG aa, OP_REG lk)
	{
		if(m_mode == CompilerElfMode)
		{
			Write(wxString::Format("bc 0x%x, 0x%x, 0x%x, %d, %d", bo, bi, bd, aa, lk));
			return;
		}

		//TODO: aa lk
		const u8 bo0 = (bo & 0x10) ? 1 : 0;
		const u8 bo1 = (bo & 0x08) ? 1 : 0;
		const u8 bo2 = (bo & 0x04) ? 1 : 0;
		const u8 bo3 = (bo & 0x02) ? 1 : 0;
		const u8 bo4 = (bo & 0x01) ? 1 : 0;

		if(bo0 && !bo1 && !bo2 && bo3 && !bo4)
		{
			DisAsm_CR_BRANCH("bdz", bi/4, bd); return;
		}
		else if(bo0 && bo1 && !bo2 && bo3 && !bo4)
		{
			DisAsm_CR_BRANCH("bdz-", bi/4, bd); return;
		}
		else if(bo0 && bo1 && !bo2 && bo3 && bo4)
		{
			DisAsm_CR_BRANCH("bdz+", bi/4, bd); return;
		}
		else if(bo0 && !bo1 && !bo2 && !bo3 && !bo4)
		{
			DisAsm_CR_BRANCH("bdnz", bi/4, bd); return;
		}
		else if(bo0 && bo1 && !bo2 && !bo3 && !bo4)
		{
			DisAsm_CR_BRANCH("bdnz-", bi/4, bd); return;
		}
		else if(bo0 && bo1 && !bo2 && !bo3 && bo4)
		{
			DisAsm_CR_BRANCH("bdnz+", bi/4, bd); return;
		}
		else if(!bo0 && !bo1 && bo2 && !bo3 && !bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("bge", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("ble", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("bne", bi/4, bd); return;
			}
		}
		else if(!bo0 && !bo1 && bo2 && bo3 && !bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("bge-", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("ble-", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("bne-", bi/4, bd); return;
			}
		}
		else if(!bo0 && !bo1 && bo2 && bo3 && bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("bge+", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("ble+", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("bne+", bi/4, bd); return;
			}
		}
		else if(!bo0 && bo1 && bo2 && !bo3 && !bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("blt", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("bgt", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("beq", bi/4, bd); return;
			}
		}
		else if(!bo0 && bo1 && bo2 && bo3 && !bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("blt-", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("bgt-", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("beq-", bi/4, bd); return;
			}
		}
		else if(!bo0 && bo1 && bo2 && bo3 && bo4)
		{
			switch(bi % 4)
			{
			case 0x0: DisAsm_CR_BRANCH("blt+", bi/4, bd); return;
			case 0x1: DisAsm_CR_BRANCH("bgt+", bi/4, bd); return;
			case 0x2: DisAsm_CR_BRANCH("beq+", bi/4, bd); return;
			}
		}
		
		Write(wxString::Format("bc [%x:%x:%x:%x:%x], cr%d[%x], 0x%x, %d, %d", bo0, bo1, bo2, bo3, bo4, bi/4, bi%4, bd, aa, lk));
	}
	virtual void SC(const s32 sc_code)
	{
		switch(sc_code)
		{
		case 0x1: Write("HyperCall"); break;
		case 0x2: Write("sc"); break;
		case 0x22: Write("HyperCall LV1"); break;
		default: Write(wxString::Format("Unknown sc: %x", sc_code));
		}
	}
	virtual void B(OP_sIMM ll, OP_REG aa, OP_REG lk)
	{
		if(m_mode == CompilerElfMode)
		{
			Write(wxString::Format("b 0x%x, %d, %d", ll, aa, lk));
			return;
		}

		switch(lk)
		{
			case 0:
				switch(aa)
				{
					case 0:	DisAsm_BRANCH("b", ll);		break;
					case 1:	DisAsm_BRANCH_A("ba", ll);	break;
				}
			break;
			
			case 1:
				switch(aa)
				{
					case 0: DisAsm_BRANCH("bl", ll);	break;
					case 1: DisAsm_BRANCH_A("bla", ll);	break;
				}
			break;
		}
	}
	
	START_OPCODES_GROUP(G_13)
		virtual void MCRF(OP_REG crfd, OP_REG crfs)
		{
			DisAsm_CR2("mcrf", crfd, crfs);
		}
		virtual void BCLR(OP_REG bo, OP_REG bi, OP_REG bh, OP_REG lk)
		{
			const u8 bo0 = (bo & 0x10) ? 1 : 0;
			const u8 bo1 = (bo & 0x08) ? 1 : 0;
			const u8 bo2 = (bo & 0x04) ? 1 : 0;
			const u8 bo3 = (bo & 0x02) ? 1 : 0;

			if(bo0 && !bo1 && bo2 && !bo3) {Write("blr"); return;}
			Write(wxString::Format("bclr [%x:%x:%x:%x], cr%d[%x], %d, %d", bo0, bo1, bo2, bo3, bi/4, bi%4, bh, lk));
		}
		virtual void CRNOR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crnor", bt, ba, bb);
		}
		virtual void CRANDC(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crandc", bt, ba, bb);
		}
		virtual void ISYNC()
		{
			Write("isync");
		}
		virtual void CRXOR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crxor", bt, ba, bb);
		}
		virtual void CRNAND(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crnand", bt, ba, bb);
		}
		virtual void CRAND(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crand", bt, ba, bb);
		}
		virtual void CREQV(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("creqv", bt, ba, bb);
		}
		virtual void CRORC(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("crorc", bt, ba, bb);
		}
		virtual void CROR(OP_REG bt, OP_REG ba, OP_REG bb)
		{
			DisAsm_INT3("cror", bt, ba, bb);
		}
		virtual void BCCTR(OP_REG bo, OP_REG bi, OP_REG bh, OP_REG lk)
		{
			switch(lk)
			{
				case 0: DisAsm_INT3("bcctr", bo, bi, bh); break;
				case 1: DisAsm_INT3("bcctrl", bo, bi, bh); break;
			}
		}
		virtual void BCTR()
		{
			Write("bctr");
		}
		virtual void BCTRL()
		{
			Write("bctrl");
		}
	END_OPCODES_GROUP(G_13);
	
	
	virtual void RLWIMI(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, OP_REG me, bool rc)
	{
		DisAsm_R2_INT3_RC("rlwimi", ra, rs, sh, mb, me, rc);
	}
	virtual void RLWINM(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, OP_REG me, bool rc)
	{
		DisAsm_R2_INT3_RC("rlwinm", ra, rs, sh, mb, me, rc);
	}
	virtual void RLWNM(OP_REG ra, OP_REG rs, OP_REG rb, OP_REG MB, OP_REG ME, bool rc)
	{
		DisAsm_R3_INT2_RC("rlwnm", ra, rs, rb, MB, ME, rc);
	}
	virtual void ORI(OP_REG rs, OP_REG ra, OP_uIMM uimm16)
	{
		if(rs == 0 && ra == 0 && uimm16 == 0)
		{
			NOP();
			return;
		}
		DisAsm_R2_IMM("ori", rs, ra, uimm16);
	}
	virtual void ORIS(OP_REG rs, OP_REG ra, OP_uIMM uimm16)
	{
		if(rs == 0 && ra == 0 && uimm16 == 0)
		{
			NOP();
			return;
		}
		DisAsm_R2_IMM("oris", rs, ra, uimm16);
	}
	virtual void XORI(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		DisAsm_R2_IMM("xori", ra, rs, uimm16);
	}
	virtual void XORIS(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		DisAsm_R2_IMM("xoris", ra, rs, uimm16);
	}
	virtual void ANDI_(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		DisAsm_R2_IMM("andi.", ra, rs, uimm16);
	}
	virtual void ANDIS_(OP_REG ra, OP_REG rs, OP_uIMM uimm16)
	{
		DisAsm_R2_IMM("andis.", ra, rs, uimm16);
	}

	START_OPCODES_GROUP(G_1e)
		virtual void RLDICL(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			if(sh == 0)
			{
				DisAsm_R2_INT1_RC("clrldi", ra, rs, mb, rc);
			}
			else if(mb == 0)
			{
				DisAsm_R2_INT1_RC("rotldi", ra, rs, sh, rc);
			}
			else if(mb == 64 - sh)
			{
				DisAsm_R2_INT1_RC("srdi", ra, rs, mb, rc);
			}
			else
			{
				DisAsm_R2_INT2_RC("rldicl", ra, rs, sh, mb, rc);
			}
		}
		virtual void RLDICR(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG me, bool rc)
		{
			DisAsm_R2_INT2_RC("rldicr", ra, rs, sh, me, rc);
		}
		virtual void RLDIC(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			DisAsm_R2_INT2_RC("rldic", ra, rs, sh, mb, rc);
		}
		virtual void RLDIMI(OP_REG ra, OP_REG rs, OP_REG sh, OP_REG mb, bool rc)
		{
			DisAsm_R2_INT2_RC("rldimi", ra, rs, sh, mb, rc);
		}
	END_OPCODES_GROUP(G_1e);
	
	START_OPCODES_GROUP(G_1f)
		virtual void CMP(OP_REG crfd, OP_REG l, OP_REG ra, OP_REG rb)
		{
			DisAsm_CR1_R2(wxString::Format("cmp%s", l ? "d" : "w"), crfd, ra, rb);
		}
		virtual void TW(OP_REG to, OP_REG ra, OP_REG rb)
		{
			DisAsm_INT1_R2("tw", to, ra, rb);
		}
		virtual void LVEBX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvebx", vd, ra, rb);
		}
		virtual void SUBFC(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("subfc", rd, ra, rb, oe, rc);
		}
		virtual void ADDC(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("addc", rd, ra, rb, oe, rc);
		}
		virtual void MULHDU(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("mulhdu", rd, ra, rb, rc);
		}
		virtual void MULHWU(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("mulhwu", rd, ra, rb, rc);
		}
		virtual void MFOCRF(OP_REG a, OP_REG fxm, OP_REG rd)
		{
			if(a)
			{
				DisAsm_R1_IMM("mfocrf", rd, fxm);
			}
			else
			{
				DisAsm_R1("mfcr", rd);
			}
		}
		virtual void LWARX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lwarx", rd, ra, rb);
		}
		virtual void LDX(OP_REG ra, OP_REG rs, OP_REG rb)
		{
			DisAsm_R3("ldx", ra, rs, rb);
		}
		virtual void LWZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lwzx", rd, ra, rb);
		}
		virtual void SLW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("slw", ra, rs, rb, rc);
		}
		virtual void CNTLZW(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("cntlzw", ra, rs, rc);
		}
		virtual void SLD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("sld", ra, rs, rb, rc);
		}
		virtual void AND(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("and", ra, rs, rb, rc);
		}
		virtual void CMPL(OP_REG crfd, OP_REG l, OP_REG ra, OP_REG rb)
		{
			DisAsm_CR1_R2(wxString::Format("cmpl%s", l ? "d" : "w"), crfd, ra, rb);
		}
		virtual void LVEHX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvehx", vd, ra, rb);
		}
		virtual void SUBF(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			 DisAsm_R3_OE_RC("subf", rd, ra, rb, oe, rc);
		}
		virtual void LDUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("ldux", rd, ra, rb);
		}
		virtual void DCBST(OP_REG ra, OP_REG rb)
		{
			DisAsm_R2("dcbst", ra, rb);
		}
		virtual void CNTLZD(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("cntlzd", ra, rs, rc);
		}
		virtual void ANDC(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("andc", ra, rs, rb, rc);
		}
		virtual void LVEWX(OP_REG vd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvewx", vd, ra, rb);
		}
		virtual void MULHD(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("mulhd", rd, ra, rb, rc);
		}
		virtual void MULHW(OP_REG rd, OP_REG ra, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("mulhw", rd, ra, rb, rc);
		}
		virtual void LDARX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("ldarx", rd, ra, rb);
		}
		virtual void DCBF(OP_REG ra, OP_REG rb)
		{
			DisAsm_R2("dcbf", ra, rb);
		}
		virtual void LBZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lbzx", rd, ra, rb);
		}
		virtual void LVX(OP_REG vrd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("lvx", vrd, ra, rb);
		}
		virtual void NEG(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			DisAsm_R2_OE_RC("neg", rd, ra, oe, rc);
		}
		virtual void LBZUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lbzux", rd, ra, rb);
		}
		virtual void NOR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			if(rs == rb)
			{
				DisAsm_R2_RC("not", ra, rs, rc);
			}
			else
			{
				DisAsm_R3_RC("nor", ra, rs, rb, rc);
			}
		}
		virtual void SUBFE(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("subfe", rd, ra, rb, oe, rc);
		}
		virtual void ADDE(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("adde", rd, ra, rb, oe, rc);
		}
		virtual void MTOCRF(OP_REG fxm, OP_REG rs)
		{
			DisAsm_INT1_R1("mtocrf", fxm, rs);
		}
		virtual void STDX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stdx.", rs, ra, rb);
		}
		virtual void STWCX_(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stwcx.", rs, ra, rb);
		}
		virtual void STWX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stwx", rs, ra, rb);
		}
		virtual void STDUX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stdux", rs, ra, rb);
		}
		virtual void ADDZE(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			DisAsm_R2_OE_RC("addze", rd, ra, oe, rc);
		}
		virtual void STDCX_(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stdcx.", rs, ra, rb);
		}
		virtual void STBX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("stbx", rs, ra, rb);
		}
		virtual void STVX(OP_REG vrd, OP_REG ra, OP_REG rb)
		{
			DisAsm_V1_R2("stvx", vrd, ra, rb);
		}
		virtual void MULLD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("mulld", rd, ra, rb, oe, rc);
		}
		virtual void ADDME(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			DisAsm_R2_OE_RC("addme", rd, ra, oe, rc);
		}
		virtual void MULLW(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("mullw", rd, ra, rb, oe, rc);
		}
		virtual void DCBTST(OP_REG th, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("dcbtst", th, ra, rb);
		}
		virtual void ADD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("add", rd, ra, rb, oe, rc);
		}
		virtual void DCBT(OP_REG ra, OP_REG rb, OP_REG th)
		{
			DisAsm_R2("dcbt", ra, rb);
		}
		virtual void LHZX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhzx", rd, ra, rb);
		}
		virtual void EQV(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("eqv", ra, rs, rb, rc);
		}
		virtual void ECIWX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("eciwx", rd, ra, rb);
		}
		virtual void LHZUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhzux", rd, ra, rb);
		}
		virtual void XOR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("xor", ra, rs, rb, rc);
		}
		virtual void MFSPR(OP_REG rd, OP_REG spr)
		{
			const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);
			switch(n)
			{
			case 0x001: DisAsm_R1("mfxer", rd); break;
			case 0x008: DisAsm_R1("mflr", rd); break;
			case 0x009: DisAsm_R1("mfctr", rd); break;
			default: DisAsm_R1_IMM("mfspr", rd, spr); break;
			}
		}
		virtual void LHAX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhax", rd, ra, rb);
		}
		virtual void ABS(OP_REG rd, OP_REG ra, OP_REG oe, bool rc)
		{
			DisAsm_R2_OE_RC("abs", rd, ra, oe, rc);
		}
		virtual void MFTB(OP_REG rd, OP_REG spr)
		{
			const u32 n = (spr >> 5) | ((spr & 0x1f) << 5);
			switch(n)
			{
			case 268: DisAsm_R1("mftb", rd); break;
			case 269: DisAsm_R1("mftbu", rd); break;
			default: DisAsm_R1_IMM("mftb", rd, spr); break;
			}
		}
		virtual void LHAUX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhaux", rd, ra, rb);
		}
		virtual void STHX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("sthx", rs, ra, rb);
		}
		virtual void ORC(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("orc", ra, rs, rb, rc);
		}
		virtual void ECOWX(OP_REG rs, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("ecowx", rs, ra, rb);
		}
		virtual void OR(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			if(rs==rb)
			{
				DisAsm_R2_RC("mr", ra, rb, rc);
			}
			else
			{
				DisAsm_R3_RC("or", ra, rs, rb, rc);
			}
		}
		virtual void DIVDU(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("divdu", rd, ra, rb, oe, rc);
		}
		virtual void DIVWU(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("divwu", rd, ra, rb, oe, rc);
		}
		virtual void MTSPR(OP_REG spr, OP_REG rs)
		{
			const u32 n = (spr & 0x1f) + ((spr >> 5) & 0x1f);

			switch(n)
			{
			case 0x001: DisAsm_R1("mtxer", rs); break;
			case 0x008: DisAsm_R1("mtlr", rs); break;
			case 0x009: DisAsm_R1("mtctr", rs); break;
			default: DisAsm_IMM_R1("mtspr", spr, rs); break;
			}
		}
		/*0x1d6*///DCBI
		virtual void DIVD(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("divd", rd, ra, rb, oe, rc);
		}
		virtual void DIVW(OP_REG rd, OP_REG ra, OP_REG rb, OP_REG oe, bool rc)
		{
			DisAsm_R3_OE_RC("divw", rd, ra, rb, oe, rc);
		}
		virtual void LWBRX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lwbrx", rd, ra, rb);
		}
		virtual void LFSX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("lfsx", frd, ra, rb);
		}
		virtual void SRW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("srw", ra, rs, rb, rc);
		}
		virtual void SRD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("srd", ra, rs, rb, rc);
		}
		virtual void LFSUX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("lfsux", frd, ra, rb);
		}
		virtual void SYNC(OP_REG l)
		{
			DisAsm_INT1("sync", l);
		}
		virtual void LFDX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("lfdx", frd, ra, rb);
		}
		virtual void LFDUX(OP_REG frd, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("lfdux", frd, ra, rb);
		}
		virtual void STFSX(OP_REG frs, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("stfsx", frs, ra, rb);
		}
		virtual void LHBRX(OP_REG rd, OP_REG ra, OP_REG rb)
		{
			DisAsm_R3("lhbrx", rd, ra, rb);
		}
		virtual void SRAW(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("sraw", ra, rs, rb, rc);
		}
		virtual void SRAD(OP_REG ra, OP_REG rs, OP_REG rb, bool rc)
		{
			DisAsm_R3_RC("srad", ra, rs, rb, rc);
		}
		virtual void SRAWI(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			DisAsm_R2_INT1_RC("srawi", ra, rs, sh, rc);
		}
		virtual void SRADI1(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			DisAsm_R2_INT1_RC("sradi", ra, rs, sh, rc);
		}
		virtual void SRADI2(OP_REG ra, OP_REG rs, OP_REG sh, bool rc)
		{
			DisAsm_R2_INT1_RC("sradi", ra, rs, sh, rc);
		}
		virtual void EIEIO()
		{
			Write("eieio");
		}
		virtual void EXTSH(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("extsh", ra, rs, rc);
		}
		virtual void EXTSB(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("extsb", ra, rs, rc);
		}
		virtual void STFIWX(OP_REG frs, OP_REG ra, OP_REG rb)
		{
			DisAsm_F1_R2("stfiwx", frs, ra, rb);
		}
		virtual void EXTSW(OP_REG ra, OP_REG rs, bool rc)
		{
			DisAsm_R2_RC("extsw", ra, rs, rc);
		}
		/*0x3d6*///ICBI
		virtual void DCBZ(OP_REG ra, OP_REG rs)
		{
			DisAsm_R2("dcbz", ra, rs);
		}
	END_OPCODES_GROUP(G_1f);
	
	virtual void LWZ(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lwz", rd, ra, d);
	}
	virtual void LWZU(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lwzu", rd, ra, d);
	}
	virtual void LBZ(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lbz", rd, ra, d);
	}
	virtual void LBZU(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lbzu", rd, ra, d);
	}
	virtual void STW(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stw", rs, ra, d);
	}
	virtual void STWU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stwu", rs, ra, d);
	}
	virtual void STB(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stb", rs, ra, d);
	}
	virtual void STBU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stbu", rs, ra, d);
	}
	virtual void LHZ(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lhz", rs, ra, d);
	}
	virtual void LHZU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lhzu", rs, ra, d);
	}
	virtual void STH(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("sth", rs, ra, d);
	}
	virtual void STHU(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("sthu", rs, ra, d);
	}
	virtual void LMW(OP_REG rd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("lmw", rd, ra, d);
	}
	virtual void STMW(OP_REG rs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_R2_IMM("stmw", rs, ra, d);
	}
	virtual void LFS(OP_REG frd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("lfs", frd, d, ra);
	}
	virtual void LFSU(OP_REG frd, OP_REG ra, OP_sIMM ds)
	{
		DisAsm_F1_IMM_R1("lfsu", frd, ds, ra);
	}
	virtual void LFD(OP_REG frd, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("lfd", frd, d, ra);
	}
	virtual void LFDU(OP_REG frd, OP_REG ra, OP_sIMM ds)
	{
		DisAsm_F1_IMM_R1("lfdu", frd, ds, ra);
	}
	virtual void STFS(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("stfs", frs, d, ra);
	}
	virtual void STFSU(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("stfsu", frs, d, ra);
	}
	virtual void STFD(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("stfd", frs, d, ra);
	}
	virtual void STFDU(OP_REG frs, OP_REG ra, OP_sIMM d)
	{
		DisAsm_F1_IMM_R1("stfdu", frs, d, ra);
	}
	
	START_OPCODES_GROUP(G_3a)
		virtual void LD(OP_REG rd, OP_REG ra, OP_sIMM ds)
		{
			DisAsm_R2_IMM("ld", rd, ra, ds);
		}
		virtual void LDU(OP_REG rd, OP_REG ra, OP_sIMM ds)
		{
			DisAsm_R2_IMM("ldu", rd, ra, ds);
		}
	END_OPCODES_GROUP(G_3a);

	START_OPCODES_GROUP(G_3b)
		virtual void FDIVS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fdivs", frd, fra, frb, rc);
		}
		virtual void FSUBS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fsubs", frd, fra, frb, rc);
		}
		virtual void FADDS(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fadds", frd, fra, frb, rc);
		}
		virtual void FSQRTS(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fsqrts", frd, frb, rc);
		}
		virtual void FRES(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fres", frd, frb, rc);
		}
		virtual void FMULS(OP_REG frd, OP_REG fra, OP_REG frc, bool rc)
		{
			DisAsm_F3_RC("fmuls", frd, fra, frc, rc);
		}
		virtual void FMADDS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fmadds", frd, fra, frc, frb, rc);
		}
		virtual void FMSUBS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fmsubs", frd, fra, frc, frb, rc);
		}
		virtual void FNMSUBS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fnmsubs", frd, fra, frc, frb, rc);
		}
		virtual void FNMADDS(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fnmadds", frd, fra, frc, frb, rc);
		}
	END_OPCODES_GROUP(G_3b);
	
	START_OPCODES_GROUP(G_3e)
		virtual void STD(OP_REG rs, OP_REG ra, OP_sIMM ds)
		{
			DisAsm_R2_IMM("std", rs, ra, ds);
		}
		virtual void STDU(OP_REG rs, OP_REG ra, OP_sIMM ds)
		{
			DisAsm_R2_IMM("stdu", rs, ra, ds);
		}
	END_OPCODES_GROUP(G_3e);

	START_OPCODES_GROUP(G_3f)
		virtual void MTFSB1(OP_REG bt, bool rc)
		{
			DisAsm_F1_RC("mtfsb1", bt, rc);
		}
		virtual void MCRFS(OP_REG bf, OP_REG bfa)
		{
			DisAsm_F2("mcrfs", bf, bfa);
		}
		virtual void MTFSB0(OP_REG bt, bool rc)
		{
			DisAsm_F1_RC("mtfsb0", bt, rc);
		}
		virtual void MTFSFI(OP_REG crfd, OP_REG i, bool rc)
		{
			DisAsm_F2_RC("mtfsfi", crfd, i, rc);
		}
		virtual void MFFS(OP_REG frd, bool rc)
		{
			DisAsm_F1_RC("mffs", frd, rc);
		}
		virtual void MTFSF(OP_REG flm, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("mtfsf", flm, frb, rc);
		}
		virtual void FCMPU(OP_REG crfd, OP_REG fra, OP_REG frb)
		{
			DisAsm_CR1_F2("fcmpu", crfd, fra, frb);
		}
		virtual void FRSP(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("frsp", frd, frb, rc);
		}
		virtual void FCTIW(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fctiw", frd, frb, rc);
		}
		virtual void FCTIWZ(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fctiwz", frd, frb, rc);
		}
		virtual void FDIV(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fdiv", frd, fra, frb, rc);
		}
		virtual void FSUB(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fsub", frd, fra, frb, rc);
		}
		virtual void FADD(OP_REG frd, OP_REG fra, OP_REG frb, bool rc)
		{
			DisAsm_F3_RC("fadd", frd, fra, frb, rc);
		}
		virtual void FSQRT(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fsqrt", frd, frb, rc);
		}
		virtual void FSEL(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fsel", frd, fra, frc, frb, rc);
		}
		virtual void FMUL(OP_REG frd, OP_REG fra, OP_REG frc, bool rc)
		{
			DisAsm_F3_RC("fmul", frd, fra, frc, rc);
		}
		virtual void FRSQRTE(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("frsqrte", frd, frb, rc);
		}
		virtual void FMSUB(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fmsub", frd, fra, frc, frb, rc);
		}
		virtual void FMADD(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fmadd", frd, fra, frc, frb, rc);
		}
		virtual void FNMSUB(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fnmsub", frd, fra, frc, frb, rc);
		}
		virtual void FNMADD(OP_REG frd, OP_REG fra, OP_REG frc, OP_REG frb, bool rc)
		{
			DisAsm_F4_RC("fnmadd", frd, fra, frc, frb, rc);
		}
		virtual void FCMPO(OP_REG crfd, OP_REG fra, OP_REG frb)
		{
			DisAsm_F3("fcmpo", crfd, fra, frb);
		}
		virtual void FNEG(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fneg", frd, frb, rc);
		}
		virtual void FMR(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fmr", frd, frb, rc);
		}
		virtual void FNABS(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fnabs", frd, frb, rc);
		}
		virtual void FABS(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fabs", frd, frb, rc);
		}
		virtual void FCTID(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fctid", frd, frb, rc);
		}
		virtual void FCTIDZ(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fctidz", frd, frb, rc);
		}
		virtual void FCFID(OP_REG frd, OP_REG frb, bool rc)
		{
			DisAsm_F2_RC("fcfid", frd, frb, rc);
		}
	END_OPCODES_GROUP(G_3f);

	virtual void UNK(const s32 code, const s32 opcode, const s32 gcode)
	{
		Write(wxString::Format("Unknown/Illegal opcode! (0x%08x : 0x%x : 0x%x)", code, opcode, gcode));
	}
};

#undef START_OPCODES_GROUP
#undef END_OPCODES_GROUP