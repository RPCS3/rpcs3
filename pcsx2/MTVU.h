
#pragma once
#include "System/SysThreads.h"
#include "Vif.h"
#include "Vif_Dma.h"
#include "VUmicro.h"
#include "Gif_Unit.h"

extern void MTVU_Unpack(void* data, VIFregisters& vifRegs);
#define volatize(x) (*reinterpret_cast<volatile uint*>(&(x)))
#define size_u32(x) (((u32)x+3u)>>2) // Rounds up a size in bytes for size in u32's
#define MTVU_ALWAYS_KICK 0
#define MTVU_SYNC_MODE   0
#define MTVU_LOG(...) do{} while(0)
//#define MTVU_LOG DevCon.WriteLn

enum MTVU_EVENT {
	MTVU_VU_EXECUTE,     // Execute VU program
	MTVU_VU_WRITE_MICRO, // Write to VU micro-mem
	MTVU_VU_WRITE_DATA,  // Write to VU data-mem
	MTVU_VIF_WRITE_COL,  // Write to Vif col reg
	MTVU_VIF_WRITE_ROW,  // Write to Vif row reg
	MTVU_VIF_UNPACK,     // Execute Vif Unpack
	MTVU_NULL_PACKET,    // Go back to beginning of buffer
	MTVU_RESET
};

// Notes:
// - This class should only be accessed from the EE thread...
// - buffer_size must be power of 2
// - ring-buffer has no complete pending packets when read_pos==write_pos
struct VU_Thread : public pxThread {
	static const u32 buffer_size = (_1mb * 16) / sizeof(u32);
	static const u32 buffer_mask = buffer_size - 1;
	__aligned(4) u32 buffer[buffer_size];
	__aligned(4) volatile s32  read_pos; // Only modified by VU thread
	__aligned(4) volatile bool isBusy;   // Is thread processing data?
	__aligned(4) s32  write_pos;    // Only modified by EE thread
	__aligned(4) s32  write_offset; // Only modified by EE thread
	__aligned(4) Mutex     mtxBusy;
	__aligned(4) Semaphore semaEvent;
	__aligned(4) Semaphore semaXGkick;
	__aligned(4) BaseVUmicroCPU*& vuCPU;
	__aligned(4) VURegs&          vuRegs;
	__aligned16  vifStruct        vif;
	__aligned16  VIFregisters     vifRegs;
	__aligned(4) u32 vuCycles[4]; // Used for VU cycle stealing hack
	__aligned(4) u32 vuCycleIdx;  // Used for VU cycle stealing hack

	VU_Thread(BaseVUmicroCPU*& _vuCPU, VURegs& _vuRegs) : 
			vuCPU(_vuCPU), vuRegs(_vuRegs) {
		m_name = L"MTVU";
		Reset();
	}
	virtual ~VU_Thread() throw() {
		pxThread::Cancel();
	}
	void InitThread() {
		Start(); // Starts the pxThread
	}
	void Reset() {
		read_pos     = 0;
		write_pos    = 0;
		write_offset = 0;
		vuCycleIdx   = 0;
		isBusy = false;
		memzero(vif);
		memzero(vifRegs);
		memzero(vuCycles);
	}
protected:
	// Should only be called by ReserveSpace()
	__ri void WaitOnSize(s32 size) {
		for(;;) {
			s32 readPos  = GetReadPos();
			if (readPos <= write_pos) break; // MTVU is reading in back of write_pos
			if (readPos >  write_pos + size) break; // Enough free front space
			if (1) { // Let MTVU run to free up buffer space
				KickStart();
				if (IsDevBuild) DevCon.WriteLn("WaitOnSize()");
				ScopedLock lock(mtxBusy);
			}
		}
	}

	// Makes sure theres enough room in the ring buffer
	// to write a continuous 'size * sizeof(u32)' bytes
	void ReserveSpace(s32 size) {
		pxAssert(write_pos < buffer_size);
		pxAssert(size      < buffer_size);
		pxAssert(size > 0);
		pxAssert(write_offset == 0);
		if (write_pos + size > buffer_size) {
			pxAssert(write_pos > 0);
			WaitOnSize(1); // Size of MTVU_NULL_PACKET
			Write(MTVU_NULL_PACKET);
			write_offset = 0;
			AtomicExchange(volatize(write_pos), 0);
		}
		WaitOnSize(size);
	}

	// Use this when reading read_pos from ee thread
	__fi volatile s32 GetReadPos() {
		return AtomicRead(read_pos);
	}
	// Use this when reading write_pos from vu thread
	__fi volatile s32 GetWritePos() {
		return AtomicRead(volatize(write_pos));
	}
	// Gets the effective write pointer after adding write_offset
	__fi u32* GetWritePtr() {
		return &buffer[(write_pos + write_offset) & buffer_mask];
	}

	__fi void incReadPos(s32 offset) { // Offset in u32 sizes
		s32 temp = (read_pos + offset) & buffer_mask;
		AtomicExchange(read_pos, temp);
	}
	__fi void incWritePos() { // Adds write_offset
		s32 temp = (write_pos + write_offset) & buffer_mask;
		write_offset = 0;
		AtomicExchange(volatize(write_pos), temp);
		if (MTVU_ALWAYS_KICK) KickStart();
		if (MTVU_SYNC_MODE)   WaitVU();
	}

	__fi u32 Read() {
		u32 ret = buffer[read_pos];
		incReadPos(1);
		return ret;
	}
	__fi void Read(void* dest, u32 size) { // Size in bytes
		memcpy_fast(dest, &buffer[read_pos], size);
		incReadPos(size_u32(size));
	}
	__fi void ReadRegs(VIFregisters* dest) {
		VIFregistersMTVU* src = (VIFregistersMTVU*)&buffer[read_pos];
		dest->cycle = src->cycle;
		dest->mode = src->mode;
		dest->num = src->num;
		dest->mask = src->mask;
		dest->itop = src->itop;
		dest->top = src->top;
		incReadPos(size_u32(sizeof(VIFregistersMTVU)));
	}

	__fi void Write(u32 val) {
		GetWritePtr()[0] = val;
		write_offset += 1;
	}
	__fi void Write(void* src, u32 size) { // Size in bytes
		memcpy_fast(GetWritePtr(), src, size);
		write_offset += size_u32(size);
	}
	__fi void WriteRegs(VIFregisters* src) {
		VIFregistersMTVU* dest = (VIFregistersMTVU*)GetWritePtr();
		dest->cycle = src->cycle;
		dest->mode = src->mode;
		dest->num = src->num;
		dest->mask = src->mask;
		dest->top = src->top;
		dest->itop = src->itop;
		write_offset += size_u32(sizeof(VIFregistersMTVU));
	}

	void ExecuteTaskInThread() {
		PCSX2_PAGEFAULT_PROTECT {
			ExecuteRingBuffer();
		} PCSX2_PAGEFAULT_EXCEPT;
	}

	void ExecuteRingBuffer() {
		for(;;) {
			semaEvent.WaitWithoutYield();
			ScopedLockBool lock(mtxBusy, isBusy);
			while (read_pos != GetWritePos()) {
				u32 tag = Read();
				switch (tag) {
					case MTVU_VU_EXECUTE: {
						vuRegs.cycle = 0;
						s32 addr     = Read();
						vifRegs.top  = Read();
						vifRegs.itop = Read();
						if (addr != -1) vuRegs.VI[REG_TPC].UL = addr;
						vuCPU->Execute(vu1RunCycles);
						gifUnit.gifPath[GIF_PATH_1].FinishGSPacketMTVU();
						semaXGkick.Post(); // Tell MTGS a path1 packet is complete
						AtomicExchange(vuCycles[vuCycleIdx], vuRegs.cycle);
						vuCycleIdx  = (vuCycleIdx + 1) & 3;
						break;
					}
					case MTVU_VU_WRITE_MICRO: {
						u32 vu_micro_addr = Read();
						u32 size = Read();
						vuCPU->Clear(vu_micro_addr, size);
						Read(&vuRegs.Micro[vu_micro_addr], size);
						break;
					}
					case MTVU_VU_WRITE_DATA: {
						u32 vu_data_addr = Read();
						u32 size = Read();
						Read(&vuRegs.Mem[vu_data_addr], size);
						break;
					}
					case MTVU_VIF_WRITE_COL:
						Read(&vif.MaskCol, sizeof(vif.MaskCol));
						break;
					case MTVU_VIF_WRITE_ROW:
						Read(&vif.MaskRow, sizeof(vif.MaskRow));
						break;
					case MTVU_VIF_UNPACK: {
						u32 vif_copy_size = (uptr)&vif.StructEnd - (uptr)&vif.tag;
						Read(&vif.tag, vif_copy_size);
						ReadRegs(&vifRegs);
						u32 size = Read();
						MTVU_Unpack(&buffer[read_pos], vifRegs);
						incReadPos(size_u32(size));
						break;
					}
					case MTVU_NULL_PACKET:
						AtomicExchange(read_pos, 0);
						break;
					jNO_DEFAULT;
				}
			}
		}
	}

	// Returns Average number of vu Cycles from last 4 runs
	u32 Get_vuCycles() { // Used for vu cycle stealing hack
		return (AtomicRead(vuCycles[0]) + AtomicRead(vuCycles[1])
			  + AtomicRead(vuCycles[2]) + AtomicRead(vuCycles[3])) >> 2;
	}
public:
	
	// Get MTVU to start processing its packets if it isn't already
	void KickStart(bool forceKick = false) {
		if ((forceKick && !semaEvent.Count())
		|| (!isBusy && GetReadPos() != write_pos)) semaEvent.Post();
	}

	// Used for assertions...
	bool IsDone() { return !isBusy && GetReadPos() == GetWritePos(); }

	// Waits till MTVU is done processing
	void WaitVU() {
		MTVU_LOG("MTVU - WaitVU!");
		for(;;) {
			if (IsDone()) break;
			//DevCon.WriteLn("WaitVU()");
			pxAssert(THREAD_VU1);
			KickStart();
			ScopedLock lock(mtxBusy);
		}
	}

	void ExecuteVU(u32 vu_addr, u32 vif_top, u32 vif_itop) {
		MTVU_LOG("MTVU - ExecuteVU!");
		ReserveSpace(4);
		Write(MTVU_VU_EXECUTE);
		Write(vu_addr);
		Write(vif_top);
		Write(vif_itop);
		incWritePos();
		gifUnit.TransferGSPacketData(GIF_TRANS_MTVU, NULL, 0);
		KickStart();
		u32 cycles = std::min(Get_vuCycles(), 3000u);
		cpuRegs.cycle += cycles * EmuConfig.Speedhacks.VUCycleSteal;
	}

	void VifUnpack(vifStruct& _vif, VIFregisters& _vifRegs, u8* data, u32 size) {
		MTVU_LOG("MTVU - VifUnpack!");
		u32 vif_copy_size = (uptr)&_vif.StructEnd - (uptr)&_vif.tag;
		ReserveSpace(1 + size_u32(vif_copy_size) + size_u32(sizeof(VIFregistersMTVU)) + 1 + size_u32(size));
		Write(MTVU_VIF_UNPACK);
		Write(&_vif.tag, vif_copy_size);
		WriteRegs(&_vifRegs);
		Write(size);
		Write(data, size);
		incWritePos();
		KickStart();
	}

	// Writes to VU's Micro Memory (size in bytes)
	void WriteMicroMem(u32 vu_micro_addr, void* data, u32 size) {
		MTVU_LOG("MTVU - WriteMicroMem!");
		ReserveSpace(3 + size_u32(size));
		Write(MTVU_VU_WRITE_MICRO);
		Write(vu_micro_addr);
		Write(size);
		Write(data, size);
		incWritePos();
	}

	// Writes to VU's Data Memory (size in bytes)
	void WriteDataMem(u32 vu_data_addr, void* data, u32 size) {
		MTVU_LOG("MTVU - WriteDataMem!");
		ReserveSpace(3 + size_u32(size));
		Write(MTVU_VU_WRITE_DATA);
		Write(vu_data_addr);
		Write(size);
		Write(data, size);
		incWritePos();
	}

	void WriteCol(vifStruct& _vif) {
		MTVU_LOG("MTVU - WriteCol!");
		ReserveSpace(1 + size_u32(sizeof(_vif.MaskCol)));
		Write(MTVU_VIF_WRITE_COL);
		Write(&_vif.MaskCol, sizeof(_vif.MaskCol));
		incWritePos();
	}

	void WriteRow(vifStruct& _vif) {
		MTVU_LOG("MTVU - WriteRow!");
		ReserveSpace(1 + size_u32(sizeof(_vif.MaskRow)));
		Write(MTVU_VIF_WRITE_ROW);
		Write(&_vif.MaskRow, sizeof(_vif.MaskRow));
		incWritePos();
	}
};

extern __aligned16 VU_Thread vu1Thread;

