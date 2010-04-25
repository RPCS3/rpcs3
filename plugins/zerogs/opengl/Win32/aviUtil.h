#define AVIIF_KEYFRAME  0x00000010L // this frame is a key frame.

#include <string>
using namespace std;

#include <memory.h>
#include <mmsystem.h>
#include <vfw.h>

BOOL AVI_Init()
{
	/* first let's make sure we are running on 1.1 */
	WORD wVer = HIWORD(VideoForWindowsVersion());
	if (wVer < 0x010a){
	     /* oops, we are too old, blow out of here */
	     //MessageBeep(MB_ICONHAND);
	     MessageBox(NULL, "Cant't init AVI File - Video for Windows version is to old", "Error", MB_OK|MB_ICONSTOP);
	     return FALSE;
	}

	AVIFileInit();

	return TRUE;
}

BOOL AVI_FileOpenWrite(PAVIFILE * pfile, const char *filename)
{
	HRESULT hr = AVIFileOpen(pfile,	   // returned file pointer
		       filename,		  // file name
		       OF_WRITE | OF_CREATE,      // mode to open file with
		       NULL);		     // use handler determined
						  // from file extension....
	if (hr != AVIERR_OK)
		return FALSE;

	return TRUE;
}

DWORD getFOURCC(const char* value)
{
	if(_stricmp(value, "DIB") == 0)
	{
		return mmioFOURCC(value[0],value[1],value[2],' ');
	}
	else if((_stricmp(value, "CVID") == 0)
		 || (_stricmp(value, "IV32") == 0)
		 || (_stricmp(value, "MSVC") == 0)
		 || (_stricmp(value, "IV50") == 0))
	{
		return mmioFOURCC(value[0],value[1],value[2],value[3]);
	}
	else
	{
		return NULL;
	}
}

// Fill in the header for the video stream....
// The video stream will run in rate ths of a second....
BOOL AVI_CreateStream(PAVIFILE pfile, PAVISTREAM * ps, int rate, // sample/second
		      unsigned long buffersize, int rectwidth, int rectheight,
					  const char* _compressor)
{
		AVISTREAMINFO strhdr;
		memset(&strhdr, 0, sizeof(strhdr));
	strhdr.fccType		= streamtypeVIDEO;// stream type
		strhdr.fccHandler	     = getFOURCC(_compressor);
	//strhdr.fccHandler	     = 0; // no compression!
		//strhdr.fccHandler	     = mmioFOURCC('D','I','B',' '); // Uncompressed
		//strhdr.fccHandler	     = mmioFOURCC('C','V','I','D'); // Cinpak
		//strhdr.fccHandler	     = mmioFOURCC('I','V','3','2'); // Intel video 3.2
		//strhdr.fccHandler	     = mmioFOURCC('M','S','V','C'); // Microsoft video 1
		//strhdr.fccHandler	     = mmioFOURCC('I','V','5','0'); // Intel video 5.0
		//strhdr.dwFlags		= AVISTREAMINFO_DISABLED;
		//strhdr.dwCaps		 =
		//strhdr.wPriority	      =
		//strhdr.wLanguage	      =
	strhdr.dwScale		= 1;
	strhdr.dwRate		 = rate;	       // rate fps
		//strhdr.dwStart		=
		//strhdr.dwLength	       =
		//strhdr.dwInitialFrames	=
	strhdr.dwSuggestedBufferSize  = buffersize;
		strhdr.dwQuality	      = -1; // use the default
		//strhdr.dwSampleSize	   =
	SetRect(&strhdr.rcFrame, 0, 0,	      // rectangle for stream
	    (int) rectwidth,
	    (int) rectheight);
		//strhdr.dwEditCount	    =
		//strhdr.dwFormatChangeCount    =
		//strcpy(strhdr.szName, "Full Frames (Uncompressed)");

	// And create the stream;
	HRESULT hr = AVIFileCreateStream(pfile,	     // file pointer
				 ps,		// returned stream pointer
				 &strhdr);	  // stream header
	if (hr != AVIERR_OK) {
		return FALSE;
	}

	return TRUE;
}

string getFOURCCVAsString(DWORD value)
{
	string returnValue = "";
	if( value == 0 )
		return returnValue;

	DWORD ch0 = value & 0x000000FF;
	returnValue.push_back((char) ch0);
	DWORD ch1 = (value & 0x0000FF00)>>8;
	returnValue.push_back((char) ch1);
	DWORD ch2 = (value & 0x00FF0000)>>16;
	returnValue.push_back((char) ch2);
	DWORD ch3 = (value & 0xFF000000)>>24;
	returnValue.push_back((char) ch3);

	return returnValue;
}

string dumpAVICOMPRESSOPTIONS(AVICOMPRESSOPTIONS opts)
{
	char tmp[255];
	string returnValue = "Dump of AVICOMPRESSOPTIONS\n";

	returnValue += "DWORD  fccType = streamtype("; returnValue += getFOURCCVAsString(opts.fccType); returnValue += ")\n";
	returnValue += "DWORD  fccHandler = "; returnValue += getFOURCCVAsString(opts.fccHandler); returnValue += "\n";

	_snprintf(tmp, 255, "DWORD  dwKeyFrameEvery = %d\n", opts.dwKeyFrameEvery);
	returnValue += tmp;

	_snprintf(tmp, 255, "DWORD  dwQuality = %d\n", opts.dwQuality);
	returnValue += tmp;

	_snprintf(tmp, 255, "DWORD  dwBytesPerSecond = %d\n", opts.dwBytesPerSecond);
	returnValue += tmp;

	if((opts.dwFlags & AVICOMPRESSF_DATARATE) == AVICOMPRESSF_DATARATE){strcpy(tmp, "DWORD  fccType = AVICOMPRESSF_DATARATE\n");}
	else if((opts.dwFlags & AVICOMPRESSF_INTERLEAVE) == AVICOMPRESSF_INTERLEAVE){strcpy(tmp, "DWORD  fccType = AVICOMPRESSF_INTERLEAVE\n");}
	else if((opts.dwFlags & AVICOMPRESSF_KEYFRAMES) == AVICOMPRESSF_KEYFRAMES){strcpy(tmp, "DWORD  fccType = AVICOMPRESSF_KEYFRAMES\n");}
	else if((opts.dwFlags & AVICOMPRESSF_VALID) == AVICOMPRESSF_VALID){strcpy(tmp, "DWORD  fccType = AVICOMPRESSF_VALID\n");}
	else {_snprintf(tmp, 255, "DWORD  dwFlags = Unknown(%d)\n", opts.dwFlags);}
	returnValue += tmp;

	_snprintf(tmp, 255, "LPVOID lpFormat = %d\n", (int)opts.lpFormat);
	returnValue += tmp;

	_snprintf(tmp, 255, "DWORD  cbFormat = %d\n", opts.cbFormat);
	returnValue += tmp;

	_snprintf(tmp, 255, "LPVOID lpParms = %d\n", (int)opts.lpParms);
	returnValue += tmp;

	_snprintf(tmp, 255, "DWORD  cbParms = %d\n", opts.cbParms);
	returnValue += tmp;

	_snprintf(tmp, 255, "DWORD  dwInterleaveEvery = %d\n", opts.dwInterleaveEvery);
	returnValue += tmp;

	return returnValue;
}

BOOL AVI_SetOptions(PAVISTREAM * ps, PAVISTREAM * psCompressed, LPBITMAPINFOHEADER lpbi,
					const char* _compressor)
{

	AVICOMPRESSOPTIONS opts;
	AVICOMPRESSOPTIONS FAR * aopts[1] = {&opts};

		memset(&opts, 0, sizeof(opts));
		opts.fccType = streamtypeVIDEO;
		opts.fccHandler	     = getFOURCC(_compressor);
		//opts.fccHandler  = 0;
		//opts.fccHandler	    = mmioFOURCC('D','I','B',' '); // Uncompressed
		//opts.fccHandler	     = mmioFOURCC('C','V','I','D'); // Cinpak
		//opts.fccHandler	     = mmioFOURCC('I','V','3','2'); // Intel video 3.2
		//opts.fccHandler	     = mmioFOURCC('M','S','V','C'); // Microsoft video 1
		//opts.fccHandler	     = mmioFOURCC('I','V','5','0'); // Intel video 5.0
		//opts.dwKeyFrameEvery = 5;
		//opts.dwQuality
		//opts.dwBytesPerSecond
		//opts.dwFlags		= AVICOMPRESSF_KEYFRAMES;
		//opts.lpFormat
		//opts.cbFormat
		//opts.lpParms
		//opts.cbParms
		//opts.dwInterleaveEvery

		/* display the compression options dialog box if specified compressor is unknown */
		if(getFOURCC(_compressor) == NULL)
		{
			if (!AVISaveOptions(NULL, 0, 1, ps, (LPAVICOMPRESSOPTIONS FAR *) &aopts))
			{
				return FALSE;
			}

			//printf("%s", dumpAVICOMPRESSOPTIONS(opts));
			//MessageBox(NULL, dumpAVICOMPRESSOPTIONS(opts).c_str(), "AVICOMPRESSOPTIONS", MB_OK);
		}

	HRESULT hr = AVIMakeCompressedStream(psCompressed, *ps, &opts, NULL);
	if (hr != AVIERR_OK) {
		return FALSE;
	}

	hr = AVIStreamSetFormat(*psCompressed, 0,
			       lpbi,		    // stream format
			       lpbi->biSize	     // format size
				   + lpbi->biClrUsed * sizeof(RGBQUAD)
				   );
	if (hr != AVIERR_OK) {
	return FALSE;
	}

	return TRUE;
}

BOOL AVI_SetText(PAVIFILE pfile, PAVISTREAM psText, char *szText, int width, int height, int TextHeight)
{
	// Fill in the stream header for the text stream....
	AVISTREAMINFO strhdr;
	DWORD dwTextFormat;
	// The text stream is in 60ths of a second....

		memset(&strhdr, 0, sizeof(strhdr));
	strhdr.fccType		= streamtypeTEXT;
	strhdr.fccHandler	     = mmioFOURCC('D', 'R', 'A', 'W');
	strhdr.dwScale		= 1;
	strhdr.dwRate		 = 60;
	strhdr.dwSuggestedBufferSize  = sizeof(szText);
	SetRect(&strhdr.rcFrame, 0, (int) height,
	    (int) width, (int) height + TextHeight); // #define TEXT_HEIGHT 20

	// ....and create the stream.
	HRESULT hr = AVIFileCreateStream(pfile, &psText, &strhdr);
	if (hr != AVIERR_OK) {
		return FALSE;
	}

	dwTextFormat = sizeof(dwTextFormat);
	hr = AVIStreamSetFormat(psText, 0, &dwTextFormat, sizeof(dwTextFormat));
	if (hr != AVIERR_OK) {
		return FALSE;
	}

	return TRUE;
}

BOOL AVI_AddFrame(PAVISTREAM psCompressed, int time, LPBITMAPINFOHEADER lpbi)
{
	int ImageSize = lpbi->biSizeImage;
	if (ImageSize == 0)
	{
		if (lpbi->biBitCount == 24)
		{
			ImageSize = lpbi->biWidth * lpbi->biHeight * 3;
		}
	}
	HRESULT hr = AVIStreamWrite(psCompressed, // stream pointer
		time, // time of this frame
		1, // number to write
		(LPBYTE) lpbi + // pointer to data
		lpbi->biSize +
		lpbi->biClrUsed * sizeof(RGBQUAD),
		ImageSize, // lpbi->biSizeImage, // size of this frame
		AVIIF_KEYFRAME, // flags....
		NULL,
		NULL);
	if (hr != AVIERR_OK)
	{
		char strMsg[255];
		_snprintf(strMsg, 255, "Error: AVIStreamWrite, error %d",hr);
		MessageBox(NULL, strMsg, "", MB_OK);
		return FALSE;
	}

	return TRUE;
}

BOOL AVI_AddText(PAVISTREAM psText, int time, char *szText)
{
	int iLen = (int)strlen(szText);

	HRESULT hr = AVIStreamWrite(psText,
			time,
			1,
			szText,
			iLen + 1,
			AVIIF_KEYFRAME,
			NULL,
			NULL);
	if (hr != AVIERR_OK)
		return FALSE;

	return TRUE;
}

BOOL AVI_CloseStream(PAVISTREAM ps, PAVISTREAM psCompressed, PAVISTREAM psText)
{
	if (ps)
		AVIStreamClose(ps);

	if (psCompressed)
		AVIStreamClose(psCompressed);

	if (psText)
		AVIStreamClose(psText);



	return TRUE;
}

BOOL AVI_CloseFile(PAVIFILE pfile)
{
	if (pfile)
		AVIFileClose(pfile);

	return TRUE;
}

BOOL AVI_Exit()
{
	AVIFileExit();

	return TRUE;
}
















/* Here are the additional functions we need! */


static PAVIFILE pfile = NULL;
static PAVISTREAM ps = NULL;
static PAVISTREAM psCompressed = NULL;
static int count = 0;


// Initialization...
bool START_AVI(const char* file_name)
{
    if(! AVI_Init())
	{
		//printf("Error - AVI_Init()\n");
		return false;
	}

    if(! AVI_FileOpenWrite(&pfile, file_name))
	{
		//printf("Error - AVI_FileOpenWrite()\n");
		return false;
	}

	return true;
}

bool ADD_FRAME_FROM_DIB_TO_AVI(const char* _compressor, int _frameRate, int width, int height, int bits, void* pdata)
{
	if(count == 0)
	{
		if(! AVI_CreateStream(pfile, &ps, _frameRate,
			width*height/bits,
			width,
			height, _compressor))
		{
			//printf("Error - AVI_CreateStream()\n");
			return false;
		}

		BITMAPINFOHEADER bi;
		memset(&bi, 0, sizeof(bi));
		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = width;
		bi.biHeight = height;
		bi.biPlanes = 1;
		bi.biBitCount = bits;
		bi.biCompression = BI_RGB;
		bi.biSizeImage = width * height * bits /8;
		if(! AVI_SetOptions(&ps, &psCompressed, &bi, _compressor))
		{
			return false;
		}
	}

	HRESULT hr = AVIStreamWrite(psCompressed, // stream pointer
		count, // time of this frame
		1, // number to write
		pdata,
		width*height/8, // lpbi->biSizeImage, // size of this frame
		AVIIF_KEYFRAME, // flags....
		NULL,
		NULL);
	if (hr != AVIERR_OK)
	{
		char strMsg[255];
		_snprintf(strMsg, 255, "Error: AVIStreamWrite, error %d",hr);
		MessageBox(NULL, strMsg, "", MB_OK);
		return FALSE;
	}

	count++;
	return true;
}

//Now we can add frames
// ie. ADD_FRAME_FROM_DIB_TO_AVI(yourDIB, "CVID", 25);
bool ADD_FRAME_FROM_DIB_TO_AVI(HANDLE dib, const char* _compressor, int _frameRate)
{
	LPBITMAPINFOHEADER lpbi;
	if(count == 0)
	{
		lpbi = (LPBITMAPINFOHEADER)GlobalLock(dib);
		if(! AVI_CreateStream(pfile, &ps, _frameRate,
			(unsigned long) lpbi->biSizeImage,
			(int) lpbi->biWidth,
			(int) lpbi->biHeight, _compressor))
		{
			//printf("Error - AVI_CreateStream()\n");
			GlobalUnlock(lpbi);
			return false;
		}

		if(! AVI_SetOptions(&ps, &psCompressed, lpbi, _compressor))
		{
			//printf("Error - AVI_SetOptions()\n");
			GlobalUnlock(lpbi);
			return false;
		}

		GlobalUnlock(lpbi);
	}

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(dib);
	if(! AVI_AddFrame(psCompressed, count * 1, lpbi))
	{
		//printf("Error - AVI_AddFrame()\n");
		GlobalUnlock(lpbi);
		return false;
	}

	GlobalUnlock(lpbi);
	count++;
	return true;
}

// The end...
bool STOP_AVI()
{
     if(! AVI_CloseStream(ps, psCompressed, NULL))
	 {
		 //printf("Error - AVI_CloseStream()\n");
		 return false;
	 }

     if(! AVI_CloseFile(pfile))
	 {
		//printf("Error - AVI_CloseFile()\n");
		return false;
	 }

     if(! AVI_Exit())
	 {
		//printf("Error - AVI_Exit()\n");
		return false;
	 }

	 return true;
}

