struct IPUStatus {
	bool InProgress;
	u8 DMAMode;
	bool DMAFinished;
	bool IRQTriggered;
	u8 TagFollow;
	u32 TagAddr;
	bool stalled;
	u8 ChainMode;
	u32 NextMem;
};

#define DMA_MODE_NORMAL 0
#define DMA_MODE_CHAIN 1

#define IPU1_TAG_FOLLOW 0
#define IPU1_TAG_QWC 1
#define IPU1_TAG_ADDR 2
#define IPU1_TAG_NONE 3

union tIPU_DMA
{
	struct
	{
		bool GIFSTALL  : 1;
		bool TIE0 :1;
		bool TIE1 : 1;
		bool ACTV1 : 1;
		bool DOTIE1  : 1;
		bool FIREINT0 : 1;
		bool FIREINT1 : 1;
		bool VIFSTALL : 1;
		bool SIFSTALL : 1;
	};
	u32 _u32;

	tIPU_DMA( u32 val ){ _u32 = val; }
	tIPU_DMA() { }

	bool test(u32 flags) const { return !!(_u32 & flags); }
	void set_flags(u32 flags) { _u32 |= flags; }
	void clear_flags(u32 flags) { _u32 &= ~flags; }
	void reset() { _u32 = 0; }
	wxString desc() const
	{
		wxString temp(L"g_nDMATransfer[");

		if (GIFSTALL) temp += L" GIFSTALL ";
		if (TIE0) temp += L" TIE0 ";
		if (TIE1) temp += L" TIE1 ";
		if (ACTV1) temp += L" ACTV1 ";
		if (DOTIE1) temp += L" DOTIE1 ";
		if (FIREINT0) temp += L" FIREINT0 ";
		if (FIREINT1) temp += L" FIREINT1 ";
		if (VIFSTALL) temp += L" VIFSTALL ";
		if (SIFSTALL) temp += L" SIFSTALL ";

		temp += L"]";
		return temp;
	}
};

extern void ipu0Interrupt();
extern void ipu1Interrupt();

extern void dmaIPU0();
extern void dmaIPU1();
extern int IPU0dma();
extern int IPU1dma();

extern void ipuDmaReset();
