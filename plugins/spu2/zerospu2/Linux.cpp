/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "zerospu2.h"

#include <assert.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <unistd.h>

#ifdef ZEROSPU2_OSS

static int oss_audio_fd = -1;
extern int errno;

#define OSS_MODE_STEREO	1

// use OSS for sound
int SetupSound()
{
    int pspeed=48000;
    int pstereo;
    int format;
    int fragsize = 0;
    int myfrag;
    int oss_speed, oss_stereo;
    
    pstereo=OSS_MODE_STEREO;
    
    oss_speed = pspeed;
    oss_stereo = pstereo;
    
    if((oss_audio_fd=open("/dev/dsp",O_WRONLY,0))==-1) {
        printf("Sound device not available!\n");
        return -1;
    }
    
    if(ioctl(oss_audio_fd,SNDCTL_DSP_RESET,0)==-1) {
        printf("Sound reset failed\n");
        return -1;
    }
    
    // we use 64 fragments with 1024 bytes each
    fragsize=10;
    myfrag=(63<<16)|fragsize;
    
    if(ioctl(oss_audio_fd,SNDCTL_DSP_SETFRAGMENT,&myfrag)==-1) {
        printf("Sound set fragment failed!\n");
        return -1;        
    }
    
    format = AFMT_S16_LE;
    
    if(ioctl(oss_audio_fd,SNDCTL_DSP_SETFMT,&format) == -1) {
        printf("Sound format not supported!\n");
        return -1;
    }
    
    if(format!=AFMT_S16_LE) {
        printf("Sound format not supported!\n");
        return -1;
    }
    
    if(ioctl(oss_audio_fd,SNDCTL_DSP_STEREO,&oss_stereo)==-1) {
        printf("Stereo mode not supported!\n");
        return -1;
    }
    
    if(ioctl(oss_audio_fd,SNDCTL_DSP_SPEED,&oss_speed)==-1) {
        printf("Sound frequency not supported\n");
        return -1;
    }
    
    if(oss_speed!=pspeed) {
        printf("Sound frequency not supported\n");
        return -1;
    }

    return 0;
}

// REMOVE SOUND
void RemoveSound()
{
    if(oss_audio_fd != -1 ) {
        close(oss_audio_fd);
        oss_audio_fd = -1;
    }
}

#define SOUNDSIZE 76800
int SoundGetBytesBuffered()
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
void SoundFeedVoiceData(unsigned char* pSound,long lBytes)
{
    if(oss_audio_fd == -1) return;
    write(oss_audio_fd,pSound,lBytes);
}

#else
// ALSA
#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API
#include <alsa/asoundlib.h>

#define ALSA_MEM_DEF

#ifdef ALSA_MEM_DEF
#define ALSA_MEM_EXTERN
#else
#define ALSA_MEM_EXTERN extern
#endif

static snd_pcm_t *handle = NULL;
static snd_pcm_uframes_t buffer_size;

#define SOUNDSIZE 500000

int SetupSound(void)
{
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_status_t *status;
    unsigned int pspeed;
    int pchannels;
    snd_pcm_format_t format;
    unsigned int buffer_time, period_time;
    int err;
    
    pchannels=2;
    
    pspeed=48000;
    format=SND_PCM_FORMAT_S16_LE;
    buffer_time=SOUNDSIZE;
    period_time=buffer_time/4;
    
    if((err=snd_pcm_open(&handle, "default", 
                         SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK))<0) {
        printf("Audio open error: %s\n", snd_strerror(err));
        return -1;
    }
    
    if((err=snd_pcm_nonblock(handle, 0))<0) {
        printf("Can't set blocking moded: %s\n", snd_strerror(err));
        return -1;
    }
    
    snd_pcm_hw_params_alloca(&hwparams);
    snd_pcm_sw_params_alloca(&swparams);
    if((err=snd_pcm_hw_params_any(handle, hwparams))<0) {
        printf("Broken configuration for this PCM: %s\n", snd_strerror(err));
        return -1;
    }
    
    if((err=snd_pcm_hw_params_set_access(handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED))<0) {
        printf("Access type not available: %s\n", snd_strerror(err));
        return -1;
    }

    if((err=snd_pcm_hw_params_set_format(handle, hwparams, format))<0) {
        printf("Sample format not available: %s\n", snd_strerror(err));
        return -1;
    }
    
    if((err=snd_pcm_hw_params_set_channels(handle, hwparams, pchannels))<0) {
        printf("Channels count not available: %s\n", snd_strerror(err));
        return -1;
    }
    
    if((err=snd_pcm_hw_params_set_rate_near(handle, hwparams, &pspeed, 0))<0) {
        printf("Rate not available: %s\n", snd_strerror(err));
        return -1;
    }
    
    if((err=snd_pcm_hw_params_set_buffer_time_near(handle, hwparams, &buffer_time, 0))<0) {
        printf("Buffer time error: %s\n", snd_strerror(err));
        return -1;
    }
    
    if((err=snd_pcm_hw_params_set_period_time_near(handle, hwparams, &period_time, 0))<0) {
        printf("Period time error: %s\n", snd_strerror(err));
        return -1;
    }

    if((err=snd_pcm_hw_params(handle, hwparams))<0) {
        printf("Unable to install hw params: %s\n", snd_strerror(err));
        return -1;
    }
    
    snd_pcm_status_alloca(&status);
    if((err=snd_pcm_status(handle, status))<0) {
        printf("Unable to get status: %s\n", snd_strerror(err));
        return -1;
    }
    
    buffer_size=snd_pcm_status_get_avail(status);

    return 0;
}

void RemoveSound()
{
    if(handle != NULL) {
        snd_pcm_drop(handle);
        snd_pcm_close(handle);
        handle = NULL;
    }
}

int SoundGetBytesBuffered()
{
    int l;
    
    if(handle == NULL)                                 // failed to open?
        return SOUNDSIZE;
    l = snd_pcm_avail_update(handle);
    if(l<0) return 0;
    if(l<buffer_size/2)                                 // can we write in at least the half of fragments?
        l=SOUNDSIZE;                                   // -> no? wait
    else l=0;                                           // -> else go on
    
    return l;
}

void SoundFeedVoiceData(unsigned char* pSound,long lBytes)
{
    if(handle == NULL) return;
    
    if(snd_pcm_state(handle) == SND_PCM_STATE_XRUN)
        snd_pcm_prepare(handle);
    snd_pcm_writei(handle,pSound, lBytes/4);
}

#endif

GtkWidget *MsgDlg;

void OnMsg_Ok() {
	gtk_widget_destroy(MsgDlg);
	gtk_main_quit();
}

void SysMessage(char *fmt, ...) {
	GtkWidget *Ok,*Txt;
	GtkWidget *Box,*Box1;
	va_list list;
	char msg[512];

	va_start(list, fmt);
	vsprintf(msg, fmt, list);
	va_end(list);

	if (msg[strlen(msg)-1] == '\n') msg[strlen(msg)-1] = 0;
    
	MsgDlg = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(MsgDlg), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW(MsgDlg), "SPU2null Msg");
	gtk_container_set_border_width(GTK_CONTAINER(MsgDlg), 5);

	Box = gtk_vbox_new(5, 0);
	gtk_container_add(GTK_CONTAINER(MsgDlg), Box);
	gtk_widget_show(Box);

	Txt = gtk_label_new(msg);
	
	gtk_box_pack_start(GTK_BOX(Box), Txt, FALSE, FALSE, 5);
	gtk_widget_show(Txt);

	Box1 = gtk_hbutton_box_new();
	gtk_box_pack_start(GTK_BOX(Box), Box1, FALSE, FALSE, 0);
	gtk_widget_show(Box1);

	Ok = gtk_button_new_with_label("Ok");
	gtk_signal_connect (GTK_OBJECT(Ok), "clicked", GTK_SIGNAL_FUNC(OnMsg_Ok), NULL);
	gtk_container_add(GTK_CONTAINER(Box1), Ok);
	GTK_WIDGET_SET_FLAGS(Ok, GTK_CAN_DEFAULT);
	gtk_widget_show(Ok);

	gtk_widget_show(MsgDlg);	

	gtk_main();
}

void CALLBACK SPU2configure() {
	SysMessage("Nothing to Configure");
}

extern char* libraryName;
extern string s_strIniPath;

void CALLBACK SPU2about() {
	SysMessage("%s %d.%d\ndeveloper: zerofrog", libraryName, SPU2_VERSION, SPU2_BUILD);
}

void SaveConfig() {
    FILE *f;
    char cfg[255];

    strcpy(cfg, s_strIniPath.c_str());
    f = fopen(cfg,"w");
    if (f == NULL) {
        printf("Failed to open %s\n", s_strIniPath.c_str());
        return;
    }
    fprintf(f, "log = %d\n", conf.Log);
    fprintf(f, "options = %d\n", conf.options);
    fclose(f);
}

void LoadConfig() {
    FILE *f;
    char cfg[255];

    memset(&conf, 0, sizeof(conf));

    strcpy(cfg, s_strIniPath.c_str());
    f = fopen(cfg, "r");
    if (f == NULL) {
        printf("Failed to open %s\n", s_strIniPath.c_str());
        conf.Log = 0;//default value
        conf.options = 0;//OPTION_TIMESTRETCH;
        SaveConfig();//save and return
        return;
    }
	fscanf(f, "log = %d\n", &conf.Log);
    fscanf(f, "options = %d\n", &conf.options);
    fclose(f);
}
