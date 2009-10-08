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

#include "PrecompiledHeader.h"
#include "Common.h"
#include "GS.h"

#include "VU.h"
#include "iR5900.h"
#include "VifDma.h"

#include "SamplProf.h"

#include <list>
#include <wx/datetime.h>

// Uncomment this to enable profiling of the GS RingBufferCopy function.
//#define PCSX2_GSRING_SAMPLING_STATS

using namespace Threading;

#ifdef DEBUG
#define MTGS_LOG Console.WriteLn
#else
#define MTGS_LOG 0&&
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

__aligned16 mtgsThreadObject mtgsThread;

struct MTGS_BufferedData
{
	u128		m_Ring[RingBufferSize];
	u8			Regs[Ps2MemSize::GSregs];

	MTGS_BufferedData() {}

	u128& operator[]( uint idx )
	{
		jASSUME( idx < RingBufferSize );
		return m_Ring[idx];
	}
};

static __aligned(32) MTGS_BufferedData RingBuffer;
extern bool renderswitch;
static volatile bool gsIsOpened = false;


#ifdef RINGBUF_DEBUG_STACK
#include <list>
std::list<uint> ringposStack;
#endif

mtgsThreadObject::mtgsThreadObject() :
	SysSuspendableThread()
,	m_RingPos( 0 )
,	m_WritePos( 0 )

,	m_lock_RingRestart()
,	m_PacketLocker()

,	m_CopyCommandTally( 0 )
,	m_CopyDataTally( 0 )
,	m_RingBufferIsBusy( false )
,	m_QueuedFrames( 0 )
,	m_lock_FrameQueueCounter()
,	m_packet_size( 0 )
,	m_packet_ringpos( 0 )

#ifdef RINGBUF_DEBUG_STACK
,	m_lock_Stack()
#endif
{
	m_name = L"MTGS";
}

void mtgsThreadObject::Start()
{
	_parent::Start();
	m_ExecMode = ExecMode_Suspending;
	SetEvent();
}

void mtgsThreadObject::OnStart()
{
	gsIsOpened		= false;

	m_RingPos		= 0;
	m_WritePos		= 0;

	m_RingBufferIsBusy	= false;

	m_QueuedFrames		= 0;
	m_packet_size		= 0;
	m_packet_ringpos	= 0;

	_parent::OnStart();
}

void mtgsThreadObject::PollStatus()
{
	RethrowException();
}

mtgsThreadObject::~mtgsThreadObject() throw()
{
	_parent::Cancel();
}

void mtgsThreadObject::OnResumeReady()
{
	m_sem_OpenDone.Reset();
}

void mtgsThreadObject::ResetGS()
{
	// MTGS Reset process:
	//  * clear the ringbuffer.
	//  * Signal a reset.
	//  * clear the path and byRegs structs (used by GIFtagDummy)

	AtomicExchange( m_RingPos, m_WritePos );

	MTGS_LOG( "MTGS: Sending Reset..." );
	SendSimplePacket( GS_RINGTYPE_RESET, 0, 0, 0 );
	SendSimplePacket( GS_RINGTYPE_FRAMESKIP, 0, 0, 0 );
	SetEvent();

	GIFPath_Reset();
}

void mtgsThreadObject::PostVsyncEnd( bool updategs )
{
	while( m_QueuedFrames > 8 )
	{
		if( m_WritePos == volatize( m_RingPos ) )
		{
			// MTGS ringbuffer is empty, but we still have queued frames in the counter?  Ouch!
			Console.Error( "MTGS > Queued framecount mismatch = %d", m_QueuedFrames );
			m_QueuedFrames = 0;
			break;
		}
		Threading::Sleep( 2 );		// Sleep off quite a bit of time, since we're obviously *waaay* ahead.
		SpinWait();
	}

	m_lock_FrameQueueCounter.Lock();
	m_QueuedFrames++;
	//Console.Status( " >> Frame Added!" );
	m_lock_FrameQueueCounter.Unlock();

	SendSimplePacket( GS_RINGTYPE_VSYNC,
		(*(u32*)(PS2MEM_GS+0x1000)&0x2000), updategs, 0);

	// No need to freeze MMX/XMM registers here since this
	// code is always called from the context of a BranchTest.
	SetEvent();
}

struct PacketTagType
{
	u32 command;
	u32 data[3];
};

static void _clean_close_gs( void* obj )
{
	if( !gsIsOpened ) return;
	gsIsOpened = false;
	if( g_plugins != NULL )
		g_plugins->m_info[PluginId_GS].CommonBindings.Close();
}

static void dummyIrqCallback()
{
	// dummy, because MTGS doesn't need this mess!
	// (and zerogs does >_<)
}

void mtgsThreadObject::OpenPlugin()
{
	if( gsIsOpened ) return;

	memcpy_aligned( RingBuffer.Regs, PS2MEM_GS, sizeof(PS2MEM_GS) );
	GSsetBaseMem( RingBuffer.Regs );
	GSirqCallback( dummyIrqCallback );

	if( renderswitch )
		Console.WriteLn( "\t\tForced software switch enabled." );

	int result;

	if( GSopen2 != NULL )
		result = GSopen2( (void*)&pDsp, 1 | (renderswitch ? 4 : 0) );
	else
		result = GSopen( (void*)&pDsp, "PCSX2", renderswitch ? 2 : 1 );

	if( result != 0 )
	{
		DevCon.WriteLn( "GSopen Failed: return code: 0x%x", result );
		throw Exception::PluginOpenError( PluginId_GS );
	}

	gsIsOpened = true;
	m_sem_OpenDone.Post();

	GSCSRr = 0x551B4000; // 0x55190000
	GSsetGameCRC( ElfCRC, 0 );
}

void mtgsThreadObject::ExecuteTask()
{
#ifdef RINGBUF_DEBUG_STACK
	PacketTagType prevCmd;
#endif

	pthread_cleanup_push( _clean_close_gs, this );
	while( true )
	{
		m_sem_event.Wait();		// ... because this does a cancel test itself..
		StateCheck( false );	// false disables cancel test here!

		m_RingBufferIsBusy = true;

		// note: m_RingPos is intentionally not volatile, because it should only
		// ever be modified by this thread.
		while( m_RingPos != volatize(m_WritePos))
		{
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
			m_lock_Stack.Unlock();
#endif

			switch( tag.command )
			{
				case GS_RINGTYPE_RESTART:
					AtomicExchange(m_RingPos, 0);

					// stall for a bit to let the MainThread have time to update the g_pGSWritePos.
					m_lock_RingRestart.Lock();
					m_lock_RingRestart.Unlock();

					StateCheck( false );		// disable cancel since the above locks are cancelable already
				continue;

				case GS_RINGTYPE_P1:
				{
					const int qsize = tag.data[0];
					const u128* data = &RingBuffer[m_RingPos+1];

					// make sure that tag>>16 is the MAX size readable
					GSgifTransfer1((u32*)(data - 0x400 + qsize), 0x4000-qsize*16);
					//GSgifTransfer1((u32*)data, qsize);
					ringposinc += qsize;
				}
				break;

				case GS_RINGTYPE_P2:
				{
					const int qsize = tag.data[0];
					const u128* data = &RingBuffer[m_RingPos+1];
					GSgifTransfer2((u32*)data, qsize);
					ringposinc += qsize;
				}
				break;

				case GS_RINGTYPE_P3:
				{
					const int qsize = tag.data[0];
					const u128* data = &RingBuffer[m_RingPos+1];
					GSgifTransfer3((u32*)data, qsize);
					ringposinc += qsize;
				}
				break;

				case GS_RINGTYPE_VSYNC:
				{
					GSvsync(tag.data[0]);
					gsFrameSkip( !tag.data[1] );

					m_lock_FrameQueueCounter.Lock();
					AtomicDecrement( m_QueuedFrames );
					jASSUME( m_QueuedFrames >= 0 );
					//Console.Status( " << Frame Removed!" );
					m_lock_FrameQueueCounter.Unlock();

					if( PADupdate != NULL )
					{
						PADupdate(0);
						PADupdate(1);
					}
				}
				break;

				case GS_RINGTYPE_FRAMESKIP:
					_gs_ResetFrameskip();
				break;

				case GS_RINGTYPE_MEMWRITE8:
					RingBuffer.Regs[tag.data[0]] = (u8)tag.data[1];
				break;
				case GS_RINGTYPE_MEMWRITE16:
					*(u16*)(RingBuffer.Regs+tag.data[0]) = (u16)tag.data[1];
				break;
				case GS_RINGTYPE_MEMWRITE32:
					*(u32*)(RingBuffer.Regs+tag.data[0]) = tag.data[1];
				break;
				case GS_RINGTYPE_MEMWRITE64:
					*(u64*)(RingBuffer.Regs+tag.data[0]) = *(u64*)&tag.data[1];
				break;

				case GS_RINGTYPE_FREEZE:
				{
					MTGS_FreezeData* data = (MTGS_FreezeData*)(*(uptr*)&tag.data[1]);
					int mode = tag.data[0];
					data->retval = GetPluginManager().DoFreeze( PluginId_GS, mode, data->fdata );
					break;
				}

				case GS_RINGTYPE_RECORD:
				{
					int record = tag.data[0];
					if( GSsetupRecording != NULL ) GSsetupRecording(record, NULL);
					if( SPU2setupRecording != NULL ) SPU2setupRecording(record, NULL);
					break;
				}

				case GS_RINGTYPE_RESET:
					MTGS_LOG( "MTGS: Receiving Reset..." );
					if( GSreset != NULL ) GSreset();
					break;

				case GS_RINGTYPE_SOFTRESET:
				{
					int mask = tag.data[0];
					MTGS_LOG( "MTGS: Receiving GIF Soft Reset (mask: %d)", mask );
					GSgifSoftReset( mask );
					break;
				}

				case GS_RINGTYPE_WRITECSR:
					GSwriteCSR( tag.data[0] );
				break;

				case GS_RINGTYPE_MODECHANGE:
					_gs_ChangeTimings( tag.data[0], tag.data[1] );
				break;
				
				case GS_RINGTYPE_CRC:
					GSsetGameCRC( tag.data[0], 0 );
				break;

				case GS_RINGTYPE_STARTTIME:
					m_iSlowStart += tag.data[0];
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
			newringpos &= RingBufferMask;
			AtomicExchange( m_RingPos, newringpos );
		}
		m_RingBufferIsBusy = false;
	}
	pthread_cleanup_pop( true );
}

void mtgsThreadObject::OnSuspendInThread()
{
	_clean_close_gs( NULL );
}

void mtgsThreadObject::OnResumeInThread( bool isSuspended )
{
	if( isSuspended )
		OpenPlugin();
}

// Waits for the GS to empty out the entire ring buffer contents.
// Used primarily for plugin startup/shutdown.
void mtgsThreadObject::WaitGS()
{
	pxAssertDev( !IsSelf(), "This method is only allowed from threads *not* named MTGS." );

	if( IsSuspended() ) return;

	// FIXME : Use semaphores instead of spinwaits.
	SetEvent();
	while( volatize(m_RingPos) != volatize(m_WritePos) ) Timeslice();
}

// Sets the gsEvent flag and releases a timeslice.
// For use in loops that wait on the GS thread to do certain things.
void mtgsThreadObject::SetEvent()
{
	m_sem_event.Post();
	m_CopyCommandTally = 0;
	m_CopyDataTally = 0;
}

void mtgsThreadObject::PrepEventWait()
{
	//Console.Notice( "MTGS Stall!  EE waits for nothing! ... except your GPU sometimes." );
	SetEvent();
	Timeslice();
}

void mtgsThreadObject::PostEventWait() const
{
}

u8* mtgsThreadObject::GetDataPacketPtr() const
{
	return (u8*)&RingBuffer[m_packet_ringpos];
}

// Closes the data packet send command, and initiates the gs thread (if needed).
void mtgsThreadObject::SendDataPacket()
{
	// make sure a previous copy block has been started somewhere.
	jASSUME( m_packet_size != 0 );

	uint temp = m_packet_ringpos + m_packet_size;
	jASSUME( temp <= RingBufferSize );
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
					assert( temp < readpos );
			}

			// Updating the writepos should never make it equal the readpos, since
			// that would stop the buffer prematurely (and indicates bad code in the
			// ringbuffer manager)
			assert( readpos != temp );
		}
	}

	AtomicExchange( m_WritePos, temp );

	m_packet_size = 0;

	if( !m_RingBufferIsBusy )
	{
		// The ringbuffer is current in a resting state, so if enough copies have
		// queued up then go ahead and initiate the GS thread..

		// Optimization notes:  What we're doing here is initiating a "burst" mode on
		// the thread, which improves its cache hit performance and makes it more friendly
		// to other threads in Pcsx2 and such.  Primary is the Command Tally, and then a
		// secondary data size threshold for games that do lots of texture swizzling.

		// 16 was the best value I found so far.
		// tested values:
		//  24 - very slow on HT machines (+5% drop in fps)
		//  8 - roughly 2% slower on HT machines.

		m_CopyDataTally += m_packet_size;
		if( ( m_CopyDataTally > 0x8000 ) || ( ++m_CopyCommandTally > 16 ) )
		{
			//Console.Status( "MTGS Kick! DataSize : 0x%5.8x, CommandTally : %d", m_CopyDataTally, m_CopyCommandTally );
			SetEvent();
		}
	}
	//m_PacketLocker.Unlock();
}

int mtgsThreadObject::PrepDataPacket( GIF_PATH pathidx, const u32* srcdata, u32 size )
{
	return PrepDataPacket( pathidx, (u8*)srcdata, size );
}

#ifdef PCSX2_GSRING_TX_STATS
static u32 ringtx_s=0;
static u32 ringtx_s_ulg=0;
static u32 ringtx_s_min=0xFFFFFFFF;
static u32 ringtx_s_max=0;
static u32 ringtx_c=0;
static u32 ringtx_inf[32][32];
static u32 ringtx_inf_s[32];
#endif

// returns the amount of giftag data not processed (in simd128 values).
// Return value is used by VU1 XGKICK to hack-fix data packets which are too
// large for VU1 memory.
// Parameters:
//  size - size of the packet data, in smd128's
int mtgsThreadObject::PrepDataPacket( GIF_PATH pathidx, const u8* srcdata, u32 size )
{
	//m_PacketLocker.Lock();

#ifdef PCSX2_GSRING_TX_STATS
	ringtx_s += size;
	ringtx_s_ulg += size&0x7F;
	ringtx_s_min = min(ringtx_s_min,size);
	ringtx_s_max = max(ringtx_s_max,size);
	ringtx_c++;
	u32 tx_sz;

	if (_BitScanReverse(&tx_sz,size))
	{
		u32 tx_algn;
		_BitScanForward(&tx_algn,size);
		ringtx_inf[tx_sz][tx_algn]++;
		ringtx_inf_s[tx_sz]+=size;
	}
	if (ringtx_s>=128*1024*1024)
	{
		Console.Status("GSRingBufCopy:128MB in %d tx -> b/tx: AVG = %.2f , max = %d, min = %d",ringtx_c,ringtx_s/(float)ringtx_c,ringtx_s_max,ringtx_s_min);
		for (int i=0;i<32;i++)
		{
			u32 total_bucket=0;
			u32 bucket_subitems=0;
			for (int j=0;j<32;j++)
			{
				if (ringtx_inf[i][j])
				{
					total_bucket+=ringtx_inf[i][j];
					bucket_subitems++;
					Console.Notice("GSRingBufCopy :tx [%d,%d] algn %d : count= %d [%.2f%%]",1<<i,(1<<(i+1))-16,1<<j,ringtx_inf[i][j],ringtx_inf[i][j]/(float)ringtx_c*100);
					ringtx_inf[i][j]=0;
				}
			}
			if (total_bucket)
				Console.Notice("GSRingBufCopy :tx [%d,%d] total : count= %d [%.2f%%] [%.2f%%]",1<<i,(1<<(i+1))-16,total_bucket,total_bucket/(float)ringtx_c*100,ringtx_inf_s[i]/(float)ringtx_s*100);
			ringtx_inf_s[i]=0;
		}
		Console.Notice("GSRingBufCopy :tx ulg count =%d [%.2f%%]",ringtx_s_ulg,ringtx_s_ulg/(float)ringtx_s*100);
		ringtx_s_ulg=0;
		ringtx_c=0;
		ringtx_s=0;
		ringtx_s_min=0xFFFFFFFF;
		ringtx_s_max=0;
	}
#endif
	// Note on volatiles: g_pGSWritePos is not modified by the GS thread,
	// so there's no need to use volatile reads here.  We still have to use
	// interlocked exchanges when we modify it, however, since the GS thread
	// is reading it.

	uint writepos = m_WritePos;

	// Checks if a previous copy was started without an accompanying call to GSRINGBUF_DONECOPY
	jASSUME( m_packet_size == 0 );

	// Sanity checks! (within the confines of our ringbuffer please!)
	jASSUME( size < RingBufferSize );
	jASSUME( writepos < RingBufferSize );

	m_packet_size = GIFPath_ParseTag(pathidx, srcdata, size);
	size		  = m_packet_size + 1; // takes into account our command qword.

	if( writepos + size < RingBufferSize )
	{
		// generic gs wait/stall.
		// if the writepos is past the readpos then we're safe.
		// But if not then we need to make sure the readpos is outside the scope of
		// the block about to be written (writepos + size)

		if( writepos < volatize(m_RingPos) )
		{
			// writepos is behind the readpos, so we need to wait until
			// readpos is out past the end of the future write pos, or until it wraps
			// around (in which case writepos will be >= readpos)

			PrepEventWait();
			while( true )
			{
				uint readpos = volatize(m_RingPos);
				if( writepos >= readpos ) break;
				if( writepos+size < readpos ) break;
				SpinWait();
			}
			PostEventWait();
		}
	}
	else if( writepos + size > RingBufferSize )
	{
		// If the incoming packet doesn't fit, then start over from
		// the start of the ring buffer (it's a lot easier than trying
		// to wrap the packet around the end of the buffer).

		// We have to be careful not to leapfrog our read-position.  If it's
		// greater than the current write position then we need to stall
		// until it loops around to the beginning of the buffer

		PrepEventWait();
		while( true )
		{
			uint readpos = volatize(m_RingPos);

			// is the buffer empty?
			if( readpos == writepos ) break;

			// Also: Wait for the readpos to go past the start of the buffer
			// Otherwise it'll stop dead in its tracks when we set the new write
			// position below (bad!)
			if( readpos < writepos && readpos != 0 ) break;

			SpinWait();
		}

		m_lock_RingRestart.Lock();
		SendSimplePacket( GS_RINGTYPE_RESTART, 0, 0, 0 );
		writepos = 0;
		AtomicExchange( m_WritePos, writepos );
		m_lock_RingRestart.Unlock();
		SetEvent();

		// stall until the read position is past the end of our incoming block,
		// or until it reaches the current write position (signals an empty buffer).
		while( true )
		{
			uint readpos = volatize(m_RingPos);

			if( readpos == m_WritePos ) break;
			if( writepos+size < readpos ) break;

			SpinWait();
		}
		PostEventWait();
	}
    else	// always true - if( writepos + size == MTGS_RINGBUFFEREND )
	{
		// Yay.  Perfect fit.  What are the odds?
		//Console.WriteLn( "MTGS > Perfect Fit!");

		PrepEventWait();
		while( true )
		{
			uint readpos = volatize(m_RingPos);

			// stop waiting if the buffer is empty!
			if( writepos == readpos ) break;

			// Copy is ready so long as readpos is less than writepos and *not*
			// equal to the base of the ringbuffer (otherwise the buffer will stop
			// when the writepos is wrapped around to zero later-on in SendDataPacket)
			if( readpos < writepos && readpos != 0 ) break;

			SpinWait();
		}
		PostEventWait();
    }

#ifdef RINGBUF_DEBUG_STACK
	m_lock_Stack.Lock();
	ringposStack.push_front( writepos );
	m_lock_Stack.Unlock();
#endif

	// Command qword: Low word is the command, and the high word is the packet
	// length in SIMDs (128 bits).

	PacketTagType& tag = (PacketTagType&)RingBuffer[m_WritePos];
	tag.command = pathidx+1;
	tag.data[0] = m_packet_size;
	m_packet_ringpos = m_WritePos + 1;

	return m_packet_size;
}

__forceinline uint mtgsThreadObject::_PrepForSimplePacket()
{
#ifdef RINGBUF_DEBUG_STACK
	m_lock_Stack.Lock();
	ringposStack.push_front( m_WritePos );
	m_lock_Stack.Unlock();
#endif

	uint future_writepos = m_WritePos+1;
	jASSUME( future_writepos <= RingBufferSize );

    future_writepos &= RingBufferMask;

	if( future_writepos == volatize(m_RingPos) )
	{
		PrepEventWait();
		do
		{
			SpinWait();
		} while( future_writepos == volatize(m_RingPos) );
		PostEventWait();
	}

	return future_writepos;
}

__forceinline void mtgsThreadObject::_FinishSimplePacket( uint future_writepos )
{
	assert( future_writepos != volatize(m_RingPos) );
	AtomicExchange( m_WritePos, future_writepos );
}

void mtgsThreadObject::SendSimplePacket( GS_RINGTYPE type, int data0, int data1, int data2 )
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

void mtgsThreadObject::SendPointerPacket( GS_RINGTYPE type, u32 data0, void* data1 )
{
	//ScopedLock locker( m_PacketLocker );

	const uint thefuture = _PrepForSimplePacket();
	PacketTagType& tag = (PacketTagType&)RingBuffer[m_WritePos];

	tag.command = type;
	tag.data[0] = data0;
	*(uptr*)&tag.data[1] = (uptr)data1;

	_FinishSimplePacket( thefuture );
}

void mtgsThreadObject::SendGameCRC( u32 crc )
{
	SendSimplePacket( GS_RINGTYPE_CRC, crc, 0, 0 );
}

void mtgsThreadObject::WaitForOpen()
{
	if( !gsIsOpened )
		m_sem_OpenDone.WaitGui();
	m_sem_OpenDone.Reset();
}

void mtgsThreadObject::Freeze( int mode, MTGS_FreezeData& data )
{
	if( mode == FREEZE_LOAD )
	{
		AtomicExchange( m_RingPos, m_WritePos );
		SendPointerPacket( GS_RINGTYPE_FREEZE, mode, &data );
		SetEvent();
		Resume();
	}
	else
		SendPointerPacket( GS_RINGTYPE_FREEZE, mode, &data );

	mtgsWaitGS();
}

// Waits for the GS to empty out the entire ring buffer contents.
// Used primarily for plugin startup/shutdown.
void mtgsWaitGS()
{
	mtgsThread.WaitGS();
}

