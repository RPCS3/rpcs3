const char *getVersionString() const { return "2.07"; }
void packssdw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x6B); }
void packsswb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x63); }
void packuswb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x67); }
void pand(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xDB); }
void pandn(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xDF); }
void pmaddwd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xF5); }
void pmulhuw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xE4); }
void pmulhw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xE5); }
void pmullw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xD5); }
void por(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xEB); }
void punpckhbw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x68); }
void punpckhwd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x69); }
void punpckhdq(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x6A); }
void punpcklbw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x60); }
void punpcklwd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x61); }
void punpckldq(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x62); }
void pxor(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xEF); }
void pavgb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xE0); }
void pavgw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xE3); }
void pmaxsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xEE); }
void pmaxub(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xDE); }
void pminsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xEA); }
void pminub(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xDA); }
void psadbw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xF6); }
void paddq(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xD4); }
void pmuludq(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xF4); }
void psubq(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xFB); }
void paddb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xFC); }
void paddw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xFD); }
void paddd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xFE); }
void paddsb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xEC); }
void paddsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xED); }
void paddusb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xDC); }
void paddusw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xDD); }
void pcmpeqb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x74); }
void pcmpeqw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x75); }
void pcmpeqd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x76); }
void pcmpgtb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x64); }
void pcmpgtw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x65); }
void pcmpgtd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x66); }
void psllw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xF1); }
void pslld(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xF2); }
void psllq(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xF3); }
void psraw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xE1); }
void psrad(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xE2); }
void psrlw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xD1); }
void psrld(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xD2); }
void psrlq(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xD3); }
void psubb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xF8); }
void psubw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xF9); }
void psubd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xFA); }
void psubsb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xE8); }
void psubsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xE9); }
void psubusb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xD8); }
void psubusw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0xD9); }
void psllw(const Mmx& mmx, int imm8) { opMMX_IMM(mmx, imm8, 0x71, 6); }
void pslld(const Mmx& mmx, int imm8) { opMMX_IMM(mmx, imm8, 0x72, 6); }
void psllq(const Mmx& mmx, int imm8) { opMMX_IMM(mmx, imm8, 0x73, 6); }
void psraw(const Mmx& mmx, int imm8) { opMMX_IMM(mmx, imm8, 0x71, 4); }
void psrad(const Mmx& mmx, int imm8) { opMMX_IMM(mmx, imm8, 0x72, 4); }
void psrlw(const Mmx& mmx, int imm8) { opMMX_IMM(mmx, imm8, 0x71, 2); }
void psrld(const Mmx& mmx, int imm8) { opMMX_IMM(mmx, imm8, 0x72, 2); }
void psrlq(const Mmx& mmx, int imm8) { opMMX_IMM(mmx, imm8, 0x73, 2); }
void pslldq(const Xmm& xmm, int imm8) { opMMX_IMM(xmm, imm8, 0x73, 7); }
void psrldq(const Xmm& xmm, int imm8) { opMMX_IMM(xmm, imm8, 0x73, 3); }
void pshufw(const Mmx& mmx, const Operand& op, uint8 imm8) { opMMX(mmx, op, 0x70, 0x00, imm8); }
void pshuflw(const Mmx& mmx, const Operand& op, uint8 imm8) { opMMX(mmx, op, 0x70, 0xF2, imm8); }
void pshufhw(const Mmx& mmx, const Operand& op, uint8 imm8) { opMMX(mmx, op, 0x70, 0xF3, imm8); }
void pshufd(const Mmx& mmx, const Operand& op, uint8 imm8) { opMMX(mmx, op, 0x70, 0x66, imm8); }
void movdqa(const Xmm& xmm, const Operand& op) { opMMX(xmm, op, 0x6F, 0x66); }
void movdqa(const Address& addr, const Xmm& xmm) { db(0x66); opModM(addr, xmm, 0x0F, 0x7F); }
void movdqu(const Xmm& xmm, const Operand& op) { opMMX(xmm, op, 0x6F, 0xF3); }
void movdqu(const Address& addr, const Xmm& xmm) { db(0xF3); opModM(addr, xmm, 0x0F, 0x7F); }
void movaps(const Xmm& xmm, const Operand& op) { opMMX(xmm, op, 0x28, 0x100); }
void movaps(const Address& addr, const Xmm& xmm) { opModM(addr, xmm, 0x0F, 0x29); }
void movss(const Xmm& xmm, const Operand& op) { opMMX(xmm, op, 0x10, 0xF3); }
void movss(const Address& addr, const Xmm& xmm) { db(0xF3); opModM(addr, xmm, 0x0F, 0x11); }
void movups(const Xmm& xmm, const Operand& op) { opMMX(xmm, op, 0x10, 0x100); }
void movups(const Address& addr, const Xmm& xmm) { opModM(addr, xmm, 0x0F, 0x11); }
void movapd(const Xmm& xmm, const Operand& op) { opMMX(xmm, op, 0x28, 0x66); }
void movapd(const Address& addr, const Xmm& xmm) { db(0x66); opModM(addr, xmm, 0x0F, 0x29); }
void movsd(const Xmm& xmm, const Operand& op) { opMMX(xmm, op, 0x10, 0xF2); }
void movsd(const Address& addr, const Xmm& xmm) { db(0xF2); opModM(addr, xmm, 0x0F, 0x11); }
void movupd(const Xmm& xmm, const Operand& op) { opMMX(xmm, op, 0x10, 0x66); }
void movupd(const Address& addr, const Xmm& xmm) { db(0x66); opModM(addr, xmm, 0x0F, 0x11); }
void addps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x58, 0x100, isXMM_XMMorMEM); }
void addss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x58, 0xF3, isXMM_XMMorMEM); }
void addpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x58, 0x66, isXMM_XMMorMEM); }
void addsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x58, 0xF2, isXMM_XMMorMEM); }
void andnps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x55, 0x100, isXMM_XMMorMEM); }
void andnpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x55, 0x66, isXMM_XMMorMEM); }
void andps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x54, 0x100, isXMM_XMMorMEM); }
void andpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x54, 0x66, isXMM_XMMorMEM); }
void cmpps(const Xmm& xmm, const Operand& op, uint8 imm8) { opGen(xmm, op, 0xC2, 0x100, isXMM_XMMorMEM, imm8); }
void cmpss(const Xmm& xmm, const Operand& op, uint8 imm8) { opGen(xmm, op, 0xC2, 0xF3, isXMM_XMMorMEM, imm8); }
void cmppd(const Xmm& xmm, const Operand& op, uint8 imm8) { opGen(xmm, op, 0xC2, 0x66, isXMM_XMMorMEM, imm8); }
void cmpsd(const Xmm& xmm, const Operand& op, uint8 imm8) { opGen(xmm, op, 0xC2, 0xF2, isXMM_XMMorMEM, imm8); }
void divps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5E, 0x100, isXMM_XMMorMEM); }
void divss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5E, 0xF3, isXMM_XMMorMEM); }
void divpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5E, 0x66, isXMM_XMMorMEM); }
void divsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5E, 0xF2, isXMM_XMMorMEM); }
void maxps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5F, 0x100, isXMM_XMMorMEM); }
void maxss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5F, 0xF3, isXMM_XMMorMEM); }
void maxpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5F, 0x66, isXMM_XMMorMEM); }
void maxsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5F, 0xF2, isXMM_XMMorMEM); }
void minps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5D, 0x100, isXMM_XMMorMEM); }
void minss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5D, 0xF3, isXMM_XMMorMEM); }
void minpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5D, 0x66, isXMM_XMMorMEM); }
void minsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5D, 0xF2, isXMM_XMMorMEM); }
void mulps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x59, 0x100, isXMM_XMMorMEM); }
void mulss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x59, 0xF3, isXMM_XMMorMEM); }
void mulpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x59, 0x66, isXMM_XMMorMEM); }
void mulsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x59, 0xF2, isXMM_XMMorMEM); }
void orps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x56, 0x100, isXMM_XMMorMEM); }
void orpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x56, 0x66, isXMM_XMMorMEM); }
void rcpps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x53, 0x100, isXMM_XMMorMEM); }
void rcpss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x53, 0xF3, isXMM_XMMorMEM); }
void rsqrtps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x52, 0x100, isXMM_XMMorMEM); }
void rsqrtss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x52, 0xF3, isXMM_XMMorMEM); }
void shufps(const Xmm& xmm, const Operand& op, uint8 imm8) { opGen(xmm, op, 0xC6, 0x100, isXMM_XMMorMEM, imm8); }
void shufpd(const Xmm& xmm, const Operand& op, uint8 imm8) { opGen(xmm, op, 0xC6, 0x66, isXMM_XMMorMEM, imm8); }
void sqrtps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x51, 0x100, isXMM_XMMorMEM); }
void sqrtss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x51, 0xF3, isXMM_XMMorMEM); }
void sqrtpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x51, 0x66, isXMM_XMMorMEM); }
void sqrtsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x51, 0xF2, isXMM_XMMorMEM); }
void subps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5C, 0x100, isXMM_XMMorMEM); }
void subss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5C, 0xF3, isXMM_XMMorMEM); }
void subpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5C, 0x66, isXMM_XMMorMEM); }
void subsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5C, 0xF2, isXMM_XMMorMEM); }
void unpckhps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x15, 0x100, isXMM_XMMorMEM); }
void unpckhpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x15, 0x66, isXMM_XMMorMEM); }
void unpcklps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x14, 0x100, isXMM_XMMorMEM); }
void unpcklpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x14, 0x66, isXMM_XMMorMEM); }
void xorps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x57, 0x100, isXMM_XMMorMEM); }
void xorpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x57, 0x66, isXMM_XMMorMEM); }
void maskmovdqu(const Xmm& reg1, const Xmm& reg2) { db(0x66);  opModR(reg1, reg2, 0x0F, 0xF7); }
void movhlps(const Xmm& reg1, const Xmm& reg2) {  opModR(reg1, reg2, 0x0F, 0x12); }
void movlhps(const Xmm& reg1, const Xmm& reg2) {  opModR(reg1, reg2, 0x0F, 0x16); }
void punpckhqdq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x6D, 0x66, isXMM_XMMorMEM); }
void punpcklqdq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x6C, 0x66, isXMM_XMMorMEM); }
void comiss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x2F, 0x100, isXMM_XMMorMEM); }
void ucomiss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x2E, 0x100, isXMM_XMMorMEM); }
void comisd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x2F, 0x66, isXMM_XMMorMEM); }
void ucomisd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x2E, 0x66, isXMM_XMMorMEM); }
void cvtpd2ps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5A, 0x66, isXMM_XMMorMEM); }
void cvtps2pd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5A, 0x100, isXMM_XMMorMEM); }
void cvtsd2ss(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5A, 0xF2, isXMM_XMMorMEM); }
void cvtss2sd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5A, 0xF3, isXMM_XMMorMEM); }
void cvtpd2dq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xE6, 0xF2, isXMM_XMMorMEM); }
void cvttpd2dq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xE6, 0x66, isXMM_XMMorMEM); }
void cvtdq2pd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xE6, 0xF3, isXMM_XMMorMEM); }
void cvtps2dq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5B, 0x66, isXMM_XMMorMEM); }
void cvttps2dq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5B, 0xF3, isXMM_XMMorMEM); }
void cvtdq2ps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x5B, 0x100, isXMM_XMMorMEM); }
void addsubpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xD0, 0x66, isXMM_XMMorMEM); }
void addsubps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xD0, 0xF2, isXMM_XMMorMEM); }
void haddpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x7C, 0x66, isXMM_XMMorMEM); }
void haddps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x7C, 0xF2, isXMM_XMMorMEM); }
void hsubpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x7D, 0x66, isXMM_XMMorMEM); }
void hsubps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x7D, 0xF2, isXMM_XMMorMEM); }
void movddup(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x12, 0xF2, isXMM_XMMorMEM); }
void movshdup(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x16, 0xF3, isXMM_XMMorMEM); }
void movsldup(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x12, 0xF3, isXMM_XMMorMEM); }
void cvtpi2ps(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2A, 0x100, isXMM_MMXorMEM); }
void cvtps2pi(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2D, 0x100, isMMX_XMMorMEM); }
void cvtsi2ss(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2A, 0xF3, isXMM_REG32orMEM); }
void cvtss2si(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2D, 0xF3, isREG32_XMMorMEM); }
void cvttps2pi(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2C, 0x100, isMMX_XMMorMEM); }
void cvttss2si(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2C, 0xF3, isREG32_XMMorMEM); }
void cvtpi2pd(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2A, 0x66, isXMM_MMXorMEM); }
void cvtpd2pi(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2D, 0x66, isMMX_XMMorMEM); }
void cvtsi2sd(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2A, 0xF2, isXMM_REG32orMEM); }
void cvtsd2si(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2D, 0xF2, isREG32_XMMorMEM); }
void cvttpd2pi(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2C, 0x66, isMMX_XMMorMEM); }
void cvttsd2si(const Operand& reg, const Operand& op) { opGen(reg, op, 0x2C, 0xF2, isREG32_XMMorMEM); }
void prefetcht0(const Address& addr) { opModM(addr, Reg32(1), 0x0F, B00011000); }
void prefetcht1(const Address& addr) { opModM(addr, Reg32(2), 0x0F, B00011000); }
void prefetcht2(const Address& addr) { opModM(addr, Reg32(3), 0x0F, B00011000); }
void prefetchnta(const Address& addr) { opModM(addr, Reg32(0), 0x0F, B00011000); }
void movhps(const Operand& op1, const Operand& op2) { opMovXMM(op1, op2, 0x16, 0x100); }
void movlps(const Operand& op1, const Operand& op2) { opMovXMM(op1, op2, 0x12, 0x100); }
void movhpd(const Operand& op1, const Operand& op2) { opMovXMM(op1, op2, 0x16, 0x66); }
void movlpd(const Operand& op1, const Operand& op2) { opMovXMM(op1, op2, 0x12, 0x66); }
void cmovo(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 0); }
void jo(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x70, 0x80, 0x0F); }
void seto(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 0); }
void cmovno(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 1); }
void jno(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x71, 0x81, 0x0F); }
void setno(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 1); }
void cmovb(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 2); }
void jb(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x72, 0x82, 0x0F); }
void setb(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 2); }
void cmovnae(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 2); }
void jnae(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x72, 0x82, 0x0F); }
void setnae(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 2); }
void cmovnb(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 3); }
void jnb(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x73, 0x83, 0x0F); }
void setnb(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 3); }
void cmovae(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 3); }
void jae(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x73, 0x83, 0x0F); }
void setae(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 3); }
void cmove(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 4); }
void je(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x74, 0x84, 0x0F); }
void sete(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 4); }
void cmovz(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 4); }
void jz(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x74, 0x84, 0x0F); }
void setz(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 4); }
void cmovne(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 5); }
void jne(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x75, 0x85, 0x0F); }
void setne(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 5); }
void cmovnz(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 5); }
void jnz(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x75, 0x85, 0x0F); }
void setnz(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 5); }
void cmovbe(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 6); }
void jbe(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x76, 0x86, 0x0F); }
void setbe(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 6); }
void cmovna(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 6); }
void jna(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x76, 0x86, 0x0F); }
void setna(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 6); }
void cmovnbe(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 7); }
void jnbe(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x77, 0x87, 0x0F); }
void setnbe(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 7); }
void cmova(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 7); }
void ja(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x77, 0x87, 0x0F); }
void seta(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 7); }
void cmovs(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 8); }
void js(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x78, 0x88, 0x0F); }
void sets(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 8); }
void cmovns(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 9); }
void jns(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x79, 0x89, 0x0F); }
void setns(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 9); }
void cmovp(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 10); }
void jp(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7A, 0x8A, 0x0F); }
void setp(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 10); }
void cmovpe(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 10); }
void jpe(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7A, 0x8A, 0x0F); }
void setpe(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 10); }
void cmovnp(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 11); }
void jnp(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7B, 0x8B, 0x0F); }
void setnp(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 11); }
void cmovpo(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 11); }
void jpo(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7B, 0x8B, 0x0F); }
void setpo(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 11); }
void cmovl(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 12); }
void jl(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7C, 0x8C, 0x0F); }
void setl(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 12); }
void cmovnge(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 12); }
void jnge(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7C, 0x8C, 0x0F); }
void setnge(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 12); }
void cmovnl(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 13); }
void jnl(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7D, 0x8D, 0x0F); }
void setnl(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 13); }
void cmovge(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 13); }
void jge(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7D, 0x8D, 0x0F); }
void setge(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 13); }
void cmovle(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 14); }
void jle(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7E, 0x8E, 0x0F); }
void setle(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 14); }
void cmovng(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 14); }
void jng(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7E, 0x8E, 0x0F); }
void setng(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 14); }
void cmovnle(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 15); }
void jnle(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7F, 0x8F, 0x0F); }
void setnle(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 15); }
void cmovg(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 15); }
void jg(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7F, 0x8F, 0x0F); }
void setg(const Operand& op) { opR_ModM(op, 8, 3, 0, 0x0F, B10010000 | 15); }
#ifdef XBYAK64
void cdqe() { db(0x48); db(0x98); }
#else
void aaa() { db(0x37); }
void aad() { db(0xD5); db(0x0A); }
void aam() { db(0xD4); db(0x0A); }
void aas() { db(0x3F); }
void daa() { db(0x27); }
void das() { db(0x2F); }
void popad() { db(0x61); }
void popfd() { db(0x9D); }
void pusha() { db(0x60); }
void pushad() { db(0x60); }
void pushfd() { db(0x9C); }
void popa() { db(0x61); }
#endif
void cbw() { db(0x66); db(0x98); }
void cdq() { db(0x99); }
void clc() { db(0xF8); }
void cld() { db(0xFC); }
void cli() { db(0xFA); }
void cmc() { db(0xF5); }
void cpuid() { db(0x0F); db(0xA2); }
void cwd() { db(0x66); db(0x99); }
void cwde() { db(0x98); }
void lahf() { db(0x9F); }
void lock() { db(0xF0); }
void nop() { db(0x90); }
void sahf() { db(0x9E); }
void stc() { db(0xF9); }
void std() { db(0xFD); }
void sti() { db(0xFB); }
void emms() { db(0x0F); db(0x77); }
void pause() { db(0xF3); db(0x90); }
void sfence() { db(0x0F); db(0xAE); db(0xF8); }
void lfence() { db(0x0F); db(0xAE); db(0xE8); }
void mfence() { db(0x0F); db(0xAE); db(0xF0); }
void monitor() { db(0x0F); db(0x01); db(0xC8); }
void mwait() { db(0x0F); db(0x01); db(0xC9); }
void rdmsr() { db(0x0F); db(0x32); }
void rdpmc() { db(0x0F); db(0x33); }
void rdtsc() { db(0x0F); db(0x31); }
void wait() { db(0x9B); }
void wbinvd() { db(0x0F); db(0x09); }
void wrmsr() { db(0x0F); db(0x30); }
void xlatb() { db(0xD7); }
void popf() { db(0x9D); }
void pushf() { db(0x9C); }
void adc(const Operand& op1, const Operand& op2) { opRM_RM(op1, op2, 0x10); }
void adc(const Operand& op, uint32 imm) { opRM_I(op, imm, 0x10, 2); }
void add(const Operand& op1, const Operand& op2) { opRM_RM(op1, op2, 0x00); }
void add(const Operand& op, uint32 imm) { opRM_I(op, imm, 0x00, 0); }
void and(const Operand& op1, const Operand& op2) { opRM_RM(op1, op2, 0x20); }
void and(const Operand& op, uint32 imm) { opRM_I(op, imm, 0x20, 4); }
void cmp(const Operand& op1, const Operand& op2) { opRM_RM(op1, op2, 0x38); }
void cmp(const Operand& op, uint32 imm) { opRM_I(op, imm, 0x38, 7); }
void or(const Operand& op1, const Operand& op2) { opRM_RM(op1, op2, 0x08); }
void or(const Operand& op, uint32 imm) { opRM_I(op, imm, 0x08, 1); }
void sbb(const Operand& op1, const Operand& op2) { opRM_RM(op1, op2, 0x18); }
void sbb(const Operand& op, uint32 imm) { opRM_I(op, imm, 0x18, 3); }
void sub(const Operand& op1, const Operand& op2) { opRM_RM(op1, op2, 0x28); }
void sub(const Operand& op, uint32 imm) { opRM_I(op, imm, 0x28, 5); }
void xor(const Operand& op1, const Operand& op2) { opRM_RM(op1, op2, 0x30); }
void xor(const Operand& op, uint32 imm) { opRM_I(op, imm, 0x30, 6); }
void dec(const Operand& op) { opIncDec(op, 0x48, 1); }
void inc(const Operand& op) { opIncDec(op, 0x40, 0); }
void div(const Operand& op) { opR_ModM(op, 0, 3, 6, 0xF6); }
void idiv(const Operand& op) { opR_ModM(op, 0, 3, 7, 0xF6); }
void imul(const Operand& op) { opR_ModM(op, 0, 3, 5, 0xF6); }
void mul(const Operand& op) { opR_ModM(op, 0, 3, 4, 0xF6); }
void neg(const Operand& op) { opR_ModM(op, 0, 3, 3, 0xF6); }
void not(const Operand& op) { opR_ModM(op, 0, 3, 2, 0xF6); }
void rcl(const Operand& op, int imm) { opShift(op, imm, 2); }
void rcl(const Operand& op, const Reg8& cl) { opShift(op, cl, 2); }
void rcr(const Operand& op, int imm) { opShift(op, imm, 3); }
void rcr(const Operand& op, const Reg8& cl) { opShift(op, cl, 3); }
void rol(const Operand& op, int imm) { opShift(op, imm, 0); }
void rol(const Operand& op, const Reg8& cl) { opShift(op, cl, 0); }
void ror(const Operand& op, int imm) { opShift(op, imm, 1); }
void ror(const Operand& op, const Reg8& cl) { opShift(op, cl, 1); }
void sar(const Operand& op, int imm) { opShift(op, imm, 7); }
void sar(const Operand& op, const Reg8& cl) { opShift(op, cl, 7); }
void shl(const Operand& op, int imm) { opShift(op, imm, 4); }
void shl(const Operand& op, const Reg8& cl) { opShift(op, cl, 4); }
void shr(const Operand& op, int imm) { opShift(op, imm, 5); }
void shr(const Operand& op, const Reg8& cl) { opShift(op, cl, 5); }
void sal(const Operand& op, int imm) { opShift(op, imm, 4); }
void sal(const Operand& op, const Reg8& cl) { opShift(op, cl, 4); }
void shld(const Operand& op, const Reg& reg, uint8 imm) { opShxd(op, reg, imm, 0xA4); }
void shld(const Operand& op, const Reg& reg, const Reg8& cl) { opShxd(op, reg, 0, 0xA4, &cl); }
void shrd(const Operand& op, const Reg& reg, uint8 imm) { opShxd(op, reg, imm, 0xAC); }
void shrd(const Operand& op, const Reg& reg, const Reg8& cl) { opShxd(op, reg, 0, 0xAC, &cl); }
void bsf(const Reg&reg, const Operand& op) { opModRM(reg, op, op.isREG(16 | i32e), op.isMEM(), 0x0F, 0xBC); }
void bsr(const Reg&reg, const Operand& op) { opModRM(reg, op, op.isREG(16 | i32e), op.isMEM(), 0x0F, 0xBD); }
void pshufb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x00, 0x66, 256, 0x38); }
void phaddw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x01, 0x66, 256, 0x38); }
void phaddd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x02, 0x66, 256, 0x38); }
void phaddsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x03, 0x66, 256, 0x38); }
void pmaddubsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x04, 0x66, 256, 0x38); }
void phsubw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x05, 0x66, 256, 0x38); }
void phsubd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x06, 0x66, 256, 0x38); }
void phsubsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x07, 0x66, 256, 0x38); }
void psignb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x08, 0x66, 256, 0x38); }
void psignw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x09, 0x66, 256, 0x38); }
void psignd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x0A, 0x66, 256, 0x38); }
void pmulhrsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x0B, 0x66, 256, 0x38); }
void pabsb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x1C, 0x66, 256, 0x38); }
void pabsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x1D, 0x66, 256, 0x38); }
void pabsd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x1E, 0x66, 256, 0x38); }
void palignr(const Mmx& mmx, const Operand& op, int imm) { opMMX(mmx, op, 0x0f, 0x66, static_cast<uint8>(imm), 0x3a); }
void blendvpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x15, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void blendvps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x14, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void packusdw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x2B, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pblendvb(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x10, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pcmpeqq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x29, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void ptest(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x17, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovsxbw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x20, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovsxbd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x21, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovsxbq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x22, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovsxwd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x23, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovsxwq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x24, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovsxdq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x25, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovzxbw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x30, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovzxbd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x31, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovzxbq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x32, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovzxwd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x33, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovzxwq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x34, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmovzxdq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x35, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pminsb(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x38, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pminsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x39, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pminuw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3A, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pminud(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3B, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmaxsb(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3C, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmaxsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3D, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmaxuw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3E, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmaxud(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3F, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmuldq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x28, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pmulld(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x40, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void phminposuw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x41, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void pcmpgtq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x37, 0x66, isXMM_XMMorMEM, 256, 0x38); }
void blendpd(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x0D, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void blendps(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x0C, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void dppd(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x41, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void dpps(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x40, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void mpsadbw(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x42, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void pblendw(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x0E, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void roundps(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x08, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void roundpd(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x09, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void roundss(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x0A, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void roundsd(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x0B, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void pcmpestrm(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x60, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void pcmpestri(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x61, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void pcmpistrm(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x62, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void pcmpistri(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x63, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void ldmxcsr(const Address& addr) { opModM(addr, Reg32(2), 0x0F, 0xAE); }
void stmxcsr(const Address& addr) { opModM(addr, Reg32(3), 0x0F, 0xAE); }
void clflush(const Address& addr) { opModM(addr, Reg32(7), 0x0F, 0xAE); }
void movntpd(const Address& addr, const Xmm& reg) { opModM(addr, Reg16(reg.getIdx()), 0x0F, 0x2B); }
void movntdq(const Address& addr, const Xmm& reg) { opModM(addr, Reg16(reg.getIdx()), 0x0F, 0xE7); }
void movsx(const Reg& reg, const Operand& op) { opMovxx(reg, op, 0xBE); }
void movzx(const Reg& reg, const Operand& op) { opMovxx(reg, op, 0xB6); }
