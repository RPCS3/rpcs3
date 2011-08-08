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

#include "portaudio.h"

#include "wchar.h"

#include <vector>

#ifdef __WIN32__
#include "pa_win_wasapi.h"
#endif

int PaCallback( const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData );

class Portaudio : public SndOutModule
{
private:
	//////////////////////////////////////////////////////////////////////////////////////////
	// Configuration Vars (unused still)

	int m_ApiId;
	wxString m_Device;

	bool m_UseHardware;

	bool m_WasapiExclusiveMode;
	
	bool m_SuggestedLatencyMinimal;
	int m_SuggestedLatencyMS;

	//////////////////////////////////////////////////////////////////////////////////////////
	// Instance vars

	int writtenSoFar;
	int writtenLastTime;
	int availableLastTime;

	bool started;
	PaStream* stream;

	std::vector<const PaDeviceInfo*> devices;

public:
	int ActualPaCallback( const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData )
	{
		StereoOut32* p1 = (StereoOut32*)outputBuffer;

		int packets = framesPerBuffer / SndOutPacketSize;

		for(int p=0; p<packets; p++, p1+=SndOutPacketSize )
			SndBuffer::ReadSamples( p1 );

		writtenSoFar += packets * SndOutPacketSize;

		return 0;
	}

	Portaudio()
	{
		m_ApiId=-1;
		m_SuggestedLatencyMinimal = true;
		m_SuggestedLatencyMS = 20;
	}

	s32 Init()
	{
		started=false;
		stream=NULL;

		ReadSettings();

		PaError err = Pa_Initialize();
		if( err != paNoError )
		{
			fprintf(stderr,"* SPU2-X: PortAudio error: %s\n", Pa_GetErrorText( err ) );
			return -1;
		}
		started=true;

		int deviceIndex = -1;

		fprintf(stderr,"* SPU2-X: Enumerating PortAudio devices:");
		for(int i=0;i<Pa_GetDeviceCount();i++)
		{
			const PaDeviceInfo * info = Pa_GetDeviceInfo(i);

			const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(info->hostApi);

			fprintf(stderr," *** Device %d: '%s' (%s)", i, info->name, apiinfo->name);

			if(apiinfo->type == m_ApiId)
			{
				static wchar_t buffer [1000];
				mbstowcs(buffer,info->name,1000);
				buffer[999]=0;

				if(m_Device == buffer)
				{
					deviceIndex = i;
					fprintf(stderr," (selected)");
				}

			}
			fprintf(stderr,"\n");
		}

		if(deviceIndex<0 && m_ApiId>=0)
		{
			for(int i=0;i<Pa_GetHostApiCount();i++)
			{
				const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(i);
				if(apiinfo->type == m_ApiId)
				{
					deviceIndex = apiinfo->defaultOutputDevice;
				}
			}
		}

		if(deviceIndex>=0)
		{
			void* infoPtr = NULL;

#ifdef __WIN32__
			PaWasapiStreamInfo info = {
				sizeof(PaWasapiStreamInfo),
				paWASAPI,
				1,
				paWinWasapiExclusive
			};

			if((m_ApiId == paWASAPI) && m_WasapiExclusiveMode)
			{
				// Pass it the Exclusive mode enable flag
				infoPtr = &info;
			}
#endif

			PaStreamParameters outParams = {

			//	PaDeviceIndex device;
			//	int channelCount;
			//	PaSampleFormat sampleFormat;
			//	PaTime suggestedLatency;
			//	void *hostApiSpecificStreamInfo;
				deviceIndex,
				2,
				paInt32,
				m_SuggestedLatencyMinimal?(SndOutPacketSize/(float)SampleRate):(m_SuggestedLatencyMS/1000.0f),
				infoPtr
			};

			err = Pa_OpenStream(&stream,
				NULL, &outParams, SampleRate,
				SndOutPacketSize,
				paNoFlag,
				PaCallback,

				NULL);
		}
		else
		{
			err = Pa_OpenDefaultStream( &stream,
				0, 2, paInt32, 48000,
				SndOutPacketSize,
#ifndef __LINUX__
				PaCallback,
#else
				PaLinuxCallback,
#endif
				NULL );
		}
		if( err != paNoError )
		{
			fprintf(stderr,"* SPU2-X: PortAudio error: %s\n", Pa_GetErrorText( err ) );
			Pa_Terminate();
			return -1;
		}

		err = Pa_StartStream( stream );
		if( err != paNoError )
		{
			fprintf(stderr,"* SPU2-X: PortAudio error: %s\n", Pa_GetErrorText( err ) );
			Pa_CloseStream(stream);
			stream=NULL;
			Pa_Terminate();
			return -1;
		}

		return 0;
	}

	void Close()
	{
		PaError err;
		if(started)
		{
			if(stream)
			{
				if(Pa_IsStreamActive(stream))
				{
					err = Pa_StopStream(stream);
					if( err != paNoError )
						fprintf(stderr,"* SPU2-X: PortAudio error: %s\n", Pa_GetErrorText( err ) );
				}

				err = Pa_CloseStream(stream);
				if( err != paNoError )
					fprintf(stderr,"* SPU2-X: PortAudio error: %s\n", Pa_GetErrorText( err ) );

				stream=NULL;
			}

			// Seems to do more harm than good.
			//PaError err = Pa_Terminate();
			//if( err != paNoError )
			//	fprintf(stderr,"* SPU2-X: PortAudio error: %s\n", Pa_GetErrorText( err ) );

			started=false;
		}
	}

	////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////
#ifdef WIN32
private:
	
	bool _ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
	{
		int wmId,wmEvent;
		int tSel = 0;

		switch(uMsg)
		{
			case WM_INITDIALOG:
			{
				wchar_t temp[128];
								
				for(int i=0;i<Pa_GetDeviceCount();i++)
				{
					const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
			
					devices.push_back(info);
				}
				
				SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_RESETCONTENT,0,0);
				SendMessageA(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_ADDSTRING,0,(LPARAM)"Default Device");
				SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_SETCURSEL,0,0);

				SendMessage(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_RESETCONTENT,0,0);
				SendMessageA(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_ADDSTRING,0,(LPARAM)"Unspecified");
				int idx = 0;
				for(int i=0;i<Pa_GetHostApiCount();i++)
				{
					const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(i);
					if(apiinfo->deviceCount > 0)
					{
						SendMessageA(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_ADDSTRING,0,(LPARAM)apiinfo->name);
						SendMessageA(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_SETITEMDATA,i+1,apiinfo->type);
					}
					if(apiinfo->type == m_ApiId)
					{
						idx = i+1;
					}
				}
				SendMessage(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_SETCURSEL,idx,0);
				
				if(idx > 0)
				{
					int api_idx = idx-1;
					SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_RESETCONTENT,0,0);
					SendMessageA(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_ADDSTRING,0,(LPARAM)"Default Device");
					SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_SETITEMDATA,0,-1);
					int idx=0;
					int i=1,j=0;
					for(std::vector<const PaDeviceInfo*>::iterator it=devices.begin(); it != devices.end(); it++, j++)
					{
						const PaDeviceInfo* info = *it;
						if(info->hostApi == api_idx && info->maxOutputChannels > 0)
						{
							SendMessageA(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_ADDSTRING,0,(LPARAM)info->name);
							SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_SETITEMDATA,i,j);			
							if(wxString::FromAscii(info->name) == m_Device)
							{
								idx = i;
							}
							i++;
						}
					}
					SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_SETCURSEL,idx,0);
				}

				INIT_SLIDER( IDC_LATENCY, 10, 200, 10, 1, 1 );
				SendMessage(GetDlgItem(hWnd,IDC_LATENCY),TBM_SETPOS,TRUE,m_SuggestedLatencyMS);
				swprintf_s(temp, L"%d ms",m_SuggestedLatencyMS);
				SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);

				if(m_SuggestedLatencyMinimal)
					SET_CHECK( IDC_MINIMIZE, true );
				else
					SET_CHECK( IDC_MANUAL, true );
				
				SET_CHECK( IDC_EXCLUSIVE, m_WasapiExclusiveMode );
			}
			break;

			case WM_COMMAND:
			{
				//wchar_t temp[128];

				wmId    = LOWORD(wParam);
				wmEvent = HIWORD(wParam);
				// Parse the menu selections:
				switch (wmId)
				{
					case IDOK:
					{
						int idx = (int)SendMessage(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_GETCURSEL,0,0);
						m_ApiId = SendMessage(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_GETITEMDATA,idx,0);

						idx = (int)SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_GETCURSEL,0,0);
						idx = SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_GETITEMDATA,idx,0);
						if(idx >= 0)
							m_Device = wxString::FromAscii( devices.at(idx)->name );
						else
							m_Device = L"default";
						
						m_SuggestedLatencyMS = (int)SendMessage( GetDlgItem( hWnd, IDC_LATENCY ), TBM_GETPOS, 0, 0 );

						if( m_SuggestedLatencyMS < 10 ) m_SuggestedLatencyMS = 10;
						if( m_SuggestedLatencyMS > 200 ) m_SuggestedLatencyMS = 200;

						m_SuggestedLatencyMinimal = SendMessage(GetDlgItem(hWnd,IDC_MINIMIZE),BM_GETCHECK,0,0)==BST_CHECKED;
						
						m_WasapiExclusiveMode = SendMessage(GetDlgItem(hWnd,IDC_EXCLUSIVE),BM_GETCHECK,0,0)==BST_CHECKED;

						EndDialog(hWnd,0);
					}
					break;

					case IDCANCEL:
						EndDialog(hWnd,0);
					break;

					case IDC_PA_HOSTAPI:
					{
						if(wmEvent == CBN_SELCHANGE)
						{
							int api_idx = (int)SendMessage(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_GETCURSEL,0,0)-1;
							int apiId = SendMessageA(GetDlgItem(hWnd,IDC_PA_HOSTAPI),CB_GETITEMDATA,api_idx,0);
							SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_RESETCONTENT,0,0);
							SendMessageA(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_ADDSTRING,0,(LPARAM)"Default Device");
							SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_SETITEMDATA,0,-1);
							int idx=0;
							int i=1,j=0;
							for(std::vector<const PaDeviceInfo*>::iterator it=devices.begin(); it != devices.end(); it++, j++)
							{
								const PaDeviceInfo* info = *it;
								if(info->hostApi == api_idx && info->maxOutputChannels > 0)
								{
									SendMessageA(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_ADDSTRING,0,(LPARAM)info->name);
									SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_SETITEMDATA,i,j);
									i++;
								}
							}
							SendMessage(GetDlgItem(hWnd,IDC_PA_DEVICE),CB_SETCURSEL,idx,0);
						}
					}
					break;

					default:
						return FALSE;
				}
			}
			break;

			case WM_HSCROLL:
			{
				wmId    = LOWORD(wParam);
				wmEvent = HIWORD(wParam);
				switch(wmId)
				{
					//case TB_ENDTRACK:
					//case TB_THUMBPOSITION:
					case TB_LINEUP:
					case TB_LINEDOWN:
					case TB_PAGEUP:
					case TB_PAGEDOWN:
						wmEvent = (int)SendMessage((HWND)lParam,TBM_GETPOS,0,0);
					case TB_THUMBTRACK:
					{
						wchar_t temp[128];
						if( wmEvent < 10 ) wmEvent = 10;
						if( wmEvent > 200 ) wmEvent = 200;
						SendMessage((HWND)lParam,TBM_SETPOS,TRUE,wmEvent);
						swprintf_s(temp, L"%d ms",wmEvent);
						SetWindowText(GetDlgItem(hWnd,IDC_LATENCY_LABEL),temp);
						break;
					}
					default:
						return FALSE;
				}
			}
			break;

			default:
				return FALSE;
		}
		return TRUE;
	}

	static BOOL CALLBACK ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK DSEnumCallback( LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext );

public:
	virtual void Configure(uptr parent)
	{		
		PaError err = Pa_Initialize(); // Initialization can be done multiple times, PA keeps a counter
		if( err != paNoError )
		{
			fprintf(stderr,"* SPU2-X: PortAudio error: %s\n", Pa_GetErrorText( err ) );
			return;
		}
		// keep portaudio initialized until the dialog closes

		INT_PTR ret;
		ret=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_PORTAUDIO),(HWND)parent,(DLGPROC)ConfigProc,1);
		if(ret==-1)
		{
			MessageBox((HWND)parent,L"Error Opening the config dialog.",L"OMG ERROR!",MB_OK | MB_SETFOREGROUND);
			return;
		}

		Pa_Terminate();
	}
#else
	virtual void Configure(uptr parent)
	{		
	}
#endif

	virtual bool Is51Out() const { return false; }

	s32 Test() const
	{
		return 0;
	}

	int GetEmptySampleCount()
	{
		long availableNow = Pa_GetStreamWriteAvailable(stream);

		int playedSinceLastTime = (writtenSoFar - writtenLastTime) + (availableNow - availableLastTime);
		writtenLastTime = writtenSoFar;
		availableLastTime = availableNow;

		// Lowest resolution here is the SndOutPacketSize we use.
		return playedSinceLastTime;
	}

	const wchar_t* GetIdent() const
	{
		return L"portaudio";
	}

	const wchar_t* GetLongName() const
	{
		return L"Portaudio (crossplatform)";
	}

	void ReadSettings()
	{
		wxString api( L"EMPTYEMPTYEMPTY" );
		m_Device = L"EMPTYEMPTYEMPTY";
#ifdef __LINUX__
		// By default on linux use the ALSA API (+99% users) -- Gregory
		CfgReadStr( L"PORTAUDIO", L"HostApi", api, L"ALSA" );
#else
		CfgReadStr( L"PORTAUDIO", L"HostApi", api, L"Unknown" );
#endif
		CfgReadStr( L"PORTAUDIO", L"Device", m_Device, L"default" );

		SetApiSettings(api);

		m_WasapiExclusiveMode = CfgReadBool( L"PORTAUDIO", L"Wasapi_Exclusive_Mode", false);
		m_SuggestedLatencyMinimal = CfgReadBool( L"PORTAUDIO", L"Minimal_Suggested_Latency", true);
		m_SuggestedLatencyMS = CfgReadInt( L"PORTAUDIO", L"Manual_Suggested_Latency_MS", 20);
		
		if( m_SuggestedLatencyMS < 10 ) m_SuggestedLatencyMS = 10;
		if( m_SuggestedLatencyMS > 200 ) m_SuggestedLatencyMS = 200;
	}

	void SetApiSettings(wxString api)
	{
		m_ApiId = -1;
		if(api == L"InDevelopment") m_ApiId = paInDevelopment; /* use while developing support for a new host API */
		if(api == L"DirectSound")	m_ApiId = paDirectSound;
		if(api == L"MME")			m_ApiId = paMME;
		if(api == L"ASIO")			m_ApiId = paASIO;
		if(api == L"SoundManager")	m_ApiId = paSoundManager;
		if(api == L"CoreAudio")		m_ApiId = paCoreAudio;
		if(api == L"OSS")			m_ApiId = paOSS;
		if(api == L"ALSA")			m_ApiId = paALSA;
		if(api == L"AL")			m_ApiId = paAL;
		if(api == L"BeOS")			m_ApiId = paBeOS;
		if(api == L"WDMKS")			m_ApiId = paWDMKS;
		if(api == L"JACK")			m_ApiId = paJACK;
		if(api == L"WASAPI")		m_ApiId = paWASAPI;
		if(api == L"AudioScienceHPI") m_ApiId = paAudioScienceHPI;
	}

	void WriteSettings() const
	{
		wxString api;
		switch(m_ApiId)
		{
		case paInDevelopment:	api = L"InDevelopment"; break; /* use while developing support for a new host API */
		case paDirectSound:		api = L"DirectSound"; break;
		case paMME:				api = L"MME"; break;
		case paASIO:			api = L"ASIO"; break;
		case paSoundManager:	api = L"SoundManager"; break;
		case paCoreAudio:		api = L"CoreAudio"; break;
		case paOSS:				api = L"OSS"; break;
		case paALSA:			api = L"ALSA"; break;
		case paAL:				api = L"AL"; break;
		case paBeOS:			api = L"BeOS"; break;
		case paWDMKS:			api = L"WDMKS"; break;
		case paJACK:			api = L"JACK"; break;
		case paWASAPI:			api = L"WASAPI"; break;
		case paAudioScienceHPI: api = L"AudioScienceHPI"; break;
		default: api = L"Unknown";
		}

		CfgWriteStr( L"PORTAUDIO", L"HostApi", api);
		CfgWriteStr( L"PORTAUDIO", L"Device", m_Device);

		CfgWriteBool( L"PORTAUDIO", L"Wasapi_Exclusive_Mode", m_WasapiExclusiveMode);
		CfgWriteBool( L"PORTAUDIO", L"Minimal_Suggested_Latency", m_SuggestedLatencyMinimal);
		CfgWriteInt( L"PORTAUDIO", L"Manual_Suggested_Latency_MS", m_SuggestedLatencyMS);
	}

} static PA;

int PaCallback( const void *inputBuffer, void *outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags,
	void *userData )
{
	return PA.ActualPaCallback(inputBuffer,outputBuffer,framesPerBuffer,timeInfo,statusFlags,userData);
}

BOOL CALLBACK Portaudio::ConfigProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	return PA._ConfigProc( hWnd, uMsg, wParam, lParam );
}

SndOutModule *PortaudioOut = &PA;
