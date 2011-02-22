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

#include "SDL_syshaptic.h"
#include "SDL_haptic_c.h"
#include "../joystick/SDL_joystick_c.h" /* For SDL_PrivateJoystickValid */


Uint8 SDL_numhaptics = 0;
SDL_Haptic **SDL_haptics = NULL;


/*
 * Initializes the Haptic devices.
 */
int
SDL_HapticInit(void)
{
    int arraylen;
    int status;

    SDL_numhaptics = 0;
    status = SDL_SYS_HapticInit();
    if (status >= 0) {
        arraylen = (status + 1) * sizeof(*SDL_haptics);
        SDL_haptics = (SDL_Haptic **) SDL_malloc(arraylen);
        if (SDL_haptics == NULL) {      /* Out of memory. */
            SDL_numhaptics = 0;
        } else {
            SDL_memset(SDL_haptics, 0, arraylen);
            SDL_numhaptics = status;
        }
        status = 0;
    }

    return status;
}


/*
 * Checks to see if the haptic device is valid
 */
static int
ValidHaptic(SDL_Haptic * haptic)
{
    int i;
    int valid;

    valid = 0;
    if (haptic != NULL) {
        for (i = 0; i < SDL_numhaptics; i++) {
            if (SDL_haptics[i] == haptic) {
                valid = 1;
                break;
            }
        }
    }

    /* Create the error here. */
    if (valid == 0) {
        SDL_SetError("Haptic: Invalid haptic device identifier");
    }

    return valid;
}


/*
 * Returns the number of available devices.
 */
int
SDL_NumHaptics(void)
{
    return SDL_numhaptics;
}


/*
 * Gets the name of a Haptic device by index.
 */
const char *
SDL_HapticName(int device_index)
{
    if ((device_index < 0) || (device_index >= SDL_numhaptics)) {
        SDL_SetError("Haptic: There are %d haptic devices available",
                     SDL_numhaptics);
        return NULL;
    }
    return SDL_SYS_HapticName(device_index);
}


/*
 * Opens a Haptic device.
 */
SDL_Haptic *
SDL_HapticOpen(int device_index)
{
    int i;
    SDL_Haptic *haptic;

    if ((device_index < 0) || (device_index >= SDL_numhaptics)) {
        SDL_SetError("Haptic: There are %d haptic devices available",
                     SDL_numhaptics);
        return NULL;
    }

    /* If the haptic is already open, return it */
    for (i = 0; SDL_haptics[i]; i++) {
        if (device_index == SDL_haptics[i]->index) {
            haptic = SDL_haptics[i];
            ++haptic->ref_count;
            return haptic;
        }
    }

    /* Create the haptic device */
    haptic = (SDL_Haptic *) SDL_malloc((sizeof *haptic));
    if (haptic == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    /* Initialize the haptic device */
    SDL_memset(haptic, 0, (sizeof *haptic));
    haptic->rumble_id = -1;
    haptic->index = device_index;
    if (SDL_SYS_HapticOpen(haptic) < 0) {
        SDL_free(haptic);
        return NULL;
    }

    /* Disable autocenter and set gain to max. */
    if (haptic->supported & SDL_HAPTIC_GAIN)
        SDL_HapticSetGain(haptic, 100);
    if (haptic->supported & SDL_HAPTIC_AUTOCENTER)
        SDL_HapticSetAutocenter(haptic, 0);

    /* Add haptic to list */
    ++haptic->ref_count;
    for (i = 0; SDL_haptics[i]; i++)
        /* Skip to next haptic */ ;
    SDL_haptics[i] = haptic;

    return haptic;
}


/*
 * Returns 1 if the device has been opened.
 */
int
SDL_HapticOpened(int device_index)
{
    int i, opened;

    opened = 0;
    for (i = 0; SDL_haptics[i]; i++) {
        if (SDL_haptics[i]->index == (Uint8) device_index) {
            opened = 1;
            break;
        }
    }
    return opened;
}


/*
 * Returns the index to a haptic device.
 */
int
SDL_HapticIndex(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    return haptic->index;
}


/*
 * Returns SDL_TRUE if mouse is haptic, SDL_FALSE if it isn't.
 */
int
SDL_MouseIsHaptic(void)
{
    if (SDL_SYS_HapticMouse() < 0)
        return SDL_FALSE;
    return SDL_TRUE;
}


/*
 * Returns the haptic device if mouse is haptic or NULL elsewise.
 */
SDL_Haptic *
SDL_HapticOpenFromMouse(void)
{
    int device_index;

    device_index = SDL_SYS_HapticMouse();

    if (device_index < 0) {
        SDL_SetError("Haptic: Mouse isn't a haptic device.");
        return NULL;
    }

    return SDL_HapticOpen(device_index);
}


/*
 * Returns SDL_TRUE if joystick has haptic features.
 */
int
SDL_JoystickIsHaptic(SDL_Joystick * joystick)
{
    int ret;

    /* Must be a valid joystick */
    if (!SDL_PrivateJoystickValid(&joystick)) {
        return -1;
    }

    ret = SDL_SYS_JoystickIsHaptic(joystick);

    if (ret > 0)
        return SDL_TRUE;
    else if (ret == 0)
        return SDL_FALSE;
    else
        return -1;
}


/*
 * Opens a haptic device from a joystick.
 */
SDL_Haptic *
SDL_HapticOpenFromJoystick(SDL_Joystick * joystick)
{
    int i;
    SDL_Haptic *haptic;

    /* Must be a valid joystick */
    if (!SDL_PrivateJoystickValid(&joystick)) {
        SDL_SetError("Haptic: Joystick isn't valid.");
        return NULL;
    }

    /* Joystick must be haptic */
    if (SDL_SYS_JoystickIsHaptic(joystick) <= 0) {
        SDL_SetError("Haptic: Joystick isn't a haptic device.");
        return NULL;
    }

    /* Check to see if joystick's haptic is already open */
    for (i = 0; SDL_haptics[i]; i++) {
        if (SDL_SYS_JoystickSameHaptic(SDL_haptics[i], joystick)) {
            haptic = SDL_haptics[i];
            ++haptic->ref_count;
            return haptic;
        }
    }

    /* Create the haptic device */
    haptic = (SDL_Haptic *) SDL_malloc((sizeof *haptic));
    if (haptic == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    /* Initialize the haptic device */
    SDL_memset(haptic, 0, sizeof(SDL_Haptic));
    haptic->rumble_id = -1;
    if (SDL_SYS_HapticOpenFromJoystick(haptic, joystick) < 0) {
        SDL_free(haptic);
        return NULL;
    }

    /* Add haptic to list */
    ++haptic->ref_count;
    for (i = 0; SDL_haptics[i]; i++)
        /* Skip to next haptic */ ;
    SDL_haptics[i] = haptic;

    return haptic;
}


/*
 * Closes a SDL_Haptic device.
 */
void
SDL_HapticClose(SDL_Haptic * haptic)
{
    int i;

    /* Must be valid */
    if (!ValidHaptic(haptic)) {
        return;
    }

    /* Check if it's still in use */
    if (--haptic->ref_count < 0) {
        return;
    }

    /* Close it, properly removing effects if needed */
    for (i = 0; i < haptic->neffects; i++) {
        if (haptic->effects[i].hweffect != NULL) {
            SDL_HapticDestroyEffect(haptic, i);
        }
    }
    SDL_SYS_HapticClose(haptic);

    /* Remove from the list */
    for (i = 0; SDL_haptics[i]; ++i) {
        if (haptic == SDL_haptics[i]) {
            SDL_haptics[i] = NULL;
            SDL_memcpy(&SDL_haptics[i], &SDL_haptics[i + 1],
                       (SDL_numhaptics - i) * sizeof(haptic));
            break;
        }
    }

    /* Free */
    SDL_free(haptic);
}

/*
 * Cleans up after the subsystem.
 */
void
SDL_HapticQuit(void)
{
    SDL_SYS_HapticQuit();
    if (SDL_haptics != NULL) {
        SDL_free(SDL_haptics);
        SDL_haptics = NULL;
    }
    SDL_numhaptics = 0;
}

/*
 * Returns the number of effects a haptic device has.
 */
int
SDL_HapticNumEffects(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    return haptic->neffects;
}


/*
 * Returns the number of effects a haptic device can play.
 */
int
SDL_HapticNumEffectsPlaying(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    return haptic->nplaying;
}


/*
 * Returns supported effects by the device.
 */
unsigned int
SDL_HapticQuery(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    return haptic->supported;
}


/*
 * Returns the number of axis on the device.
 */
int
SDL_HapticNumAxes(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    return haptic->naxes;
}

/*
 * Checks to see if the device can support the effect.
 */
int
SDL_HapticEffectSupported(SDL_Haptic * haptic, SDL_HapticEffect * effect)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    if ((haptic->supported & effect->type) != 0)
        return SDL_TRUE;
    return SDL_FALSE;
}

/*
 * Creates a new haptic effect.
 */
int
SDL_HapticNewEffect(SDL_Haptic * haptic, SDL_HapticEffect * effect)
{
    int i;

    /* Check for device validity. */
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    /* Check to see if effect is supported */
    if (SDL_HapticEffectSupported(haptic, effect) == SDL_FALSE) {
        SDL_SetError("Haptic: Effect not supported by haptic device.");
        return -1;
    }

    /* See if there's a free slot */
    for (i = 0; i < haptic->neffects; i++) {
        if (haptic->effects[i].hweffect == NULL) {

            /* Now let the backend create the real effect */
            if (SDL_SYS_HapticNewEffect(haptic, &haptic->effects[i], effect)
                != 0) {
                return -1;      /* Backend failed to create effect */
            }

            SDL_memcpy(&haptic->effects[i].effect, effect,
                       sizeof(SDL_HapticEffect));
            return i;
        }
    }

    SDL_SetError("Haptic: Device has no free space left.");
    return -1;
}

/*
 * Checks to see if an effect is valid.
 */
static int
ValidEffect(SDL_Haptic * haptic, int effect)
{
    if ((effect < 0) || (effect >= haptic->neffects)) {
        SDL_SetError("Haptic: Invalid effect identifier.");
        return 0;
    }
    return 1;
}

/*
 * Updates an effect.
 */
int
SDL_HapticUpdateEffect(SDL_Haptic * haptic, int effect,
                       SDL_HapticEffect * data)
{
    if (!ValidHaptic(haptic) || !ValidEffect(haptic, effect)) {
        return -1;
    }

    /* Can't change type dynamically. */
    if (data->type != haptic->effects[effect].effect.type) {
        SDL_SetError("Haptic: Updating effect type is illegal.");
        return -1;
    }

    /* Updates the effect */
    if (SDL_SYS_HapticUpdateEffect(haptic, &haptic->effects[effect], data) <
        0) {
        return -1;
    }

    SDL_memcpy(&haptic->effects[effect].effect, data,
               sizeof(SDL_HapticEffect));
    return 0;
}


/*
 * Runs the haptic effect on the device.
 */
int
SDL_HapticRunEffect(SDL_Haptic * haptic, int effect, Uint32 iterations)
{
    if (!ValidHaptic(haptic) || !ValidEffect(haptic, effect)) {
        return -1;
    }

    /* Run the effect */
    if (SDL_SYS_HapticRunEffect(haptic, &haptic->effects[effect], iterations)
        < 0) {
        return -1;
    }

    return 0;
}

/*
 * Stops the haptic effect on the device.
 */
int
SDL_HapticStopEffect(SDL_Haptic * haptic, int effect)
{
    if (!ValidHaptic(haptic) || !ValidEffect(haptic, effect)) {
        return -1;
    }

    /* Stop the effect */
    if (SDL_SYS_HapticStopEffect(haptic, &haptic->effects[effect]) < 0) {
        return -1;
    }

    return 0;
}

/*
 * Gets rid of a haptic effect.
 */
void
SDL_HapticDestroyEffect(SDL_Haptic * haptic, int effect)
{
    if (!ValidHaptic(haptic) || !ValidEffect(haptic, effect)) {
        return;
    }

    /* Not allocated */
    if (haptic->effects[effect].hweffect == NULL) {
        return;
    }

    SDL_SYS_HapticDestroyEffect(haptic, &haptic->effects[effect]);
}

/*
 * Gets the status of a haptic effect.
 */
int
SDL_HapticGetEffectStatus(SDL_Haptic * haptic, int effect)
{
    if (!ValidHaptic(haptic) || !ValidEffect(haptic, effect)) {
        return -1;
    }

    if ((haptic->supported & SDL_HAPTIC_STATUS) == 0) {
        SDL_SetError("Haptic: Device does not support status queries.");
        return -1;
    }

    return SDL_SYS_HapticGetEffectStatus(haptic, &haptic->effects[effect]);
}

/*
 * Sets the global gain of the device.
 */
int
SDL_HapticSetGain(SDL_Haptic * haptic, int gain)
{
    const char *env;
    int real_gain, max_gain;

    if (!ValidHaptic(haptic)) {
        return -1;
    }

    if ((haptic->supported & SDL_HAPTIC_GAIN) == 0) {
        SDL_SetError("Haptic: Device does not support setting gain.");
        return -1;
    }

    if ((gain < 0) || (gain > 100)) {
        SDL_SetError("Haptic: Gain must be between 0 and 100.");
        return -1;
    }

    /* We use the envvar to get the maximum gain. */
    env = SDL_getenv("SDL_HAPTIC_GAIN_MAX");
    if (env != NULL) {
        max_gain = SDL_atoi(env);

        /* Check for sanity. */
        if (max_gain < 0)
            max_gain = 0;
        else if (max_gain > 100)
            max_gain = 100;

        /* We'll scale it linearly with SDL_HAPTIC_GAIN_MAX */
        real_gain = (gain * max_gain) / 100;
    } else {
        real_gain = gain;
    }

    if (SDL_SYS_HapticSetGain(haptic, real_gain) < 0) {
        return -1;
    }

    return 0;
}

/*
 * Makes the device autocenter, 0 disables.
 */
int
SDL_HapticSetAutocenter(SDL_Haptic * haptic, int autocenter)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    if ((haptic->supported & SDL_HAPTIC_AUTOCENTER) == 0) {
        SDL_SetError("Haptic: Device does not support setting autocenter.");
        return -1;
    }

    if ((autocenter < 0) || (autocenter > 100)) {
        SDL_SetError("Haptic: Autocenter must be between 0 and 100.");
        return -1;
    }

    if (SDL_SYS_HapticSetAutocenter(haptic, autocenter) < 0) {
        return -1;
    }

    return 0;
}

/*
 * Pauses the haptic device.
 */
int
SDL_HapticPause(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    if ((haptic->supported & SDL_HAPTIC_PAUSE) == 0) {
        SDL_SetError("Haptic: Device does not support setting pausing.");
        return -1;
    }

    return SDL_SYS_HapticPause(haptic);
}

/*
 * Unpauses the haptic device.
 */
int
SDL_HapticUnpause(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    if ((haptic->supported & SDL_HAPTIC_PAUSE) == 0) {
        return 0;               /* Not going to be paused, so we pretend it's unpaused. */
    }

    return SDL_SYS_HapticUnpause(haptic);
}

/*
 * Stops all the currently playing effects.
 */
int
SDL_HapticStopAll(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    return SDL_SYS_HapticStopAll(haptic);
}

static void
SDL_HapticRumbleCreate(SDL_HapticEffect * efx)
{
   SDL_memset(efx, 0, sizeof(SDL_HapticEffect));
   efx->type = SDL_HAPTIC_SINE;
   efx->periodic.period = 1000;
   efx->periodic.magnitude = 0x4000;
   efx->periodic.length = 5000;
   efx->periodic.attack_length = 0;
   efx->periodic.fade_length = 0;
}

/*
 * Checks to see if rumble is supported.
 */
int
SDL_HapticRumbleSupported(SDL_Haptic * haptic)
{
    SDL_HapticEffect efx;

    if (!ValidHaptic(haptic)) {
        return -1;
    }

    SDL_HapticRumbleCreate(&efx);
    return SDL_HapticEffectSupported(haptic, &efx);
}

/*
 * Initializes the haptic device for simple rumble playback.
 */
int
SDL_HapticRumbleInit(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    /* Already allocated. */
    if (haptic->rumble_id >= 0) {
        return 0;
    }

    /* Copy over. */
    SDL_HapticRumbleCreate(&haptic->rumble_effect);
    haptic->rumble_id = SDL_HapticNewEffect(haptic, &haptic->rumble_effect);
    if (haptic->rumble_id >= 0) {
        return 0;
    }
    return -1;
}

/*
 * Runs simple rumble on a haptic device
 */
int
SDL_HapticRumblePlay(SDL_Haptic * haptic, float strength, Uint32 length)
{
    int ret;
    SDL_HapticPeriodic *efx;

    if (!ValidHaptic(haptic)) {
        return -1;
    }

    if (haptic->rumble_id < 0) {
        SDL_SetError("Haptic: Rumble effect not initialized on haptic device");
        return -1;
    }

    /* Clamp strength. */
    if (strength > 1.0f) {
        strength = 1.0f;
    }
    else if (strength < 0.0f) {
        strength = 0.0f;
    }

    /* New effect. */
    efx = &haptic->rumble_effect.periodic;
    efx->magnitude = (Sint16)(32767.0f*strength);
    efx->length = length;
    ret = SDL_HapticUpdateEffect(haptic, haptic->rumble_id, &haptic->rumble_effect);

    return SDL_HapticRunEffect(haptic, haptic->rumble_id, 1);
}

/*
 * Stops the simple rumble on a haptic device.
 */
int
SDL_HapticRumbleStop(SDL_Haptic * haptic)
{
    if (!ValidHaptic(haptic)) {
        return -1;
    }

    if (haptic->rumble_id < 0) {
        SDL_SetError("Haptic: Rumble effect not initialized on haptic device");
        return -1;
    }

    return SDL_HapticStopEffect(haptic, haptic->rumble_id);
}


