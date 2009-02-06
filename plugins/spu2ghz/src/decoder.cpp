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

extern "C" {
#include "liba52/inttypes.h"
#include "liba52/a52.h"
#include "liba52/mm_accel.h"
}

extern u32 spdif_read_data(u8 *buff, u32 max_data);

#define DATA_SIZE 0x100000

u32 use51=0;

u8  databuffer[DATA_SIZE];
u32 data_in_buffer;

s32 output_buffer[0x10000][6];
s32 output_write_cursor=0;
s32 output_read_cursor=0;
s32 output_buffer_data=0;

int flags,srate,bitrate;

#define CHANNEL_CENTER 1
#define CHANNEL_STEREO 2
#define CHANNEL_SURROUND 4
#define CHANNEL_LFE 8

int sample_flags = 0;

u32 frame_size;

a52_state_t* ac3dec;
sample_t    *decode_buffer = NULL;

s32 data_rate=4;

int state=0;

FILE *fSpdifDump;

extern u32 core;
void __fastcall ReadInput(V_Core& thiscore, s32& PDataL,s32& PDataR);

union spdif_frame { // total size: 32bits
	struct {
		u32 preamble:4;		//4
		u32 databits:24;	//28
		u32 valid:1;		//29
		u32 subcode:1;		//30
		u32 chanstat:1;		//31
		u32 parity:1;		//32 // parity not including preamble
	} bits;
	u32 whole;
};

union spdif_block {
	spdif_frame frames[192];
	u8			bytes [192*4];
};

/*
spdif_block bbuffer[2];
u8 *bbuff = bbuffer[0].bytes;
u32 bbuff_bytes = 0;
*/

bool check_frame(spdif_frame f)
{
	u32 w = f.whole>>4;
	u32 t = 0;

	for(int i=0;i>28;i++)
	{
		t=t^(w&1);
		w>>=1;
	}

	return (t==0)&&(f.bits.valid);
}

void spdif_Write(s32 data)
{
	spdif_frame f;

	f.whole=data;

	if(check_frame(f))
	{
		int dec = f.bits.databits;
		databuffer[data_in_buffer++]=(dec    )&0xFF;
		databuffer[data_in_buffer++]=(dec>> 8)&0xFF;
		databuffer[data_in_buffer++]=(dec>>16)&0xFF;
	}
}

void spdif_remove_data(unsigned int bytes)
{
	if(bytes<data_in_buffer)
	{
		memcpy(databuffer,databuffer+bytes,data_in_buffer-bytes);
	}
	data_in_buffer-=bytes;
}

s32 stoi(sample_t n) //input: [-1..1]
{
	s32 sign=(n<0) + (n>0)*-1; //1 if positive, -1 if negative, 0 otherwise
	n=abs(n)+1; //make it [1..2]
	s32 k=*(s32*)&n;
	k=k&0x7FFFFF;
	return k*sign;
}

void spdif_update()
{
	s32 Data,Zero;

	core=0;
	V_Core& thiscore( Cores[core] );
	for(int i=0;i<data_rate;i++)
	{
		ReadInput(thiscore, Data,Zero);
		
		if(fSpdifDump)
		{
			fwrite(&Data,4,1,fSpdifDump);
			fwrite(&Zero,4,1,fSpdifDump);
		}

		if(ac3dec)
			spdif_Write(Data);
	}

	if(!ac3dec) return;

	if(state==0)
	{
		if(data_in_buffer<7) return;

		if(databuffer[0]!=0)
		{
			flags=0;
		}
		frame_size = a52_syncinfo (databuffer, &flags, &srate, &bitrate);
		if(frame_size==0)
		{
			spdif_remove_data(1);
		}
		else
		{
			state=1;
		}
	}
	if(state==1)
	{
		if(data_in_buffer<frame_size) return;

		flags = A52_ADJUST_LEVEL;

		if(use51) flags|=A52_3F2R|A52_LFE;
		else	  flags|=A52_STEREO;

		sample_t level=1;
		a52_frame(ac3dec,databuffer,&flags,&level,0);

		//decode
		for(int i=0;i<6;i++)
		{
			a52_block(ac3dec);

			// processing
			for(int j=0;j<256;j++)
			{
				s32* samples = output_buffer[output_write_cursor];
				sample_flags=0;

				int output_cursor=j;

				int n=0;
				if(flags&A52_LFE) 
				{
					sample_flags|=CHANNEL_LFE;

					samples[3] = stoi(decode_buffer[(0<<8)+output_cursor]); //lfe
					n=1;
				}

				switch(flags&(~A52_LFE))
				{
				case A52_STEREO:
					sample_flags |= CHANNEL_STEREO;
					samples[0] = stoi(decode_buffer[((n+0)<<8)+output_cursor]); //l
					samples[1] = stoi(decode_buffer[((n+1)<<8)+output_cursor]); //r
					samples[2] = 0; //c
					samples[4] = 0; //sl
					samples[5] = 0; //sr
					break;
				case A52_2F1R:
					sample_flags |= CHANNEL_STEREO|CHANNEL_SURROUND;
					samples[0] = stoi(decode_buffer[((n+0)<<8)+output_cursor]); //l
					samples[1] = stoi(decode_buffer[((n+1)<<8)+output_cursor]); //r
					samples[2] = 0; //c
					samples[4] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //sl
					samples[5] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //sr
					break;
				case A52_2F2R:
					sample_flags |= CHANNEL_STEREO|CHANNEL_SURROUND;
					samples[0] = stoi(decode_buffer[((n+0)<<8)+output_cursor]); //l
					samples[1] = stoi(decode_buffer[((n+1)<<8)+output_cursor]); //r
					samples[2] = 0; //c
					samples[4] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //sl
					samples[5] = stoi(decode_buffer[((n+3)<<8)+output_cursor]); //sr
					break;
				case A52_3F:
					sample_flags |= CHANNEL_STEREO|CHANNEL_CENTER;
					samples[0] = stoi(decode_buffer[((n+0)<<8)+output_cursor]); //l
					samples[1] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //r
					samples[2] = stoi(decode_buffer[((n+1)<<8)+output_cursor]); //c
					samples[4] = 0; //sl
					samples[5] = 0; //sr
					break;
				case A52_3F1R:
					sample_flags |= CHANNEL_STEREO|CHANNEL_SURROUND|CHANNEL_CENTER;
					samples[0] = stoi(decode_buffer[((n+0)<<8)+output_cursor]); //l
					samples[1] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //r
					samples[2] = stoi(decode_buffer[((n+1)<<8)+output_cursor]); //c
					samples[4] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //sl
					samples[5] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //sr
					break;
				case A52_3F2R:
					sample_flags |= CHANNEL_STEREO|CHANNEL_SURROUND|CHANNEL_CENTER;
					samples[0] = stoi(decode_buffer[((n+0)<<8)+output_cursor]); //l
					samples[1] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //r
					samples[2] = stoi(decode_buffer[((n+1)<<8)+output_cursor]); //c
					samples[4] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //sl
					samples[5] = stoi(decode_buffer[((n+2)<<8)+output_cursor]); //sr
					break;
				default:
					samples[0] = stoi(decode_buffer[((n+0)<<8)+output_cursor]); //l
					samples[1] = stoi(decode_buffer[((n+1)<<8)+output_cursor]); //r
					samples[2] = 0; //c
					samples[4] = 0; //sl
					samples[5] = 0; //sr
					break;
				}

				output_write_cursor=(output_write_cursor+1)&0xFFFF;
				output_buffer_data++;
			}
		}
		spdif_remove_data(frame_size);
	}
}

u32  spdif_init()
{
	data_rate=1; // words/tick

	ac3dec = a52_init(0);
	if(!ac3dec) return 1;

	decode_buffer = a52_samples(ac3dec);
	a52_dynrng (ac3dec, NULL, NULL);
	data_in_buffer=0;

	fSpdifDump = fopen("logs/spdif.dat","wb");
	return 0;
}

void spdif_set51(u32 is_5_1_out)
{
	use51 = is_5_1_out;
}

void spdif_shutdown()
{
	if(ac3dec) a52_free(ac3dec);
	ac3dec=NULL;
	if(fSpdifDump)
		fclose(fSpdifDump);
}

int spdif_decode_one(s32 *channels)
{
	channels[0]=0;
	channels[1]=0;
	channels[2]=0;
	channels[3]=0;
	channels[4]=0;
	channels[5]=0;

	if(output_buffer_data==0) return 0;

	memcpy(channels,output_buffer[output_read_cursor],4*6);
	output_read_cursor=(output_read_cursor+1)&0xFFFF;
	output_buffer_data--;

	return sample_flags;
}

void spdif_get_samples(s32*samples)
{
	s32 channels[6];
	s32 flags = spdif_decode_one(channels);

	if(use51)
	{
		samples[0]=0;
		samples[1]=0;
		samples[2]=0;
		samples[3]=0;
		samples[4]=0;
		samples[5]=0;

		if(flags&CHANNEL_STEREO)
		{
			samples[0]=channels[0];
			samples[1]=channels[1];
		}

		if(flags&CHANNEL_CENTER)
		{
			samples[2]=channels[2];
		}

		if(flags&CHANNEL_LFE)
		{
			samples[3]=channels[3];
		}

		if(flags&CHANNEL_SURROUND)
		{
			samples[4]=channels[4];
			samples[5]=channels[5];
		}
	}
	else // downmix to stereo (no DPL2 encoding... yet ;)
	{
		samples[0]=0;
		samples[1]=0;

		if(flags&CHANNEL_STEREO)
		{
			samples[0]=channels[0];
			samples[1]=channels[1];
		}

		if(flags&CHANNEL_CENTER)
		{
			samples[0]+=channels[2];
			samples[1]+=channels[2];
		}

		if(flags&CHANNEL_LFE)
		{
			samples[0]+=channels[3];
			samples[1]+=channels[3];
		}

		if(flags&CHANNEL_SURROUND)
		{
			samples[0]+=channels[4];
			samples[1]+=channels[5];
		}
	}
}
