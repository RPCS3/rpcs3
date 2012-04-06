
#pragma once
#include "System/SysThreads.h"
#include "Vif.h"
#include "Vif_Dma.h"
#include "VUmicro.h"

#define MTVU_LOG(...) do{} while(0)
//#define MTVU_LOG DevCon.WriteLn

// Notes:
// - This class should only be accessed from the EE thread...
// - buffer_size must be power of 2
// - ring-buffer has no complete pending packets when read_pos==write_pos
class VU_Thread : public pxThread {
	static const u32 buffer_size = (_1mb * 16) / sizeof(u32);
	static const u32 buffer_mask = buffer_size - 1;
	__aligned(4) u32 buffer[buffer_size];
	__aligned(4) volatile s32  read_pos; // Only modified by VU thread
	__aligned(4) volatile bool isBusy;   // Is thread processing data?
	__aligned(4) s32  write_pos;    // Only modified by EE thread
	__aligned(4) s32  write_offset; // Only modified by EE thread
	__aligned(4) Mutex     mtxBusy;
	__aligned(4) Semaphore semaEvent;
	__aligned(4) BaseVUmicroCPU*& vuCPU;
	__aligned(4) VURegs&          vuRegs;

public:
	__aligned16  vifStruct        vif;
	__aligned16  VIFregisters     vifRegs;
	__aligned(4) Semaphore semaXGkick;
	__aligned(4) u32 vuCycles[4]; // Used for VU cycle stealing hack
	__aligned(4) u32 vuCycleIdx;  // Used for VU cycle stealing hack

	VU_Thread(BaseVUmicroCPU*& _vuCPU, VURegs& _vuRegs);
	virtual ~VU_Thread() throw();

	void Reset();

	// Get MTVU to start processing its packets if it isn't already
	void KickStart(bool forceKick = false);

	// Used for assertions...
	bool IsDone();

	// Waits till MTVU is done processing
	void WaitVU();

	void ExecuteVU(u32 vu_addr, u32 vif_top, u32 vif_itop);

	void VifUnpack(vifStruct& _vif, VIFregisters& _vifRegs, u8* data, u32 size);

	// Writes to VU's Micro Memory (size in bytes)
	void WriteMicroMem(u32 vu_micro_addr, void* data, u32 size);

	// Writes to VU's Data Memory (size in bytes)
	void WriteDataMem(u32 vu_data_addr, void* data, u32 size);

	void WriteCol(vifStruct& _vif);

	void WriteRow(vifStruct& _vif);

protected:
	void ExecuteTaskInThread();

private:
	void ExecuteRingBuffer();

	void WaitOnSize(s32 size);
	void ReserveSpace(s32 size);

	volatile s32 GetReadPos();
	volatile s32 GetWritePos();
	u32* GetWritePtr();

	void incReadPos(s32 offset);
	void incWritePos();

	u32 Read();
	void Read(void* dest, u32 size);
	void ReadRegs(VIFregisters* dest);

	void Write(u32 val);
	void Write(void* src, u32 size);
	void WriteRegs(VIFregisters* src);

	u32 Get_vuCycles();
};

extern __aligned16 VU_Thread vu1Thread;