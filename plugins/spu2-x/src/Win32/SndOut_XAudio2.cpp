/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2.1 of the the License, or (at your
 * option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 */

#define _WIN32_DCOM

#include "spu2.h"
#include "dialogs.h"
#include <MMReg.h>
#include <xaudio2.h>


static const double SndOutNormalizer = (double)(1UL<<(SndOutVolumeShift+16));

class XAudio2Mod: public SndOutModule
{
private:
	static const int PacketsPerBuffer = 1;
	static const int MAX_BUFFER_COUNT = 3;

	class BaseStreamingVoice : public IXAudio2VoiceCallback
	{
	protected:
		SndBuffer* m_sndout;
		IXAudio2SourceVoice* pSourceVoice;
		s16* qbuffer;

		const uint m_nBuffers;
		const uint m_nChannels;
		const uint m_BufferSize;
		const uint m_BufferSizeBytes;

		CRITICAL_SECTION cs;

	public:
		int GetEmptySampleCount() const
		{
			XAUDIO2_VOICE_STATE state;
			pSourceVoice->GetState( &state );
			return state.SamplesPlayed & (m_BufferSize-1);
		}

		virtual ~BaseStreamingVoice()
		{
			IXAudio2SourceVoice* killMe = pSourceVoice;
			pSourceVoice = NULL;
			killMe->FlushSourceBuffers();
			EnterCriticalSection( &cs );
			killMe->DestroyVoice();
			SAFE_DELETE_ARRAY( qbuffer );
			LeaveCriticalSection( &cs );
			DeleteCriticalSection( &cs );
		}

		BaseStreamingVoice( SndBuffer* sb, uint numChannels ) :
			m_sndout( sb ),
			m_nBuffers( Config_XAudio2.NumBuffers ),
			m_nChannels( numChannels ),
			m_BufferSize( SndOutPacketSize/2 * m_nChannels * PacketsPerBuffer ),
			m_BufferSizeBytes( m_BufferSize * sizeof(s16) )
		{
		}
		
		virtual void Init( IXAudio2* pXAudio2 ) = 0;
		
	protected:
		// Several things must be initialized separate of the constructor, due to the fact that
		// virtual calls can't be made from the constructor's context.
		void _init( IXAudio2* pXAudio2, uint chanConfig )
		{			
			WAVEFORMATEX wfx;

			wfx.wFormatTag		= WAVE_FORMAT_PCM;
			wfx.nSamplesPerSec	= SampleRate;
			wfx.nChannels		= m_nChannels;
			wfx.wBitsPerSample	= 16;
			wfx.nBlockAlign		= 2*m_nChannels;
			wfx.nAvgBytesPerSec	= SampleRate * wfx.nBlockAlign;
			wfx.cbSize			= 0;

			//wfx.SubFormat				= KSDATAFORMAT_SUBTYPE_PCM;
			//wfx.dwChannelMask			= chanConfig;

			//
			// Create an XAudio2 voice to stream this wave
			//
			HRESULT hr;
			if( FAILED(hr = pXAudio2->CreateSourceVoice( &pSourceVoice, &wfx,
				XAUDIO2_VOICE_NOSRC, 1.0f, this ) ) )
			{
				SysMessage( "Error %#X creating source voice\n", hr );
				SAFE_RELEASE( pXAudio2 );
				return;
			}

			InitializeCriticalSection( &cs );
			EnterCriticalSection( &cs );

			pSourceVoice->FlushSourceBuffers();
			pSourceVoice->Start( 0, 0 );

			qbuffer = new s16[m_nBuffers * m_BufferSize];
			ZeroMemory( qbuffer, m_BufferSizeBytes * m_nBuffers );

			// Start some buffers.

			for( uint i=0; i<m_nBuffers; i++ )
			{
				XAUDIO2_BUFFER buf = {0};
				buf.AudioBytes = m_BufferSizeBytes;
				buf.pContext   = &qbuffer[i*m_BufferSize];
				buf.pAudioData = (BYTE*)buf.pContext;
				pSourceVoice->SubmitSourceBuffer( &buf );
			}

			LeaveCriticalSection( &cs );
		}

	};
	
	
	class StreamingVoice_Stereo : public BaseStreamingVoice
	{
	public:
		StreamingVoice_Stereo( SndBuffer* sb, IXAudio2* pXAudio2 ) :
			BaseStreamingVoice( sb, 2 )
		{
		}
		
		virtual ~StreamingVoice_Stereo() {}

		void Init( IXAudio2* pXAudio2 )
		{
			_init( pXAudio2, SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT );
		}

	protected:
		STDMETHOD_(void, OnVoiceProcessingPassStart) () {}
		STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) { };
		STDMETHOD_(void, OnVoiceProcessingPassEnd) () {}
		STDMETHOD_(void, OnStreamEnd) () {}
		STDMETHOD_(void, OnBufferStart) ( void* ) {}
		STDMETHOD_(void, OnBufferEnd) ( void* context )
		{
			EnterCriticalSection( &cs );

			// All of these checks are necessary because XAudio2 is wonky shizat.
			if( pSourceVoice == NULL || context == NULL ) return;

			s16* qb = (s16*)context;

			for(int p=0; p<PacketsPerBuffer; p++, qb+=SndOutPacketSize )
				m_sndout->ReadSamples( qb );

			XAUDIO2_BUFFER buf = {0};
			buf.AudioBytes	= m_BufferSizeBytes;
			buf.pAudioData	= (BYTE*)context;
			buf.pContext	= context;

			pSourceVoice->SubmitSourceBuffer( &buf );
			LeaveCriticalSection( &cs );
		}
		STDMETHOD_(void, OnLoopEnd) ( void* ) {}   
		STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error) { };

	};

	class StreamingVoice_Surround51 : public BaseStreamingVoice
	{
	public:
		//LPF_data m_lpf_left;
		//LPF_data m_lpf_right;
		
		s32 buffer[2 * SndOutPacketSize * PacketsPerBuffer];

		StreamingVoice_Surround51( SndBuffer* sb, IXAudio2* pXAudio2 ) :
			BaseStreamingVoice( sb, 6 )
			//m_lpf_left( Config_XAudio2.LowpassLFE, SampleRate ),
			//m_lpf_right( Config_XAudio2.LowpassLFE, SampleRate )
		{
		}

		virtual ~StreamingVoice_Surround51() {}

		void Init( IXAudio2* pXAudio2 )
		{
			_init( pXAudio2, SPEAKER_5POINT1 );
		}
		
	protected:
		STDMETHOD_(void, OnVoiceProcessingPassStart) () {}
		STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) { };
		STDMETHOD_(void, OnVoiceProcessingPassEnd) () {}
		STDMETHOD_(void, OnStreamEnd) () {}
		STDMETHOD_(void, OnBufferStart) ( void* ) {}
		STDMETHOD_(void, OnBufferEnd) ( void* context )
		{
			EnterCriticalSection( &cs );

			// All of these checks are necessary because XAudio2 is wonky shizat.
			if( pSourceVoice == NULL || context == NULL ) return;

			s16* qb = (s16*)context;

			for(int p=0; p<PacketsPerBuffer; p++ )
			{
				m_sndout->ReadSamples( buffer );
				const s32* src = buffer;

				for( int i=0; i<SndOutPacketSize/2; i++, qb+=6, src+=2 )
				{
					// Left and right Front!
					qb[0] = SndScaleVol( src[0] );
					qb[1] = SndScaleVol( src[1] );
					
					// Center and Subwoofer/LFE -->
					// This method is simple and sounds nice.  It relies on the speaker/soundcard
					// systems do to their own low pass / crossover.  Manual lowpass is wasted effort
					// and can't match solid state results anyway.
					
					qb[2] = qb[3] = (src[0] + src[1]) >> (SndOutVolumeShift+1);
					
					// Left and right rear!
					qb[4] = SndScaleVol( src[0] );
					qb[5] = SndScaleVol( src[1] );
				}

			}

			XAUDIO2_BUFFER buf = { 0 };
			buf.AudioBytes = m_BufferSizeBytes;
			buf.pAudioData = (BYTE*)context;
			buf.pContext = context;

			pSourceVoice->SubmitSourceBuffer( &buf );
			LeaveCriticalSection( &cs );
		}
		STDMETHOD_(void, OnLoopEnd) ( void* ) {}   
		STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error) { };

	};

	IXAudio2* pXAudio2;
	IXAudio2MasteringVoice* pMasteringVoice;
	BaseStreamingVoice* voiceContext;

public:

	s32 Init( SndBuffer *sb )
	{
		HRESULT hr;

		//
		// Initialize XAudio2
		//
		CoInitializeEx( NULL, COINIT_MULTITHREADED );

		UINT32 flags = 0;
#ifdef _DEBUG
		flags |= XAUDIO2_DEBUG_ENGINE;
#endif

		if ( FAILED(hr = XAudio2Create( &pXAudio2, flags ) ) )
		{
			SysMessage( "Failed to init XAudio2 engine: %#X\n", hr );
			CoUninitialize();
			return -1;
		}

		XAUDIO2_DEVICE_DETAILS deviceDetails;
		pXAudio2->GetDeviceDetails( 0, &deviceDetails );

		//
		// Create a mastering voice
		//
		if ( FAILED(hr = pXAudio2->CreateMasteringVoice( &pMasteringVoice, 0, SampleRate ) ) )
		{
			SysMessage( "Failed creating mastering voice: %#X\n", hr );
			SAFE_RELEASE( pXAudio2 );
			CoUninitialize();
			return -1;
		}

		if( Config_XAudio2.ExpandTo51 && deviceDetails.OutputFormat.Format.nChannels >= 6 )
		{
			ConLog( "* SPU2 > 5.1 speaker expansion enabled." );
			voiceContext = new StreamingVoice_Surround51( sb, pXAudio2 );
		}
		else
		{
			voiceContext = new StreamingVoice_Stereo( sb, pXAudio2 );
		}

		voiceContext->Init( pXAudio2 );

		return 0;
	}

	void Close()
	{
		// Clean up?
		// All XAudio2 interfaces are released when the engine is destroyed,
		// but being tidy never hurt.

		// Actually it can hurt.  As of DXSDK Aug 2008, doing a full cleanup causes
		// XA2 on Vista to crash.  Even if you copy/paste code directly from Microsoft.
		// But doing no cleanup at all causes XA2 under XP to crash.  So after much trial
		// and error we found a happy compromise as follows:

		SAFE_DELETE_OBJ( voiceContext );

		voiceContext = NULL;

		if( pMasteringVoice != NULL )
			pMasteringVoice->DestroyVoice();

		pMasteringVoice = NULL;

		SAFE_RELEASE( pXAudio2 );

		CoUninitialize();
	}

	virtual void Configure(HWND parent)
	{
	}

	virtual bool Is51Out() const { return false; }

	s32  Test() const
	{
		return 0;
	}

	int GetEmptySampleCount() const
	{
		if( voiceContext == NULL ) return 0;
		return voiceContext->GetEmptySampleCount();
	}

	const wchar_t* GetIdent() const
	{
		return _T("xaudio2");
	}

	const wchar_t* GetLongName() const
	{
		return _T("XAudio 2 (Recommended)");
	}

} XA2;

SndOutModule *XAudio2Out = &XA2;
