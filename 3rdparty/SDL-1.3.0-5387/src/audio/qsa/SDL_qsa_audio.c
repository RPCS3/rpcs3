/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include "SDL_config.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sched.h>
#include <sys/select.h>
#include <sys/neutrino.h>
#include <sys/asoundlib.h>

#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "SDL_qsa_audio.h"

/* The tag name used by QSA audio framework */
#define DRIVER_NAME "qsa"

/* default channel communication parameters */
#define DEFAULT_CPARAMS_RATE   44100
#define DEFAULT_CPARAMS_VOICES 1

#define DEFAULT_CPARAMS_FRAG_SIZE 4096
#define DEFAULT_CPARAMS_FRAGS_MIN 1
#define DEFAULT_CPARAMS_FRAGS_MAX 1

#define QSA_NO_WORKAROUNDS  0x00000000
#define QSA_MMAP_WORKAROUND 0x00000001

struct BuggyCards
{
    char *cardname;
    unsigned long bugtype;
};

#define QSA_WA_CARDS             3
#define QSA_MAX_CARD_NAME_LENGTH 33

struct BuggyCards buggycards[QSA_WA_CARDS] = {
    {"Sound Blaster Live!", QSA_MMAP_WORKAROUND},
    {"Vortex 8820", QSA_MMAP_WORKAROUND},
    {"Vortex 8830", QSA_MMAP_WORKAROUND},
};

/* List of found devices */
#define QSA_MAX_DEVICES       32
#define QSA_MAX_NAME_LENGTH   81+16     /* Hardcoded in QSA, can't be changed */

typedef struct _QSA_Device
{
    char name[QSA_MAX_NAME_LENGTH];     /* Long audio device name for SDL  */
    int cardno;
    int deviceno;
} QSA_Device;

QSA_Device qsa_playback_device[QSA_MAX_DEVICES];
uint32_t qsa_playback_devices;

QSA_Device qsa_capture_device[QSA_MAX_DEVICES];
uint32_t qsa_capture_devices;

static inline void
QSA_SetError(const char *fn, int status)
{
    SDL_SetError("QSA: %s() failed: %s", fn, snd_strerror(status));
}

/* card names check to apply the workarounds */
static int
QSA_CheckBuggyCards(_THIS, unsigned long checkfor)
{
    char scardname[QSA_MAX_CARD_NAME_LENGTH];
    int it;

    if (snd_card_get_name
        (this->hidden->cardno, scardname, QSA_MAX_CARD_NAME_LENGTH - 1) < 0) {
        return 0;
    }

    for (it = 0; it < QSA_WA_CARDS; it++) {
        if (SDL_strcmp(buggycards[it].cardname, scardname) == 0) {
            if (buggycards[it].bugtype == checkfor) {
                return 1;
            }
        }
    }

    return 0;
}

static void
QSA_ThreadInit(_THIS)
{
    struct sched_param param;
    int status;

    /* Increase default 10 priority to 25 to avoid jerky sound */
    status = SchedGet(0, 0, &param);
    param.sched_priority = param.sched_curpriority + 15;
    status = SchedSet(0, 0, SCHED_NOCHANGE, &param);
}

/* PCM channel parameters initialize function */
static void
QSA_InitAudioParams(snd_pcm_channel_params_t * cpars)
{
    SDL_memset(cpars, 0, sizeof(snd_pcm_channel_params_t));

    cpars->channel = SND_PCM_CHANNEL_PLAYBACK;
    cpars->mode = SND_PCM_MODE_BLOCK;
    cpars->start_mode = SND_PCM_START_DATA;
    cpars->stop_mode = SND_PCM_STOP_STOP;
    cpars->format.format = SND_PCM_SFMT_S16_LE;
    cpars->format.interleave = 1;
    cpars->format.rate = DEFAULT_CPARAMS_RATE;
    cpars->format.voices = DEFAULT_CPARAMS_VOICES;
    cpars->buf.block.frag_size = DEFAULT_CPARAMS_FRAG_SIZE;
    cpars->buf.block.frags_min = DEFAULT_CPARAMS_FRAGS_MIN;
    cpars->buf.block.frags_max = DEFAULT_CPARAMS_FRAGS_MAX;
}

/* This function waits until it is possible to write a full sound buffer */
static void
QSA_WaitDevice(_THIS)
{
    fd_set wfds;
    fd_set rfds;
    int selectret;
    struct timeval timeout;

    if (!this->hidden->iscapture) {
        FD_ZERO(&wfds);
        FD_SET(this->hidden->audio_fd, &wfds);
    } else {
        FD_ZERO(&rfds);
        FD_SET(this->hidden->audio_fd, &rfds);
    }

    do {
        /* Setup timeout for playing one fragment equal to 2 seconds          */
        /* If timeout occured than something wrong with hardware or driver    */
        /* For example, Vortex 8820 audio driver stucks on second DAC because */
        /* it doesn't exist !                                                 */
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        this->hidden->timeout_on_wait = 0;

        if (!this->hidden->iscapture) {
            selectret =
                select(this->hidden->audio_fd + 1, NULL, &wfds, NULL,
                       &timeout);
        } else {
            selectret =
                select(this->hidden->audio_fd + 1, &rfds, NULL, NULL,
                       &timeout);
        }

        switch (selectret) {
        case -1:
            {
                SDL_SetError("QSA: select() failed: %s\n", strerror(errno));
                return;
            }
            break;
        case 0:
            {
                SDL_SetError("QSA: timeout on buffer waiting occured\n");
                this->hidden->timeout_on_wait = 1;
                return;
            }
            break;
        default:
            {
                if (!this->hidden->iscapture) {
                    if (FD_ISSET(this->hidden->audio_fd, &wfds)) {
                        return;
                    }
                } else {
                    if (FD_ISSET(this->hidden->audio_fd, &rfds)) {
                        return;
                    }
                }
            }
            break;
        }
    } while (1);
}

static void
QSA_PlayDevice(_THIS)
{
    snd_pcm_channel_status_t cstatus;
    int written;
    int status;
    int towrite;
    void *pcmbuffer;

    if ((!this->enabled) || (!this->hidden)) {
        return;
    }

    towrite = this->spec.size;
    pcmbuffer = this->hidden->pcm_buf;

    /* Write the audio data, checking for EAGAIN (buffer full) and underrun */
    do {
        written =
            snd_pcm_plugin_write(this->hidden->audio_handle, pcmbuffer,
                                 towrite);
        if (written != towrite) {
            /* Check if samples playback got stuck somewhere in hardware or in */
            /* the audio device driver */
            if ((errno == EAGAIN) && (written == 0)) {
                if (this->hidden->timeout_on_wait != 0) {
                    SDL_SetError("QSA: buffer playback timeout\n");
                    return;
                }
            }

            /* Check for errors or conditions */
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                /* Let a little CPU time go by and try to write again */
                SDL_Delay(1);

                /* if we wrote some data */
                towrite -= written;
                pcmbuffer += written * this->spec.channels;
                continue;
            } else {
                if ((errno == EINVAL) || (errno == EIO)) {
                    SDL_memset(&cstatus, 0, sizeof(cstatus));
                    if (!this->hidden->iscapture) {
                        cstatus.channel = SND_PCM_CHANNEL_PLAYBACK;
                    } else {
                        cstatus.channel = SND_PCM_CHANNEL_CAPTURE;
                    }

                    status =
                        snd_pcm_plugin_status(this->hidden->audio_handle,
                                              &cstatus);
                    if (status < 0) {
                        QSA_SetError("snd_pcm_plugin_status", status);
                        return;
                    }

                    if ((cstatus.status == SND_PCM_STATUS_UNDERRUN) ||
                        (cstatus.status == SND_PCM_STATUS_READY)) {
                        if (!this->hidden->iscapture) {
                            status =
                                snd_pcm_plugin_prepare(this->hidden->
                                                       audio_handle,
                                                       SND_PCM_CHANNEL_PLAYBACK);
                        } else {
                            status =
                                snd_pcm_plugin_prepare(this->hidden->
                                                       audio_handle,
                                                       SND_PCM_CHANNEL_CAPTURE);
                        }
                        if (status < 0) {
                            QSA_SetError("snd_pcm_plugin_prepare", status);
                            return;
                        }
                    }
                    continue;
                } else {
                    return;
                }
            }
        } else {
            /* we wrote all remaining data */
            towrite -= written;
            pcmbuffer += written * this->spec.channels;
        }
    } while ((towrite > 0) && (this->enabled));

    /* If we couldn't write, assume fatal error for now */
    if (towrite != 0) {
        this->enabled = 0;
    }
}

static Uint8 *
QSA_GetDeviceBuf(_THIS)
{
    return this->hidden->pcm_buf;
}

static void
QSA_CloseDevice(_THIS)
{
    if (this->hidden != NULL) {
        if (this->hidden->audio_handle != NULL) {
            if (!this->hidden->iscapture) {
                /* Finish playing available samples */
                snd_pcm_plugin_flush(this->hidden->audio_handle,
                                     SND_PCM_CHANNEL_PLAYBACK);
            } else {
                /* Cancel unread samples during capture */
                snd_pcm_plugin_flush(this->hidden->audio_handle,
                                     SND_PCM_CHANNEL_CAPTURE);
            }
            snd_pcm_close(this->hidden->audio_handle);
            this->hidden->audio_handle = NULL;
        }

        if (this->hidden->pcm_buf != NULL) {
            SDL_FreeAudioMem(this->hidden->pcm_buf);
            this->hidden->pcm_buf = NULL;
        }

        SDL_free(this->hidden);
        this->hidden = NULL;
    }
}

static int
QSA_OpenDevice(_THIS, const char *devname, int iscapture)
{
    int status = 0;
    int format = 0;
    SDL_AudioFormat test_format = 0;
    int found = 0;
    snd_pcm_channel_setup_t csetup;
    snd_pcm_channel_params_t cparams;

    /* Initialize all variables that we clean on shutdown */
    this->hidden =
        (struct SDL_PrivateAudioData *) SDL_calloc(1,
                                                   (sizeof
                                                    (struct
                                                     SDL_PrivateAudioData)));
    if (this->hidden == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden, 0, sizeof(struct SDL_PrivateAudioData));

    /* Initialize channel transfer parameters to default */
    QSA_InitAudioParams(&cparams);

    /* Initialize channel direction: capture or playback */
    this->hidden->iscapture = iscapture;

    /* Find deviceid and cardid by device name for playback */
    if ((!this->hidden->iscapture) && (devname != NULL)) {
        uint32_t device;
        int32_t status;

        /* Search in the playback devices */
        device = 0;
        do {
            status = SDL_strcmp(qsa_playback_device[device].name, devname);
            if (status == 0) {
                /* Found requested device */
                this->hidden->deviceno = qsa_playback_device[device].deviceno;
                this->hidden->cardno = qsa_playback_device[device].cardno;
                break;
            }
            device++;
            if (device >= qsa_playback_devices) {
                QSA_CloseDevice(this);
                SDL_SetError("No such playback device");
                return 0;
            }
        } while (1);
    }

    /* Find deviceid and cardid by device name for capture */
    if ((this->hidden->iscapture) && (devname != NULL)) {
        /* Search in the capture devices */
        uint32_t device;
        int32_t status;

        /* Searching in the playback devices */
        device = 0;
        do {
            status = SDL_strcmp(qsa_capture_device[device].name, devname);
            if (status == 0) {
                /* Found requested device */
                this->hidden->deviceno = qsa_capture_device[device].deviceno;
                this->hidden->cardno = qsa_capture_device[device].cardno;
                break;
            }
            device++;
            if (device >= qsa_capture_devices) {
                QSA_CloseDevice(this);
                SDL_SetError("No such capture device");
                return 0;
            }
        } while (1);
    }

    /* Check if SDL requested default audio device */
    if (devname == NULL) {
        /* Open system default audio device */
        if (!this->hidden->iscapture) {
            status = snd_pcm_open_preferred(&this->hidden->audio_handle,
                                            &this->hidden->cardno,
                                            &this->hidden->deviceno,
                                            SND_PCM_OPEN_PLAYBACK);
        } else {
            status = snd_pcm_open_preferred(&this->hidden->audio_handle,
                                            &this->hidden->cardno,
                                            &this->hidden->deviceno,
                                            SND_PCM_OPEN_CAPTURE);
        }
    } else {
        /* Open requested audio device */
        if (!this->hidden->iscapture) {
            status =
                snd_pcm_open(&this->hidden->audio_handle,
                             this->hidden->cardno, this->hidden->deviceno,
                             SND_PCM_OPEN_PLAYBACK);
        } else {
            status =
                snd_pcm_open(&this->hidden->audio_handle,
                             this->hidden->cardno, this->hidden->deviceno,
                             SND_PCM_OPEN_CAPTURE);
        }
    }

    /* Check if requested device is opened */
    if (status < 0) {
        this->hidden->audio_handle = NULL;
        QSA_CloseDevice(this);
        QSA_SetError("snd_pcm_open", status);
        return 0;
    }

    if (!QSA_CheckBuggyCards(this, QSA_MMAP_WORKAROUND)) {
        /* Disable QSA MMAP plugin for buggy audio drivers */
        status =
            snd_pcm_plugin_set_disable(this->hidden->audio_handle,
                                       PLUGIN_DISABLE_MMAP);
        if (status < 0) {
            QSA_CloseDevice(this);
            QSA_SetError("snd_pcm_plugin_set_disable", status);
            return 0;
        }
    }

    /* Try for a closest match on audio format */
    format = 0;
    /* can't use format as SND_PCM_SFMT_U8 = 0 in qsa */
    found = 0;

    for (test_format = SDL_FirstAudioFormat(this->spec.format); !found;) {
        /* if match found set format to equivalent QSA format */
        switch (test_format) {
        case AUDIO_U8:
            {
                format = SND_PCM_SFMT_U8;
                found = 1;
            }
            break;
        case AUDIO_S8:
            {
                format = SND_PCM_SFMT_S8;
                found = 1;
            }
            break;
        case AUDIO_S16LSB:
            {
                format = SND_PCM_SFMT_S16_LE;
                found = 1;
            }
            break;
        case AUDIO_S16MSB:
            {
                format = SND_PCM_SFMT_S16_BE;
                found = 1;
            }
            break;
        case AUDIO_U16LSB:
            {
                format = SND_PCM_SFMT_U16_LE;
                found = 1;
            }
            break;
        case AUDIO_U16MSB:
            {
                format = SND_PCM_SFMT_U16_BE;
                found = 1;
            }
            break;
        case AUDIO_S32LSB:
            {
                format = SND_PCM_SFMT_S32_LE;
                found = 1;
            }
            break;
        case AUDIO_S32MSB:
            {
                format = SND_PCM_SFMT_S32_BE;
                found = 1;
            }
            break;
        case AUDIO_F32LSB:
            {
                format = SND_PCM_SFMT_FLOAT_LE;
                found = 1;
            }
            break;
        case AUDIO_F32MSB:
            {
                format = SND_PCM_SFMT_FLOAT_BE;
                found = 1;
            }
            break;
        default:
            {
                break;
            }
        }

        if (!found) {
            test_format = SDL_NextAudioFormat();
        }
    }

    /* assumes test_format not 0 on success */
    if (test_format == 0) {
        QSA_CloseDevice(this);
        SDL_SetError("QSA: Couldn't find any hardware audio formats");
        return 0;
    }

    this->spec.format = test_format;

    /* Set the audio format */
    cparams.format.format = format;

    /* Set mono/stereo/4ch/6ch/8ch audio */
    cparams.format.voices = this->spec.channels;

    /* Set rate */
    cparams.format.rate = this->spec.freq;

    /* Setup the transfer parameters according to cparams */
    status = snd_pcm_plugin_params(this->hidden->audio_handle, &cparams);
    if (status < 0) {
        QSA_CloseDevice(this);
        QSA_SetError("snd_pcm_channel_params", status);
        return 0;
    }

    /* Make sure channel is setup right one last time */
    SDL_memset(&csetup, '\0', sizeof(csetup));
    if (!this->hidden->iscapture) {
        csetup.channel = SND_PCM_CHANNEL_PLAYBACK;
    } else {
        csetup.channel = SND_PCM_CHANNEL_CAPTURE;
    }

    /* Setup an audio channel */
    if (snd_pcm_plugin_setup(this->hidden->audio_handle, &csetup) < 0) {
        QSA_CloseDevice(this);
        SDL_SetError("QSA: Unable to setup channel\n");
        return 0;
    }

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    this->hidden->pcm_len = this->spec.size;

    if (this->hidden->pcm_len == 0) {
        this->hidden->pcm_len =
            csetup.buf.block.frag_size * this->spec.channels *
            (snd_pcm_format_width(format) / 8);
    }

    /*
     * Allocate memory to the audio buffer and initialize with silence
     *  (Note that buffer size must be a multiple of fragment size, so find
     *  closest multiple)
     */
    this->hidden->pcm_buf =
        (Uint8 *) SDL_AllocAudioMem(this->hidden->pcm_len);
    if (this->hidden->pcm_buf == NULL) {
        QSA_CloseDevice(this);
        SDL_OutOfMemory();
        return 0;
    }
    SDL_memset(this->hidden->pcm_buf, this->spec.silence,
               this->hidden->pcm_len);

    /* get the file descriptor */
    if (!this->hidden->iscapture) {
        this->hidden->audio_fd =
            snd_pcm_file_descriptor(this->hidden->audio_handle,
                                    SND_PCM_CHANNEL_PLAYBACK);
    } else {
        this->hidden->audio_fd =
            snd_pcm_file_descriptor(this->hidden->audio_handle,
                                    SND_PCM_CHANNEL_CAPTURE);
    }

    if (this->hidden->audio_fd < 0) {
        QSA_CloseDevice(this);
        QSA_SetError("snd_pcm_file_descriptor", status);
        return 0;
    }

    /* Prepare an audio channel */
    if (!this->hidden->iscapture) {
        /* Prepare audio playback */
        status =
            snd_pcm_plugin_prepare(this->hidden->audio_handle,
                                   SND_PCM_CHANNEL_PLAYBACK);
    } else {
        /* Prepare audio capture */
        status =
            snd_pcm_plugin_prepare(this->hidden->audio_handle,
                                   SND_PCM_CHANNEL_CAPTURE);
    }

    if (status < 0) {
        QSA_CloseDevice(this);
        QSA_SetError("snd_pcm_plugin_prepare", status);
        return 0;
    }

    /* We're really ready to rock and roll. :-) */
    return 1;
}

int
QSA_DetectDevices(int iscapture)
{
    uint32_t it;
    uint32_t cards;
    uint32_t devices;
    int32_t status;

    /* Detect amount of available devices       */
    /* this value can be changed in the runtime */
    cards = snd_cards();

    /* If io-audio manager is not running we will get 0 as number */
    /* of available audio devices                                 */
    if (cards == 0) {
        /* We have no any available audio devices */
        return 0;
    }

    /* Find requested devices by type */
    if (!iscapture) {
        /* Playback devices enumeration requested */
        for (it = 0; it < cards; it++) {
            devices = 0;
            do {
                status =
                    snd_card_get_longname(it,
                                          qsa_playback_device
                                          [qsa_playback_devices].name,
                                          QSA_MAX_NAME_LENGTH);
                if (status == EOK) {
                    snd_pcm_t *handle;

                    /* Add device number to device name */
                    sprintf(qsa_playback_device[qsa_playback_devices].name +
                            SDL_strlen(qsa_playback_device
                                       [qsa_playback_devices].name), " d%d",
                            devices);

                    /* Store associated card number id */
                    qsa_playback_device[qsa_playback_devices].cardno = it;

                    /* Check if this device id could play anything */
                    status =
                        snd_pcm_open(&handle, it, devices,
                                     SND_PCM_OPEN_PLAYBACK);
                    if (status == EOK) {
                        qsa_playback_device[qsa_playback_devices].deviceno =
                            devices;
                        status = snd_pcm_close(handle);
                        if (status == EOK) {
                            qsa_playback_devices++;
                        }
                    } else {
                        /* Check if we got end of devices list */
                        if (status == -ENOENT) {
                            break;
                        }
                    }
                } else {
                    break;
                }

                /* Check if we reached maximum devices count */
                if (qsa_playback_devices >= QSA_MAX_DEVICES) {
                    break;
                }
                devices++;
            } while (1);

            /* Check if we reached maximum devices count */
            if (qsa_playback_devices >= QSA_MAX_DEVICES) {
                break;
            }
        }
    } else {
        /* Capture devices enumeration requested */
        for (it = 0; it < cards; it++) {
            devices = 0;
            do {
                status =
                    snd_card_get_longname(it,
                                          qsa_capture_device
                                          [qsa_capture_devices].name,
                                          QSA_MAX_NAME_LENGTH);
                if (status == EOK) {
                    snd_pcm_t *handle;

                    /* Add device number to device name */
                    sprintf(qsa_capture_device[qsa_capture_devices].name +
                            SDL_strlen(qsa_capture_device
                                       [qsa_capture_devices].name), " d%d",
                            devices);

                    /* Store associated card number id */
                    qsa_capture_device[qsa_capture_devices].cardno = it;

                    /* Check if this device id could play anything */
                    status =
                        snd_pcm_open(&handle, it, devices,
                                     SND_PCM_OPEN_CAPTURE);
                    if (status == EOK) {
                        qsa_capture_device[qsa_capture_devices].deviceno =
                            devices;
                        status = snd_pcm_close(handle);
                        if (status == EOK) {
                            qsa_capture_devices++;
                        }
                    } else {
                        /* Check if we got end of devices list */
                        if (status == -ENOENT) {
                            break;
                        }
                    }

                    /* Check if we reached maximum devices count */
                    if (qsa_capture_devices >= QSA_MAX_DEVICES) {
                        break;
                    }
                } else {
                    break;
                }
                devices++;
            } while (1);

            /* Check if we reached maximum devices count */
            if (qsa_capture_devices >= QSA_MAX_DEVICES) {
                break;
            }
        }
    }

    /* Return amount of available playback or capture devices */
    if (!iscapture) {
        return qsa_playback_devices;
    } else {
        return qsa_capture_devices;
    }
}

const char *
QSA_GetDeviceName(int index, int iscapture)
{
    if (!iscapture) {
        if (index >= qsa_playback_devices) {
            return "No such playback device";
        }

        return qsa_playback_device[index].name;
    } else {
        if (index >= qsa_capture_devices) {
            return "No such capture device";
        }

        return qsa_capture_device[index].name;
    }
}

void
QSA_WaitDone(_THIS)
{
    if (!this->hidden->iscapture) {
        if (this->hidden->audio_handle != NULL) {
            /* Wait till last fragment is played and stop channel */
            snd_pcm_plugin_flush(this->hidden->audio_handle,
                                 SND_PCM_CHANNEL_PLAYBACK);
        }
    } else {
        if (this->hidden->audio_handle != NULL) {
            /* Discard all unread data and stop channel */
            snd_pcm_plugin_flush(this->hidden->audio_handle,
                                 SND_PCM_CHANNEL_CAPTURE);
        }
    }
}

void
QSA_Deinitialize(void)
{
    /* Clear devices array on shutdown */
    SDL_memset(qsa_playback_device, 0x00,
               sizeof(QSA_Device) * QSA_MAX_DEVICES);
    SDL_memset(qsa_capture_device, 0x00,
               sizeof(QSA_Device) * QSA_MAX_DEVICES);
    qsa_playback_devices = 0;
    qsa_capture_devices = 0;
}

static int
QSA_Init(SDL_AudioDriverImpl * impl)
{
    snd_pcm_t *handle = NULL;
    int32_t status = 0;

    /* Clear devices array */
    SDL_memset(qsa_playback_device, 0x00,
               sizeof(QSA_Device) * QSA_MAX_DEVICES);
    SDL_memset(qsa_capture_device, 0x00,
               sizeof(QSA_Device) * QSA_MAX_DEVICES);
    qsa_playback_devices = 0;
    qsa_capture_devices = 0;

    /* Set function pointers                                     */
    /* DeviceLock and DeviceUnlock functions are used default,   */
    /* provided by SDL, which uses pthread_mutex for lock/unlock */
    impl->DetectDevices = QSA_DetectDevices;
    impl->GetDeviceName = QSA_GetDeviceName;
    impl->OpenDevice = QSA_OpenDevice;
    impl->ThreadInit = QSA_ThreadInit;
    impl->WaitDevice = QSA_WaitDevice;
    impl->PlayDevice = QSA_PlayDevice;
    impl->GetDeviceBuf = QSA_GetDeviceBuf;
    impl->CloseDevice = QSA_CloseDevice;
    impl->WaitDone = QSA_WaitDone;
    impl->Deinitialize = QSA_Deinitialize;
    impl->LockDevice = NULL;
    impl->UnlockDevice = NULL;

    impl->OnlyHasDefaultOutputDevice = 0;
    impl->ProvidesOwnCallbackThread = 0;
    impl->SkipMixerLock = 0;
    impl->HasCaptureSupport = 1;
    impl->OnlyHasDefaultOutputDevice = 0;
    impl->OnlyHasDefaultInputDevice = 0;

    /* Check if io-audio manager is running or not */
    status = snd_cards();
    if (status == 0) {
        /* if no, return immediately */
        return 1;
    }

    return 1;   /* this audio target is available. */
}

AudioBootStrap QSAAUDIO_bootstrap = {
    DRIVER_NAME, "QNX QSA Audio", QSA_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
