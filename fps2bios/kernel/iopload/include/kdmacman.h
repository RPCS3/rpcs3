#ifndef __DMACMAN_H__
#define __DMACMAN_H__

#define DMACMAN_VER	0x101

//////////////////////////////  D_CHCR - DMA Channel Control Register
#define DMAf_30		0x40000000	// unknown; set on 'to' direction
#define DMAf_TR		0x01000000	// DMA transfer
#define DMAf_LI		0x00000400	// Linked list GPU; also SPU & SIF0
#define DMAf_CO		0x00000200	// Continuous stream
#define DMAf_08		0x00000100	// unknown
#define DMAf_DR		0x00000001	// Direction to=0/from=1
//  31           24 23           16 15            8 7             0
// ÖÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ·
// º ³?³ ³ ³ ³ ³ ³Tº ³ ³ ³ ³ ³ ³ ³ º ³ ³ ³ ³ ³L³C³?º ³ ³ ³ ³ ³ ³ ³Dº
// º ³?³ ³ ³ ³ ³ ³Rº ³ ³ ³ ³ ³ ³ ³ º ³ ³ ³ ³ ³I³O³?º ³ ³ ³ ³ ³ ³ ³Rº
// ÓÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄ½
//   30          24                          10 9 8               0

//////////////////////////////  DPCR - DMA Primary Control Register
// ÖÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ·
// º   67  ³ DMA 6 º DMA 5 ³ DMA 4 º DMA 3 ³ DMA 2 º DMA 1 ³ DMA 0 º 0xBF8010F0 DPCR
// ÓÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄ½
// ÖÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ·
// º       ³ DMA85 º DMA12 ³ DMA11 º DMA10 ³ DMA 9 º DMA 8 ³ DMA 7 º 0xBF801570 DPCR_
// ÓÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄ½
// ÖÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄÒÄÂÄÂÄÂÄÂÄÂÄÂÄÂÄ·
// º       ³       º       ³       º       ³ DMA15 º DMA14 ³ DMA13 º 0xBF8015F0 DPCR__
// ÓÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄÐÄÁÄÁÄÁÄÁÄÁÄÁÄÁÄ½

////////// DPCR
#define DMAch_MDECin	0
#define DMAch_MDECout	1
#define DMAch_GPU	2	// SIF2 both directions
#define DMAch_CD	3
#define DMAch_SPU	4
#define DMAch_PIO	5
#define DMAch_GPUotc	6

#define DMAch_67	67	// strange

////////// DPCR_
#define DMAch_SPU2	7
#define DMAch_8		8
#define DMAch_SIF0	9	// SIFout IOP->EE
#define DMAch_SIF1	10	// SIFin  EE->IOP
#define DMAch_SIO2in	11
#define DMAch_SIO2out	12

#define DMAch_85	85	// stange, very strange

////////// DPCR__
#define DMAch_13	13
#define DMAch_14	14
#define DMAch_15	15

int  dmacman_start(int argc, char* argv[]);			// 0
int  dmacman_deinit();						// 2

void dmacman_call4_SetD_MADR(unsigned int ch, int value);	// 4
int  dmacman_call5_GetD_MADR(unsigned int ch);			// 5
void dmacman_call6_SetD_BCR(unsigned int ch, int value);	// 6
int  dmacman_call7_GetD_BCR(unsigned int ch);			// 7
void dmacman_call8_SetD_CHCR(unsigned int ch, int value);	// 8
int  dmacman_call9_GetD_CHCR(unsigned int ch);			// 9
void dmacman_call10_SetD_TADR(unsigned int ch, int value);	//10
int  dmacman_call11_GetD_TADR(unsigned int ch);			//11
void dmacman_call12_Set_4_9_A(unsigned int ch, int value);	//12
int  dmacman_call13_Get_4_9_A(unsigned int ch);			//13
void dmacman_call14_SetDPCR(int value);				//14
int  dmacman_call15_GetDPCR();					//15
void dmacman_call16_SetDPCR2(int value);			//16
int  dmacman_call17_GetDPCR2();					//17
void dmacman_call18_SetDPCR3(int value);			//18
int  dmacman_call19_GetDPCR3();					//19
void dmacman_call20_SetDICR(int value);				//20
int  dmacman_call21_GetDICR();					//21
void dmacman_call22_SetDICR2(int value);			//22
int  dmacman_call23_GetDICR2();					//23
void dmacman_call24_setBF80157C(int value);			//24
int  dmacman_call25_getBF80157C();				//25
void dmacman_call26_setBF801578(int value);			//26
int  dmacman_call27_getBF801578();				//27
int  dmacman_call28_SetDMA(int ch, int address, int size, int count, int dir);	//28
int  dmacman_call29_SetDMA_chainedSPU_SIF0(int ch, int size, int c);//29
int  dmacman_call30_SetDMA_SIF0(int ch, int size, int c);	//30
int  dmacman_call31_SetDMA_SIF1(int ch, int size);		//31
void dmacman_call32_StartTransfer(int ch);			//32
void dmacman_call33_SetVal(int ch, int value);			//33
void dmacman_call34_EnableDMAch(int ch);			//34
void dmacman_call35_DisableDMAch(int ch);			//35

//SIF2 DMA ch 2 (GPU)
#define DMAch_SIF2_MADR		(*(volatile int*)0xBF8010A0)
#define DMAch_SIF2_BCR		(*(volatile int*)0xBF8010A4)
#define DMAch_SIF2_BCR_size	(*(volatile short*)0xBF8010A4)
#define DMAch_SIF2_BCR_count	(*(volatile short*)0xBF8010A6)
#define DMAch_SIF2_CHCR		(*(volatile int*)0xBF8010A8)
//SIF0 DMA ch 9
#define DMAch_SIF9_MADR		(*(volatile int*)0xBF801520)
#define DMAch_SIF9_BCR		(*(volatile int*)0xBF801524)
#define DMAch_SIF9_BCR_size	(*(volatile short*)0xBF801524)
#define DMAch_SIF9_BCR_count	(*(volatile short*)0xBF801526)
#define DMAch_SIF9_CHCR		(*(volatile int*)0xBF801528)
#define DMAch_SIF9_TADR		(*(volatile int*)0xBF80152C)
//SIF1 DMA ch 10 (0xA)
#define DMAch_SIFA_MADR		(*(volatile int*)0xBF801530)
#define DMAch_SIFA_BCR		(*(volatile int*)0xBF801534)
#define DMAch_SIFA_BCR_size	(*(volatile short*)0xBF801534)
#define DMAch_SIFA_BCR_count	(*(volatile short*)0xBF801536)
#define DMAch_SIFA_CHCR		(*(volatile int*)0xBF801538)

#define DMAch_DPCR		(*(volatile int*)0xBF8010F0)
#define DMAch_DPCR2		(*(volatile int*)0xBF801570)

#endif//__DMACMAN_H__
