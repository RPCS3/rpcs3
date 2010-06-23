/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

enum CSRfifoState
{
    CSR_FIFO_NORMAL = 0, // Neither empty or almost full.
    CSR_FIFO_EMPTY, // Empty
    CSR_FIFO_FULL, // Almost Full
    CSR_FIFO_RESERVED
};

union tGS_CSRw
{
    struct
    {
        u32 SIGNAL : 1; // SIGNAL event
        u32 FINISH : 1; // FINISH event
        u32 HSINT : 1; // HSYNC Interrupt
        u32 VSINT : 1; // VSYNC Interrupt
        u32 EDWINT : 1; // Rect Area Write Termination Interrupt
    };
    u32 _u32;

    void reset() { _u32 = 0; }
    void fill()
    {
		SIGNAL = true;
		FINISH = true;
		HSINT = true;
		VSINT = true;
		EDWINT = true;
    }
};

// I'm initializing this as 64 bit because GSCSRr is 64 bit. There only appeared to be 32 bits worth of fields,
// and CSRw is 32 bit, though, so I'm not sure if that's correct.
union tGS_CSR
{
    struct
    {
        // Start Interrupts.
        // If reading, 1 means a signal has been generated.
        u64 SIGNAL : 1; // SIGNAL event
        u64 FINISH : 1; // FINISH event
        u64 HSINT : 1; // HSYNC Interrupt
        u64 VSINT : 1; // VSYNC Interrupt
        u64 EDWINT : 1; // Rect Area Write Termination Interrupt
        // End of Interrupts. Those 5 fields together are 0x1f.

        u64 undefined : 2; // Should both be 0.
        u64 reserved1 : 1;
        u64 FLUSH : 1; // Drawing Suspend And FIFO Clear
        u64 RESET : 1; // GS System Reset
        u64 reserved2 : 2;
        u64 NFIELD : 1;
        u64 FIELD : 1; // If the field currently displayed in Interlace mode is even or odd
        u64 FIFO : 2;
        u64 REV : 8; // The GS's Revision number
        u64 ID : 8; // The GS's Id.
        u64 reserved3 : 32;
    };
    u64 _u64;

    void reset()
    {
        _u64 = 0;
        FIFO = CSR_FIFO_EMPTY;
        REV = 0x1D; // GS Revision
        ID = 0x55; // GS ID
    }

    void set(u64 value)
    {
        _u64 = value;
    }

    bool interrupts() { return (SIGNAL || FINISH || HSINT || VSINT || EDWINT); }

    void setAllInterrupts(bool value)
    {
        SIGNAL = FINISH = HSINT = VSINT = EDWINT = value;
    }

	tGS_CSR(u64 val) { _u64 = val; }
	tGS_CSR(u32 val) { _u64 = (u64)val; }
	tGS_CSR() { reset(); }
};

union tGS_IMR
{
    struct
    {
        u32 reserved1 : 8;
        u32 SIGMSK : 1;
        u32 FINISHMSK : 1;
        u32 HSMSK : 1;
        u32 VSMSK : 1;
        u32 EDWMSK : 1;
        u32 undefined : 2; // Should both be set to 1.
        u32 reserved2 : 17;
    };
    u32 _u32;
    void reset()
    {
        _u32 = 0;
        SIGMSK = FINISHMSK = HSMSK = VSMSK = EDWMSK = true;
        undefined = 0x3;
    }
    void set(u32 value)
    {
        _u32 = (value & 0x1f00); // Set only the interrupt mask fields.
        undefined = 0x3; // These should always be set.
    }

    bool masked() { return (SIGMSK || FINISHMSK || HSMSK || VSMSK || EDWMSK); }
};

#define PS2MEM_GS		g_RealGSMem
#define PS2GS_BASE(mem) (g_RealGSMem+(mem&0x13ff))

#define GSCSRregs      ((tGS_CSR&)*(g_RealGSMem+0x1000))
#define GSIMRregs      ((tGS_IMR&)*(g_RealGSMem+0x1010))

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

extern int  GIFPath_ParseTag(GIF_PATH pathidx, const u8* pMem, u32 size, bool TestOnly);
extern void GIFPath_Reset();
extern void GIFPath_Clear( GIF_PATH pathidx );

extern GS_RegionMode gsRegionMode;

/////////////////////////////////////////////////////////////////////////////
// MTGS Threaded Class Declaration

// Uncomment this to enable the MTGS debug stack, which tracks to ensure reads
// and writes stay synchronized.  Warning: the debug stack is VERY slow.
//#define RINGBUF_DEBUG_STACK

enum MTGS_RingCommand
{
	GS_RINGTYPE_P1
,	GS_RINGTYPE_P2
,	GS_RINGTYPE_P3
,	GS_RINGTYPE_MEMWRITE64

,	GS_RINGTYPE_MEMWRITE8
,	GS_RINGTYPE_MEMWRITE16
,	GS_RINGTYPE_MEMWRITE32

,	GS_RINGTYPE_RESTART
,	GS_RINGTYPE_VSYNC
,	GS_RINGTYPE_FRAMESKIP
,	GS_RINGTYPE_FREEZE
,	GS_RINGTYPE_RECORD
,	GS_RINGTYPE_RESET			// issues a GSreset() command.
,	GS_RINGTYPE_SOFTRESET		// issues a soft reset for the GIF
,	GS_RINGTYPE_WRITECSR
,	GS_RINGTYPE_MODECHANGE		// for issued mode changes.
,	GS_RINGTYPE_CRC
};


struct MTGS_FreezeData
{
	freezeData*	fdata;
	s32			retval;		// value returned from the call, valid only after an mtgsWaitGS()
};

// --------------------------------------------------------------------------------------
//  SysMtgsThread
// --------------------------------------------------------------------------------------
class SysMtgsThread : public SysThreadBase
{
	typedef SysThreadBase _parent;

public:
	// note: when m_RingPos == m_WritePos, the fifo is empty
	uint			m_RingPos;			// cur pos gs is reading from
	uint			m_WritePos;			// cur pos ee thread is writing to

	volatile bool	m_RingBufferIsBusy;
	volatile u32	m_SignalRingEnable;
	volatile s32	m_SignalRingPosition;

	int				m_QueuedFrameCount;
	u32				m_RingWrapSpot;

	Mutex			m_lock_RingBufferBusy;
	Semaphore		m_sem_OnRingReset;

	// used to keep multiple threads from sending packets to the ringbuffer concurrently.
	// (currently not used or implemented -- is a planned feature for a future threaded VU1)
	//MutexLockRecursive m_PacketLocker;

	// Used to delay the sending of events.  Performance is better if the ringbuffer
	// has more than one command in it when the thread is kicked.
	int				m_CopyDataTally;

	Semaphore		m_sem_OpenDone;
	volatile bool	m_PluginOpened;

	// These vars maintain instance data for sending Data Packets.
	// Only one data packet can be constructed and uploaded at a time.

	uint			m_packet_size;		// size of the packet (data only, ie. not including the 16 byte command!)
	uint			m_packet_ringpos;	// index of the data location in the ringbuffer.

#ifdef RINGBUF_DEBUG_STACK
	Threading::Mutex m_lock_Stack;
#endif

public:
	SysMtgsThread();
	virtual ~SysMtgsThread() throw();

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

	void RestartRingbuffer( uint packsize=0 );
	void SendSimplePacket( MTGS_RingCommand type, int data0, int data1, int data2 );
	void SendPointerPacket( MTGS_RingCommand type, u32 data0, void* data1 );

	u8* GetDataPacketPtr() const;
	void SetEvent();
	void PostVsyncEnd();

	bool IsPluginOpened() const { return m_PluginOpened; }

protected:
	void OpenPlugin();
	void ClosePlugin();

	void OnStart();
	void OnResumeReady();

	void OnSuspendInThread();
	void OnPauseInThread() {}
	void OnResumeInThread( bool IsSuspended );
	void OnCleanupInThread();

	// Used internally by SendSimplePacket type functions
	uint _PrepForSimplePacket();
	void _FinishSimplePacket( uint future_writepos );
	void ExecuteTaskInThread();
};

// GetMTGS() is a required external implementation. This function is *NOT* provided
// by the PCSX2 core library.  It provides an interface for the linking User Interface
// apps or DLLs to reference their own instance of SysMtgsThread (also allowing them
// to extend the class and override virtual methods).
//
extern SysMtgsThread& GetMTGS();

/////////////////////////////////////////////////////////////////////////////
// Generalized GS Functions and Stuff

extern void gsInit();
extern s32 gsOpen();
extern void gsClose();
extern void gsReset();
extern void gsOnModeChanged( Fixed100 framerate, u32 newTickrate );
extern void gsSetRegionMode( GS_RegionMode isPal );
extern void gsResetFrameSkip();
extern void gsPostVsyncEnd();
extern void gsFrameSkip();

// Some functions shared by both the GS and MTGS
extern void _gs_ResetFrameskip();


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

void gsIrq();

extern u32 CSRw;

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

