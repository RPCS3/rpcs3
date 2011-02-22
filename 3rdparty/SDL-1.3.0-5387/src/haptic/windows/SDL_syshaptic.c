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

#ifdef SDL_HAPTIC_DINPUT

#include "SDL_haptic.h"
#include "../SDL_syshaptic.h"
#include "SDL_joystick.h"
#include "../../joystick/SDL_sysjoystick.h"     /* For the real SDL_Joystick */
#include "../../joystick/windows/SDL_dxjoystick_c.h"      /* For joystick hwdata */


#define MAX_HAPTICS  32


/*
 * List of available haptic devices.
 */
static struct
{
    DIDEVICEINSTANCE instance;
    char *name;
    SDL_Haptic *haptic;
    DIDEVCAPS capabilities;
} SDL_hapticlist[MAX_HAPTICS];


/*
 * Haptic system hardware data.
 */
struct haptic_hwdata
{
    LPDIRECTINPUTDEVICE2 device;
    DWORD axes[3];              /* Axes to use. */
    int is_joystick;            /* Device is loaded as joystick. */
};


/*
 * Haptic system effect data.
 */
struct haptic_hweffect
{
    DIEFFECT effect;
    LPDIRECTINPUTEFFECT ref;
};


/*
 * Internal stuff.
 */
static LPDIRECTINPUT dinput = NULL;


/*
 * External stuff.
 */
extern HWND SDL_HelperWindow;


/*
 * Prototypes.
 */
static void DI_SetError(const char *str, HRESULT err);
static int DI_GUIDIsSame(const GUID * a, const GUID * b);
static int SDL_SYS_HapticOpenFromInstance(SDL_Haptic * haptic,
                                          DIDEVICEINSTANCE instance);
static int SDL_SYS_HapticOpenFromDevice2(SDL_Haptic * haptic,
                                         LPDIRECTINPUTDEVICE2 device2);
static DWORD DIGetTriggerButton(Uint16 button);
static int SDL_SYS_SetDirection(DIEFFECT * effect, SDL_HapticDirection * dir,
                                int naxes);
static int SDL_SYS_ToDIEFFECT(SDL_Haptic * haptic, DIEFFECT * dest,
                              SDL_HapticEffect * src);
static void SDL_SYS_HapticFreeDIEFFECT(DIEFFECT * effect, int type);
static REFGUID SDL_SYS_HapticEffectType(SDL_HapticEffect * effect);
/* Callbacks. */
static BOOL CALLBACK EnumHapticsCallback(const DIDEVICEINSTANCE *
                                         pdidInstance, VOID * pContext);
static BOOL CALLBACK DI_EffectCallback(LPCDIEFFECTINFO pei, LPVOID pv);


/* 
 * Like SDL_SetError but for DX error codes.
 */
static void
DI_SetError(const char *str, HRESULT err)
{
    /*
       SDL_SetError("Haptic: %s - %s: %s", str,
       DXGetErrorString8A(err), DXGetErrorDescription8A(err));
     */
    SDL_SetError("Haptic error %s", str);
}


/*
 * Checks to see if two GUID are the same.
 */
static int
DI_GUIDIsSame(const GUID * a, const GUID * b)
{
    if (((a)->Data1 == (b)->Data1) &&
        ((a)->Data2 == (b)->Data2) &&
        ((a)->Data3 == (b)->Data3) &&
        (SDL_strcmp((a)->Data4, (b)->Data4) == 0))
        return 1;
    return 0;
}


/*
 * Initializes the haptic subsystem.
 */
int
SDL_SYS_HapticInit(void)
{
    HRESULT ret;
    HINSTANCE instance;

    if (dinput != NULL) {       /* Already open. */
        SDL_SetError("Haptic: SubSystem already open.");
        return -1;
    }

    /* Clear all the memory. */
    SDL_memset(SDL_hapticlist, 0, sizeof(SDL_hapticlist));

    SDL_numhaptics = 0;

    ret = CoInitialize(NULL);
    if (FAILED(ret)) {
        DI_SetError("Coinitialize", ret);
        return -1;
    }

    ret = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER,
                           &IID_IDirectInput, (LPVOID) & dinput);
    if (FAILED(ret)) {
        DI_SetError("CoCreateInstance", ret);
        return -1;
    }

    /* Because we used CoCreateInstance, we need to Initialize it, first. */
    instance = GetModuleHandle(NULL);
    if (instance == NULL) {
        SDL_SetError("GetModuleHandle() failed with error code %d.",
                     GetLastError());
        return -1;
    }
    ret = IDirectInput_Initialize(dinput, instance, DIRECTINPUT_VERSION);
    if (FAILED(ret)) {
        DI_SetError("Initializing DirectInput device", ret);
        return -1;
    }

    /* Look for haptic devices. */
    ret = IDirectInput_EnumDevices(dinput,
                                   0,
                                   EnumHapticsCallback,
                                   NULL,
                                   DIEDFL_FORCEFEEDBACK |
                                   DIEDFL_ATTACHEDONLY);
    if (FAILED(ret)) {
        DI_SetError("Enumerating DirectInput devices", ret);
        return -1;
    }

    return SDL_numhaptics;
}

/*
 * Callback to find the haptic devices.
 */
static BOOL CALLBACK
EnumHapticsCallback(const DIDEVICEINSTANCE * pdidInstance, VOID * pContext)
{
    HRESULT ret;
    LPDIRECTINPUTDEVICE device;

    /* Copy the instance over, useful for creating devices. */
    SDL_memcpy(&SDL_hapticlist[SDL_numhaptics].instance, pdidInstance,
               sizeof(DIDEVICEINSTANCE));

    /* Open the device */
    ret = IDirectInput_CreateDevice(dinput, &pdidInstance->guidInstance,
                                    &device, NULL);
    if (FAILED(ret)) {
        /* DI_SetError("Creating DirectInput device",ret); */
        return DIENUM_CONTINUE;
    }

    /* Get capabilities. */
    SDL_hapticlist[SDL_numhaptics].capabilities.dwSize = sizeof(DIDEVCAPS);
    ret = IDirectInputDevice_GetCapabilities(device,
                                             &SDL_hapticlist[SDL_numhaptics].
                                             capabilities);
    if (FAILED(ret)) {
        /* DI_SetError("Getting device capabilities",ret); */
        IDirectInputDevice_Release(device);
        return DIENUM_CONTINUE;
    }

    /* Copy the name */
    SDL_hapticlist[SDL_numhaptics].name = WIN_StringToUTF8(SDL_hapticlist[SDL_numhaptics].instance.tszProductName);

    /* Close up device and count it. */
    IDirectInputDevice_Release(device);
    SDL_numhaptics++;

    /* Watch out for hard limit. */
    if (SDL_numhaptics >= MAX_HAPTICS)
        return DIENUM_STOP;

    return DIENUM_CONTINUE;
}


/*
 * Return the name of a haptic device, does not need to be opened.
 */
const char *
SDL_SYS_HapticName(int index)
{
    return SDL_hapticlist[index].name;
}


/*
 * Callback to get all supported effects.
 */
#define EFFECT_TEST(e,s)               \
if (DI_GUIDIsSame(&pei->guid, &(e)))   \
   haptic->supported |= (s)
static BOOL CALLBACK
DI_EffectCallback(LPCDIEFFECTINFO pei, LPVOID pv)
{
    /* Prepare the haptic device. */
    SDL_Haptic *haptic = (SDL_Haptic *) pv;

    /* Get supported. */
    EFFECT_TEST(GUID_Spring, SDL_HAPTIC_SPRING);
    EFFECT_TEST(GUID_Damper, SDL_HAPTIC_DAMPER);
    EFFECT_TEST(GUID_Inertia, SDL_HAPTIC_INERTIA);
    EFFECT_TEST(GUID_Friction, SDL_HAPTIC_FRICTION);
    EFFECT_TEST(GUID_ConstantForce, SDL_HAPTIC_CONSTANT);
    EFFECT_TEST(GUID_CustomForce, SDL_HAPTIC_CUSTOM);
    EFFECT_TEST(GUID_Sine, SDL_HAPTIC_SINE);
    EFFECT_TEST(GUID_Square, SDL_HAPTIC_SQUARE);
    EFFECT_TEST(GUID_Triangle, SDL_HAPTIC_TRIANGLE);
    EFFECT_TEST(GUID_SawtoothUp, SDL_HAPTIC_SAWTOOTHUP);
    EFFECT_TEST(GUID_SawtoothDown, SDL_HAPTIC_SAWTOOTHDOWN);
    EFFECT_TEST(GUID_RampForce, SDL_HAPTIC_RAMP);

    /* Check for more. */
    return DIENUM_CONTINUE;
}


/*
 * Callback to get supported axes.
 */
static BOOL CALLBACK
DI_DeviceObjectCallback(LPCDIDEVICEOBJECTINSTANCE dev, LPVOID pvRef)
{
    SDL_Haptic *haptic = (SDL_Haptic *) pvRef;

    if ((dev->dwType & DIDFT_AXIS) && (dev->dwFlags & DIDOI_FFACTUATOR)) {

        haptic->hwdata->axes[haptic->naxes] = dev->dwOfs;
        haptic->naxes++;

        /* Currently using the artificial limit of 3 axes. */
        if (haptic->naxes >= 3) {
            return DIENUM_STOP;
        }
    }

    return DIENUM_CONTINUE;
}


/*
 * Opens the haptic device from the file descriptor.
 *
 *    Steps:
 *       - Open temporary DirectInputDevice interface.
 *       - Create DirectInputDevice2 interface.
 *       - Release DirectInputDevice interface.
 *       - Call SDL_SYS_HapticOpenFromDevice2
 */
static int
SDL_SYS_HapticOpenFromInstance(SDL_Haptic * haptic, DIDEVICEINSTANCE instance)
{
    HRESULT ret;
    int ret2;
    LPDIRECTINPUTDEVICE device;

    /* Allocate the hwdata */
    haptic->hwdata = (struct haptic_hwdata *)
        SDL_malloc(sizeof(*haptic->hwdata));
    if (haptic->hwdata == NULL) {
        SDL_OutOfMemory();
        goto creat_err;
    }
    SDL_memset(haptic->hwdata, 0, sizeof(*haptic->hwdata));

    /* Open the device */
    ret = IDirectInput_CreateDevice(dinput, &instance.guidInstance,
                                    &device, NULL);
    if (FAILED(ret)) {
        DI_SetError("Creating DirectInput device", ret);
        goto creat_err;
    }

    /* Now get the IDirectInputDevice2 interface, instead. */
    ret = IDirectInputDevice_QueryInterface(device,
                                            &IID_IDirectInputDevice2,
                                            (LPVOID *) & haptic->hwdata->
                                            device);
    /* Done with the temporary one now. */
    IDirectInputDevice_Release(device);
    if (FAILED(ret)) {
        DI_SetError("Querying DirectInput interface", ret);
        goto creat_err;
    }

    ret2 = SDL_SYS_HapticOpenFromDevice2(haptic, haptic->hwdata->device);
    if (ret2 < 0) {
        goto query_err;
    }

    return 0;

  query_err:
    IDirectInputDevice2_Release(haptic->hwdata->device);
  creat_err:
    if (haptic->hwdata != NULL) {
        SDL_free(haptic->hwdata);
        haptic->hwdata = NULL;
    }
    return -1;
}


/*
 * Opens the haptic device from the file descriptor.
 *
 *    Steps:
 *       - Set cooperative level.
 *       - Set data format.
 *       - Acquire exclusiveness.
 *       - Reset actuators.
 *       - Get supported featuers.
 */
static int
SDL_SYS_HapticOpenFromDevice2(SDL_Haptic * haptic,
                              LPDIRECTINPUTDEVICE2 device2)
{
    HRESULT ret;
    DIPROPDWORD dipdw;

    /* We'll use the device2 from now on. */
    haptic->hwdata->device = device2;

    /* Grab it exclusively to use force feedback stuff. */
    ret = IDirectInputDevice2_SetCooperativeLevel(haptic->hwdata->device,
                                                  SDL_HelperWindow,
                                                  DISCL_EXCLUSIVE |
                                                  DISCL_BACKGROUND);
    if (FAILED(ret)) {
        DI_SetError("Setting cooperative level to exclusive", ret);
        goto acquire_err;
    }

    /* Set data format. */
    ret = IDirectInputDevice2_SetDataFormat(haptic->hwdata->device,
                                            &c_dfDIJoystick2);
    if (FAILED(ret)) {
        DI_SetError("Setting data format", ret);
        goto acquire_err;
    }

    /* Get number of axes. */
    ret = IDirectInputDevice2_EnumObjects(haptic->hwdata->device,
                                          DI_DeviceObjectCallback,
                                          haptic, DIDFT_AXIS);
    if (FAILED(ret)) {
        DI_SetError("Getting device axes", ret);
        goto acquire_err;
    }

    /* Acquire the device. */
    ret = IDirectInputDevice2_Acquire(haptic->hwdata->device);
    if (FAILED(ret)) {
        DI_SetError("Acquiring DirectInput device", ret);
        goto acquire_err;
    }

    /* Reset all actuators - just in case. */
    ret = IDirectInputDevice2_SendForceFeedbackCommand(haptic->hwdata->device,
                                                       DISFFC_RESET);
    if (FAILED(ret)) {
        DI_SetError("Resetting device", ret);
        goto acquire_err;
    }

    /* Enabling actuators. */
    ret = IDirectInputDevice2_SendForceFeedbackCommand(haptic->hwdata->device,
                                                       DISFFC_SETACTUATORSON);
    if (FAILED(ret)) {
        DI_SetError("Enabling actuators", ret);
        goto acquire_err;
    }

    /* Get supported effects. */
    ret = IDirectInputDevice2_EnumEffects(haptic->hwdata->device,
                                          DI_EffectCallback, haptic,
                                          DIEFT_ALL);
    if (FAILED(ret)) {
        DI_SetError("Enumerating supported effects", ret);
        goto acquire_err;
    }
    if (haptic->supported == 0) {       /* Error since device supports nothing. */
        SDL_SetError("Haptic: Internal error on finding supported effects.");
        goto acquire_err;
    }

    /* Check autogain and autocenter. */
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = 10000;
    ret = IDirectInputDevice2_SetProperty(haptic->hwdata->device,
                                          DIPROP_FFGAIN, &dipdw.diph);
    if (!FAILED(ret)) {         /* Gain is supported. */
        haptic->supported |= SDL_HAPTIC_GAIN;
    }
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = DIPROPAUTOCENTER_OFF;
    ret = IDirectInputDevice2_SetProperty(haptic->hwdata->device,
                                          DIPROP_AUTOCENTER, &dipdw.diph);
    if (!FAILED(ret)) {         /* Autocenter is supported. */
        haptic->supported |= SDL_HAPTIC_AUTOCENTER;
    }

    /* Status is always supported. */
    haptic->supported |= SDL_HAPTIC_STATUS | SDL_HAPTIC_PAUSE;

    /* Check maximum effects. */
    haptic->neffects = 128;     /* This is not actually supported as thus under windows,
                                   there is no way to tell the number of EFFECTS that a
                                   device can hold, so we'll just use a "random" number
                                   instead and put warnings in SDL_haptic.h */
    haptic->nplaying = 128;     /* Even more impossible to get this then neffects. */

    /* Prepare effects memory. */
    haptic->effects = (struct haptic_effect *)
        SDL_malloc(sizeof(struct haptic_effect) * haptic->neffects);
    if (haptic->effects == NULL) {
        SDL_OutOfMemory();
        goto acquire_err;
    }
    /* Clear the memory */
    SDL_memset(haptic->effects, 0,
               sizeof(struct haptic_effect) * haptic->neffects);

    return 0;

    /* Error handling */
  acquire_err:
    IDirectInputDevice2_Unacquire(haptic->hwdata->device);
    return -1;

}


/*
 * Opens a haptic device for usage.
 */
int
SDL_SYS_HapticOpen(SDL_Haptic * haptic)
{
    return SDL_SYS_HapticOpenFromInstance(haptic,
                                          SDL_hapticlist[haptic->index].
                                          instance);
}


/*
 * Opens a haptic device from first mouse it finds for usage.
 */
int
SDL_SYS_HapticMouse(void)
{
    int i;

    /* Grab the first mouse haptic device we find. */
    for (i = 0; i < SDL_numhaptics; i++) {
        if (SDL_hapticlist[i].capabilities.dwDevType == DIDEVTYPE_MOUSE) {
            return i;
        }
    }

    return -1;
}


/*
 * Checks to see if a joystick has haptic features.
 */
int
SDL_SYS_JoystickIsHaptic(SDL_Joystick * joystick)
{
    if (joystick->hwdata->Capabilities.dwFlags & DIDC_FORCEFEEDBACK) {
        return SDL_TRUE;
    }

    return SDL_FALSE;
}


/*
 * Checks to see if the haptic device and joystick and in reality the same.
 */
int
SDL_SYS_JoystickSameHaptic(SDL_Haptic * haptic, SDL_Joystick * joystick)
{
    HRESULT ret;
    DIDEVICEINSTANCE hap_instance, joy_instance;

    /* Get the device instances. */
    ret = IDirectInputDevice2_GetDeviceInfo(haptic->hwdata->device,
                                            &hap_instance);
    if (FAILED(ret)) {
        return 0;
    }
    ret = IDirectInputDevice2_GetDeviceInfo(joystick->hwdata->InputDevice,
                                            &joy_instance);
    if (FAILED(ret)) {
        return 0;
    }

    if (DI_GUIDIsSame(&hap_instance.guidInstance, &joy_instance.guidInstance))
        return 1;

    return 0;
}


/*
 * Opens a SDL_Haptic from a SDL_Joystick.
 */
int
SDL_SYS_HapticOpenFromJoystick(SDL_Haptic * haptic, SDL_Joystick * joystick)
{
    int i, ret;
    HRESULT idret;
    DIDEVICEINSTANCE joy_instance;

    /* Since it comes from a joystick we have to try to match it with a haptic device on our haptic list. */
    for (i=0; i<SDL_numhaptics; i++) {
        idret = IDirectInputDevice2_GetDeviceInfo(joystick->hwdata->InputDevice,
              &joy_instance);
        if (FAILED(idret)) {
            return -1;
        }
        if (DI_GUIDIsSame(&SDL_hapticlist[i].instance.guidInstance,
                          &joy_instance.guidInstance)) {
            haptic->index = i;
            break;
        }
    }
    if (i >= SDL_numhaptics) {
        return -1;
    }

    /* Allocate the hwdata */
    haptic->hwdata = (struct haptic_hwdata *)
        SDL_malloc(sizeof(*haptic->hwdata));
    if (haptic->hwdata == NULL) {
        SDL_OutOfMemory();
        return -1;
    }
    SDL_memset(haptic->hwdata, 0, sizeof(*haptic->hwdata));

    /* Now open the device. */
    ret =
        SDL_SYS_HapticOpenFromDevice2(haptic, joystick->hwdata->InputDevice);
    if (ret < 0) {
        return -1;
    }

    /* It's using the joystick device. */
    haptic->hwdata->is_joystick = 1;

    return 0;
}


/*
 * Closes the haptic device.
 */
void
SDL_SYS_HapticClose(SDL_Haptic * haptic)
{
    if (haptic->hwdata) {

        /* Free effects. */
        SDL_free(haptic->effects);
        haptic->effects = NULL;
        haptic->neffects = 0;

        /* Clean up */
        IDirectInputDevice2_Unacquire(haptic->hwdata->device);
        /* Only release if isn't grabbed by a joystick. */
        if (haptic->hwdata->is_joystick == 0) {
            IDirectInputDevice2_Release(haptic->hwdata->device);
        }

        /* Free */
        SDL_free(haptic->hwdata);
        haptic->hwdata = NULL;
    }
}


/* 
 * Clean up after system specific haptic stuff
 */
void
SDL_SYS_HapticQuit(void)
{
    int i;

    for (i = 0; i < SDL_arraysize(SDL_hapticlist); ++i) {
        if (SDL_hapticlist[i].name) {
            SDL_free(SDL_hapticlist[i].name);
            SDL_hapticlist[i].name = NULL;
        }
    }

    IDirectInput_Release(dinput);
    dinput = NULL;
}


/*
 * Converts an SDL trigger button to an DIEFFECT trigger button.
 */
static DWORD
DIGetTriggerButton(Uint16 button)
{
    DWORD dwTriggerButton;

    dwTriggerButton = DIEB_NOTRIGGER;

    if (button != 0) {
        dwTriggerButton = DIJOFS_BUTTON(button - 1);
    }

    return dwTriggerButton;
}


/*
 * Sets the direction.
 */
static int
SDL_SYS_SetDirection(DIEFFECT * effect, SDL_HapticDirection * dir, int naxes)
{
    LONG *rglDir;

    /* Handle no axes a part. */
    if (naxes == 0) {
        effect->dwFlags |= DIEFF_SPHERICAL;     /* Set as default. */
        effect->rglDirection = NULL;
        return 0;
    }

    /* Has axes. */
    rglDir = SDL_malloc(sizeof(LONG) * naxes);
    if (rglDir == NULL) {
        SDL_OutOfMemory();
        return -1;
    }
    SDL_memset(rglDir, 0, sizeof(LONG) * naxes);
    effect->rglDirection = rglDir;

    switch (dir->type) {
    case SDL_HAPTIC_POLAR:
        effect->dwFlags |= DIEFF_POLAR;
        rglDir[0] = dir->dir[0];
        return 0;
    case SDL_HAPTIC_CARTESIAN:
        effect->dwFlags |= DIEFF_CARTESIAN;
        rglDir[0] = dir->dir[0];
        if (naxes > 1)
            rglDir[1] = dir->dir[1];
        if (naxes > 2)
            rglDir[2] = dir->dir[2];
        return 0;
    case SDL_HAPTIC_SPHERICAL:
        effect->dwFlags |= DIEFF_SPHERICAL;
        rglDir[0] = dir->dir[0];
        if (naxes > 1)
            rglDir[1] = dir->dir[1];
        if (naxes > 2)
            rglDir[2] = dir->dir[2];
        return 0;

    default:
        SDL_SetError("Haptic: Unknown direction type.");
        return -1;
    }
}

#define CONVERT(x)   (((x) > 0x7FFF) ? 10000 : ((x)*10000) / 0x7FFF)
/*
 * Creates the DIEFFECT from a SDL_HapticEffect.
 */
static int
SDL_SYS_ToDIEFFECT(SDL_Haptic * haptic, DIEFFECT * dest,
                   SDL_HapticEffect * src)
{
    int i;
    DICONSTANTFORCE *constant;
    DIPERIODIC *periodic;
    DICONDITION *condition;     /* Actually an array of conditions - one per axis. */
    DIRAMPFORCE *ramp;
    DICUSTOMFORCE *custom;
    DIENVELOPE *envelope;
    SDL_HapticConstant *hap_constant;
    SDL_HapticPeriodic *hap_periodic;
    SDL_HapticCondition *hap_condition;
    SDL_HapticRamp *hap_ramp;
    SDL_HapticCustom *hap_custom;
    DWORD *axes;

    /* Set global stuff. */
    SDL_memset(dest, 0, sizeof(DIEFFECT));
    dest->dwSize = sizeof(DIEFFECT);    /* Set the structure size. */
    dest->dwSamplePeriod = 0;   /* Not used by us. */
    dest->dwGain = 10000;       /* Gain is set globally, not locally. */
    dest->dwFlags = DIEFF_OBJECTOFFSETS;        /* Seems obligatory. */

    /* Envelope. */
    envelope = SDL_malloc(sizeof(DIENVELOPE));
    if (envelope == NULL) {
        SDL_OutOfMemory();
        return -1;
    }
    SDL_memset(envelope, 0, sizeof(DIENVELOPE));
    dest->lpEnvelope = envelope;
    envelope->dwSize = sizeof(DIENVELOPE);      /* Always should be this. */

    /* Axes. */
    dest->cAxes = haptic->naxes;
    if (dest->cAxes > 0) {
        axes = SDL_malloc(sizeof(DWORD) * dest->cAxes);
        if (axes == NULL) {
            SDL_OutOfMemory();
            return -1;
        }
        axes[0] = haptic->hwdata->axes[0];      /* Always at least one axis. */
        if (dest->cAxes > 1) {
            axes[1] = haptic->hwdata->axes[1];
        }
        if (dest->cAxes > 2) {
            axes[2] = haptic->hwdata->axes[2];
        }
        dest->rgdwAxes = axes;
    }


    /* The big type handling switch, even bigger then linux's version. */
    switch (src->type) {
    case SDL_HAPTIC_CONSTANT:
        hap_constant = &src->constant;
        constant = SDL_malloc(sizeof(DICONSTANTFORCE));
        if (constant == NULL) {
            SDL_OutOfMemory();
            return -1;
        }
        SDL_memset(constant, 0, sizeof(DICONSTANTFORCE));

        /* Specifics */
        constant->lMagnitude = CONVERT(hap_constant->level);
        dest->cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        dest->lpvTypeSpecificParams = constant;

        /* Generics */
        dest->dwDuration = hap_constant->length * 1000; /* In microseconds. */
        dest->dwTriggerButton = DIGetTriggerButton(hap_constant->button);
        dest->dwTriggerRepeatInterval = hap_constant->interval;
        dest->dwStartDelay = hap_constant->delay * 1000;        /* In microseconds. */

        /* Direction. */
        if (SDL_SYS_SetDirection(dest, &hap_constant->direction, dest->cAxes)
            < 0) {
            return -1;
        }

        /* Envelope */
        if ((hap_constant->attack_length == 0)
            && (hap_constant->fade_length == 0)) {
            SDL_free(dest->lpEnvelope);
            dest->lpEnvelope = NULL;
        } else {
            envelope->dwAttackLevel = CONVERT(hap_constant->attack_level);
            envelope->dwAttackTime = hap_constant->attack_length * 1000;
            envelope->dwFadeLevel = CONVERT(hap_constant->fade_level);
            envelope->dwFadeTime = hap_constant->fade_length * 1000;
        }

        break;

    case SDL_HAPTIC_SINE:
    case SDL_HAPTIC_SQUARE:
    case SDL_HAPTIC_TRIANGLE:
    case SDL_HAPTIC_SAWTOOTHUP:
    case SDL_HAPTIC_SAWTOOTHDOWN:
        hap_periodic = &src->periodic;
        periodic = SDL_malloc(sizeof(DIPERIODIC));
        if (periodic == NULL) {
            SDL_OutOfMemory();
            return -1;
        }
        SDL_memset(periodic, 0, sizeof(DIPERIODIC));

        /* Specifics */
        periodic->dwMagnitude = CONVERT(hap_periodic->magnitude);
        periodic->lOffset = CONVERT(hap_periodic->offset);
        periodic->dwPhase = hap_periodic->phase;
        periodic->dwPeriod = hap_periodic->period * 1000;
        dest->cbTypeSpecificParams = sizeof(DIPERIODIC);
        dest->lpvTypeSpecificParams = periodic;

        /* Generics */
        dest->dwDuration = hap_periodic->length * 1000; /* In microseconds. */
        dest->dwTriggerButton = DIGetTriggerButton(hap_periodic->button);
        dest->dwTriggerRepeatInterval = hap_periodic->interval;
        dest->dwStartDelay = hap_periodic->delay * 1000;        /* In microseconds. */

        /* Direction. */
        if (SDL_SYS_SetDirection(dest, &hap_periodic->direction, dest->cAxes)
            < 0) {
            return -1;
        }

        /* Envelope */
        if ((hap_periodic->attack_length == 0)
            && (hap_periodic->fade_length == 0)) {
            SDL_free(dest->lpEnvelope);
            dest->lpEnvelope = NULL;
        } else {
            envelope->dwAttackLevel = CONVERT(hap_periodic->attack_level);
            envelope->dwAttackTime = hap_periodic->attack_length * 1000;
            envelope->dwFadeLevel = CONVERT(hap_periodic->fade_level);
            envelope->dwFadeTime = hap_periodic->fade_length * 1000;
        }

        break;

    case SDL_HAPTIC_SPRING:
    case SDL_HAPTIC_DAMPER:
    case SDL_HAPTIC_INERTIA:
    case SDL_HAPTIC_FRICTION:
        hap_condition = &src->condition;
        condition = SDL_malloc(sizeof(DICONDITION) * dest->cAxes);
        if (condition == NULL) {
            SDL_OutOfMemory();
            return -1;
        }
        SDL_memset(condition, 0, sizeof(DICONDITION));

        /* Specifics */
        for (i = 0; i < (int) dest->cAxes; i++) {
            condition[i].lOffset = CONVERT(hap_condition->center[i]);
            condition[i].lPositiveCoefficient =
                CONVERT(hap_condition->right_coeff[i]);
            condition[i].lNegativeCoefficient =
                CONVERT(hap_condition->left_coeff[i]);
            condition[i].dwPositiveSaturation =
                CONVERT(hap_condition->right_sat[i]);
            condition[i].dwNegativeSaturation =
                CONVERT(hap_condition->left_sat[i]);
            condition[i].lDeadBand = CONVERT(hap_condition->deadband[i]);
        }
        dest->cbTypeSpecificParams = sizeof(DICONDITION) * dest->cAxes;
        dest->lpvTypeSpecificParams = condition;

        /* Generics */
        dest->dwDuration = hap_condition->length * 1000;        /* In microseconds. */
        dest->dwTriggerButton = DIGetTriggerButton(hap_condition->button);
        dest->dwTriggerRepeatInterval = hap_condition->interval;
        dest->dwStartDelay = hap_condition->delay * 1000;       /* In microseconds. */

        /* Direction. */
        if (SDL_SYS_SetDirection(dest, &hap_condition->direction, dest->cAxes)
            < 0) {
            return -1;
        }

        /* Envelope - Not actually supported by most CONDITION implementations. */
        SDL_free(dest->lpEnvelope);
        dest->lpEnvelope = NULL;

        break;

    case SDL_HAPTIC_RAMP:
        hap_ramp = &src->ramp;
        ramp = SDL_malloc(sizeof(DIRAMPFORCE));
        if (ramp == NULL) {
            SDL_OutOfMemory();
            return -1;
        }
        SDL_memset(ramp, 0, sizeof(DIRAMPFORCE));

        /* Specifics */
        ramp->lStart = CONVERT(hap_ramp->start);
        ramp->lEnd = CONVERT(hap_ramp->end);
        dest->cbTypeSpecificParams = sizeof(DIRAMPFORCE);
        dest->lpvTypeSpecificParams = ramp;

        /* Generics */
        dest->dwDuration = hap_ramp->length * 1000;     /* In microseconds. */
        dest->dwTriggerButton = DIGetTriggerButton(hap_ramp->button);
        dest->dwTriggerRepeatInterval = hap_ramp->interval;
        dest->dwStartDelay = hap_ramp->delay * 1000;    /* In microseconds. */

        /* Direction. */
        if (SDL_SYS_SetDirection(dest, &hap_ramp->direction, dest->cAxes) < 0) {
            return -1;
        }

        /* Envelope */
        if ((hap_ramp->attack_length == 0) && (hap_ramp->fade_length == 0)) {
            SDL_free(dest->lpEnvelope);
            dest->lpEnvelope = NULL;
        } else {
            envelope->dwAttackLevel = CONVERT(hap_ramp->attack_level);
            envelope->dwAttackTime = hap_ramp->attack_length * 1000;
            envelope->dwFadeLevel = CONVERT(hap_ramp->fade_level);
            envelope->dwFadeTime = hap_ramp->fade_length * 1000;
        }

        break;

    case SDL_HAPTIC_CUSTOM:
        hap_custom = &src->custom;
        custom = SDL_malloc(sizeof(DICUSTOMFORCE));
        if (custom == NULL) {
            SDL_OutOfMemory();
            return -1;
        }
        SDL_memset(custom, 0, sizeof(DICUSTOMFORCE));

        /* Specifics */
        custom->cChannels = hap_custom->channels;
        custom->dwSamplePeriod = hap_custom->period * 1000;
        custom->cSamples = hap_custom->samples;
        custom->rglForceData =
            SDL_malloc(sizeof(LONG) * custom->cSamples * custom->cChannels);
        for (i = 0; i < hap_custom->samples * hap_custom->channels; i++) {      /* Copy data. */
            custom->rglForceData[i] = CONVERT(hap_custom->data[i]);
        }
        dest->cbTypeSpecificParams = sizeof(DICUSTOMFORCE);
        dest->lpvTypeSpecificParams = custom;

        /* Generics */
        dest->dwDuration = hap_custom->length * 1000;   /* In microseconds. */
        dest->dwTriggerButton = DIGetTriggerButton(hap_custom->button);
        dest->dwTriggerRepeatInterval = hap_custom->interval;
        dest->dwStartDelay = hap_custom->delay * 1000;  /* In microseconds. */

        /* Direction. */
        if (SDL_SYS_SetDirection(dest, &hap_custom->direction, dest->cAxes) <
            0) {
            return -1;
        }

        /* Envelope */
        if ((hap_custom->attack_length == 0)
            && (hap_custom->fade_length == 0)) {
            SDL_free(dest->lpEnvelope);
            dest->lpEnvelope = NULL;
        } else {
            envelope->dwAttackLevel = CONVERT(hap_custom->attack_level);
            envelope->dwAttackTime = hap_custom->attack_length * 1000;
            envelope->dwFadeLevel = CONVERT(hap_custom->fade_level);
            envelope->dwFadeTime = hap_custom->fade_length * 1000;
        }

        break;


    default:
        SDL_SetError("Haptic: Unknown effect type.");
        return -1;
    }

    return 0;
}


/*
 * Frees an DIEFFECT allocated by SDL_SYS_ToDIEFFECT.
 */
static void
SDL_SYS_HapticFreeDIEFFECT(DIEFFECT * effect, int type)
{
    DICUSTOMFORCE *custom;

    if (effect->lpEnvelope != NULL) {
        SDL_free(effect->lpEnvelope);
        effect->lpEnvelope = NULL;
    }
    if (effect->rgdwAxes != NULL) {
        SDL_free(effect->rgdwAxes);
        effect->rgdwAxes = NULL;
    }
    if (effect->lpvTypeSpecificParams != NULL) {
        if (type == SDL_HAPTIC_CUSTOM) {        /* Must free the custom data. */
            custom = (DICUSTOMFORCE *) effect->lpvTypeSpecificParams;
            SDL_free(custom->rglForceData);
            custom->rglForceData = NULL;
        }
        SDL_free(effect->lpvTypeSpecificParams);
        effect->lpvTypeSpecificParams = NULL;
    }
    if (effect->rglDirection != NULL) {
        SDL_free(effect->rglDirection);
        effect->rglDirection = NULL;
    }
}


/*
 * Gets the effect type from the generic SDL haptic effect wrapper.
 */
static REFGUID
SDL_SYS_HapticEffectType(SDL_HapticEffect * effect)
{
    switch (effect->type) {
    case SDL_HAPTIC_CONSTANT:
        return &GUID_ConstantForce;

    case SDL_HAPTIC_RAMP:
        return &GUID_RampForce;

    case SDL_HAPTIC_SQUARE:
        return &GUID_Square;

    case SDL_HAPTIC_SINE:
        return &GUID_Sine;

    case SDL_HAPTIC_TRIANGLE:
        return &GUID_Triangle;

    case SDL_HAPTIC_SAWTOOTHUP:
        return &GUID_SawtoothUp;

    case SDL_HAPTIC_SAWTOOTHDOWN:
        return &GUID_SawtoothDown;

    case SDL_HAPTIC_SPRING:
        return &GUID_Spring;

    case SDL_HAPTIC_DAMPER:
        return &GUID_Damper;

    case SDL_HAPTIC_INERTIA:
        return &GUID_Inertia;

    case SDL_HAPTIC_FRICTION:
        return &GUID_Friction;

    case SDL_HAPTIC_CUSTOM:
        return &GUID_CustomForce;

    default:
        SDL_SetError("Haptic: Unknown effect type.");
        return NULL;
    }
}


/*
 * Creates a new haptic effect.
 */
int
SDL_SYS_HapticNewEffect(SDL_Haptic * haptic, struct haptic_effect *effect,
                        SDL_HapticEffect * base)
{
    HRESULT ret;

    /* Get the type. */
    REFGUID type = SDL_SYS_HapticEffectType(base);
    if (type == NULL) {
        goto err_hweffect;
    }

    /* Alloc the effect. */
    effect->hweffect = (struct haptic_hweffect *)
        SDL_malloc(sizeof(struct haptic_hweffect));
    if (effect->hweffect == NULL) {
        SDL_OutOfMemory();
        goto err_hweffect;
    }

    /* Get the effect. */
    if (SDL_SYS_ToDIEFFECT(haptic, &effect->hweffect->effect, base) < 0) {
        goto err_effectdone;
    }

    /* Create the actual effect. */
    ret = IDirectInputDevice2_CreateEffect(haptic->hwdata->device, type,
                                           &effect->hweffect->effect,
                                           &effect->hweffect->ref, NULL);
    if (FAILED(ret)) {
        DI_SetError("Unable to create effect", ret);
        goto err_effectdone;
    }

    return 0;

  err_effectdone:
    SDL_SYS_HapticFreeDIEFFECT(&effect->hweffect->effect, base->type);
  err_hweffect:
    if (effect->hweffect != NULL) {
        SDL_free(effect->hweffect);
        effect->hweffect = NULL;
    }
    return -1;
}


/*
 * Updates an effect.
 */
int
SDL_SYS_HapticUpdateEffect(SDL_Haptic * haptic,
                           struct haptic_effect *effect,
                           SDL_HapticEffect * data)
{
    HRESULT ret;
    DWORD flags;
    DIEFFECT temp;

    /* Get the effect. */
    SDL_memset(&temp, 0, sizeof(DIEFFECT));
    if (SDL_SYS_ToDIEFFECT(haptic, &temp, data) < 0) {
        goto err_update;
    }

    /* Set the flags.  Might be worthwhile to diff temp with loaded effect and
     *  only change those parameters. */
    flags = DIEP_DIRECTION |
        DIEP_DURATION |
        DIEP_ENVELOPE |
        DIEP_STARTDELAY |
        DIEP_TRIGGERBUTTON |
        DIEP_TRIGGERREPEATINTERVAL | DIEP_TYPESPECIFICPARAMS;

    /* Create the actual effect. */
    ret =
        IDirectInputEffect_SetParameters(effect->hweffect->ref, &temp, flags);
    if (FAILED(ret)) {
        DI_SetError("Unable to update effect", ret);
        goto err_update;
    }

    /* Copy it over. */
    SDL_SYS_HapticFreeDIEFFECT(&effect->hweffect->effect, data->type);
    SDL_memcpy(&effect->hweffect->effect, &temp, sizeof(DIEFFECT));

    return 0;

  err_update:
    SDL_SYS_HapticFreeDIEFFECT(&temp, data->type);
    return -1;
}


/*
 * Runs an effect.
 */
int
SDL_SYS_HapticRunEffect(SDL_Haptic * haptic, struct haptic_effect *effect,
                        Uint32 iterations)
{
    HRESULT ret;
    DWORD iter;

    /* Check if it's infinite. */
    if (iterations == SDL_HAPTIC_INFINITY) {
        iter = INFINITE;
    } else
        iter = iterations;

    /* Run the effect. */
    ret = IDirectInputEffect_Start(effect->hweffect->ref, iter, 0);
    if (FAILED(ret)) {
        DI_SetError("Running the effect", ret);
        return -1;
    }

    return 0;
}


/*
 * Stops an effect.
 */
int
SDL_SYS_HapticStopEffect(SDL_Haptic * haptic, struct haptic_effect *effect)
{
    HRESULT ret;

    ret = IDirectInputEffect_Stop(effect->hweffect->ref);
    if (FAILED(ret)) {
        DI_SetError("Unable to stop effect", ret);
        return -1;
    }

    return 0;
}


/*
 * Frees the effect.
 */
void
SDL_SYS_HapticDestroyEffect(SDL_Haptic * haptic, struct haptic_effect *effect)
{
    HRESULT ret;

    ret = IDirectInputEffect_Unload(effect->hweffect->ref);
    if (FAILED(ret)) {
        DI_SetError("Removing effect from the device", ret);
    }
    SDL_SYS_HapticFreeDIEFFECT(&effect->hweffect->effect,
                               effect->effect.type);
    SDL_free(effect->hweffect);
    effect->hweffect = NULL;
}


/*
 * Gets the status of a haptic effect.
 */
int
SDL_SYS_HapticGetEffectStatus(SDL_Haptic * haptic,
                              struct haptic_effect *effect)
{
    HRESULT ret;
    DWORD status;

    ret = IDirectInputEffect_GetEffectStatus(effect->hweffect->ref, &status);
    if (FAILED(ret)) {
        DI_SetError("Getting effect status", ret);
        return -1;
    }

    if (status == 0)
        return SDL_FALSE;
    return SDL_TRUE;
}


/*
 * Sets the gain.
 */
int
SDL_SYS_HapticSetGain(SDL_Haptic * haptic, int gain)
{
    HRESULT ret;
    DIPROPDWORD dipdw;

    /* Create the weird structure thingy. */
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = gain * 100;  /* 0 to 10,000 */

    /* Try to set the autocenter. */
    ret = IDirectInputDevice2_SetProperty(haptic->hwdata->device,
                                          DIPROP_FFGAIN, &dipdw.diph);
    if (FAILED(ret)) {
        DI_SetError("Setting gain", ret);
        return -1;
    }

    return 0;
}


/*
 * Sets the autocentering.
 */
int
SDL_SYS_HapticSetAutocenter(SDL_Haptic * haptic, int autocenter)
{
    HRESULT ret;
    DIPROPDWORD dipdw;

    /* Create the weird structure thingy. */
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = (autocenter == 0) ? DIPROPAUTOCENTER_OFF :
        DIPROPAUTOCENTER_ON;

    /* Try to set the autocenter. */
    ret = IDirectInputDevice2_SetProperty(haptic->hwdata->device,
                                          DIPROP_AUTOCENTER, &dipdw.diph);
    if (FAILED(ret)) {
        DI_SetError("Setting autocenter", ret);
        return -1;
    }

    return 0;
}


/*
 * Pauses the device.
 */
int
SDL_SYS_HapticPause(SDL_Haptic * haptic)
{
    HRESULT ret;

    /* Pause the device. */
    ret = IDirectInputDevice2_SendForceFeedbackCommand(haptic->hwdata->device,
                                                       DISFFC_PAUSE);
    if (FAILED(ret)) {
        DI_SetError("Pausing the device", ret);
        return -1;
    }

    return 0;
}


/*
 * Pauses the device.
 */
int
SDL_SYS_HapticUnpause(SDL_Haptic * haptic)
{
    HRESULT ret;

    /* Unpause the device. */
    ret = IDirectInputDevice2_SendForceFeedbackCommand(haptic->hwdata->device,
                                                       DISFFC_CONTINUE);
    if (FAILED(ret)) {
        DI_SetError("Pausing the device", ret);
        return -1;
    }

    return 0;
}


/*
 * Stops all the playing effects on the device.
 */
int
SDL_SYS_HapticStopAll(SDL_Haptic * haptic)
{
    HRESULT ret;

    /* Try to stop the effects. */
    ret = IDirectInputDevice2_SendForceFeedbackCommand(haptic->hwdata->device,
                                                       DISFFC_STOPALL);
    if (FAILED(ret)) {
        DI_SetError("Stopping the device", ret);
        return -1;
    }

    return 0;
}


#endif /* SDL_HAPTIC_DINPUT */
