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

#include "spu2.h"
#include <stdio.h>

#define WAVONLY

typedef struct {
	//Main Header
  char           riffID[4];
  long           riffSize;
  char           riffTYPE[4];
	//Format Tag
  char           chunkID[4];
  long           chunkSize;
  short          wFormatTag;
  unsigned short wChannels;
  unsigned long  dwSamplesPerSec;
  unsigned long  dwAvgBytesPerSec;
  unsigned short wBlockAlign;
  unsigned short wBitsPerSample;
	//Data Tag
  char           dataID[4];
  long           dataSize;
} WAVEHeader;

int wavedump_ok=0;
int datasize;
FILE *fdump;

#ifndef WAVONLY
int flacdump_open();
void flacdump_close();
void flacdump_write(s16 left,s16 right);
int oggvdump_open();
void oggvdump_close();
void oggvdump_write(s16 left,s16 right);
#endif

int wavedump_open()
{
#ifndef WAVONLY
	if(WaveDumpFormat==1) return flacdump_open();
	if(WaveDumpFormat==2) return oggvdump_open();
#endif

	fdump=fopen(WaveLogFileName,"wb");
	if(fdump==NULL) return 0;

	fseek(fdump,sizeof(WAVEHeader),SEEK_SET);

	datasize=0;
	wavedump_ok=1;
	return 1;
}

void wavedump_flush()
{
	WAVEHeader w;

	memcpy(w.riffID,"RIFF",4);
	w.riffSize=datasize+36;
	memcpy(w.riffTYPE,"WAVE",4);
	memcpy(w.chunkID,"fmt ",4);
	w.chunkSize=0x10;
	w.wFormatTag=1;
	w.wChannels=2;
	w.dwSamplesPerSec=48000;
	w.dwAvgBytesPerSec=48000*4;
  	w.wBlockAlign=4;
	w.wBitsPerSample=16;
	memcpy(w.dataID,"data",4);
	w.dataSize=datasize;

	fseek(fdump,0,SEEK_SET);
	fwrite(&w,sizeof(w),1,fdump);

	fseek(fdump,datasize+sizeof(w),SEEK_SET);
}

void wavedump_close()
{
	if(!wavedump_ok) return;

	wavedump_flush();
#ifndef WAVONLY
	if(WaveDumpFormat==1) { flacdump_close(); return;}
	if(WaveDumpFormat==2) { oggvdump_close(); return;}
#endif


	fclose(fdump);
	wavedump_ok=0;
}

void wavedump_write(s16 left,s16 right)
{
	s16 buffer[2]={left,right};

	if(!wavedump_ok) return;

#ifndef WAVONLY
	if(WaveDumpFormat==1) return flacdump_write(left,right);
	if(WaveDumpFormat==2) return oggvdump_write(left,right);
#endif
	datasize+=4;

	fwrite(buffer,4,1,fdump);

	if((datasize&1023)==0)
		wavedump_flush();
}

FILE *recordFile;
int   recordSize;
int recording;

void RecordStart()
{
	if(recording&&recordFile)
		fclose(recordFile);

	recordFile=fopen("recording.wav","wb");
	if(recordFile==NULL) return;

	fseek(recordFile,sizeof(WAVEHeader),SEEK_SET);

	recordSize=0;
	recording=1;
}

void RecordFlush()
{
	WAVEHeader w;

	memcpy(w.riffID,"RIFF",4);
	w.riffSize=recordSize+36;
	memcpy(w.riffTYPE,"WAVE",4);
	memcpy(w.chunkID,"fmt ",4);
	w.chunkSize=0x10;
	w.wFormatTag=1;
	w.wChannels=2;
	w.dwSamplesPerSec=48000;
	w.dwAvgBytesPerSec=48000*4;
  	w.wBlockAlign=4;
	w.wBitsPerSample=16;
	memcpy(w.dataID,"data",4);
	w.dataSize=recordSize;

	fseek(recordFile,0,SEEK_SET);
	fwrite(&w,sizeof(w),1,recordFile);
	fseek(recordFile,recordSize+sizeof(w),SEEK_SET);
}

void RecordStop()
{
	if(!recording)
		return;
	recording=0;

	RecordFlush();

	fclose(recordFile);
}

void RecordWrite(s16 left, s16 right)
{
	if(!recording)
		return;

	s16 buffer[2]={left,right};

	recordSize+=4;

	fwrite(buffer,4,1,recordFile);

	if((recordSize&1023)==0)
		RecordFlush();

}
