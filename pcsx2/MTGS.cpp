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

#include <list>

#include <wx/datetime.h>

#include "Common.h"
#include "VU.h"
#include "GS.h"
#include "iR5900.h"
#include "VifDma.h"

#include "SamplProf.h"

// Uncomment this to enable profiling of the GS RingBufferCopy function.
//#define PCSX2_GSRING_SAMPLING_STATS

using namespace Threading;
using namespace std;

#ifdef DEBUG
#define MTGS_LOG Console::WriteLn
#else
#define MTGS_LOG 0&&
#endif

// forces the compiler to treat a non-volatile value as volatile.
// This allows us to declare the vars as non-volatile and only use
// them as volatile when appropriate (more optimized).

#define volatize(x) (*reinterpret_cast<volatile uint*>(&(x)))

/////////////////////////////////////////////////////////////////////////////
//   BEGIN  --  MTGS GIFtag Parse Implementation
//
// The MTGS needs a dummy "GS plugin" for processing SIGNAL, FINISH, and LABEL
// commands.  These commands trigger gsIRQs, which need to be handled accurately
// in synch with the EE (which can be running several frames ahead of the MTGS)
//
// Yeah, it's a lot of work, but the performance gains are huge, even on HT cpus.

// unpack the registers
// registers are stored as a sequence of 4 bit values in the
// upper 64 bits of the GIFTAG.  That sucks for us, so we unpack
// them into an 8 bit array.
__forceinline void GIFPath::PrepRegs(bool doPrep = 1)
{
	if (!doPrep) return;
	int loopEnd = ((tag.nreg-1)&0xf) + 1;
	u32 tempreg = tag.regs[0];

	for (int i = 0; i < loopEnd; i++) {
		if (i == 8) tempreg = tag.regs[1];
		regs[i] = tempreg & 0xf;
		tempreg >>= 4;
	}
}

__forceinline void GIFPath::SetTag(const void* mem)
{
	tag		= *((GIFTAG*)mem);
	curreg	= 0;
}

static void _mtgsFreezeGIF( SaveStateBase& state, GIFPath (&paths)[3] )
{
	for(int i=0; i<3; i++ )
	{
		state.Freeze( paths[i].tag );
		state.Freeze( paths[i].curreg );
	}

	for(int i=0; i<3; i++ )
	{
		state.Freeze( paths[i].regs );
	}
}

void SaveStateBase::mtgsFreeze()
{
	FreezeTag( "mtgs" );
	mtgsThread->Freeze( *this );
}


static void RegHandlerSIGNAL(const u32* data)
{
	MTGS_LOG("MTGS SIGNAL data %x_%x CSRw %x IMR %x CSRr\n",data[0], data[1], CSRw, GSIMR, GSCSRr);

	GSSIGLBLID->SIGID = (GSSIGLBLID->SIGID&~data[1])|(data[0]&data[1]);

	if ((CSRw & 0x1))
	{
		if (!(GSIMR&0x100) )
		{
			gsIrq();
		}

		GSCSRr |= 1; // signal
	}
}

static void RegHandlerFINISH(const u32* data)
{
	MTGS_LOG("MTGS FINISH data %x_%x CSRw %x\n", data[0], data[1], CSRw);

	if ((CSRw & 0x2))
	{
		if (!(GSIMR&0x200))
			gsIrq();

		GSCSRr |= 2; // finish
	}
}

static void RegHandlerLABEL(const u32* data)
{
	GSSIGLBLID->LBLID = (GSSIGLBLID->LBLID&~data[1])|(data[0]&data[1]);
}

//  END  --  MTGS GIFtag Parse Implementation
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  MTGS Threaded Class Implementation

mtgsThreadObject* mtgsThread = NULL;

#ifdef RINGBUF_DEBUG_STACK
#include <list>
std::list<uint> ringposStack;
#endif

#ifdef PCSX2_DEBUG
// debug variable used to check for bad code bits where copies are started
// but never closed, or closed without having been started.  (GSRingBufCopy calls
// should always be followed by a call to GSRINGBUF_DONECOPY)
// And it's not even used in the debug code.
//static int copyLock = 0;
#endif

typedef void (*GIFRegHandler)(const u32* data);
static GIFRegHandler s_GSHandlers[3] = { RegHandlerSIGNAL, RegHandlerFINISH, RegHandlerLABEL };

mtgsThreadObject::mtgsThreadObject() :
	PersistentThread()
,	m_RingPos( 0 )
,	m_WritePos( 0 )

,	m_sem_InitDone()
,	m_lock_RingRestart()
,	m_PacketLocker( true )		// true - makes it a recursive lock

,	m_CopyCommandTally( 0 )
,	m_CopyDataTally( 0 )
,	m_RingBufferIsBusy( 0 )
,	m_QueuedFrames( 0 )
,	m_lock_FrameQueueCounter()
,	m_packet_size( 0 )
,	m_packet_ringpos( 0 )

#ifdef RINGBUF_DEBUG_STACK
,	m_lock_Stack()
#endif
,	m_RingBuffer( m_RingBufferSize + (Ps2MemSize::GSregs/sizeof(u128)) )
,	m_gsMem( (u8*)m_RingBuffer.GetPtr( m_RingBufferSize ) )
{
	memzero_obj( m_path );
}

void mtgsThreadObject::Start()
{
	m_sem_InitDone.Reset();
	PersistentThread::Start();

	// Wait for the thread to finish initialization (it runs GSopen, which can take
	// some time since it's creating a new window and all), and then check for errors.

	m_sem_InitDone.WaitGui();

	if( m_returncode != 0 )	// means the thread failed to init the GS plugin
		throw Exception::PluginOpenError( PluginId_GS );
}

mtgsThreadObject::~mtgsThreadObject() throw()
{
	mtgsThreadObject::Cancel();
}

// Closes the GS "forcefully" without waiting for it to finish rendering it's pending
// queue of GS data.
void mtgsThreadObject::Cancel()
{
	//SendSimplePacket( GS_RINGTYPE_QUIT, 0, 0, 0 );
	//SetEvent();
	//m_sem_finished.WaitGui();
	PersistentThread::Cancel();
}

void mtgsThreadObject::Reset()
{
	// MTGS Reset process:
	//  * clear the ringbuffer.
	//  * Signal a reset.
	//  * clear the path and byRegs structs (used by GIFtagDummy)

	AtomicExchange( m_RingPos, m_WritePos );

	MTGS_LOG( "MTGS: Sending Reset..." );
	SendSimplePacket( GS_RINGTYPE_RESET, 0, 0, 0 );
	SendSimplePacket( GS_RINGTYPE_FRAMESKIP, 0, 0, 0 );

	memzero_obj( m_path );
}

#define incPmem(x) {											 \
	pMem += (x) * 16;											 \
	size += (x) * 16;											 \
	if ((pathidx==GIF_PATH_1)&&(pMem>=vuMemEnd)) pMem -= 0x4000; \
}
#define subVal(x) ((x) ? (x-1) : 0)

__forceinline int mtgsThreadObject::_gifTransferDummy(GIF_PATH pathidx, const u8* pMem, u32 size)
{
	GIFPath& path		= m_path[pathidx];
	u32 finish			= (pathidx == GIF_PATH_1) ? 0x4000 : (size<<4);
	u32 oldSize			= 0;
	u32 numRegs			= 0;
	const u8* vuMemEnd	= pMem + (size<<4);	// End of VU Mem
	bool pathContinue	= !!path.tag.nloop;	// Continue From Last Transfer
	bool EOP			= 0;
	bool hasRegAD		= 0;
	size				= 0;

	while(!EOP && size < finish) {

		if(!pathContinue) {
			path.SetTag(pMem);
			path.PrepRegs(!path.tag.flg);
			incPmem(1);
		}
		pathContinue = 0;
		oldSize		 = size;
		numRegs		 = ((path.tag.nreg-1)&0xf)+1;
		EOP			 = ((pathidx == GIF_PATH_2) ? 0 : path.tag.eop);

		if (!path.tag.nloop || size >= finish) continue;

		switch(path.tag.flg) {
			case GIF_FLG_PACKED:
				for (u32 i = 0; i < path.tag.nloop; i++) {
					for (u32 j = path.curreg; j < numRegs; j++) {
						if (path.regs[j] == 0x0e) {
							const int handler = pMem[8];
							if (handler >= 0x60 && handler < 0x63) {
								//DevCon::Status("GIF Tag Interrupt");
								s_GSHandlers[handler&0x3]((const u32*)pMem);
							}
							hasRegAD = 1;
						}
						incPmem(1);
						path.curreg = 0;
						if (size >= finish) goto endLoop;
					}
					if (!hasRegAD) { // Optimization: No Need to Loop
						incPmem(numRegs * subVal(path.tag.nloop));
						break;
					}
				}
				break;
			case GIF_FLG_REGLIST:
				numRegs = (numRegs + 1) / 2;
				incPmem((numRegs - path.curreg) * path.tag.nloop);
				incPmem((numRegs * subVal(path.tag.nloop)));
				break;
			case GIF_FLG_IMAGE:
			case GIF_FLG_IMAGE2:
				incPmem(path.tag.nloop);
				break;
		}
	}
endLoop:

	// This handles cases where we skipped too far ahead because of bulky IMAGE/REGLIST tags
	// or PACKED tags w/o RegA+D (they don't compare 'size < finish' every qwc).
	if (size > finish) size = finish;

	// Sets information to resume partial transfers
	if (!EOP && path.tag.nloop && pathidx) {
		int diff = (size - oldSize) / 16;
		if (!(path.tag.flg&2)) { // PACKED/REGLIST
			path.curreg		= (diff % path.tag.nloop);
			path.tag.nloop -= (diff / numRegs);
		}
		else path.tag.nloop -= diff;
	}
	else {
		path.curreg	   = 0;
		path.tag.nloop = 0;
	}

	if (pathidx == GIF_PATH_3) {
		gif->madr +=  size;
		gif->qwc  -= (size/16);
		if (EOP)				 Path3progress = STOPPED_MODE;
		else if (path.tag.flg&2) Path3progress = IMAGE_MODE;
		else					 Path3progress = TRANSFER_MODE;
	}

	return (size / 16);
}

// Processes a GIFtag & packet, and throws out some gsIRQs as needed.
// Used to keep interrupts in sync with the EE, while the GS itself
// runs potentially several frames behind.
__forceinline int mtgsThreadObject::gifTransferDummy(GIF_PATH pathidx, const u8* pMem, u32 size)
{
#ifdef PCSX2_GSRING_SAMPLING_STATS
	static uptr profStartPtr = 0;
	static uptr profEndPtr = 0;
	if (profStartPtr == 0) {
		__asm
		{
	__beginfunc:
			mov profStartPtr, offset __beginfunc;
			mov profEndPtr, offset __endfunc;
		}
		ProfilerRegisterSource( "GSRingBufCopy", (void*)profStartPtr, profEndPtr - profStartPtr );
	}
#endif

	int retSize = _gifTransferDummy(pathidx, pMem, size);

#ifdef PCSX2_GSRING_SAMPLING_STATS
	__asm
	{
		__endfunc:
				nop;
	}
#endif
	return retSize;
}

void mtgsThreadObject::PostVsyncEnd( bool updategs )
{
	while( m_QueuedFrames > 8 )
	{
		if( m_WritePos == volatize( m_RingPos ) )
		{
			// MTGS ringbuffer is empty, but we still have queued frames in the counter?  Ouch!
			Console::Error( "MTGS > Queued framecount mismatch = %d", m_QueuedFrames );
			m_QueuedFrames = 0;
			break;
		}
		Threading::Sleep( 2 );		// Sleep off quite a bit of time, since we're obviously *waaay* ahead.
		SpinWait();
	}

	m_lock_FrameQueueCounter.Lock();
	m_QueuedFrames++;
	//Console::Status( " >> Frame Added!" );
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

extern bool renderswitch;

static void _clean_close_gs( void* obj )
{
	if( g_plugins != NULL )
		g_plugins->m_info[PluginId_GS].CommonBindings.Close();
}

void mtgsThreadObject::_RingbufferLoop()
{
	pthread_cleanup_push( _clean_close_gs, this );
	while( true )
	{
		m_sem_event.Wait();

		AtomicExchange( m_RingBufferIsBusy, 1 );

		// note: m_RingPos is intentionally not volatile, because it should only
		// ever be modified by this thread.
		while( m_RingPos != volatize(m_WritePos))
		{
			wxASSERT( m_RingPos < m_RingBufferSize );

			const PacketTagType& tag = (PacketTagType&)m_RingBuffer[m_RingPos];
			u32 ringposinc = 1;

#ifdef RINGBUF_DEBUG_STACK
			// pop a ringpos off the stack.  It should match this one!

			m_lock_Stack.Lock();
			uptr stackpos = ringposStack.back();
			if( stackpos != m_RingPos )
			{
				Console::Error( "MTGS Ringbuffer Critical Failure ---> %x to %x (prevCmd: %x)\n", stackpos, m_RingPos, prevCmd.command );
			}
			wxASSERT( stackpos == m_RingPos );
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
				continue;

				case GS_RINGTYPE_P1:
				{
					const int qsize = tag.data[0];
					const u128* data = m_RingBuffer.GetPtr( m_RingPos+1 );

					// make sure that tag>>16 is the MAX size readable
					GSgifTransfer1((u32*)(data - 0x400 + qsize), 0x4000-qsize*16);
					//GSgifTransfer1((u32*)data, qsize);
					ringposinc += qsize;
				}
				break;

				case GS_RINGTYPE_P2:
				{
					const int qsize = tag.data[0];
					const u128* data = m_RingBuffer.GetPtr( m_RingPos+1 );
					GSgifTransfer2((u32*)data, qsize);
					ringposinc += qsize;
				}
				break;

				case GS_RINGTYPE_P3:
				{
					const int qsize = tag.data[0];
					const u128* data = m_RingBuffer.GetPtr( m_RingPos+1 );
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
					//Console::Status( " << Frame Removed!" );
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
					m_gsMem[tag.data[0]] = (u8)tag.data[1];
				break;
				case GS_RINGTYPE_MEMWRITE16:
					*(u16*)(m_gsMem+tag.data[0]) = (u16)tag.data[1];
				break;
				case GS_RINGTYPE_MEMWRITE32:
					*(u32*)(m_gsMem+tag.data[0]) = tag.data[1];
				break;
				case GS_RINGTYPE_MEMWRITE64:
					*(u64*)(m_gsMem+tag.data[0]) = *(u64*)&tag.data[1];
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

				case GS_RINGTYPE_STARTTIME:
					m_iSlowStart += tag.data[0];
				break;

				case GS_RINGTYPE_QUIT:
					// have to use some low level code, because all the standard Close api does is
					// trigger this very ringbuffer message!
				return;

#ifdef PCSX2_DEVBUILD
				default:
					Console::Error("GSThreadProc, bad packet (%x) at m_RingPos: %x, m_WritePos: %x", tag.command, m_RingPos, m_WritePos);
					wxASSERT_MSG( false, L"Bad packet encountered in the MTGS Ringbuffer." );
					m_RingPos = m_WritePos;
					continue;
#else
				// Optimized performance in non-Dev builds.
				jNO_DEFAULT;
#endif
			}

			uint newringpos = m_RingPos + ringposinc;
			wxASSERT( newringpos <= m_RingBufferSize );
			newringpos &= m_RingBufferMask;
			AtomicExchange( m_RingPos, newringpos );
		}
		AtomicExchange( m_RingBufferIsBusy, 0 );
	}
	pthread_cleanup_pop( true );
}

static void dummyIrqCallback()
{
	// dummy, because MTGS doesn't need this mess!
	// (and zerogs does >_<)
}

sptr mtgsThreadObject::ExecuteTask()
{
	memcpy_aligned( m_gsMem, PS2MEM_GS, sizeof(PS2MEM_GS) );
	GSsetBaseMem( m_gsMem );
	GSirqCallback( dummyIrqCallback );

	Console::WriteLn( (wxString)L"\t\tForced software switch: " + (renderswitch ? L"Enabled" : L"Disabled") );
	m_returncode = GSopen( (void*)&pDsp, "PCSX2", renderswitch ? 2 : 1 );
	DevCon::WriteLn( "MTGS: GSopen Finished, return code: 0x%x", m_returncode );

	GSCSRr = 0x551B4000; // 0x55190000
	m_sem_InitDone.Post();
	if (m_returncode != 0) { return m_returncode; }		// error msg will be issued to the user by Plugins.c

#ifdef RINGBUF_DEBUG_STACK
	PacketTagType prevCmd;
#endif

	_RingbufferLoop();
	return 0;
}

// Waits for the GS to empty out the entire ring buffer contents.
// Used primarily for plugin startup/shutdown.
void mtgsThreadObject::WaitGS()
{
	// Freeze registers because some kernel code likes to destroy them
	SetEvent();
	while( volatize(m_RingPos) != volatize(m_WritePos) )
	{
		Timeslice();
		//SpinWait();
	}
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
	//Console::Notice( "MTGS Stall!  EE waits for nothing! ... except your GPU sometimes." );
	SetEvent();
	Timeslice();
}

void mtgsThreadObject::PostEventWait() const
{
}

u8* mtgsThreadObject::GetDataPacketPtr() const
{
	return (u8*)m_RingBuffer.GetPtr( m_packet_ringpos );
}

// Closes the data packet send command, and initiates the gs thread (if needed).
void mtgsThreadObject::SendDataPacket()
{
	// make sure a previous copy block has been started somewhere.
	jASSUME( m_packet_size != 0 );

	uint temp = m_packet_ringpos + m_packet_size;
	jASSUME( temp <= m_RingBufferSize );
	temp &= m_RingBufferMask;

	if( IsDebugBuild )
	{
		if( m_packet_ringpos + m_packet_size < m_RingBufferSize )
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
			//Console::Status( "MTGS Kick! DataSize : 0x%5.8x, CommandTally : %d", m_CopyDataTally, m_CopyCommandTally );
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

#ifdef PCSX2_GSRING_SAMPLING_STATS
static u32 GSRingBufCopySz = 0;
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
		Console::Status("GSRingBufCopy:128MB in %d tx -> b/tx: AVG = %.2f , max = %d, min = %d",ringtx_c,ringtx_s/(float)ringtx_c,ringtx_s_max,ringtx_s_min);
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
					Console::Notice("GSRingBufCopy :tx [%d,%d] algn %d : count= %d [%.2f%%]",1<<i,(1<<(i+1))-16,1<<j,ringtx_inf[i][j],ringtx_inf[i][j]/(float)ringtx_c*100);
					ringtx_inf[i][j]=0;
				}
			}
			if (total_bucket)
				Console::Notice("GSRingBufCopy :tx [%d,%d] total : count= %d [%.2f%%] [%.2f%%]",1<<i,(1<<(i+1))-16,total_bucket,total_bucket/(float)ringtx_c*100,ringtx_inf_s[i]/(float)ringtx_s*100);
			ringtx_inf_s[i]=0;
		}
		Console::Notice("GSRingBufCopy :tx ulg count =%d [%.2f%%]",ringtx_s_ulg,ringtx_s_ulg/(float)ringtx_s*100);
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
	jASSUME( size < m_RingBufferSize );
	jASSUME( writepos < m_RingBufferSize );

	m_packet_size = gifTransferDummy(pathidx, srcdata, size);
	size		  = m_packet_size + 1; // takes into account our command qword.

	if( writepos + size < m_RingBufferSize )
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
	else if( writepos + size > m_RingBufferSize )
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
		//Console::WriteLn( "MTGS > Perfect Fit!");

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

	PacketTagType& tag = (PacketTagType&)m_RingBuffer[m_WritePos];
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
	jASSUME( future_writepos <= m_RingBufferSize );

    future_writepos &= m_RingBufferMask;

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
	PacketTagType& tag = (PacketTagType&)m_RingBuffer[m_WritePos];

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
	PacketTagType& tag = (PacketTagType&)m_RingBuffer[m_WritePos];

	tag.command = type;
	tag.data[0] = data0;
	*(uptr*)&tag.data[1] = (uptr)data1;

	_FinishSimplePacket( thefuture );
}

// Waits for the GS to empty out the entire ring buffer contents.
// Used primarily for plugin startup/shutdown.
void mtgsWaitGS()
{
	if( mtgsThread == NULL ) return;
	mtgsThread->WaitGS();
}

// Exceptions:
//   ThreadCreationError - Thready could not be created (indicates OS resource limitations)
//   PluginFailure - GS plugin's "GSopen" call failed.
//
void mtgsOpen()
{
	// better not be a thread already running, yo!
	if( mtgsThread != NULL ) return;

	mtgsThread = new mtgsThreadObject();

	try
	{
		mtgsThread->Start();
	}
	catch( ... )
	{
		// if the thread start fails for any reason then set the handle to null.
		// The handle is used as a NULL test of thread running status, which is why
		// we really need to do this. :)
		safe_delete( mtgsThread );
		throw;
	}
}


void mtgsThreadObject::GIFSoftReset( int mask )
{
	if(mask & 1) memzero_obj(m_path[0]);
	if(mask & 2) memzero_obj(m_path[1]);
	if(mask & 4) memzero_obj(m_path[2]);

	if( GSgifSoftReset == NULL ) return;

	MTGS_LOG( "MTGS: Sending GIF Soft Reset (mask: %d)", mask );
	mtgsThread->SendSimplePacket( GS_RINGTYPE_SOFTRESET, mask, 0, 0 );
}

void mtgsThreadObject::Freeze( SaveStateBase& state )
{
	_mtgsFreezeGIF( state, this->m_path );
}

// this function is needed because of recompiled calls from iGS.cpp
// (currently used in GCC only)
//void mtgsRingBufSimplePacket( s32 command, u32 data0, u32 data1, u32 data2 )
//{
//	mtgsThread->SendSimplePacket( (GS_RINGTYPE)command, data0, data1, data2 );
//}
