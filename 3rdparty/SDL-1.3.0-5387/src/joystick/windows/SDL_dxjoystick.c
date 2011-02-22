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

#ifdef SDL_JOYSTICK_DINPUT

/* DirectInput joystick driver; written by Glenn Maynard, based on Andrei de
 * A. Formiga's WINMM driver. 
 *
 * Hats and sliders are completely untested; the app I'm writing this for mostly
 * doesn't use them and I don't own any joysticks with them. 
 *
 * We don't bother to use event notification here.  It doesn't seem to work
 * with polled devices, and it's fine to call IDirectInputDevice2_GetDeviceData and
 * let it return 0 events. */

#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"
#define INITGUID /* Only set here, if set twice will cause mingw32 to break. */
#include "SDL_dxjoystick_c.h"


#ifndef DIDFT_OPTIONAL
#define DIDFT_OPTIONAL		0x80000000
#endif


#define INPUT_QSIZE	32      /* Buffer up to 32 input messages */
#define MAX_JOYSTICKS	8
#define AXIS_MIN	-32768  /* minimum value for axis coordinate */
#define AXIS_MAX	32767   /* maximum value for axis coordinate */
#define JOY_AXIS_THRESHOLD	(((AXIS_MAX)-(AXIS_MIN))/100)   /* 1% motion */

/* external variables referenced. */
extern HWND SDL_HelperWindow;


/* local variables */
static LPDIRECTINPUT dinput = NULL;
extern HRESULT(WINAPI * DInputCreate) (HINSTANCE hinst, DWORD dwVersion,
                                       LPDIRECTINPUT * ppDI,
                                       LPUNKNOWN punkOuter);
static DIDEVICEINSTANCE SYS_Joystick[MAX_JOYSTICKS];    /* array to hold joystick ID values */
static char *SYS_JoystickNames[MAX_JOYSTICKS];
static int SYS_NumJoysticks;
static HINSTANCE DInputDLL = NULL;


/* local prototypes */
static void SetDIerror(const char *function, HRESULT code);
static BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE *
                                           pdidInstance, VOID * pContext);
static BOOL CALLBACK EnumDevObjectsCallback(LPCDIDEVICEOBJECTINSTANCE dev,
                                            LPVOID pvRef);
static Uint8 TranslatePOV(DWORD value);
static int SDL_PrivateJoystickAxis_Int(SDL_Joystick * joystick, Uint8 axis,
                                       Sint16 value);
static int SDL_PrivateJoystickHat_Int(SDL_Joystick * joystick, Uint8 hat,
                                      Uint8 value);
static int SDL_PrivateJoystickButton_Int(SDL_Joystick * joystick,
                                         Uint8 button, Uint8 state);

/* Taken from Wine - Thanks! */
DIOBJECTDATAFORMAT dfDIJoystick2[] = {
  { &GUID_XAxis,DIJOFS_X,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_YAxis,DIJOFS_Y,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_ZAxis,DIJOFS_Z,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RxAxis,DIJOFS_RX,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RyAxis,DIJOFS_RY,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RzAxis,DIJOFS_RZ,DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Slider,DIJOFS_SLIDER(0),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Slider,DIJOFS_SLIDER(1),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_POV,DIJOFS_POV(0),DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE,0},
  { &GUID_POV,DIJOFS_POV(1),DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE,0},
  { &GUID_POV,DIJOFS_POV(2),DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE,0},
  { &GUID_POV,DIJOFS_POV(3),DIDFT_OPTIONAL|DIDFT_POV|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(0),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(1),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(2),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(3),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(4),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(5),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(6),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(7),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(8),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(9),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(10),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(11),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(12),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(13),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(14),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(15),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(16),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(17),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(18),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(19),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(20),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(21),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(22),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(23),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(24),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(25),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(26),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(27),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(28),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(29),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(30),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(31),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(32),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(33),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(34),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(35),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(36),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(37),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(38),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(39),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(40),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(41),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(42),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(43),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(44),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(45),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(46),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(47),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(48),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(49),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(50),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(51),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(52),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(53),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(54),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(55),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(56),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(57),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(58),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(59),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(60),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(61),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(62),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(63),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(64),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(65),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(66),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(67),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(68),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(69),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(70),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(71),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(72),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(73),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(74),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(75),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(76),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(77),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(78),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(79),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(80),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(81),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(82),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(83),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(84),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(85),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(86),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(87),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(88),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(89),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(90),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(91),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(92),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(93),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(94),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(95),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(96),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(97),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(98),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(99),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(100),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(101),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(102),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(103),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(104),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(105),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(106),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(107),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(108),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(109),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(110),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(111),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(112),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(113),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(114),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(115),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(116),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(117),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(118),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(119),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(120),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(121),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(122),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(123),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(124),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(125),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(126),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { NULL,DIJOFS_BUTTON(127),DIDFT_OPTIONAL|DIDFT_BUTTON|DIDFT_ANYINSTANCE,0},
  { &GUID_XAxis,FIELD_OFFSET(DIJOYSTATE2,lVX),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_YAxis,FIELD_OFFSET(DIJOYSTATE2,lVY),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_ZAxis,FIELD_OFFSET(DIJOYSTATE2,lVZ),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RxAxis,FIELD_OFFSET(DIJOYSTATE2,lVRx),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RyAxis,FIELD_OFFSET(DIJOYSTATE2,lVRy),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RzAxis,FIELD_OFFSET(DIJOYSTATE2,lVRz),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Slider,FIELD_OFFSET(DIJOYSTATE2,rglVSlider[0]),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Slider,FIELD_OFFSET(DIJOYSTATE2,rglVSlider[1]),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_XAxis,FIELD_OFFSET(DIJOYSTATE2,lAX),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_YAxis,FIELD_OFFSET(DIJOYSTATE2,lAY),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_ZAxis,FIELD_OFFSET(DIJOYSTATE2,lAZ),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RxAxis,FIELD_OFFSET(DIJOYSTATE2,lARx),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RyAxis,FIELD_OFFSET(DIJOYSTATE2,lARy),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RzAxis,FIELD_OFFSET(DIJOYSTATE2,lARz),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Slider,FIELD_OFFSET(DIJOYSTATE2,rglASlider[0]),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Slider,FIELD_OFFSET(DIJOYSTATE2,rglASlider[1]),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_XAxis,FIELD_OFFSET(DIJOYSTATE2,lFX),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_YAxis,FIELD_OFFSET(DIJOYSTATE2,lFY),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_ZAxis,FIELD_OFFSET(DIJOYSTATE2,lFZ),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RxAxis,FIELD_OFFSET(DIJOYSTATE2,lFRx),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RyAxis,FIELD_OFFSET(DIJOYSTATE2,lFRy),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_RzAxis,FIELD_OFFSET(DIJOYSTATE2,lFRz),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Slider,FIELD_OFFSET(DIJOYSTATE2,rglFSlider[0]),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
  { &GUID_Slider,FIELD_OFFSET(DIJOYSTATE2,rglFSlider[1]),DIDFT_OPTIONAL|DIDFT_AXIS|DIDFT_ANYINSTANCE,0},
};

const DIDATAFORMAT c_dfDIJoystick2 = {
    sizeof(DIDATAFORMAT),
    sizeof(DIOBJECTDATAFORMAT),
    DIDF_ABSAXIS,
    sizeof(DIJOYSTATE2),
    SDL_arraysize(dfDIJoystick2),
    dfDIJoystick2
};


/* Convert a DirectInput return code to a text message */
static void
SetDIerror(const char *function, HRESULT code)
{
    /*
    SDL_SetError("%s() [%s]: %s", function,
                 DXGetErrorString9A(code), DXGetErrorDescription9A(code));
     */
    SDL_SetError("%s() DirectX error %d", function, code);
}


/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int
SDL_SYS_JoystickInit(void)
{
    HRESULT result;
    HINSTANCE instance;

    SYS_NumJoysticks = 0;

    result = CoInitialize(NULL);
    if (FAILED(result)) {
        SetDIerror("CoInitialize", result);
        return (-1);
    }

    result = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER,
                              &IID_IDirectInput, (LPVOID)&dinput);

    if (FAILED(result)) {
        SetDIerror("CoCreateInstance", result);
        return (-1);
    }

    /* Because we used CoCreateInstance, we need to Initialize it, first. */
    instance = GetModuleHandle(NULL);
    if (instance == NULL) {
        SDL_SetError("GetModuleHandle() failed with error code %d.",
                     GetLastError());
        return (-1);
    }
    result = IDirectInput_Initialize(dinput, instance, DIRECTINPUT_VERSION);

    if (FAILED(result)) {
        SetDIerror("IDirectInput::Initialize", result);
        return (-1);
    }

    /* Look for joysticks, wheels, head trackers, gamepads, etc.. */
    result = IDirectInput_EnumDevices(dinput,
                                      DIDEVTYPE_JOYSTICK,
                                      EnumJoysticksCallback,
                                      NULL, DIEDFL_ATTACHEDONLY);

    return SYS_NumJoysticks;
}

static BOOL CALLBACK
EnumJoysticksCallback(const DIDEVICEINSTANCE * pdidInstance, VOID * pContext)
{
    SDL_memcpy(&SYS_Joystick[SYS_NumJoysticks], pdidInstance,
               sizeof(DIDEVICEINSTANCE));
    SYS_JoystickNames[SYS_NumJoysticks] = WIN_StringToUTF8(pdidInstance->tszProductName);
    SYS_NumJoysticks++;

    if (SYS_NumJoysticks >= MAX_JOYSTICKS)
        return DIENUM_STOP;

    return DIENUM_CONTINUE;
}

/* Function to get the device-dependent name of a joystick */
const char *
SDL_SYS_JoystickName(int index)
{
    return SYS_JoystickNames[index];
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int
SDL_SYS_JoystickOpen(SDL_Joystick * joystick)
{
    HRESULT result;
    LPDIRECTINPUTDEVICE device;
    DIPROPDWORD dipdw;

    SDL_memset(&dipdw, 0, sizeof(DIPROPDWORD));
    dipdw.diph.dwSize = sizeof(DIPROPDWORD);
    dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);


    /* allocate memory for system specific hardware data */
    joystick->hwdata =
        (struct joystick_hwdata *) SDL_malloc(sizeof(struct joystick_hwdata));
    if (joystick->hwdata == NULL) {
        SDL_OutOfMemory();
        return (-1);
    }
    SDL_memset(joystick->hwdata, 0, sizeof(struct joystick_hwdata));
    joystick->hwdata->buffered = 1;
    joystick->hwdata->Capabilities.dwSize = sizeof(DIDEVCAPS);

    result =
        IDirectInput_CreateDevice(dinput,
                                  &SYS_Joystick[joystick->index].
                                  guidInstance, &device, NULL);
    if (FAILED(result)) {
        SetDIerror("IDirectInput::CreateDevice", result);
        return (-1);
    }

    /* Now get the IDirectInputDevice2 interface, instead. */
    result = IDirectInputDevice_QueryInterface(device,
                                               &IID_IDirectInputDevice2,
                                               (LPVOID *) & joystick->
                                               hwdata->InputDevice);
    /* We are done with this object.  Use the stored one from now on. */
    IDirectInputDevice_Release(device);

    if (FAILED(result)) {
        SetDIerror("IDirectInputDevice::QueryInterface", result);
        return (-1);
    }

    /* Aquire shared access. Exclusive access is required for forces,
     * though. */
    result =
        IDirectInputDevice2_SetCooperativeLevel(joystick->hwdata->
                                                InputDevice, SDL_HelperWindow,
                                                DISCL_EXCLUSIVE |
                                                DISCL_BACKGROUND);
    if (FAILED(result)) {
        SetDIerror("IDirectInputDevice2::SetCooperativeLevel", result);
        return (-1);
    }

    /* Use the extended data structure: DIJOYSTATE2. */
    result =
        IDirectInputDevice2_SetDataFormat(joystick->hwdata->InputDevice,
                                          &c_dfDIJoystick2);
    if (FAILED(result)) {
        SetDIerror("IDirectInputDevice2::SetDataFormat", result);
        return (-1);
    }

    /* Get device capabilities */
    result =
        IDirectInputDevice2_GetCapabilities(joystick->hwdata->InputDevice,
                                            &joystick->hwdata->Capabilities);

    if (FAILED(result)) {
        SetDIerror("IDirectInputDevice2::GetCapabilities", result);
        return (-1);
    }

    /* Force capable? */
    if (joystick->hwdata->Capabilities.dwFlags & DIDC_FORCEFEEDBACK) {

        result = IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);

        if (FAILED(result)) {
            SetDIerror("IDirectInputDevice2::Acquire", result);
            return (-1);
        }

        /* reset all accuators. */
        result =
            IDirectInputDevice2_SendForceFeedbackCommand(joystick->hwdata->
                                                         InputDevice,
                                                         DISFFC_RESET);

        /* Not necessarily supported, ignore if not supported.
        if (FAILED(result)) {
            SetDIerror("IDirectInputDevice2::SendForceFeedbackCommand",
                       result);
            return (-1);
        }
        */

        result = IDirectInputDevice2_Unacquire(joystick->hwdata->InputDevice);

        if (FAILED(result)) {
            SetDIerror("IDirectInputDevice2::Unacquire", result);
            return (-1);
        }

        /* Turn on auto-centering for a ForceFeedback device (until told
         * otherwise). */
        dipdw.diph.dwObj = 0;
        dipdw.diph.dwHow = DIPH_DEVICE;
        dipdw.dwData = DIPROPAUTOCENTER_ON;

        result =
            IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
                                            DIPROP_AUTOCENTER, &dipdw.diph);

        /* Not necessarily supported, ignore if not supported.
        if (FAILED(result)) {
            SetDIerror("IDirectInputDevice2::SetProperty", result);
            return (-1);
        }
        */
    }

    /* What buttons and axes does it have? */
    IDirectInputDevice2_EnumObjects(joystick->hwdata->InputDevice,
                                    EnumDevObjectsCallback, joystick,
                                    DIDFT_BUTTON | DIDFT_AXIS | DIDFT_POV);

    dipdw.diph.dwObj = 0;
    dipdw.diph.dwHow = DIPH_DEVICE;
    dipdw.dwData = INPUT_QSIZE;

    /* Set the buffer size */
    result =
        IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
                                        DIPROP_BUFFERSIZE, &dipdw.diph);

    if (result == DI_POLLEDDEVICE) {
        /* This device doesn't support buffering, so we're forced
         * to use less reliable polling. */
        joystick->hwdata->buffered = 0;
    } else if (FAILED(result)) {
        SetDIerror("IDirectInputDevice2::SetProperty", result);
        return (-1);
    }

    return (0);
}

static BOOL CALLBACK
EnumDevObjectsCallback(LPCDIDEVICEOBJECTINSTANCE dev, LPVOID pvRef)
{
    SDL_Joystick *joystick = (SDL_Joystick *) pvRef;
    HRESULT result;
    input_t *in = &joystick->hwdata->Inputs[joystick->hwdata->NumInputs];

    in->ofs = dev->dwOfs;

    if (dev->dwType & DIDFT_BUTTON) {
        in->type = BUTTON;
        in->num = joystick->nbuttons;
        joystick->nbuttons++;
    } else if (dev->dwType & DIDFT_POV) {
        in->type = HAT;
        in->num = joystick->nhats;
        joystick->nhats++;
    } else if (dev->dwType & DIDFT_AXIS) {
        DIPROPRANGE diprg;
        DIPROPDWORD dilong;

        in->type = AXIS;
        in->num = joystick->naxes;

        diprg.diph.dwSize = sizeof(diprg);
        diprg.diph.dwHeaderSize = sizeof(diprg.diph);
        diprg.diph.dwObj = dev->dwOfs;
        diprg.diph.dwHow = DIPH_BYOFFSET;
        diprg.lMin = AXIS_MIN;
        diprg.lMax = AXIS_MAX;

        result =
            IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
                                            DIPROP_RANGE, &diprg.diph);
        if (FAILED(result)) {
            return DIENUM_CONTINUE;     /* don't use this axis */
        }

        /* Set dead zone to 0. */
        dilong.diph.dwSize = sizeof(dilong);
        dilong.diph.dwHeaderSize = sizeof(dilong.diph);
        dilong.diph.dwObj = dev->dwOfs;
        dilong.diph.dwHow = DIPH_BYOFFSET;
        dilong.dwData = 0;
        result =
            IDirectInputDevice2_SetProperty(joystick->hwdata->InputDevice,
                                            DIPROP_DEADZONE, &dilong.diph);
        if (FAILED(result)) {
            return DIENUM_CONTINUE;     /* don't use this axis */
        }

        joystick->naxes++;
    } else {
        /* not supported at this time */
        return DIENUM_CONTINUE;
    }

    joystick->hwdata->NumInputs++;

    if (joystick->hwdata->NumInputs == MAX_INPUTS) {
        return DIENUM_STOP;     /* too many */
    }

    return DIENUM_CONTINUE;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void
SDL_SYS_JoystickUpdate_Polled(SDL_Joystick * joystick)
{
    DIJOYSTATE2 state;
    HRESULT result;
    int i;

    result =
        IDirectInputDevice2_GetDeviceState(joystick->hwdata->InputDevice,
                                           sizeof(DIJOYSTATE2), &state);
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
        result =
            IDirectInputDevice2_GetDeviceState(joystick->hwdata->InputDevice,
                                               sizeof(DIJOYSTATE2), &state);
    }

    /* Set each known axis, button and POV. */
    for (i = 0; i < joystick->hwdata->NumInputs; ++i) {
        const input_t *in = &joystick->hwdata->Inputs[i];

        switch (in->type) {
        case AXIS:
            switch (in->ofs) {
            case DIJOFS_X:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lX);
                break;
            case DIJOFS_Y:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lY);
                break;
            case DIJOFS_Z:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lZ);
                break;
            case DIJOFS_RX:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lRx);
                break;
            case DIJOFS_RY:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lRy);
                break;
            case DIJOFS_RZ:
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.lRz);
                break;
            case DIJOFS_SLIDER(0):
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.rglSlider[0]);
                break;
            case DIJOFS_SLIDER(1):
                SDL_PrivateJoystickAxis_Int(joystick, in->num,
                                            (Sint16) state.rglSlider[1]);
                break;
            }

            break;

        case BUTTON:
            SDL_PrivateJoystickButton_Int(joystick, in->num,
                                          (Uint8) (state.
                                                   rgbButtons[in->ofs -
                                                              DIJOFS_BUTTON0]
                                                   ? SDL_PRESSED :
                                                   SDL_RELEASED));
            break;
        case HAT:
            {
                Uint8 pos = TranslatePOV(state.rgdwPOV[in->ofs -
                                                       DIJOFS_POV(0)]);
                SDL_PrivateJoystickHat_Int(joystick, in->num, pos);
                break;
            }
        }
    }
}

void
SDL_SYS_JoystickUpdate_Buffered(SDL_Joystick * joystick)
{
    int i;
    HRESULT result;
    DWORD numevents;
    DIDEVICEOBJECTDATA evtbuf[INPUT_QSIZE];

    numevents = INPUT_QSIZE;
    result =
        IDirectInputDevice2_GetDeviceData(joystick->hwdata->InputDevice,
                                          sizeof(DIDEVICEOBJECTDATA), evtbuf,
                                          &numevents, 0);
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
        result =
            IDirectInputDevice2_GetDeviceData(joystick->hwdata->InputDevice,
                                              sizeof(DIDEVICEOBJECTDATA),
                                              evtbuf, &numevents, 0);
    }

    /* Handle the events or punt */
    if (FAILED(result))
        return;

    for (i = 0; i < (int) numevents; ++i) {
        int j;

        for (j = 0; j < joystick->hwdata->NumInputs; ++j) {
            const input_t *in = &joystick->hwdata->Inputs[j];

            if (evtbuf[i].dwOfs != in->ofs)
                continue;

            switch (in->type) {
            case AXIS:
                SDL_PrivateJoystickAxis(joystick, in->num,
                                        (Sint16) evtbuf[i].dwData);
                break;
            case BUTTON:
                SDL_PrivateJoystickButton(joystick, in->num,
                                          (Uint8) (evtbuf[i].
                                                   dwData ? SDL_PRESSED :
                                                   SDL_RELEASED));
                break;
            case HAT:
                {
                    Uint8 pos = TranslatePOV(evtbuf[i].dwData);
                    SDL_PrivateJoystickHat(joystick, in->num, pos);
                }
            }
        }
    }
}


static Uint8
TranslatePOV(DWORD value)
{
    const int HAT_VALS[] = {
        SDL_HAT_UP,
        SDL_HAT_UP | SDL_HAT_RIGHT,
        SDL_HAT_RIGHT,
        SDL_HAT_DOWN | SDL_HAT_RIGHT,
        SDL_HAT_DOWN,
        SDL_HAT_DOWN | SDL_HAT_LEFT,
        SDL_HAT_LEFT,
        SDL_HAT_UP | SDL_HAT_LEFT
    };

    if (LOWORD(value) == 0xFFFF)
        return SDL_HAT_CENTERED;

    /* Round the value up: */
    value += 4500 / 2;
    value %= 36000;
    value /= 4500;

    if (value >= 8)
        return SDL_HAT_CENTERED;        /* shouldn't happen */

    return HAT_VALS[value];
}

/* SDL_PrivateJoystick* doesn't discard duplicate events, so we need to
 * do it. */
static int
SDL_PrivateJoystickAxis_Int(SDL_Joystick * joystick, Uint8 axis, Sint16 value)
{
    if (joystick->axes[axis] != value)
        return SDL_PrivateJoystickAxis(joystick, axis, value);
    return 0;
}

static int
SDL_PrivateJoystickHat_Int(SDL_Joystick * joystick, Uint8 hat, Uint8 value)
{
    if (joystick->hats[hat] != value)
        return SDL_PrivateJoystickHat(joystick, hat, value);
    return 0;
}

static int
SDL_PrivateJoystickButton_Int(SDL_Joystick * joystick, Uint8 button,
                              Uint8 state)
{
    if (joystick->buttons[button] != state)
        return SDL_PrivateJoystickButton(joystick, button, state);
    return 0;
}

void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
    HRESULT result;

    result = IDirectInputDevice2_Poll(joystick->hwdata->InputDevice);
    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        IDirectInputDevice2_Acquire(joystick->hwdata->InputDevice);
        IDirectInputDevice2_Poll(joystick->hwdata->InputDevice);
    }

    if (joystick->hwdata->buffered)
        SDL_SYS_JoystickUpdate_Buffered(joystick);
    else
        SDL_SYS_JoystickUpdate_Polled(joystick);
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
    IDirectInputDevice2_Unacquire(joystick->hwdata->InputDevice);
    IDirectInputDevice2_Release(joystick->hwdata->InputDevice);

    if (joystick->hwdata != NULL) {
        /* free system specific hardware data */
        SDL_free(joystick->hwdata);
    }
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
    int i;

    for (i = 0; i < SDL_arraysize(SYS_JoystickNames); ++i) {
        if (SYS_JoystickNames[i]) {
            SDL_free(SYS_JoystickNames[i]);
            SYS_JoystickNames[i] = NULL;
        }
    }

    IDirectInput_Release(dinput);
    dinput = NULL;
}

#endif /* SDL_JOYSTICK_DINPUT */

/* vi: set ts=4 sw=4 expandtab: */
