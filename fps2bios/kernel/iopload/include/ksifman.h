#ifndef __SIFMAN_H__
#define __SIFMAN_H__

#include <tamtypes.h>

#define SIFMAN_VER	0x101

#define SIF_FROM_IOP	0x0
#define SIF_TO_IOP		0x1
#define SIF_FROM_EE		0x0
#define SIF_TO_EE		0x1

#define SIF_DMA_INT_I	0x2
#define SIF_DMA_INT_O	0x4
#define SIF_DMA_SPR		0x8
#define SIF_DMA_BSN		0x10
#define SIF_DMA_TAG		0x20

struct sifman_DMA {	//t_sif_dma_transfer
	void *data;
	void *addr;
	int	 size;
	int	 attr;
};

void SifDeinit();				//2

void SifSIF2Init();				//4
void SifInit();					//5 (21,25,26)
void SifSetDChain();			//6
u32  SifSetDma(struct sifman_DMA* psd, int len);	//7 (26)
int  SifDmaStat(u32 id);				//8 (26)
void SifSend(struct sifman_DMA sd);	//9
void SifSendSync();			//10
int  SifIsSending();		//11
void SifSetSIF0DMA(void *data, int size, int attr);	//12
void SifSendSync0();		//13
int  SifIsSending0();		//14
void SifSetSIF1DMA(void *data, int size, int attr);	//15
void SifSendSync1();		//16
int  SifIsSending1();		//17
void SifSetSIF2DMA(void *data, int size, int attr);	//18
void SifSendSync2();		//19
int  SifIsSending2();		//20
int  SifGetEEIOPflags();	//21
int  SifSetEEIOPflags(int val);		//22(21)
int  SifGetIOPEEflags();			//23
int  SifSetIOPEEflags(int val);		//24(28)
int  SifGetEErcvaddr();			//25
int  SifGetIOPrcvaddr();			//26
int  SifSetIOPrcvaddr(int val);		//27
void SifSet1450_2();				//28
int  SifCheckInit();				//29(21,26)
void SifSet0CB(int (*_function)(int), int _param);//30
void SifReset0CB();				//31

#endif /* __SIFMAN_H__ */
