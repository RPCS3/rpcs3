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

#include "PrecompiledHeader.h"
#include "Common.h"

#include <list>
#include <wx/datetime.h>

#include "GS.h"
#include "Elfheader.h"
#include "SamplProf.h"


// Uncomment this to enable profiling of the GS RingBufferCopy function.
//#define PCSX2_GSRING_SAMPLING_STATS

using namespace Threading;

#if 0 // PCSX2_DEBUG
#	define MTGS_LOG Console.WriteLn
#else
#	define MTGS_LOG 0&&
#endif

// forces the compiler to treat a non-volatile value as volatile.
// This allows us to declare the vars as non-volatile and only use
// them as volatile when appropriate (more optimized).

#define volatize(x) (*reinterpret_cast<volatile uint*>(&(x)))


// =====================================================================================================
//  MTGS Threaded Class Implementation
// =====================================================================================================

// Size of the ringbuffer as a power of 2 -- size is a multiple of simd128s.
// (actual size is 1<<m_RingBufferSizeFactor simd vectors [128-bit values])
// A value of 19 is a 8meg ring buffer.  18 would be 4 megs, and 20 would be 16 megs.
// Default was 2mb, but some games with lots of MTGS activity want 8mb to run fast (rama)
static const uint RingBufferSizeFactor = 19;

// size of the ringbuffer in simd128's.
static const uint RingBufferSize = 1<<RingBufferSizeFactor;

// Mask to apply to ring buffer indices to wrap the pointer from end to
// start (the wrapping is what makes it a ringbuffer, yo!)
static const uint RingBufferMask = RingBufferSize - 1;

struct MTGS_BufferedData
{
	u128		m_Ring[RingBufferSize];
	u8			Regs[Ps2MemSize::GSregs];

	MTGS_BufferedData() {}

	u128& operator[]( uint idx )
	{
		pxAssert( idx < RingBufferSize );
		return m_Ring[idx];
	}
};

static __aligned(32) MTGS_BufferedData RingBuffer;
extern bool renderswitch;


#ifdef RINGBUF_DEBUG_STACK
#include <list>
std::list<uint> ringposStack;
#endif

SysMtgsThread::SysMtgsThread() :
	SysThreadBase()
#ifdef RINGBUF_DEBUG_STACK
,	m_lock_Stack()
#endif
{
	m_name = L"MTGS";

	// All other state vars are initialized by OnStart().
}

void SysMtgsThread::OnStart()
{
	m_PluginOpened		= false;

	m_RingPos			= 0;
	m_WritePos			= 0;
	m_RingBufferIsBusy	= false;
	m_packet_size		= 0;
	m_packet_ringpos	= 0;

	m_QueuedFrameCount	= 0;
	m_SignalRingEnable	= 0;
	m_SignalRingPosition= 0;
	m_RingWrapSpot		= 0;

	m_CopyDataTally		= 0;

	_parent::OnStart();
}

SysMtgsThread::~SysMtgsThread() throw()
{
	_parent::Cancel();
}

void SysMtgsThread::OnResumeReady()
{
	m_sem_OpenDone.Reset();
}

void SysMtgsThread::ResetGS()
{
	// MTGS Reset process:
	//  * clear the ringbuffer.
	//  * Signal a reset.
	//  * clear the path and byRegs structs (used by GIFtagDummy)

	m_RingPos = m_WritePos;

	MTGS_LOG( "MTGS: Sending Reset..." );
	SendSimplePacket( GS_RINGTYPE_RESET, 0, 0, 0 );
	SendSimplePacket( GS_RINGTYPE_FRAMESKIP, 0, 0, 0 );
	SetEvent();

	GIFPath_Reset();
}

struct RingCmdPacket_Vsync
{
	u8		regset1[0x100];
	u32		csr;
	u32		imr;
	GSRegSIGBLID	siglblid;
};

void SysMtgsThread::PostVsyncEnd()
{
	// Optimization note: Typically regset1 isn't needed.  The regs in that area are typically
	// changed infrequently, usually during video mode changes.  However, on modern systems the
	// 256-byte copy is only a few dozen cycles -- executed 60 times a second -- so probably
	// not worth the effort or overhead of trying to selectively avoid it.

	PrepDataPacket(GS_RINGTYPE_VSYNC, sizeof(RingCmdPacket_Vsync));
	RingCmdPacket_Vsync& local( *(RingCmdPacket_Vsync*)GetDataPacketPtr() );
	memcpy_fast( local.regset1, PS2MEM_GS, sizeof(local.regset1) );
	local.csr = GSCSRr;
	local.imr = GSIMR;
	local.siglblid = GSSIGLBLID;
	SendDataPacket();

	// Alter-frame flushing!  Restarts the ringbuffer (wraps) on every other frame.  This is a
	// mandatory feature that prevents the MTGS from queuing more than 2 frames at any time.
	// (queued frames cause input lag and desynced audio -- bad!).  Ring restarts work for this
	// because they act as sync points where the EE must stall to wait for the GS to catch-up,
	// and they also allow us to reuse the front of the ringbuffer more often, which should improve
	// L2 cache performance.

	if( m_QueuedFrameCount > 0 )
		RestartRingbuffer();
	else
	{
		m_QueuedFrameCount++;
		SetEvent();
	}
}

struct PacketTagType
{
	u32 command;
	u32 data[3];
};

static void dummyIrqCallback()
{
	// dummy, because MTGS doesn't need this mess!
	// (and zerogs does >_<)
}

void SysMtgsThread::OpenPlugin()
{
	if( m_PluginOpened ) return;

	memcpy_aligned( RingBuffer.Regs, PS2MEM_GS, sizeof(PS2MEM_GS) );
	GSsetBaseMem( RingBuffer.Regs );
	GSirqCallback( dummyIrqCallback );

	int result;

	if( GSopen2 != NULL )
		result = GSopen2( (void*)&pDsp, 1 | (renderswitch ? 4 : 0) );
	else
		result = GSopen( (void*)&pDsp, "PCSX2", renderswitch ? 2 : 1 );

	// Vsync on / off ?
	if( renderswitch )
	{
		Console.Indent(2).WriteLn( "Forced software switch enabled." );
		if (EmuConfig.GS.VsyncEnable)
		{
			// Better turn Vsync off now, as in most cases sw rendering is not fast enough to support a steady 60fps.
			// Having Vsync still enabled then means a big cut in speed and sloppy rendering.
			// It's possible though that some users have very fast machines, and rather kept Vsync enabled,
			// but let's assume this is the minority. At least for now ;)
			GSsetVsync( false );
			Console.Indent(2).WriteLn( "Vsync temporarily disabled" );
		}
	}
	else
	{
		GSsetVsync( EmuConfig.GS.FrameLimitEnable && EmuConfig.GS.VsyncEnable );
	}

	if( result != 0 )
	{
		DevCon.WriteLn( "GSopen Failed: return code: 0x%x", result );
		throw Exception::PluginOpenError( PluginId_GS );
	}

// This is the preferred place to implement DXGI fullscreen overrides, using LoadLibrary.
// But I hate COM, I don't know to make this work, and I don't have DX10, so I give up
// and enjoy my working DX9 alt-enter instead. Someone else can fix this mess. --air

// Also: Prolly needs some DX10 header includes?  Which ones?  Too many, I gave up.

#if 0    // defined(__WXMSW__) && defined(_MSC_VER)
	wxDynamicLibrary dynlib( L"dxgi.dll" );
	SomeFuncTypeIDunno isThisEvenTheRightFunctionNameIDunno = dynlib.GetSymbol("CreateDXGIFactory");
	if( isThisEvenTheRightFunctionNameIDunno )
	{
		// Is this how LoadLibrary for COM works?  I dunno.  I dont care.

		IDXGIFactory* pFactory;
		hr = isThisEvenTheRightFunctionNameIDunno(__uuidof(IDXGIFactory), (void**)(&pFactory) );
		pFactory->MakeWindowAssociation((HWND)&pDsp, DXGI_MWA_NO_WINDOW_CHANGES);
		pFactory->Release();
	}
#endif

	m_PluginOpened = true;
	m_sem_OpenDone.Post();

	GSsetGameCRC( ElfCRC, 0 );
}

class RingBufferLock : public ScopedLock
{
protected:
	SysMtgsThread&		m_mtgs;

public:
	RingBufferLock( SysMtgsThread& mtgs )
		: ScopedLock( mtgs.m_lock_RingBufferBusy )
		, m_mtgs( mtgs )
	{
		m_mtgs.m_RingBufferIsBusy = true;
	}

	virtual ~RingBufferLock() throw()
	{
		m_mtgs.m_RingBufferIsBusy = false;
	}
};

void SysMtgsThread::ExecuteTaskInThread()
{
#ifdef RINGBUF_DEBUG_STACK
	PacketTagType prevCmd;
#endif

	while( true )
	{
		// Performance note: Both of these perform cancellation tests, but pthread_testcancel
		// is very optimized (only 1 instruction test in most cases), so no point in trying
		// to avoid it.

		m_sem_event.WaitWithoutYield();
		StateCheckInThread();

		{
		RingBufferLock busy( *this );

		// note: m_RingPos is intentionally not volatile, because it should only
		// ever be modified by this thread.
		while( m_RingPos != volatize(m_WritePos))
		{
			if( EmuConfig.GS.DisableOutput )
			{
				m_RingPos = m_WritePos;
				continue;
			}

			pxAssert( m_RingPos < RingBufferSize );

			const PacketTagType& tag = (PacketTagType&)RingBuffer[m_RingPos];
			u32 ringposinc = 1;

#ifdef RINGBUF_DEBUG_STACK
			// pop a ringpos off the stack.  It should match this one!

			m_lock_Stack.Lock();
			uptr stackpos = ringposStack.back();
			if( stackpos != m_RingPos )
			{
				Console.Error( "MTGS Ringbuffer Critical Failure ---> %x to %x (prevCmd: %x)\n", stackpos, m_RingPos, prevCmd.command );
			}
			pxAssert( stackpos == m_RingPos );
			prevCmd = tag;
			ringposStack.pop_back();
			m_lock_Stack.Release();
#endif

			switch( tag.command )
			{
				case GS_RINGTYPE_PATH:
				{
					const int qsize = tag.data[0];
					const u128* data = &RingBuffer[m_RingPos+1];

					MTGS_LOG( "(MTGS Packet Read) ringtype=P2, qwc=%u", qsize );

					// All GIFpath data is sent through Path2, which is the hack-free giftag
					// parser on the GS plugin side of the world.  PCSX2 now handles *all* the
					// hacks, wrap-arounds, and (soon!) partial transfers during its own internal
					// GIFtag parse, so the hack-free path on the GS is the preferred one for
					// all packets.  -air

					GSgifTransfer2((u32*)data, qsize);
					ringposinc += qsize;
				}
				break;

				case GS_RINGTYPE_RESTART:
					//MTGS_LOG( "(MTGS Packet Read) ringtype=Restart" );
					m_RingPos = 0;
				continue;

				case GS_RINGTYPE_VSYNC:
				{
					const int qsize = tag.data[0];
					ringposinc += qsize;

					MTGS_LOG( "(MTGS Packet Read) ringtype=Vsync, field=%u, skip=%s", tag.data[0], tag.data[1] ? "true" : "false" );
					
					// Mail in the important GS registers.
					RingCmdPacket_Vsync& local((RingCmdPacket_Vsync&)RingBuffer[m_RingPos+1]);
					
					memcpy_fast( RingBuffer.Regs, local.regset1, sizeof(local.regset1));
					((u32&)RingBuffer.Regs[0x1000]) = local.csr;
					((u32&)RingBuffer.Regs[0x1010]) = local.imr;
					((GSRegSIGBLID&)RingBuffer.Regs[0x1080]) = local.siglblid;
					
					GSvsync(!(local.csr & 0x2000));
					gsFrameSkip();

					// if we're not using GSOpen2, then the GS window is on this thread (MTGS thread),
					// so we need to call PADupdate from here.
					if( (GSopen2 == NULL) && (PADupdate != NULL) )
						PADupdate(0);

					StateCheckInThread();
				}
				break;

				case GS_RINGTYPE_FRAMESKIP:
					MTGS_LOG( "(MTGS Packet Read) ringtype=Frameskip" );
					_gs_ResetFrameskip();
				break;

				case GS_RINGTYPE_FREEZE:
				{
					MTGS_FreezeData* data = (MTGS_FreezeData*)(*(uptr*)&tag.data[1]);
					int mode = tag.data[0];
					data->retval = GetCorePlugins().DoFreeze( PluginId_GS, mode, data->fdata );
				}
				break;

				case GS_RINGTYPE_RESET:
					MTGS_LOG( "(MTGS Packet Read) ringtype=Reset" );
					if( GSreset != NULL ) GSreset();
				break;

				case GS_RINGTYPE_SOFTRESET:
				{
					int mask = tag.data[0];
					MTGS_LOG( "(MTGS Packet Read) ringtype=SoftReset" );
					GSgifSoftReset( mask );
				}
				break;

				case GS_RINGTYPE_MODECHANGE:
					// [TODO] some frameskip sync logic might be needed here!
				break;

				case GS_RINGTYPE_CRC:
					GSsetGameCRC( tag.data[0], 0 );
				break;

#ifdef PCSX2_DEVBUILD
				default:
					Console.Error("GSThreadProc, bad packet (%x) at m_RingPos: %x, m_WritePos: %x", tag.command, m_RingPos, m_WritePos);
					pxFail( "Bad packet encountered in the MTGS Ringbuffer." );
					m_RingPos = m_WritePos;
				continue;
#else
				// Optimized performance in non-Dev builds.
				jNO_DEFAULT;
#endif
			}

			uint newringpos = m_RingPos + ringposinc;
			pxAssert( newringpos <= RingBufferSize );
			m_RingPos = newringpos & RingBufferMask;

			if( m_SignalRingEnable != 0 )
			{
				// The EEcore has requested a signal after some amount of processed data.
				if( AtomicExchangeSub( m_SignalRingPosition, ringposinc ) <= 0 )
				{
					// Make sure to post the signal after the m_RingPos has been updated...
					AtomicExchange( m_SignalRingEnable, 0 );
					m_sem_OnRingReset.Post();
					continue;
				}
			}
		}
		}

		// Safety valve in case standard signals fail for some reason -- this ensures the EEcore
		// won't sleep the eternity, even if SignalRingPosition didn't reach 0 for some reason.
		// Important: Need to unlock the MTGS busy signal PRIOR, so that EEcore SetEvent() calls
		// parallel to this handler aren't accidentally blocked.
		if( AtomicExchange( m_SignalRingEnable, 0 ) != 0 )
		{
			//Console.Warning( "(MTGS Thread) Dangling RingSignal on empty buffer!  signalpos=0x%06x", AtomicExchange( m_SignalRingPosition, 0 ) );
			AtomicExchange( m_SignalRingPosition, 0 );
			m_sem_OnRingReset.Post();
		}

		//Console.Warning( "(MTGS Thread) Nothing to do!  ringpos=0x%06x", m_RingPos );
	}
}

void SysMtgsThread::ClosePlugin()
{
	if( !m_PluginOpened ) return;
	m_PluginOpened = false;
	GetCorePlugins().Close( PluginId_GS );
}

void SysMtgsThread::OnSuspendInThread()
{
	ClosePlugin();
	_parent::OnSuspendInThread();
}

void SysMtgsThread::OnResumeInThread( bool isSuspended )
{
	if( isSuspended )
		OpenPlugin();

	_parent::OnResumeInThread( isSuspended );
}

void SysMtgsThread::OnCleanupInThread()
{
	ClosePlugin();
	_parent::OnCleanupInThread();
}

// Waits for the GS to empty out the entire ring buffer contents.
// Used primarily for plugin startup/shutdown.
void SysMtgsThread::WaitGS()
{
	pxAssertDev( !IsSelf(), "This method is only allowed from threads *not* named MTGS." );

	if( m_ExecMode == ExecMode_NoThreadYet || !IsRunning() ) return;
	if( !pxAssertDev( IsOpen(), "MTGS Warning!  WaitGS issued on a closed thread." ) ) return;

	if( volatize(m_RingPos) != m_WritePos )
	{
		SetEvent();
		RethrowException();

		do {
			m_lock_RingBufferBusy.Wait();
			RethrowException();
		} while( volatize(m_RingPos) != m_WritePos );
	}
	
	// Completely synchronize GS and MTGS register states.
	memcpy_fast( RingBuffer.Regs, PS2MEM_GS, sizeof(RingBuffer.Regs) );
}

// Sets the gsEvent flag and releases a timeslice.
// For use in loops that wait on the GS thread to do certain things.
void SysMtgsThread::SetEvent()
{
	if( !m_RingBufferIsBusy )
		m_sem_event.Post();

	m_CopyDataTally = 0;
}

u8* SysMtgsThread::GetDataPacketPtr() const
{
	return (u8*)&RingBuffer[m_packet_ringpos];
}

// Closes the data packet send command, and initiates the gs thread (if needed).
void SysMtgsThread::SendDataPacket()
{
	// make sure a previous copy block has been started somewhere.
	pxAssert( m_packet_size != 0 );

	uint temp = m_packet_ringpos + m_packet_size;
	pxAssert( temp <= RingBufferSize );
	temp &= RingBufferMask;

	if( IsDebugBuild )
	{
		if( m_packet_ringpos + m_packet_size < RingBufferSize )
		{
			uint readpos = volatize(m_RingPos);
			if( readpos != m_WritePos )
			{
				// The writepos should never leapfrog the readpos
				// since that indicates a bad write.
				if( m_packet_ringpos < readpos )
					pxAssert( temp < readpos );
			}

			// Updating the writepos should never make it equal the readpos, since
			// that would stop the buffer prematurely (and indicates bad code in the
			// ringbuffer manager)
			pxAssert( readpos != temp );
		}
	}

	m_WritePos = temp;

	if( EmuConfig.GS.SynchronousMTGS )
	{
		WaitGS();
	}
	else if( !m_RingBufferIsBusy )
	{
		m_CopyDataTally += m_packet_size;
		if( m_CopyDataTally > 0x2000 ) SetEvent();
	}

	m_packet_size = 0;

	//m_PacketLocker.Release();
}

int SysMtgsThread::PrepDataPacket( MTGS_RingCommand cmd, u32 size )
{
	// Note on volatiles: m_WritePos is not modified by the GS thread, so there's no need
	// to use volatile reads here.  We do cache it though, since we know it never changes,
	// except for calls to RingbufferRestert() -- handled below.
	uint writepos = m_WritePos;

	// Checks if a previous copy was started without an accompanying call to GSRINGBUF_DONECOPY
	pxAssert( m_packet_size == 0 );

	// Sanity checks! (within the confines of our ringbuffer please!)
	pxAssert( size < RingBufferSize );
	pxAssert( writepos < RingBufferSize );

	m_packet_size = size;
	++size;			// takes into account our RingCommand QWC.

	if( writepos + size < RingBufferSize )
	{
		// generic gs wait/stall.
		// if the writepos is past the readpos then we're safe.
		// But if not then we need to make sure the readpos is outside the scope of
		// the block about to be written (writepos + size)

		uint readpos = volatize(m_RingPos);
		if( (writepos < readpos) && (writepos+size >= readpos) )
		{
			// writepos is behind the readpos and will overlap it if we commit the data,
			// so we need to wait until readpos is out past the end of the future write pos,
			// or until it wraps around (in which case writepos will be >= readpos).

			// Ideally though we want to wait longer, because if we just toss in this packet
			// the next packet will likely stall up too.  So lets set a condition for the MTGS
			// thread to wake up the EE once there's a sizable chunk of the ringbuffer emptied.

			uint totalAccum	= (m_RingWrapSpot - readpos) + writepos;
			uint somedone	= totalAccum / 4;
			if( somedone < size+1 ) somedone = size + 1;

			// FMV Optimization: FMVs typically send *very* little data to the GS, in some cases
			// every other frame is nothing more than a page swap.  Sleeping the EEcore is a
			// waste of time, and we get better results using a spinwait.

			if( somedone > 0x80 )
			{
				pxAssertDev( m_SignalRingEnable == 0, "MTGS Thread Synchronization Error" );
				m_SignalRingPosition = somedone;

				//Console.WriteLn( Color_Blue, "(EEcore Sleep) GenStall \tringpos=0x%06x, writepos=0x%06x, wrapspot=0x%06x, signalpos=0x%06x", readpos, writepos, m_RingWrapSpot, m_SignalRingPosition );

				do {
					AtomicExchange( m_SignalRingEnable, 1 );
					SetEvent();
					m_sem_OnRingReset.WaitWithoutYield();
					readpos = volatize(m_RingPos);
					//Console.WriteLn( Color_Blue, "(EEcore Awake) Report!\tringpos=0x%06x", readpos );
				} while( (writepos < readpos) && (writepos+size >= readpos) );

				pxAssertDev( m_SignalRingPosition <= 0, "MTGS Thread Synchronization Error" );
			}
			else
			{
				SetEvent();
				do {
					SpinWait();
					readpos = volatize(m_RingPos);
				} while( (writepos < readpos) && (writepos+size >= readpos) );
			}
		}
	}
	else if( writepos + size > RingBufferSize )
	{
		pxAssert( writepos != 0 );

		// If the incoming packet doesn't fit, then start over from the start of the ring
		// buffer (it's a lot easier than trying to wrap the packet around the end of the
		// buffer).

		//Console.WriteLn( "MTGS > Ringbuffer Got Filled!");
		RestartRingbuffer( size );
		writepos = m_WritePos;
	}
    else	// always true - if( writepos + size == MTGS_RINGBUFFEREND )
	{
		// Yay.  Perfect fit.  What are the odds?
		// Copy is ready so long as readpos is less than writepos and *not* equal to the
		// base of the ringbuffer (otherwise the buffer will stop when the writepos is
		// wrapped around to zero later-on in SendDataPacket).

		uint readpos = volatize(m_RingPos);
		//Console.WriteLn( "MTGS > Perfect Fit!\tringpos=0x%06x, writepos=0x%06x", readpos, writepos );
		if( readpos > writepos || readpos == 0 )
		{
			uint totalAccum	= (readpos == 0) ? RingBufferSize : ((m_RingWrapSpot - readpos) + writepos);
			uint somedone	= totalAccum / 4;
			if( somedone < size+1 ) somedone = size + 1;

			// FMV Optimization: (see above) This condition of a perfect fit is so rare that optimizing
			// for it is pointless -- but it was also mindlessly simple copy-paste.  So there. :p

			if( somedone > 0x80 )
			{
				m_SignalRingPosition = somedone;

				//Console.WriteLn( Color_Blue, "(MTGS Sync) EEcore Perfect Sleep!\twrapspot=0x%06x, ringpos=0x%06x, writepos=0x%06x, signalpos=0x%06x", m_RingWrapSpot, readpos, writepos, m_SignalRingPosition );

				do {
					AtomicExchange( m_SignalRingEnable, 1 );
					SetEvent();
					m_sem_OnRingReset.WaitWithoutYield();
					readpos = volatize(m_RingPos);
					//Console.WriteLn( Color_Blue, "(MTGS Sync) EEcore Perfect Post-sleep Report!\tringpos=0x%06x", readpos );
				} while( (writepos < readpos) || (readpos==0) );

				pxAssertDev( m_SignalRingPosition <= 0, "MTGS Thread Synchronization Error" );
			}
			else
			{
				//Console.WriteLn( Color_Blue, "(MTGS Sync) EEcore Perfect Spin!" );
				SetEvent();
				do {
					SpinWait();
					readpos = volatize(m_RingPos);
				} while( (writepos < readpos) || (readpos==0) );
			}
		}

		m_QueuedFrameCount = 0;
		m_RingWrapSpot = RingBufferSize;
    }

#ifdef RINGBUF_DEBUG_STACK
	m_lock_Stack.Lock();
	ringposStack.push_front( writepos );
	m_lock_Stack.Release();
#endif

	// Command qword: Low word is the command, and the high word is the packet
	// length in SIMDs (128 bits).

	PacketTagType& tag = (PacketTagType&)RingBuffer[m_WritePos];
	tag.command = cmd;
	tag.data[0] = m_packet_size;
	m_packet_ringpos = m_WritePos + 1;

	return m_packet_size;
}

// Returns the amount of giftag data processed (in simd128 values).
// Return value is used by VU1's XGKICK instruction to wrap the data
// around VU memory instead of having buffer overflow...
// Parameters:
//  size - size of the packet data, in smd128's
int SysMtgsThread::PrepDataPacket( GIF_PATH pathidx, const u8* srcdata, u32 size )
{
	//m_PacketLocker.Acquire();

	return PrepDataPacket( GS_RINGTYPE_PATH, GIFPath_ParseTag(pathidx, srcdata, size) );
}

void SysMtgsThread::RestartRingbuffer( uint packsize )
{
	if( m_WritePos == 0 ) return;
	const uint thefuture = packsize;

	//Console.WriteLn( Color_Magenta, "**** Ringbuffer Restart!!" );
	// Always kick the MTGS into action for a ringbuffer restart.
	SetEvent();

	uint readpos = volatize(m_RingPos);

	if( (readpos > m_WritePos) || (readpos <= thefuture) )
	{
		// We have to be careful not to leapfrog our read-position, which would happen if
		// it's greater than the current write position (since wrapping writepos to 0 would
		// be the act of skipping PAST readpos).  Stall until it loops around to the
		// beginning of the buffer, and past the size of our packet allocation.

		uint somedone;

		if( readpos > m_WritePos )
			somedone = (m_RingWrapSpot - readpos) + packsize + 1;
		else
			somedone = (packsize + 1) - readpos;

		if( somedone > 0x80 )
		{
			m_SignalRingPosition = somedone;
			//Console.WriteLn( Color_Blue, "(EEcore Sleep) Restart!\tringpos=0x%06x, writepos=0x%06x, wrapspot=0x%06x, signalpos=0x%06x",
			//	readpos, m_WritePos, m_RingWrapSpot, m_SignalRingPosition );

			do {
				AtomicExchange( m_SignalRingEnable, 1 );
				SetEvent();
				m_sem_OnRingReset.WaitWithoutYield();
				readpos = volatize(m_RingPos);
				//Console.WriteLn( Color_Blue, "(EEcore Awake) Report!\tringpos=0x%06x", readpos );
			} while( (readpos > m_WritePos) || (readpos <= thefuture) );
		}
		else
		{
			SetEvent();
			do {
				SpinWait();
				readpos = volatize(m_RingPos);
			} while( (readpos > m_WritePos) || (readpos <= thefuture) );
		}
	}

	PacketTagType& tag = (PacketTagType&)RingBuffer[m_WritePos];

	tag.command = GS_RINGTYPE_RESTART;

	m_RingWrapSpot = m_WritePos;
	m_WritePos = 0;
	m_QueuedFrameCount = 0;

	if( EmuConfig.GS.SynchronousMTGS )
		WaitGS();
}

__forceinline uint SysMtgsThread::_PrepForSimplePacket()
{
#ifdef RINGBUF_DEBUG_STACK
	m_lock_Stack.Lock();
	ringposStack.push_front( m_WritePos );
	m_lock_Stack.Release();
#endif

	uint future_writepos = m_WritePos+1;
	pxAssert( future_writepos <= RingBufferSize );

    future_writepos &= RingBufferMask;
    if( future_writepos == 0 )
    {
		m_QueuedFrameCount = 0;
		m_RingWrapSpot = RingBufferSize;
	}

	uint readpos = volatize(m_RingPos);
	if( future_writepos == readpos )
	{
		// The ringbuffer read pos is blocking the future write position, so stall out
		// until the read position has moved.

		uint totalAccum	= (m_RingWrapSpot - readpos) + future_writepos;
		uint somedone	= totalAccum / 4;

		if( somedone > 0x80 )
		{
			m_SignalRingPosition = somedone;

			//Console.WriteLn( Color_Blue, "(MTGS Sync) EEcore Simple Sleep!\t\twrapspot=0x%06x, ringpos=0x%06x, writepos=0x%06x, signalpos=0x%06x", m_RingWrapSpot, readpos, writepos, m_SignalRingPosition );

			do {
				AtomicExchange( m_SignalRingEnable, 1 );
				SetEvent();
				m_sem_OnRingReset.WaitWithoutYield();
				readpos = volatize(m_RingPos);
				//Console.WriteLn( Color_Blue, "(MTGS Sync) EEcore Simple Post-sleep Report!\tringpos=0x%06x", readpos );
			} while( future_writepos  == readpos );

			pxAssertDev( m_SignalRingPosition <= 0, "MTGS Thread Synchronization Error" );
		}
		else
		{
			SetEvent();
			do {
				SpinWait();
			} while( future_writepos == volatize(m_RingPos) );
		}
	}

	return future_writepos;
}

__forceinline void SysMtgsThread::_FinishSimplePacket( uint future_writepos )
{
	pxAssert( future_writepos != volatize(m_RingPos) );
	m_WritePos = future_writepos;

	if( EmuConfig.GS.SynchronousMTGS )
		WaitGS();
	else
		++m_CopyDataTally;
}

void SysMtgsThread::SendSimplePacket( MTGS_RingCommand type, int data0, int data1, int data2 )
{
	//ScopedLock locker( m_PacketLocker );

	const uint thefuture = _PrepForSimplePacket();
	PacketTagType& tag = (PacketTagType&)RingBuffer[m_WritePos];

	tag.command = type;
	tag.data[0] = data0;
	tag.data[1] = data1;
	tag.data[2] = data2;

	_FinishSimplePacket( thefuture );
}

void SysMtgsThread::SendPointerPacket( MTGS_RingCommand type, u32 data0, void* data1 )
{
	//ScopedLock locker( m_PacketLocker );

	const uint thefuture = _PrepForSimplePacket();
	PacketTagType& tag = (PacketTagType&)RingBuffer[m_WritePos];

	tag.command = type;
	tag.data[0] = data0;
	*(uptr*)&tag.data[1] = (uptr)data1;

	_FinishSimplePacket( thefuture );
}

void SysMtgsThread::SendGameCRC( u32 crc )
{
	SendSimplePacket( GS_RINGTYPE_CRC, crc, 0, 0 );
}

void SysMtgsThread::WaitForOpen()
{
	if( m_PluginOpened ) return;
	Resume();

	// Two-phase timeout on MTGS opening, so that possible errors are handled
	// in a timely fashion.  We check for errors after 2 seconds, and then give it
	// another 12 seconds if no errors occurred (this might seem long, but sometimes a
	// GS plugin can be very stubborned, especially in debug mode builds).

	if( !m_sem_OpenDone.Wait( wxTimeSpan(0, 0, 2, 0) ) )
	{
		RethrowException();

		if( !m_sem_OpenDone.Wait( wxTimeSpan(0, 0, 12, 0) ) )
		{
			RethrowException();

			// Not opened yet, and no exceptions.  Weird?  You decide!
			// TODO : implement a user confirmation to cancel the action and exit the
			//   emulator forcefully, or to continue waiting on the GS.

			throw Exception::PluginOpenError( PluginId_GS, "The MTGS thread has become unresponsive while waiting for the GS plugin to open." );
		}
	}

	RethrowException();
}

void SysMtgsThread::Freeze( int mode, MTGS_FreezeData& data )
{
	GetCorePlugins().Open( PluginId_GS );
	SendPointerPacket( GS_RINGTYPE_FREEZE, mode, &data );
	Resume();
	WaitGS();
}
