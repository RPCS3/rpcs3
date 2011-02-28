const char *getVersionString() const { return "2.991"; }
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
void seto(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 0); }
void cmovno(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 1); }
void jno(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x71, 0x81, 0x0F); }
void setno(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 1); }
void cmovb(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 2); }
void jb(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x72, 0x82, 0x0F); }
void setb(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 2); }
void cmovc(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 2); }
void jc(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x72, 0x82, 0x0F); }
void setc(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 2); }
void cmovnae(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 2); }
void jnae(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x72, 0x82, 0x0F); }
void setnae(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 2); }
void cmovnb(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 3); }
void jnb(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x73, 0x83, 0x0F); }
void setnb(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 3); }
void cmovae(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 3); }
void jae(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x73, 0x83, 0x0F); }
void setae(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 3); }
void cmovnc(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 3); }
void jnc(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x73, 0x83, 0x0F); }
void setnc(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 3); }
void cmove(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 4); }
void je(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x74, 0x84, 0x0F); }
void sete(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 4); }
void cmovz(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 4); }
void jz(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x74, 0x84, 0x0F); }
void setz(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 4); }
void cmovne(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 5); }
void jne(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x75, 0x85, 0x0F); }
void setne(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 5); }
void cmovnz(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 5); }
void jnz(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x75, 0x85, 0x0F); }
void setnz(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 5); }
void cmovbe(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 6); }
void jbe(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x76, 0x86, 0x0F); }
void setbe(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 6); }
void cmovna(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 6); }
void jna(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x76, 0x86, 0x0F); }
void setna(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 6); }
void cmovnbe(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 7); }
void jnbe(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x77, 0x87, 0x0F); }
void setnbe(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 7); }
void cmova(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 7); }
void ja(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x77, 0x87, 0x0F); }
void seta(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 7); }
void cmovs(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 8); }
void js(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x78, 0x88, 0x0F); }
void sets(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 8); }
void cmovns(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 9); }
void jns(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x79, 0x89, 0x0F); }
void setns(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 9); }
void cmovp(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 10); }
void jp(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7A, 0x8A, 0x0F); }
void setp(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 10); }
void cmovpe(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 10); }
void jpe(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7A, 0x8A, 0x0F); }
void setpe(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 10); }
void cmovnp(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 11); }
void jnp(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7B, 0x8B, 0x0F); }
void setnp(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 11); }
void cmovpo(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 11); }
void jpo(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7B, 0x8B, 0x0F); }
void setpo(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 11); }
void cmovl(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 12); }
void jl(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7C, 0x8C, 0x0F); }
void setl(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 12); }
void cmovnge(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 12); }
void jnge(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7C, 0x8C, 0x0F); }
void setnge(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 12); }
void cmovnl(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 13); }
void jnl(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7D, 0x8D, 0x0F); }
void setnl(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 13); }
void cmovge(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 13); }
void jge(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7D, 0x8D, 0x0F); }
void setge(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 13); }
void cmovle(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 14); }
void jle(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7E, 0x8E, 0x0F); }
void setle(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 14); }
void cmovng(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 14); }
void jng(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7E, 0x8E, 0x0F); }
void setng(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 14); }
void cmovnle(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 15); }
void jnle(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7F, 0x8F, 0x0F); }
void setnle(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 15); }
void cmovg(const Reg32e& reg, const Operand& op) { opModRM(reg, op, op.isREG(i32e), op.isMEM(), 0x0F, B01000000 | 15); }
void jg(const char *label, LabelType type = T_AUTO) { opJmp(label, type, 0x7F, 0x8F, 0x0F); }
void setg(const Operand& op) { opR_ModM(op, 8, 0, 0x0F, B10010000 | 15); }
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
void rdtscp() { db(0x0F); db(0x01); db(0xF9); }
void wait() { db(0x9B); }
void wbinvd() { db(0x0F); db(0x09); }
void wrmsr() { db(0x0F); db(0x30); }
void xlatb() { db(0xD7); }
void popf() { db(0x9D); }
void pushf() { db(0x9C); }
void vzeroall() { db(0xC5); db(0xFC); db(0x77); }
void vzeroupper() { db(0xC5); db(0xF8); db(0x77); }
void xgetbv() { db(0x0F); db(0x01); db(0xD0); }
void f2xm1() { db(0xD9); db(0xF0); }
void fabs() { db(0xD9); db(0xE1); }
void faddp() { db(0xDE); db(0xC1); }
void fchs() { db(0xD9); db(0xE0); }
void fcom() { db(0xD8); db(0xD1); }
void fcomp() { db(0xD8); db(0xD9); }
void fcompp() { db(0xDE); db(0xD9); }
void fcos() { db(0xD9); db(0xFF); }
void fdecstp() { db(0xD9); db(0xF6); }
void fdivp() { db(0xDE); db(0xF9); }
void fdivrp() { db(0xDE); db(0xF1); }
void fincstp() { db(0xD9); db(0xF7); }
void fld1() { db(0xD9); db(0xE8); }
void fldl2t() { db(0xD9); db(0xE9); }
void fldl2e() { db(0xD9); db(0xEA); }
void fldpi() { db(0xD9); db(0xEB); }
void fldlg2() { db(0xD9); db(0xEC); }
void fldln2() { db(0xD9); db(0xED); }
void fldz() { db(0xD9); db(0xEE); }
void fmulp() { db(0xDE); db(0xC9); }
void fnop() { db(0xD9); db(0xD0); }
void fpatan() { db(0xD9); db(0xF3); }
void fprem() { db(0xD9); db(0xF8); }
void fprem1() { db(0xD9); db(0xF5); }
void fptan() { db(0xD9); db(0xF2); }
void frndint() { db(0xD9); db(0xFC); }
void fscale() { db(0xD9); db(0xFD); }
void fsin() { db(0xD9); db(0xFE); }
void fsincos() { db(0xD9); db(0xFB); }
void fsqrt() { db(0xD9); db(0xFA); }
void fsubp() { db(0xDE); db(0xE9); }
void fsubrp() { db(0xDE); db(0xE1); }
void ftst() { db(0xD9); db(0xE4); }
void fucom() { db(0xDD); db(0xE1); }
void fucomp() { db(0xDD); db(0xE9); }
void fucompp() { db(0xDA); db(0xE9); }
void fxam() { db(0xD9); db(0xE5); }
void fxch() { db(0xD9); db(0xC9); }
void fxtract() { db(0xD9); db(0xF4); }
void fyl2x() { db(0xD9); db(0xF1); }
void fyl2xp1() { db(0xD9); db(0xF9); }
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
void div(const Operand& op) { opR_ModM(op, 0, 6, 0xF6); }
void idiv(const Operand& op) { opR_ModM(op, 0, 7, 0xF6); }
void imul(const Operand& op) { opR_ModM(op, 0, 5, 0xF6); }
void mul(const Operand& op) { opR_ModM(op, 0, 4, 0xF6); }
void neg(const Operand& op) { opR_ModM(op, 0, 3, 0xF6); }
void not(const Operand& op) { opR_ModM(op, 0, 2, 0xF6); }
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
void pshufb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x00, 0x66, NONE, 0x38); }
void phaddw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x01, 0x66, NONE, 0x38); }
void phaddd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x02, 0x66, NONE, 0x38); }
void phaddsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x03, 0x66, NONE, 0x38); }
void pmaddubsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x04, 0x66, NONE, 0x38); }
void phsubw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x05, 0x66, NONE, 0x38); }
void phsubd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x06, 0x66, NONE, 0x38); }
void phsubsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x07, 0x66, NONE, 0x38); }
void psignb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x08, 0x66, NONE, 0x38); }
void psignw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x09, 0x66, NONE, 0x38); }
void psignd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x0A, 0x66, NONE, 0x38); }
void pmulhrsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x0B, 0x66, NONE, 0x38); }
void pabsb(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x1C, 0x66, NONE, 0x38); }
void pabsw(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x1D, 0x66, NONE, 0x38); }
void pabsd(const Mmx& mmx, const Operand& op) { opMMX(mmx, op, 0x1E, 0x66, NONE, 0x38); }
void palignr(const Mmx& mmx, const Operand& op, int imm) { opMMX(mmx, op, 0x0f, 0x66, static_cast<uint8>(imm), 0x3a); }
void blendvpd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x15, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void blendvps(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x14, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void packusdw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x2B, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pblendvb(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x10, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pcmpeqq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x29, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void ptest(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x17, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovsxbw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x20, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovsxbd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x21, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovsxbq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x22, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovsxwd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x23, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovsxwq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x24, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovsxdq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x25, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovzxbw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x30, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovzxbd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x31, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovzxbq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x32, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovzxwd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x33, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovzxwq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x34, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmovzxdq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x35, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pminsb(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x38, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pminsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x39, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pminuw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3A, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pminud(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3B, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmaxsb(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3C, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmaxsd(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3D, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmaxuw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3E, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmaxud(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x3F, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmuldq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x28, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pmulld(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x40, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void phminposuw(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x41, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void pcmpgtq(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0x37, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void aesdec(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xDE, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void aesdeclast(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xDF, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void aesenc(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xDC, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void aesenclast(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xDD, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
void aesimc(const Xmm& xmm, const Operand& op) { opGen(xmm, op, 0xDB, 0x66, isXMM_XMMorMEM, NONE, 0x38); }
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
void pclmulqdq(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0x44, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void aeskeygenassist(const Xmm& xmm, const Operand& op, int imm) { opGen(xmm, op, 0xDF, 0x66, isXMM_XMMorMEM, static_cast<uint8>(imm), 0x3A); }
void ldmxcsr(const Address& addr) { opModM(addr, Reg32(2), 0x0F, 0xAE); }
void stmxcsr(const Address& addr) { opModM(addr, Reg32(3), 0x0F, 0xAE); }
void clflush(const Address& addr) { opModM(addr, Reg32(7), 0x0F, 0xAE); }
void movntpd(const Address& addr, const Xmm& reg) { opModM(addr, Reg16(reg.getIdx()), 0x0F, 0x2B); }
void movntdq(const Address& addr, const Xmm& reg) { opModM(addr, Reg16(reg.getIdx()), 0x0F, 0xE7); }
void movsx(const Reg& reg, const Operand& op) { opMovxx(reg, op, 0xBE); }
void movzx(const Reg& reg, const Operand& op) { opMovxx(reg, op, 0xB6); }
void fadd(const Address& addr) { opFpuMem(addr, 0x00, 0xD8, 0xDC, 0, 0); }
void fiadd(const Address& addr) { opFpuMem(addr, 0xDE, 0xDA, 0x00, 0, 0); }
void fcom(const Address& addr) { opFpuMem(addr, 0x00, 0xD8, 0xDC, 2, 0); }
void fcomp(const Address& addr) { opFpuMem(addr, 0x00, 0xD8, 0xDC, 3, 0); }
void fdiv(const Address& addr) { opFpuMem(addr, 0x00, 0xD8, 0xDC, 6, 0); }
void fidiv(const Address& addr) { opFpuMem(addr, 0xDE, 0xDA, 0x00, 6, 0); }
void fdivr(const Address& addr) { opFpuMem(addr, 0x00, 0xD8, 0xDC, 7, 0); }
void fidivr(const Address& addr) { opFpuMem(addr, 0xDE, 0xDA, 0x00, 7, 0); }
void ficom(const Address& addr) { opFpuMem(addr, 0xDE, 0xDA, 0x00, 2, 0); }
void ficomp(const Address& addr) { opFpuMem(addr, 0xDE, 0xDA, 0x00, 3, 0); }
void fild(const Address& addr) { opFpuMem(addr, 0xDF, 0xDB, 0xDF, 0, 5); }
void fist(const Address& addr) { opFpuMem(addr, 0xDF, 0xDB, 0x00, 2, 0); }
void fistp(const Address& addr) { opFpuMem(addr, 0xDF, 0xDB, 0xDF, 3, 7); }
void fisttp(const Address& addr) { opFpuMem(addr, 0xDF, 0xDB, 0xDD, 1, 0); }
void fld(const Address& addr) { opFpuMem(addr, 0x00, 0xD9, 0xDD, 0, 0); }
void fmul(const Address& addr) { opFpuMem(addr, 0x00, 0xD8, 0xDC, 1, 0); }
void fimul(const Address& addr) { opFpuMem(addr, 0xDE, 0xDA, 0x00, 1, 0); }
void fst(const Address& addr) { opFpuMem(addr, 0x00, 0xD9, 0xDD, 2, 0); }
void fstp(const Address& addr) { opFpuMem(addr, 0x00, 0xD9, 0xDD, 3, 0); }
void fsub(const Address& addr) { opFpuMem(addr, 0x00, 0xD8, 0xDC, 4, 0); }
void fisub(const Address& addr) { opFpuMem(addr, 0xDE, 0xDA, 0x00, 4, 0); }
void fsubr(const Address& addr) { opFpuMem(addr, 0x00, 0xD8, 0xDC, 5, 0); }
void fisubr(const Address& addr) { opFpuMem(addr, 0xDE, 0xDA, 0x00, 5, 0); }
void fadd(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xD8C0, 0xDCC0); }
void faddp(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0x0000, 0xDEC0); }
void fcmovb(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDAC0, 0x00C0); }
void fcmove(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDAC8, 0x00C8); }
void fcmovbe(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDAD0, 0x00D0); }
void fcmovu(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDAD8, 0x00D8); }
void fcmovnb(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDBC0, 0x00C0); }
void fcmovne(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDBC8, 0x00C8); }
void fcmovnbe(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDBD0, 0x00D0); }
void fcmovnu(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDBD8, 0x00D8); }
void fcomi(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDBF0, 0x00F0); }
void fcomip(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDFF0, 0x00F0); }
void fucomi(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDBE8, 0x00E8); }
void fucomip(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xDFE8, 0x00E8); }
void fdiv(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xD8F0, 0xDCF8); }
void fdivp(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0x0000, 0xDEF8); }
void fdivr(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xD8F8, 0xDCF0); }
void fdivrp(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0x0000, 0xDEF0); }
void fmul(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xD8C8, 0xDCC8); }
void fmulp(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0x0000, 0xDEC8); }
void fsub(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xD8E0, 0xDCE8); }
void fsubp(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0x0000, 0xDEE8); }
void fsubr(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0xD8E8, 0xDCE0); }
void fsubrp(const Fpu& reg1, const Fpu& reg2) { opFpuFpu(reg1, reg2, 0x0000, 0xDEE0); }
void fcom(const Fpu& reg) { opFpu(reg, 0xD8, 0xD0); }
void fcomp(const Fpu& reg) { opFpu(reg, 0xD8, 0xD8); }
void ffree(const Fpu& reg) { opFpu(reg, 0xDD, 0xC0); }
void fld(const Fpu& reg) { opFpu(reg, 0xD9, 0xC0); }
void fst(const Fpu& reg) { opFpu(reg, 0xDD, 0xD0); }
void fstp(const Fpu& reg) { opFpu(reg, 0xDD, 0xD8); }
void fucom(const Fpu& reg) { opFpu(reg, 0xDD, 0xE0); }
void fucomp(const Fpu& reg) { opFpu(reg, 0xDD, 0xE8); }
void fxch(const Fpu& reg) { opFpu(reg, 0xD9, 0xC8); }
void vaddpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x58, true); }
void vaddps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x58, true); }
void vaddsd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0x58, false); }
void vaddss(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F3, 0x58, false); }
void vsubpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x5C, true); }
void vsubps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x5C, true); }
void vsubsd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0x5C, false); }
void vsubss(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F3, 0x5C, false); }
void vmulpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x59, true); }
void vmulps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x59, true); }
void vmulsd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0x59, false); }
void vmulss(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F3, 0x59, false); }
void vdivpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x5E, true); }
void vdivps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x5E, true); }
void vdivsd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0x5E, false); }
void vdivss(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F3, 0x5E, false); }
void vmaxpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x5F, true); }
void vmaxps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x5F, true); }
void vmaxsd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0x5F, false); }
void vmaxss(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F3, 0x5F, false); }
void vminpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x5D, true); }
void vminps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x5D, true); }
void vminsd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0x5D, false); }
void vminss(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F3, 0x5D, false); }
void vandpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x54, true); }
void vandps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x54, true); }
void vandnpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x55, true); }
void vandnps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x55, true); }
void vorpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x56, true); }
void vorps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x56, true); }
void vxorpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x57, true); }
void vxorps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F, 0x57, true); }
void vblendpd(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x0D, true, 0); db(imm); }
void vblendpd(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x0D, true, 0); db(imm); }
void vblendps(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x0C, true, 0); db(imm); }
void vblendps(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x0C, true, 0); db(imm); }
void vdppd(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x41, false, 0); db(imm); }
void vdppd(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x41, false, 0); db(imm); }
void vdpps(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x40, true, 0); db(imm); }
void vdpps(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x40, true, 0); db(imm); }
void vmpsadbw(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x42, false, 0); db(imm); }
void vmpsadbw(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x42, false, 0); db(imm); }
void vpblendw(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x0E, false, 0); db(imm); }
void vpblendw(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x0E, false, 0); db(imm); }
void vroundsd(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x0B, false, 0); db(imm); }
void vroundsd(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x0B, false, 0); db(imm); }
void vroundss(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x0A, false, 0); db(imm); }
void vroundss(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x0A, false, 0); db(imm); }
void vpclmulqdq(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x44, false, 0); db(imm); }
void vpclmulqdq(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x44, false, 0); db(imm); }
void vpermilps(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x0C, true, 0); }
void vpermilpd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x0D, true, 0); }
void vcmppd(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xC2, true, -1); db(imm); }
void vcmppd(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xC2, true, -1); db(imm); }
void vcmpps(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F, 0xC2, true, -1); db(imm); }
void vcmpps(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F, 0xC2, true, -1); db(imm); }
void vcmpsd(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_F2, 0xC2, false, -1); db(imm); }
void vcmpsd(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_F2, 0xC2, false, -1); db(imm); }
void vcmpss(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_F3, 0xC2, false, -1); db(imm); }
void vcmpss(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_F3, 0xC2, false, -1); db(imm); }
void vcvtsd2ss(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_F2, 0x5A, false, -1); }
void vcvtsd2ss(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_F2, 0x5A, false, -1); }
void vcvtss2sd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_F3, 0x5A, false, -1); }
void vcvtss2sd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_F3, 0x5A, false, -1); }
void vinsertps(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x21, false, 0); db(imm); }
void vinsertps(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x21, false, 0); db(imm); }
void vpacksswb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x63, false, -1); }
void vpacksswb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x63, false, -1); }
void vpackssdw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x6B, false, -1); }
void vpackssdw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x6B, false, -1); }
void vpackuswb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x67, false, -1); }
void vpackuswb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x67, false, -1); }
void vpackusdw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x2B, false, -1); }
void vpackusdw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x2B, false, -1); }
void vpaddb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xFC, false, -1); }
void vpaddb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xFC, false, -1); }
void vpaddw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xFD, false, -1); }
void vpaddw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xFD, false, -1); }
void vpaddd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xFE, false, -1); }
void vpaddd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xFE, false, -1); }
void vpaddq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xD4, false, -1); }
void vpaddq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xD4, false, -1); }
void vpaddsb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xEC, false, -1); }
void vpaddsb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xEC, false, -1); }
void vpaddsw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xED, false, -1); }
void vpaddsw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xED, false, -1); }
void vpaddusb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xDC, false, -1); }
void vpaddusb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xDC, false, -1); }
void vpaddusw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xDD, false, -1); }
void vpaddusw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xDD, false, -1); }
void vpalignr(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F3A | PP_66, 0x0F, false, -1); db(imm); }
void vpalignr(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F3A | PP_66, 0x0F, false, -1); db(imm); }
void vpand(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xDB, false, -1); }
void vpand(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xDB, false, -1); }
void vpandn(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xDF, false, -1); }
void vpandn(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xDF, false, -1); }
void vpavgb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xE0, false, -1); }
void vpavgb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xE0, false, -1); }
void vpavgw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xE3, false, -1); }
void vpavgw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xE3, false, -1); }
void vpcmpeqb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x74, false, -1); }
void vpcmpeqb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x74, false, -1); }
void vpcmpeqw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x75, false, -1); }
void vpcmpeqw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x75, false, -1); }
void vpcmpeqd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x76, false, -1); }
void vpcmpeqd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x76, false, -1); }
void vpcmpeqq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x29, false, -1); }
void vpcmpeqq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x29, false, -1); }
void vpcmpgtb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x64, false, -1); }
void vpcmpgtb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x64, false, -1); }
void vpcmpgtw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x65, false, -1); }
void vpcmpgtw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x65, false, -1); }
void vpcmpgtd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x66, false, -1); }
void vpcmpgtd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x66, false, -1); }
void vpcmpgtq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x37, false, -1); }
void vpcmpgtq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x37, false, -1); }
void vphaddw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x01, false, -1); }
void vphaddw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x01, false, -1); }
void vphaddd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x02, false, -1); }
void vphaddd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x02, false, -1); }
void vphaddsw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x03, false, -1); }
void vphaddsw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x03, false, -1); }
void vphsubw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x05, false, -1); }
void vphsubw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x05, false, -1); }
void vphsubd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x06, false, -1); }
void vphsubd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x06, false, -1); }
void vphsubsw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x07, false, -1); }
void vphsubsw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x07, false, -1); }
void vpmaddwd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xF5, false, -1); }
void vpmaddwd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xF5, false, -1); }
void vpmaddubsw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x04, false, -1); }
void vpmaddubsw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x04, false, -1); }
void vpmaxsb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x3C, false, -1); }
void vpmaxsb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x3C, false, -1); }
void vpmaxsw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xEE, false, -1); }
void vpmaxsw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xEE, false, -1); }
void vpmaxsd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x3D, false, -1); }
void vpmaxsd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x3D, false, -1); }
void vpmaxub(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xDE, false, -1); }
void vpmaxub(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xDE, false, -1); }
void vpmaxuw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x3E, false, -1); }
void vpmaxuw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x3E, false, -1); }
void vpmaxud(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x3F, false, -1); }
void vpmaxud(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x3F, false, -1); }
void vpminsb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x38, false, -1); }
void vpminsb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x38, false, -1); }
void vpminsw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xEA, false, -1); }
void vpminsw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xEA, false, -1); }
void vpminsd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x39, false, -1); }
void vpminsd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x39, false, -1); }
void vpminub(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xDA, false, -1); }
void vpminub(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xDA, false, -1); }
void vpminuw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x3A, false, -1); }
void vpminuw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x3A, false, -1); }
void vpminud(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x3B, false, -1); }
void vpminud(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x3B, false, -1); }
void vpmulhuw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xE4, false, -1); }
void vpmulhuw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xE4, false, -1); }
void vpmulhrsw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x0B, false, -1); }
void vpmulhrsw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x0B, false, -1); }
void vpmulhw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xE5, false, -1); }
void vpmulhw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xE5, false, -1); }
void vpmullw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xD5, false, -1); }
void vpmullw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xD5, false, -1); }
void vpmulld(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x40, false, -1); }
void vpmulld(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x40, false, -1); }
void vpmuludq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xF4, false, -1); }
void vpmuludq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xF4, false, -1); }
void vpmuldq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x28, false, -1); }
void vpmuldq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x28, false, -1); }
void vpor(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xEB, false, -1); }
void vpor(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xEB, false, -1); }
void vpsadbw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xF6, false, -1); }
void vpsadbw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xF6, false, -1); }
void vpshufb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x00, false, -1); }
void vpsignb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x08, false, -1); }
void vpsignb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x08, false, -1); }
void vpsignw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x09, false, -1); }
void vpsignw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x09, false, -1); }
void vpsignd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F38 | PP_66, 0x0A, false, -1); }
void vpsignd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F38 | PP_66, 0x0A, false, -1); }
void vpsllw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xF1, false, -1); }
void vpsllw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xF1, false, -1); }
void vpslld(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xF2, false, -1); }
void vpslld(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xF2, false, -1); }
void vpsllq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xF3, false, -1); }
void vpsllq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xF3, false, -1); }
void vpsraw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xE1, false, -1); }
void vpsraw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xE1, false, -1); }
void vpsrad(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xE2, false, -1); }
void vpsrad(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xE2, false, -1); }
void vpsrlw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xD1, false, -1); }
void vpsrlw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xD1, false, -1); }
void vpsrld(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xD2, false, -1); }
void vpsrld(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xD2, false, -1); }
void vpsrlq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xD3, false, -1); }
void vpsrlq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xD3, false, -1); }
void vpsubb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xF8, false, -1); }
void vpsubb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xF8, false, -1); }
void vpsubw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xF9, false, -1); }
void vpsubw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xF9, false, -1); }
void vpsubd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xFA, false, -1); }
void vpsubd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xFA, false, -1); }
void vpsubq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xFB, false, -1); }
void vpsubq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xFB, false, -1); }
void vpsubsb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xE8, false, -1); }
void vpsubsb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xE8, false, -1); }
void vpsubsw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xE9, false, -1); }
void vpsubsw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xE9, false, -1); }
void vpsubusb(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xD8, false, -1); }
void vpsubusb(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xD8, false, -1); }
void vpsubusw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xD9, false, -1); }
void vpsubusw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xD9, false, -1); }
void vpunpckhbw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x68, false, -1); }
void vpunpckhbw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x68, false, -1); }
void vpunpckhwd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x69, false, -1); }
void vpunpckhwd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x69, false, -1); }
void vpunpckhdq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x6A, false, -1); }
void vpunpckhdq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x6A, false, -1); }
void vpunpckhqdq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x6D, false, -1); }
void vpunpckhqdq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x6D, false, -1); }
void vpunpcklbw(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x60, false, -1); }
void vpunpcklbw(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x60, false, -1); }
void vpunpcklwd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x61, false, -1); }
void vpunpcklwd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x61, false, -1); }
void vpunpckldq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x62, false, -1); }
void vpunpckldq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x62, false, -1); }
void vpunpcklqdq(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x6C, false, -1); }
void vpunpcklqdq(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x6C, false, -1); }
void vpxor(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xEF, false, -1); }
void vpxor(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xEF, false, -1); }
void vrcpss(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_F3, 0x53, false, -1); }
void vrcpss(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_F3, 0x53, false, -1); }
void vrsqrtss(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_F3, 0x52, false, -1); }
void vrsqrtss(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_F3, 0x52, false, -1); }
void vshufpd(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0xC6, true, -1); db(imm); }
void vshufpd(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0xC6, true, -1); db(imm); }
void vshufps(const Xmm& xm1, const Xmm& xm2, const Operand& op, uint8 imm) { opAVX_X_X_XM(xm1, xm2, op, MM_0F, 0xC6, true, -1); db(imm); }
void vshufps(const Xmm& xmm, const Operand& op, uint8 imm) { opAVX_X_X_XM(xmm, xmm, op, MM_0F, 0xC6, true, -1); db(imm); }
void vsqrtsd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_F2, 0x51, false, -1); }
void vsqrtsd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_F2, 0x51, false, -1); }
void vsqrtss(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_F3, 0x51, false, -1); }
void vsqrtss(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_F3, 0x51, false, -1); }
void vunpckhpd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x15, true, -1); }
void vunpckhpd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x15, true, -1); }
void vunpckhps(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F, 0x15, true, -1); }
void vunpckhps(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F, 0x15, true, -1); }
void vunpcklpd(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F | PP_66, 0x14, true, -1); }
void vunpcklpd(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F | PP_66, 0x14, true, -1); }
void vunpcklps(const Xmm& xm1, const Xmm& xm2, const Operand& op) { opAVX_X_X_XM(xm1, xm2, op, MM_0F, 0x14, true, -1); }
void vunpcklps(const Xmm& xmm, const Operand& op) { opAVX_X_X_XM(xmm, xmm, op, MM_0F, 0x14, true, -1); }
void vaeskeygenassist(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0xDF, false, 0, imm); }
void vroundpd(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0x09, true, 0, imm); }
void vroundps(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0x08, true, 0, imm); }
void vpermilpd(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0x05, true, 0, imm); }
void vpermilps(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0x04, true, 0, imm); }
void vpcmpestri(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0x61, false, 0, imm); }
void vpcmpestrm(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0x60, false, 0, imm); }
void vpcmpistri(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0x63, false, 0, imm); }
void vpcmpistrm(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F3A | PP_66, 0x62, false, 0, imm); }
void vtestps(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x0E, true, 0); }
void vtestpd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x0F, true, 0); }
void vcomisd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_66, 0x2F, false, -1); }
void vcomiss(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F, 0x2F, false, -1); }
void vcvtdq2ps(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F, 0x5B, true, -1); }
void vcvtps2dq(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_66, 0x5B, true, -1); }
void vcvttps2dq(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_F3, 0x5B, true, -1); }
void vmovapd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_66, 0x28, true, -1); }
void vmovaps(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F, 0x28, true, -1); }
void vmovddup(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_F2, 0x12, true, -1); }
void vmovdqa(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_66, 0x6F, true, -1); }
void vmovdqu(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_F3, 0x6F, true, -1); }
void vmovshdup(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_F3, 0x16, true, -1); }
void vmovsldup(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_F3, 0x12, true, -1); }
void vmovupd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_66, 0x10, true, -1); }
void vmovups(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F, 0x10, true, -1); }
void vpabsb(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x1C, false, -1); }
void vpabsw(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x1D, false, -1); }
void vpabsd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x1E, false, -1); }
void vphminposuw(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x41, false, -1); }
void vpmovsxbw(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x20, false, -1); }
void vpmovsxbd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x21, false, -1); }
void vpmovsxbq(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x22, false, -1); }
void vpmovsxwd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x23, false, -1); }
void vpmovsxwq(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x24, false, -1); }
void vpmovsxdq(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x25, false, -1); }
void vpmovzxbw(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x30, false, -1); }
void vpmovzxbd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x31, false, -1); }
void vpmovzxbq(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x32, false, -1); }
void vpmovzxwd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x33, false, -1); }
void vpmovzxwq(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x34, false, -1); }
void vpmovzxdq(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x35, false, -1); }
void vpshufd(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_66, 0x70, false, -1, imm); }
void vpshufhw(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_F3, 0x70, false, -1, imm); }
void vpshuflw(const Xmm& xm, const Operand& op, uint8 imm) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_F2, 0x70, false, -1, imm); }
void vptest(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F38 | PP_66, 0x17, false, -1); }
void vrcpps(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F, 0x53, true, -1); }
void vrsqrtps(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F, 0x52, true, -1); }
void vsqrtpd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_66, 0x51, true, -1); }
void vsqrtps(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F, 0x51, true, -1); }
void vucomisd(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F | PP_66, 0x2E, false, -1); }
void vucomiss(const Xmm& xm, const Operand& op) { opAVX_X_XM_IMM(xm, op, MM_0F, 0x2E, false, -1); }
void vmovapd(const Address& addr, const Xmm& xmm) { opAVX_X_XM_IMM(xmm, addr, MM_0F | PP_66, 0x29, true, -1); }
void vmovaps(const Address& addr, const Xmm& xmm) { opAVX_X_XM_IMM(xmm, addr, MM_0F, 0x29, true, -1); }
void vmovdqa(const Address& addr, const Xmm& xmm) { opAVX_X_XM_IMM(xmm, addr, MM_0F | PP_66, 0x7F, true, -1); }
void vmovdqu(const Address& addr, const Xmm& xmm) { opAVX_X_XM_IMM(xmm, addr, MM_0F | PP_F3, 0x7F, true, -1); }
void vmovupd(const Address& addr, const Xmm& xmm) { opAVX_X_XM_IMM(xmm, addr, MM_0F | PP_66, 0x11, true, -1); }
void vmovups(const Address& addr, const Xmm& xmm) { opAVX_X_XM_IMM(xmm, addr, MM_0F, 0x11, true, -1); }
void vaddsubpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0xD0, true, -1); }
void vaddsubps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0xD0, true, -1); }
void vhaddpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x7C, true, -1); }
void vhaddps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0x7C, true, -1); }
void vhsubpd(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_66, 0x7D, true, -1); }
void vhsubps(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F | PP_F2, 0x7D, true, -1); }
void vaesenc(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xDC, false, 0); }
void vaesenclast(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xDD, false, 0); }
void vaesdec(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xDE, false, 0); }
void vaesdeclast(const Xmm& xmm, const Operand& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xDF, false, 0); }
void vmaskmovps(const Xmm& xm1, const Xmm& xm2, const Address& addr) { opAVX_X_X_XM(xm1, xm2, addr, MM_0F38 | PP_66, 0x2C, true, 0); }
void vmaskmovps(const Address& addr, const Xmm& xm1, const Xmm& xm2) { opAVX_X_X_XM(xm2, xm1, addr, MM_0F38 | PP_66, 0x2E, true, 0); }
void vmaskmovpd(const Xmm& xm1, const Xmm& xm2, const Address& addr) { opAVX_X_X_XM(xm1, xm2, addr, MM_0F38 | PP_66, 0x2D, true, 0); }
void vmaskmovpd(const Address& addr, const Xmm& xm1, const Xmm& xm2) { opAVX_X_X_XM(xm2, xm1, addr, MM_0F38 | PP_66, 0x2F, true, 0); }
void vmovhpd(const Xmm& x, const Operand& op1, const Operand& op2 = Operand()) { if (!op2.isNone() && !op2.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, op1, op2, MM_0F | PP_66, 0x16, false); }
void vmovhpd(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_66, 0x17, false); }
void vmovhps(const Xmm& x, const Operand& op1, const Operand& op2 = Operand()) { if (!op2.isNone() && !op2.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, op1, op2, MM_0F, 0x16, false); }
void vmovhps(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, xm0, addr, MM_0F, 0x17, false); }
void vmovlpd(const Xmm& x, const Operand& op1, const Operand& op2 = Operand()) { if (!op2.isNone() && !op2.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, op1, op2, MM_0F | PP_66, 0x12, false); }
void vmovlpd(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_66, 0x13, false); }
void vmovlps(const Xmm& x, const Operand& op1, const Operand& op2 = Operand()) { if (!op2.isNone() && !op2.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, op1, op2, MM_0F, 0x12, false); }
void vmovlps(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, xm0, addr, MM_0F, 0x13, false); }
void vfmadd132pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x98, true, 1); }
void vfmadd213pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xA8, true, 1); }
void vfmadd231pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xB8, true, 1); }
void vfmadd132ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x98, true, 0); }
void vfmadd213ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xA8, true, 0); }
void vfmadd231ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xB8, true, 0); }
void vfmadd132sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x99, false, 1); }
void vfmadd213sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xA9, false, 1); }
void vfmadd231sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xB9, false, 1); }
void vfmadd132ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x99, false, 0); }
void vfmadd213ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xA9, false, 0); }
void vfmadd231ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xB9, false, 0); }
void vfmaddsub132pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x96, true, 1); }
void vfmaddsub213pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xA6, true, 1); }
void vfmaddsub231pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xB6, true, 1); }
void vfmaddsub132ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x96, true, 0); }
void vfmaddsub213ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xA6, true, 0); }
void vfmaddsub231ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xB6, true, 0); }
void vfmsubadd132pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x97, true, 1); }
void vfmsubadd213pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xA7, true, 1); }
void vfmsubadd231pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xB7, true, 1); }
void vfmsubadd132ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x97, true, 0); }
void vfmsubadd213ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xA7, true, 0); }
void vfmsubadd231ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xB7, true, 0); }
void vfmsub132pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9A, true, 1); }
void vfmsub213pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAA, true, 1); }
void vfmsub231pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBA, true, 1); }
void vfmsub132ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9A, true, 0); }
void vfmsub213ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAA, true, 0); }
void vfmsub231ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBA, true, 0); }
void vfmsub132sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9B, false, 1); }
void vfmsub213sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAB, false, 1); }
void vfmsub231sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBB, false, 1); }
void vfmsub132ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9B, false, 0); }
void vfmsub213ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAB, false, 0); }
void vfmsub231ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBB, false, 0); }
void vfnmadd132pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9C, true, 1); }
void vfnmadd213pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAC, true, 1); }
void vfnmadd231pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBC, true, 1); }
void vfnmadd132ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9C, true, 0); }
void vfnmadd213ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAC, true, 0); }
void vfnmadd231ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBC, true, 0); }
void vfnmadd132sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9D, false, 1); }
void vfnmadd213sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAD, false, 1); }
void vfnmadd231sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBD, false, 1); }
void vfnmadd132ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9D, false, 0); }
void vfnmadd213ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAD, false, 0); }
void vfnmadd231ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBD, false, 0); }
void vfnmsub132pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9E, true, 1); }
void vfnmsub213pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAE, true, 1); }
void vfnmsub231pd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBE, true, 1); }
void vfnmsub132ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9E, true, 0); }
void vfnmsub213ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAE, true, 0); }
void vfnmsub231ps(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBE, true, 0); }
void vfnmsub132sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9F, false, 1); }
void vfnmsub213sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAF, false, 1); }
void vfnmsub231sd(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBF, false, 1); }
void vfnmsub132ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0x9F, false, 0); }
void vfnmsub213ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xAF, false, 0); }
void vfnmsub231ss(const Xmm& xmm, const Xmm& op1, const Operand& op2 = Operand()) { opAVX_X_X_XM(xmm, op1, op2, MM_0F38 | PP_66, 0xBF, false, 0); }
void vaesimc(const Xmm& x, const Operand& op) { opAVX_X_XM_IMM(x, op, MM_0F38 | PP_66, 0xDB, false, 0); }
void vbroadcastf128(const Ymm& y, const Address& addr) { opAVX_X_XM_IMM(y, addr, MM_0F38 | PP_66, 0x1A, true, 0); }
void vbroadcastsd(const Ymm& y, const Address& addr) { opAVX_X_XM_IMM(y, addr, MM_0F38 | PP_66, 0x19, true, 0); }
void vbroadcastss(const Xmm& x, const Address& addr) { opAVX_X_XM_IMM(x, addr, MM_0F38 | PP_66, 0x18, true, 0); }
void vextractf128(const Operand& op, const Ymm& y, uint8 imm) { opAVX_X_XM_IMM(y, cvtReg(op, op.isXMM(), Operand::YMM), MM_0F3A | PP_66, 0x19, true, 0, imm); }
void vinsertf128(const Ymm& y1, const Ymm& y2, const Operand& op, uint8 imm) { opAVX_X_X_XM(y1, y2, cvtReg(op, op.isXMM(), Operand::YMM), MM_0F3A | PP_66, 0x18, true, 0); db(imm); }
void vperm2f128(const Ymm& y1, const Ymm& y2, const Operand& op, uint8 imm) { opAVX_X_X_XM(y1, y2, op, MM_0F3A | PP_66, 0x06, true, 0); db(imm); }
void vlddqu(const Xmm& x, const Address& addr) { opAVX_X_X_XM(x, x.isXMM() ? xm0 : ym0, addr, MM_0F | PP_F2, 0xF0, true, 0); }
void vldmxcsr(const Address& addr) { opAVX_X_X_XM(xm2, xm0, addr, MM_0F, 0xAE, false, -1); }
void vstmxcsr(const Address& addr) { opAVX_X_X_XM(xm3, xm0, addr, MM_0F, 0xAE, false, -1); }
void vmaskmovdqu(const Xmm& x1, const Xmm& x2) { opAVX_X_X_XM(x1, xm0, x2, MM_0F | PP_66, 0xF7, false, -1); }
void vpextrb(const Operand& op, const Xmm& x, uint8 imm) { if (!op.isREG(i32e) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, xm0, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x14, false); db(imm); }
void vpextrw(const Reg& r, const Xmm& x, uint8 imm) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, x, MM_0F | PP_66, 0xC5, false, r.isBit(64) ? 1 : 0); db(imm); }
void vpextrw(const Address& addr, const Xmm& x, uint8 imm) { opAVX_X_X_XM(x, xm0, addr, MM_0F3A | PP_66, 0x15, false); db(imm); }
void vpextrd(const Operand& op, const Xmm& x, uint8 imm) { if (!op.isREG(32) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, xm0, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x16, false, 0); db(imm); }
void vpinsrb(const Xmm& x1, const Xmm& x2, const Operand& op, uint8 imm) { if (!op.isREG(32) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x1, x2, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x20, false); db(imm); }
void vpinsrb(const Xmm& x, const Operand& op, uint8 imm) { if (!op.isREG(32) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, x, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x20, false); db(imm); }
void vpinsrw(const Xmm& x1, const Xmm& x2, const Operand& op, uint8 imm) { if (!op.isREG(32) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x1, x2, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F | PP_66, 0xC4, false); db(imm); }
void vpinsrw(const Xmm& x, const Operand& op, uint8 imm) { if (!op.isREG(32) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, x, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F | PP_66, 0xC4, false); db(imm); }
void vpinsrd(const Xmm& x1, const Xmm& x2, const Operand& op, uint8 imm) { if (!op.isREG(32) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x1, x2, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x22, false, 0); db(imm); }
void vpinsrd(const Xmm& x, const Operand& op, uint8 imm) { if (!op.isREG(32) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, x, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x22, false, 0); db(imm); }
void vpmovmskb(const Reg32e& r, const Xmm& x) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, x, MM_0F | PP_66, 0xD7, false); }
void vpslldq(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm7, x1, x2, MM_0F | PP_66, 0x73, false); db(imm); }
void vpslldq(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm7, x, x, MM_0F | PP_66, 0x73, false); db(imm); }
void vpsrldq(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm3, x1, x2, MM_0F | PP_66, 0x73, false); db(imm); }
void vpsrldq(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm3, x, x, MM_0F | PP_66, 0x73, false); db(imm); }
void vpsllw(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm6, x1, x2, MM_0F | PP_66, 0x71, false); db(imm); }
void vpsllw(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm6, x, x, MM_0F | PP_66, 0x71, false); db(imm); }
void vpslld(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm6, x1, x2, MM_0F | PP_66, 0x72, false); db(imm); }
void vpslld(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm6, x, x, MM_0F | PP_66, 0x72, false); db(imm); }
void vpsllq(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm6, x1, x2, MM_0F | PP_66, 0x73, false); db(imm); }
void vpsllq(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm6, x, x, MM_0F | PP_66, 0x73, false); db(imm); }
void vpsraw(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm4, x1, x2, MM_0F | PP_66, 0x71, false); db(imm); }
void vpsraw(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm4, x, x, MM_0F | PP_66, 0x71, false); db(imm); }
void vpsrad(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm4, x1, x2, MM_0F | PP_66, 0x72, false); db(imm); }
void vpsrad(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm4, x, x, MM_0F | PP_66, 0x72, false); db(imm); }
void vpsrlw(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm2, x1, x2, MM_0F | PP_66, 0x71, false); db(imm); }
void vpsrlw(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm2, x, x, MM_0F | PP_66, 0x71, false); db(imm); }
void vpsrld(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm2, x1, x2, MM_0F | PP_66, 0x72, false); db(imm); }
void vpsrld(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm2, x, x, MM_0F | PP_66, 0x72, false); db(imm); }
void vpsrlq(const Xmm& x1, const Xmm& x2, uint8 imm) { opAVX_X_X_XM(xm2, x1, x2, MM_0F | PP_66, 0x73, false); db(imm); }
void vpsrlq(const Xmm& x, uint8 imm) { opAVX_X_X_XM(xm2, x, x, MM_0F | PP_66, 0x73, false); db(imm); }
void vblendvpd(const Xmm& x1, const Xmm& x2, const Operand& op, const Xmm& x4) { opAVX_X_X_XM(x1, x2, op, MM_0F3A | PP_66, 0x4B, true); db(x4.getIdx() << 4); }
void vblendvpd(const Xmm& x1, const Operand& op, const Xmm& x4) { opAVX_X_X_XM(x1, x1, op, MM_0F3A | PP_66, 0x4B, true); db(x4.getIdx() << 4); }
void vblendvps(const Xmm& x1, const Xmm& x2, const Operand& op, const Xmm& x4) { opAVX_X_X_XM(x1, x2, op, MM_0F3A | PP_66, 0x4A, true); db(x4.getIdx() << 4); }
void vblendvps(const Xmm& x1, const Operand& op, const Xmm& x4) { opAVX_X_X_XM(x1, x1, op, MM_0F3A | PP_66, 0x4A, true); db(x4.getIdx() << 4); }
void vpblendvb(const Xmm& x1, const Xmm& x2, const Operand& op, const Xmm& x4) { opAVX_X_X_XM(x1, x2, op, MM_0F3A | PP_66, 0x4C, false); db(x4.getIdx() << 4); }
void vpblendvb(const Xmm& x1, const Operand& op, const Xmm& x4) { opAVX_X_X_XM(x1, x1, op, MM_0F3A | PP_66, 0x4C, false); db(x4.getIdx() << 4); }
void vmovd(const Xmm& x, const Reg32& reg) { opAVX_X_X_XM(x, xm0, Xmm(reg.getIdx()), MM_0F | PP_66, 0x6E, false, 0); }
void vmovd(const Xmm& x, const Address& addr) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_66, 0x6E, false, 0); }
void vmovd(const Reg32& reg, const Xmm& x) { opAVX_X_X_XM(x, xm0, Xmm(reg.getIdx()), MM_0F | PP_66, 0x7E, false, 0); }
void vmovd(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_66, 0x7E, false, 0); }
void vmovhlps(const Xmm& x1, const Xmm& x2, const Operand& op = Operand()) { if (!op.isNone() && !op.isXMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x1, x2, op, MM_0F, 0x12, false); }
void vmovlhps(const Xmm& x1, const Xmm& x2, const Operand& op = Operand()) { if (!op.isNone() && !op.isXMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x1, x2, op, MM_0F, 0x16, false); }
void vmovmskpd(const Reg& r, const Xmm& x) { if (!r.isBit(i32e)) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x.isXMM() ? Xmm(r.getIdx()) : Ymm(r.getIdx()), x.isXMM() ? xm0 : ym0, x, MM_0F | PP_66, 0x50, true, 0); }
void vmovmskps(const Reg& r, const Xmm& x) { if (!r.isBit(i32e)) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x.isXMM() ? Xmm(r.getIdx()) : Ymm(r.getIdx()), x.isXMM() ? xm0 : ym0, x, MM_0F, 0x50, true, 0); }
void vmovntdq(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, x.isXMM() ? xm0 : ym0, addr, MM_0F | PP_66, 0xE7, true); }
void vmovntpd(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, x.isXMM() ? xm0 : ym0, addr, MM_0F | PP_66, 0x2B, true); }
void vmovntps(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, x.isXMM() ? xm0 : ym0, addr, MM_0F, 0x2B, true); }
void vmovntdqa(const Xmm& x, const Address& addr) { opAVX_X_X_XM(x, xm0, addr, MM_0F38 | PP_66, 0x2A, false); }
void vmovsd(const Xmm& x1, const Xmm& x2, const Operand& op = Operand()) { if (!op.isNone() && !op.isXMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x1, x2, op, MM_0F | PP_F2, 0x10, false); }
void vmovsd(const Xmm& x, const Address& addr) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_F2, 0x10, false); }
void vmovsd(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_F2, 0x11, false); }
void vmovss(const Xmm& x1, const Xmm& x2, const Operand& op = Operand()) { if (!op.isNone() && !op.isXMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x1, x2, op, MM_0F | PP_F3, 0x10, false); }
void vmovss(const Xmm& x, const Address& addr) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_F3, 0x10, false); }
void vmovss(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_F3, 0x11, false); }
void vcvtss2si(const Reg32& r, const Operand& op) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, op, MM_0F | PP_F3, 0x2D, false, 0); }
void vcvttss2si(const Reg32& r, const Operand& op) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, op, MM_0F | PP_F3, 0x2C, false, 0); }
void vcvtsd2si(const Reg32& r, const Operand& op) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, op, MM_0F | PP_F2, 0x2D, false, 0); }
void vcvttsd2si(const Reg32& r, const Operand& op) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, op, MM_0F | PP_F2, 0x2C, false, 0); }
void vcvtsi2ss(const Xmm& x, const Operand& op1, const Operand& op2 = Operand()) { if (!op2.isNone() && !(op2.isREG(i32e) || op2.isMEM())) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, op1, cvtReg(op2, op2.isREG(), Operand::XMM), MM_0F | PP_F3, 0x2A, false, (op1.isMEM() || op2.isMEM()) ? -1 : (op1.isREG(32) || op2.isREG(32)) ? 0 : 1); }
void vcvtsi2sd(const Xmm& x, const Operand& op1, const Operand& op2 = Operand()) { if (!op2.isNone() && !(op2.isREG(i32e) || op2.isMEM())) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, op1, cvtReg(op2, op2.isREG(), Operand::XMM), MM_0F | PP_F2, 0x2A, false, (op1.isMEM() || op2.isMEM()) ? -1 : (op1.isREG(32) || op2.isREG(32)) ? 0 : 1); }
void vcvtps2pd(const Xmm& x, const Operand& op) { if (!op.isMEM() && !op.isXMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, x.isXMM() ? xm0 : ym0, cvtReg(op, !op.isMEM(), x.isXMM() ? Operand::XMM : Operand::YMM), MM_0F, 0x5A, true); }
void vcvtdq2pd(const Xmm& x, const Operand& op) { if (!op.isMEM() && !op.isXMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, x.isXMM() ? xm0 : ym0, cvtReg(op, !op.isMEM(), x.isXMM() ? Operand::XMM : Operand::YMM), MM_0F | PP_F3, 0xE6, true); }
void vcvtpd2ps(const Xmm& x, const Operand& op) { if (x.isYMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(op.isYMM() ? Ymm(x.getIdx()) : x, op.isYMM() ? ym0 : xm0, op, MM_0F | PP_66, 0x5A, true); }
void vcvtpd2dq(const Xmm& x, const Operand& op) { if (x.isYMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(op.isYMM() ? Ymm(x.getIdx()) : x, op.isYMM() ? ym0 : xm0, op, MM_0F | PP_F2, 0xE6, true); }
void vcvttpd2dq(const Xmm& x, const Operand& op) { if (x.isYMM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(op.isYMM() ? Ymm(x.getIdx()) : x, op.isYMM() ? ym0 : xm0, op, MM_0F | PP_66, 0xE6, true); }
void vmovq(const Xmm& x, const Address& addr) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_F3, 0x7E, false, -1); }
void vmovq(const Address& addr, const Xmm& x) { opAVX_X_X_XM(x, xm0, addr, MM_0F | PP_66, 0xD6, false, -1); }
#ifdef XBYAK64
void vmovq(const Xmm& x, const Reg64& reg) { opAVX_X_X_XM(x, xm0, Xmm(reg.getIdx()), MM_0F | PP_66, 0x6E, false, 1); }
void vmovq(const Reg64& reg, const Xmm& x) { opAVX_X_X_XM(x, xm0, Xmm(reg.getIdx()), MM_0F | PP_66, 0x7E, false, 1); }
void vmovq(const Xmm& x1, const Xmm& x2) { opAVX_X_X_XM(x1, xm0, x2, MM_0F | PP_F3, 0x7E, false, -1); }
void vpextrq(const Operand& op, const Xmm& x, uint8 imm) { if (!op.isREG(64) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, xm0, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x16, false, 1); db(imm); }
void vpinsrq(const Xmm& x1, const Xmm& x2, const Operand& op, uint8 imm) { if (!op.isREG(64) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x1, x2, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x22, false, 1); db(imm); }
void vpinsrq(const Xmm& x, const Operand& op, uint8 imm) { if (!op.isREG(64) && !op.isMEM()) throw ERR_BAD_COMBINATION; opAVX_X_X_XM(x, x, cvtReg(op, !op.isMEM(), Operand::XMM), MM_0F3A | PP_66, 0x22, false, 1); db(imm); }
void vcvtss2si(const Reg64& r, const Operand& op) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, op, MM_0F | PP_F3, 0x2D, false, 1); }
void vcvttss2si(const Reg64& r, const Operand& op) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, op, MM_0F | PP_F3, 0x2C, false, 1); }
void vcvtsd2si(const Reg64& r, const Operand& op) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, op, MM_0F | PP_F2, 0x2D, false, 1); }
void vcvttsd2si(const Reg64& r, const Operand& op) { opAVX_X_X_XM(Xmm(r.getIdx()), xm0, op, MM_0F | PP_F2, 0x2C, false, 1); }
void movsxd(const Reg64& reg, const Operand& op) { opMovsxd(reg, op); }
#endif
