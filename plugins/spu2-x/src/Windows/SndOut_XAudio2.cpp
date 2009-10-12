/* SPU2-X, A plugin for Emulating the Sound Processing Unit of the Playstation 2
 * Developed and maintained by the Pcsx2 Development Team.
 * 
 * Original portions from SPU2ghz are (c) 2008 by David Quintana [gigaherz]
 *
 * SPU2-X is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * SPU2-X is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SPU2-X.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Global.h"

#define _WIN32_DCOM
#include "Dialogs.h"

#include <atlbase.h>
#include <xaudio2.h>

namespace Exception
{
	class XAudio2Error : public std::runtime_error
	{
	protected:
		static const char* SomeKindaErrorString( HRESULT hr )
		{
			switch( hr )
			{
				case XAUDIO2_E_INVALID_CALL:
					return "Invalid call for the XA2 object state.";

				case XAUDIO2_E_DEVICE_INVALIDATED:
					return "Device is unavailable, unplugged, unsupported, or has been consumed by The Nothing.";
			}
			return "Unknown error code!";
		}

	public:
		const HRESULT ErrorCode;
		string m_Message;

		const char* CMessage() const
		{
			return m_Message.c_str();
		}

		virtual ~XAudio2Error() throw() {}
		XAudio2Error( const HRESULT result, const std::string& msg ) :
			runtime_error( msg ),
			ErrorCode( result ),
			m_Message()
		{
			char omg[1024];
			sprintf_s( omg, "%s (code 0x%x)\n\n%s", what(), ErrorCode, SomeKindaErrorString( ErrorCode ) );
			m_Message = omg;
		}
	};
}

static const double SndOutNormalizer = (double)(1UL<<(SndOutVolumeShift+16));

class XAudio2Mod: public SndOutModule
{
private:
	static const int PacketsPerBuffer = 1;
	static const int MAX_BUFFER_COUNT = 3;

	class BaseStreamingVoice : public IXAudio2VoiceCallback
	{
	protected:
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
		}

		BaseStreamingVoice( uint numChannels ) :
			m_nBuffers( Config_XAudio2.NumBuffers ),
			m_nChannels( numChannels ),
			m_BufferSize( SndOutPacketSize * m_nChannels * PacketsPerBuffer ),
			m_BufferSizeBytes( m_BufferSize * sizeof(s16) )
		{
		}
		
		virtual void Init( IXAudio2* pXAudio2 ) = 0;
		
	protected:
		// Several things must be initialized separate of the constructor, due to the fact that
		// virtual calls can't be made from the constructor's context.
		void _init( IXAudio2* pXAudio2, uint chanConfig )
		{			
			WAVEFORMATEXTENSIBLE wfx;

			memset(&wfx, 0, sizeof(WAVEFORMATEXTENSIBLE));
			wfx.Format.wFormatTag		= WAVE_FORMAT_EXTENSIBLE;
			wfx.Format.nSamplesPerSec	= SampleRate;
			wfx.Format.nChannels		= m_nChannels;
			wfx.Format.wBitsPerSample	= 16;
			wfx.Format.nBlockAlign		= wfx.Format.nChannels*wfx.Format.wBitsPerSample/8;
			wfx.Format.nAvgBytesPerSec	= SampleRate * wfx.Format.nBlockAlign;
			wfx.Format.cbSize			= sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
			wfx.Samples.wValidBitsPerSample = 16;
			wfx.dwChannelMask			= chanConfig;
			wfx.SubFormat				= KSDATAFORMAT_SUBTYPE_PCM;

			//
			// Create an XAudio2 voice to stream this wave
			//
			HRESULT hr;
			if( FAILED(hr = pXAudio2->CreateSourceVoice( &pSourceVoice, (WAVEFORMATEX*)&wfx,
				XAUDIO2_VOICE_NOSRC, 1.0f, this ) ) )
			{
				throw Exception::XAudio2Error( hr, "XAudio2 CreateSourceVoice failure." );
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

		STDMETHOD_(void, OnVoiceProcessingPassStart) () {}
		STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32) { };
		STDMETHOD_(void, OnVoiceProcessingPassEnd) () {}
		STDMETHOD_(void, OnStreamEnd) () {}
		STDMETHOD_(void, OnBufferStart) ( void* ) {}
		STDMETHOD_(void, OnLoopEnd) ( void* ) {}   
		STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error) { };
	};
	
	template< typename T >
	class StreamingVoice : public BaseStreamingVoice
	{
	public:
		StreamingVoice( IXAudio2* pXAudio2 ) :
			BaseStreamingVoice( sizeof(T) / sizeof( s16 ) )
		{
		}
		
		virtual ~StreamingVoice()
		{
			IXAudio2SourceVoice* killMe = pSourceVoice;
			pSourceVoice = NULL;
			killMe->FlushSourceBuffers();
			EnterCriticalSection( &cs );
			killMe->DestroyVoice();
			safe_delete_array( qbuffer );
			LeaveCriticalSection( &cs );
			DeleteCriticalSection( &cs );
		}

		void Init( IXAudio2* pXAudio2 )
		{
			int chanMask = 0;
			switch(m_nChannels)
			{
				case 1: chanMask |= SPEAKER_FRONT_CENTER; break;
				case 2: chanMask |= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT; break;
				case 3: chanMask |= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY; break;
				case 4: chanMask |= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT; break;
				case 5: chanMask |= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT; break;
				case 6: chanMask |= SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_LOW_FREQUENCY; break;
			}
			_init( pXAudio2, chanMask );
		}

	protected:
		STDMETHOD_(void, OnBufferEnd) ( void* context )
		{
			EnterCriticalSection( &cs );

			// All of these checks are necessary because XAudio2 is wonky shizat.
			if( pSourceVoice == NULL || context == NULL )
			{
				LeaveCriticalSection( &cs );
				return;
			}

			T* qb = (T*)context;

			for(int p=0; p<PacketsPerBuffer; p++, qb+=SndOutPacketSize )
				SndBuffer::ReadSamples( qb );

			XAUDIO2_BUFFER buf = {0};
			buf.AudioBytes	= m_BufferSizeBytes;
			buf.pAudioData	= (BYTE*)context;
			buf.pContext	= context;

			pSourceVoice->SubmitSourceBuffer( &buf );
			LeaveCriticalSection( &cs );
		}

	};

	CComPtr<IXAudio2> pXAudio2;
	IXAudio2MasteringVoice* pMasteringVoice;
	BaseStreamingVoice* voiceContext;

public:

	s32 Init()
	{
		HRESULT hr;

		jASSUME( pXAudio2 == NULL );

		//
		// Initialize XAudio2
		//
		CoInitializeEx( NULL, COINIT_MULTITHREADED );

		UINT32 flags = 0;
		if( IsDebugBuild )
			flags |= XAUDIO2_DEBUG_ENGINE;

		try
		{
			if ( FAILED(hr = XAudio2Create( &pXAudio2, flags ) ) )
				throw Exception::XAudio2Error( hr,
					"Failed to init XAudio2 engine.  XA2 may not be available on your system.\n"
					"Ensure that you have the latest DirectX runtimes installed, or use \n"
					"DirectX / WaveOut drivers instead.  Error Details:"
				);

			XAUDIO2_DEVICE_DETAILS deviceDetails;
			pXAudio2->GetDeviceDetails( 0, &deviceDetails );

			if( StereoExpansionEnabled )
				deviceDetails.OutputFormat.Format.nChannels	= 6;

			// Any windows driver should support stereo at the software level, I should think!
			jASSUME( deviceDetails.OutputFormat.Format.nChannels > 1 );

			//
			// Create a mastering voice
			//
			if ( FAILED(hr = pXAudio2->CreateMasteringVoice( &pMasteringVoice, deviceDetails.OutputFormat.Format.nChannels, SampleRate ) ) )
			{
				SysMessage( "Failed creating mastering voice: %#X\n", hr );
				CoUninitialize();
				return -1;
			}

			switch( deviceDetails.OutputFormat.Format.nChannels )
			{
				case 2:
					ConLog( "* SPU2 > Using normal 2 speaker stereo output.\n" );
					voiceContext = new StreamingVoice<StereoOut16>( pXAudio2 );
				break;

				case 3:
					ConLog( "* SPU2 > 2.1 speaker expansion enabled.\n" );
					voiceContext = new StreamingVoice<Stereo21Out16>( pXAudio2 );
				break;

				case 4:
					ConLog( "* SPU2 > 4 speaker expansion enabled [quadraphenia]\n" );
					voiceContext = new StreamingVoice<StereoQuadOut16>( pXAudio2 );
				break;
							
				case 5:
					ConLog( "* SPU2 > 4.1 speaker expansion enabled.\n" );
					voiceContext = new StreamingVoice<Stereo41Out16>( pXAudio2 );
				break;

				case 6:
				case 7:
					ConLog( "* SPU2 > 5.1 speaker expansion enabled.\n" );
					voiceContext = new StreamingVoice<Stereo51Out16>( pXAudio2 );   //"normal" stereo upmix
					//voiceContext = new StreamingVoice<Stereo51Out16DplII>( pXAudio2 );  //gigas PLII
				break;

				default:	// anything 8 or more gets the 7.1 treatment!
					ConLog( "* SPU2 > 7.1 speaker expansion enabled.\n" );
					voiceContext = new StreamingVoice<Stereo51Out16>( pXAudio2 );
				break;
			}
			
			voiceContext->Init( pXAudio2 );
		}
		catch( Exception::XAudio2Error& ex )
		{
			SysMessage( ex.CMessage() );
			pXAudio2.Release();
			CoUninitialize();
			return -1;
		}

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

		safe_delete( voiceContext );

		voiceContext = NULL;

		if( pMasteringVoice != NULL )
			pMasteringVoice->DestroyVoice();

		pMasteringVoice = NULL;

		pXAudio2.Release();
		CoUninitialize();
	}

	virtual void Configure(uptr parent)
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
		return L"xaudio2";
	}

	const wchar_t* GetLongName() const
	{
		return L"XAudio 2 (Recommended)";
	}

	void ReadSettings()
	{
	}
	
	void WriteSettings() const
	{
	}

} static XA2;

SndOutModule *XAudio2Out = &XA2;
