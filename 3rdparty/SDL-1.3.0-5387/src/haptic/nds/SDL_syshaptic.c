/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 2008 Edgar Simo

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

#ifdef SDL_HAPTIC_NDS

#include "SDL_haptic.h"
#include "../SDL_syshaptic.h"
#include "SDL_joystick.h"
#include <nds/memory.h>
#include <nds/arm9/rumble.h>

#define MAX_HAPTICS  1
/* right now only the ezf3in1 (and maybe official rumble pak) are supported
   and there can only be one of those in at a time (in GBA slot.) */

static SDL_Haptic *nds_haptic = NULL;

struct haptic_hwdata
{
    enum
    { NONE, OFFICIAL, EZF3IN1 } type;
    int pos;
};


void
NDS_EZF_OpenNorWrite() 
{
    GBA_BUS[0x0FF0000] = 0xD200;
    GBA_BUS[0x0000000] = 0x1500;
    GBA_BUS[0x0010000] = 0xD200;
    GBA_BUS[0x0020000] = 0x1500;
    GBA_BUS[0x0E20000] = 0x1500;
    GBA_BUS[0x0FE0000] = 0x1500;
}

void
NDS_EZF_CloseNorWrite() 
{
    GBA_BUS[0x0FF0000] = 0xD200;
    GBA_BUS[0x0000000] = 0x1500;
    GBA_BUS[0x0010000] = 0xD200;
    GBA_BUS[0x0020000] = 0x1500;
    GBA_BUS[0x0E20000] = 0xD200;
    GBA_BUS[0x0FE0000] = 0x1500;
}

void
NDS_EZF_ChipReset()
{
    GBA_BUS[0x0000] = 0x00F0;
    GBA_BUS[0x1000] = 0x00F0;
} uint32 NDS_EZF_IsPresent() 
{
    vuint16 id1, id2;

    NDS_EZF_OpenNorWrite();

    GBA_BUS[0x0555] = 0x00AA;
    GBA_BUS[0x02AA] = 0x0055;
    GBA_BUS[0x0555] = 0x0090;
    GBA_BUS[0x1555] = 0x00AA;
    GBA_BUS[0x12AA] = 0x0055;
    GBA_BUS[0x1555] = 0x0090;
    id1 = GBA_BUS[0x0001];
    id2 = GBA_BUS[0x1001];
    if ((id1 != 0x227E) || (id2 != 0x227E)) {
        NDS_EZF_CloseNorWrite();
        return 0;
    }
    id1 = GBA_BUS[0x000E];
    id2 = GBA_BUS[0x100E];

    NDS_EZF_CloseNorWrite();
    if (id1 == 0x2218 && id2 == 0x2218) {
        return 1;
    }
    return 0;
}
void
NDS_EZF_SetShake(u8 pos) 
{
    u16 data = ((pos % 3) | 0x00F0);
    GBA_BUS[0x0FF0000] = 0xD200;
    GBA_BUS[0x0000000] = 0x1500;
    GBA_BUS[0x0010000] = 0xD200;
    GBA_BUS[0x0020000] = 0x1500;
    GBA_BUS[0x0F10000] = data;
    GBA_BUS[0x0FE0000] = 0x1500;

    GBA_BUS[0] = 0x0000;        /* write any value for vibration. */
    GBA_BUS[0] = 0x0002;
}

static int
SDL_SYS_LogicError(void)
{
    SDL_SetError("Logic error: No haptic devices available.");
    return 0;
}


int
SDL_SYS_HapticInit(void)
{
    int ret = 0;
    if (isRumbleInserted()) {
        /* official rumble pak is present. */
        ret = 1;
        printf("debug: haptic present: nintendo\n");
    } else if (NDS_EZF_IsPresent()) {
        /* ezflash 3-in-1 pak is present. */
        ret = 1;
        printf("debug: haptic present: ezf3in1\n");
        NDS_EZF_ChipReset();
    } else {
        printf("debug: no haptic found\n");
    }

    return ret;
}


const char *
SDL_SYS_HapticName(int index)
{
    if (nds_haptic) {
        switch (nds_haptic->hwdata->type) {
        case OFFICIAL:
            return "Nintendo DS Rumble Pak";
        case EZF3IN1:
            return "EZFlash 3-in-1 Rumble";
        default:
            return NULL;
        }
    }
    return NULL;
}


int
SDL_SYS_HapticOpen(SDL_Haptic * haptic)
{
    if (!haptic) {
        return -1;
    }

    haptic->hwdata = SDL_malloc(sizeof(struct haptic_hwdata));
    if (!haptic->hwdata) {
        SDL_OutOfMemory();
        return -1;
    }
    nds_haptic = haptic;

    haptic->supported = SDL_HAPTIC_CONSTANT;

    /* determine what is here, if anything */
    haptic->hwdata->type = NONE;
    if (isRumbleInserted()) {
        /* official rumble pak is present. */
        haptic->hwdata->type = OFFICIAL;
    } else if (NDS_EZF_IsPresent()) {
        /* ezflash 3-in-1 pak is present. */
        haptic->hwdata->type = EZF3IN1;
        NDS_EZF_ChipReset();
    } else {
        /* no haptic present */
        SDL_SYS_LogicError();
        return -1;
    }

    return 0;
}


int
SDL_SYS_HapticMouse(void)
{
    return -1;
}


int
SDL_SYS_JoystickIsHaptic(SDL_Joystick * joystick)
{
    return 0;
}


int
SDL_SYS_HapticOpenFromJoystick(SDL_Haptic * haptic, SDL_Joystick * joystick)
{
    /*SDL_SYS_LogicError(); */
    return -1;
}


int
SDL_SYS_JoystickSameHaptic(SDL_Haptic * haptic, SDL_Joystick * joystick)
{
    return 0;
}


void
SDL_SYS_HapticClose(SDL_Haptic * haptic)
{
    return;
}


void
SDL_SYS_HapticQuit(void)
{
    return;
}


int
SDL_SYS_HapticNewEffect(SDL_Haptic * haptic,
                        struct haptic_effect *effect, SDL_HapticEffect * base)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticUpdateEffect(SDL_Haptic * haptic,
                           struct haptic_effect *effect,
                           SDL_HapticEffect * data)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticRunEffect(SDL_Haptic * haptic, struct haptic_effect *effect,
                        Uint32 iterations)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticStopEffect(SDL_Haptic * haptic, struct haptic_effect *effect)
{
    SDL_SYS_LogicError();
    return -1;
}


void
SDL_SYS_HapticDestroyEffect(SDL_Haptic * haptic, struct haptic_effect *effect)
{
    SDL_SYS_LogicError();
    return;
}


int
SDL_SYS_HapticGetEffectStatus(SDL_Haptic * haptic,
                              struct haptic_effect *effect)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticSetGain(SDL_Haptic * haptic, int gain)
{
    SDL_SYS_LogicError();
    return -1;
}


int
SDL_SYS_HapticSetAutocenter(SDL_Haptic * haptic, int autocenter)
{
    SDL_SYS_LogicError();
    return -1;
}

int
SDL_SYS_HapticPause(SDL_Haptic * haptic)
{
    SDL_SYS_LogicError();
    return -1;
}

int
SDL_SYS_HapticUnpause(SDL_Haptic * haptic)
{
    SDL_SYS_LogicError();
    return -1;
}

int
SDL_SYS_HapticStopAll(SDL_Haptic * haptic)
{
    SDL_SYS_LogicError();
    return -1;
}



#endif /* SDL_HAPTIC_NDS */
/* vi: set ts=4 sw=4 expandtab: */
