/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#define _disVUOpcodes(VU) \
 \
/*****************/ \
/* LOWER OPCODES */ \
/*****************/ \
 \
MakeDisF(dis##VU##MI_DIV,		dName("DIV");     dCP232f(_Fs_, _Fsf_); dCP232f(_Ft_, _Ftf_);) \
MakeDisF(dis##VU##MI_SQRT,		dName("SQRT");    dCP232f(_Ft_, _Ftf_);) \
MakeDisF(dis##VU##MI_RSQRT, 	dName("RSQRT");   dCP232f(_Fs_, _Fsf_); dCP232f(_Ft_, _Ftf_);) \
MakeDisF(dis##VU##MI_IADDI, 	dName("IADDI");   dCP232i(_Ft_); dCP232i(_Fs_); dImm5();) \
MakeDisF(dis##VU##MI_IADDIU, 	dName("IADDIU");  dCP232i(_Ft_); dCP232i(_Fs_); dImm15();) \
MakeDisF(dis##VU##MI_IADD, 		dName("IADD");    dCP232i(_Fd_); dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_IAND, 		dName("IAND");    dCP232i(_Fd_); dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_IOR, 		dName("IOR");     dCP232i(_Fd_); dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_ISUB, 		dName("ISUB");    dCP232i(_Fd_); dCP232i(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_ISUBIU, 	dName("ISUBIU");  dCP232i(_Ft_); dCP232i(_Fs_); dImm15();) \
MakeDisF(dis##VU##MI_MOVE, 		if (_Fs_ == 0 && _Ft_ == 0) { dNameU("NOP"); } else { dNameU("MOVE"); dCP2128f(_Ft_); dCP2128f(_Fs_); }) \
MakeDisF(dis##VU##MI_MFIR, 		dNameU("MFIR");   dCP2128f(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_MTIR, 		dNameU("MTIR");   dCP232i(_Ft_); dCP232f(_Fs_, _Fsf_);) \
MakeDisF(dis##VU##MI_MR32, 		dNameU("MR32");    dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_LQ, 		dNameU("LQ");      dCP2128f(_Ft_); dCP232i(_Fs_); dImm11();) \
MakeDisF(dis##VU##MI_LQD, 		dNameU("LQD");     dCP2128f(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_LQI, 		dNameU("LQI");     dCP2128f(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_SQ, 		dNameU("SQ");      dCP2128f(_Fs_); dCP232i(_Ft_); dImm11(); ) \
MakeDisF(dis##VU##MI_SQD, 		dNameU("SQD");     dCP2128f(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_SQI, 		dNameU("SQI");     dCP2128f(_Fs_); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_ILW, 		dNameU("ILW");     dCP232i(_Ft_); dImm11(); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_ISW, 		dNameU("ISW");     dCP232i(_Ft_); dImm11(); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_ILWR, 		dNameU("ILWR");    dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_ISWR, 		dNameU("ISWR");    dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_LOI, 		dName("LOI"); ) \
MakeDisF(dis##VU##MI_RINIT, 	dNameU("RINIT");   dCP232i(REG_R); dCP232f(_Fs_, _Fsf_);) \
MakeDisF(dis##VU##MI_RGET, 		dNameU("RGET");    dCP232i(REG_R); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_RNEXT, 	dNameU("RNEXT");   dCP232i(REG_R); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_RXOR, 		dNameU("RXOR");    dCP232i(REG_R); dCP232f(_Fs_, _Fsf_);) \
MakeDisF(dis##VU##MI_WAITQ, 	dName("WAITQ");  ) \
MakeDisF(dis##VU##MI_FSAND, 	dName("FSAND");   dCP232i(_Ft_); dCP232i(REG_STATUS_FLAG); sprintf(ostr, "%s %.3x,", ostr, code&0xfff); ) \
MakeDisF(dis##VU##MI_FSEQ, 		dName("FSEQ");    dCP232i(_Ft_); dCP232i(REG_STATUS_FLAG); sprintf(ostr, "%s %.3x,", ostr, code&0xfff);) \
MakeDisF(dis##VU##MI_FSOR, 		dName("FSOR");    dCP232i(_Ft_); dCP232i(REG_STATUS_FLAG); sprintf(ostr, "%s %.3x,", ostr, code&0xfff);) \
MakeDisF(dis##VU##MI_FSSET, 	dName("FSSET");   dCP232i(REG_STATUS_FLAG);) \
MakeDisF(dis##VU##MI_FMAND, 	dName("FMAND");   dCP232i(_Ft_); dCP232i(REG_MAC_FLAG); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_FMEQ, 		dName("FMEQ");    dCP232i(_Ft_); dCP232i(REG_MAC_FLAG); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_FMOR, 		dName("FMOR");    dCP232i(_Ft_); dCP232i(REG_MAC_FLAG); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_FCAND, 	dName("FCAND");   dCP232i(1);    sprintf(ostr, "%s %8.8x,", ostr, code&0xffffff); ) \
MakeDisF(dis##VU##MI_FCEQ, 		dName("FCEQ");    dCP232i(1);    dCP232i(REG_CLIP_FLAG);) \
MakeDisF(dis##VU##MI_FCOR, 		dName("FCOR");    dCP232i(1);    dCP232i(REG_CLIP_FLAG);) \
MakeDisF(dis##VU##MI_FCSET, 	dName("FCSET");   dCP232i(REG_CLIP_FLAG); sprintf(ostr, "%s %.6x,", ostr, code&0xffffff); ) \
MakeDisF(dis##VU##MI_FCGET, 	dName("FCGET");   dCP232i(_Ft_); dCP232i(REG_CLIP_FLAG);) \
MakeDisF(dis##VU##MI_IBEQ, 		dName("IBEQ");    dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBGEZ, 	dName("IBEZ");    dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBGTZ, 	dName("IBGTZ");   dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBLEZ, 	dName("IBLEZ");   dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBLTZ, 	dName("IBLTZ");   dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_IBNE, 		dName("IBNE");    dImm11(); dCP232i(_Ft_); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_B, 		dName("B");       dImm11();) \
MakeDisF(dis##VU##MI_BAL, 		dName("BAL");     dImm11(); dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_JR, 		dName("JR");      dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_JALR, 		dName("JALR");    dCP232i(_Ft_); dCP232i(_Fs_); ) \
MakeDisF(dis##VU##MI_MFP, 		dNameU("MFP");     dCP2128f(_Ft_); dCP232i(REG_P);) \
MakeDisF(dis##VU##MI_WAITP, 	dName("WAITP");  ) \
MakeDisF(dis##VU##MI_ESADD, 	dName("ESADD");  dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ERSADD, 	dName("ERSADD"); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ELENG, 	dName("ELENG"); dCP2128f(_Fs_); ) \
MakeDisF(dis##VU##MI_ERLENG, 	dName("ERLENG"); dCP2128f(_Fs_); ) \
MakeDisF(dis##VU##MI_EATANxy,	dName("EATANxy"); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_EATANxz,	dName("EATANxz"); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ESUM, 		dName("ESUM");  dCP232i(_Fs_); ) \
MakeDisF(dis##VU##MI_ERCPR, 	dName("ERCPR"); dCP232f(_Fs_, _Fsf_); ) \
MakeDisF(dis##VU##MI_ESQRT, 	dName("ESQRT"); dCP232f(_Fs_, _Fsf_); ) \
MakeDisF(dis##VU##MI_ERSQRT, 	dName("ERSQRT"); dCP232f(_Fs_, _Fsf_); ) \
MakeDisF(dis##VU##MI_ESIN, 		dName("ESIN"); dCP232f(_Fs_, _Fsf_);  ) \
MakeDisF(dis##VU##MI_EATAN, 	dName("EATAN"); dCP232f(_Fs_, _Fsf_); ) \
MakeDisF(dis##VU##MI_EEXP, 		dName("EEXP");  dCP232f(_Fs_, _Fsf_); ) \
MakeDisF(dis##VU##MI_XITOP, 	dName("XITOP");  dCP232i(_Ft_);) \
MakeDisF(dis##VU##MI_XGKICK, 	dName("XGKICK"); dCP232i(_Fs_);) \
MakeDisF(dis##VU##MI_XTOP, 		dName("XTOP");   dCP232i(_Ft_);) \
 \
 \
/*****************/ \
/* UPPER OPCODES */ \
/*****************/ \
 \
MakeDisF(dis##VU##MI_ABS, 		dNameU("ABS");	 dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ADD, 		dNameU("ADD");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_ADDi,		dNameU("ADDi");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_ADDq, 		dNameU("ADDq");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_ADDx, 		dNameU("ADDx");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_ADDy, 		dNameU("ADDy");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_ADDz, 		dNameU("ADDz");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_ADDw, 		dNameU("ADDw");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_ADDA, 		dNameU("ADDA");	 dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_ADDAi,		dNameU("ADDAi");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_ADDAq, 	dNameU("ADDAq");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_ADDAx, 	dNameU("ADDAx");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_ADDAy, 	dNameU("ADDAy");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_ADDAz, 	dNameU("ADDAz");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_ADDAw, 	dNameU("ADDAw");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_SUB, 		dNameU("SUB");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_SUBi, 		dNameU("SUBi");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_SUBq, 		dNameU("SUBq");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_SUBx, 		dNameU("SUBx");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_SUBy, 		dNameU("SUBy");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_SUBz, 		dNameU("SUBz");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_SUBw, 		dNameU("SUBw");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_SUBA, 		dNameU("SUBA");	 dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_SUBAi,		dNameU("SUBAi");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_SUBAq, 	dNameU("SUBAq");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_SUBAx, 	dNameU("SUBAx");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_SUBAy, 	dNameU("SUBAy");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_SUBAz, 	dNameU("SUBAz");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_SUBAw, 	dNameU("SUBAw");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MUL, 		dNameU("MUL");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MULi, 		dNameU("MULi");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MULq, 		dNameU("MULq");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MULx, 		dNameU("MULx");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MULy, 		dNameU("MULy");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MULz, 		dNameU("MULz");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MULw, 		dNameU("MULw");	 dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MULA, 		dNameU("MULA"); 	 dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MULAi,		dNameU("MULAi");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MULAq, 	dNameU("MULAq");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MULAx, 	dNameU("MULAx");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MULAy, 	dNameU("MULAy");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MULAz, 	dNameU("MULAz");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MULAw, 	dNameU("MULAw");	 dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MADD, 		dNameU("MADD");   dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MADDi, 	dNameU("MADDi");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MADDq, 	dNameU("MADDq");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MADDx, 	dNameU("MADDx");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MADDy, 	dNameU("MADDy");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MADDz, 	dNameU("MADDz");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MADDw, 	dNameU("MADDw");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MADDA, 	dNameU("MADDA");  dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MADDAi, 	dNameU("MADDAi"); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MADDAq, 	dNameU("MADDAq"); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MADDAx, 	dNameU("MADDAx"); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MADDAy, 	dNameU("MADDAy"); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MADDAz, 	dNameU("MADDAz"); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MADDAw, 	dNameU("MADDAw"); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MSUB, 		dNameU("MSUB");   dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBi, 	dNameU("MSUBi");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MSUBq, 	dNameU("MSUBq");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MSUBx, 	dNameU("MSUBx");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBy, 	dNameU("MSUBy");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBz, 	dNameU("MSUBz");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBw, 	dNameU("MSUBw");  dCP2128f(_Fd_); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBA, 	dNameU("MSUBA");  dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBAi, 	dNameU("MSUBAi"); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MSUBAq, 	dNameU("MSUBAq"); dCP2ACCf(); dCP2128f(_Fs_); dCP232iF(REG_Q);) \
MakeDisF(dis##VU##MI_MSUBAx, 	dNameU("MSUBAx"); dCP2ACCf(); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBAy, 	dNameU("MSUBAy"); dCP2ACCf(); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBAz, 	dNameU("MSUBAz"); dCP2ACCf(); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MSUBAw, 	dNameU("MSUBAw"); dCP2ACCf(); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MAX, 		dNameU("MAX");    dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MAXi, 		dNameU("MAXi");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MAXx, 		dNameU("MAXx");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MAXy, 		dNameU("MAXy");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MAXz, 		dNameU("MAXz");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MAXw, 		dNameU("MAXw");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_MINI, 		dNameU("MINI");   dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_MINIi, 	dNameU("MINIi");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232iF(REG_I);) \
MakeDisF(dis##VU##MI_MINIx, 	dNameU("MINIx");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232x(_Ft_);) \
MakeDisF(dis##VU##MI_MINIy, 	dNameU("MINIy");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232y(_Ft_);) \
MakeDisF(dis##VU##MI_MINIz, 	dNameU("MINIz");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232z(_Ft_);) \
MakeDisF(dis##VU##MI_MINIw, 	dNameU("MINIw");  dCP2128f(_Fd_); dCP2128f(_Fs_); dCP232w(_Ft_);) \
MakeDisF(dis##VU##MI_OPMULA, 	dNameU("OPMULA"); dCP2ACCf(); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_OPMSUB, 	dNameU("OPMSUB"); dCP2128f(_Fd_); dCP2128f(_Fs_); dCP2128f(_Ft_);) \
MakeDisF(dis##VU##MI_NOP, 		dName("NOP");) \
MakeDisF(dis##VU##MI_FTOI0, 	dNameU("FTOI0");  dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_FTOI4, 	dNameU("FTOI4");  dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_FTOI12, 	dNameU("FTOI12"); dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_FTOI15, 	dNameU("FTOI15"); dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ITOF0, 	dNameU("ITOF0");  dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ITOF4, 	dNameU("ITOF4");  dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ITOF12, 	dNameU("ITOF12"); dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_ITOF15, 	dNameU("ITOF15"); dCP2128f(_Ft_); dCP2128f(_Fs_);) \
MakeDisF(dis##VU##MI_CLIP, 		dNameU("CLIP");   dCP2128f(_Fs_); dCP232w(_Ft_);) \

