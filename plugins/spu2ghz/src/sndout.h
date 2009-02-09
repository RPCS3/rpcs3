//GiGaHeRz's SPU2 Driver
//Copyright (c) 2003-2008, David Quintana <gigaherz@gmail.com>
//
//This library is free software; you can redistribute it and/or
//modify it under the terms of the GNU Lesser General Public
//License as published by the Free Software Foundation; either
//version 2.1 of the License, or (at your option) any later version.
//
//This library is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//Lesser General Public License for more details.
//
//You should have received a copy of the GNU Lesser General Public
//License along with this library; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
#ifndef SNDOUT_H_INCLUDE
#define SNDOUT_H_INCLUDE

// Number of stereo samples per SndOut block.
// All drivers must work in units of this size when communicating with
// SndOut.
static const int SndOutPacketSize = 1024;

static const int SndOutVolumeShiftBase = 8;
extern int SndOutVolumeShift;

#define pcmlog
extern FILE *wavelog;

s32  SndInit();
void SndClose();
s32  SndWrite(s32 ValL, s32 ValR);
s32  SndTest();
void SndConfigure(HWND parent, u32 outmodidx );
bool SndGetStats(u32 *written, u32 *played);

int FindOutputModuleById( const char* omodid );

class SndBuffer
{
public:
	virtual ~SndBuffer() {}

	virtual void WriteSamples(s32 *buffer, int nSamples)=0;
	virtual void PauseOnWrite(bool doPause)=0;

	virtual void ReadSamples( s16* bData )=0;
	virtual void ReadSamples( s32* bData )=0;

	//virtual s32  GetBufferUsage()=0;
	//virtual s32  GetBufferSize()=0;
};

class SndOutModule
{
public:
	// Virtual destructor, because it helps fight C+++ funny-business.
	virtual ~SndOutModule(){};

	// Returns a unique identification string for this driver.
	// (usually just matches the driver's cpp filename)
	virtual const char* GetIdent() const=0;

	// Returns the long name / description for this driver.
	// (for use in configuration screen)
	virtual const char* GetLongName() const=0;

	virtual s32  Init(SndBuffer *buffer)=0;
	virtual void Close()=0;
	virtual s32  Test() const=0;
	virtual void Configure(HWND parent)=0;
	virtual bool Is51Out() const=0;

	// Returns the number of empty samples in the output buffer.
	// (which is effectively the amount of data played since the last update)
	virtual int GetEmptySampleCount() const=0;
};

//internal
extern SndOutModule *WaveOut;
extern SndOutModule *DSoundOut;
extern SndOutModule *FModOut;
extern SndOutModule *ASIOOut;
extern SndOutModule *XAudio2Out;
extern SndOutModule *DSound51Out;

extern SndOutModule* mods[];

#endif // SNDOUT_H_INCLUDE
