Hello

  This is a small list of steps in order to build portaudio
(Currently v19-devel) into a VS2005 DLL and lib file.
This DLL contains all 5 current Win32 PA APIS (MME/DS/ASIO/WASAPI/WDMKS)

1)Copy the source dirs that comes with the ASIO SDK inside src\hostapi\asio\ASIOSDK
  so you should now have example:
  
  portaudio19svn\src\hostapi\asio\ASIOSDK\common
  portaudio19svn\src\hostapi\asio\ASIOSDK\host
  portaudio19svn\src\hostapi\asio\ASIOSDK\host\sample
  portaudio19svn\src\hostapi\asio\ASIOSDK\host\pc
  portaudio19svn\src\hostapi\asio\ASIOSDK\host\mac (not needed)
  
  You dont need "driver"

  To build without ASIO (or another Host API) see the "Building without ASIO support" section below.
  
2)
  *If you have Visual Studio 6.0*, please make sure you have it updated with the latest (and final)
  microsoft libraries for it, namely:
  
  Service pack 5:         
     Latest known URL:  
     http://msdn2.microsoft.com/en-us/vstudio/aa718363.aspx 
	 Yes there EXISTS a service pack 6 , but the processor pack (below) isnt compatible with it.
	 
  Processor Pack(only works with above SP5)
     Latest known URL:
     http://msdn2.microsoft.com/en-us/vstudio/Aa718349.aspx
	 This isn't absolutely required for portaudio, but if you plan on using SSE intrinsics and similar things.
	 Up to you to decide upon Service pack 5 or 6 depending on your need for intrinsics.

  Platform SDK (Feb 2003) : 
     Latest known URL:  
     http://www.microsoft.com/msdownload/platformsdk/sdkupdate/psdk-full.htm
	 (This will allow your code base to be x64 friendly, with correct defines 
	 for LONG_PTR and such)
	 NOTE A) Yes you have to use IE activex scripts to install that - wont work in Firefox, you 
	 may have to temporarily change tyour default browser(aint life unfair)
	 NOTE B) Dont forget to hit "Register PSDK Directories with Visual Studio". 
	 you can make sure its right in VC6 if you open tools/options/directories/include files and you see SDK 2003 as the FIRST entry
	 (it must be the same for libs)
  
  DirectX 9.0 SDK Update - (Summer 2003)
    Latest known URL:
    http://www.microsoft.com/downloads/details.aspx?familyid=9216652f-51e0-402e-b7b5-feb68d00f298&displaylang=en
    Again register the links in VC6, and check inside vc6 if headers are in second place right after SDk 2003
	
  *If you have 7.0(VC.NET/2001) or 7.1(VC.2003) *
  then I suggest you open portaudio.dsp (and convert if needed)
 
  *If you have Visual Studio 2005 * (or later), I suggest you open the portaudio.sln file
  which contains 2 projects (portaudio & portaudio_static) each with 6 configurations: Win32/x64 in both Debug, Release and ReleaseMinDependency,
  last of which removes dependency of all but basic OS system DLLs.

  hit compile and hope for the best.
 
3)Now in any  project, in which you require portaudio,
  you can just link with portaudio_x86.lib, (or _x64) and of course include the 
  relevant headers
  (portaudio.h, and/or pa_asio.h , pa_x86_plain_converters.h) See (*)
  
4) Your new exe should now use portaudio_xXX.dll.


Have fun!

(*): you may want to add/remove some DLL entry points.
Right now those 6 entries are _not_ from portaudio.h

(from portaudio.def)
(...)
PaAsio_GetAvailableLatencyValues    @50
PaAsio_ShowControlPanel             @51
PaUtil_InitializeX86PlainConverters @52
PaAsio_GetInputChannelName          @53
PaAsio_GetOutputChannelName         @54
PaUtil_SetLogPrintFunction          @55


*** Building without ASIO support ***

To build PortAudio without ASIO support you need to:
  A. Make sure your project doesn't try to build any ASIO SDK files.
     If you're using one of the shipped projects, remove the ASIO related files 
     from the project.

  B. Make sure your project doesn't try to build the PortAudio ASIO
     implementation files:
	      src/hostapi/pa_asio.cpp
        src/hostapi/iasiothiscallresolver.cpp
     If you're using one of the shipped projects remove them from the project.

  C. Set the PA_USE_ASIO preprocessor symbol to zero (i.e. PA_USE_ASIO=0) in the project properties.
     In VS2005 this can be added under
     Project Properties > Configuration Properties > C/C++ > Preprocessor > Preprocessor Definitions

     Setting PA_USE_ASIO=0 stops src/os/win/pa_win_hostapis.c 
     from trying to initialize the PA ASIO implementation.

  D. Remove PaAsio_* entry points from portaudio.def, or comment them out with ;


A similar procedure can be used to omit any of the other host APIs from the 
build. The relevant preprocessor symbols used by pa_win_hostapis.c are:
PA_USE_WMME, PA_USE_DSOUND, PA_USE_ASIO, PA_USE_WASAPI and PA_USE_WDMKS

-----
David Viens, davidv@plogue.com
Robert Bielik, robert@xponaut.se
