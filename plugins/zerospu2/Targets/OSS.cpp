/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

 // Modified by arcum42@gmail.com

#include "Linux.h"
#include "OSS.h"

static int oss_audio_fd = -1;
extern int errno;

// use OSS for sound
int OSSSetupSound()
{
	int pspeed = SAMPLE_RATE;
	int pstereo;
	int format;
	int fragsize = 0;
	int myfrag;
	int oss_speed, oss_stereo;
	int err;

	pstereo=OSS_MODE_STEREO;

	oss_speed = pspeed;
	oss_stereo = pstereo;

	oss_audio_fd=open("/dev/dsp",O_WRONLY,0);
	if (oss_audio_fd == -1)
	{
		ERROR_LOG("Sound device not available!\n");
		return -1;
	}

	err= ioctl(oss_audio_fd,SNDCTL_DSP_RESET,0);
	if (err == -1)
	{
		ERROR_LOG("Sound reset failed\n");
		return -1;
	}

	// we use 64 fragments with 1024 bytes each
	fragsize = 10;
	myfrag = (63 << 16) | fragsize;
	err = ioctl(oss_audio_fd,SNDCTL_DSP_SETFRAGMENT,&myfrag);
	if (err == -1)
	{
		ERROR_LOG("Sound set fragment failed!\n");
		return -1;
	}

	format = AFMT_S16_LE;
	err = ioctl(oss_audio_fd,SNDCTL_DSP_SETFMT,&format);
	if ((err == -1) || (format!=AFMT_S16_LE))
	{
		ERROR_LOG("Sound format not supported!\n");
		return -1;
	}

	err = ioctl(oss_audio_fd,SNDCTL_DSP_STEREO,&oss_stereo);
	if (err == -1)
	{
		ERROR_LOG("Stereo mode not supported!\n");
		return -1;
	}

	err = ioctl(oss_audio_fd,SNDCTL_DSP_SPEED,&oss_speed);
	if ((err == -1) || (oss_speed!=pspeed))
	{
		ERROR_LOG("Sound frequency not supported\n");
		return -1;
	}

	return 0;
}

// REMOVE SOUND
void OSSRemoveSound()
{
	if(oss_audio_fd != -1 ) {
		close(oss_audio_fd);
		oss_audio_fd = -1;
	}
}

int OSSSoundGetBytesBuffered()
{
	audio_buf_info info;
	unsigned long l;

	if(oss_audio_fd == -1)
		return SOUNDSIZE;
	if(ioctl(oss_audio_fd,SNDCTL_DSP_GETOSPACE,&info)==-1)
		return 0;
	else {
        // can we write in at least the half of fragments?
		if(info.fragments<(info.fragstotal>>1))
			return SOUNDSIZE;
	}

	return 0;
}

// FEED SOUND DATA
void OSSSoundFeedVoiceData(unsigned char* pSound,long lBytes)
{
	if(oss_audio_fd == -1) return;
	write(oss_audio_fd,pSound,lBytes);
}
