/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Common.h"
#include "System/SysThreads.h"

extern __aligned16 u8 g_RealGSMem[Ps2MemSize::GSregs];

#define PS2MEM_GS		g_RealGSMem
#define PS2GS_BASE(mem) (g_RealGSMem+(mem&0x13ff))

#define GSCSRr		((u64&)*(g_RealGSMem+0x1000))
#define GSIMR		((u32&)*(g_RealGSMem+0x1010))
#define GSSIGLBLID	((GSRegSIGBLID&)*(g_RealGSMem+0x1080))

enum GS_RegionMode
{
	Region_NTSC,
	Region_PAL
};

enum GIF_PATH
{
	GIF_PATH_1 = 0,
	GIF_PATH_2,
	GIF_PATH_3,
};

extern int  GIFPath_ParseTag(GIF_PATH pathidx, const u8* pMem, u32 size);
extern void GIFPath_Reset();
extern void GIFPath_Clear( GIF_PATH pathidx );

extern GS_RegionMode gsRegionMode;

/////////////////////////////////////////////////////////////////////////////
// MTGS Threaded Class Declaration

// Uncomment this to enable the MTGS debug stack, which tracks to ensure reads
// and writes stay synchronized.  Warning: the debug stack is VERY slow.
//#define RINGBUF_DEBUG_STACK

enum GS_RINGTYPE
{
	GS_RINGTYPE_RESTART = 0
,	GS_RINGTYPE_P1
,	GS_RINGTYPE_P2
,	GS_RINGTYPE_P3
,	GS_RINGTYPE_VSYNC
,	GS_RINGTYPE_FRAMESKIP
,	GS_RINGTYPE_MEMWRITE8
,	GS_RINGTYPE_MEMWRITE16
,	GS_RINGTYPE_MEMWRITE32
,	GS_RINGTYPE_MEMWRITE64
,	GS_RINGTYPE_FREEZE
,	GS_RINGTYPE_RECORD
,	GS_RINGTYPE_RESET		// issues a GSreset() command.
,	GS_RINGTYPE_SOFTRESET	// issues a soft reset for the GIF
,	GS_RINGTYPE_WRITECSR
,	GS_RINGTYPE_MODECHANGE	// for issued mode changes.
,	GS_RINGTYPE_CRC
,	GS_RINGTYPE_STARTTIME	// special case for min==max fps frameskip settings
};


struct MTGS_FreezeData
{
	freezeData*	fdata;
	s32			retval;		// value returned from the call, valid only after an mtgsWaitGS()
};

class mtgsThreadObject : public SysSuspendableThread
{
	typedef SysSuspendableThread _parent;

protected:
	// note: when g_pGSRingPos == g_pGSWritePos, the fifo is empty
	uint m_RingPos;		// cur pos gs is reading from
	uint m_WritePos;	// cur pos ee thread is writing to

	Semaphore m_sem_OpenDone;
	MutexLock m_lock_RingRestart;

	// used to keep multiple threads from sending packets to the ringbuffer concurrently.
	MutexLockRecursive m_PacketLocker;

	// Used to delay the sending of events.  Performance is better if the ringbuffer
	// has more than one command in it when the thread is kicked.
	int m_CopyCommandTally;
	int m_CopyDataTally;
	volatile bool m_RingBufferIsBusy;

	// Counts the number of vsync frames queued in the MTGS ringbuffer.  This is used to
	// throttle the number of frames allowed to be rendered ahead of time for games that
	// run very fast and have little or no ringbuffer overhead (typically opening menus)
	volatile s32 m_QueuedFrames;

	// Protection lock for the frame queue counter -- needed because we can't safely
	// AtomicExchange from two threads.
	MutexLock m_lock_FrameQueueCounter;

	// These vars maintain instance data for sending Data Packets.
	// Only one data packet can be constructed and uploaded at a time.

	uint m_packet_size;		// size of the packet (data only, ie. not including the 16 byte command!)
	uint m_packet_ringpos;	// index of the data location in the ringbuffer.

#ifdef RINGBUF_DEBUG_STACK
	Threading::MutexLock m_lock_Stack;
#endif

public:
	mtgsThreadObject();
	virtual ~mtgsThreadObject() throw();

	void Start();
	void OnStart();
	void PollStatus();

	// Waits for the GS to empty out the entire ring buffer contents.
	// Used primarily for plugin startup/shutdown.
	void WaitGS();
	void ResetGS();

	int PrepDataPacket( GIF_PATH pathidx, const u8*  srcdata, u32 size );
	int	PrepDataPacket( GIF_PATH pathidx, const u32* srcdata, u32 size );
	void SendDataPacket();
	void SendGameCRC( u32 crc );
	void WaitForOpen();
	void Freeze( int mode, MTGS_FreezeData& data );

	void SendSimplePacket( GS_RINGTYPE type, int data0, int data1, int data2 );
	void SendPointerPacket( GS_RINGTYPE type, u32 data0, void* data1 );

	u8* GetDataPacketPtr() const;
	void SetEvent();
	void PostVsyncEnd( bool updategs );

protected:
	void OpenPlugin();
	void OnSuspendInThread();
	void OnPauseInThread() {}
	void OnResumeInThread( bool IsSuspended );

	void OnResumeReady();

	// Saves MMX/XMM REGS, posts an event to the mtgsThread flag and releases a timeslice.
	// For use in surrounding loops that wait on the mtgs.
	void PrepEventWait();

	// Restores MMX/XMM REGS.  For use in surrounding loops that wait on the mtgs.
	void PostEventWait() const;

	// Used internally by SendSimplePacket type functions
	uint _PrepForSimplePacket();
	void _FinishSimplePacket( uint future_writepos );
	void ExecuteTask();
};

extern __aligned16 mtgsThreadObject mtgsThread;

void mtgsWaitGS();

/////////////////////////////////////////////////////////////////////////////
// Generalized GS Functions and Stuff

extern void gsInit();
extern s32 gsOpen();
extern void gsClose();
extern void gsReset();
extern void gsOnModeChanged( u32 framerate, u32 newTickrate );
extern void gsSetRegionMode( GS_RegionMode isPal );
extern void gsResetFrameSkip();
extern void gsSyncLimiterLostTime( s32 deltaTime );
extern void gsDynamicSkipEnable();
extern void gsPostVsyncEnd( bool updategs );
extern void gsFrameSkip( bool forceskip );

// Some functions shared by both the GS and MTGS
extern void _gs_ResetFrameskip();
extern void _gs_ChangeTimings( u32 framerate, u32 iTicks );


// used for resetting GIF fifo
extern void gsGIFReset();
extern void gsCSRwrite(u32 value);

extern void gsWrite8(u32 mem, u8 value);
extern void gsWrite16(u32 mem, u16 value);
extern void gsWrite32(u32 mem, u32 value);

extern void __fastcall gsWrite64_page_00( u32 mem, const mem64_t* value );
extern void __fastcall gsWrite64_page_01( u32 mem, const mem64_t* value );
extern void __fastcall gsWrite64_generic( u32 mem, const mem64_t* value );

extern void __fastcall gsWrite128_page_00( u32 mem, const mem128_t* value );
extern void __fastcall gsWrite128_page_01( u32 mem, const mem128_t* value );
extern void __fastcall gsWrite128_generic( u32 mem, const mem128_t* value );

extern u8   gsRead8(u32 mem);
extern u16  gsRead16(u32 mem);
extern u32  gsRead32(u32 mem);
extern u64  gsRead64(u32 mem);


//extern void gsWrite64(u32 mem, u64 value);


void gsConstWrite8(u32 mem, int mmreg);
void gsConstWrite16(u32 mem, int mmreg);
void gsConstWrite32(u32 mem, int mmreg);
void gsConstWrite64(u32 mem, int mmreg);
void gsConstWrite128(u32 mem, int mmreg);

int gsConstRead8(u32 x86reg, u32 mem, u32 sign);
int gsConstRead16(u32 x86reg, u32 mem, u32 sign);
int gsConstRead32(u32 x86reg, u32 mem);
void gsConstRead64(u32 mem, int mmreg);
void gsConstRead128(u32 mem, int xmmreg);

void gsIrq();

extern u32 CSRw;
extern u64 m_iSlowStart;

// GS Playback
enum gsrun
{
	GSRUN_TRANS1	= 1,
	GSRUN_TRANS2,
	GSRUN_TRANS3,
	GSRUN_VSYNC
};

#ifdef PCSX2_DEVBUILD

extern int g_SaveGSStream;
extern int g_nLeftGSFrames;

#endif

extern void SaveGSState(const wxString& file);
extern void LoadGSState(const wxString& file);
extern void RunGSState( memLoadingState& f );

